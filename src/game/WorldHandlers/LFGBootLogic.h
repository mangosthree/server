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

#ifndef MANGOS_LFGBOOTLOGIC_H
#define MANGOS_LFGBOOTLOGIC_H

/**
 * @file LFGBootLogic.h
 * @brief Threshold selection, tally resolution and remaining-time math for
 *        the LFG boot (vote-kick) system, decoupled from LFGMgr.
 *
 * LFGMgr::AttemptToKickPlayer / CastVote / FinishBootVote (LFGMgrProposal.cpp)
 * collect the votes and call into this header for every decision that does
 * not need a live Player/Group -- the same boundary LFGProposalLogic.h and
 * LFGRoleAssignment.h document: src/tests/CMakeLists.txt only links
 * `proto`/`shared` into mangos_tests, so testable logic has to stay
 * game-free. No Player, no Group, no ObjectGuid -- raw std::uint64_t for
 * guids, matching m2's LFGLogic::ShouldVoteKick.
 *
 * The vote thresholds are server policy -- the client parses the
 * votes-needed field and never reads it back -- so they are isolated here,
 * in one place, and can be revisited without touching the tally or
 * resolution machinery.
 */

#include <cstdint>

namespace LFGBootLogic
{
    /// Votes needed to resolve a boot, by group kind. The client cannot
    /// settle this: it parses the votes-needed field and never reads it
    /// back, so the number is server policy. A five-man kick is a majority
    /// of the group, which is three -- not the four TrinityCore uses.
    enum BootVotesNeeded
    {
        LFD_BOOT_VOTES_NEEDED = 3,
        LFR_BOOT_VOTES_NEEDED = 15
    };

    /// Boot votes a single group may win before the budget is spent.
    enum BootBudget
    {
        BOOT_MAX_KICKS = 2
    };

    /// How a boot vote ended.
    enum BootResolution
    {
        BOOT_PENDING = 0,
        BOOT_PASSED,
        BOOT_FAILED
    };

    /// Running tally of one boot vote.
    struct BootTally
    {
        std::uint32_t total = 0;    ///< answers cast (agree or deny)
        std::uint32_t agree = 0;
        std::uint32_t deny = 0;
    };

    /// Votes needed for this group kind.
    inline std::uint32_t RequiredVotes(bool isRaidFinder)
    {
        return isRaidFinder ? std::uint32_t(LFR_BOOT_VOTES_NEEDED)
                             : std::uint32_t(LFD_BOOT_VOTES_NEEDED);
    }

    /// A player may never start a vote against themselves.
    inline bool ShouldVoteKick(std::uint64_t target, std::uint64_t kicker)
    {
        return target != kicker;
    }

    /// Resolves a tally against the threshold. `>=`, never `==` -- a vote
    /// that arrives after the deciding one must not be silently dropped.
    /// Agree is checked first, so a simultaneous agree/deny majority (only
    /// reachable with a tiny threshold) resolves as passed.
    inline BootResolution Resolve(BootTally const& tally, std::uint32_t votesNeeded)
    {
        if (tally.agree >= votesNeeded)
        {
            return BOOT_PASSED;
        }

        if (tally.deny >= votesNeeded)
        {
            return BOOT_FAILED;
        }

        return BOOT_PENDING;
    }

    /// Seconds left, clamped at 0. Mirrors m2's LFGLogic::RemainingSeconds
    /// semantics so the two forks stay comparable: 0 when duration <= 0 or
    /// elapsed has reached duration; a negative elapsed (clock skew) is
    /// treated as no time having passed yet rather than extending the timer.
    inline std::int64_t RemainingSeconds(std::int64_t start, std::int64_t duration,
                                         std::int64_t now)
    {
        if (duration <= 0)
        {
            return 0;
        }

        std::int64_t elapsed = now - start;
        if (elapsed < 0)
        {
            elapsed = 0;
        }

        std::int64_t const remaining = duration - elapsed;
        return (remaining > 0) ? remaining : 0;
    }
}

#endif
