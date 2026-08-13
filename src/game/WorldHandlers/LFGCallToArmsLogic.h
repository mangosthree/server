/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
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

#ifndef MANGOS_LFGCALLTOARMSLOGIC_H
#define MANGOS_LFGCALLTOARMSLOGIC_H

/**
 * @file LFGCallToArmsLogic.h
 * @brief Which roles the Dungeon Finder is calling for, and who that pays.
 *
 * Same boundary the other LFG logic headers document (LFGRewardLogic.h,
 * LFGRoleAssignment.h): src/tests/CMakeLists.txt links only `proto`/`shared`
 * into mangos_tests, so nothing here may reach into the game. Callers resolve
 * the DBC row and walk the live queue, then pass raw counts in.
 *
 * The role bits come from LFGRoleAssignment.h rather than being declared
 * again, so the two cannot drift.
 */

#include "LFGRewardLogic.h"
#include "LFGRoleAssignment.h"

#include <algorithm>
#include <cstdint>

namespace LFGCallToArmsLogic
{
    /// Seats a 5-man has to fill. Supply is normalised by these before the
    /// roles are compared -- three queued damage dealers are not a surplus.
    enum GroupNeed
    {
        NEED_TANKS   = 1,
        NEED_HEALERS = 1,
        NEED_DPS     = 3
    };

    /// LFGDungeons.dbc typeID that marks a random slot rather than a
    /// specific dungeon.
    enum SlotType
    {
        SLOT_TYPE_RANDOM = 6
    };

    /**
    * Which roles are called, given the queue supply for one random slot.
    *
    * Retail never published its trigger, so the rule is server policy: a role
    * is called when its supply, weighted by how many of it a group needs, is
    * strictly the scarcest. A balanced queue calls nobody, and neither does
    * an empty one -- a shortage of everybody is a shortage of nobody.
    *
    * \arg \c tanks \c healers \c dps
    *   Players queued offering each role. One player who ticked several
    *   roles counts once for each of them.
    * \arg \c minQueued
    *   Players that must be queued for the slot before any shortage is
    *   declared there. 0 disables the slot.
    * \return
    *   ROLE_TANK / ROLE_HEALER / ROLE_DAMAGE bits, or 0 for no shortage.
    */
    inline std::uint32_t ComputeShortageMask(std::uint32_t tanks,
                                             std::uint32_t healers,
                                             std::uint32_t dps,
                                             std::uint32_t minQueued)
    {
        std::uint64_t const queued =
            std::uint64_t(tanks) + std::uint64_t(healers) + std::uint64_t(dps);

        if (minQueued == 0 || queued < minQueued)
        {
            return 0;
        }

        // Cross-multiplied by the 1/1/3 needs so the three are comparable as
        // integers, and in 64 bits so a large queue cannot wrap.
        std::uint64_t const supplyTank   = std::uint64_t(tanks) * NEED_DPS;
        std::uint64_t const supplyHealer = std::uint64_t(healers) * NEED_DPS;
        std::uint64_t const supplyDps    = std::uint64_t(dps) * NEED_TANKS;

        std::uint64_t const scarcest =
            std::min(supplyTank, std::min(supplyHealer, supplyDps));
        std::uint64_t const plentiful =
            std::max(supplyTank, std::max(supplyHealer, supplyDps));

        if (scarcest == plentiful)
        {
            return 0;
        }

        std::uint32_t mask = 0;
        if (supplyTank == scarcest)
        {
            mask |= LFGRoleAssignment::ROLE_TANK;
        }
        if (supplyHealer == scarcest)
        {
            mask |= LFGRoleAssignment::ROLE_HEALER;
        }
        if (supplyDps == scarcest)
        {
            mask |= LFGRoleAssignment::ROLE_DAMAGE;
        }
        return mask;
    }

    /**
    * Whether a dungeon slot can carry a Call to Arms.
    *
    * The level-85 random heroic slots, and only those: a levelling random is
    * a normal-difficulty row, and a specific dungeon is not a random slot at
    * all.
    *
    * \arg \c typeID
    *   LFGDungeons.dbc typeID.
    * \arg \c type
    *   The row's reward tier, as ClassifyDungeon returns it.
    */
    inline bool IsCallToArmsSlot(std::uint32_t typeID, DungeonTypes type)
    {
        return typeID == SLOT_TYPE_RANDOM && type == DUNGEON_CATACLYSM_HEROIC;
    }

    /**
    * Whether a member earns the satchel for the seat they were given.
    *
    * Both halves are required: the reward answers a call, so it goes to a
    * player who queued alone into a called role. Somebody brought along by a
    * premade filled that seat before the call went out.
    *
    * \arg \c queuedSolo
    *   The member joined the queue on their own.
    * \arg \c assignedRole
    *   The seat they were given; the leader bit is ignored.
    * \arg \c shortageMask
    *   Roles called for the slot at that moment. 0 pays nobody.
    */
    inline bool ShortageEligibleAtFormation(bool queuedSolo,
                                            std::uint8_t assignedRole,
                                            std::uint32_t shortageMask)
    {
        std::uint32_t const seat = std::uint32_t(assignedRole) &
            ~std::uint32_t(LFGRoleAssignment::ROLE_LEADER);

        return queuedSolo && (seat & shortageMask) != 0;
    }
}

#endif
