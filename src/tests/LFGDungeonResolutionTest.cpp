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

#include "LFGDungeonResolution.h"

/**
 * @file LFGDungeonResolutionTest.cpp
 * @brief Coverage for LFGDungeonResolution -- expanding a random-dungeon row
 * into its concrete candidates (group_id union LFGDungeonsGroupingmap.dbc)
 * and the uniform pick over that set. The fixture below mirrors the real
 * 15595 LFGDungeons.dbc / LFGDungeonsGroupingmap.dbc data (LFD_PHASE7B_SPEC.md
 * section 1.1 C2 / V7), reduced to a representative subset.
 */

namespace
{
    using namespace LFGDungeonResolution;

    DungeonRow D(uint32 id, uint32 typeID, uint32 groupID)
    {
        DungeonRow row;
        row.id = id;
        row.typeID = typeID;
        row.groupID = groupID;
        return row;
    }

    GroupingRow G(uint32 dungeonID, uint32 randomID)
    {
        GroupingRow row;
        row.dungeonID = dungeonID;
        row.randomID = randomID;
        return row;
    }

    /// Representative LFGDungeons.dbc rows: Cata heroics (grp 12, including
    /// the Hour of Twilight trio which ALSO carries grp 12 via group_id --
    /// V7), Cata normals (grp 13), a seasonal boss (grp 11), an LFR wing
    /// (group_id 0, V8), a raid sharing grp 12's id, and a random row that
    /// happens to share grp 12.
    std::vector<DungeonRow> Dungeons()
    {
        std::vector<DungeonRow> rows;
        rows.push_back(D(319, TYPE_HEROIC_DUNGEON, 12));   // Vortex Pinnacle
        rows.push_back(D(324, TYPE_HEROIC_DUNGEON, 12));   // Throne of the Tides
        rows.push_back(D(326, TYPE_HEROIC_DUNGEON, 12));   // The Deadmines
        rows.push_back(D(435, TYPE_HEROIC_DUNGEON, 12));   // End Time
        rows.push_back(D(437, TYPE_HEROIC_DUNGEON, 12));   // Well of Eternity
        rows.push_back(D(439, TYPE_HEROIC_DUNGEON, 12));   // Hour of Twilight
        rows.push_back(D(302, TYPE_DUNGEON, 13));          // Cata normal
        rows.push_back(D(303, TYPE_DUNGEON, 13));          // Cata normal
        rows.push_back(D(285, TYPE_DUNGEON, 11));          // The Headless Horseman (seasonal)
        rows.push_back(D(416, TYPE_DUNGEON, 0));           // Siege of Wyrmrest Temple (LFR, grp 0)
        rows.push_back(D(800, 2, 12));                     // typeID 2 (raid) sharing grp 12
        rows.push_back(D(999, TYPE_RANDOM_DUNGEON, 12));   // random row sharing grp 12
        return rows;
    }

    /// LFGDungeonsGroupingmap.dbc: the Hour of Twilight trio maps to BOTH
    /// Random Cataclysm Heroic (301) and Random Hour of Twilight (434).
    std::vector<GroupingRow> Grouping()
    {
        std::vector<GroupingRow> rows;
        rows.push_back(G(435, 301));
        rows.push_back(G(435, 434));
        rows.push_back(G(437, 301));
        rows.push_back(G(437, 434));
        rows.push_back(G(439, 301));
        rows.push_back(G(439, 434));
        return rows;
    }

    DungeonRow Random300() { return D(300, TYPE_RANDOM_DUNGEON, 13); }  ///< Random Cataclysm Dungeon
    DungeonRow Random301() { return D(301, TYPE_RANDOM_DUNGEON, 12); }  ///< Random Cataclysm Heroic
    DungeonRow Random434() { return D(434, TYPE_RANDOM_DUNGEON, 33); }  ///< Random Hour of Twilight Heroic
}

TEST(LFGDungeonResolution_cata_heroic_bucket_by_group)
{
    std::set<uint32> out;
    ExpandRandom(Random301(), Dungeons(), Grouping(), out);

    REQUIRE(out.size() == 6);
    CHECK(out.count(319));
    CHECK(out.count(324));
    CHECK(out.count(326));
    CHECK(out.count(435));
    CHECK(out.count(437));
    CHECK(out.count(439));
}

TEST(LFGDungeonResolution_cata_normal_bucket_by_group)
{
    std::set<uint32> out;
    ExpandRandom(Random300(), Dungeons(), Grouping(), out);

    REQUIRE(out.size() == 2);
    CHECK(out.count(302));
    CHECK(out.count(303));
}

TEST(LFGDungeonResolution_hour_of_twilight_via_grouping_map_only)
{
    std::set<uint32> out;
    ExpandRandom(Random434(), Dungeons(), Grouping(), out);

    REQUIRE(out.size() == 3);
    CHECK(out.count(435));
    CHECK(out.count(437));
    CHECK(out.count(439));
}

TEST(LFGDungeonResolution_grouping_map_duplicates_dedupe)
{
    // 435/437/439 satisfy Random 301 through group_id AND the grouping map;
    // the set must still hold exactly one copy of each (six total, not nine).
    std::set<uint32> out;
    ExpandRandom(Random301(), Dungeons(), Grouping(), out);

    CHECK(out.size() == 6);
}

TEST(LFGDungeonResolution_random_rows_never_join_a_bucket)
{
    std::set<uint32> out;
    ExpandRandom(Random301(), Dungeons(), Grouping(), out);

    CHECK(!out.count(999));
}

TEST(LFGDungeonResolution_raid_and_zero_group_rows_excluded)
{
    std::set<uint32> out;
    ExpandRandom(Random301(), Dungeons(), Grouping(), out);

    CHECK(!out.count(800));
    CHECK(!out.count(416));
}

TEST(LFGDungeonResolution_seasonal_group_isolated)
{
    std::set<uint32> heroic;
    ExpandRandom(Random301(), Dungeons(), Grouping(), heroic);
    CHECK(!heroic.count(285));

    std::set<uint32> normal;
    ExpandRandom(Random300(), Dungeons(), Grouping(), normal);
    CHECK(!normal.count(285));
}

TEST(LFGDungeonResolution_pick_empty_returns_zero)
{
    std::set<uint32> empty;
    CHECK(Pick(empty, 0) == 0);
    CHECK(Pick(empty, 42) == 0);
}

TEST(LFGDungeonResolution_pick_is_rng_mod_size)
{
    std::set<uint32> candidates;
    candidates.insert(319);
    candidates.insert(324);
    candidates.insert(326);
    candidates.insert(435);
    candidates.insert(437);
    candidates.insert(439);

    CHECK(Pick(candidates, 0) == 319);
    CHECK(Pick(candidates, 5) == 439);
    CHECK(Pick(candidates, 6) == 319);
    CHECK(Pick(candidates, 7) == 324);
}

TEST(LFGDungeonResolution_pick_singleton_is_identity)
{
    std::set<uint32> candidates;
    candidates.insert(322);   // e.g. Grim Batol -- a specific selection

    CHECK(Pick(candidates, 0) == 322);
    CHECK(Pick(candidates, 99) == 322);
}
