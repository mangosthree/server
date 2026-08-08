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

#include "LFGRoleAssignment.h"

/**
 * @file LFGRoleAssignmentTest.cpp
 * @brief Coverage for LFGRoleAssignment::Assign -- the decision LFGMgr uses
 * to work out how many of each role a queue entry still needs.
 *
 * The multi-role cases are the point of the file: the previous code matched
 * the selected mask exactly, so a player who ticked two roles counted as
 * none, their needs never dropped, and they were never merged into a group.
 */

namespace
{
    using namespace LFGRoleAssignment;

    const uint8 TANKS_5MAN = 1;
    const uint8 HEALS_5MAN = 1;
    const uint8 DPS_5MAN = 3;

    Counts Assign5Man(std::vector<uint8> const& masks)
    {
        return Assign(masks, TANKS_5MAN, HEALS_5MAN, DPS_5MAN);
    }
}

TEST(LFGRoleAssignment_empty_queue_seats_nobody)
{
    Counts c = Assign5Man(std::vector<uint8>());
    CHECK(c.tanks == 0);
    CHECK(c.healers == 0);
    CHECK(c.dps == 0);
    CHECK(c.seated == 0);
}

TEST(LFGRoleAssignment_single_role_each_fills_a_5man)
{
    std::vector<uint8> masks;
    masks.push_back(ROLE_TANK);
    masks.push_back(ROLE_HEALER);
    masks.push_back(ROLE_DAMAGE);
    masks.push_back(ROLE_DAMAGE);
    masks.push_back(ROLE_DAMAGE);

    Counts c = Assign5Man(masks);
    CHECK(c.tanks == 1);
    CHECK(c.healers == 1);
    CHECK(c.dps == 3);
    CHECK(c.seated == 5);
}

TEST(LFGRoleAssignment_leader_bit_is_ignored)
{
    std::vector<uint8> masks;
    masks.push_back(uint8(ROLE_TANK | ROLE_LEADER));
    masks.push_back(ROLE_HEALER);

    Counts c = Assign5Man(masks);
    CHECK(c.tanks == 1);
    CHECK(c.healers == 1);
    CHECK(c.seated == 2);
}

TEST(LFGRoleAssignment_multi_role_player_is_seated_not_dropped)
{
    // The regression: tank+damage used to match no case and count as nothing.
    std::vector<uint8> masks;
    masks.push_back(uint8(ROLE_TANK | ROLE_DAMAGE));

    Counts c = Assign5Man(masks);
    CHECK(c.seated == 1);
    CHECK(c.tanks == 1);
    CHECK(c.dps == 0);
}

TEST(LFGRoleAssignment_all_three_roles_seats_scarcest_first)
{
    std::vector<uint8> masks;
    masks.push_back(uint8(ROLE_TANK | ROLE_HEALER | ROLE_DAMAGE));

    Counts c = Assign5Man(masks);
    CHECK(c.tanks == 1);
    CHECK(c.healers == 0);
    CHECK(c.dps == 0);
    CHECK(c.seated == 1);
}

TEST(LFGRoleAssignment_single_role_players_seated_before_flexible)
{
    // The dedicated tank must take the tank slot; the flexible player then
    // falls through to damage rather than stealing it.
    std::vector<uint8> masks;
    masks.push_back(uint8(ROLE_TANK | ROLE_DAMAGE));
    masks.push_back(ROLE_TANK);

    Counts c = Assign5Man(masks);
    CHECK(c.tanks == 1);
    CHECK(c.dps == 1);
    CHECK(c.seated == 2);
}

TEST(LFGRoleAssignment_flexible_player_completes_a_group)
{
    std::vector<uint8> masks;
    masks.push_back(ROLE_TANK);
    masks.push_back(ROLE_DAMAGE);
    masks.push_back(ROLE_DAMAGE);
    masks.push_back(ROLE_DAMAGE);
    masks.push_back(uint8(ROLE_HEALER | ROLE_DAMAGE));

    Counts c = Assign5Man(masks);
    CHECK(c.tanks == 1);
    CHECK(c.healers == 1);
    CHECK(c.dps == 3);
    CHECK(c.seated == 5);
}

TEST(LFGRoleAssignment_no_role_selected_is_not_seated)
{
    std::vector<uint8> masks;
    masks.push_back(ROLE_NONE);
    masks.push_back(ROLE_TANK);

    Counts c = Assign5Man(masks);
    CHECK(c.seated == 1);
    CHECK(c.tanks == 1);
}

TEST(LFGRoleAssignment_surplus_single_role_is_not_seated)
{
    // Two dedicated tanks cannot both play; only one is seated, and the
    // caller sees seated < size and treats the group as not viable.
    std::vector<uint8> masks;
    masks.push_back(ROLE_TANK);
    masks.push_back(ROLE_TANK);

    Counts c = Assign5Man(masks);
    CHECK(c.tanks == 1);
    CHECK(c.seated == 1);
}

TEST(LFGRoleAssignment_dps_capacity_is_respected)
{
    std::vector<uint8> masks;
    for (int i = 0; i < 5; ++i)
    {
        masks.push_back(ROLE_DAMAGE);
    }

    Counts c = Assign5Man(masks);
    CHECK(c.dps == 3);
    CHECK(c.seated == 3);
}

TEST(LFGRoleAssignment_flexible_yields_tank_slot_to_a_dedicated_tank)
{
    // The case that stranded a queue: a player who ticked all three roles
    // must not squat the tank slot when a dedicated tank is also present.
    std::vector<uint8> masks;
    masks.push_back(ROLE_HEALER);
    masks.push_back(uint8(ROLE_TANK | ROLE_HEALER | ROLE_DAMAGE));
    masks.push_back(ROLE_TANK);

    Counts c = Assign5Man(masks);
    CHECK(c.tanks == 1);
    CHECK(c.healers == 1);
    CHECK(c.dps == 1);
    CHECK(c.seated == 3);
}

TEST(LFGRoleAssignment_flexible_paladin_with_full_group_takes_damage)
{
    std::vector<uint8> masks;
    masks.push_back(ROLE_TANK);
    masks.push_back(ROLE_HEALER);
    masks.push_back(ROLE_DAMAGE);
    masks.push_back(ROLE_DAMAGE);
    masks.push_back(uint8(ROLE_TANK | ROLE_HEALER | ROLE_DAMAGE));

    Counts c = Assign5Man(masks);
    CHECK(c.tanks == 1);
    CHECK(c.healers == 1);
    CHECK(c.dps == 3);
    CHECK(c.seated == 5);
}

TEST(LFGRoleAssignment_all_flexible_fills_every_slot)
{
    std::vector<uint8> masks;
    for (int i = 0; i < 5; ++i)
    {
        masks.push_back(uint8(ROLE_TANK | ROLE_HEALER | ROLE_DAMAGE));
    }

    Counts c = Assign5Man(masks);
    CHECK(c.tanks == 1);
    CHECK(c.healers == 1);
    CHECK(c.dps == 3);
    CHECK(c.seated == 5);
}
