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

TEST(LFGPackets_RoleChosen_leader_bit_alone_is_not_accepted)
{
    // PLAYER_ROLE_LEADER (0x01) with no TANK/HEALER/DAMAGE bit: the client
    // builds a roleList from roles & (2|4|8) and asserts it is non-nil
    // (LFGFrame.lua:166). Accepted = 1 here would crash the default UI.
    WorldPacket packet(SMSG_ROLE_CHOSEN, 16);
    LFGPackets::BuildRoleChosen(packet, 0x1122334455667788ULL, 0x01);

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,
        0x00,
        0x01,0x00,0x00,0x00
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
    data.votePassed = true;
    data.myVoteCompleted = false;
    data.myVote = true;
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

TEST(LFGPackets_BootProposalUpdate_four_byte_prefix)
{
    // C-5 regression guard: the legacy sender wrote three leading bytes
    // where the client reads four. Pin all four flag bytes with a
    // distinguishable 1/0/1/0 pattern so a dropped byte fails this test.
    LFGPackets::BootProposalUpdate data;
    data.voteInProgress = true;
    data.votePassed = false;
    data.myVoteCompleted = true;
    data.myVote = false;

    WorldPacket packet(SMSG_LFG_BOOT_PROPOSAL_UPDATE, 64);
    REQUIRE(LFGPackets::BuildBootProposalUpdate(packet, data));

    CHECK(packet.size() >= 4);
    CHECK_BYTES(packet.contents(), 4, { 0x01, 0x00, 0x01, 0x00 });
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

    // Phase 3b
    CHECK(uint32(SMSG_LFG_QUEUE_STATUS) == 0x78B4u);
    CHECK(uint32(SMSG_LFG_UPDATE_STATUS) == 0x31A4u);
    CHECK(uint32(SMSG_LFG_JOIN_RESULT) == 0x38B6u);
    CHECK(uint32(SMSG_LFG_PROPOSAL_UPDATE) == 0x7DA6u);
    CHECK(uint32(SMSG_LFG_SLOT_INVALID) == 0x54B5u);
    CHECK(uint32(SMSG_LFG_UPDATE_STATUS_NONE) == 0x7CA1u);
    CHECK(uint32(CMSG_LFG_JOIN) == 0x2430u);
    CHECK(uint32(CMSG_LFG_LEAVE) == 0x2433u);
    CHECK(uint32(CMSG_LFG_PROPOSAL_RESULT) == 0x0403u);
    CHECK(uint32(CMSG_LFG_SET_ROLES) == 0x0480u);
    CHECK(uint32(CMSG_SET_LFG_COMMENT) == 0x0530u);
    CHECK(uint32(CMSG_LFG_SET_BOOT_VOTE) == 0x04B3u);
    CHECK(uint32(CMSG_LFG_TELEPORT) == 0x2482u);
    CHECK(uint32(CMSG_LFG_LOCK_INFO_REQUEST) == 0x0412u);
}

// =======================================================================
// Phase 3b -- bit-packed disp2 SMSG builders and CMSG readers.
//
// No LFG_CATA_ANALYSIS.md exists on disk for this phase (checked the
// workspace root -- absent). Every fixture below is instead hand-traced
// bit-by-bit against LFGPackets.h's documented WriteBit/WriteBits/
// WriteByteSeq call order for each packet, which was itself verified
// against the live IDA database (see that header's per-packet comments
// for function addresses and evidence). All guids below use
// 0x0011223344556677 (byte 7 == 0x00) or a similar single-zero-byte
// pattern, matching 3a's convention, specifically to exercise
// WriteByteSeq/ReadByteSeq's skip-and-XOR path -- unlike 3a's flat
// packets, these carry genuinely packed GUIDs, so the path is finally
// reachable here.
// =======================================================================

// ---------------------------------------------------------------------
// SMSG_LFG_QUEUE_STATUS (0x78B4) -- bit region = 8 bits, fixed
// ---------------------------------------------------------------------

TEST(LFGPackets_QueueStatus_exact_fixture)
{
    LFGPackets::QueueStatus data;
    data.ticket.requesterGuid = 0x0011223344556677ULL;   // g[7] == 0x00
    data.ticket.time = 500;
    data.ticket.id = 42;
    data.ticket.type = RIDE_TYPE_LFG;
    data.slot = 285;
    data.avgWaitTimeMe = 700;
    data.avgWaitTime = 400;
    data.avgWaitTimeByRole = { 100, 200, 300 };
    data.lastNeeded = { 1, 2, 3 };
    data.queuedTime = 600;

    WorldPacket packet(SMSG_LFG_QUEUE_STATUS, 64);
    LFGPackets::BuildQueueStatus(packet, data);

    CHECK_BYTES(packet.contents(), packet.size(), {
        0xFB,                           // 8 guid mask bits (byte 7 clear)
        0x76,                           // ByteSeq g0
        0x01, 0x64,0x00,0x00,0x00,      // lastNeeded[0], avgWaitTimeByRole[0]
        0x02, 0xC8,0x00,0x00,0x00,      // [1]
        0x03, 0x2C,0x01,0x00,0x00,      // [2]
        0x32,                           // ByteSeq g4
        0x10,                           // ByteSeq g6
        0x90,0x01,0x00,0x00,            // avgWaitTime=400
        0xF4,0x01,0x00,0x00,            // ticket.time=500
        0x1D,0x01,0x00,0x00,            // slot=285
        0x58,0x02,0x00,0x00,            // queuedTime=600
        0x23,                           // ByteSeq g5
        // ByteSeq g7 SKIPPED -- guid byte is zero
        0x45,                           // ByteSeq g3
        0x2A,0x00,0x00,0x00,            // ticket.id=42
        0x67,                           // ByteSeq g1
        0x54,                           // ByteSeq g2
        0xBC,0x02,0x00,0x00,            // avgWaitTimeMe=700
        0x03,0x00,0x00,0x00             // ticket.type=RIDE_TYPE_LFG
    });
}

TEST(LFGPackets_QueueStatus_bit_region_is_exactly_8_bits)
{
    // All-nonzero guid: all 8 WriteBit calls fire true, and nothing else
    // in this packet is bit-packed -- so byte 0 == 0xFF only if the bit
    // region is exactly one byte (8 bits), matching the plan.
    LFGPackets::QueueStatus data;
    data.ticket.requesterGuid = 0x1122334455667788ULL;

    WorldPacket packet(SMSG_LFG_QUEUE_STATUS, 64);
    LFGPackets::BuildQueueStatus(packet, data);

    REQUIRE(packet.size() > 0);
    CHECK_EQ(uint32(packet.contents()[0]), 0xFFu);
}

// ---------------------------------------------------------------------
// SMSG_LFG_UPDATE_STATUS (0x31A4) -- bit region = 45 bits
// ---------------------------------------------------------------------

// The three bytes after ByteSeq g6 are the still-wanted role counts, not
// padding. tc-preservation hardcodes them to zero and calls them "unk", but
// the client stores all three (sub_140395EE0 -> msg +346/+347/+348) and reads
// them back. This pins that they are carried, not dropped.
TEST(LFGPackets_UpdateStatus_carries_the_needs_triple)
{
    LFGPackets::UpdateStatus data;
    data.ticket.requesterGuid = 0x0011223344556677ULL;
    data.ticket.id = 42;
    data.ticket.time = 100;
    data.ticket.type = RIDE_TYPE_LFG;
    data.reason = 5;
    data.isParty = true;
    data.joined = true;
    data.lfgJoined = true;
    data.queued = true;
    data.needs = { 1, 2, 3 };            // tank, healer, damage still wanted

    WorldPacket packet(SMSG_LFG_UPDATE_STATUS, 64);
    REQUIRE(LFGPackets::BuildUpdateStatus(packet, data));

    // Identical to the exact fixture except the triple after ByteSeq g6.
    CHECK_BYTES(packet.contents(), packet.size(), {
        0xC0,0x00,0x00,0x30,0x05,0xF8,
        0x05,
        0x2A,0x00,0x00,0x00,
        0x64,0x00,0x00,0x00,
        0x10,
        0x01,0x02,0x03,                 // needs -- carried, not zeroed
        0x67,
        0x54,
        0x32,
        0x45,
        0x23,
        0x76,
        0x03,0x00,0x00,0x00
    });
}

TEST(LFGPackets_UpdateStatus_exact_fixture)
{
    LFGPackets::UpdateStatus data;
    data.ticket.requesterGuid = 0x0011223344556677ULL;   // g[7] == 0x00
    data.ticket.id = 42;
    data.ticket.time = 100;
    data.ticket.type = RIDE_TYPE_LFG;
    data.reason = 5;
    data.isParty = true;
    data.joined = true;
    data.lfgJoined = true;
    data.queued = true;
    // slots and comment left empty -- their WriteBits(0, N) count fields
    // are all-zero bits, keeping this fixture's bit region tractable by
    // hand while still covering the single-bit GUID/flag fields for real.

    WorldPacket packet(SMSG_LFG_UPDATE_STATUS, 64);
    REQUIRE(LFGPackets::BuildUpdateStatus(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0xC0,0x00,0x00,0x30,0x05,0xF8,  // 45-bit region, 3 padding bits
        0x05,                            // reason
        // comment empty -- no bytes
        0x2A,0x00,0x00,0x00,            // ticket.id=42
        0x64,0x00,0x00,0x00,            // ticket.time=100
        0x10,                            // ByteSeq g6
        0x00,0x00,0x00,                 // needs -- default zeros
        0x67,                            // ByteSeq g1
        0x54,                            // ByteSeq g2
        0x32,                            // ByteSeq g4
        0x45,                            // ByteSeq g3
        0x23,                            // ByteSeq g5
        0x76,                            // ByteSeq g0
        0x03,0x00,0x00,0x00,            // ticket.type=RIDE_TYPE_LFG
        // ByteSeq g7 SKIPPED -- guid byte is zero
    });
}

TEST(LFGPackets_UpdateStatus_bit_region_is_exactly_45_bits)
{
    // All-nonzero guid, all flags true, empty slots/comment: the bit
    // region is 14 single bits + 24 zero bits (slots count) + 9 zero
    // bits (comment length) = 45 bits, rounding up to 6 bytes. `reason`
    // lands at byte offset 6 only if that rounding is right.
    LFGPackets::UpdateStatus data;
    data.ticket.requesterGuid = 0x1122334455667788ULL;
    data.isParty = true;
    data.joined = true;
    data.lfgJoined = true;
    data.queued = true;
    data.reason = 9;

    WorldPacket packet(SMSG_LFG_UPDATE_STATUS, 64);
    REQUIRE(LFGPackets::BuildUpdateStatus(packet, data));

    REQUIRE(packet.size() >= 7);
    CHECK_EQ(uint32(packet.contents()[6]), 9u);
}

TEST(LFGPackets_UpdateStatus_non_empty_comment_flush_discipline)
{
    // Proves the write side's explicit FlushBits() (and WriteStringData's
    // own internal one) lands the pending bit byte BEFORE "LFM tank"
    // starts, not spliced into the middle of it -- this is the task's
    // #1 risk, made concrete: operator<<(std::string) does NOT auto-flush,
    // only the templated scalar append<T> does.
    LFGPackets::UpdateStatus data;
    data.ticket.requesterGuid = 0x1122334455667788ULL;
    data.reason = 9;
    data.comment = "LFM tank";

    WorldPacket packet(SMSG_LFG_UPDATE_STATUS, 64);
    REQUIRE(LFGPackets::BuildUpdateStatus(packet, data));

    // 6-byte (45-bit) region, then reason (1 byte), then the comment.
    REQUIRE(packet.size() >= 15);
    CHECK_EQ(uint32(packet.contents()[6]), 9u);

    std::string comment(reinterpret_cast<const char*>(packet.contents()) + 7, 8);
    CHECK_STR(comment, "LFM tank");
}

TEST(LFGPackets_UpdateStatus_rejects_comment_over_9_bit_cap)
{
    LFGPackets::UpdateStatus data;
    data.comment.assign(512, 'x');   // 9-bit count field caps at 511

    WorldPacket packet(SMSG_LFG_UPDATE_STATUS, 0);
    CHECK(!LFGPackets::BuildUpdateStatus(packet, data));
    CHECK(packet.empty());
}

// ---------------------------------------------------------------------
// SMSG_LFG_JOIN_RESULT (0x38B6) -- bit region = 32 + 30*N bits
// ---------------------------------------------------------------------

TEST(LFGPackets_JoinResult_exact_fixture)
{
    LFGPackets::JoinResult data;
    data.ticket.requesterGuid = 0x0011223344556677ULL;   // g[7] == 0x00
    data.ticket.type = RIDE_TYPE_LFG;
    data.ticket.id = 42;
    data.ticket.time = 100;
    data.result = 1;
    data.resultDetail = 2;

    LFGPackets::JoinResultBlacklist bl;
    bl.guid = 0x1122334455667788ULL;   // all-nonzero, keeps this tractable
    bl.slots = { { 285, 4, 10, 85 } };
    data.blacklist = { bl };

    WorldPacket packet(SMSG_LFG_JOIN_RESULT, 128);
    REQUIRE(LFGPackets::BuildJoinResult(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x03,0x00,0x00,0x00,            // ticket.type=RIDE_TYPE_LFG
        0x01,                            // result
        0x2A,0x00,0x00,0x00,            // ticket.id=42
        0x02,                            // resultDetail
        0x64,0x00,0x00,0x00,            // ticket.time=100
        0xB0,0x00,0x00,0x1F,0xF0,0x00,0x00,0x7C,   // 62-bit region, N=1
        0x04,0x00,0x00,0x00,            // slot.reason=4
        0x0A,0x00,0x00,0x00,            // slot.subReason1=10
        0x55,0x00,0x00,0x00,            // slot.subReason2=85
        0x1D,0x01,0x00,0x00,            // slot.slot=285
        0x67,0x32,0x76,0x89,0x45,0x54,0x23,0x10,   // blacklist guid ByteSeq
        0x67,0x32,0x45,0x23,0x76,       // ticket ByteSeq g1,g4,g3,g5,g0
        // ByteSeq g7 SKIPPED -- guid byte is zero
        0x54,0x10                       // ticket ByteSeq g2,g6
    });
}

TEST(LFGPackets_JoinResult_bit_region_scales_32_plus_30N)
{
    LFGPackets::JoinResult data;
    data.ticket.requesterGuid = 0x1122334455667788ULL;   // all-nonzero

    WorldPacket zeroEntries(SMSG_LFG_JOIN_RESULT, 64);
    REQUIRE(LFGPackets::BuildJoinResult(zeroEntries, data));
    // 14 fixed bytes + ceil(32/8)=4 (N=0, no rounding) + 8 ticket ByteSeq
    CHECK_EQ(zeroEntries.size(), size_t(26));

    LFGPackets::JoinResultBlacklist bl;
    bl.guid = 0x1122334455667788ULL;   // all-nonzero
    data.blacklist = { bl };

    WorldPacket oneEntry(SMSG_LFG_JOIN_RESULT, 64);
    REQUIRE(LFGPackets::BuildJoinResult(oneEntry, data));
    // 14 fixed + ceil(62/8)=8 (N=1, 2 padding bits) + 8 blacklist guid
    // ByteSeq + 8 ticket ByteSeq
    CHECK_EQ(oneEntry.size(), size_t(38));
}

// A rejection test at the public Build* API would need to resize a
// vector past 2^24 (16.7M) entries to trip the real bound -- expensive
// and slow for what it proves. FitsBits() is exercised directly instead;
// BuildJoinResult / BuildProposalUpdate / BuildUpdateStatus all delegate
// their bounds checks to it, so this covers the same logic.
TEST(LFGPackets_FitsBits_boundary_checks)
{
    CHECK(LFGPacketDetail::FitsBits((size_t(1) << 22) - 1, 22));
    CHECK(!LFGPacketDetail::FitsBits(size_t(1) << 22, 22));
    CHECK(LFGPacketDetail::FitsBits((size_t(1) << 23) - 1, 23));
    CHECK(!LFGPacketDetail::FitsBits(size_t(1) << 23, 23));
    CHECK(LFGPacketDetail::FitsBits((size_t(1) << 24) - 1, 24));
    CHECK(!LFGPacketDetail::FitsBits(size_t(1) << 24, 24));
}

// ---------------------------------------------------------------------
// SMSG_LFG_PROPOSAL_UPDATE (0x7DA6) -- bit region = 40 + 5*N bits
// ---------------------------------------------------------------------

TEST(LFGPackets_ProposalUpdate_exact_fixture)
{
    LFGPackets::ProposalUpdate data;
    data.ticket.requesterGuid = 0x0011223344556677ULL;   // g[7] == 0x00
    data.ticket.time = 100;
    data.ticket.id = 42;
    data.ticket.type = RIDE_TYPE_LFG;
    data.instanceId = 0x1122334455667788ULL;             // all-nonzero
    data.completedMask = 0x0F;
    data.slot = 285;
    data.proposalId = 7;
    data.state = 2;
    data.proposalSilent = true;

    LFGPackets::ProposalUpdatePlayer player;
    player.roles = 5;
    player.me = true;
    player.sameParty = false;
    player.myParty = true;
    player.responded = true;
    player.accepted = false;
    data.players = { player };

    WorldPacket packet(SMSG_LFG_PROPOSAL_UPDATE, 128);
    REQUIRE(LFGPackets::BuildProposalUpdate(packet, data));

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x64,0x00,0x00,0x00,            // ticket.time=100
        0x0F,0x00,0x00,0x00,            // completedMask
        0x2A,0x00,0x00,0x00,            // ticket.id=42
        0x03,0x00,0x00,0x00,            // ticket.type=RIDE_TYPE_LFG
        0x1D,0x01,0x00,0x00,            // slot=285
        0x07,0x00,0x00,0x00,            // proposalId
        0x02,                            // state
        0xDF,0x80,0x00,0x01,0xAF,0xF8,  // 45-bit region, N=1
        0x23,0x54,0x23,0x10,0x76,0x32,0x67,   // ByteSeq g5,ig3,ig6,g6,g0,ig5,g1
        0x05,0x00,0x00,0x00,            // player.roles
        0x10,0x32,0x89,0x76,0x54,       // ByteSeq ig7,g4,ig0,ig1,g2
        // ByteSeq g7 SKIPPED -- guid byte is zero
        0x67,0x45,0x45                  // ByteSeq ig2,g3,ig4
    });
}

TEST(LFGPackets_ProposalUpdate_bit_region_scales_40_plus_5N)
{
    LFGPackets::ProposalUpdate data;
    data.ticket.requesterGuid = 0x1122334455667788ULL;   // all-nonzero
    data.instanceId = 0x1122334455667788ULL;              // all-nonzero

    WorldPacket zeroPlayers(SMSG_LFG_PROPOSAL_UPDATE, 64);
    REQUIRE(LFGPackets::BuildProposalUpdate(zeroPlayers, data));
    // 25 fixed bytes + ceil(40/8)=5 (N=0, no rounding) + 16 ByteSeq (8+8)
    CHECK_EQ(zeroPlayers.size(), size_t(46));

    data.players = { LFGPackets::ProposalUpdatePlayer{} };

    WorldPacket onePlayer(SMSG_LFG_PROPOSAL_UPDATE, 64);
    REQUIRE(LFGPackets::BuildProposalUpdate(onePlayer, data));
    // 25 fixed + ceil(45/8)=6 (N=1, 3 padding bits) + 4 (roles) + 16 ByteSeq
    CHECK_EQ(onePlayer.size(), size_t(51));
}

// ---------------------------------------------------------------------
// SMSG_LFG_SLOT_INVALID (0x54B5) -- three plain uint32, no bit-packing
// ---------------------------------------------------------------------

// Wire order is reason, reasonArg2, reasonArg1 -- arg2 precedes arg1, per
// the handler's event args and FrameXML/LFGFrame.lua:189-191.
TEST(LFGPackets_SlotInvalid_exact_fixture)
{
    LFGPackets::SlotInvalid data;
    data.reason = 4;
    data.reasonArg2 = 10;
    data.reasonArg1 = 285;

    WorldPacket packet(SMSG_LFG_SLOT_INVALID, 12);
    LFGPackets::BuildSlotInvalid(packet, data);

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x04,0x00,0x00,0x00,
        0x0A,0x00,0x00,0x00,
        0x1D,0x01,0x00,0x00
    });
}

// ---------------------------------------------------------------------
// SMSG_LFG_UPDATE_STATUS_NONE (0x7CA1) -- zero payload
// ---------------------------------------------------------------------

TEST(LFGPackets_UpdateStatusNone_has_no_payload)
{
    WorldPacket packet(SMSG_LFG_UPDATE_STATUS_NONE, 0);
    LFGPackets::BuildUpdateStatusNone(packet);

    CHECK(packet.empty());
}

// ---------------------------------------------------------------------
// CMSG_LFG_JOIN (0x2430)
// ---------------------------------------------------------------------

TEST(LFGPackets_ReadJoin_exact_fixture)
{
    WorldPacket packet(CMSG_LFG_JOIN, 64);
    packet << uint32(7);                              // roles
    packet << uint32(1) << uint32(2) << uint32(3);     // needs
    packet.WriteBits(2, 9);                            // comment length
    packet.WriteBits(2, 24);                           // slots count
    packet.FlushBits();
    packet.WriteStringData("AB");                      // raw comment bytes
    packet << uint32(285) << uint32(187);               // slots

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x07,0x00,0x00,0x00,
        0x01,0x00,0x00,0x00,
        0x02,0x00,0x00,0x00,
        0x03,0x00,0x00,0x00,
        0x01,0x00,0x00,0x01,0x00,
        0x41,0x42,
        0x1D,0x01,0x00,0x00,
        0xBB,0x00,0x00,0x00
    });

    LFGPackets::JoinRequest out;
    LFGPackets::ReadJoin(packet, out);

    CHECK_EQ(out.roles, 7u);
    CHECK_EQ(out.needs[0], 1u);
    CHECK_EQ(out.needs[1], 2u);
    CHECK_EQ(out.needs[2], 3u);
    CHECK_STR(out.comment, "AB");
    REQUIRE(out.slots.size() == 2);
    CHECK_EQ(out.slots[0], 285u);
    CHECK_EQ(out.slots[1], 187u);
}

// ---------------------------------------------------------------------
// CMSG_LFG_LEAVE (0x2433)
// ---------------------------------------------------------------------

TEST(LFGPackets_ReadLeave_exact_fixture_zero_guid_byte)
{
    const uint64 guid = 0x0011223344556677ULL;   // g[7] == 0x00
    uint8 g[8];
    for (int i = 0; i < 8; ++i)
    {
        g[i] = uint8((guid >> (i * 8)) & 0xFF);
    }

    WorldPacket packet(CMSG_LFG_LEAVE, 64);
    packet << uint32(9);           // roles
    packet << int32(50);            // ticket.time
    packet << uint32(uint32(RIDE_TYPE_LFG));
    packet << uint32(77);           // ticket.id

    packet.WriteBit(g[4]);
    packet.WriteBit(g[5]);
    packet.WriteBit(g[0]);
    packet.WriteBit(g[6]);
    packet.WriteBit(g[2]);
    packet.WriteBit(g[7]);
    packet.WriteBit(g[1]);
    packet.WriteBit(g[3]);
    packet.FlushBits();

    packet.WriteByteSeq(g[7]);   // skipped -- zero byte
    packet.WriteByteSeq(g[4]);
    packet.WriteByteSeq(g[3]);
    packet.WriteByteSeq(g[2]);
    packet.WriteByteSeq(g[6]);
    packet.WriteByteSeq(g[0]);
    packet.WriteByteSeq(g[1]);
    packet.WriteByteSeq(g[5]);

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x09,0x00,0x00,0x00,
        0x32,0x00,0x00,0x00,
        0x03,0x00,0x00,0x00,
        0x4D,0x00,0x00,0x00,
        0xFB,
        // g7 SKIPPED
        0x32,0x45,0x54,0x10,0x76,0x67,0x23
    });

    LFGPackets::LeaveRequest out;
    LFGPackets::ReadLeave(packet, out);

    CHECK_EQ(out.roles, 9u);
    CHECK_EQ(int(out.ticket.time), 50);
    CHECK(out.ticket.type == RIDE_TYPE_LFG);
    CHECK_EQ(out.ticket.id, 77u);
    CHECK_EQ(out.ticket.requesterGuid, guid);
}

// ---------------------------------------------------------------------
// CMSG_LFG_PROPOSAL_RESULT (0x0403)
// ---------------------------------------------------------------------

TEST(LFGPackets_ReadProposalResult_exact_fixture)
{
    const uint64 ticketGuid = 0x0011223344556677ULL;   // g[7] == 0x00
    const uint64 instanceGuid = 0x1122334455667788ULL;  // all-nonzero

    WorldPacket packet(CMSG_LFG_PROPOSAL_RESULT, 64);
    packet << uint32(99);          // proposalId
    packet << int32(10);            // ticket.time
    packet << uint32(uint32(RIDE_TYPE_LFG));
    packet << uint32(5);            // ticket.id

    uint8 g[8];
    uint8 ig[8];
    for (int i = 0; i < 8; ++i)
    {
        g[i] = uint8((ticketGuid >> (i * 8)) & 0xFF);
        ig[i] = uint8((instanceGuid >> (i * 8)) & 0xFF);
    }

    packet.WriteBit(g[4]);
    packet.WriteBit(g[5]);
    packet.WriteBit(g[0]);
    packet.WriteBit(g[6]);
    packet.WriteBit(g[2]);
    packet.WriteBit(g[7]);
    packet.WriteBit(g[1]);
    packet.WriteBit(g[3]);
    packet.FlushBits();

    packet.WriteByteSeq(g[7]);   // skipped -- zero byte
    packet.WriteByteSeq(g[4]);
    packet.WriteByteSeq(g[3]);
    packet.WriteByteSeq(g[2]);
    packet.WriteByteSeq(g[6]);
    packet.WriteByteSeq(g[0]);
    packet.WriteByteSeq(g[1]);
    packet.WriteByteSeq(g[5]);

    packet.WriteBit(ig[7]);
    packet.WriteBit(true);        // accepted
    packet.WriteBit(ig[1]);
    packet.WriteBit(ig[3]);
    packet.WriteBit(ig[0]);
    packet.WriteBit(ig[5]);
    packet.WriteBit(ig[4]);
    packet.WriteBit(ig[6]);
    packet.WriteBit(ig[2]);
    packet.FlushBits();

    packet.WriteByteSeq(ig[7]);
    packet.WriteByteSeq(ig[1]);
    packet.WriteByteSeq(ig[5]);
    packet.WriteByteSeq(ig[6]);
    packet.WriteByteSeq(ig[3]);
    packet.WriteByteSeq(ig[4]);
    packet.WriteByteSeq(ig[0]);
    packet.WriteByteSeq(ig[2]);

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x63,0x00,0x00,0x00,
        0x0A,0x00,0x00,0x00,
        0x03,0x00,0x00,0x00,
        0x05,0x00,0x00,0x00,
        0xFB,
        // g7 SKIPPED
        0x32,0x45,0x54,0x10,0x76,0x67,0x23,
        0xFF,0x80,
        0x10,0x76,0x32,0x23,0x54,0x45,0x89,0x67
    });

    LFGPackets::ProposalResult out;
    LFGPackets::ReadProposalResult(packet, out);

    CHECK_EQ(out.proposalId, 99u);
    CHECK_EQ(int(out.ticket.time), 10);
    CHECK(out.ticket.type == RIDE_TYPE_LFG);
    CHECK_EQ(out.ticket.id, 5u);
    CHECK_EQ(out.ticket.requesterGuid, ticketGuid);
    CHECK(out.accepted);
    CHECK_EQ(out.instanceId, instanceGuid);
}

// ---------------------------------------------------------------------
// CMSG_LFG_SET_ROLES (0x0480)
// ---------------------------------------------------------------------

TEST(LFGPackets_ReadSetRoles)
{
    WorldPacket packet(CMSG_LFG_SET_ROLES, 4);
    packet << uint32(11);

    CHECK_BYTES(packet.contents(), packet.size(), { 0x0B,0x00,0x00,0x00 });

    LFGPackets::SetRolesRequest out;
    LFGPackets::ReadSetRoles(packet, out);
    CHECK_EQ(out.rolesDesired, 11u);
}

// ---------------------------------------------------------------------
// CMSG_SET_LFG_COMMENT (0x0530)
// ---------------------------------------------------------------------

TEST(LFGPackets_ReadSetComment_non_empty_flush_discipline)
{
    WorldPacket packet(CMSG_SET_LFG_COMMENT, 16);
    packet.WriteBits(5, 9);      // comment length
    packet.FlushBits();
    packet.WriteStringData("Hello");

    CHECK_BYTES(packet.contents(), packet.size(), {
        0x02,0x80,
        0x48,0x65,0x6C,0x6C,0x6F
    });

    LFGPackets::SetCommentRequest out;
    LFGPackets::ReadSetComment(packet, out);
    CHECK_STR(out.comment, "Hello");
}

// ---------------------------------------------------------------------
// CMSG_LFG_SET_BOOT_VOTE (0x04B3)
// ---------------------------------------------------------------------

TEST(LFGPackets_ReadSetBootVote_true)
{
    WorldPacket packet(CMSG_LFG_SET_BOOT_VOTE, 4);
    packet.WriteBit(true);
    packet.FlushBits();

    CHECK_BYTES(packet.contents(), packet.size(), { 0x80 });

    LFGPackets::SetBootVoteRequest out;
    LFGPackets::ReadSetBootVote(packet, out);
    CHECK(out.vote);
}

TEST(LFGPackets_ReadSetBootVote_false)
{
    WorldPacket packet(CMSG_LFG_SET_BOOT_VOTE, 4);
    packet.WriteBit(false);
    packet.FlushBits();

    CHECK_BYTES(packet.contents(), packet.size(), { 0x00 });

    LFGPackets::SetBootVoteRequest out;
    LFGPackets::ReadSetBootVote(packet, out);
    CHECK(!out.vote);
}

// ---------------------------------------------------------------------
// CMSG_LFG_TELEPORT (0x2482)
// ---------------------------------------------------------------------

TEST(LFGPackets_ReadTeleport_true)
{
    WorldPacket packet(CMSG_LFG_TELEPORT, 4);
    packet.WriteBit(true);
    packet.FlushBits();

    CHECK_BYTES(packet.contents(), packet.size(), { 0x80 });

    LFGPackets::TeleportRequest out;
    LFGPackets::ReadTeleport(packet, out);
    CHECK(out.teleportOut);
}

// ---------------------------------------------------------------------
// CMSG_LFG_LOCK_INFO_REQUEST (0x0412)
// ---------------------------------------------------------------------

TEST(LFGPackets_ReadLockInfoRequest_true)
{
    WorldPacket packet(CMSG_LFG_LOCK_INFO_REQUEST, 4);
    packet.WriteBit(true);
    packet.FlushBits();

    CHECK_BYTES(packet.contents(), packet.size(), { 0x80 });

    LFGPackets::LockInfoRequest out;
    LFGPackets::ReadLockInfoRequest(packet, out);
    CHECK(out.player);
}

// ---------------------------------------------------------------------
// DeriveUpdateFlags -- the Joined/Queued/LfgJoined derivation table
// (tc-preservation LFGHandler.cpp:336-386; client consumes Joined as the
// payload gate and LfgJoined as the eye-clear signal).
// ---------------------------------------------------------------------

TEST(LFGPackets_DeriveUpdateFlags_join_initial)
{
    LFGPackets::UpdateFlags f = LFGPackets::DeriveUpdateFlags(24, 0);
    CHECK(f.joined);
    CHECK(!f.queued);
    CHECK(f.lfgJoined);
}

TEST(LFGPackets_DeriveUpdateFlags_join_queue)
{
    LFGPackets::UpdateFlags f = LFGPackets::DeriveUpdateFlags(6, 0);
    CHECK(f.joined);
    CHECK(f.queued);
    CHECK(f.lfgJoined);
}

TEST(LFGPackets_DeriveUpdateFlags_added_to_queue)
{
    LFGPackets::UpdateFlags f = LFGPackets::DeriveUpdateFlags(13, 0);
    CHECK(f.joined);
    CHECK(f.queued);
    CHECK(f.lfgJoined);
}

TEST(LFGPackets_DeriveUpdateFlags_proposal_begin)
{
    LFGPackets::UpdateFlags f = LFGPackets::DeriveUpdateFlags(14, 0);
    CHECK(f.joined);
    CHECK(!f.queued);
    CHECK(f.lfgJoined);
}

TEST(LFGPackets_DeriveUpdateFlags_removed_from_queue)
{
    LFGPackets::UpdateFlags f = LFGPackets::DeriveUpdateFlags(8, 0);
    CHECK(!f.joined);
    CHECK(!f.queued);
    CHECK(!f.lfgJoined);
}

TEST(LFGPackets_DeriveUpdateFlags_rolecheck_failed)
{
    LFGPackets::UpdateFlags f = LFGPackets::DeriveUpdateFlags(7, 0);
    CHECK(!f.joined);
    CHECK(!f.queued);
    CHECK(f.lfgJoined);
}

TEST(LFGPackets_DeriveUpdateFlags_update_status_states)
{
    // reason 15: joined = state not in {0,1,5,6}; queued = state in {2,7}
    struct Row { uint8 state; bool joined; bool queued; };
    const Row rows[] = {
        {0, false, false}, {1, false, false}, {2, true, true},
        {3, true, false},  {4, true, false},  {5, false, false},
        {6, false, false}, {7, true, true},
    };
    for (Row const& row : rows)
    {
        LFGPackets::UpdateFlags f = LFGPackets::DeriveUpdateFlags(15, row.state);
        CHECK(f.joined == row.joined);
        CHECK(f.queued == row.queued);
        CHECK(f.lfgJoined);
    }
}
