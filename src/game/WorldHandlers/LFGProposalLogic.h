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

#ifndef MANGOS_LFGPROPOSALLOGIC_H
#define MANGOS_LFGPROPOSALLOGIC_H

/**
 * @file LFGProposalLogic.h
 * @brief Decides who gets removed and who gets re-queued when a dungeon
 *        proposal fails, decoupled from LFGMgr.
 *
 * A failed proposal (decline or 45-second expiry) has to sort its members
 * into two piles: the decliner and everyone who shared their original party
 * leaves, everyone else goes back to the queue. LFGMgr::RemoveProposal
 * (LFGMgrProposal.cpp) collects the participants and calls ResolveFailure();
 * the decision lives here so it can be unit tested without linking `game` --
 * src/tests/CMakeLists.txt only links `proto`/`shared` into mangos_tests, the
 * same boundary LFGLockReason.h and LFGRoleAssignment.h document.
 *
 * Numeric parity with LFGMgr.h's LFGProposalAnswer is kept in sync by hand
 * (precedent: LFGRoleAssignment.h) -- this header stays game-free.
 */

#include "Platform/Define.h"

#include <map>
#include <utility>
#include <vector>

namespace LFGProposalLogic
{
    enum Answer
    {
        ANSWER_PENDING = -1,
        ANSWER_DENY    = 0,
        ANSWER_AGREE   = 1
    };

    enum Disposition
    {
        DISPOSITION_REQUEUE = 0,   ///< survivor: goes back to the queue
        DISPOSITION_REMOVE  = 1    ///< decliner, or shares a unit with one
    };

    /// One proposal participant, reduced to what the outcome depends on.
    struct Member
    {
        uint64 guid = 0;       ///< player guid, raw
        uint64 unitGuid = 0;   ///< original party guid, or guid when solo
        int answer = ANSWER_PENDING;
    };

    /// True when every member has agreed.
    inline bool AllAgreed(std::vector<Member> const& members)
    {
        for (Member const& member : members)
        {
            if (member.answer != ANSWER_AGREE)
            {
                return false;
            }
        }

        return !members.empty();
    }

    /// Resolves a failed proposal. On expiry every PENDING answer becomes
    /// DENY (mutated in place so the caller can report it). A member is
    /// removed when anyone in their unit denied; everyone else re-queues.
    inline void ResolveFailure(std::vector<Member>& members, bool expired,
                               std::vector<Disposition>& dispositions)
    {
        if (expired)
        {
            for (Member& member : members)
            {
                if (member.answer == ANSWER_PENDING)
                {
                    member.answer = ANSWER_DENY;
                }
            }
        }

        dispositions.assign(members.size(), DISPOSITION_REQUEUE);
        for (size_t i = 0; i < members.size(); ++i)
        {
            if (members[i].answer != ANSWER_DENY)
            {
                continue;
            }

            for (size_t j = 0; j < members.size(); ++j)
            {
                if (members[j].unitGuid == members[i].unitGuid)
                {
                    dispositions[j] = DISPOSITION_REMOVE;
                }
            }
        }
    }

    /// Queue key for the surviving members: keepKey when its unit survives,
    /// else the smallest surviving unit guid; 0 when nobody survives.
    inline uint64 PickRequeueKey(std::vector<Member> const& members,
                                 std::vector<Disposition> const& dispositions,
                                 uint64 keepKey)
    {
        uint64 best = 0;
        for (size_t i = 0; i < members.size(); ++i)
        {
            if (dispositions[i] != DISPOSITION_REQUEUE)
            {
                continue;
            }

            if (members[i].unitGuid == keepKey)
            {
                return keepKey;
            }

            if (best == 0 || members[i].unitGuid < best)
            {
                best = members[i].unitGuid;
            }
        }

        return best;
    }

    /// One proposal player's own party, flattened: (player guid, party guid).
    /// A solo player's party guid is 0.
    typedef std::pair<uint64, uint64> PlayerGroupPair;

    /// The pre-existing party contributing the most members to a proposal --
    /// so CreateDungeonGroup can keep that Group object and its leader
    /// instead of tearing the premade down and rebuilding it (spec 7c,
    /// section 3.3). Ties break on the lowest guid; 0 when nobody in
    /// \a playerToGroup has a party (every player's second is 0).
    inline uint64 PickHostGroup(std::vector<PlayerGroupPair> const& playerToGroup)
    {
        std::map<uint64, uint32> counts;
        for (PlayerGroupPair const& entry : playerToGroup)
        {
            if (entry.second != 0)
            {
                ++counts[entry.second];
            }
        }

        uint64 best = 0;
        uint32 bestCount = 0;
        for (std::map<uint64, uint32>::const_iterator itr = counts.begin();
             itr != counts.end(); ++itr)
        {
            // Most members wins; lowest guid breaks the tie deterministically.
            if (itr->second > bestCount ||
                (itr->second == bestCount && best != 0 && itr->first < best))
            {
                best = itr->first;
                bestCount = itr->second;
            }
        }

        return best;
    }
}

#endif
