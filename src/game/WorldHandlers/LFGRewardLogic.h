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

#ifndef MANGOS_LFGREWARDLOGIC_H
#define MANGOS_LFGREWARDLOGIC_H

/**
 * @file LFGRewardLogic.h
 * @brief Dungeon classification, removal-penalty selection and the reward
 *        arithmetic behind LFG completion, decoupled from LFGMgr.
 *
 * Same boundary the other LFG logic headers document (LFGBootLogic.h,
 * LFGProposalLogic.h, LFGRoleAssignment.h): src/tests/CMakeLists.txt links
 * only `proto`/`shared` into mangos_tests, so anything testable has to stay
 * game-free. No Player, no Group, no DBC lookups -- callers resolve the DBC
 * row and pass raw fields in.
 *
 * DungeonTypes lives here rather than in LFGMgr.h because ClassifyDungeon
 * returns it; LFGMgr.h includes this header, so call sites are unchanged.
 * Values 0-4 are persisted in dungeonfinder_item_rewards.dungeon_type and
 * must keep their numbering.
 */

#include <cstdint>

/// Reward tier of an LFGDungeons row. Stored as a raw int in
/// dungeonfinder_item_rewards.dungeon_type -- do not renumber 0-4.
enum DungeonTypes
{
    DUNGEON_CLASSIC          = 0,
    DUNGEON_TBC              = 1,
    DUNGEON_TBC_HEROIC       = 2,
    DUNGEON_WOTLK            = 3,
    DUNGEON_WOTLK_HEROIC     = 4,
    DUNGEON_CATACLYSM        = 5,
    DUNGEON_CATACLYSM_HEROIC = 6,
    DUNGEON_UNKNOWN
};

namespace LFGRewardLogic
{
    /// Dungeon difficulty as LFGDungeons.dbc stores it, mirroring
    /// DUNGEON_DIFFICULTY_NORMAL / _HEROIC without including DBCEnums.h.
    enum DungeonDifficultyValue
    {
        DIFFICULTY_VALUE_NORMAL = 0,
        DIFFICULTY_VALUE_HEROIC = 1
    };

    /// Currencies the finder pays out.
    enum RewardCurrency
    {
        LFG_CURRENCY_JUSTICE = 395,
        LFG_CURRENCY_VALOR   = 396
    };

    /// Cataclysm random-dungeon currency, in the hundredths CurrencyMgr
    /// stores and SMSG_LFG_PLAYER_REWARD carries -- the client divides a
    /// currency carrying CurrencyTypes flag 0x08 by 100 before display, and
    /// both of ours do. These are era constants of the expansion, not
    /// operator data, so they live beside the code rather than in the DB.
    enum RewardAmount
    {
        /// 150 Valor per level-85 random heroic. The era "first seven per
        /// week" needs no counter: the DBC's own 1000-per-week purse cap
        /// clamps the seventh run and zeroes the rest.
        CATA_HEROIC_VALOR = 15000,
        /// 70 Justice instead, once the Valor week is spent.
        CATA_HEROIC_JUSTICE_FALLBACK = 7000,
        /// 140 Justice per random Cataclysm normal, seven per week.
        CATA_NORMAL_JUSTICE = 14000,
        CATA_NORMAL_JUSTICE_RUNS_PER_WEEK = 7,
        /// Random Cataclysm Dungeon spans levels 80-85, but its Justice
        /// starts with the 83+ reward quests -- below that the slot is
        /// served by the older Lich King pair, gold and experience only.
        CATA_NORMAL_JUSTICE_MIN_LEVEL = 83
    };

    /// What a removal from an LFD group costs the player who left it.
    enum class RemovalPenalty
    {
        NONE,
        DESERTER,
        CLEAR_COOLDOWN,
        DESERTER_AND_CLEAR_COOLDOWN
    };

    /// Reward tier for an LFGDungeons row. Every expansion returns from its
    /// own case: an unknown difficulty inside a known expansion is
    /// DUNGEON_UNKNOWN, never the next expansion's tier.
    inline DungeonTypes ClassifyDungeon(std::uint32_t expansionLevel,
                                        std::uint32_t difficulty)
    {
        switch (expansionLevel)
        {
            case 0:
                // Classic has no heroic split in the finder's own table.
                return DUNGEON_CLASSIC;
            case 1:
                if (difficulty == DIFFICULTY_VALUE_NORMAL)
                {
                    return DUNGEON_TBC;
                }
                if (difficulty == DIFFICULTY_VALUE_HEROIC)
                {
                    return DUNGEON_TBC_HEROIC;
                }
                return DUNGEON_UNKNOWN;
            case 2:
                if (difficulty == DIFFICULTY_VALUE_NORMAL)
                {
                    return DUNGEON_WOTLK;
                }
                if (difficulty == DIFFICULTY_VALUE_HEROIC)
                {
                    return DUNGEON_WOTLK_HEROIC;
                }
                return DUNGEON_UNKNOWN;
            case 3:
                if (difficulty == DIFFICULTY_VALUE_NORMAL)
                {
                    return DUNGEON_CATACLYSM;
                }
                if (difficulty == DIFFICULTY_VALUE_HEROIC)
                {
                    return DUNGEON_CATACLYSM_HEROIC;
                }
                return DUNGEON_UNKNOWN;
            default:
                return DUNGEON_UNKNOWN;
        }
    }

    /**
    * Penalty owed when a member leaves or is removed from an LFD group.
    *
    * \arg \c removeMethod
    *   Group::RemoveMember's method: 1 is a kick, anything else is the
    *   player leaving of their own accord.
    * \arg \c dungeonInProgress
    *   The group's run had not finished yet. A leave after the final boss
    *   costs nothing.
    * \arg \c remainingMembers
    *   Members left in the group once this one is gone.
    * \arg \c minRemaining
    *   Config: members that must remain for a leaver to earn Deserter.
    *   0 -- the default -- means no threshold at all.
    * \arg \c deserterOnVoteKick
    *   Config: whether a vote-kicked player is treated as a leaver.
    * \return
    *   What to apply to the departing player. A kick always clears the
    *   random cooldown, whether or not it also deserters.
    */
    inline RemovalPenalty PenaltyForRemoval(std::uint8_t removeMethod,
                                            bool dungeonInProgress,
                                            std::size_t remainingMembers,
                                            std::uint32_t minRemaining,
                                            bool deserterOnVoteKick)
    {
        if (removeMethod == 1)
        {
            return deserterOnVoteKick ? RemovalPenalty::DESERTER_AND_CLEAR_COOLDOWN
                                      : RemovalPenalty::CLEAR_COOLDOWN;
        }

        if (!dungeonInProgress)
        {
            return RemovalPenalty::NONE;
        }

        return (remainingMembers >= std::size_t(minRemaining))
            ? RemovalPenalty::DESERTER : RemovalPenalty::NONE;
    }

    /// Luck of the Draw stacks for a group holding this many solo-queued
    /// members. One stack each, capped at three -- the cap is the spell's
    /// own, from SpellAuraOptions, not a number chosen here.
    inline std::uint32_t LuckOfTheDrawStacks(std::uint32_t soloJoinedMembers)
    {
        std::uint32_t const maxStacks = 3;
        return (soloJoinedMembers < maxStacks) ? soloJoinedMembers : maxStacks;
    }

    /// Multiplier on the stored reward row. dungeonfinder_rewards holds the
    /// subsequent-run value, so the first run of the day pays double.
    inline std::uint32_t FirstRewardMultiplier(bool hasDoneDaily)
    {
        return hasDoneDaily ? 1u : 2u;
    }

    /// Whether a dungeonfinder_item_rewards row serves this dungeon: same
    /// tier, and the dungeon's average level inside the row's band.
    inline bool ItemRewardMatches(std::uint32_t rowMin, std::uint32_t rowMax,
                                  std::uint32_t rowType, std::uint32_t avgLevel,
                                  std::uint32_t wantedType)
    {
        return (rowType == wantedType) && (avgLevel >= rowMin) && (avgLevel <= rowMax);
    }

    /// Completions still paying this period, mirroring the client's
    /// LFGRewardsFrame_EstimateRemainingCompletions. Quantities are whatever
    /// unit the caller uses throughout -- it is a ratio, so hundredths and
    /// display units give the same answer.
    inline std::uint32_t RemainingWeeklyCompletions(std::uint32_t weekQuantity,
                                                    std::uint32_t weekCap,
                                                    std::uint32_t perCompletion)
    {
        if (perCompletion == 0 || weekQuantity >= weekCap)
        {
            return 0;
        }

        std::uint32_t const left = weekCap - weekQuantity;
        return (left + perCompletion - 1) / perCompletion;
    }
}

#endif
