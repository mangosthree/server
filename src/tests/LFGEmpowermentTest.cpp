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

#include "LFGEmpowerment.h"

/**
 * @file LFGEmpowermentTest.cpp
 * @brief Coverage for LFGEmpowerment::IsEmpowered against the client's
 * LFD_IsEmpowered decision table (UIParent.lua:3649-3666).
 */

TEST(LFGEmpowerment_solo_is_always_empowered)
{
    LFGEmpowerment::State state;
    CHECK(LFGEmpowerment::IsEmpowered(state));
}

TEST(LFGEmpowerment_party_leader_is_empowered)
{
    LFGEmpowerment::State state;
    state.inParty = true;
    state.isPartyLeader = true;

    CHECK(LFGEmpowerment::IsEmpowered(state));
}

TEST(LFGEmpowerment_party_member_is_not_empowered)
{
    LFGEmpowerment::State state;
    state.inParty = true;

    CHECK(!LFGEmpowerment::IsEmpowered(state));
}

TEST(LFGEmpowerment_lfd_party_member_is_empowered)
{
    LFGEmpowerment::State state;
    state.inParty = true;
    state.hasLfgRestrictions = true;

    CHECK(LFGEmpowerment::IsEmpowered(state));
}

TEST(LFGEmpowerment_lfd_raid_assistant_is_empowered)
{
    LFGEmpowerment::State state;
    state.inRaid = true;
    state.hasLfgRestrictions = true;
    state.isRaidAssistant = true;

    CHECK(LFGEmpowerment::IsEmpowered(state));
}

TEST(LFGEmpowerment_lfd_raid_member_is_not_empowered)
{
    LFGEmpowerment::State state;
    state.inRaid = true;
    state.hasLfgRestrictions = true;

    CHECK(!LFGEmpowerment::IsEmpowered(state));
}

TEST(LFGEmpowerment_raid_leader_beats_missing_restrictions)
{
    // The client's second clause (leader) fires before the third
    // (restrictions), so a raid leader is empowered even when the raid
    // was never LFD-formed.
    LFGEmpowerment::State state;
    state.inRaid = true;
    state.isRaidLeader = true;

    CHECK(LFGEmpowerment::IsEmpowered(state));
}
