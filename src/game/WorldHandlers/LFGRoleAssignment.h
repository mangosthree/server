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

#ifndef MANGOS_LFGROLEASSIGNMENT_H
#define MANGOS_LFGROLEASSIGNMENT_H

/**
 * @file LFGRoleAssignment.h
 * @brief Seats queued players into one role each, decoupled from LFGMgr.
 *
 * A player may tick several roles in the Dungeon Finder; the server has to
 * decide which single role each of them actually fills before it can say how
 * many of each role the group still needs. LFGMgr::CountAssignedRoles
 * (LFGMgr.cpp) collects the selected masks and calls Assign(); the decision
 * lives here so it can be unit tested without linking `game` --
 * src/tests/CMakeLists.txt only links `proto`/`shared` into mangos_tests, the
 * same boundary LFGLockReason.h and LFGPackets.h document.
 *
 * The role bits below are the wire contract and carry the same values as
 * LFGMgr.h's LFGRoles enum. They are declared again here, independently,
 * rather than shared, for the reason LFGLockReason.h gives: LFGMgr.h pulls in
 * Group.h and the rest of the game headers this boundary excludes.
 */

#include "Platform/Define.h"

#include <vector>

namespace LFGRoleAssignment
{
    enum Role
    {
        ROLE_NONE   = 0x00,
        ROLE_LEADER = 0x01,
        ROLE_TANK   = 0x02,
        ROLE_HEALER = 0x04,
        ROLE_DAMAGE = 0x08
    };

    /// How many of each role ended up seated.
    struct Counts
    {
        Counts() : tanks(0), healers(0), dps(0), seated(0) {}

        uint8 tanks;
        uint8 healers;
        uint8 dps;
        uint8 seated;   ///< total placed; less than the input size means someone had no free slot
    };

    /**
    * Seats each selected role mask into exactly one role.
    *
    * Players who ticked a single role have no choice and are placed first, so
    * a flexible player can never take a slot someone else needed. The
    * remaining players are then fitted into whatever is still short, scarcest
    * role first (tank, then healer, then damage), which is what stops a
    * tank-or-damage player from being parked in a damage slot while the group
    * still has no tank.
    *
    * \arg \c masks
    *   One entry per queued player; the leader bit is ignored.
    * \arg \c maxTanks \c maxHealers \c maxDps
    *   Slot capacity for the content being queued for (1/1/3 for 5-man).
    * \return
    *   The seated counts. \c seated below \c masks.size() means at least one
    *   player selected no role, or only roles that were already full.
    */
    inline Counts Assign(std::vector<uint8> const& masks, uint8 maxTanks,
                         uint8 maxHealers, uint8 maxDps)
    {
        Counts out;
        std::vector<uint8> flexible;

        for (std::vector<uint8>::const_iterator it = masks.begin();
             it != masks.end(); ++it)
        {
            uint8 mask = *it & ~uint8(ROLE_LEADER);
            switch (mask)
            {
                case ROLE_TANK:
                    if (out.tanks < maxTanks)
                    {
                        ++out.tanks;
                        ++out.seated;
                    }
                    break;
                case ROLE_HEALER:
                    if (out.healers < maxHealers)
                    {
                        ++out.healers;
                        ++out.seated;
                    }
                    break;
                case ROLE_DAMAGE:
                    if (out.dps < maxDps)
                    {
                        ++out.dps;
                        ++out.seated;
                    }
                    break;
                default:
                    if (mask != ROLE_NONE)
                    {
                        flexible.push_back(mask);
                    }
                    break;
            }
        }

        for (std::vector<uint8>::const_iterator it = flexible.begin();
             it != flexible.end(); ++it)
        {
            if ((*it & ROLE_TANK) && out.tanks < maxTanks)
            {
                ++out.tanks;
                ++out.seated;
            }
            else if ((*it & ROLE_HEALER) && out.healers < maxHealers)
            {
                ++out.healers;
                ++out.seated;
            }
            else if ((*it & ROLE_DAMAGE) && out.dps < maxDps)
            {
                ++out.dps;
                ++out.seated;
            }
        }

        return out;
    }
}

#endif
