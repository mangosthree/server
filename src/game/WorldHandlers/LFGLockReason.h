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

#ifndef MANGOS_LFGLOCKREASON_H
#define MANGOS_LFGLOCKREASON_H

/**
 * @file LFGLockReason.h
 * @brief Pure LFG dungeon-lock-reason decision, decoupled from Player,
 * ObjectMgr, and DBCStores.
 *
 * LFGMgr::GetPlayerLockList (LFGMgr.cpp) gathers the primitives below from
 * the real player/DBC/world-DB state and calls Compute(); the decision
 * itself lives here so it can be unit tested without linking `game` --
 * src/tests/CMakeLists.txt only links `proto`/`shared` into mangos_tests,
 * the same boundary LFGPackets.h documents at its own top comment.
 *
 * The reason codes below are the wire contract (the client's
 * LFG_INSTANCE_INVALID_CODES index, LFD_CATA_ANALYSIS.md section 1.9) and
 * carry the same numeric values as LFGMgr.h's LFGForbiddenTypes enum. They
 * are declared again here, independently, rather than shared, for the same
 * reason LFGPackets.h does not include LFGMgr.h: LFGMgr.h pulls in Group.h
 * and the rest of the game headers that boundary excludes.
 */

#include "Platform/Define.h"

namespace LFGLockReason
{
    // ------------------------------------------------------------------
    // Wire-protocol reason codes (LFD_CATA_ANALYSIS.md section 1.9).
    // Compute() below only ever returns a subset of these -- the rest
    // (AREA_NOT_EXPLORED, the attunement pair, the TOO_SOON pair, and
    // TEMPORARILY_DISABLED) have no existing m3 data source to drive them
    // and are declared here only so a future caller has the right numeric
    // constant to reach for instead of inventing one.
    // ------------------------------------------------------------------

    const uint32 REASON_NONE                = 0;    ///< not locked
    const uint32 REASON_EXPANSION           = 1;    ///< EXPANSION_TOO_LOW
    const uint32 REASON_LOW_LEVEL           = 2;    ///< LEVEL_TOO_LOW
    const uint32 REASON_HIGH_LEVEL          = 3;    ///< LEVEL_TOO_HIGH
    const uint32 REASON_LOW_GEAR_SCORE      = 4;    ///< GEAR_TOO_LOW, numeric sub-reasons
    const uint32 REASON_HIGH_GEAR_SCORE     = 5;    ///< GEAR_TOO_HIGH, numeric sub-reasons
    const uint32 REASON_RAID                = 6;    ///< RAID_LOCKED
    const uint32 REASON_AREA_NOT_EXPLORED   = 9;    ///< AREA_NOT_EXPLORED, no data source yet
    const uint32 REASON_ATTUNEMENT_LOW      = 1001; ///< renders LEVEL_TOO_LOW
    const uint32 REASON_ATTUNEMENT_HIGH     = 1002; ///< renders LEVEL_TOO_HIGH
    const uint32 REASON_QUEST_INCOMPLETE    = 1022;
    const uint32 REASON_MISSING_ITEM        = 1025;
    const uint32 REASON_TOO_SOON_REALM      = 1029; ///< both 1029/1030 render TOO_SOON
    const uint32 REASON_TOO_SOON_CHARACTER  = 1030;
    const uint32 REASON_NOT_IN_SEASON       = 1031;
    const uint32 REASON_MISSING_ACHIEVEMENT = 1034;
    const uint32 REASON_TEMPORARILY_DISABLED = 10000;

    /// LfgType::LFG_TYPE_RAID (LFGMgr.h) -- duplicated as a literal for the
    /// same independence reason as the codes above.
    const uint32 DUNGEON_TYPE_RAID = 2;

    /**
     * Inputs needed to decide one dungeon's lock reason for one player.
     * Deliberately plain data; the caller gathers every field from Player,
     * DBCStores, and ObjectMgr::GetDungeonFinderRequirements so this
     * function needs none of them. Field-for-field mirror of the decision
     * tree that used to live inline in LFGMgr::FindRandomDungeonsNotForPlayer
     * (LFGMgr.cpp).
     */
    struct CheckInput
    {
        uint32 playerLevel = 0;
        uint32 playerExpansion = 0;
        bool   isAlliance = true;

        uint32 dungeonExpansionLevel = 0;
        uint32 dungeonTypeID = 0;          ///< LfgType
        uint32 dungeonMinLevel = 0;
        uint32 dungeonMaxLevel = 0;

        bool   isSeasonal = false;
        bool   seasonActive = false;

        /// False both when no dungeonfinder_requirements row exists for
        /// (mapId, difficulty) AND when the caller could not even look one
        /// up -- LFGDungeons.dbc has 49 rows with mapID == -1
        /// (LFD_CATA_ANALYSIS.md Phase 5's trap); the caller must not cast
        /// that to uint32 and must simply leave hasRequirements false.
        bool   hasRequirements = false;
        uint32 requiredItemLevel = 0;      ///< 0 == no gear floor
        uint32 currentItemLevel = 0;       ///< Player::GetEquipGearScore(false, false)
        bool   requiresAchievement = false;
        bool   hasAchievement = true;
        bool   requiresAllianceQuest = false;
        bool   hasCompletedAllianceQuest = true;
        bool   requiresHordeQuest = false;
        bool   hasCompletedHordeQuest = true;
        bool   requiresItem1 = false;
        bool   hasItem1 = true;
        bool   requiresItem2 = false;
        bool   hasItem2 = true;
    };

    /**
     * Result of a lock check. reason == REASON_NONE means "not locked".
     * subReason1/subReason2 are meaningful only for the two gear-score
     * reasons (required item level, current item level -- section 1.9's
     * sub-reason contract); every other reason leaves them 0.
     */
    struct Result
    {
        uint32 reason = REASON_NONE;
        uint32 subReason1 = 0;
        uint32 subReason2 = 0;
    };

    /**
     * Decide why (if at all) a player is locked out of one dungeon. Checks
     * run in the same precedence as the original inline version: expansion,
     * raid type, level bounds, season, then -- only when a requirements row
     * was found -- gear, achievement, quest, item. The first failing check
     * wins, exactly as the original if/else-if chain did.
     */
    inline Result Compute(CheckInput const& in)
    {
        Result result;

        if (in.dungeonExpansionLevel > in.playerExpansion)
        {
            result.reason = REASON_EXPANSION;
        }
        else if (in.dungeonTypeID == DUNGEON_TYPE_RAID)
        {
            result.reason = REASON_RAID;
        }
        else if (in.dungeonMinLevel > in.playerLevel)
        {
            result.reason = REASON_LOW_LEVEL;
        }
        else if (in.dungeonMaxLevel < in.playerLevel)
        {
            result.reason = REASON_HIGH_LEVEL;
        }
        else if (in.isSeasonal && !in.seasonActive)
        {
            result.reason = REASON_NOT_IN_SEASON;
        }
        else if (in.hasRequirements)
        {
            if (in.requiredItemLevel != 0 && in.currentItemLevel < in.requiredItemLevel)
            {
                result.reason = REASON_LOW_GEAR_SCORE;
                result.subReason1 = in.requiredItemLevel;
                result.subReason2 = in.currentItemLevel;
            }
            else if (in.requiresAchievement && !in.hasAchievement)
            {
                result.reason = REASON_MISSING_ACHIEVEMENT;
            }
            else if (in.isAlliance && in.requiresAllianceQuest
                     && !in.hasCompletedAllianceQuest)
            {
                result.reason = REASON_QUEST_INCOMPLETE;
            }
            else if (!in.isAlliance && in.requiresHordeQuest
                     && !in.hasCompletedHordeQuest)
            {
                result.reason = REASON_QUEST_INCOMPLETE;
            }
            else if (in.requiresItem1)
            {
                if (!in.hasItem1 && (!in.requiresItem2 || !in.hasItem2))
                {
                    result.reason = REASON_MISSING_ITEM;
                }
            }
            else if (in.requiresItem2 && !in.hasItem2)
            {
                result.reason = REASON_MISSING_ITEM;
            }
        }

        return result;
    }
}

#endif
