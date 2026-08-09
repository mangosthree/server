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

/**
 * @file LFGHandler.cpp
 * @brief 4.3.4 Dungeon Finder opcode handlers and SMSG senders. Packet
 *        layouts live in LFGPackets.h; queue logic lives in LFGMgr.
 */

#include <string>
#include <vector>
#include <set>
#include "WorldSession.h"
#include "DBCStores.h"
#include "Group.h"
#include "LFGMgr.h"
#include "LFGPackets.h"
#include "Log.h"
#include "Player.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "World.h"
#include "PlayerRegistry.h"

void WorldSession::HandleLfgJoinOpcode(WorldPacket& recv_data)
{
    DEBUG_FILTER_LOG(LOG_FILTER_LFG, "CMSG_LFG_JOIN");

    LFGPackets::JoinRequest request;
    LFGPackets::ReadJoin(recv_data, request);

    if (!sWorld.getConfig(CONFIG_BOOL_LFG_ENABLE))
    {
        return;
    }

    Player* player = GetPlayer();

    // The client only greys the button; refuse silently, exactly as retail
    // does -- there is no "leader only" message in GlobalStrings.
    if (!LFGMgr::IsEmpowered(player))
    {
        return;
    }

    std::set<uint32> dungeons;
    for (uint32 slot : request.slots)
    {
        uint32 dungeonId = slot & 0x00FFFFFF;
        if (sLfgDungeonsStore.LookupEntry(dungeonId))
        {
            dungeons.insert(dungeonId);
        }
    }

    if (dungeons.empty())
    {
        return;
    }

    sLFGMgr.JoinLFG(request.roles, dungeons, request.comment, player);
}

void WorldSession::HandleLfgLeaveOpcode(WorldPacket& recv_data)
{
    DEBUG_FILTER_LOG(LOG_FILTER_LFG, "CMSG_LFG_LEAVE");

    LFGPackets::LeaveRequest request;
    LFGPackets::ReadLeave(recv_data, request);

    Player* player = GetPlayer();
    Group* group = player->GetGroup();

    // The client only greys the button; refuse silently, exactly as retail
    // does -- there is no "leader only" message in GlobalStrings. The ticket
    // guid the client echoes is deliberately ignored in favour of the
    // session's own identity.
    if (!LFGMgr::IsEmpowered(player))
    {
        return;
    }

    sLFGMgr.LeaveLFG(player, group != nullptr);
}

void WorldSession::HandleLfgSetRolesOpcode(WorldPacket& recv_data)
{
    LFGPackets::SetRolesRequest request;
    LFGPackets::ReadSetRoles(recv_data, request);

    Player* player = GetPlayer();
    Group* group = player->GetGroup();
    if (!group)
    {
        return;                          // solo role choices are client-local
    }

    uint32 roles = request.rolesDesired;
    if (!LFGMgr::CanPerformSelectedRoles(player->getClass(), uint8(roles)))
    {
        roles = 0;                       // treated as "no role" => check fails
    }

    DEBUG_FILTER_LOG(LOG_FILTER_LFG, "CMSG_LFG_SET_ROLES roles %u", roles);
    sLFGMgr.PerformRoleCheck(player, group, uint8(roles));
}

void WorldSession::HandleLfgGetStatusOpcode(WorldPacket& /*recv_data*/)
{
    DEBUG_FILTER_LOG(LOG_FILTER_LFG, "CMSG_LFG_GET_STATUS");

    if (!sWorld.getConfig(CONFIG_BOOL_LFG_ENABLE))
    {
        return;
    }

    sLFGMgr.SendStatusUpdate(GetPlayer());
}

void WorldSession::HandleLfgProposalResultOpcode(WorldPacket& recv_data)
{
    LFGPackets::ProposalResult request;
    LFGPackets::ReadProposalResult(recv_data, request);

    DEBUG_FILTER_LOG(LOG_FILTER_LFG,
        "CMSG_LFG_PROPOSAL_RESULT proposal %u accepted %u",
        request.proposalId, uint32(request.accepted));

    // The echoed ticket/instance guid are informational (spec V4); the
    // session's own player is the identity that answers.
    sLFGMgr.ProposalUpdate(request.proposalId, GetPlayer()->GetObjectGuid(),
                           request.accepted);
}

void WorldSession::HandleLfgTeleportOpcode(WorldPacket& recv_data)
{
    LFGPackets::TeleportRequest request;
    LFGPackets::ReadTeleport(recv_data, request);

    DEBUG_FILTER_LOG(LOG_FILTER_LFG, "CMSG_LFG_TELEPORT out %u",
        uint32(request.teleportOut));

    // No LFG.Enable gate: without LFG no group status exists and the
    // manager denies with INVALID_LOCATION on its own.
    sLFGMgr.TeleportPlayer(GetPlayer(), request.teleportOut, false);
}

void WorldSession::HandleSearchLfgJoinOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_LFG_SEARCH_JOIN");

    uint32 temp, entry;
    recv_data >> temp;

    entry = (temp & 0x00FFFFFF);
    // LfgType type = LfgType((temp >> 24) & 0x000000FF);

    // SendLfgSearchResults(type, entry);
}

void WorldSession::HandleSearchLfgLeaveOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_LFG_SEARCH_LEAVE");

    recv_data >> Unused<uint32>();                          // join id?
}

void WorldSession::HandleSetLfgCommentOpcode(WorldPacket& recv_data)
{
    DEBUG_FILTER_LOG(LOG_FILTER_LFG, "CMSG_SET_LFG_COMMENT");

    LFGPackets::SetCommentRequest request;
    LFGPackets::ReadSetComment(recv_data, request);

    sLFGMgr.SetPlayerComment(GetPlayer()->GetObjectGuid(), request.comment);
}

/// Fires on Dungeon Finder panel open (LFDFrame.xml's OnShow calls both
/// RequestLFDPlayerLockInfo and RequestLFDPartyLockInfo, each a separate
/// CMSG_LFG_LOCK_INFO_REQUEST distinguished by this one bit -- section 3.22).
void WorldSession::HandleLfgLockInfoRequestOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_LFG_LOCK_INFO_REQUEST");

    LFGPackets::LockInfoRequest request;
    LFGPackets::ReadLockInfoRequest(recv_data, request);

    if (request.player)
    {
        SendLfgPlayerLockInfo();
    }
    else
    {
        SendLfgPartyLockInfo();
    }
}

void WorldSession::SendLfgSearchResults(LfgType type, uint32 entry)
{
    WorldPacket data(SMSG_LFG_SEARCH_RESULTS);
    data << uint32(type);                                   // type
    data << uint32(entry);                                  // entry from LFGDungeons.dbc

    uint8 isGuidsPresent = 0;
    data << uint8(isGuidsPresent);
    if (isGuidsPresent)
    {
        uint32 guids_count = 0;
        data << uint32(guids_count);
        for (uint32 i = 0; i < guids_count; ++i)
        {
            data << uint64(0);                              // player/group guid
        }
    }

    uint32 groups_count = 1;
    data << uint32(groups_count);                           // groups count
    data << uint32(groups_count);                           // groups count (total?)

    for (uint32 i = 0; i < groups_count; ++i)
    {
        data << uint64(1);                                  // group guid

        uint32 flags = 0x92;
        data << uint32(flags);                              // flags

        if (flags & 0x2)
        {
            data << uint8(0);                               // comment string, max len 256
        }

        if (flags & 0x10)
        {
            for (uint32 j = 0; j < 3; ++j)
            {
                data << uint8(0);                           // roles
            }
        }

        if (flags & 0x80)
        {
            data << uint64(0);                              // instance guid
            data << uint32(0);                              // completed encounters
        }
    }

    size_t pl_count_pos = data.wpos();
    data << uint32(0);                            // players count

    size_t tpl_count_pos = data.wpos();
    data << uint32(0);                            // players count (total?)

    uint32 playerCount = 0;

    sPlayerRegistry.ForEach([this, &playerCount, &data](Player* plr)->void
    {
        ++playerCount;
        if (plr && (plr->GetTeam() == _player->GetTeam()) && plr->IsInWorld())
        {
            data << plr->GetObjectGuid();                       // guid

            uint32 flags = 0xFF;
            data << uint32(flags);                              // flags

            if (flags & 0x1)
            {
                data << uint8(plr->getLevel());
                data << uint8(plr->getClass());
                data << uint8(plr->getRace());

                for (uint32 i = 0; i < 3; ++i)
                {
                    data << uint8(0);                           // talent spec x/x/x
                }

                data << uint32(0);                              // armor
                data << uint32(0);                              // spd/heal
                data << uint32(0);                              // spd/heal
                data << uint32(0);                              // HasteMelee
                data << uint32(0);                              // HasteRanged
                data << uint32(0);                              // HasteSpell
                data << float(0);                               // MP5
                data << float(0);                               // MP5 Combat
                data << uint32(0);                              // AttackPower
                data << uint32(0);                              // Agility
                data << uint32(0);                              // Health
                data << uint32(0);                              // Mana
                data << uint32(0);                              // Unk1
                data << float(0);                               // Unk2
                data << uint32(0);                              // Defence
                data << uint32(0);                              // Dodge
                data << uint32(0);                              // Block
                data << uint32(0);                              // Parry
                data << uint32(0);                              // Crit
                data << uint32(0);                              // Expertise
            }

            if (flags & 0x2)
            {
                data << "";                                     // comment
            }

            if (flags & 0x4)
            {
                data << uint8(0);                               // group leader
            }

            if (flags & 0x8)
            {
                data << uint64(1);                              // group guid
            }

            if (flags & 0x10)
            {
                data << uint8(0);                               // roles
            }

            if (flags & 0x20)
            {
                data << uint32(plr->GetTerrain()->GetZoneId(plr->Where().X(), plr->Where().Y(), plr->Where().Z()));               // areaid
            }

            if (flags & 0x40)
            {
                data << uint8(0);                               // status
            }

            if (flags & 0x80)
            {
                data << uint64(0);                              // instance guid
                data << uint32(0);                              // completed encounters
            }
        }
    });

    data.put(pl_count_pos, playerCount);
    data.put(tpl_count_pos, playerCount);

    SendPacket(&data);
}

void WorldSession::SendLfgJoinResult(LfgJoinResult result, uint8 resultDetail,
                                     RideTicket const& ticket,
                                     std::vector<LFGPackets::JoinResultBlacklist> const& blacklist)
{
    LFGPackets::JoinResult data;
    data.ticket = ticket;
    data.result = uint8(result);
    data.resultDetail = resultDetail;
    data.blacklist = blacklist;

    WorldPacket packet(SMSG_LFG_JOIN_RESULT, 32);
    if (LFGPackets::BuildJoinResult(packet, data))
    {
        SendPacket(&packet);
    }
}

void WorldSession::SendLfgUpdateStatus(LFGPackets::UpdateStatus const& status)
{
    WorldPacket packet(SMSG_LFG_UPDATE_STATUS, 32 + status.slots.size() * 4);
    if (LFGPackets::BuildUpdateStatus(packet, status))
    {
        SendPacket(&packet);
    }
}

void WorldSession::SendLfgQueueStatus(LFGPackets::QueueStatus const& status)
{
    WorldPacket packet(SMSG_LFG_QUEUE_STATUS, 47);
    LFGPackets::BuildQueueStatus(packet, status);
    SendPacket(&packet);
}

void WorldSession::SendLfgRoleCheckUpdate(LFGPackets::RoleCheckUpdate const& roleCheck)
{
    WorldPacket packet(SMSG_LFG_ROLE_CHECK_UPDATE,
                       7 + roleCheck.dungeons.size() * 4 + roleCheck.members.size() * 14);
    if (LFGPackets::BuildRoleCheckUpdate(packet, roleCheck))
    {
        SendPacket(&packet);
    }
}

void WorldSession::SendLfgRoleChosen(uint64 rawGuid, uint32 roles)
{
    WorldPacket packet(SMSG_ROLE_CHOSEN, 13);
    LFGPackets::BuildRoleChosen(packet, rawGuid, roles);
    SendPacket(&packet);
}

void WorldSession::SendLfgProposalUpdate(LFGPackets::ProposalUpdate const& proposal)
{
    WorldPacket packet(SMSG_LFG_PROPOSAL_UPDATE, 36 + proposal.players.size() * 6);
    if (LFGPackets::BuildProposalUpdate(packet, proposal))
    {
        SendPacket(&packet);
    }
}

void WorldSession::SendLfgTeleportError(uint8 error)
{
    WorldPacket data(SMSG_LFG_TELEPORT_DENIED, 4);
    LFGPackets::BuildTeleportDenied(data, uint32(error));
    SendPacket(&data);
}

void WorldSession::SendLfgRewards(LFGRewards const& rewards)
{
    DEBUG_LOG("SMSG_LFG_PLAYER_REWARD");

    WorldPacket data(SMSG_LFG_PLAYER_REWARD, 42);
    data << uint32(rewards.randomDungeonEntry);
    data << uint32(rewards.groupDungeonEntry);
    data << uint8(rewards.hasDoneDaily);
    data << uint32(1);
    data << uint32(rewards.moneyReward);
    data << uint32(rewards.expReward);
    data << uint32(0);
    data << uint32(0);
    if (rewards.itemID != 0)
    {
        ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(rewards.itemID);
        if (pProto)
        {
            data << uint8(1);
            data << uint32(rewards.itemID);
            data << uint32(pProto->DisplayInfoID);
            data << uint32(rewards.itemAmount);
        }
    }
    else
    {
        data << uint8(0);
    }
    SendPacket(&data);
}

/// Sends SMSG_LFG_BOOT_PROPOSAL_UPDATE for one recipient.
///
/// timeLeft is computed inline here rather than through
/// LFGBootLogic::RemainingSeconds (LFGBootLogic.h, added in a later commit
/// of this same series) so this commit stays independently buildable; the
/// clamp formula is identical -- no `/1000`, no `uint8` truncation, floored
/// at 0 (C-8).
void WorldSession::SendLfgBootProposalUpdate(LFGBoot const& boot, uint32 votesNeeded)
{
    DEBUG_LOG("SMSG_LFG_BOOT_PROPOSAL_UPDATE");

    ObjectGuid plrGuid = GetPlayer()->GetObjectGuid();

    proposalAnswerMap::const_iterator ansItr = boot.answers.find(plrGuid);
    LFGProposalAnswer plrAnswer = (ansItr != boot.answers.end()) ? ansItr->second : LFG_ANSWER_PENDING;

    uint32 totalVotes = 0, bootVotes = 0;
    for (proposalAnswerMap::const_iterator it = boot.answers.begin(); it != boot.answers.end(); ++it)
    {
        if (it->second != LFG_ANSWER_PENDING)
        {
            ++totalVotes;
            if (it->second == LFG_ANSWER_AGREE)
            {
                ++bootVotes;
            }
        }
    }

    time_t const now = time(NULL);
    time_t const deadline = boot.startTime + LFG_TIME_BOOT;
    uint32 const timeLeft = (deadline > now) ? uint32(deadline - now) : 0;

    LFGPackets::BootProposalUpdate data;
    data.voteInProgress = boot.inProgress;
    data.votePassed = false;   // LFGBoot gains a real votePassed field in a
                                // later commit of this series; wired there.
    data.myVoteCompleted = plrAnswer != LFG_ANSWER_PENDING;
    data.myVote = plrAnswer == LFG_ANSWER_AGREE;
    data.target = boot.playerVotedOn.GetRawValue();
    data.totalVotes = totalVotes;
    data.bootVotes = bootVotes;
    data.timeLeft = timeLeft;
    data.votesNeeded = votesNeeded;
    data.reason = boot.reason;

    WorldPacket packet(SMSG_LFG_BOOT_PROPOSAL_UPDATE, 28 + boot.reason.length() + 1);
    if (LFGPackets::BuildBootProposalUpdate(packet, data))
    {
        SendPacket(&packet);
    }
    else
    {
        sLog.outError("WorldSession::SendLfgBootProposalUpdate: reason too long (%u bytes), packet not sent",
                      uint32(boot.reason.length()));
    }
}

/// Builds and sends SMSG_LFG_PLAYER_INFO for the requesting player. Drives
/// the padlocks in the Dungeon Finder's own list (LFG_LOCK_INFO_RECEIVED).
/// randomDungeons is intentionally left empty: the reward fields it carries
/// (Completion/Purse/shortage-tier quantities) are Phase 9's job, and the
/// client tolerates an empty array here -- it just leaves the "Type:"
/// dropdown in specific-dungeon mode (section 1.3) until that phase lands.
void WorldSession::SendLfgPlayerLockInfo()
{
    Player* player = GetPlayer();

    LFGPackets::PlayerInfo info;
    info.lockedDungeons = sLFGMgr.GetPlayerLockList(player);

    WorldPacket data(SMSG_LFG_PLAYER_INFO, 5 + info.lockedDungeons.size() * 16);
    if (LFGPackets::BuildPlayerInfo(data, info))
    {
        SendPacket(&data);
    }
}

/// Builds and sends SMSG_LFG_PARTY_INFO for the requesting player's group.
/// Self is skipped (section 3.11); with no group this sends a valid,
/// empty member list.
void WorldSession::SendLfgPartyLockInfo()
{
    Player* player = GetPlayer();

    LFGPackets::PartyInfo info;

    if (Group* group = player->GetGroup())
    {
        ObjectGuid selfGuid = player->GetObjectGuid();

        for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* member = itr->getSource();
            if (!member || member->GetObjectGuid() == selfGuid)
            {
                continue;
            }

            LFGPackets::LFGPartyInfoMember memberInfo;
            memberInfo.guid = member->GetObjectGuid().GetRawValue();
            memberInfo.lockedDungeons = sLFGMgr.GetPlayerLockList(member);

            info.members.push_back(memberInfo);
        }
    }

    WorldPacket data(SMSG_LFG_PARTY_INFO, 1 + info.members.size() * 12);
    if (LFGPackets::BuildPartyInfo(data, info))
    {
        SendPacket(&data);
    }
}


