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

#include "LFGCallToArmsLogic.h"

/**
 * @file LFGCallToArmsLogicTest.cpp
 * @brief Coverage for LFGCallToArmsLogic -- which roles a queue is short of,
 * which dungeon slots can call for them, and who the call pays.
 */

using namespace LFGCallToArmsLogic;
using namespace LFGRoleAssignment;

TEST(LFGCallToArms_Shortage_empty_queue_calls_nobody)
{
    // A shortage of everybody is a shortage of nobody. The queue being empty
    // must not read as "no tanks and no healers".
    CHECK(ComputeShortageMask(0, 0, 0, 1) == 0u);
}

TEST(LFGCallToArms_Shortage_balanced_queue_calls_nobody)
{
    // Exactly one group's worth of each seat.
    CHECK(ComputeShortageMask(1, 1, 3, 1) == 0u);
    CHECK(ComputeShortageMask(4, 4, 12, 1) == 0u);
}

TEST(LFGCallToArms_Shortage_single_role)
{
    CHECK(ComputeShortageMask(0, 1, 3, 1) == uint32(ROLE_TANK));
    CHECK(ComputeShortageMask(1, 0, 3, 1) == uint32(ROLE_HEALER));
    CHECK(ComputeShortageMask(1, 1, 0, 1) == uint32(ROLE_DAMAGE));
}

TEST(LFGCallToArms_Shortage_normalises_by_group_need)
{
    // Three players offering one role each is not balanced: a group needs
    // three damage dealers, so weighted supply is 3/3/1 and damage is short.
    CHECK(ComputeShortageMask(1, 1, 1, 1) == uint32(ROLE_DAMAGE));
}

TEST(LFGCallToArms_Shortage_ties_call_both)
{
    CHECK(ComputeShortageMask(0, 0, 5, 1) == uint32(ROLE_TANK | ROLE_HEALER));
    CHECK(ComputeShortageMask(5, 5, 30, 1) == uint32(ROLE_TANK | ROLE_HEALER));
    // Weighted 3/6/3 -- a tank and a damage dealer are equally scarce.
    CHECK(ComputeShortageMask(1, 2, 3, 1) == uint32(ROLE_TANK | ROLE_DAMAGE));
}

TEST(LFGCallToArms_Shortage_scarcest_wins_on_a_busy_queue)
{
    // Weighted 6/15/30.
    CHECK(ComputeShortageMask(2, 5, 30, 1) == uint32(ROLE_TANK));
}

TEST(LFGCallToArms_Shortage_min_queued_gate)
{
    CHECK(ComputeShortageMask(0, 0, 1, 2) == 0u);
    CHECK(ComputeShortageMask(0, 0, 1, 1) == uint32(ROLE_TANK | ROLE_HEALER));
}

TEST(LFGCallToArms_Shortage_min_queued_zero_disables)
{
    CHECK(ComputeShortageMask(0, 0, 1, 0) == 0u);
    CHECK(ComputeShortageMask(1, 1, 1, 0) == 0u);
}

TEST(LFGCallToArms_Shortage_large_counts_do_not_wrap)
{
    // Weighted in 64 bits: 3000000/3000000/3000000, still balanced.
    CHECK(ComputeShortageMask(1000000, 1000000, 3000000, 1) == 0u);
    CHECK(ComputeShortageMask(1000000, 2000000, 3000000, 1)
          == uint32(ROLE_TANK | ROLE_DAMAGE));
}

TEST(LFGCallToArms_Slot_only_random_cataclysm_heroic)
{
    CHECK(IsCallToArmsSlot(6, DUNGEON_CATACLYSM_HEROIC) == true);
    CHECK(IsCallToArmsSlot(6, DUNGEON_CATACLYSM) == false);
    CHECK(IsCallToArmsSlot(6, DUNGEON_WOTLK_HEROIC) == false);
    CHECK(IsCallToArmsSlot(6, DUNGEON_UNKNOWN) == false);
}

TEST(LFGCallToArms_Slot_specific_dungeon_is_not_a_slot)
{
    CHECK(IsCallToArmsSlot(1, DUNGEON_CATACLYSM_HEROIC) == false);
    CHECK(IsCallToArmsSlot(0, DUNGEON_CATACLYSM_HEROIC) == false);
}

TEST(LFGCallToArms_Eligible_solo_in_a_called_seat)
{
    CHECK(ShortageEligibleAtFormation(true, ROLE_TANK, ROLE_TANK) == true);
}

TEST(LFGCallToArms_Eligible_premade_never_qualifies)
{
    // The seat was filled before the call went out.
    CHECK(ShortageEligibleAtFormation(false, ROLE_TANK, ROLE_TANK) == false);
}

TEST(LFGCallToArms_Eligible_wrong_seat)
{
    CHECK(ShortageEligibleAtFormation(true, ROLE_DAMAGE, ROLE_TANK) == false);
}

TEST(LFGCallToArms_Eligible_leader_bit_is_stripped)
{
    CHECK(ShortageEligibleAtFormation(true, ROLE_LEADER | ROLE_TANK, ROLE_TANK)
          == true);
    CHECK(ShortageEligibleAtFormation(true, ROLE_LEADER, ROLE_TANK) == false);
}

TEST(LFGCallToArms_Eligible_no_shortage_pays_nobody)
{
    CHECK(ShortageEligibleAtFormation(true, ROLE_TANK, 0) == false);
    CHECK(ShortageEligibleAtFormation(true, ROLE_HEALER, 0) == false);
}
