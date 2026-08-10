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

#ifndef MANGOS_LFGDUNGEONRESOLUTION_H
#define MANGOS_LFGDUNGEONRESOLUTION_H

/**
 * @file LFGDungeonResolution.h
 * @brief Expands a random-dungeon row (LFGDungeons.dbc typeID 6) into the
 *        concrete dungeons it can resolve to, and picks one.
 *
 * The bucket rule is group_id union LFGDungeonsGroupingmap.dbc -- group_id
 * alone cannot express Random Hour of Twilight 434 (LFD_PHASE7B_SPEC.md
 * section 1.1 C2). LFGMgr::JoinLFG (LFGMgrQueue.cpp) expands at join time so
 * the queue entry stores the pruned concrete set; SendDungeonProposal
 * (LFGMgrProposal.cpp) draws from it at proposal time.
 *
 * Deliberately self-contained: only Platform/Define.h and the standard
 * library. Same proto-boundary gate as LFGProposalLogic.h / LFGRoleAssignment.h
 * -- src/tests/CMakeLists.txt only links `proto`/`shared` into mangos_tests.
 */

#include "Platform/Define.h"

#include <set>
#include <vector>

namespace LFGDungeonResolution
{
    /// Numeric parity with LFGMgr.h's LfgType, kept in sync by hand -- this
    /// header stays game-free (precedent: LFGProposalLogic.h).
    enum
    {
        TYPE_DUNGEON        = 1,
        TYPE_HEROIC_DUNGEON = 5,
        TYPE_RANDOM_DUNGEON = 6
    };

    /// One LFGDungeons.dbc row, reduced to what expansion depends on.
    struct DungeonRow
    {
        uint32 id = 0;
        uint32 typeID = 0;
        uint32 groupID = 0;
    };

    /// One LFGDungeonsGroupingmap.dbc row.
    struct GroupingRow
    {
        uint32 dungeonID = 0;
        uint32 randomID = 0;
    };

    /// Concrete candidates for one random row: every 5-man sharing its
    /// group_id plus every grouping-map member -- the client's own bucket
    /// definition (LFGDungeonGroup.dbc names the buckets).
    inline void ExpandRandom(DungeonRow const& randomRow,
                             std::vector<DungeonRow> const& dungeons,
                             std::vector<GroupingRow> const& grouping,
                             std::set<uint32>& out)
    {
        out.clear();
        for (DungeonRow const& row : dungeons)
        {
            if (row.typeID != TYPE_DUNGEON && row.typeID != TYPE_HEROIC_DUNGEON)
            {
                continue;
            }

            if (row.groupID != 0 && row.groupID == randomRow.groupID)
            {
                out.insert(row.id);
            }
        }

        for (GroupingRow const& row : grouping)
        {
            if (row.randomID == randomRow.id)
            {
                out.insert(row.dungeonID);
            }
        }
    }

    /// Uniform pick over a candidate set; rng is any random value (the
    /// caller rolls it -- this stays deterministic and testable). 0 on empty.
    inline uint32 Pick(std::set<uint32> const& candidates, uint32 rng)
    {
        if (candidates.empty())
        {
            return 0;
        }

        std::set<uint32>::const_iterator itr = candidates.begin();
        std::advance(itr, size_t(rng % uint32(candidates.size())));
        return *itr;
    }
}

#endif
