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

#include "LFGPackets.h"
#include "Opcodes.h"

/**
 * @file
 * @brief Exact-byte fixtures for the ten flat (disp1) 4.3.4 LFG packets.
 *
 * Every layout was cross-checked against client-truth/binary/Wow-64.c and
 * re-verified live against the IDA database during this session (see
 * LFGPackets.h's per-packet comments for the reader function each came
 * from). None of the ten flat packets carries a packed GUID -- every GUID
 * field here is raw uint64 -- so the skip-and-XOR path that WriteByteSeq /
 * ReadByteSeq exist for is exercised in ByteBufferTest.cpp instead, not
 * here. What these fixtures do cover is a raw GUID containing an internal
 * zero byte, to prove the plain uint64 write does not special-case it the
 * way the packed path would.
 */

// ---------------------------------------------------------------------
// SMSG_LFG_ROLE_CHECK_UPDATE (0x0336)
// ---------------------------------------------------------------------

TEST(LFGPackets_RoleCheckUpdate_exact_fixture)
{
    LFGPackets::RoleCheckUpdate data;
    data.status = 2;
    data.isBeginning = true;
    data.dungeons = { 285, 187 };
    data.members = {
        { 0x1122334455667788ULL, true, 14, 85 },
        { 0x0011223344556677ULL, false, 8, 80 }   // leading zero byte, raw GUID
    };

    WorldPacket packet(SMSG_LFG_ROLE_CHECK_UPDATE, 64);
    REQUIRE(LFGPackets::BuildRoleCheckUpdate(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x02,0x00,0x00,0x00,
        0x01,
        0x02,
        0x1D,0x01,0x00,0x00,
        0xBB,0x00,0x00,0x00,
        0x02,
        0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,
        0x01,
        0x0E,0x00,0x00,0x00,
        0x55,
        0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00,
        0x00,
        0x08,0x00,0x00,0x00,
        0x50
    });
}

TEST(LFGPackets_RoleCheckUpdate_rejects_too_many_dungeons)
{
    LFGPackets::RoleCheckUpdate data;
    data.dungeons.resize(256);

    WorldPacket packet(SMSG_LFG_ROLE_CHECK_UPDATE, 0);
    CHECK(!LFGPackets::BuildRoleCheckUpdate(packet, data));
    CHECK(packet.empty());
}

// ---------------------------------------------------------------------
// SMSG_ROLE_CHOSEN (0x6A26) -- Opcodes.h names it without the LFG infix
// ---------------------------------------------------------------------

TEST(LFGPackets_RoleChosen_accepted)
{
    WorldPacket packet(SMSG_ROLE_CHOSEN, 16);
    LFGPackets::BuildRoleChosen(packet, 0x0807060504030201ULL, 0x0000000A);

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
        0x01,
        0x0A,0x00,0x00,0x00
    });
}

TEST(LFGPackets_RoleChosen_no_roles_is_not_accepted)
{
    WorldPacket packet(SMSG_ROLE_CHOSEN, 16);
    LFGPackets::BuildRoleChosen(packet, 0x1122334455667788ULL, 0);

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,
        0x00,
        0x00,0x00,0x00,0x00
    });
}

// ---------------------------------------------------------------------
// SMSG_LFG_TELEPORT_DENIED (0x0E14)
// ---------------------------------------------------------------------

TEST(LFGPackets_TeleportDenied)
{
    WorldPacket packet(SMSG_LFG_TELEPORT_DENIED, 4);
    LFGPackets::BuildTeleportDenied(packet, 2);

    CHECK_BYTES(packet.contents(), packet.size(), { 0x02,0x00,0x00,0x00 });
}

// ---------------------------------------------------------------------
// SMSG_LFG_BOOT_PROPOSAL_UPDATE (0x0F05)
// ---------------------------------------------------------------------

TEST(LFGPackets_BootProposalUpdate_exact_fixture)
{
    LFGPackets::BootProposalUpdate data;
    data.voteInProgress = true;
    data.myVoteCompleted = true;
    data.myVote = false;
    data.voteEcho = true;
    data.target = 0x0011223344556677ULL;   // leading zero byte, raw GUID
    data.totalVotes = 5;
    data.bootVotes = 3;
    data.timeLeft = 42;
    data.votesNeeded = 3;
    data.reason = "AFK";

    WorldPacket packet(SMSG_LFG_BOOT_PROPOSAL_UPDATE, 64);
    REQUIRE(LFGPackets::BuildBootProposalUpdate(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x01,
        0x01,
        0x00,
        0x01,
        0x77,0x66,0x55,0x44,0x33,0x22,0x11,0x00,
        0x05,0x00,0x00,0x00,
        0x03,0x00,0x00,0x00,
        0x2A,0x00,0x00,0x00,
        0x03,0x00,0x00,0x00,
        0x41,0x46,0x4B,0x00
    });
}

TEST(LFGPackets_BootProposalUpdate_empty_reason_is_just_the_terminator)
{
    LFGPackets::BootProposalUpdate data;
    // all flags false, target/votes zero: focus is entirely on reason.

    WorldPacket packet(SMSG_LFG_BOOT_PROPOSAL_UPDATE, 64);
    REQUIRE(LFGPackets::BuildBootProposalUpdate(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00
    });
}

TEST(LFGPackets_BootProposalUpdate_reason_length_boundary)
{
    LFGPackets::BootProposalUpdate valid;
    valid.reason.assign(255, 'x');   // 255 chars + NUL == the 256-byte cap
    WorldPacket okPacket(SMSG_LFG_BOOT_PROPOSAL_UPDATE, 300);
    CHECK(LFGPackets::BuildBootProposalUpdate(okPacket, valid));

    LFGPackets::BootProposalUpdate tooLong;
    tooLong.reason.assign(256, 'x');
    WorldPacket rejected(SMSG_LFG_BOOT_PROPOSAL_UPDATE, 0);
    CHECK(!LFGPackets::BuildBootProposalUpdate(rejected, tooLong));
    CHECK(rejected.empty());
}

// ---------------------------------------------------------------------
// SMSG_LFG_PARTY_INFO (0x2325)
// ---------------------------------------------------------------------

TEST(LFGPackets_PartyInfo_exact_fixture)
{
    LFGPackets::PartyInfo data;
    data.members.resize(2);
    data.members[0].guid = 0x1122334455667788ULL;
    data.members[0].lockedDungeons = { { 285, 4, 10, 85 } };
    data.members[1].guid = 0x0011223344556600ULL;   // two zero bytes, raw GUID
    // members[1] has no locked dungeons.

    WorldPacket packet(SMSG_LFG_PARTY_INFO, 64);
    REQUIRE(LFGPackets::BuildPartyInfo(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x02,
        0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,
        0x01,0x00,0x00,0x00,
        0x1D,0x01,0x00,0x00,
        0x04,0x00,0x00,0x00,
        0x0A,0x00,0x00,0x00,
        0x55,0x00,0x00,0x00,
        0x00,0x66,0x55,0x44,0x33,0x22,0x11,0x00,
        0x00,0x00,0x00,0x00
    });
}

TEST(LFGPackets_PartyInfo_rejects_too_many_members)
{
    LFGPackets::PartyInfo data;
    data.members.resize(256);

    WorldPacket packet(SMSG_LFG_PARTY_INFO, 0);
    CHECK(!LFGPackets::BuildPartyInfo(packet, data));
    CHECK(packet.empty());
}

// ---------------------------------------------------------------------
// SMSG_LFG_PLAYER_REWARD (0x6834)
// ---------------------------------------------------------------------

TEST(LFGPackets_PlayerReward_exact_fixture)
{
    LFGPackets::PlayerReward data;
    data.queuedSlot = 285;
    data.actualSlot = 187;
    data.rewardMoney = 12345;
    data.addedXp = 6789;
    data.items = {
        { 45, 100, 1, false },
        { 0, 0, 5, true }   // zero item id -- a plain field, not a GUID
    };

    WorldPacket packet(SMSG_LFG_PLAYER_REWARD, 64);
    REQUIRE(LFGPackets::BuildPlayerReward(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x1D,0x01,0x00,0x00,
        0xBB,0x00,0x00,0x00,
        0x39,0x30,0x00,0x00,
        0x85,0x1A,0x00,0x00,
        0x02,
        0x2D,0x00,0x00,0x00,
        0x64,0x00,0x00,0x00,
        0x01,0x00,0x00,0x00,
        0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x05,0x00,0x00,0x00,
        0x01
    });
}

TEST(LFGPackets_PlayerReward_rejects_too_many_items)
{
    LFGPackets::PlayerReward data;
    data.items.resize(256);

    WorldPacket packet(SMSG_LFG_PLAYER_REWARD, 0);
    CHECK(!LFGPackets::BuildPlayerReward(packet, data));
    CHECK(packet.empty());
}

// ---------------------------------------------------------------------
// SMSG_LFG_OFFER_CONTINUE (0x6B27)
// ---------------------------------------------------------------------

TEST(LFGPackets_OfferContinue)
{
    WorldPacket packet(SMSG_LFG_OFFER_CONTINUE, 4);
    LFGPackets::BuildOfferContinue(packet, 285);

    CHECK_BYTES(packet.contents(), packet.size(), { 0x1D,0x01,0x00,0x00 });
}

// ---------------------------------------------------------------------
// SMSG_OPEN_LFG_DUNGEON_FINDER (0x2C37)
// ---------------------------------------------------------------------

TEST(LFGPackets_OpenDungeonFinder)
{
    WorldPacket packet(SMSG_OPEN_LFG_DUNGEON_FINDER, 4);
    LFGPackets::BuildOpenDungeonFinder(packet, 71);

    CHECK_BYTES(packet.contents(), packet.size(), { 0x47,0x00,0x00,0x00 });
}

// ---------------------------------------------------------------------
// SMSG_LFG_DISABLED (0x0815)
// ---------------------------------------------------------------------

TEST(LFGPackets_Disabled_has_no_payload)
{
    WorldPacket packet(SMSG_LFG_DISABLED, 0);
    LFGPackets::BuildDisabled(packet);

    CHECK(packet.empty());
}

// ---------------------------------------------------------------------
// SMSG_LFG_PLAYER_INFO (0x4B36) -- the largest of the ten
// ---------------------------------------------------------------------

TEST(LFGPackets_PlayerInfo_exact_fixture)
{
    LFGPackets::LFGRandomDungeonEntry entry;
    entry.slot = 285;
    entry.firstReward = true;
    entry.completionQuantity = 1;
    entry.completionLimit = 2;
    entry.completionCurrencyId = 3;
    entry.specificQuantity = 4;
    entry.specificLimit = 5;
    entry.overallQuantity = 6;
    entry.overallLimit = 7;
    entry.purseWeeklyQuantity = 8;
    entry.purseWeeklyLimit = 9;
    entry.purseQuantity = 10;
    entry.purseLimit = 11;
    entry.quantity = 12;
    entry.completedMask = 0x0000000F;
    entry.shortageEligible = true;
    // tiers[0] and [2] left at roleMask == 0 -- must skip their reward block.
    entry.shortageTiers[1].roleMask = 4;               // healer
    entry.shortageTiers[1].money = 100;
    entry.shortageTiers[1].xp = 200;
    entry.shortageTiers[1].items = { { 55, 66, 1, false } };
    entry.rewardMoney = 500;
    entry.rewardXp = 600;
    entry.items = { { 77, 0, 2, true } };

    LFGPackets::PlayerInfo data;
    data.randomDungeons = { entry };
    data.lockedDungeons = { { 999, 1, 2, 3 } };

    WorldPacket packet(SMSG_LFG_PLAYER_INFO, 256);
    REQUIRE(LFGPackets::BuildPlayerInfo(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x01,
        0x1D,0x01,0x00,0x00,
        0x01,
        0x01,0x00,0x00,0x00,
        0x02,0x00,0x00,0x00,
        0x03,0x00,0x00,0x00,
        0x04,0x00,0x00,0x00,
        0x05,0x00,0x00,0x00,
        0x06,0x00,0x00,0x00,
        0x07,0x00,0x00,0x00,
        0x08,0x00,0x00,0x00,
        0x09,0x00,0x00,0x00,
        0x0A,0x00,0x00,0x00,
        0x0B,0x00,0x00,0x00,
        0x0C,0x00,0x00,0x00,
        0x0F,0x00,0x00,0x00,
        0x01,
        0x00,0x00,0x00,0x00,
        0x04,0x00,0x00,0x00,
        0x64,0x00,0x00,0x00,
        0xC8,0x00,0x00,0x00,
        0x01,
        0x37,0x00,0x00,0x00,
        0x42,0x00,0x00,0x00,
        0x01,0x00,0x00,0x00,
        0x00,
        0x00,0x00,0x00,0x00,
        0xF4,0x01,0x00,0x00,
        0x58,0x02,0x00,0x00,
        0x01,
        0x4D,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x02,0x00,0x00,0x00,
        0x01,
        0x01,0x00,0x00,0x00,
        0xE7,0x03,0x00,0x00,
        0x01,0x00,0x00,0x00,
        0x02,0x00,0x00,0x00,
        0x03,0x00,0x00,0x00
    });
}

TEST(LFGPackets_PlayerInfo_empty_is_two_zero_counts)
{
    LFGPackets::PlayerInfo data;

    WorldPacket packet(SMSG_LFG_PLAYER_INFO, 8);
    REQUIRE(LFGPackets::BuildPlayerInfo(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x00,
        0x00,0x00,0x00,0x00
    });
}

TEST(LFGPackets_PlayerInfo_rejects_too_many_random_dungeons)
{
    LFGPackets::PlayerInfo data;
    data.randomDungeons.resize(256);

    WorldPacket packet(SMSG_LFG_PLAYER_INFO, 0);
    CHECK(!LFGPackets::BuildPlayerInfo(packet, data));
    CHECK(packet.empty());
}

TEST(LFGPackets_PlayerInfo_rejects_too_many_items_in_one_entry)
{
    LFGPackets::LFGRandomDungeonEntry entry;
    entry.items.resize(256);

    LFGPackets::PlayerInfo data;
    data.randomDungeons = { entry };

    WorldPacket packet(SMSG_LFG_PLAYER_INFO, 0);
    CHECK(!LFGPackets::BuildPlayerInfo(packet, data));
    CHECK(packet.empty());
}

// ---------------------------------------------------------------------
// RideTicket -- infrastructure for Phase 3b, not consumed by anything above
// ---------------------------------------------------------------------

TEST(LFGPackets_RideTicket_default_state)
{
    RideTicket ticket;

    CHECK_EQ(int(ticket.time), 0);
    CHECK(ticket.type == RIDE_TYPE_NONE);
    CHECK_EQ(int(ticket.id), 0);
    CHECK_EQ(int(ticket.requesterGuid), 0);
}

// ---------------------------------------------------------------------
// Opcode regression -- pins Phase 2's fixed values against silent drift.
// ---------------------------------------------------------------------

TEST(LFGPackets_opcode_values)
{
    CHECK(uint32(SMSG_LFG_ROLE_CHECK_UPDATE) == 0x0336u);
    CHECK(uint32(SMSG_LFG_TELEPORT_DENIED) == 0x0E14u);
    CHECK(uint32(SMSG_LFG_BOOT_PROPOSAL_UPDATE) == 0x0F05u);
    CHECK(uint32(SMSG_LFG_PARTY_INFO) == 0x2325u);
    CHECK(uint32(SMSG_LFG_PLAYER_INFO) == 0x4B36u);
    CHECK(uint32(SMSG_LFG_PLAYER_REWARD) == 0x6834u);
    CHECK(uint32(SMSG_ROLE_CHOSEN) == 0x6A26u);
    CHECK(uint32(SMSG_LFG_OFFER_CONTINUE) == 0x6B27u);
    CHECK(uint32(SMSG_OPEN_LFG_DUNGEON_FINDER) == 0x2C37u);
    CHECK(uint32(SMSG_LFG_DISABLED) == 0x0815u);
}
