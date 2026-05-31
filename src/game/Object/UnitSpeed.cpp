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

#include "Unit.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "SpellMgr.h"
#include "QuestDef.h"
#include "Player.h"
#include "Creature.h"
#include "Spell.h"
#include "Group.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"
#include "Formulas.h"
#include "Pet.h"
#include "Util.h"
#include "Totem.h"
#include "Vehicle.h"
#include "BattleGround/BattleGround.h"
#include "InstanceData.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "MapPersistentStateMgr.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "VMapFactory.h"
#include "MovementGenerator.h"
#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"
#include "CreatureLinkingMgr.h"
#include "GameTime.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#include "ElunaConfig.h"
#include "ElunaEventMgr.h"
#endif /* ENABLE_ELUNA */

#include <math.h>
#include <stdarg.h>

// Base movement-speed table; defined in Unit.cpp.
extern float baseMoveSpeed[MAX_MOVE_TYPE];

/**
 * @brief Recalculates movement speed for a move type from active modifiers.
 *
 * @param mtype The movement type to update.
 * @param forced True to send a forced speed change packet to players.
 * @param ratio Additional multiplier applied after recalculation.
 */
void Unit::UpdateSpeed(UnitMoveType mtype, bool forced, float ratio, bool ignoreChange)
{
    // not in combat pet have same speed as owner
    switch (mtype)
    {
        case MOVE_RUN:
        case MOVE_WALK:
        case MOVE_SWIM:
            if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet() && hasUnitState(UNIT_STAT_FOLLOW))
            {
                if (Unit* owner = GetOwner())
                {
                    SetSpeedRate(mtype, owner->GetSpeedRate(mtype), forced, ignoreChange);
                    return;
                }
            }
            break;
        default:
            break;
    }

    int32 main_speed_mod  = 0;
    float stack_bonus     = 1.0f;
    float non_stack_bonus = 1.0f;

    switch (mtype)
    {
        case MOVE_WALK:
            break;
        case MOVE_RUN:
        {
            if (IsMounted()) // Use on mount auras
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK)) / 100.0f;
            }
            else
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_SPEED_ALWAYS);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_SPEED_NOT_STACK)) / 100.0f;
            }
            break;
        }
        case MOVE_RUN_BACK:
            return;
        case MOVE_SWIM:
        {
            main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_INCREASE_SWIM_SPEED);
            break;
        }
        case MOVE_SWIM_BACK:
            return;
        case MOVE_FLIGHT:
        {
            if (IsMounted()) // Use on mount auras
            {
                main_speed_mod  = GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_STACKING);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED_NOT_STACKING)) / 100.0f;
            }
            else             // Use not mount (shapeshift for example) auras (should stack)
            {
                main_speed_mod  = GetTotalAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED);
                stack_bonus     = GetTotalAuraMultiplier(SPELL_AURA_MOD_FLIGHT_SPEED_STACKING);
                non_stack_bonus = (100.0f + GetMaxPositiveAuraModifier(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACKING)) / 100.0f;
            }
            break;
        }
        case MOVE_FLIGHT_BACK:
            return;
        default:
            sLog.outError("Unit::UpdateSpeed: Unsupported move type (%d)", mtype);
            return;
    }

    float bonus = non_stack_bonus > stack_bonus ? non_stack_bonus : stack_bonus;
    // now we ready for speed calculation
    float speed  = main_speed_mod ? bonus * (100.0f + main_speed_mod) / 100.0f : bonus;

    switch (mtype)
    {
        case MOVE_RUN:
        case MOVE_SWIM:
        case MOVE_FLIGHT:
        {
            // Normalize speed by 191 aura SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED if need
            // TODO: possible affect only on MOVE_RUN
            if (int32 normalization = GetMaxPositiveAuraModifier(SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED))
            {
                // Use speed from aura
                float max_speed = normalization / baseMoveSpeed[mtype];
                if (speed > max_speed)
                {
                    speed = max_speed;
                }
            }
            break;
        }
        default:
            break;
    }

    // for creature case, we check explicit if mob searched for assistance
    if (GetTypeId() == TYPEID_UNIT)
    {
        if (((Creature*)this)->HasSearchedAssistance())
        {
            speed *= 0.66f;                                  // best guessed value, so this will be 33% reduction. Based off initial speed, mob can then "run", "walk fast" or "walk".
        }
    }
    // for player case, we look for some custom rates
    else
    {
        if (GetDeathState() == CORPSE)
        {
            speed *= sWorld.getConfig(((Player*)this)->InBattleGround() ? CONFIG_FLOAT_GHOST_RUN_SPEED_BG : CONFIG_FLOAT_GHOST_RUN_SPEED_WORLD);
        }
    }

    // Apply strongest slow aura mod to speed
    int32 slow = GetMaxNegativeAuraModifier(SPELL_AURA_MOD_DECREASE_SPEED);
    if (slow)
    {
        speed *= (100.0f + slow) / 100.0f;
        float min_speed = (float)GetMaxPositiveAuraModifier(SPELL_AURA_MOD_MINIMUM_SPEED) / 100.0f;
        if (speed < min_speed)
        {
            speed = min_speed;
        }
    }

    if (GetTypeId() == TYPEID_UNIT)
    {
        switch (mtype)
        {
            case MOVE_RUN:
                speed *= ((Creature*)this)->GetCreatureInfo()->SpeedRun;
                break;
            case MOVE_WALK:
                speed *= ((Creature*)this)->GetCreatureInfo()->SpeedWalk;
                break;
            default:
                break;
        }
    }

    SetSpeedRate(mtype, speed * ratio, forced, ignoreChange);
}

/**
 * @brief Gets the current movement speed for a move type.
 *
 * @param mtype The movement type.
 * @return The resulting speed value.
 */
float Unit::GetSpeed(UnitMoveType mtype) const
{
    return m_speed_rate[mtype] * baseMoveSpeed[mtype];
}

struct SetSpeedRateHelper
{
    explicit SetSpeedRateHelper(UnitMoveType _mtype, bool _forced, bool _ignoreChange) : mtype(_mtype), forced(_forced), ignoreChange(_ignoreChange) {}
    void operator()(Unit* unit) const { unit->UpdateSpeed(mtype, forced, 1.0f, ignoreChange); }
    UnitMoveType mtype;
    bool forced, ignoreChange;
};

/**
 * @brief Sets the speed rate for a move type and broadcasts the change.
 *
 * @param mtype The movement type.
 * @param rate The new speed rate.
 * @param forced True to use forced speed change handling for players.
 */
void Unit::SetSpeedRate(UnitMoveType mtype, float rate, bool forced, bool ignoreChange)
{
    if (rate < 0)
    {
        rate = 0.0f;
    }

    // Update speed only on change
    if (m_speed_rate[mtype] != rate || ignoreChange)
    {
        m_speed_rate[mtype] = rate;

        PropagateSpeedChange();

        WorldPacket data;
        ObjectGuid guid = GetObjectGuid();

        if (forced && GetTypeId() == TYPEID_PLAYER)
        {
            // register forced speed changes for WorldSession::HandleForceSpeedChangeAck
            // and do it only for real sent packets and use run for run/mounted as client expected
            ++((Player*)this)->m_forced_speed_changes[mtype];
            switch (mtype)
            {
                case MOVE_WALK:
                {
                    data.Initialize(SMSG_MOVE_SET_WALK_SPEED, 1 + 8 + 4 + 4);
                    data.WriteGuidMask<0, 4, 5, 2, 3, 1, 6, 7>(guid);
                    data.WriteGuidBytes<6, 1, 5>(guid);
                    data << float(GetSpeed(mtype));
                    data.WriteGuidBytes<2>(guid);
                    data << uint32(0);
                    data.WriteGuidBytes<4, 0, 7, 3>(guid);
                    break;
                }
                case MOVE_RUN:
                {
                    data.Initialize(SMSG_MOVE_SET_RUN_SPEED, 1 + 8 + 4 + 4 );
                    data.WriteGuidMask<6, 1, 5, 2, 7, 0, 3, 4>(guid);
                    data.WriteGuidBytes<5, 3, 1, 4>(guid);
                    data << uint32(0);
                    data << float(GetSpeed(mtype));
                    data.WriteGuidBytes<6, 0, 7, 2>(guid);
                    break;
                }
                case MOVE_RUN_BACK:
                {
                    data.Initialize(SMSG_MOVE_SET_RUN_BACK_SPEED, 1 + 8 + 4 + 4 );
                    data.WriteGuidMask<0, 6, 2, 1, 3, 5, 4, 7>(guid);
                    data.WriteGuidBytes<5>(guid);
                    data << uint32(0);
                    data << float(GetSpeed(mtype));
                    data.WriteGuidBytes<0, 4, 7, 3, 1, 2, 6>(guid);
                    break;
                }
                case MOVE_SWIM:
                {
                    data.Initialize(SMSG_MOVE_SET_SWIM_SPEED, 1 + 8 + 4 + 4 );
                    data.WriteGuidMask<5, 4, 7, 3, 2, 0, 1, 6>(guid);
                    data.WriteGuidBytes<0>(guid);
                    data << uint32(0);
                    data.WriteGuidBytes<6, 3, 5, 2>(guid);
                    data << float(GetSpeed(mtype));
                    data.WriteGuidBytes<1, 7, 4>(guid);
                    break;
                }
                case MOVE_SWIM_BACK:
                {
                    data.Initialize(SMSG_MOVE_SET_SWIM_BACK_SPEED, 1 + 8 + 4 + 4 );
                    data.WriteGuidMask<4, 2, 3, 6, 5, 1, 0, 7>(guid);
                    data << uint32(0);
                    data.WriteGuidBytes<0, 3, 4, 6, 5, 1>(guid);
                    data << float(GetSpeed(mtype));
                    data.WriteGuidBytes<0, 7>(guid);
                    break;
                }
                case MOVE_TURN_RATE:
                {
                    data.Initialize(SMSG_MOVE_SET_TURN_RATE, 1 + 8 + 4 + 4 );
                    data.WriteGuidMask<7, 2, 1, 0, 4, 5, 6, 3>(guid);
                    data.WriteGuidBytes<5, 7, 2>(guid);
                    data << float(GetSpeed(mtype));
                    data.WriteGuidBytes<3, 1, 0>(guid);
                    data << uint32(0);
                    data.WriteGuidBytes<6, 4>(guid);
                    break;
                }
                case MOVE_FLIGHT:
                {
                    data.Initialize(SMSG_MOVE_SET_FLIGHT_SPEED, 1 + 8 + 4 + 4 );
                    data.WriteGuidMask<0, 5, 1, 6, 3, 2, 7, 4>(guid);
                    data.WriteGuidBytes<0, 1, 7, 5>(guid);
                    data << float(GetSpeed(mtype));
                    data << uint32(0);
                    data.WriteGuidBytes<2, 6, 3, 4>(guid);
                    break;
                }
                case MOVE_FLIGHT_BACK:
                {
                    data.Initialize(SMSG_MOVE_SET_FLIGHT_BACK_SPEED, 1 + 8 + 4 + 4 );
                    data.WriteGuidMask<1, 2, 6, 4, 7, 3, 0, 5>(guid);

                    data.WriteGuidBytes<3>(guid);
                    data << uint32(0);
                    data.WriteGuidBytes<6>(guid);
                    data << float(GetSpeed(mtype));
                    data.WriteGuidBytes<1, 2, 4, 0, 5, 7>(guid);
                    break;
                }
                case MOVE_PITCH_RATE:
                {
                    data.Initialize(SMSG_MOVE_SET_PITCH_RATE, 1 + 8 + 4 + 4 );
                    data.WriteGuidMask<1, 2, 6, 7, 0, 3, 5, 4>(guid);

                    data << float(GetSpeed(mtype));
                    data.WriteGuidBytes<6, 4, 0>(guid);
                    data << uint32(0);
                    data.WriteGuidBytes<1, 2, 7, 3, 5>(guid);
                    break;
                }
                default:
                    sLog.outError("Unit::SetSpeed: Unsupported move type (%d), data not sent to client.", mtype);
                    return;
            }

            ((Player*)this)->GetSession()->SendPacket(&data);
        }

        m_movementInfo.UpdateTime(GameTime::GetGameTimeMS());

        // TODO: Actually such opcodes should (always?) be packed with SMSG_COMPRESSED_MOVES
        switch (mtype)
        {
            case MOVE_WALK:
            {
                data.Initialize(SMSG_SPLINE_MOVE_SET_WALK_SPEED, 1 + 8 + 4);
                data.WriteGuidMask<0, 6, 7, 3, 5, 1, 2, 4>(guid);
                data.WriteGuidBytes<0, 4, 7, 1, 5, 3>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<6, 2>(guid);
                break;
            }
            case MOVE_RUN:
            {
                data.Initialize(SMSG_SPLINE_MOVE_SET_RUN_SPEED, 1 + 8 + 4);
                data.WriteGuidMask<4, 0, 5, 7, 6, 3, 1, 2>(guid);
                data.WriteGuidBytes<0, 7, 6, 5, 3, 4>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<2, 1>(guid);
                break;
            }
            case MOVE_RUN_BACK:
            {
                data.Initialize(SMSG_SPLINE_MOVE_SET_RUN_BACK_SPEED, 1 + 8 + 4);
                data.WriteGuidMask<1, 2, 6, 0, 3, 7, 5, 4>(guid);
                data.WriteGuidBytes<1>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<2, 4, 0, 3, 6, 5, 7>(guid);
                break;
            }
            case MOVE_SWIM:
            {
                data.Initialize(SMSG_SPLINE_MOVE_SET_SWIM_SPEED, 1 + 8 + 4);
                data.WriteGuidMask<4, 2, 5, 0, 7, 6, 3, 1>(guid);
                data.WriteGuidBytes<5, 6, 1, 0, 2, 4>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<7, 3>(guid);
                break;
            }
            case MOVE_SWIM_BACK:
            {
                data.Initialize(SMSG_SPLINE_MOVE_SET_SWIM_BACK_SPEED, 1 + 8 + 4);
                data.WriteGuidMask<0, 1, 3, 6, 4, 5, 7, 2>(guid);
                data.WriteGuidBytes<5, 3, 1, 0, 7, 6>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<4, 2>(guid);
                break;
            }
            case MOVE_TURN_RATE:
            {
                data.Initialize(SMSG_SPLINE_MOVE_SET_TURN_RATE, 1 + 8 + 4);
                data.WriteGuidMask<2, 4, 6, 1, 3, 5, 7, 0>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<1, 5, 3, 2, 7, 4, 6, 0>(guid);
                break;
            }
            case MOVE_FLIGHT:
            {
                data.Initialize(SMSG_SPLINE_MOVE_SET_FLIGHT_SPEED, 1 + 8 + 4);
                data.WriteGuidMask<7, 4, 0, 1, 3, 6, 5, 2>(guid);
                data.WriteGuidBytes<0, 5, 4, 7, 3, 2, 1, 6>(guid);
                data << float(GetSpeed(mtype));
                break;
            }
            case MOVE_FLIGHT_BACK:
            {
                data.Initialize(SMSG_SPLINE_MOVE_SET_FLIGHT_BACK_SPEED, 1 + 8 + 4);
                data.WriteGuidMask<2, 1, 6, 5, 0, 3, 4, 7>(guid);
                data.WriteGuidBytes<5>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<6, 1, 0, 2, 3, 7, 4>(guid);
                break;
            }
            case MOVE_PITCH_RATE:
            {
                data.Initialize(SMSG_SPLINE_MOVE_SET_PITCH_RATE, 1 + 8 + 4);
                data.WriteGuidMask<3, 5, 6, 1, 0, 4, 7, 2>(guid);
                data.WriteGuidBytes<1, 5, 7, 0, 6, 3, 2>(guid);
                data << float(GetSpeed(mtype));
                data.WriteGuidBytes<4>(guid);
                break;
            }
            default:
                sLog.outError("Unit::SetSpeed: Unsupported move type (%d), data not sent to client.", mtype);
                return;
        }

        SendMessageToSet(&data, false);
    }

    CallForAllControlledUnits(SetSpeedRateHelper(mtype, forced, ignoreChange), CONTROLLED_PET | CONTROLLED_GUARDIANS | CONTROLLED_CHARM | CONTROLLED_MINIPET);
}

/**
 * @brief Applies or removes the feared state.
 *
 * @param apply True to apply fear; false to remove it.
 * @param casterGuid The caster responsible for the effect.
 * @param spellID The spell that caused the effect.
 * @param time The remaining flee duration.
 */
void Unit::SetFeared(bool apply, ObjectGuid casterGuid, uint32 spellID, uint32 time)
{
    if (apply)
    {
        if (HasAuraType(SPELL_AURA_PREVENTS_FLEEING))
        {
            return;
        }

        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_FLEEING);

        GetMotionMaster()->MovementExpired(false);
        CastStop(GetObjectGuid() == casterGuid ? spellID : 0);

        Unit* caster = IsInWorld() ?  GetMap()->GetUnit(casterGuid) : NULL;

        GetMotionMaster()->MoveFleeing(caster, time);       // caster==NULL processed in MoveFleeing
    }
    else
    {
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_FLEEING);

        GetMotionMaster()->MovementExpired(false);

        if (GetTypeId() != TYPEID_PLAYER && IsAlive())
        {
            Creature* c = ((Creature*)this);
            // restore appropriate movement generator
            if (getVictim())
            {
                GetMotionMaster()->MoveChase(getVictim());
            }
            else
            {
                GetMotionMaster()->Initialize();
            }

            // attack caster if can
            if (Unit* caster = IsInWorld() ? GetMap()->GetUnit(casterGuid) : NULL)
            {
                c->AttackedBy(caster);
            }
        }
    }

    if (GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)this)->SetClientControl(this, !apply);
    }
}

/**
 * @brief Applies or removes the confused state.
 *
 * @param apply True to apply confusion; false to remove it.
 * @param casterGuid The caster responsible for the effect.
 * @param spellID The spell that caused the effect.
 */
void Unit::SetConfused(bool apply, ObjectGuid casterGuid, uint32 spellID)
{
    if (apply)
    {
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);

        CastStop(GetObjectGuid() == casterGuid ? spellID : 0);

         if (GetTypeId() == TYPEID_UNIT)
         {
             SetTargetGuid(ObjectGuid());
             GetMotionMaster()->MoveConfused();
         }
    }
    else
    {
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_CONFUSED);

        GetMotionMaster()->MovementExpired(false);

        if (GetTypeId() != TYPEID_PLAYER && IsAlive())
        {
            // restore appropriate movement generator
            if (getVictim())
            {
                GetMotionMaster()->MoveChase(getVictim());
            }
            else
            {
                GetMotionMaster()->Initialize();
            }
        }
    }

    if (GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)this)->SetClientControl(this, !apply);
    }
}

/**
 * @brief Applies or removes feign death state handling.
 *
 * @param apply True to enable feign death; false to clear it.
 * @param casterGuid The caster responsible for the effect.
 */
void Unit::SetFeignDeath(bool apply, ObjectGuid casterGuid, uint32 /*spellID*/)
{
    if (apply)
    {
        /*
        WorldPacket data(SMSG_FEIGN_DEATH_RESISTED, 9);
        data<<GetGUID();
        data<<uint8(0);
        SendMessageToSet(&data,true);
        */

        if (GetTypeId() != TYPEID_PLAYER)
        {
            StopMoving();
        }
        else
        {
            ((Player*)this)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
        }

        // blizz like 2.0.x
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
        // blizz like 2.0.x
        SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
        // blizz like 2.0.x
        SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

        addUnitState(UNIT_STAT_DIED);
        CombatStop();
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // prevent interrupt message
        if (casterGuid == GetObjectGuid())
        {
            FinishSpell(CURRENT_GENERIC_SPELL, false);
        }
        InterruptNonMeleeSpells(true);
        GetHostileRefManager().deleteReferences();
    }
    else
    {
        /*
        WorldPacket data(SMSG_FEIGN_DEATH_RESISTED, 9);
        data<<GetGUID();
        data<<uint8(1);
        SendMessageToSet(&data,true);
        */
        // blizz like 2.0.x
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
        // blizz like 2.0.x
        RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
        // blizz like 2.0.x
        RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

        clearUnitState(UNIT_STAT_DIED);

        if (GetTypeId() != TYPEID_PLAYER && IsAlive())
        {
            // restore appropriate movement generator
            if (getVictim())
            {
                GetMotionMaster()->MoveChase(getVictim());
            }
            else
            {
                GetMotionMaster()->Initialize();
            }
        }
    }
}
