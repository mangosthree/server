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
 * @file UnitPower.cpp
 * @brief Unit power/resource accessors and modifiers split out of Unit.cpp.
 *
 * Pure file move (cohesion split): method bodies are byte-identical and the
 * declarations remain in Unit.h. No behaviour change.
 */

#include "Unit.h"
#include "Player.h"
#include "Creature.h"
#include "Pet.h"
#include "Group.h"
#include "DBCStores.h"

/**
 * @brief Looks up the power-array index used by a given class for a power type.
 *
 * @param powerId The power type to translate.
 * @param classId The player or creature class id.
 * @return The index within the unit's stored power array.
 */
uint32 Unit::GetPowerIndexByClass(Powers powerId, uint32 classId)
{
    MANGOS_ASSERT(powerId < MAX_POWERS);
    MANGOS_ASSERT(classId < MAX_CLASSES);

    return sChrClassXPowerTypesStore[classId][uint32(powerId)];
};

Powers Unit::GetPowerTypeByIndex(uint32 index, uint32 classId)
{
    MANGOS_ASSERT(index < MAX_STORED_POWERS);
    MANGOS_ASSERT(classId < MAX_CLASSES);

    return Powers(sChrClassXPowerIndexStore[classId][index]);
}

uint32 Unit::GetPower(Powers power) const
{
    if (power == POWER_HEALTH)
    {
        return GetHealth();
    }

    uint32 powerIndex = GetPowerIndex(power);
    if (powerIndex == INVALID_POWER_INDEX)
    {
        return 0;
    }

    return GetUInt32Value(UNIT_FIELD_POWER1 + powerIndex);
}

uint32 Unit::GetPowerByIndex(uint32 index) const
{
    MANGOS_ASSERT(index < MAX_STORED_POWERS);

    return GetUInt32Value(UNIT_FIELD_POWER1 + index);
}

uint32 Unit::GetMaxPower(Powers power) const
{
    if (power == POWER_HEALTH)
    {
        return GetMaxHealth();
    }

    uint32 powerIndex = GetPowerIndex(power);
    if (powerIndex == INVALID_POWER_INDEX)
    {
        return 0;
    }

    return GetUInt32Value(UNIT_FIELD_MAXPOWER1 + powerIndex);
}

uint32 Unit::GetMaxPowerByIndex(uint32 index) const
{
    MANGOS_ASSERT(index < MAX_STORED_POWERS);

    return GetUInt32Value(UNIT_FIELD_MAXPOWER1 + index);
}

void Unit::SetPower(Powers power, int32 val)
{
    if (power == POWER_HEALTH)
    {
        return SetHealth(val >= 0 ? val : 0);
    }

    uint32 powerIndex = GetPowerIndex(power);
    if (powerIndex == INVALID_POWER_INDEX)
    {
        return;
    }

    return SetPowerByIndex(powerIndex, val);
}

void Unit::SetPowerByIndex(uint32 powerIndex, int32 val)
{
    int32 maxPower = GetMaxPowerByIndex(powerIndex);
    if (val > maxPower)
    {
        val = maxPower;
    }

    if (val < 0)
    {
        val = 0;
    }

    if (GetPowerByIndex(powerIndex) == val)
    {
        return;
    }

    MANGOS_ASSERT(powerIndex < MAX_STORED_POWERS);
    SetInt32Value(UNIT_FIELD_POWER1 + powerIndex, val);

    Powers power = getPowerType(powerIndex);
    MANGOS_ASSERT(power != INVALID_POWER);

    if (IsInWorld())
    {
        WorldPacket data(SMSG_POWER_UPDATE);
        data << GetPackGUID();
        data << uint32(1); // iteration count
        // for (int i = 0; i < count; ++i)
        data << uint8(power);
        data << uint32(val);
        SendMessageToSet(&data, true);
    }

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (((Player*)this)->GetGroup())
        {
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_CUR_POWER);
        }
    }
    else if (((Creature*)this)->IsPet())
    {
        Pet* pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
            {
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_CUR_POWER);
            }
        }
    }

    // modifying holy power resets it's fade timer
    if (power == POWER_HOLY_POWER)
    {
        ResetHolyPowerRegenTimer();
    }
}

/**
 * @brief Sets a power maximum and clamps current power if necessary.
 *
 * @param power The power type to update.
 * @param val The new maximum value.
 */
void Unit::SetMaxPower(Powers power, int32 val)
{
    if (power == POWER_HEALTH)
    {
        return SetMaxHealth(val >= 0 ? val : 0);
    }

    uint32 powerIndex = GetPowerIndex(power);
    if (powerIndex == INVALID_POWER_INDEX)
    {
        return;
    }

    return SetMaxPowerByIndex(powerIndex, val);
}

void Unit::SetMaxPowerByIndex(uint32 powerIndex, int32 val)
{
    int32 cur_power = GetPowerByIndex(powerIndex);
    SetStatInt32Value(UNIT_FIELD_MAXPOWER1 + powerIndex, val);

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (((Player*)this)->GetGroup())
        {
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_MAX_POWER);
        }
    }
    else if (((Creature*)this)->IsPet())
    {
        Pet* pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
            {
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MAX_POWER);
            }
        }
    }

    if (val < cur_power)
    {
        SetPowerByIndex(powerIndex, val);
    }
}

/**
 * @brief Applies or removes a flat modifier to current power.
 *
 * @param power The power type to modify.
 * @param val The amount to apply or remove.
 * @param apply True to apply the modifier; false to remove it.
 */
void Unit::ApplyPowerMod(Powers power, uint32 val, bool apply)
{
    ApplyModUInt32Value(UNIT_FIELD_POWER1 + power, val, apply);

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (((Player*)this)->GetGroup())
        {
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_CUR_POWER);
        }
    }
    else if (((Creature*)this)->IsPet())
    {
        Pet* pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
            {
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_CUR_POWER);
            }
        }
    }
}

/**
 * @brief Applies or removes a flat modifier to maximum power.
 *
 * @param power The power type to modify.
 * @param val The amount to apply or remove.
 * @param apply True to apply the modifier; false to remove it.
 */
void Unit::ApplyMaxPowerMod(Powers power, uint32 val, bool apply)
{
    ApplyModUInt32Value(UNIT_FIELD_MAXPOWER1 + power, val, apply);

    // group update
    if (GetTypeId() == TYPEID_PLAYER)
    {
        if (((Player*)this)->GetGroup())
        {
            ((Player*)this)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_MAX_POWER);
        }
    }
    else if (((Creature*)this)->IsPet())
    {
        Pet* pet = ((Pet*)this);
        if (pet->isControlled())
        {
            Unit* owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && ((Player*)owner)->GetGroup())
            {
                ((Player*)owner)->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MAX_POWER);
            }
        }
    }
}

/**
 * @brief Registers or unregisters an aura in the proc-trigger-damage list.
 *
 * @param aura The aura to add or remove.
 * @param apply True to add the aura; false to remove it.
 */
void Unit::ApplyAuraProcTriggerDamage(Aura* aura, bool apply)
{
    AuraList& tAuraProcTriggerDamage = m_modAuras[SPELL_AURA_PROC_TRIGGER_DAMAGE];
    if (apply)
    {
        tAuraProcTriggerDamage.push_back(aura);
    }
    else
    {
        tAuraProcTriggerDamage.remove(aura);
    }
}

/**
 * @brief Gets the default base value for a power type.
 *
 * @param power The power type to query.
 * @return The created base power value.
 */
uint32 Unit::GetCreatePowers(Powers power) const
{
    switch (power)
    {
        case POWER_HEALTH:      return 0;                   // is it really should be here?
        case POWER_MANA:        return GetCreateMana();
        case POWER_RAGE:        return POWER_RAGE_DEFAULT;
        case POWER_FOCUS:
            if (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_HUNTER)
            {
                return POWER_FOCUS_DEFAULT;
            }
            return (GetTypeId() == TYPEID_PLAYER || !((Creature const*)this)->IsPet() || ((Pet const*)this)->getPetType() != HUNTER_PET ? 0 : POWER_FOCUS_DEFAULT);
        case POWER_ENERGY:      return POWER_ENERGY_DEFAULT;
        case POWER_RUNE:        return (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_DEATH_KNIGHT ? POWER_RUNE_DEFAULT : 0);
        case POWER_RUNIC_POWER: return (GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_DEATH_KNIGHT ? POWER_RUNIC_POWER_DEFAULT : 0);
        case POWER_SOUL_SHARDS: return 0;
        case POWER_ECLIPSE:     return 0;                   // TODO: fix me
        case POWER_HOLY_POWER:  return 0;
    }

    return 0;
}

uint32 Unit::GetCreateMaxPowers(Powers power) const
{
    switch (power)
    {
        case POWER_HOLY_POWER:
            return GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_PALADIN ? POWER_HOLY_POWER_DEFAULT : 0;
        case POWER_SOUL_SHARDS:
            return GetTypeId() == TYPEID_PLAYER && ((Player const*)this)->getClass() == CLASS_WARLOCK ? POWER_SOUL_SHARDS_DEFAULT : 0;
        default:
            return GetCreatePowers(power);
    }

    return 0;
}
