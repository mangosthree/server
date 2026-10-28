/*
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/*
 * @file Vehicle.cpp
 * This file contains the code needed for CMaNGOS to support vehicles
 * Currently implemented
 * - Board to board a passenger onto a vehicle (includes checks)
 * - Unboard to unboard a passenger from the vehicle
 * - SwitchSeat to switch to another seat of the same vehicle
 * - CanBoard to check if a passenger can board a vehicle
 * - Internal helper to set the controlling and spells for a vehicle's seat
 * - Internal helper to control the available seats of a vehicle
 */

#include "Vehicle.h"
#include "Utilities/Errors.h"
#include "Platform/Define.h"
#include "SharedDefines.h"
#include "ObjectGuid.h"
#include "Log.h"
#include "Unit.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "ObjectMgr.h"
#include "SQLStorages.h"
#include "Util.h"
#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"
#include "MapManager.h"
#include "TemporarySummon.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /*ENABLE_ELUNA*/

void ObjectMgr::LoadVehicleAccessory()
{
    sVehicleAccessoryStorage.Load();

    sLog.outString(">> Loaded %u vehicle accessories", sVehicleAccessoryStorage.GetRecordCount());
    sLog.outString();

    // Check content
    for (SQLMultiStorage::SQLSIterator<VehicleAccessory> itr = sVehicleAccessoryStorage.getDataBegin<VehicleAccessory>(); itr < sVehicleAccessoryStorage.getDataEnd<VehicleAccessory>(); ++itr)
    {
        if (!sCreatureStorage.LookupEntry<CreatureInfo>(itr->vehicleEntry))
        {
            sLog.outErrorDb("Table `vehicle_accessory` has entry (vehicle entry: %u, seat %u, passenger %u) where vehicle_entry is invalid, skip vehicle.", itr->vehicleEntry, itr->seatId, itr->passengerEntry);
            sVehicleAccessoryStorage.EraseEntry(itr->vehicleEntry);
            continue;
        }
        if (!sCreatureStorage.LookupEntry<CreatureInfo>(itr->passengerEntry))
        {
            sLog.outErrorDb("Table `vehicle_accessory` has entry (vehicle entry: %u, seat %u, passenger %u) where accessory_entry is invalid, skip vehicle.", itr->vehicleEntry, itr->seatId, itr->passengerEntry);
            sVehicleAccessoryStorage.EraseEntry(itr->vehicleEntry);
            continue;
        }
        if (itr->seatId >= MAX_VEHICLE_SEAT)
        {
            sLog.outErrorDb("Table `vehicle_accessory` has entry (vehicle entry: %u, seat %u, passenger %u) where seat is invalid (must be between 0 and %u), skip vehicle.", itr->vehicleEntry, itr->seatId, itr->passengerEntry, MAX_VEHICLE_SEAT - 1);
            sVehicleAccessoryStorage.EraseEntry(itr->vehicleEntry);
            continue;
        }
    }
}

/*
 * Constructor of VehicleInfo
 *
 * @param owner         MUST be provided owner of the vehicle (type Unit)
 * @param vehicleEntry  MUST be provided dbc-entry of the vehicle
 * @param overwriteNpcEntry Use to overwrite the GetEntry() result for selecting associated passengers
 *
 * This function will initialise the VehicleInfo of the vehicle owner
 * Also the seat-map is created here
 */
/// Guesswork kept from the original code: what a summoned vehicle waits before
/// despawning once its controller has unboarded, unless a script says otherwise.
static const uint32 VEHICLE_DEFAULT_DESPAWN_DELAY = 1000;

/// How far to the side of the seat a passenger is set down on unboarding.
/// Measured on five retail exits in the 18019 Gilneas captures (both seats of
/// Greymane's horse and of Crowley's, and a stagecoach): every exit spline
/// ended 1.0 yard out from the seat, perpendicular to the vehicle's facing.
/// The side is the RIGHT - unless someone is already standing there. Krennan,
/// let off half a second after the rider had landed on the right, went left;
/// Crowley and his rider, ejected in the same tick, both went right.
static const float VEHICLE_EXIT_SIDE_STEP = 1.0f;

/// Anything standing closer than this to the right-hand spot claims it.
static const float VEHICLE_EXIT_SPOT_RADIUS = 0.75f;

VehicleInfo::VehicleInfo(Unit* owner, VehicleEntry const* vehicleEntry, uint32 overwriteNpcEntry) :
    m_vehicleEntry(vehicleEntry),
    m_creatureSeats(0),
    m_playerSeats(0),
    m_overwriteNpcEntry(overwriteNpcEntry),
    m_isInitialized(false),
    m_owner(owner),
    m_lastPose(owner->Where()),
    m_updatePositionsTimer(500),
    m_despawnDelay(VEHICLE_DEFAULT_DESPAWN_DELAY)
{
    MANGOS_ASSERT(vehicleEntry);
    MANGOS_ASSERT(m_owner);

    // Initial fill of available seats for the vehicle
    for (uint8 i = 0; i < MAX_VEHICLE_SEAT; ++i)
    {
        if (uint32 seatId = vehicleEntry->SeatID[i])
        {
            if (VehicleSeatEntry const* seatEntry = sVehicleSeatStore.LookupEntry(seatId))
            {
                m_vehicleSeats.insert(VehicleSeatMap::value_type(i, seatEntry));

                if (IsUsableSeatForCreature(seatEntry->Flags))
                {
                    m_creatureSeats |= 1 << i;
                }

                if (IsUsableSeatForPlayer(seatEntry->Flags, seatEntry->FlagsB))
                {
                    m_playerSeats |= 1 << i;
                }
            }
        }
    }
}

VehicleInfo::~VehicleInfo()
{
    ((Unit*)m_owner)->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);

    UnBoardAll();                                           // Script-boarded passengers hold pointers into this object

    RemoveAccessoriesFromMap();                             // Remove accessories (for example required with player vehicles)
}

/**
 * Ejects every remaining passenger. Aura-boarded riders normally leave with
 * their aura; this sweeps the rest - a passenger left seated past the
 * vehicle's death keeps a dangling TransportInfo and crashes on its next
 * use (found the hard way: SetVehicleId(0) while a rescued watchman still
 * rode the player).
 */
void VehicleInfo::UnBoardAll()
{
    while (!m_passengers.empty())
    {
        WorldObject* passenger = m_passengers.begin()->first;

        if (passenger->isType(TYPEMASK_UNIT))
        {
            UnBoard((Unit*)passenger, false);
        }
        else
        {
            UnBoardPassenger(passenger);
        }
    }
}

void VehicleInfo::Initialize()
{
    if (!m_overwriteNpcEntry)
    {
        m_overwriteNpcEntry = m_owner->GetEntry();
    }

    // Loading passengers (rough version only!)
    SQLMultiStorage::SQLMSIteratorBounds<VehicleAccessory> bounds = sVehicleAccessoryStorage.getBounds<VehicleAccessory>(m_overwriteNpcEntry);
    for (SQLMultiStorage::SQLMultiSIterator<VehicleAccessory> itr = bounds.first; itr != bounds.second; ++itr)
    {
        if (Creature* summoned = m_owner->SummonCreature(itr->passengerEntry, m_owner->Where().X(), m_owner->Where().Y(), m_owner->Where().Z(), 2 * m_owner->Where().Facing(), TEMPSPAWN_DEAD_DESPAWN, 0))
        {
            DEBUG_LOG("VehicleInfo(of %s)::Initialize: Load vehicle accessory %s onto seat %u", m_owner->GetGuidStr().c_str(), summoned->GetGuidStr().c_str(), itr->seatId);
            m_accessoryGuids.insert(summoned->GetObjectGuid());
            int32 basepoint0 = itr->seatId + 1;
            summoned->CastCustomSpell((Unit*)m_owner, SPELL_RIDE_VEHICLE_HARDCODED, &basepoint0, NULL, NULL, true);
#ifdef ENABLE_ELUNA
            if (Eluna* e = summoned->GetEluna())
            {
                e->OnInstallAccessory(this, summoned);
            }
#endif
        }
    }

    // Initialize movement limitations
    uint32 vehicleFlags = GetVehicleEntry()->Flags;
    Unit* pVehicle = (Unit*)m_owner;

    if (vehicleFlags & VEHICLE_FLAG_NO_STRAFE)
    {
        pVehicle->m_movementInfo.AddMovementFlags2(MOVEFLAG2_NO_STRAFE);
    }
    if (vehicleFlags & VEHICLE_FLAG_NO_JUMPING)
    {
        pVehicle->m_movementInfo.AddMovementFlags2(MOVEFLAG2_NO_JUMPING);
    }
    if (vehicleFlags & VEHICLE_FLAG_FULLSPEEDTURNING)
    {
        pVehicle->m_movementInfo.AddMovementFlags2(MOVEFLAG2_FULLSPEEDTURNING);
    }
    if (vehicleFlags & VEHICLE_FLAG_ALLOW_PITCHING)
    {
        pVehicle->m_movementInfo.AddMovementFlags2(MOVEFLAG2_ALLOW_PITCHING);
    }
    if (vehicleFlags & VEHICLE_FLAG_FULLSPEEDPITCHING)
    {
        pVehicle->m_movementInfo.AddMovementFlags2(MOVEFLAG2_FULLSPEEDPITCHING);
    }

    if (vehicleFlags & VEHICLE_FLAG_FIXED_POSITION)
    {
        pVehicle->SetRoot(true);
    }

    // Initialize power type based on DBC values (creatures only)
    if (pVehicle->GetTypeId() == TYPEID_UNIT)
    {
        if (PowerDisplayEntry const* powerEntry = sPowerDisplayStore.LookupEntry(GetVehicleEntry()->PowerDisplayID_0))
        {
            pVehicle->SetPowerType(Powers(powerEntry->power));
        }
    }

    m_isInitialized = true;

//#ifdef ENABLE_ELUNA
//    sEluna->OnInstall(this);
//#endif
}

/*
 * This function will board a passenger onto a vehicle
 *
 * @param passenger MUST be provided. This Unit will be boarded onto the vehicles (if it checks out)
 * @param seat      Seat to which the passenger will be boarded (if can, elsewise an alternative will be selected if possible)
 */
/* ******************************* Passengers, and their seats ************************* */

Geometry::Frame VehicleInfo::SeatFrame() const
{
    return Geometry::Frame::Deck(m_owner->GetObjectGuid().GetRawValue());
}

Geometry::Placement VehicleInfo::SeatPoseOf(Geometry::Vector3 const& worldPoint,
                                            float worldFacing) const
{
    // The vehicle's own basis turns a world delta into seat axes. The negated delta is
    // MaNGOS' stored convention and is kept exactly: every seat offset in the database is
    // expressed in it.
    const Geometry::Vector3 delta = worldPoint - m_owner->Where().Pos();
    const Geometry::Vector3 flat =
        m_owner->Where().Basis().rot.mul(Geometry::Vector3(-delta.x, -delta.y, 0.0f));

    Geometry::Placement seat;
    seat.EnterFrame(SeatFrame(), Geometry::Vector3(flat.x, flat.y, delta.z),
                    worldFacing - m_owner->Where().Facing());
    return seat;
}

bool VehicleInfo::HasOnBoard(WorldObject const* passenger) const
{
    MANGOS_ASSERT(passenger);

    // Down from the (possible) passenger until we reach our owner, or no passenger at all.
    while (passenger->IsBoarded())
    {
        if (passenger->GetTransportInfo()->GetTransport() == m_owner)
        {
            return true;
        }

        passenger = passenger->GetTransportInfo()->GetTransport();
    }

    return false;
}

void VehicleInfo::BoardPassenger(WorldObject* passenger, Geometry::Placement const& seatPose,
                                 uint8 seat)
{
    TransportInfo* transportInfo = new TransportInfo(passenger, this, seatPose, seat);

    m_passengers.insert(PassengerMap::value_type(passenger, transportInfo));

    // The passenger needs fast access to transportInfo
    passenger->SetTransportInfo(transportInfo);
}

void VehicleInfo::UnBoardPassenger(WorldObject* passenger)
{
    PassengerMap::iterator itr = m_passengers.find(passenger);

    if (itr == m_passengers.end())
    {
        return;
    }

    passenger->SetTransportInfo(NULL);

    delete itr->second;

    m_passengers.erase(itr);
}

TransportInfo::TransportInfo(WorldObject* owner, VehicleInfo* transport,
                             Geometry::Placement const& seatPose, uint8 seat) :
    m_owner(owner),
    m_transport(transport),
    m_seatPose(seatPose),
    m_seat(seat)
{
    MANGOS_ASSERT(owner && m_transport);
    SetSeatPose(seatPose);
}

WorldObject* TransportInfo::GetTransport() const
{
    return m_transport->GetOwner();
}

ObjectGuid TransportInfo::GetTransportGuid() const
{
    return m_transport->GetOwner()->GetObjectGuid();
}

bool TransportInfo::IsOnVehicle() const
{
    return m_transport->GetOwner()->GetTypeId() == TYPEID_PLAYER ||
           m_transport->GetOwner()->GetTypeId() == TYPEID_UNIT;
}

bool TransportInfo::HasOnBoard(WorldObject const* passenger) const
{
    return m_transport->HasOnBoard(passenger);
}

void TransportInfo::SetSeatPose(Geometry::Placement const& seatPose)
{
    m_seatPose = seatPose;

    // The extent is the passenger's, not the pose's: a placement that carries none makes
    // every reach test act as though the riders were points.
    m_seatPose.Resize(m_owner->Where().Extent());

    // The passenger's own placement is NOT touched here: the map derives grid
    // cells from it (Map::PlayerRelocation, Map::Remove), so it must always
    // hold a world-frame pose. The seat pose lives in m_seatPose and in the
    // movement info below; VehicleInfo::UpdateGlobalPositions refreshes the
    // world pose from it as the vehicle moves.

    // The create block a LATER observer receives carries this offset, so a rider that moved
    // and then had someone arrive would otherwise be drawn at the spot it boarded at.
    if (m_owner->GetTypeId() == TYPEID_UNIT || m_owner->GetTypeId() == TYPEID_PLAYER)
    {
        Unit* unit = static_cast<Unit*>(m_owner);
        if (!unit->m_movementInfo.GetTransportGuid().IsEmpty())
        {
            unit->m_movementInfo.SetTransportData(GetTransportGuid(), m_seatPose.X(),
                                                  m_seatPose.Y(), m_seatPose.Z(),
                                                  m_seatPose.Facing(), 0, m_seat);
        }
    }
}

void VehicleInfo::Board(Unit* passenger, uint8 seat)
{
    MANGOS_ASSERT(passenger);

    DEBUG_LOG("VehicleInfo(of %s)::Board: Try to board passenger %s to seat %u", m_owner->GetGuidStr().c_str(), passenger->GetGuidStr().c_str(), seat);

    // This check is also called in Spell::CheckCast()
    if (!CanBoard(passenger))
    {
        return;
    }

    // Use the planned seat only if the seat is valid, possible to choose and empty
    if (!IsSeatAvailableFor(passenger, seat))
        if (!GetUsableSeatFor(passenger, seat))
        {
            return;
        }

    VehicleSeatEntry const* seatEntry = GetSeatEntry(seat);
    MANGOS_ASSERT(seatEntry);

    // ToDo: Unboard passenger from a MOTransport when they are properly implemented
    /*if (TransportInfo* transportInfo = passenger->GetTransportInfo())
    {
        WorldObject* transporter = transportInfo->GetTransport();

        // Must be a MO transporter
        MANGOS_ASSERT(transporter->GetObjectGuid().IsMOTransport());

        ((Transport*)transporter)->UnBoardPassenger(passenger);
    }*/

    DEBUG_LOG("VehicleInfo::Board: Board passenger: %s to seat %u", passenger->GetGuidStr().c_str(), seat);

    // Calculate passengers local position
    float lx, ly, lz, lo;
    CalculateBoardingPositionOf(passenger->Where().X(), passenger->Where().Y(), passenger->Where().Z(), passenger->Where().Facing(), lx, ly, lz, lo);

    Geometry::Placement seatPose;
    seatPose.EnterFrame(SeatFrame(), Geometry::Vector3(lx, ly, lz), lo);
    BoardPassenger(passenger, seatPose, seat);

    // Set data for createobject packets
    // No ONTRANSPORT bit in 4.3.4: the transport guid in the movement block IS the flag.
    passenger->m_movementInfo.SetTransportData(m_owner->GetObjectGuid(), lx, ly, lz, lo, 0, seat);

    if (passenger->GetTypeId() == TYPEID_PLAYER)
    {
        Player* pPlayer = (Player*)passenger;
        pPlayer->RemovePet(PET_SAVE_AS_CURRENT);

        WorldPacket data(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA);
        pPlayer->GetSession()->SendPacket(&data);

        // SMSG_BREAK_TARGET (?)
    }

    if (!passenger->IsRooted())
    {
        passenger->SetRoot(true);
    }

    // Glide to the seat's own offset. Launch() takes a boarded unit's start
    // point from its seat pose, so the whole spline stays in the local frame
    Movement::MoveSplineInit init(*passenger);
    init.MoveTo(seatEntry->AttachmentOffset_0,
                seatEntry->AttachmentOffset_1,
                seatEntry->AttachmentOffset_2);
    // Seat yaw/pitch/roll is a model-attachment rotation the client applies
    // itself - not a facing, which has no pitch or roll - so this stays 0
    init.SetFacing(0.0f);
    init.SetBoardVehicle();
    init.Launch();

    // Apply passenger modifications
    ApplySeatMods(passenger, seatEntry->Flags);

    // Snap the passenger's world pose to the seat it now occupies. `Update`
    // re-derives world positions only after the owner has moved a yard from
    // `m_lastPose`, which starts at the owner's current pose - so boarding a
    // STATIONARY vehicle would otherwise never resync the rider at all. The
    // composed position is the boarding spot, so this is same-cell and does
    // no grid surgery in the normal case.
    UpdateGlobalPositionOf(passenger, lx, ly, lz, lo);

#ifdef ENABLE_ELUNA
    if (Eluna* e = passenger->GetEluna())
    {
        e->OnAddPassenger(this, passenger, seat);
    }
#endif
}

/*
 * This function will switch the seat of a passenger on the same vehicle
 *
 * @param passenger MUST be provided. This Unit will change its seat on the vehicle
 * @param seat      Seat to which the passenger will be switched
 */
void VehicleInfo::SwitchSeat(Unit* passenger, uint8 seat)
{
    MANGOS_ASSERT(passenger);

    DEBUG_LOG("VehicleInfo::SwitchSeat: passenger: %s try to switch to seat %u", passenger->GetGuidStr().c_str(), seat);

    // Switching seats is not possible
    if (m_vehicleEntry->Flags & VEHICLE_FLAG_DISABLE_SWITCH)
    {
        return;
    }

    PassengerMap::const_iterator itr = m_passengers.find(passenger);
    MANGOS_ASSERT(itr != m_passengers.end());

    // We are already boarded to this seat
    if (itr->second->GetTransportSeat() == seat)
    {
        return;
    }

    // Check if it's a valid seat
    if (!IsSeatAvailableFor(passenger, seat))
    {
        return;
    }

    VehicleSeatEntry const* seatEntry = GetSeatEntry(itr->second->GetTransportSeat());
    MANGOS_ASSERT(seatEntry);

    // Switching seats is only allowed if this flag is set
    if (~seatEntry->Flags & SEAT_FLAG_CAN_SWITCH)
    {
        return;
    }

    // Remove passenger modifications of the old seat
    RemoveSeatMods(passenger, seatEntry->Flags);

    // Set to new seat
    itr->second->SetTransportSeat(seat);

    // Get seatEntry of new seat
    seatEntry = GetSeatEntry(seat);
    MANGOS_ASSERT(seatEntry);

    // Glide to the new seat's own offset, in the vehicle's local frame
    Movement::MoveSplineInit init(*passenger);
    init.MoveTo(seatEntry->AttachmentOffset_0,
                seatEntry->AttachmentOffset_1,
                seatEntry->AttachmentOffset_2);
    // No facing is set: seat yaw/pitch/roll is a model-attachment rotation
    // the client applies itself, so the local orientation stays as it is
    // It seems that Seat switching is sent without SplineFlag BoardVehicle
    init.Launch();

    // Apply passenger modifications of the new seat
    ApplySeatMods(passenger, seatEntry->Flags);
}

/*
 * This function will Unboard a passenger
 *
 * @param passenger         MUST be provided. This Unit will be unboarded from the vehicle
 * @param changeVehicle     If set, the passenger is expected to be directly boarded to another vehicle,
 *                          and hence he will not be unboarded but only removed from this vehicle.
 */
/// Is some other unit already standing on the spot a passenger would step
/// off to? Passengers still aboard do not count, nor does the vehicle.
bool VehicleInfo::IsExitSpotTaken(Unit const* passenger, float x, float y) const
{
    std::list<Unit*> nearby;
    MaNGOS::AnyUnitInObjectRangeCheck check(m_owner, VEHICLE_EXIT_SIDE_STEP * 2 + 1.0f);
    MaNGOS::UnitListSearcher<MaNGOS::AnyUnitInObjectRangeCheck> searcher(nearby, check);
    Cell::VisitAllObjects(m_owner, searcher, VEHICLE_EXIT_SIDE_STEP * 2 + 1.0f);

    for (std::list<Unit*>::const_iterator itr = nearby.begin(); itr != nearby.end(); ++itr)
    {
        Unit* unit = *itr;

        if (unit == m_owner || unit == passenger || HasOnBoard(unit))
        {
            continue;
        }

        const float dx = unit->Where().X() - x;
        const float dy = unit->Where().Y() - y;

        if (dx * dx + dy * dy < VEHICLE_EXIT_SPOT_RADIUS * VEHICLE_EXIT_SPOT_RADIUS)
        {
            return true;
        }
    }

    return false;
}

void VehicleInfo::UnBoard(Unit* passenger, bool changeVehicle)
{
    MANGOS_ASSERT(passenger);

    DEBUG_LOG("VehicleInfo::Unboard: passenger: %s", passenger->GetGuidStr().c_str());

    PassengerMap::const_iterator itr = m_passengers.find(passenger);
    MANGOS_ASSERT(itr != m_passengers.end());

    VehicleSeatEntry const* seatEntry = GetSeatEntry(itr->second->GetTransportSeat());
    MANGOS_ASSERT(seatEntry);

    // Needed after the passenger record below is gone: where the seat was.
    const Geometry::Placement seatPose = itr->second->Seat();

    UnBoardPassenger(passenger);

    // Remove passenger modifications
    RemoveSeatMods(passenger, seatEntry->Flags);

    if (!changeVehicle)                                     // Send expected unboarding packages
    {
        // Update movementInfo
        passenger->m_movementInfo.ClearTransportData();

        if (passenger->GetTypeId() == TYPEID_PLAYER)
        {
            Player* pPlayer = (Player*)passenger;
            pPlayer->ResummonPetTemporaryUnSummonedIfAny();
            pPlayer->SetFallInformation(0, pPlayer->Where().Z());

            // SMSG_PET_DISMISS_SOUND (?)
        }

        if (passenger->IsRooted())
        {
            passenger->SetRoot(false);
        }

        // Set the passenger down BESIDE the seat rather than inside the vehicle:
        // the seat's world position, pushed one yard out perpendicular to the
        // vehicle's facing (right for even seats, left for odd), at the
        // vehicle's own ground height, facing the way the vehicle faces. That
        // is what the retail exit splines do - see VEHICLE_EXIT_SIDE_STEP.
        float gx, gy, gz, go;
        CalculateGlobalPositionOf(seatPose.X(), seatPose.Y(), seatPose.Z(),
                                  seatPose.Facing(), gx, gy, gz, go);

        const float facing = m_owner->Where().Facing();
        float ex = gx + cos(facing - M_PI_F / 2) * VEHICLE_EXIT_SIDE_STEP;
        float ey = gy + sin(facing - M_PI_F / 2) * VEHICLE_EXIT_SIDE_STEP;

        if (IsExitSpotTaken(passenger, ex, ey))
        {
            ex = gx + cos(facing + M_PI_F / 2) * VEHICLE_EXIT_SIDE_STEP;
            ey = gy + sin(facing + M_PI_F / 2) * VEHICLE_EXIT_SIDE_STEP;
        }

        Movement::MoveSplineInit init(*passenger);
        init.MoveTo(ex, ey, m_owner->Where().Z());
        init.SetFacing(facing);
        init.SetExitVehicle();
        init.Launch();

        // Only now does the rider get his own body back.
        //
        // Releasing the vehicle is half of it: the client picks its mover from
        // control updates alone, so told (vehicle, 0) and nothing else it ends
        // up controlling NOTHING, and a second later the vehicle despawns and
        // that slot points at a deleted object - not rooted, moverless, every
        // key dead. But the restoring update cannot be sent from
        // `RemoveSeatMods` either: that runs before the transport data above is
        // cleared and before the exit spline is away, so the client is still
        // holding him on a vehicle and quietly declines the mover. Both were
        // confirmed on the wire - the (player, 1) update went out and no
        // CMSG_SET_ACTIVE_MOVER ever came back. Sent here, the exit is already
        // complete and there is nothing left to decline.
        if (passenger->GetTypeId() == TYPEID_PLAYER &&
            (seatEntry->Flags & SEAT_FLAG_CAN_CONTROL))
        {
            ((Player*)passenger)->SetClientControl(passenger, 1);
        }

        // Despawn if passenger was accessory
        if (passenger->GetTypeId() == TYPEID_UNIT && m_accessoryGuids.find(passenger->GetObjectGuid()) != m_accessoryGuids.end())
        {
            Creature* cPassenger = static_cast<Creature*>(passenger);
            // TODO Same TODO as in VehicleInfo::RemoveAccessoriesFromMap
            cPassenger->ForcedDespawn(5000);
            m_accessoryGuids.erase(passenger->GetObjectGuid());
        }
    }
#ifdef ENABLE_ELUNA
    if (Eluna* e = passenger->GetEluna())
    {
        e->OnRemovePassenger(this, passenger);
    }
#endif

    // Some creature vehicles get despawned after passenger unboarding
    if (m_owner->GetTypeId() == TYPEID_UNIT)
    {
        // TODO: Guesswork, but seems to be fairly near correct
        // Only if the passenger was on control seat? Also depending on some flags
        if ((seatEntry->Flags & SEAT_FLAG_CAN_CONTROL) &&
                !(m_vehicleEntry->Flags & (VEHICLE_FLAG_UNK4 | VEHICLE_FLAG_UNK20)))
        {
            if (((Creature*)m_owner)->IsTemporarySummon())
            {
                ((Creature*)m_owner)->ForcedDespawn(m_despawnDelay);
            }
        }
    }
}

/*
 * This function will check if a passenger can be boarded
 *
 * @param passenger         Unit that attempts to board onto a vehicle
 */
bool VehicleInfo::CanBoard(Unit* passenger) const
{
    if (!passenger)
    {
        return false;
    }

    // Passenger is this vehicle
    if (passenger == m_owner)
    {
        return false;
    }

    // Passenger is already on this vehicle (in this case switching seats is required)
    if (passenger->IsBoarded() && passenger->GetTransportInfo()->GetTransport() == m_owner)
    {
        return false;
    }

    // Prevent circular boarding: passenger (could only be vehicle) must not have m_owner on board
    if (passenger->IsVehicle() && passenger->GetVehicleInfo()->HasOnBoard(m_owner))
    {
        return false;
    }

    // Check if we have at least one empty seat
    if (!GetEmptySeats())
    {
        return false;
    }

    // Passenger is already boarded
    if (m_passengers.find(passenger) != m_passengers.end())
    {
        return false;
    }

    // Check for empty player seats
    if (passenger->GetTypeId() == TYPEID_PLAYER)
    {
        return GetEmptySeatsMask() & m_playerSeats;
    }

    // Check for empty creature seats
    return GetEmptySeatsMask() & m_creatureSeats;
}

Unit* VehicleInfo::GetPassenger(uint8 seat) const
{
    for (PassengerMap::const_iterator itr = m_passengers.begin(); itr != m_passengers.end(); ++itr)
        if (itr->second->GetTransportSeat() == seat)
        {
            return (Unit*)itr->first;
        }

    return NULL;
}

// Helper function to undo the turning of the vehicle to calculate a relative position of the passenger when boarding
void VehicleInfo::CalculateBoardingPositionOf(float gx, float gy, float gz, float go, float& lx, float& ly, float& lz, float& lo) const
{
    const Geometry::Placement seat = SeatPoseOf(Geometry::Vector3(gx, gy, gz), go);
    lx = seat.X();
    ly = seat.Y();
    lz = seat.Z();
    lo = seat.Facing();
}

void VehicleInfo::Update(uint32 diff)
{
    if (m_updatePositionsTimer < diff)
    {
        if ((m_owner->Where().Pos() - m_lastPose.Pos()).magnitude() > 1.0f ||
                Geometry::Placement::NormalizeOrientation(
                    m_owner->Where().Facing() - m_lastPose.Facing()) > 0.01f)
        {
            UpdateGlobalPositions();
        }

        m_updatePositionsTimer = 500;
    }
    else
    {
        m_updatePositionsTimer -= diff;
    }
}

void VehicleInfo::UpdateGlobalPositions()
{
    for (PassengerMap::const_iterator itr = m_passengers.begin(); itr != m_passengers.end(); ++itr)
    {
        Geometry::Placement const& seat = itr->second->Seat();
        UpdateGlobalPositionOf(itr->first, seat.X(), seat.Y(), seat.Z(), seat.Facing());
    }

    m_lastPose = m_owner->Where();
}

void VehicleInfo::UpdateGlobalPositionOf(WorldObject* passenger, float lx, float ly, float lz, float lo) const
{
    float gx, gy, gz, go;
    CalculateGlobalPositionOf(lx, ly, lz, lo, gx, gy, gz, go);

    // Map relocation derives the passenger's OLD cell from its placement; a
    // pose in any frame but the owner's would name a bogus cell (a seat pose
    // reads as the map origin, whose grid is typically not even loaded, and
    // the removal walk would dereference NULL). Must not happen for a vehicle
    // passenger - repair the pose in place, skip the grid walk, and complain.
    if (passenger->Where().CurrentFrame() != m_owner->Where().CurrentFrame())
    {
        sLog.outError("VehicleInfo::UpdateGlobalPositionOf: %s placement is "
                      "not in %s's world frame; repairing without relocation.",
                      passenger->GetGuidStr().c_str(),
                      m_owner->GetGuidStr().c_str());
        passenger->Place().Rebase(m_owner->Where().CurrentFrame());
        passenger->Place().MoveTo(gx, gy, gz, go);
        return;
    }

    if (passenger->GetTypeId() == TYPEID_PLAYER)
    {
        m_owner->GetMap()->PlayerRelocation((Player*)passenger, gx, gy, gz, go);
    }
    else if (passenger->GetTypeId() == TYPEID_UNIT)
    {
        m_owner->GetMap()->CreatureRelocation((Creature*)passenger, gx, gy, gz, go);
    }
    else
    {
        return;
    }

    if (((Unit*)passenger)->IsVehicle())
    {
        ((Unit*)passenger)->GetVehicleInfo()->UpdateGlobalPositions();
    }
}

void VehicleInfo::CalculateGlobalPositionOf(float lx, float ly, float lz, float lo, float& gx, float& gy, float& gz, float& go) const
{
    // A seat offset carried into the world by the vehicle's own basis -- the one place a
    // local-to-world composition IS legitimate, because a vehicle is a grid unit whose
    // pose the server actually owns (unlike a global transport's hull).
    const Geometry::Vector3 world =
        m_owner->Where().Basis().localToWorld(Geometry::Vector3(lx, ly, lz));
    gx = world.x;
    gy = world.y;
    gz = world.z;
    go = Geometry::Placement::NormalizeOrientation(lo + m_owner->Where().Facing());
}

void VehicleInfo::RemoveAccessoriesFromMap()
{
    // Remove all accessories
    for (GuidSet::const_iterator itr = m_accessoryGuids.begin(); itr != m_accessoryGuids.end(); ++itr)
    {
        if (Creature* pAccessory = m_owner->GetMap()->GetCreature(*itr))
        {
            // TODO - unclear how long to despawn, also maybe some flag etc depending
            pAccessory->ForcedDespawn(5000);
        }
    }
    m_accessoryGuids.clear();
    m_isInitialized = false;
}

/* ************************************************************************************************
 *          Helper function for seat control
 * ***********************************************************************************************/

/// Get the Vehicle SeatEntry of a seat by position
VehicleSeatEntry const* VehicleInfo::GetSeatEntry(uint8 seat) const
{
    VehicleSeatMap::const_iterator itr = m_vehicleSeats.find(seat);
    return itr != m_vehicleSeats.end() ? itr->second : NULL;
}

/*
 * This function will get a usable seat for a passenger
 *
 * @param passenger         MUST be provided. Unit for which to try to get a free seat
 * @param seat              will contain an available seat if returned true
 * @return                  return TRUE if and only if an available seat was found. In this case @seat will contain the id
 */
bool VehicleInfo::GetUsableSeatFor(Unit* passenger, uint8& seat) const
{
    MANGOS_ASSERT(passenger);

    uint8 possibleSeats = (passenger->GetTypeId() == TYPEID_PLAYER) ? (GetEmptySeatsMask() & m_playerSeats) : (GetEmptySeatsMask() & m_creatureSeats);

    // No usable seats available
    if (!possibleSeats)
    {
        return false;
    }

    // Start with 0
    seat = 0;

    for (uint8 i = 1; seat < MAX_VEHICLE_SEAT; i <<= 1, ++seat)
        if (possibleSeats & i)
        {
            return true;
        }

    return false;
}

/// Returns if a @passenger could board onto @seat - @passenger MUST be provided
bool VehicleInfo::IsSeatAvailableFor(Unit* passenger, uint8 seat) const
{
    MANGOS_ASSERT(passenger);

    return seat < MAX_VEHICLE_SEAT &&
           (GetEmptySeatsMask() & (passenger->GetTypeId() == TYPEID_PLAYER ? m_playerSeats : m_creatureSeats) & (1 << seat));
}

/// Wrapper to collect all taken seats
uint8 VehicleInfo::GetTakenSeatsMask() const
{
    uint8 takenSeatsMask = 0;

    for (PassengerMap::const_iterator itr = m_passengers.begin(); itr != m_passengers.end(); ++itr)
    {
        takenSeatsMask |= 1 << itr->second->GetTransportSeat();
    }

    return takenSeatsMask;
}

bool VehicleInfo:: IsUsableSeatForPlayer(uint32 seatFlags, uint32 seatFlagsB) const
{
    return seatFlags & SEAT_FLAG_CAN_EXIT ||
           seatFlags & SEAT_FLAG_UNCONTROLLED ||
           seatFlagsB &
           (SEAT_FLAG_B_USABLE_FORCED   | SEAT_FLAG_B_USABLE_FORCED_2 |
            SEAT_FLAG_B_USABLE_FORCED_3 | SEAT_FLAG_B_USABLE_FORCED_4);
}

/// Add control and such modifiers to a passenger if required
void VehicleInfo::ApplySeatMods(Unit* passenger, uint32 seatFlags)
{
    Unit* pVehicle = (Unit*)m_owner;                        // Vehicles are alawys Unit

    if (seatFlags & SEAT_FLAG_NOT_SELECTABLE)
    {
        passenger->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }

    if (passenger->GetTypeId() == TYPEID_PLAYER)
    {
        Player* pPlayer = (Player*)passenger;

        // group update
        if (pPlayer->GetGroup())
        {
            pPlayer->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_VEHICLE_SEAT);
        }

        if (seatFlags & SEAT_FLAG_CAN_CONTROL)
        {
            pPlayer->GetCamera().SetView(pVehicle);

            pPlayer->SetCharm(pVehicle);
            pVehicle->SetCharmerGuid(pPlayer->GetObjectGuid());

            pVehicle->addUnitState(UNIT_STAT_CONTROLLED);
            pVehicle->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

            pPlayer->SetClientControl(pVehicle, 1);
            pPlayer->SetMover(pVehicle);

            // Unconfirmed - default speed handling
            if (pVehicle->GetTypeId() == TYPEID_UNIT)
            {
                if (!pPlayer->IsWalking() && pVehicle->IsWalking())
                {
                    ((Creature*)pVehicle)->SetWalk(false, true);
                }
                else if (pPlayer->IsWalking() && !pVehicle->IsWalking())
                {
                    ((Creature*)pVehicle)->SetWalk(true, true);
                }

                // set vehicle faction as per the controller faction
                ((Creature*)pVehicle)->SetFactionTemporary(pPlayer->getFaction(), TEMPFACTION_NONE);
            }
        }

        if (seatFlags & SEAT_FLAG_CAN_CAST)
        {
            CharmInfo* charmInfo = pVehicle->InitCharmInfo(pVehicle);
            charmInfo->InitVehicleCreateSpells();

            pPlayer->PossessSpellInitialize();
        }
    }
    else if (passenger->GetTypeId() == TYPEID_UNIT)
    {
        if (seatFlags & SEAT_FLAG_CAN_CONTROL)
        {
            passenger->SetCharm(pVehicle);
            pVehicle->SetCharmerGuid(passenger->GetObjectGuid());
        }

        ((Creature*)passenger)->AI()->SetCombatMovement(false);
        // Not entirely sure how this must be handled in relation to CONTROL
        // But in any way this at least would require some changes in the movement system most likely
        passenger->GetMotionMaster()->Clear(false, true);
        passenger->GetMotionMaster()->MoveIdle();
    }
}

/// Remove control and such modifiers to a passenger if they were added
void VehicleInfo::RemoveSeatMods(Unit* passenger, uint32 seatFlags)
{
    Unit* pVehicle = (Unit*)m_owner;

    if (seatFlags & SEAT_FLAG_NOT_SELECTABLE)
    {
        passenger->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }

    if (passenger->GetTypeId() == TYPEID_PLAYER)
    {
        Player* pPlayer = (Player*)passenger;

        // group update
        if (pPlayer->GetGroup())
        {
            pPlayer->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_VEHICLE_SEAT);
        }

        if (seatFlags & SEAT_FLAG_CAN_CONTROL)
        {
            pPlayer->SetCharm(NULL);
            pVehicle->SetCharmerGuid(ObjectGuid());

            pPlayer->SetClientControl(pVehicle, 0);
            pPlayer->SetMover(NULL);

            // Control of his own body is NOT handed back here - `UnBoard` does
            // it once the exit is complete. See the note there.

            pVehicle->clearUnitState(UNIT_STAT_CONTROLLED);
            pVehicle->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

            // must be called after movement control unapplying
            pPlayer->GetCamera().ResetView();

            // reset vehicle faction
            if (pVehicle->GetTypeId() == TYPEID_UNIT)
            {
                ((Creature*)pVehicle)->ClearTemporaryFaction();
            }
        }

        if (seatFlags & SEAT_FLAG_CAN_CAST)
        {
            pPlayer->RemovePetActionBar();
        }
    }
    else if (passenger->GetTypeId() == TYPEID_UNIT)
    {
        if (seatFlags & SEAT_FLAG_CAN_CONTROL)
        {
            passenger->SetCharm(NULL);
            pVehicle->SetCharmerGuid(ObjectGuid());
        }

        // Reinitialize movement
        ((Creature*)passenger)->AI()->SetCombatMovement(true, true);
        if (!passenger->getVictim())
        {
            passenger->GetMotionMaster()->Initialize();
        }
    }
}
