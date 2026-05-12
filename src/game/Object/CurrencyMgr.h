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

#ifndef MANGOS_H_CURRENCYMGR
#define MANGOS_H_CURRENCYMGR

#include "Common.h"
#include <unordered_map>

class Player;
class QueryResult;
struct CurrencyTypesEntry;

/**
 * Per-currency flags persisted in character_currencies.flags. Cata client
 * only uses two: SHOW_IN_BACKPACK toggles whether the currency appears in
 * the on-screen tracker, UNUSED is reserved.
 */
enum PlayerCurrencyFlag
{
    PLAYERCURRENCY_FLAG_NONE                = 0x0,
    PLAYERCURRENCY_FLAG_UNK1                = 0x1,  // unused?
    PLAYERCURRENCY_FLAG_UNK2                = 0x2,  // unused?
    PLAYERCURRENCY_FLAG_SHOW_IN_BACKPACK    = 0x4,
    PLAYERCURRENCY_FLAG_UNUSED              = 0x8,

    PLAYERCURRENCY_MASK_USED_BY_CLIENT =
        PLAYERCURRENCY_FLAG_SHOW_IN_BACKPACK |
        PLAYERCURRENCY_FLAG_UNUSED,
};

/**
 * Per-currency-row dirty state. Drives whether Save emits INSERT, UPDATE,
 * DELETE, or nothing.
 */
enum PlayerCurrencyState
{
    PLAYERCURRENCY_UNCHANGED        = 0,
    PLAYERCURRENCY_CHANGED          = 1,
    PLAYERCURRENCY_NEW              = 2,
    PLAYERCURRENCY_REMOVED          = 3
};

/**
 * Per-currency state held in the map keyed by currency-id.
 * currencyEntry caches the DBC row to avoid repeated lookups; it must be
 * populated whenever a new entry is added to the map.
 */
struct PlayerCurrency
{
    PlayerCurrencyState state;
    uint32 totalCount;
    uint32 weekCount;
    uint32 seasonCount;
    uint8 flags;
    CurrencyTypesEntry const* currencyEntry;
};

typedef std::unordered_map<uint32, PlayerCurrency> PlayerCurrenciesMap;

/**
 * CurrencyMgr — owns a Player's Cata-era currency map (honor points,
 * conquest points, justice, valor, plus per-meta categories) and the
 * lifecycle that touches it: Get / Modify / Set, week & season caps,
 * SMSG packet emission, weekly reset, and DB persistence to
 * character_currencies. Extracted from Player.cpp on 2026-05-12
 * alongside GlyphMgr and HonorMgr, following the same pattern (state
 * owned here, public API preserved as thin inline delegates on Player,
 * owner back-pointer for callbacks into Player / Unit / WorldSession).
 *
 * What stays on Player and is NOT in CurrencyMgr:
 *  - BuyCurrencyFromVendorSlot: vendor-system integration, calls
 *    ModifyCount via the Player API.
 *  - CurrencyAddedQuestCheck / CurrencyRemovedQuestCheck: quest-tracker
 *    integration. ModifyCount calls these through m_owner so the quest
 *    side-effects fire at the right moment.
 *  - SendNotifyLootItemRemoved(_, bool currency): loot system, the
 *    "currency" param is a loot-type indicator, not a currency method.
 *  - HasCurrencyCount / HasCurrencySeasonCount: inline convenience
 *    methods that just call the count getters; trivial to keep inline
 *    on Player.
 */
class CurrencyMgr
{
    public:
        explicit CurrencyMgr(Player* owner) : m_owner(owner) {}

        // Totals
        uint32 GetCount(uint32 id) const;
        uint32 GetSeasonCount(uint32 id) const;
        uint32 GetWeekCount(uint32 id) const;

        // Caps. Take a CurrencyTypesEntry rather than an id so callers
        // already holding the DBC entry don't re-look it up.
        uint32 GetWeekCap(CurrencyTypesEntry const* currency) const;
        uint32 GetTotalCap(CurrencyTypesEntry const* currency) const;

        // Mutations
        void ModifyCount(uint32 id, int32 count, bool modifyWeek = true, bool modifySeason = true, bool ignoreMultipliers = false);
        void SetCount(uint32 id, uint32 count);
        void SetFlags(uint32 currencyId, uint8 flags);
        void ResetWeekCounts();

        // Client notifications
        void SendAll() const;
        void SendWeekCap(uint32 id) const;
        void SendWeekCap(CurrencyTypesEntry const* currency) const;

        // DB lifecycle
        void Load(QueryResult* result);
        void Save();

    private:
        Player* m_owner;
        PlayerCurrenciesMap m_currencies;
};

#endif // MANGOS_H_CURRENCYMGR
