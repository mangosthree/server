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

#ifndef MANGOS_H_RUNEMGR
#define MANGOS_H_RUNEMGR

#include "Common.h"

class Player;
class Aura;

#define MAX_RUNES               6

enum RuneCooldowns
{
    RUNE_BASE_COOLDOWN          = 10000,
    RUNE_MISS_COOLDOWN          = 1500     // cooldown applied on runes when the spell misses
};

enum RuneType
{
    RUNE_BLOOD                  = 0,
    RUNE_UNHOLY                 = 1,
    RUNE_FROST                  = 2,
    RUNE_DEATH                  = 3,
    NUM_RUNE_TYPES              = 4
};

struct RuneInfo
{
    uint8  BaseRune;
    uint8  CurrentRune;
    uint16 BaseCooldown;
    uint16 Cooldown;                                        // msec
    Aura const* ConvertAura;
};

struct Runes
{
    RuneInfo runes[MAX_RUNES];
    uint8 runeState;                                        // mask of available runes
    uint32 lastUsedRuneMask;

    void SetRuneState(uint8 index, bool set = true)
    {
        if (set)
        {
            runeState |= (1 << index);                      // usable
        }
        else
        {
            runeState &= ~(1 << index);                     // on cooldown
        }
    }
};

/**
 * @brief Owns a death knight player's rune state and rune-system behaviour.
 *
 * Held by value on Player as m_runeMgr; rune state is runtime-only (rebuilt by
 * Init() on create/login, never persisted). All access requires the owner to be
 * a death knight - see callers' getClass() checks.
 */
class RuneMgr
{
    public:
        explicit RuneMgr(Player* owner) : m_owner(owner) {}

        uint8 GetRunesState() const { return m_data.runeState; }
        RuneType GetBaseRune(uint8 index) const { return RuneType(m_data.runes[index].BaseRune); }
        RuneType GetCurrentRune(uint8 index) const { return RuneType(m_data.runes[index].CurrentRune); }
        uint16 GetRuneCooldown(uint8 index) const { return m_data.runes[index].Cooldown; }
        uint16 GetBaseRuneCooldown(uint8 index) const { return m_data.runes[index].BaseCooldown; }
        uint8 GetRuneCooldownFraction(uint8 index) const;
        void UpdateRuneRegen(RuneType rune);
        void UpdateRuneRegen();
        bool IsBaseRuneSlotsOnCooldown(RuneType runeType) const;
        void ClearLastUsedRuneMask() { m_data.lastUsedRuneMask = 0; }
        bool IsLastUsedRune(uint8 index) const { return (m_data.lastUsedRuneMask & (1 << index)) != 0; }
        void SetLastUsedRune(RuneType type) { m_data.lastUsedRuneMask |= 1 << uint32(type); }
        void SetBaseRune(uint8 index, RuneType baseRune) { m_data.runes[index].BaseRune = baseRune; }
        void SetCurrentRune(uint8 index, RuneType currentRune) { m_data.runes[index].CurrentRune = currentRune; }
        void SetRuneCooldown(uint8 index, uint16 cooldown) { m_data.runes[index].Cooldown = cooldown; m_data.SetRuneState(index, (cooldown == 0) ? true : false); }
        void SetBaseRuneCooldown(uint8 index, uint16 cooldown) { m_data.runes[index].BaseCooldown = cooldown; }
        void SetRuneConvertAura(uint8 index, Aura const* aura) { m_data.runes[index].ConvertAura = aura; }
        void AddRuneByAuraEffect(uint8 index, RuneType newType, Aura const* aura);
        void RemoveRunesByAuraEffect(Aura const* aura);
        void RestoreBaseRune(uint8 index);
        void ConvertRune(uint8 index, RuneType newType);
        bool ActivateRunes(RuneType type, uint32 count);
        void ResyncRunes();
        void AddRunePower(uint8 index);
        void Init();

    private:
        Player* m_owner;
        Runes m_data;
};

#endif
