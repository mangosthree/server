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

#include "LFGProposalLogic.h"

/**
 * @file LFGProposalLogicTest.cpp
 * @brief Coverage for LFGProposalLogic -- who gets dropped and who gets
 * re-queued when a dungeon-ready proposal fails (decline or expiry), and
 * which guid survivors re-queue under.
 */

namespace
{
    using namespace LFGProposalLogic;

    Member M(uint64 guid, uint64 unit, int answer)
    {
        Member m;
        m.guid = guid;
        m.unitGuid = unit ? unit : guid;
        m.answer = answer;
        return m;
    }
}

TEST(LFGProposalLogic_AllAgreed_empty_is_false)
{
    std::vector<Member> members;
    CHECK(!AllAgreed(members));
}

TEST(LFGProposalLogic_AllAgreed_pending_is_false)
{
    std::vector<Member> members;
    members.push_back(M(1, 0, ANSWER_AGREE));
    members.push_back(M(2, 0, ANSWER_AGREE));
    members.push_back(M(3, 0, ANSWER_AGREE));
    members.push_back(M(4, 0, ANSWER_AGREE));
    members.push_back(M(5, 0, ANSWER_PENDING));

    CHECK(!AllAgreed(members));
}

TEST(LFGProposalLogic_AllAgreed_five_agrees_is_true)
{
    std::vector<Member> members;
    for (uint64 i = 1; i <= 5; ++i)
    {
        members.push_back(M(i, 0, ANSWER_AGREE));
    }

    CHECK(AllAgreed(members));
}

TEST(LFGProposalLogic_decline_removes_only_the_decliner)
{
    std::vector<Member> members;
    members.push_back(M(1, 0, ANSWER_DENY));
    members.push_back(M(2, 0, ANSWER_AGREE));
    members.push_back(M(3, 0, ANSWER_AGREE));
    members.push_back(M(4, 0, ANSWER_AGREE));
    members.push_back(M(5, 0, ANSWER_AGREE));

    std::vector<Disposition> dispositions;
    ResolveFailure(members, false, dispositions);

    REQUIRE(dispositions.size() == 5);
    CHECK(dispositions[0] == DISPOSITION_REMOVE);
    CHECK(dispositions[1] == DISPOSITION_REQUEUE);
    CHECK(dispositions[2] == DISPOSITION_REQUEUE);
    CHECK(dispositions[3] == DISPOSITION_REQUEUE);
    CHECK(dispositions[4] == DISPOSITION_REQUEUE);

    CHECK(members[0].answer == ANSWER_DENY);
    CHECK(members[1].answer == ANSWER_AGREE);
}

TEST(LFGProposalLogic_decline_takes_the_whole_party_along)
{
    std::vector<Member> members;
    members.push_back(M(1, 100, ANSWER_DENY));
    members.push_back(M(2, 100, ANSWER_AGREE));
    members.push_back(M(3, 0, ANSWER_AGREE));
    members.push_back(M(4, 0, ANSWER_AGREE));
    members.push_back(M(5, 0, ANSWER_AGREE));

    std::vector<Disposition> dispositions;
    ResolveFailure(members, false, dispositions);

    REQUIRE(dispositions.size() == 5);
    CHECK(dispositions[0] == DISPOSITION_REMOVE);
    CHECK(dispositions[1] == DISPOSITION_REMOVE);
    CHECK(dispositions[2] == DISPOSITION_REQUEUE);
    CHECK(dispositions[3] == DISPOSITION_REQUEUE);
    CHECK(dispositions[4] == DISPOSITION_REQUEUE);
}

TEST(LFGProposalLogic_outsider_decline_keeps_the_party)
{
    std::vector<Member> members;
    members.push_back(M(1, 100, ANSWER_AGREE));
    members.push_back(M(2, 100, ANSWER_AGREE));
    members.push_back(M(3, 0, ANSWER_DENY));
    members.push_back(M(4, 0, ANSWER_AGREE));
    members.push_back(M(5, 0, ANSWER_AGREE));

    std::vector<Disposition> dispositions;
    ResolveFailure(members, false, dispositions);

    REQUIRE(dispositions.size() == 5);
    CHECK(dispositions[0] == DISPOSITION_REQUEUE);
    CHECK(dispositions[1] == DISPOSITION_REQUEUE);
    CHECK(dispositions[2] == DISPOSITION_REMOVE);
    CHECK(dispositions[3] == DISPOSITION_REQUEUE);
    CHECK(dispositions[4] == DISPOSITION_REQUEUE);
}

TEST(LFGProposalLogic_expiry_flips_pending_to_deny)
{
    std::vector<Member> members;
    members.push_back(M(1, 0, ANSWER_AGREE));
    members.push_back(M(2, 0, ANSWER_AGREE));
    members.push_back(M(3, 0, ANSWER_AGREE));
    members.push_back(M(4, 0, ANSWER_AGREE));
    members.push_back(M(5, 0, ANSWER_PENDING));

    std::vector<Disposition> dispositions;
    ResolveFailure(members, true, dispositions);

    REQUIRE(dispositions.size() == 5);
    CHECK(dispositions[0] == DISPOSITION_REQUEUE);
    CHECK(dispositions[1] == DISPOSITION_REQUEUE);
    CHECK(dispositions[2] == DISPOSITION_REQUEUE);
    CHECK(dispositions[3] == DISPOSITION_REQUEUE);
    CHECK(dispositions[4] == DISPOSITION_REMOVE);

    CHECK(members[4].answer == ANSWER_DENY);
    CHECK(members[0].answer == ANSWER_AGREE);
}

TEST(LFGProposalLogic_expiry_everyone_pending_removes_all)
{
    std::vector<Member> members;
    for (uint64 i = 1; i <= 5; ++i)
    {
        members.push_back(M(i, 0, ANSWER_PENDING));
    }

    std::vector<Disposition> dispositions;
    ResolveFailure(members, true, dispositions);

    REQUIRE(dispositions.size() == 5);
    for (size_t i = 0; i < dispositions.size(); ++i)
    {
        CHECK(dispositions[i] == DISPOSITION_REMOVE);
        CHECK(members[i].answer == ANSWER_DENY);
    }
}

TEST(LFGProposalLogic_requeue_key_keeps_surviving_key)
{
    std::vector<Member> members;
    members.push_back(M(1, 0, ANSWER_DENY));
    members.push_back(M(2, 0, ANSWER_AGREE));
    members.push_back(M(3, 0, ANSWER_AGREE));

    std::vector<Disposition> dispositions;
    ResolveFailure(members, false, dispositions);

    uint64 key = PickRequeueKey(members, dispositions, 2);
    CHECK(key == 2);
}

TEST(LFGProposalLogic_requeue_key_moves_off_removed_key)
{
    std::vector<Member> members;
    members.push_back(M(1, 100, ANSWER_DENY));
    members.push_back(M(2, 100, ANSWER_AGREE));
    members.push_back(M(3, 50, ANSWER_AGREE));
    members.push_back(M(4, 200, ANSWER_AGREE));

    std::vector<Disposition> dispositions;
    ResolveFailure(members, false, dispositions);

    // keepKey (100) was removed along with its whole unit; the smallest
    // surviving unit guid among {50, 200} is 50.
    uint64 key = PickRequeueKey(members, dispositions, 100);
    CHECK(key == 50);
}

TEST(LFGProposalLogic_requeue_key_zero_when_none_survive)
{
    std::vector<Member> members;
    for (uint64 i = 1; i <= 5; ++i)
    {
        members.push_back(M(i, 0, ANSWER_PENDING));
    }

    std::vector<Disposition> dispositions;
    ResolveFailure(members, true, dispositions);

    uint64 key = PickRequeueKey(members, dispositions, 1);
    CHECK(key == 0);
}
