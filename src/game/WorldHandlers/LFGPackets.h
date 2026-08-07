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
 * @brief 4.3.4 (build 15595) LFG packet layer -- the flat, non-bit-packed half.
 *
 * Every layout here was transcribed from the client's own reader functions in
 * client-truth/binary/Wow-64.c and independently re-verified against the live
 * IDA database during implementation (see LFD_CATA_ANALYSIS.md section 3.8-3.20).
 * The four bit-packed disp2 packets (SMSG_LFG_UPDATE_STATUS, SMSG_LFG_JOIN_RESULT,
 * SMSG_LFG_QUEUE_STATUS, SMSG_LFG_PROPOSAL_UPDATE) are deliberately NOT here --
 * that is Phase 3b.
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

    /// Builds SMSG_ROLE_CHOSEN. Fixed shape, always succeeds.
    inline void BuildRoleChosen(WorldPacket& out, uint64 guid, uint32 roleMask)
    {
        out << uint64(guid);
        out << uint8(roleMask > 0);
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
    // ---------------------------------------------------------------------

    struct BootProposalUpdate
    {
        bool voteInProgress = false;
        bool myVoteCompleted = false;
        bool myVote = false;
        bool voteEcho = false;      ///< fourth wire flag; not read back by
                                     ///< this handler, name is inferred
        uint64 target = 0;          ///< victim guid, raw
        uint32 totalVotes = 0;
        uint32 bootVotes = 0;
        uint32 timeLeft = 0;        ///< seconds
        uint32 votesNeeded = 0;
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
        out << uint8(data.myVoteCompleted);
        out << uint8(data.myVote);
        out << uint8(data.voteEcho);
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
        std::array<LFGShortageReward, 3> shortageTiers;   ///< tank, healer, dps
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
}

#endif
