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

#include "LFGBootLogic.h"

/**
 * @file LFGBootLogicTest.cpp
 * @brief Coverage for LFGBootLogic -- vote threshold selection, self-kick
 * refusal, tally resolution and the clamped remaining-time math behind the
 * LFG boot (vote-kick) system.
 */

using namespace LFGBootLogic;

TEST(LFGBootLogic_RequiredVotes_lfd)
{
    CHECK(RequiredVotes(false) == 4u);
}

TEST(LFGBootLogic_RequiredVotes_lfr)
{
    CHECK(RequiredVotes(true) == 15u);
}

TEST(LFGBootLogic_ShouldVoteKick_distinct_players)
{
    CHECK(ShouldVoteKick(1, 2) == true);
}

TEST(LFGBootLogic_ShouldVoteKick_self_is_refused)
{
    CHECK(ShouldVoteKick(1, 1) == false);
}

TEST(LFGBootLogic_Resolve_below_both_thresholds_is_pending)
{
    BootTally tally;
    tally.total = 2;
    tally.agree = 1;
    tally.deny = 1;

    CHECK(Resolve(tally, 4) == BOOT_PENDING);
}

TEST(LFGBootLogic_Resolve_at_threshold_passes)
{
    BootTally tally;
    tally.agree = 4;

    // Proves >=, not ==.
    CHECK(Resolve(tally, 4) == BOOT_PASSED);
}

TEST(LFGBootLogic_Resolve_above_threshold_passes)
{
    BootTally tally;
    tally.agree = 5;

    // The == regression guard.
    CHECK(Resolve(tally, 4) == BOOT_PASSED);
}

TEST(LFGBootLogic_Resolve_deny_wins)
{
    BootTally tally;
    tally.deny = 4;

    CHECK(Resolve(tally, 4) == BOOT_FAILED);
}

TEST(LFGBootLogic_Resolve_agree_beats_deny)
{
    BootTally tally;
    tally.agree = 4;
    tally.deny = 4;

    // Order is defined: agree is checked first.
    CHECK(Resolve(tally, 4) == BOOT_PASSED);
}

TEST(LFGBootLogic_Resolve_lfr_threshold_not_yet_reached)
{
    BootTally tally;
    tally.agree = 4;

    CHECK(Resolve(tally, 15) == BOOT_PENDING);
}

TEST(LFGBootLogic_RemainingSeconds_mid_vote)
{
    CHECK(RemainingSeconds(1000, 120, 1030) == 90);
}

TEST(LFGBootLogic_RemainingSeconds_exactly_expired)
{
    CHECK(RemainingSeconds(1000, 120, 1120) == 0);
}

TEST(LFGBootLogic_RemainingSeconds_well_past_does_not_wrap)
{
    // C-8 fault (c): no clamp meant this wrapped through uint8 in the
    // legacy sender. Confirm the replacement floors at 0 instead.
    CHECK(RemainingSeconds(1000, 120, 9999) == 0);
}

TEST(LFGBootLogic_RemainingSeconds_clock_skew_returns_full_duration)
{
    CHECK(RemainingSeconds(1000, 120, 500) == 120);
}

TEST(LFGBootLogic_RemainingSeconds_zero_duration)
{
    CHECK(RemainingSeconds(1000, 0, 1000) == 0);
}

TEST(LFGBootLogic_five_man_unanimity_all_three_agree_passes)
{
    // Victim auto-DENY, kicker auto-AGREE, three survivors all AGREE.
    BootTally tally;
    tally.agree = 1 + 3;   // kicker + three survivors
    tally.deny = 1;        // victim
    tally.total = tally.agree + tally.deny;

    CHECK(Resolve(tally, RequiredVotes(false)) == BOOT_PASSED);
}

TEST(LFGBootLogic_five_man_unanimity_one_abstention_deadlocks)
{
    // Same group, but one survivor never votes: neither side reaches 4.
    BootTally tally;
    tally.agree = 1 + 2;   // kicker + two survivors
    tally.deny = 1;        // victim
    tally.total = tally.agree + tally.deny;

    CHECK(Resolve(tally, RequiredVotes(false)) == BOOT_PENDING);
}

TEST(LFGBootLogic_budget_constant)
{
    CHECK(int(BOOT_MAX_KICKS) == 3);
}
