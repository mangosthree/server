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

#ifndef MANGOS_LFGPACKETS_H
#define MANGOS_LFGPACKETS_H

/**
 * @file LFGPackets.h
 * @brief 4.3.4 (build 15595) LFG packet layer -- flat and bit-packed.
 *
 * Phase 3a's flat (disp1) builders were transcribed from the client's own
 * reader functions in client-truth/binary/Wow-64.c and independently
 * re-verified against the live IDA database during implementation.
 * Phase 3b adds the six bit-packed disp2 SMSG builders (QUEUE_STATUS,
 * UPDATE_STATUS, JOIN_RESULT, PROPOSAL_UPDATE, SLOT_INVALID,
 * UPDATE_STATUS_NONE) and the CMSG readers, verified directly against the
 * live IDA database and cross-checked against tc-preservation's 4.3.4
 * packet layer (reference, not blueprint -- see each function's own
 * comment for its evidence trail). SMSG_LFG_SEARCH_RESULTS is NOT DECODED
 * and is not implemented in either phase.
 *
 * Deliberately self-contained: only WorldPacket.h and the standard library.
 * LFGMgr.h pulls in Group.h (BattleGround.h, LootMgr.h, ...), which the
 * mangos_tests binary does not link (see src/tests/CMakeLists.txt) -- so a
 * builder namespace that needs to be unit-testable cannot live there. Callers
 * translate LFGMgr's richer structures into the plain data below at the call
 * site; nothing in this header knows about Player, Group, or WorldSession.
 */

#include "WorldPacket.h"

#include <array>
#include <string>
#include <vector>

/// 4.3.4 ticket kind carried by RideTicket::type. Value 2 is intentionally
/// absent -- confirmed against tc-preservation's RideType, not a typo.
enum RideType
{
    RIDE_TYPE_NONE          = 0,
    RIDE_TYPE_BATTLEGROUNDS = 1,
    RIDE_TYPE_LFG           = 3
};

/**
 * 4.3.4 queue ticket -- replaces the bare ObjectGuid the WotLK-era code used
 * to identify a dungeon-finder request (analysis section 3.3). Not consumed
 * by any packet in this file: every disp1 packet here still carries a raw
 * GUID. Introduced now because the bit-packed disp2 packets (Phase 3b) never
 * write it as a block -- each interleaves its four fields with its own bit
 * and byte runs -- so the struct has to exist before those builders do.
 */
struct RideTicket
{
    int32 time = 0;                ///< queue join time
    RideType type = RIDE_TYPE_NONE;
    uint32 id = 0;                 ///< queue / ticket id
    uint64 requesterGuid = 0;      ///< player or group guid, raw
};

/// Wire layout for a single dungeon-finder lock reason. Shared by
/// SMSG_LFG_PLAYER_INFO and SMSG_LFG_PARTY_INFO, both of which write
/// Slot, Reason, SubReason1, SubReason2 via the client's common lock-list
/// reader (sub_1407021A0). SMSG_LFG_JOIN_RESULT (Phase 3b) uses a different
/// field order and must not reuse this struct.
struct LFGLockedDungeon
{
    uint32 slot = 0;
    uint32 reason = 0;
    uint32 subReason1 = 0;        ///< required item level, for gear locks
    uint32 subReason2 = 0;        ///< current item level, for gear locks
};

/// A single dungeon-finder reward item or currency payout.
struct LFGRewardItem
{
    uint32 id = 0;                ///< item id, or currency id when isCurrency
    uint32 displayId = 0;         ///< 0 for currency
    uint32 quantity = 0;
    bool isCurrency = false;
};

namespace LFGPacketDetail
{
    /// Every LFG count field in this file is a wire uint8; bounds-check
    /// before writing anything so a rejected builder leaves `out` untouched.
    inline bool FitsU8(size_t count)
    {
        return count <= size_t(0xFF);
    }

    /// Bounds-check for a bit-packed wire count field (WriteBits(count, bits)
    /// in the Phase 3b builders below).
    inline bool FitsBits(size_t count, unsigned bits)
    {
        return count <= ((size_t(1) << bits) - 1);
    }

    /// Splits a raw GUID into its 8 little-endian bytes for the packed-GUID
    /// bit/byte-sequence writes below. Index 0 is the low byte -- matches
    /// ByteBuffer::WriteGuidBytes's own `(uint8*)&guid` addressing on this
    /// little-endian target, and ObjectGuid::operator[]'s indexing.
    inline std::array<uint8, 8> GuidBytes(uint64 guid)
    {
        std::array<uint8, 8> bytes{};
        for (int i = 0; i < 8; ++i)
        {
            bytes[i] = uint8((guid >> (i * 8)) & 0xFF);
        }

        return bytes;
    }

    /// Reassembles a raw GUID from 8 bytes produced by ReadBit +
    /// ReadByteSeq (see the CMSG readers below).
    inline uint64 AssembleGuid(std::array<uint8, 8> const& bytes)
    {
        uint64 guid = 0;
        for (int i = 0; i < 8; ++i)
        {
            guid |= uint64(bytes[i]) << (i * 8);
        }

        return guid;
    }
}

namespace LFGPackets
{
    // ---------------------------------------------------------------------
    // SMSG_LFG_ROLE_CHECK_UPDATE (0x0336) -- sub_1406FF4C0
    // ---------------------------------------------------------------------

    /// One role-check participant. The leader MUST be first in
    /// RoleCheckUpdate::members -- the client has no other way to tell.
    struct LFGRoleCheckMember
    {
        uint64 guid = 0;           ///< raw, not packed
        bool roleCheckComplete = false;
        uint32 rolesDesired = 0;   ///< bit1 tank, bit2 healer, bit3 dps
        uint8 level = 0;
    };

    struct RoleCheckUpdate
    {
        uint32 status = 0;         ///< LfgRoleCheckState
        bool isBeginning = false;  ///< nonzero shows the role-check UI
        std::vector<uint32> dungeons;               ///< resolved dungeon entries
        std::vector<LFGRoleCheckMember> members;    ///< leader first
    };

    /// Builds SMSG_LFG_ROLE_CHECK_UPDATE. False (and `out` untouched) if
    /// dungeons or members exceeds the wire uint8 count.
    inline bool BuildRoleCheckUpdate(WorldPacket& out, RoleCheckUpdate const& data)
    {
        if (!LFGPacketDetail::FitsU8(data.dungeons.size()) ||
            !LFGPacketDetail::FitsU8(data.members.size()))
        {
            return false;
        }

        out << uint32(data.status);
        out << uint8(data.isBeginning);

        out << uint8(data.dungeons.size());
        for (uint32 dungeon : data.dungeons)
        {
            out << dungeon;
        }

        out << uint8(data.members.size());
        for (LFGRoleCheckMember const& member : data.members)
        {
            out << uint64(member.guid);
            out << uint8(member.roleCheckComplete);
            out << member.rolesDesired;
            out << member.level;
        }

        return true;
    }

    // ---------------------------------------------------------------------
    // SMSG_ROLE_CHOSEN / "SMSG_LFG_ROLE_CHOSEN" (0x6A26) -- sub_1406FA550
    //
    // Opcodes.h already names this SMSG_ROLE_CHOSEN (no LFG infix); that
    // predates this branch and is left as-is.
    // ---------------------------------------------------------------------

    /// TANK | HEALER | DAMAGE. The LEADER bit alone is not a role.
    uint32 const ROLE_MASK_ANY = 0x0E;

    /// Accepted is false unless a real role bit is set; the client asserts.
    inline void BuildRoleChosen(WorldPacket& out, uint64 guid, uint32 roleMask)
    {
        out << uint64(guid);
        out << uint8((roleMask & ROLE_MASK_ANY) != 0);
        out << roleMask;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_TELEPORT_DENIED (0x0E14) -- sub_1406FA8B0
    // ---------------------------------------------------------------------

    /// Builds SMSG_LFG_TELEPORT_DENIED. `error` is an LFGTeleportError value;
    /// the client switches on it directly (0/5/7 silent, 1/2/4/6 specific
    /// messages, everything else message 142).
    inline void BuildTeleportDenied(WorldPacket& out, uint32 error)
    {
        out << error;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_BOOT_PROPOSAL_UPDATE (0x0F05) -- sub_1406FA6F0
    // Flag names from the client's own log string at 0x1409F89E0 ("In
    // Progress / I Voted / My Vote", slots 1/3/4) and from slot 2 selecting
    // ERR_PARTY_LFG_BOOT_VOTE_SUCCEEDED (680) vs _FAILED (681) at
    // 0x1406FA86E. votesNeeded is parsed but never pushed to Lua.
    // ---------------------------------------------------------------------

    struct BootProposalUpdate
    {
        bool voteInProgress = false;
        bool votePassed = false;    ///< selects the client's "vote passed" vs
                                     ///< "vote failed" line; only read when
                                     ///< voteInProgress is false
        bool myVoteCompleted = false;
        bool myVote = false;
        uint64 target = 0;          ///< victim guid, raw
        uint32 totalVotes = 0;
        uint32 bootVotes = 0;
        uint32 timeLeft = 0;        ///< seconds
        uint32 votesNeeded = 0;     ///< wire-only; never reaches Lua
        std::string reason;         ///< NUL-terminated on the wire, max 256
                                     ///< bytes including the terminator
    };

    /// Builds SMSG_LFG_BOOT_PROPOSAL_UPDATE. False (and `out` untouched) if
    /// reason would not fit the client's bounded CString reader
    /// (sub_1405A7E20, 256-byte cap including the terminator).
    inline bool BuildBootProposalUpdate(WorldPacket& out, BootProposalUpdate const& data)
    {
        if (data.reason.size() > size_t(255))
        {
            return false;
        }

        out << uint8(data.voteInProgress);
        out << uint8(data.votePassed);
        out << uint8(data.myVoteCompleted);
        out << uint8(data.myVote);
        out << uint64(data.target);
        out << data.totalVotes;
        out << data.bootVotes;
        out << data.timeLeft;
        out << data.votesNeeded;
        out << data.reason.c_str();   // appends bytes + NUL terminator

        return true;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_PARTY_INFO (0x2325) -- sub_140707590 / parse sub_140706D40
    // ---------------------------------------------------------------------

    struct LFGPartyInfoMember
    {
        uint64 guid = 0;             ///< raw; self is skipped by the caller
        std::vector<LFGLockedDungeon> lockedDungeons;
    };

    struct PartyInfo
    {
        std::vector<LFGPartyInfoMember> members;
    };

    /// Builds SMSG_LFG_PARTY_INFO. False (and `out` untouched) if the member
    /// count exceeds the wire uint8.
    inline bool BuildPartyInfo(WorldPacket& out, PartyInfo const& data)
    {
        if (!LFGPacketDetail::FitsU8(data.members.size()))
        {
            return false;
        }

        out << uint8(data.members.size());
        for (LFGPartyInfoMember const& member : data.members)
        {
            out << uint64(member.guid);
            out << uint32(member.lockedDungeons.size());
            for (LFGLockedDungeon const& lock : member.lockedDungeons)
            {
                out << lock.slot;
                out << lock.reason;
                out << lock.subReason1;
                out << lock.subReason2;
            }
        }

        return true;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_PLAYER_REWARD (0x6834) -- sub_1406FF8B0
    // ---------------------------------------------------------------------

    struct PlayerReward
    {
        uint32 queuedSlot = 0;       ///< random dungeon entry
        uint32 actualSlot = 0;       ///< actual dungeon entry
        uint32 rewardMoney = 0;
        uint32 addedXp = 0;
        std::vector<LFGRewardItem> items;
    };

    /// Builds SMSG_LFG_PLAYER_REWARD. False (and `out` untouched) if items
    /// exceeds the wire uint8 count.
    inline bool BuildPlayerReward(WorldPacket& out, PlayerReward const& data)
    {
        if (!LFGPacketDetail::FitsU8(data.items.size()))
        {
            return false;
        }

        out << data.queuedSlot;
        out << data.actualSlot;
        out << data.rewardMoney;
        out << data.addedXp;

        out << uint8(data.items.size());
        for (LFGRewardItem const& item : data.items)
        {
            out << item.id;
            out << item.displayId;
            out << item.quantity;
            out << uint8(item.isCurrency);
        }

        return true;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_OFFER_CONTINUE (0x6B27) -- sub_1406FA980
    // ---------------------------------------------------------------------

    /// Builds SMSG_LFG_OFFER_CONTINUE. The client masks & 0xFFFFFF on read
    /// (only the low 24 bits are the dungeon entry); this builder writes
    /// `dungeonEntry` verbatim rather than pre-masking, so a caller that
    /// passes an already-dirty high byte finds out from its own tests
    /// instead of losing the bits silently here.
    inline void BuildOfferContinue(WorldPacket& out, uint32 dungeonEntry)
    {
        out << dungeonEntry;
    }

    // ---------------------------------------------------------------------
    // SMSG_OPEN_LFG_DUNGEON_FINDER (0x2C37) -- sub_1406F9120
    // ---------------------------------------------------------------------

    /// Builds SMSG_OPEN_LFG_DUNGEON_FINDER. Fixed shape, always succeeds.
    inline void BuildOpenDungeonFinder(WorldPacket& out, uint32 dungeonId)
    {
        out << dungeonId;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_DISABLED (0x0815) -- sub_1407070D0
    // ---------------------------------------------------------------------

    /// Builds SMSG_LFG_DISABLED. The client reads nothing at all (a pure
    /// state reset); this exists only so the opcode has the same Build*
    /// call shape as every other packet in this file.
    inline void BuildDisabled(WorldPacket& /*out*/)
    {
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_PLAYER_INFO (0x4B36) -- sub_140704CB0 / lock list sub_1407021A0
    // ---------------------------------------------------------------------

    /// One Call to Arms shortage-role reward tier. When roleMask is 0 the
    /// client's reader skips the rest of the iteration entirely (confirmed
    /// at Wow-64.c:1829314) -- money/xp/items below must not be written for
    /// a zero-mask tier. This is the opposite of TrinityCore, which always
    /// writes three placeholder zero fields; following TC's shape here
    /// would desynchronise the stream.
    struct LFGShortageReward
    {
        uint32 roleMask = 0;         ///< 0 => tier absent, nothing else written
        uint32 money = 0;
        uint32 xp = 0;
        std::vector<LFGRewardItem> items;
    };

    /// One random-dungeon reward entry (client struct LFGEntryInfo).
    struct LFGRandomDungeonEntry
    {
        uint32 slot = 0;
        bool firstReward = false;
        uint32 completionQuantity = 0;
        uint32 completionLimit = 0;
        uint32 completionCurrencyId = 0;
        uint32 specificQuantity = 0;
        uint32 specificLimit = 0;
        uint32 overallQuantity = 0;
        uint32 overallLimit = 0;
        uint32 purseWeeklyQuantity = 0;
        uint32 purseWeeklyLimit = 0;
        uint32 purseQuantity = 0;
        uint32 purseLimit = 0;
        uint32 quantity = 0;
        uint32 completedMask = 0;
        bool shortageEligible = false;
        std::array<LFGShortageReward, 3> shortageTiers;   ///< severity: rare, uncommon, plentiful
        uint32 rewardMoney = 0;
        uint32 rewardXp = 0;
        std::vector<LFGRewardItem> items;
    };

    struct PlayerInfo
    {
        std::vector<LFGRandomDungeonEntry> randomDungeons;
        std::vector<LFGLockedDungeon> lockedDungeons;
    };

    /// Builds SMSG_LFG_PLAYER_INFO. False (and `out` untouched) if
    /// randomDungeons, or any entry's shortage-tier or outer item list,
    /// exceeds its wire uint8 count. All counts are validated before any
    /// byte is written.
    inline bool BuildPlayerInfo(WorldPacket& out, PlayerInfo const& data)
    {
        if (!LFGPacketDetail::FitsU8(data.randomDungeons.size()))
        {
            return false;
        }

        for (LFGRandomDungeonEntry const& entry : data.randomDungeons)
        {
            if (!LFGPacketDetail::FitsU8(entry.items.size()))
            {
                return false;
            }

            for (LFGShortageReward const& tier : entry.shortageTiers)
            {
                if (!LFGPacketDetail::FitsU8(tier.items.size()))
                {
                    return false;
                }
            }
        }

        out << uint8(data.randomDungeons.size());
        for (LFGRandomDungeonEntry const& entry : data.randomDungeons)
        {
            out << entry.slot;
            out << uint8(entry.firstReward);
            out << entry.completionQuantity;
            out << entry.completionLimit;
            out << entry.completionCurrencyId;
            out << entry.specificQuantity;
            out << entry.specificLimit;
            out << entry.overallQuantity;
            out << entry.overallLimit;
            out << entry.purseWeeklyQuantity;
            out << entry.purseWeeklyLimit;
            out << entry.purseQuantity;
            out << entry.purseLimit;
            out << entry.quantity;
            out << entry.completedMask;
            out << uint8(entry.shortageEligible);

            for (LFGShortageReward const& tier : entry.shortageTiers)
            {
                out << tier.roleMask;
                if (tier.roleMask != 0)
                {
                    out << tier.money;
                    out << tier.xp;
                    out << uint8(tier.items.size());
                    for (LFGRewardItem const& item : tier.items)
                    {
                        out << item.id;
                        out << item.displayId;
                        out << item.quantity;
                        out << uint8(item.isCurrency);
                    }
                }
            }

            out << entry.rewardMoney;
            out << entry.rewardXp;
            out << uint8(entry.items.size());
            for (LFGRewardItem const& item : entry.items)
            {
                out << item.id;
                out << item.displayId;
                out << item.quantity;
                out << uint8(item.isCurrency);
            }
        }

        out << uint32(data.lockedDungeons.size());
        for (LFGLockedDungeon const& lock : data.lockedDungeons)
        {
            out << lock.slot;
            out << lock.reason;
            out << lock.subReason1;
            out << lock.subReason2;
        }

        return true;
    }

    // ---------------------------------------------------------------------
    // Phase 3b -- bit-packed disp2 SMSG builders and CMSG readers.
    //
    // Every layout below was derived directly from
    // client-truth/binary/Wow-64.i64
    // via the live IDA MCP and cross-checked against
    // MANGOS_archive/tc-preservation's src/server/game/Server/Packets/
    // LFGPackets.cpp (TrinityCore's own 4.3.4 packet layer -- reference,
    // not blueprint, per workspace rules). Evidence trail per packet is in
    // its own comment block below; the short version:
    //   - SMSG_LFG_UPDATE_STATUS: TC's field order reproduces the exact
    //     45-bit total, and the first 5 fields (2 single bits, a 24-bit
    //     count, 2 more single bits, a 9-bit count) were hand-traced
    //     against sub_140395EE0's shift/OR pattern and match positionally.
    //   - SMSG_LFG_JOIN_RESULT / SMSG_LFG_PROPOSAL_UPDATE: TC's structure
    //     reproduces the exact 32+30N / 40+5N totals given by the task.
    //   - CMSG_LFG_JOIN and CMSG_LFG_LEAVE: the client's SEND functions
    //     (sub_140316A50, sub_1403158F0 + shared sub_1403424D0) are
    //     *not* obfuscated (unlike the SMSG reader/Ctor functions) and
    //     were decompiled and traced bit-for-bit; both match TC exactly,
    //     including the precise packed-GUID bit and byte-sequence order.
    //     This is the strongest evidence in this phase and, since the
    //     ticket-guid packing helper (sub_1403424D0) is shared client-side
    //     infrastructure, it corroborates the SMSG-side ticket-guid order
    //     TC uses too.
    //   - CMSG_LFG_PROPOSAL_RESULT: same direct trace, confirmed via a
    //     bijective mapping from the client's local struct offsets to
    //     TC's named instanceId indices (see the function's own comment).
    //   - CMSG_LFG_SET_BOOT_VOTE and CMSG_LFG_TELEPORT: the client's SEND
    //     functions were traced all the way into the low-level append
    //     primitives (sub_140313E20 -> sub_140313BE0 -> sub_1405A7830) and
    //     proven to left-shift the bool into bit 7 before the byte hits
    //     the wire (0x80/0x00, not 0/1) -- this DIVERGES from
    //     tc-preservation, which reads both as plain bytes via
    //     `operator>>(bool&)`. Implemented here as ReadBit() per the
    //     binary. See each function's comment; flagged in the PR/report.
    //   - SMSG_LFG_SLOT_INVALID / SMSG_LFG_UPDATE_STATUS_NONE: not in TC
    //     at all (STATUS_UNHANDLED / STATUS_NEVER in tc-preservation's
    //     Opcodes.cpp) -- derived purely from the binary.
    // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------
    // SMSG_LFG_QUEUE_STATUS (0x78B4) -- sub_1403680E0 / read sub_140360010
    // ---------------------------------------------------------------------

    struct QueueStatus
    {
        RideTicket ticket;
        uint32 slot = 0;
        int32 avgWaitTimeMe = 0;
        int32 avgWaitTime = 0;
        std::array<int32, 3> avgWaitTimeByRole{};
        std::array<uint8, 3> lastNeeded{};
        uint32 queuedTime = 0;
    };

    /// Builds SMSG_LFG_QUEUE_STATUS. Fixed shape, always succeeds. Bit
    /// region is exactly the ticket GUID's 8 mask bits (matches the plan's
    /// "SMSG_LFG_QUEUE_STATUS = 8" total) -- confirmed by hand-tracing
    /// sub_140360010's opening shift/OR chain, which unpacks one wire byte
    /// into 8 struct fields before any byte-level read begins.
    inline void BuildQueueStatus(WorldPacket& out, QueueStatus const& data)
    {
        std::array<uint8, 8> g = LFGPacketDetail::GuidBytes(data.ticket.requesterGuid);

        out.WriteBit(g[3]);
        out.WriteBit(g[0]);
        out.WriteBit(g[2]);
        out.WriteBit(g[6]);
        out.WriteBit(g[5]);
        out.WriteBit(g[7]);
        out.WriteBit(g[1]);
        out.WriteBit(g[4]);
        out.FlushBits();

        out.WriteByteSeq(g[0]);

        for (uint8 i = 0; i < 3; ++i)
        {
            out << uint8(data.lastNeeded[i]);
            out << int32(data.avgWaitTimeByRole[i]);
        }

        out.WriteByteSeq(g[4]);
        out.WriteByteSeq(g[6]);
        out << int32(data.avgWaitTime);
        out << uint32(data.ticket.time);
        out << uint32(data.slot);
        out << uint32(data.queuedTime);
        out.WriteByteSeq(g[5]);
        out.WriteByteSeq(g[7]);
        out.WriteByteSeq(g[3]);
        out << uint32(data.ticket.id);
        out.WriteByteSeq(g[1]);
        out.WriteByteSeq(g[2]);
        out << int32(data.avgWaitTimeMe);
        out << uint32(data.ticket.type);
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_UPDATE_STATUS (0x31A4) -- sub_14039B8C0 / read sub_140395EE0
    // ---------------------------------------------------------------------

    struct UpdateStatus
    {
        RideTicket ticket;
        uint8 reason = 0;
        std::vector<uint32> slots;
        std::string comment;          ///< wire length is a 9-bit count, max 511
        std::array<uint8, 3> needs{}; ///< tank/healer/damage still wanted
        bool isParty = false;
        bool joined = false;
        bool lfgJoined = false;
        bool queued = false;
    };

    /// Builds SMSG_LFG_UPDATE_STATUS. False (and `out` untouched) if slots
    /// or comment exceed their wire bit-count fields (24 and 9 bits).
    /// Bit region totals exactly 45 bits, matching the plan.
    inline bool BuildUpdateStatus(WorldPacket& out, UpdateStatus const& data)
    {
        if (!LFGPacketDetail::FitsBits(data.slots.size(), 24) ||
            !LFGPacketDetail::FitsBits(data.comment.size(), 9))
        {
            return false;
        }

        std::array<uint8, 8> g = LFGPacketDetail::GuidBytes(data.ticket.requesterGuid);

        out.WriteBit(g[1]);
        out.WriteBit(data.isParty);
        out.WriteBits(uint32(data.slots.size()), 24);
        out.WriteBit(g[6]);
        out.WriteBit(data.joined);
        out.WriteBits(uint32(data.comment.size()), 9);
        out.WriteBit(g[4]);
        out.WriteBit(g[7]);
        out.WriteBit(g[2]);
        out.WriteBit(data.lfgJoined);
        out.WriteBit(g[0]);
        out.WriteBit(g[3]);
        out.WriteBit(g[5]);
        out.WriteBit(data.queued);
        out.FlushBits();

        out << uint8(data.reason);
        out.WriteStringData(data.comment);   // raw bytes, length already sent

        out << uint32(data.ticket.id);
        out << uint32(data.ticket.time);
        out.WriteByteSeq(g[6]);

        // tc-preservation hardcodes three zero bytes here and calls them
        // "unk - Always 0", but the client keeps all three: the reader
        // (sub_140395EE0) stores them at msg +346/+347/+348 and the handler
        // reads every one back. Carry real values instead.
        for (uint8 i = 0; i < 3; ++i)
        {
            out << data.needs[i];
        }

        out.WriteByteSeq(g[1]);
        out.WriteByteSeq(g[2]);
        out.WriteByteSeq(g[4]);
        out.WriteByteSeq(g[3]);
        out.WriteByteSeq(g[5]);
        out.WriteByteSeq(g[0]);
        out << uint32(data.ticket.type);
        out.WriteByteSeq(g[7]);

        for (uint32 slot : data.slots)
        {
            out << slot;
        }

        return true;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_UPDATE_STATUS flag derivation
    // ---------------------------------------------------------------------

    /// Joined/Queued/LfgJoined wire bits for SMSG_LFG_UPDATE_STATUS, derived
    /// from the update reason and player state exactly as tc-preservation's
    /// 4.3.4 LFGHandler.cpp:336-386 does. Numeric constants mirror LFGMgr.h's
    /// LfgUpdateType / LFGState, kept in sync by hand (precedent:
    /// LFGLockReason.h) because this header must stay game-lib-free.
    struct UpdateFlags
    {
        bool joined = false;
        bool queued = false;
        bool lfgJoined = false;
    };

    inline UpdateFlags DeriveUpdateFlags(uint8 reason, uint8 state)
    {
        UpdateFlags flags;
        switch (reason)
        {
            case 24:                              // JOIN_INITIAL
            case 3:                               // JOIN_RAIDBROWSER
            case 14:                              // PROPOSAL_BEGIN
                flags.joined = true;
                break;
            case 6:                               // JOIN (rolecheck begins)
            case 13:                              // ADDED_TO_QUEUE
                flags.joined = true;
                flags.queued = true;
                break;
            case 15:                              // UPDATE_STATUS
                flags.joined = state != 1 && state != 0 && state != 5 && state != 6;
                flags.queued = state == 2 || state == 7;
                break;
            default:
                break;
        }

        flags.lfgJoined = reason != 8;            // REMOVED_FROM_QUEUE
        return flags;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_JOIN_RESULT (0x38B6) -- sub_1403C11F0 / read sub_1403BFF40
    // ---------------------------------------------------------------------

    /// One blacklisted-dungeon lock reason inside a JoinResultBlacklist
    /// entry. Field order (Reason, SubReason1, SubReason2, Slot) is NOT
    /// the same as LFGLockedDungeon's (Slot, Reason, SubReason1,
    /// SubReason2) -- confirmed against tc-preservation and flagged
    /// already by 3a's own comment on LFGLockedDungeon above.
    struct JoinResultBlacklistSlot
    {
        uint32 slot = 0;
        uint32 reason = 0;
        uint32 subReason1 = 0;
        uint32 subReason2 = 0;
    };

    struct JoinResultBlacklist
    {
        uint64 guid = 0;              ///< player or group guid, packed
        std::vector<JoinResultBlacklistSlot> slots;
    };

    struct JoinResult
    {
        RideTicket ticket;
        uint8 result = 0;
        uint8 resultDetail = 0;
        std::vector<JoinResultBlacklist> blacklist;
    };

    /// Builds SMSG_LFG_JOIN_RESULT. False (and `out` untouched) if
    /// blacklist exceeds its 24-bit wire count, or any entry's slots
    /// exceeds its 22-bit wire count. Bit region totals 32 + 30*N bits,
    /// N = blacklist.size(), matching the plan.
    inline bool BuildJoinResult(WorldPacket& out, JoinResult const& data)
    {
        if (!LFGPacketDetail::FitsBits(data.blacklist.size(), 24))
        {
            return false;
        }

        for (JoinResultBlacklist const& bl : data.blacklist)
        {
            if (!LFGPacketDetail::FitsBits(bl.slots.size(), 22))
            {
                return false;
            }
        }

        std::array<uint8, 8> g = LFGPacketDetail::GuidBytes(data.ticket.requesterGuid);

        out << uint32(data.ticket.type);
        out << uint8(data.result);
        out << uint32(data.ticket.id);
        out << uint8(data.resultDetail);
        out << uint32(data.ticket.time);

        out.WriteBit(g[2]);
        out.WriteBit(g[7]);
        out.WriteBit(g[3]);
        out.WriteBit(g[0]);
        out.WriteBits(uint32(data.blacklist.size()), 24);

        std::vector<std::array<uint8, 8>> blacklistGuids;
        blacklistGuids.reserve(data.blacklist.size());
        for (JoinResultBlacklist const& bl : data.blacklist)
        {
            std::array<uint8, 8> bg = LFGPacketDetail::GuidBytes(bl.guid);
            out.WriteBit(bg[7]);
            out.WriteBit(bg[5]);
            out.WriteBit(bg[3]);
            out.WriteBit(bg[6]);
            out.WriteBit(bg[0]);
            out.WriteBit(bg[2]);
            out.WriteBit(bg[4]);
            out.WriteBit(bg[1]);
            out.WriteBits(uint32(bl.slots.size()), 22);
            blacklistGuids.push_back(bg);
        }

        out.WriteBit(g[4]);
        out.WriteBit(g[5]);
        out.WriteBit(g[1]);
        out.WriteBit(g[6]);
        out.FlushBits();

        for (size_t i = 0; i < data.blacklist.size(); ++i)
        {
            JoinResultBlacklist const& bl = data.blacklist[i];
            for (JoinResultBlacklistSlot const& slot : bl.slots)
            {
                out << slot.reason;
                out << slot.subReason1;
                out << slot.subReason2;
                out << slot.slot;
            }

            std::array<uint8, 8> const& bg = blacklistGuids[i];
            out.WriteByteSeq(bg[2]);
            out.WriteByteSeq(bg[5]);
            out.WriteByteSeq(bg[1]);
            out.WriteByteSeq(bg[0]);
            out.WriteByteSeq(bg[4]);
            out.WriteByteSeq(bg[3]);
            out.WriteByteSeq(bg[6]);
            out.WriteByteSeq(bg[7]);
        }

        out.WriteByteSeq(g[1]);
        out.WriteByteSeq(g[4]);
        out.WriteByteSeq(g[3]);
        out.WriteByteSeq(g[5]);
        out.WriteByteSeq(g[0]);
        out.WriteByteSeq(g[7]);
        out.WriteByteSeq(g[2]);
        out.WriteByteSeq(g[6]);

        return true;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_PROPOSAL_UPDATE (0x7DA6) -- sub_1403AE020 / read sub_14039E750
    // ---------------------------------------------------------------------

    struct ProposalUpdatePlayer
    {
        uint32 roles = 0;
        bool me = false;
        bool sameParty = false;
        bool myParty = false;
        bool responded = false;
        bool accepted = false;
    };

    struct ProposalUpdate
    {
        RideTicket ticket;
        uint64 instanceId = 0;        ///< dungeon instance map guid, packed
        uint32 proposalId = 0;
        uint32 slot = 0;
        int8 state = 0;
        uint32 completedMask = 0;
        bool proposalSilent = false;
        std::vector<ProposalUpdatePlayer> players;
    };

    /// Builds SMSG_LFG_PROPOSAL_UPDATE. False (and `out` untouched) if
    /// players exceeds its 23-bit wire count. Bit region totals
    /// 40 + 5*N bits, N = players.size(), matching the plan.
    inline bool BuildProposalUpdate(WorldPacket& out, ProposalUpdate const& data)
    {
        if (!LFGPacketDetail::FitsBits(data.players.size(), 23))
        {
            return false;
        }

        std::array<uint8, 8> g = LFGPacketDetail::GuidBytes(data.ticket.requesterGuid);
        std::array<uint8, 8> ig = LFGPacketDetail::GuidBytes(data.instanceId);

        out << uint32(data.ticket.time);
        out << uint32(data.completedMask);
        out << uint32(data.ticket.id);
        out << uint32(data.ticket.type);
        out << uint32(data.slot);
        out << uint32(data.proposalId);
        out << uint8(data.state);

        out.WriteBit(ig[4]);
        out.WriteBit(g[3]);
        out.WriteBit(g[7]);
        out.WriteBit(g[0]);
        out.WriteBit(ig[1]);
        out.WriteBit(data.proposalSilent);
        out.WriteBit(g[4]);
        out.WriteBit(g[5]);
        out.WriteBit(ig[3]);
        out.WriteBits(uint32(data.players.size()), 23);
        out.WriteBit(ig[7]);

        for (ProposalUpdatePlayer const& p : data.players)
        {
            out.WriteBit(p.sameParty);
            out.WriteBit(p.myParty);
            out.WriteBit(p.accepted);
            out.WriteBit(p.responded);
            out.WriteBit(p.me);
        }

        out.WriteBit(ig[5]);
        out.WriteBit(g[6]);
        out.WriteBit(ig[2]);
        out.WriteBit(ig[6]);
        out.WriteBit(g[2]);
        out.WriteBit(g[1]);
        out.WriteBit(ig[0]);
        out.FlushBits();

        out.WriteByteSeq(g[5]);
        out.WriteByteSeq(ig[3]);
        out.WriteByteSeq(ig[6]);
        out.WriteByteSeq(g[6]);
        out.WriteByteSeq(g[0]);
        out.WriteByteSeq(ig[5]);
        out.WriteByteSeq(g[1]);

        for (ProposalUpdatePlayer const& p : data.players)
        {
            out << p.roles;
        }

        out.WriteByteSeq(ig[7]);
        out.WriteByteSeq(g[4]);
        out.WriteByteSeq(ig[0]);
        out.WriteByteSeq(ig[1]);
        out.WriteByteSeq(g[2]);
        out.WriteByteSeq(g[7]);
        out.WriteByteSeq(ig[2]);
        out.WriteByteSeq(g[3]);
        out.WriteByteSeq(ig[4]);

        return true;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_SLOT_INVALID (0x54B5) -- sub_14034E940 / read sub_14034E120
    // ---------------------------------------------------------------------

    /// Wire layout confirmed directly from sub_14034E120: three plain
    /// uint32 reads (offsets +36, +40, +32 of the message object, in that
    /// order), no bit-packing at all despite living in the disp2 table.
    /// tc-preservation does not implement this opcode at all
    /// (STATUS_UNHANDLED in its Opcodes.cpp).
    ///
    /// Names come from the client: handler sub_1406FAA60 fires event 0x20B
    /// ("LFG_INVALID_ERROR_MESSAGE") with args (a1[9], a1[8], a1[10]), and
    /// FrameXML/LFGFrame.lua:189-191 unpacks those as
    /// `reason, reasonArg1, reasonArg2`, indexing LFG_INSTANCE_INVALID_CODES
    /// by `reason`. Mapping that back through the read order gives the wire
    /// sequence below -- note arg2 precedes arg1 on the wire. There is no
    /// dungeon/slot field in this packet despite the opcode name.
    struct SlotInvalid
    {
        uint32 reason = 0;                              ///< LFG_INSTANCE_INVALID_CODES index
        uint32 reasonArg2 = 0;                          ///< second Lua arg; wire-precedes arg1
        uint32 reasonArg1 = 0;                          ///< first Lua arg
    };

    /// Builds SMSG_LFG_SLOT_INVALID. Fixed shape, always succeeds.
    inline void BuildSlotInvalid(WorldPacket& out, SlotInvalid const& data)
    {
        out << data.reason;
        out << data.reasonArg2;
        out << data.reasonArg1;
    }

    // ---------------------------------------------------------------------
    // SMSG_LFG_UPDATE_STATUS_NONE (0x7CA1) -- sub_14036ACB0
    // ---------------------------------------------------------------------

    /// Builds SMSG_LFG_UPDATE_STATUS_NONE. The Ctor (sub_14036ACB0) never
    /// calls a field-reading sub-function at all -- unlike every other
    /// disp2 LFG packet's Ctor, which all call into a dedicated reader
    /// sub-function -- so the client reads zero payload bytes for this
    /// opcode. tc-preservation does not implement it either
    /// (STATUS_NEVER). Exists only so the opcode has the same Build*
    /// call shape as the rest of this file.
    inline void BuildUpdateStatusNone(WorldPacket& /*out*/)
    {
    }

    // ---------------------------------------------------------------------
    // CMSG readers. SMSG_LFG_SEARCH_RESULTS (0x54A1) is genuinely NOT
    // DECODED and is not implemented. Its client-send counterparts
    // CMSG_LFG_SEARCH_JOIN (sub_140314100, opcode 0x531 + one uint32) and
    // CMSG_LFG_SEARCH_LEAVE (sub_140314CB0, opcode 0x500 + one uint32) ARE
    // decoded, but belong to the Raid Browser, which is out of scope here.
    // CMSG_LFG_GET_STATUS (0x2581) is confirmed empty: its send function
    // sub_140312420 writes the opcode and nothing else.
    // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------
    // CMSG_LFG_JOIN (0x2430) -- client send sub_140316A50
    // ---------------------------------------------------------------------

    struct JoinRequest
    {
        uint32 roles = 0;
        std::array<uint32, 3> needs{};   ///< hardcoded 3-wide on the client
        std::vector<uint32> slots;
        std::string comment;
    };

    /// Parses CMSG_LFG_JOIN. sub_140316A50 is unobfuscated and was
    /// decompiled directly: Roles, 3x Needs, a 9-bit comment length, a
    /// 24-bit slot count, the raw comment bytes, then the slot list --
    /// byte-for-byte identical field order to tc-preservation's
    /// LFGJoin::Read().
    inline void ReadJoin(WorldPacket& data, JoinRequest& out)
    {
        data >> out.roles;
        for (uint32& need : out.needs)
        {
            data >> need;
        }

        uint32 commentLength = data.ReadBits(9);
        out.slots.resize(data.ReadBits(24));
        out.comment = data.ReadString(commentLength);

        for (uint32& slot : out.slots)
        {
            data >> slot;
        }
    }

    // ---------------------------------------------------------------------
    // CMSG_LFG_LEAVE (0x2433) -- client send sub_1403158F0 + sub_1403424D0
    // ---------------------------------------------------------------------

    struct LeaveRequest
    {
        uint32 roles = 0;
        RideTicket ticket;
    };

    /// Parses CMSG_LFG_LEAVE. sub_1403158F0 (unobfuscated) writes Roles
    /// then delegates the ticket to sub_1403424D0, a shared helper also
    /// used by CMSG_LFG_PROPOSAL_RESULT. Both the WriteBit mask order and
    /// the WriteByteSeq order were hand-traced through sub_1403424D0's
    /// Horner-expression byte-pack and confirmed to match
    /// tc-preservation's LFGLeave::Read() exactly, bit for bit.
    inline void ReadLeave(WorldPacket& data, LeaveRequest& out)
    {
        data >> out.roles;
        data >> out.ticket.time;
        uint32 type = 0;
        data >> type;
        out.ticket.type = RideType(type);
        data >> out.ticket.id;

        std::array<uint8, 8> g{};
        g[4] = uint8(data.ReadBit());
        g[5] = uint8(data.ReadBit());
        g[0] = uint8(data.ReadBit());
        g[6] = uint8(data.ReadBit());
        g[2] = uint8(data.ReadBit());
        g[7] = uint8(data.ReadBit());
        g[1] = uint8(data.ReadBit());
        g[3] = uint8(data.ReadBit());

        data.ReadByteSeq(g[7]);
        data.ReadByteSeq(g[4]);
        data.ReadByteSeq(g[3]);
        data.ReadByteSeq(g[2]);
        data.ReadByteSeq(g[6]);
        data.ReadByteSeq(g[0]);
        data.ReadByteSeq(g[1]);
        data.ReadByteSeq(g[5]);

        out.ticket.requesterGuid = LFGPacketDetail::AssembleGuid(g);
    }

    // ---------------------------------------------------------------------
    // CMSG_LFG_PROPOSAL_RESULT (0x0403) -- client send sub_140315360
    // ---------------------------------------------------------------------

    struct ProposalResult
    {
        uint32 proposalId = 0;
        RideTicket ticket;
        uint64 instanceId = 0;        ///< packed, second guid in the packet
        bool accepted = false;
    };

    /// Parses CMSG_LFG_PROPOSAL_RESULT. ProposalId then the ticket (via
    /// the same sub_1403424D0 helper as CMSG_LFG_LEAVE, so that part is
    /// confirmed the same way). The second bit region (Accepted
    /// interleaved with the 8 instanceId guid bits) was hand-traced
    /// through sub_140315360's Horner expression; the 8 struct offsets
    /// it references map bijectively onto tc-preservation's named
    /// instanceId[7,1,3,0,5,4,6,2] sequence with Accepted as the 2nd bit,
    /// confirming the order below.
    inline void ReadProposalResult(WorldPacket& data, ProposalResult& out)
    {
        data >> out.proposalId;
        data >> out.ticket.time;
        uint32 type = 0;
        data >> type;
        out.ticket.type = RideType(type);
        data >> out.ticket.id;

        std::array<uint8, 8> g{};
        g[4] = uint8(data.ReadBit());
        g[5] = uint8(data.ReadBit());
        g[0] = uint8(data.ReadBit());
        g[6] = uint8(data.ReadBit());
        g[2] = uint8(data.ReadBit());
        g[7] = uint8(data.ReadBit());
        g[1] = uint8(data.ReadBit());
        g[3] = uint8(data.ReadBit());

        data.ReadByteSeq(g[7]);
        data.ReadByteSeq(g[4]);
        data.ReadByteSeq(g[3]);
        data.ReadByteSeq(g[2]);
        data.ReadByteSeq(g[6]);
        data.ReadByteSeq(g[0]);
        data.ReadByteSeq(g[1]);
        data.ReadByteSeq(g[5]);

        out.ticket.requesterGuid = LFGPacketDetail::AssembleGuid(g);

        std::array<uint8, 8> ig{};
        ig[7] = uint8(data.ReadBit());
        out.accepted = data.ReadBit();
        ig[1] = uint8(data.ReadBit());
        ig[3] = uint8(data.ReadBit());
        ig[0] = uint8(data.ReadBit());
        ig[5] = uint8(data.ReadBit());
        ig[4] = uint8(data.ReadBit());
        ig[6] = uint8(data.ReadBit());
        ig[2] = uint8(data.ReadBit());

        data.ReadByteSeq(ig[7]);
        data.ReadByteSeq(ig[1]);
        data.ReadByteSeq(ig[5]);
        data.ReadByteSeq(ig[6]);
        data.ReadByteSeq(ig[3]);
        data.ReadByteSeq(ig[4]);
        data.ReadByteSeq(ig[0]);
        data.ReadByteSeq(ig[2]);

        out.instanceId = LFGPacketDetail::AssembleGuid(ig);
    }

    // ---------------------------------------------------------------------
    // CMSG_LFG_SET_ROLES (0x0480) -- client send sub_1403140C0
    // ---------------------------------------------------------------------

    struct SetRolesRequest
    {
        uint32 rolesDesired = 0;
    };

    /// Parses CMSG_LFG_SET_ROLES. sub_1403140C0 writes exactly one plain
    /// uint32; trivial, fully confirmed.
    inline void ReadSetRoles(WorldPacket& data, SetRolesRequest& out)
    {
        data >> out.rolesDesired;
    }

    // ---------------------------------------------------------------------
    // CMSG_SET_LFG_COMMENT (0x0530) -- client send sub_1403162C0
    // ---------------------------------------------------------------------

    struct SetCommentRequest
    {
        std::string comment;
    };

    /// Parses CMSG_SET_LFG_COMMENT. sub_1403162C0 (unobfuscated) writes a
    /// 9-bit length then the raw string bytes -- matches
    /// tc-preservation's LFGSetComment::Read() exactly.
    inline void ReadSetComment(WorldPacket& data, SetCommentRequest& out)
    {
        uint32 length = data.ReadBits(9);
        out.comment = data.ReadString(length);
    }

    // ---------------------------------------------------------------------
    // CMSG_LFG_SET_BOOT_VOTE (0x04B3) -- client send sub_140314350
    // ---------------------------------------------------------------------

    struct SetBootVoteRequest
    {
        bool vote = false;
    };

    /// Parses CMSG_LFG_SET_BOOT_VOTE. DIVERGES FROM tc-preservation, which
    /// reads this field as a plain byte (`_worldPacket >> Vote`).
    /// sub_140314350 was traced into the client's low-level append
    /// primitives (sub_140313E20 -> sub_140313BE0 -> sub_1405A7830): the
    /// pending value is left-shifted into bit 7 before the byte hits the
    /// wire (0x80 for true, 0x00 for false), which is a single WriteBit()
    /// call, not a plain uint8 write -- `read<char>() > 0` would read
    /// 0x80 as a negative signed char and silently always return false.
    /// Implemented as ReadBit() per the binary; flagged in the PR/report
    /// for a second look, since tc-preservation is inconsistent between
    /// this field and CMSG_LFG_LOCK_INFO_REQUEST's Player (which it does
    /// read via ReadBit(), matching this same client codegen pattern).
    inline void ReadSetBootVote(WorldPacket& data, SetBootVoteRequest& out)
    {
        out.vote = data.ReadBit();
    }

    // ---------------------------------------------------------------------
    // CMSG_LFG_TELEPORT (0x2482) -- client send sub_140314790
    // ---------------------------------------------------------------------

    struct TeleportRequest
    {
        bool teleportOut = false;
    };

    /// Parses CMSG_LFG_TELEPORT. Same divergence from tc-preservation and
    /// same evidence as ReadSetBootVote above -- sub_140314790 is
    /// byte-for-byte the same codegen pattern as sub_140314350.
    inline void ReadTeleport(WorldPacket& data, TeleportRequest& out)
    {
        out.teleportOut = data.ReadBit();
    }

    // ---------------------------------------------------------------------
    // CMSG_LFG_LOCK_INFO_REQUEST (0x0412) -- client send sub_1403141C0
    // ---------------------------------------------------------------------

    struct LockInfoRequest
    {
        bool player = false;
    };

    /// Parses CMSG_LFG_LOCK_INFO_REQUEST. Matches tc-preservation's
    /// LFGGetSystemInfo::Read() (ReadBit()) -- consistent with the same
    /// single-WriteBit-then-flush codegen traced for BootVote/Teleport
    /// above (sub_1403141C0 is structurally identical to sub_140314350
    /// and sub_140314790).
    inline void ReadLockInfoRequest(WorldPacket& data, LockInfoRequest& out)
    {
        out.player = data.ReadBit();
    }
}

#endif
