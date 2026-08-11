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

#ifndef MANGOS_LFGEMPOWERMENT_H
#define MANGOS_LFGEMPOWERMENT_H

/**
 * Server-side mirror of the client's LFD_IsEmpowered (UIParent.lua:3649).
 *
 * The client only greys the control; JoinLFG (0x140700860) carries no
 * leadership test, so the server is the only enforcement point.
 */
namespace LFGEmpowerment
{
    /// Inputs mirroring the six client predicates, in the client's order.
    struct State
    {
        State()
            : inParty(false),
              inRaid(false),
              isPartyLeader(false),
              isRaidLeader(false),
              isRaidAssistant(false),
              hasLfgRestrictions(false)
        {
        }

        bool inParty;              ///< GetNumPartyMembers() > 0
        bool inRaid;               ///< GetNumRaidMembers() > 0
        bool isPartyLeader;        ///< IsPartyLeader()
        bool isRaidLeader;         ///< IsRaidLeader()
        bool isRaidAssistant;      ///< IsRaidOfficer()
        bool hasLfgRestrictions;   ///< group carries GROUPTYPE_LFD_RESTRICTED
    };

    /// True when this player may queue or dequeue for the group.
    inline bool IsEmpowered(State const& s)
    {
        if (!s.inParty && !s.inRaid)
        {
            return true;
        }

        if (s.isPartyLeader || s.isRaidLeader)
        {
            return true;
        }

        if (s.hasLfgRestrictions && (!s.inRaid || s.isRaidAssistant))
        {
            return true;
        }

        return false;
    }
}

#endif
