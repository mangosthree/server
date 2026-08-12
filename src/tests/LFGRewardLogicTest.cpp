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

#include "LFGRewardLogic.h"

/**
 * @file LFGRewardLogicTest.cpp
 * @brief Coverage for LFGRewardLogic -- dungeon tier classification, the
 * penalty owed for leaving or being kicked from an LFD group, and the
 * reward arithmetic the completion path and the reward preview share.
 */

using namespace LFGRewardLogic;

TEST(LFGRewardLogic_Classify_classic_normal)
{
    CHECK(ClassifyDungeon(0, 0) == DUNGEON_CLASSIC);
}

TEST(LFGRewardLogic_Classify_classic_ignores_difficulty)
{
    // Classic carries no heroic tier in the finder's own table; this pins
    // current behaviour rather than asserting a game rule.
    CHECK(ClassifyDungeon(0, 1) == DUNGEON_CLASSIC);
}

TEST(LFGRewardLogic_Classify_tbc_tiers)
{
    CHECK(ClassifyDungeon(1, 0) == DUNGEON_TBC);
    CHECK(ClassifyDungeon(1, 1) == DUNGEON_TBC_HEROIC);
}

TEST(LFGRewardLogic_Classify_wotlk_tiers)
{
    CHECK(ClassifyDungeon(2, 0) == DUNGEON_WOTLK);
    CHECK(ClassifyDungeon(2, 1) == DUNGEON_WOTLK_HEROIC);
}

TEST(LFGRewardLogic_Classify_cataclysm_tiers)
{
    CHECK(ClassifyDungeon(3, 0) == DUNGEON_CATACLYSM);
    CHECK(ClassifyDungeon(3, 1) == DUNGEON_CATACLYSM_HEROIC);
}

TEST(LFGRewardLogic_Classify_unknown_difficulty_does_not_fall_through)
{
    // The switch this replaced had no return on the difficulty miss, so a
    // TBC row of unknown difficulty was classified as a WotLK one. Every
    // expansion now answers for itself.
    CHECK(ClassifyDungeon(1, 2) == DUNGEON_UNKNOWN);
    CHECK(ClassifyDungeon(2, 5) == DUNGEON_UNKNOWN);
    CHECK(ClassifyDungeon(3, std::uint32_t(-1)) == DUNGEON_UNKNOWN);
}

TEST(LFGRewardLogic_Classify_future_expansion_is_unknown)
{
    CHECK(ClassifyDungeon(4, 0) == DUNGEON_UNKNOWN);
}

TEST(LFGRewardLogic_Penalty_leave_in_progress_deserters_by_default)
{
    CHECK(PenaltyForRemoval(0, true, 4, 0, false) == RemovalPenalty::DESERTER);
}

TEST(LFGRewardLogic_Penalty_leave_has_no_threshold_by_default)
{
    // TrinityCore only deserters when four would remain. Era evidence
    // refutes that number and supports no replacement, so the default is
    // unconditional and the threshold is config.
    CHECK(PenaltyForRemoval(0, true, 1, 0, false) == RemovalPenalty::DESERTER);
    CHECK(PenaltyForRemoval(0, true, 0, 0, false) == RemovalPenalty::DESERTER);
}

TEST(LFGRewardLogic_Penalty_leave_after_completion_is_free)
{
    CHECK(PenaltyForRemoval(0, false, 4, 0, false) == RemovalPenalty::NONE);
}

TEST(LFGRewardLogic_Penalty_kick_clears_the_cooldown)
{
    CHECK(PenaltyForRemoval(1, true, 4, 0, false) == RemovalPenalty::CLEAR_COOLDOWN);
    CHECK(PenaltyForRemoval(1, false, 4, 0, false) == RemovalPenalty::CLEAR_COOLDOWN);
}

TEST(LFGRewardLogic_Penalty_threshold_config)
{
    CHECK(PenaltyForRemoval(0, true, 3, 4, false) == RemovalPenalty::NONE);
    CHECK(PenaltyForRemoval(0, true, 4, 4, false) == RemovalPenalty::DESERTER);
}

TEST(LFGRewardLogic_Penalty_kick_config_treats_a_kick_as_a_leave)
{
    CHECK(PenaltyForRemoval(1, true, 4, 0, true)
          == RemovalPenalty::DESERTER_AND_CLEAR_COOLDOWN);
}

TEST(LFGRewardLogic_FirstRewardMultiplier)
{
    CHECK(FirstRewardMultiplier(true) == 1u);
    CHECK(FirstRewardMultiplier(false) == 2u);
}

TEST(LFGRewardLogic_LuckOfTheDrawStacks)
{
    // One stack per stranger, capped at the spell's own maximum of three.
    // A group that queued together as five has no strangers and no buff.
    CHECK(LuckOfTheDrawStacks(0) == 0u);
    CHECK(LuckOfTheDrawStacks(1) == 1u);
    CHECK(LuckOfTheDrawStacks(2) == 2u);
    CHECK(LuckOfTheDrawStacks(3) == 3u);
    CHECK(LuckOfTheDrawStacks(4) == 3u);
    CHECK(LuckOfTheDrawStacks(5) == 3u);
}

TEST(LFGRewardLogic_ItemRewardMatches_band)
{
    CHECK(ItemRewardMatches(70, 80, 4, 75, 4) == true);
    CHECK(ItemRewardMatches(70, 80, 4, 70, 4) == true);
    CHECK(ItemRewardMatches(70, 80, 4, 80, 4) == true);
    CHECK(ItemRewardMatches(70, 80, 4, 81, 4) == false);
    CHECK(ItemRewardMatches(70, 80, 4, 69, 4) == false);
}

TEST(LFGRewardLogic_ItemRewardMatches_type)
{
    CHECK(ItemRewardMatches(70, 80, 3, 75, 4) == false);
}

TEST(LFGRewardLogic_RemainingWeeklyCompletions)
{
    // Raw hundredths throughout, matching what the callers pass: 15000 is
    // 150 Valor, 100000 the weekly purse cap. Seven runs fill the cap, the
    // seventh partially.
    CHECK(RemainingWeeklyCompletions(0, 100000, 15000) == 7u);
    CHECK(RemainingWeeklyCompletions(85000, 100000, 15000) == 1u);
    CHECK(RemainingWeeklyCompletions(90000, 100000, 15000) == 1u);
    CHECK(RemainingWeeklyCompletions(100000, 100000, 15000) == 0u);
}

TEST(LFGRewardLogic_RemainingWeeklyCompletions_guards)
{
    CHECK(RemainingWeeklyCompletions(0, 100000, 0) == 0u);
    CHECK(RemainingWeeklyCompletions(120000, 100000, 15000) == 0u);
}
