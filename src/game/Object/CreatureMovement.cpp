/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file CreatureMovement.cpp
 * @brief Cohesion split of Creature.cpp -- creature movement-flag state (walk/swim/fly/hover/levitate/feather-fall/root/water-walk).
 *
 * Same Creature class; no behaviour change. CMake file(GLOB Object/*.cpp)
 * picks this file up automatically; Creature.h is unchanged.
 */

#include "Creature.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "GridMap.h"

/**
 * @brief Enables or disables walk mode for the creature.
 *
 * @param enable true to walk; false to run.
 * @param asDefault true to also update the default running state.
 */
void Creature::SetWalk(bool enable, bool asDefault)
{
    if (asDefault)
    {
        if (enable)
        {
            clearUnitState(UNIT_STAT_RUNNING);
        }
        else
        {
            addUnitState(UNIT_STAT_RUNNING);
        }
    }

    // Nothing changed?
    if (enable == m_movementInfo.HasMovementFlag(MOVEFLAG_WALK_MODE))
    {
        return;
    }

    if (enable)
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_WALK_MODE);
    }
    else
    {
        m_movementInfo.RemoveMovementFlag(MOVEFLAG_WALK_MODE);
    }

    if (IsInWorld())
    {
        WorldPacket data(enable ? SMSG_SPLINE_MOVE_SET_WALK_MODE : SMSG_SPLINE_MOVE_SET_RUN_MODE, 9);
        if (enable)
        {
            data.WriteGuidMask<7, 6, 5, 1, 3, 4, 2, 0>(GetObjectGuid());
            data.WriteGuidBytes<4, 2, 1, 6, 5, 0, 7, 3>(GetObjectGuid());
        }
        else
        {
            data.WriteGuidMask<5, 6, 3, 7, 2, 0, 4, 1>(GetObjectGuid());
            data.WriteGuidBytes<7, 0, 4, 6, 5, 1, 2, 3>(GetObjectGuid());
        }

        SendMessageToSet(&data, true);
    }
}

/**
 * @brief Enables or disables levitation movement flags.
 *
 * @param enable true to levitate; false to clear the flag.
 */
void Creature::SetLevitate(bool enable)
{
    if (enable)
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_LEVITATING);
    }
    else
    {
        m_movementInfo.RemoveMovementFlag(MOVEFLAG_LEVITATING);
    }

    if (IsInWorld())
    {
        WorldPacket data(enable ? SMSG_SPLINE_MOVE_GRAVITY_DISABLE : SMSG_SPLINE_MOVE_GRAVITY_ENABLE, 9);
        if (enable)
        {
            data.WriteGuidMask<7, 3, 4, 2, 5, 1, 0, 6>(GetObjectGuid());
            data.WriteGuidBytes<7, 1, 3, 4, 6, 2, 5, 0>(GetObjectGuid());
        }
        else
        {
            data.WriteGuidMask<5, 4, 7, 1, 3, 6, 2, 0>(GetObjectGuid());
            data.WriteGuidBytes<7, 3, 4, 2, 1, 6, 0, 5>(GetObjectGuid());
        }

        SendMessageToSet(&data, true);
    }
}

/**
 * @brief Enables or disables swim movement flags and broadcasts the change.
 *
 * @param enable true to swim; false to stop swimming.
 */
void Creature::SetSwim(bool enable)
{
    if (enable == m_movementInfo.HasMovementFlag(MOVEFLAG_SWIMMING))
    {
        return;
    }

    if (enable)
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_SWIMMING);
    }
    else
    {
        m_movementInfo.RemoveMovementFlag(MOVEFLAG_SWIMMING);
    }

    if (IsInWorld())
    {
        WorldPacket data(enable ? SMSG_SPLINE_MOVE_START_SWIM : SMSG_SPLINE_MOVE_STOP_SWIM, 9);
        if (enable)
        {
            data.WriteGuidMask<1, 6, 0, 7, 3, 5, 2, 4>(GetObjectGuid());
            data.WriteGuidBytes<3, 7, 2, 5, 6, 4, 1, 0>(GetObjectGuid());
        }
        else
        {
            data.WriteGuidMask<4, 1, 5, 3, 0, 7, 2, 6>(GetObjectGuid());
            data.WriteGuidBytes<6, 0, 7, 2, 3, 1, 5, 4>(GetObjectGuid());
        }

        SendMessageToSet(&data, true);
    }
}

/**
 * @brief Whether this creature may swim instead of being clamped to the ground.
 */
bool Creature::CanSwim() const
{
    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CANT_SWIM))
    {
        return false;
    }

    if ((GetCreatureInfo()->InhabitType & INHABIT_WATER) || HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CAN_SWIM))
    {
        return true;
    }

    if (GetCreatureInfo()->ExtraFlags & CREATURE_FLAG_EXTRA_WALK_IN_WATER)
    {
        return false;
    }

    // a ground creature already in swim mode keeps it until the water is left
    if (IsSwimming())
    {
        return true;
    }

    // not relocated yet during Create(); position is not valid to probe
    if (!IsInWorld())
    {
        return false;
    }

    // the client default: a creature immersed in swim-deep liquid surface-swims
    GridMapLiquidData liquidData;
    return GetTerrain()->IsSwimmable(GetPositionX(), GetPositionY(), GetPositionZ(),
                                     GetObjectBoundingRadius(), &liquidData) &&
           GetPositionZ() < liquidData.level + 2.0f; // not on a bridge/ledge above the water body
}

/**
 * @brief Syncs MOVEFLAG_SWIMMING with the liquid at the current position.
 *
 * The spawn-time check in InitEntry only fires once; without this a shore
 * spawned creature pathing through deep water keeps its land movement
 * state (falling animation, run speed). Matches the swim half of TC's
 * Creature::UpdateMovementCapabilities.
 */
void Creature::UpdateSwimmingState()
{
    // client-moved creatures (possess) own their movement flags
    if (hasUnitState(UNIT_STAT_CONTROLLED))
    {
        return;
    }

    // explicit bottom-walkers keep their spawn-time movement flags
    if (GetCreatureInfo()->ExtraFlags & CREATURE_FLAG_EXTRA_WALK_IN_WATER)
    {
        return;
    }

    GridMapLiquidData liquidData;
    bool swimmable = !HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CANT_SWIM) &&
                     GetTerrain()->IsSwimmable(GetPositionX(), GetPositionY(), GetPositionZ(),
                                               GetObjectBoundingRadius(), &liquidData) &&
                     GetPositionZ() < liquidData.level + 2.0f; // not on a bridge/ledge above the water body

    SetSwim(swimmable);
}

/**
 * @brief Placeholder for enabling or disabling flight.
 *
 * @param enable Unused flight toggle.
 */
void Creature::SetCanFly(bool enable)
{
    if (enable)
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_CAN_FLY);
    }
    else
    {
        m_movementInfo.RemoveMovementFlag(MOVEFLAG_CAN_FLY);
    }

    if (IsInWorld())
    {
        WorldPacket data(enable ? SMSG_SPLINE_MOVE_SET_FLYING : SMSG_SPLINE_MOVE_UNSET_FLYING, 9);
        if (enable)
        {
            data.WriteGuidMask<0, 4, 1, 6, 7, 2, 3, 5>(GetObjectGuid());
            data.WriteGuidBytes<7, 0, 5, 6, 4, 1, 3, 2>(GetObjectGuid());
        }
        else
        {
            data.WriteGuidMask<5, 0, 4, 7, 2, 3, 1, 6>(GetObjectGuid());
            data.WriteGuidBytes<7, 2, 3, 4, 5, 1, 6, 0>(GetObjectGuid());
        }

        SendMessageToSet(&data, true);
    }
}

/**
 * @brief Enables or disables feather-fall movement behavior.
 *
 * @param enable true to enable feather fall; false to restore normal falling.
 */
void Creature::SetFeatherFall(bool enable)
{
    if (enable)
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_SAFE_FALL);
    }
    else
    {
        m_movementInfo.RemoveMovementFlag(MOVEFLAG_SAFE_FALL);
    }

    if (IsInWorld())
    {
        WorldPacket data(enable ? SMSG_SPLINE_MOVE_FEATHER_FALL : SMSG_SPLINE_MOVE_NORMAL_FALL, 9);
        if (enable)
        {
            data.WriteGuidMask<3, 2, 7, 5, 4, 6, 1, 0>(GetObjectGuid());
            data.WriteGuidBytes<1, 4, 7, 6, 2, 0, 5, 3>(GetObjectGuid());
        }
        else
        {
            data.WriteGuidMask<3, 5, 1, 0, 7, 6, 2, 4>(GetObjectGuid());
            data.WriteGuidBytes<7, 6, 2, 0, 5, 4, 3, 1>(GetObjectGuid());
        }

        SendMessageToSet(&data, true);
    }
}

/**
 * @brief Enables or disables hover movement behavior.
 *
 * @param enable true to hover; false to unset hover.
 */
void Creature::SetHover(bool enable)
{
    if (enable)
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_HOVER);
    }
    else
    {
        m_movementInfo.RemoveMovementFlag(MOVEFLAG_HOVER);
    }

    if (IsInWorld())
    {
        WorldPacket data(enable ? SMSG_SPLINE_MOVE_SET_HOVER : SMSG_SPLINE_MOVE_UNSET_HOVER, 9);
        if (enable)
        {
            data.WriteGuidMask<3, 7, 0, 1, 4, 6, 2, 5>(GetObjectGuid());
            data.WriteGuidBytes<2, 4, 3, 1, 7, 0, 5, 6>(GetObjectGuid());
        }
        else
        {
            data.WriteGuidMask<6, 7, 4, 0, 3, 1, 5, 2>(GetObjectGuid());
            data.WriteGuidBytes<4, 5, 3, 0, 2, 7, 6, 1>(GetObjectGuid());
        }

        SendMessageToSet(&data, false);
    }
}

/**
 * @brief Enables or disables root movement behavior.
 *
 * @param enable true to root; false to unroot.
 */
void Creature::SetRoot(bool enable)
{
    if (enable)
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_ROOT);
    }
    else
    {
        m_movementInfo.RemoveMovementFlag(MOVEFLAG_ROOT);
    }

    if (IsInWorld())
    {
        WorldPacket data(enable ? SMSG_SPLINE_MOVE_ROOT : SMSG_SPLINE_MOVE_UNROOT, 9);
        if (enable)
        {
            data.WriteGuidMask<5, 4, 6, 1, 3, 7, 2, 0>(GetObjectGuid());
            data.WriteGuidBytes<2, 1, 7, 3, 5, 0, 6, 4>(GetObjectGuid());
        }
        else
        {
            data.WriteGuidMask<0, 1, 6, 5, 3, 2, 7, 4>(GetObjectGuid());
            data.WriteGuidBytes<6, 3, 1, 5, 2, 0, 7, 4>(GetObjectGuid());
        }

        SendMessageToSet(&data, true);
    }
}

/**
 * @brief Enables or disables water-walking behavior.
 *
 * @param enable true to water walk; false to restore land walking.
 */
void Creature::SetWaterWalk(bool enable)
{
    if (enable)
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_WATERWALKING);
    }
    else
    {
        m_movementInfo.RemoveMovementFlag(MOVEFLAG_WATERWALKING);
    }

    if (IsInWorld())
    {
        WorldPacket data(enable ? SMSG_SPLINE_MOVE_WATER_WALK : SMSG_SPLINE_MOVE_LAND_WALK, 9);
        if (enable)
        {
            data.WriteGuidMask<6, 1, 4, 2, 3, 7, 5, 0>(GetObjectGuid());
            data.WriteGuidBytes<0, 6, 3, 7, 4, 2, 5, 1>(GetObjectGuid());
        }
        else
        {
            data.WriteGuidMask<5, 0, 4, 6, 7, 2, 3, 1>(GetObjectGuid());
            data.WriteGuidBytes<5, 7, 3, 4, 1, 2, 0, 6>(GetObjectGuid());
        }

        SendMessageToSet(&data, true);
    }
}
