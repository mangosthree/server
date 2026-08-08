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

#include "TestHarness.h"

#include "LFGLockReason.h"

/**
 * @file
 * @brief Coverage for LFGLockReason::Compute -- the decision LFGMgr's
 * GetPlayerLockList (LFGMgr.cpp) delegates to for every dungeon in every
 * SMSG_LFG_PLAYER_INFO / SMSG_LFG_PARTY_INFO reply.
 *
 * Cases are built from a "everything passes" baseline (Deadmines-shaped:
 * low-level, non-seasonal, no dungeonfinder_requirements row) with exactly
 * one field flipped per test, so each case isolates one branch of the
 * precedence chain. The precedence tests at the end pin the ordering itself
 * -- expansion before raid before level before season before the
 * requirements-gated checks -- matching the original inline decision tree
 * in LFGMgr::FindRandomDungeonsNotForPlayer before this file existed.
 */

namespace
{
    LFGLockReason::CheckInput Baseline()
    {
        LFGLockReason::CheckInput in;
        in.playerLevel = 20;
        in.playerExpansion = 0;
        in.isAlliance = true;

        in.dungeonExpansionLevel = 0;
        in.dungeonTypeID = 1;          // LFG_TYPE_DUNGEON
        in.dungeonMinLevel = 15;
        in.dungeonMaxLevel = 20;

        in.isSeasonal = false;
        in.seasonActive = false;

        in.hasRequirements = false;    // no dungeonfinder_requirements row
        return in;
    }
}

TEST(LFGLockReason_Baseline_not_locked)
{
    LFGLockReason::Result result = LFGLockReason::Compute(Baseline());
    CHECK(result.reason == LFGLockReason::REASON_NONE);
    CHECK(result.subReason1 == 0);
    CHECK(result.subReason2 == 0);
}

TEST(LFGLockReason_expansion_too_low)
{
    LFGLockReason::CheckInput in = Baseline();
    in.dungeonExpansionLevel = 3;      // Cataclysm dungeon
    in.playerExpansion = 0;            // classic-only account

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_EXPANSION);
}

TEST(LFGLockReason_raid_type_always_locked)
{
    LFGLockReason::CheckInput in = Baseline();
    in.dungeonTypeID = LFGLockReason::DUNGEON_TYPE_RAID;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_RAID);
}

TEST(LFGLockReason_level_too_low)
{
    LFGLockReason::CheckInput in = Baseline();
    in.playerLevel = 10;               // below dungeonMinLevel (15)

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_LOW_LEVEL);
}

TEST(LFGLockReason_player_below_high_level_dungeon_min)
{
    // Halls of Reflection-shaped: high minLevel, level-20 player under it --
    // the DoD's own verification scenario. The client's rendered string is
    // "You must advance to a higher level," which is LOW_LEVEL (2), not
    // HIGH_LEVEL (3): the player is too low for the dungeon, not the other
    // way around.
    LFGLockReason::CheckInput in = Baseline();
    in.dungeonMinLevel = 80;
    in.dungeonMaxLevel = 80;
    in.playerLevel = 20;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_LOW_LEVEL);
}

TEST(LFGLockReason_level_above_dungeon_max)
{
    LFGLockReason::CheckInput in = Baseline();
    in.dungeonMinLevel = 1;
    in.dungeonMaxLevel = 10;
    in.playerLevel = 20;               // above dungeonMaxLevel

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_HIGH_LEVEL);
}

TEST(LFGLockReason_seasonal_not_active)
{
    LFGLockReason::CheckInput in = Baseline();
    in.isSeasonal = true;
    in.seasonActive = false;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_NOT_IN_SEASON);
}

TEST(LFGLockReason_seasonal_active_not_locked)
{
    LFGLockReason::CheckInput in = Baseline();
    in.isSeasonal = true;
    in.seasonActive = true;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_NONE);
}

TEST(LFGLockReason_gear_too_low_carries_numeric_sub_reasons)
{
    // Section 1.9's sub-reason contract: codes 4/5 need numeric
    // required/current item level, not zeros.
    LFGLockReason::CheckInput in = Baseline();
    in.hasRequirements = true;
    in.requiredItemLevel = 346;
    in.currentItemLevel = 300;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_LOW_GEAR_SCORE);
    CHECK(result.subReason1 == 346);
    CHECK(result.subReason2 == 300);
}

TEST(LFGLockReason_gear_exactly_at_floor_not_locked)
{
    LFGLockReason::CheckInput in = Baseline();
    in.hasRequirements = true;
    in.requiredItemLevel = 346;
    in.currentItemLevel = 346;         // equal, not below -- should pass

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_NONE);
}

TEST(LFGLockReason_no_requirements_row_skips_gear_check)
{
    // hasRequirements=false is also how the mapID==-1 guard in
    // LFGMgr::GetPlayerLockList expresses "could not look up requirements"
    // -- must never fall through to the gear/achievement/quest/item checks.
    LFGLockReason::CheckInput in = Baseline();
    in.hasRequirements = false;
    in.requiredItemLevel = 999;        // would fail if evaluated
    in.currentItemLevel = 0;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_NONE);
}

TEST(LFGLockReason_missing_achievement)
{
    LFGLockReason::CheckInput in = Baseline();
    in.hasRequirements = true;
    in.requiresAchievement = true;
    in.hasAchievement = false;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_MISSING_ACHIEVEMENT);
}

TEST(LFGLockReason_quest_incomplete_alliance)
{
    LFGLockReason::CheckInput in = Baseline();
    in.isAlliance = true;
    in.hasRequirements = true;
    in.requiresAllianceQuest = true;
    in.hasCompletedAllianceQuest = false;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_QUEST_INCOMPLETE);
}

TEST(LFGLockReason_quest_incomplete_horde)
{
    LFGLockReason::CheckInput in = Baseline();
    in.isAlliance = false;
    in.hasRequirements = true;
    in.requiresHordeQuest = true;
    in.hasCompletedHordeQuest = false;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_QUEST_INCOMPLETE);
}

TEST(LFGLockReason_horde_player_ignores_alliance_quest_requirement)
{
    LFGLockReason::CheckInput in = Baseline();
    in.isAlliance = false;
    in.hasRequirements = true;
    in.requiresAllianceQuest = true;
    in.hasCompletedAllianceQuest = false;  // irrelevant -- player is Horde

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_NONE);
}

TEST(LFGLockReason_missing_item_when_only_item1_required)
{
    LFGLockReason::CheckInput in = Baseline();
    in.hasRequirements = true;
    in.requiresItem1 = true;
    in.hasItem1 = false;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_MISSING_ITEM);
}

TEST(LFGLockReason_item2_satisfies_when_item1_missing)
{
    // Original nested logic: item1 required, absent, but item2 (an
    // alternate) is present -- that satisfies the requirement.
    LFGLockReason::CheckInput in = Baseline();
    in.hasRequirements = true;
    in.requiresItem1 = true;
    in.hasItem1 = false;
    in.requiresItem2 = true;
    in.hasItem2 = true;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_NONE);
}

TEST(LFGLockReason_item1_present_ignores_missing_item2)
{
    // item1 present satisfies the OR outright; item2's state never matters
    // once item1 is required and held (matches the original's outer "if
    // (req->item)" branch never falling into the item2-only else).
    LFGLockReason::CheckInput in = Baseline();
    in.hasRequirements = true;
    in.requiresItem1 = true;
    in.hasItem1 = true;
    in.requiresItem2 = true;
    in.hasItem2 = false;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_NONE);
}

TEST(LFGLockReason_missing_item2_only)
{
    LFGLockReason::CheckInput in = Baseline();
    in.hasRequirements = true;
    in.requiresItem1 = false;
    in.requiresItem2 = true;
    in.hasItem2 = false;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_MISSING_ITEM);
}

TEST(LFGLockReason_precedence_expansion_before_raid)
{
    LFGLockReason::CheckInput in = Baseline();
    in.dungeonTypeID = LFGLockReason::DUNGEON_TYPE_RAID;
    in.dungeonExpansionLevel = 3;
    in.playerExpansion = 0;

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_EXPANSION);
}

TEST(LFGLockReason_precedence_raid_before_level)
{
    LFGLockReason::CheckInput in = Baseline();
    in.dungeonTypeID = LFGLockReason::DUNGEON_TYPE_RAID;
    in.dungeonMinLevel = 85;
    in.playerLevel = 1;                // also fails the level check

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_RAID);
}

TEST(LFGLockReason_precedence_level_before_requirements)
{
    LFGLockReason::CheckInput in = Baseline();
    in.playerLevel = 1;                // fails level too
    in.hasRequirements = true;
    in.requiredItemLevel = 500;
    in.currentItemLevel = 0;           // also fails gear

    LFGLockReason::Result result = LFGLockReason::Compute(in);
    CHECK(result.reason == LFGLockReason::REASON_LOW_LEVEL);
}
