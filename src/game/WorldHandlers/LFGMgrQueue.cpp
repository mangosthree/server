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

#include <string>
#include <set>
#include <vector>
#include "DBCEnums.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "GameEventMgr.h"
#include "Group.h"
#include "LFGMgr.h"
#include "LFGDungeonResolution.h"
#include "Object.h"
#include "Player.h"
#include "PlayerRegistry.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"

/**
 * @file LFGMgrQueue.cpp
 * @brief Cohesion split of LFGMgr.cpp -- queue join/leave and player/party access: JoinLFG/LeaveLFG, player-or-party data, join-result computation, player status getters/setters. Same LFGMgr class; no behaviour change. CMake file(GLOB) picks this file up automatically; LFGMgr.h is unchanged.
 */

void LFGMgr::JoinLFG(uint32 roles, std::set<uint32> dungeons, std::string comments, Player* plr)
{
    // Todo:
    //       - see if any of this code/information can be put into a generalized class for other use
    //       - look into splitting this into 2 fns- one for player case, one for group
    if (!plr || !plr->GetSession())
    {
        return;
    }

    Group* pGroup = plr->GetGroup();
    ObjectGuid guid = (pGroup) ? pGroup->GetObjectGuid() : plr->GetObjectGuid();
    uint32 randomDungeonID = 0; // used later if random dungeon has been chosen

    LFGPlayers* currentInfo = GetPlayerOrPartyData(guid);

    // check if we actually have info on the player/group right now
    if (currentInfo)
    {
        bool groupCurrentlyInDungeon = pGroup && pGroup->isLFGGroup() && currentInfo->currentState != LFG_STATE_FINISHED_DUNGEON;

        // are they already queued?
        if (currentInfo->currentState == LFG_STATE_QUEUED)
        {
            // remove from that queue so they can later join this one
            queueSet::iterator qItr = m_queueSet.find(guid);
            if (qItr != m_queueSet.end())
            {
                m_queueSet.erase(qItr);
            }
            // note: do we need to send a packet telling them the current queue is over?
        }

        // are they already in a dungeon?
        if (groupCurrentlyInDungeon)
        {
            std::set<uint32> currentDungeon = currentInfo->dungeonList;

            dungeons.clear();
            dungeons.insert(*currentDungeon.begin()); // they should only have 1 dungeon in the map
        }
    }

    // used for upcoming checks
    bool isRandom  = false;

    LfgJoinResult result = GetJoinResult(plr);
    if (result == ERR_LFG_OK)
    {
        bool isRaid    = false;
        bool isDungeon = false;

        // additional checks on dungeon selection
        for (std::set<uint32>::iterator it = dungeons.begin(); it != dungeons.end(); ++it)
        {
            LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(*it);
            if (!dungeon)
            {
                continue;
            }

            switch (dungeon->typeID)
            {
                case LFG_TYPE_RANDOM_DUNGEON:
                    if (dungeons.size() > 1)
                    {
                        result = ERR_LFG_INVALID_SLOT;
                    }
                    else
                    {
                        isRandom = true;
                        randomDungeonID = *it;
                    }
                    [[fallthrough]];
                case LFG_TYPE_DUNGEON:
                case LFG_TYPE_HEROIC_DUNGEON:
                    if (isRaid)
                    {
                        result = ERR_LFG_MISMATCHED_SLOTS;
                    }
                    isDungeon = true;
                    break;
                case LFG_TYPE_RAID:
                    if (isDungeon)
                    {
                        result = ERR_LFG_MISMATCHED_SLOTS;
                    }
                    isRaid = true;
                    break;
                default: // one of the other types
                    result = ERR_LFG_INVALID_SLOT;
                    break;
            }
        }
    }

    // since our join result may have just changed, check it again
    if (result == ERR_LFG_OK && isRandom)
    {
        LfgDungeonsEntry const* randomDungeon = sLfgDungeonsStore.LookupEntry(randomDungeonID);
        if (randomDungeon)
        {
            // The client's own bucket definition: group_id plus the
            // grouping-map rows (Random Hour of Twilight has ONLY the
            // latter -- LFD_PHASE7B_SPEC.md section 1.1 C2).
            std::vector<LFGDungeonResolution::DungeonRow> dungeonRows;
            for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
            {
                if (LfgDungeonsEntry const* row = sLfgDungeonsStore.LookupEntry(id))
                {
                    LFGDungeonResolution::DungeonRow reduced;
                    reduced.id = row->ID;
                    reduced.typeID = row->typeID;
                    reduced.groupID = row->group_id;
                    dungeonRows.push_back(reduced);
                }
            }

            std::vector<LFGDungeonResolution::GroupingRow> groupingRows;
            for (uint32 id = 0; id < sLfgDungeonsGroupingMapStore.GetNumRows(); ++id)
            {
                if (LfgDungeonsGroupingMapEntry const* row = sLfgDungeonsGroupingMapStore.LookupEntry(id))
                {
                    LFGDungeonResolution::GroupingRow reduced;
                    reduced.dungeonID = row->dungeonID;
                    reduced.randomID = row->randomID;
                    groupingRows.push_back(reduced);
                }
            }

            LFGDungeonResolution::DungeonRow randomRow;
            randomRow.id = randomDungeon->ID;
            randomRow.typeID = randomDungeon->typeID;
            randomRow.groupID = randomDungeon->group_id;

            // dungeons becomes the CONCRETE candidate set; the random row
            // itself no longer rides along (old code left it in).
            LFGDungeonResolution::ExpandRandom(randomRow, dungeonRows, groupingRows, dungeons);
        }

        if (!randomDungeon || dungeons.empty())
        {
            result = ERR_LFG_INTERNAL_ERROR;
        }
    }

    // Lock pruning: strip dungeons any participant is locked out of and
    // record why, for SMSG_LFG_JOIN_RESULT's blacklist.
    std::vector<LFGPackets::JoinResultBlacklist> blacklist;
    if (result == ERR_LFG_OK)
    {
        auto pruneLocks = [&](Player* p)
        {
            std::vector<LFGLockedDungeon> locks = GetPlayerLockList(p);
            if (locks.empty())
            {
                return;
            }

            LFGPackets::JoinResultBlacklist* entry = nullptr;
            for (LFGLockedDungeon const& lock : locks)
            {
                uint32 id = lock.slot & 0x00FFFFFF;
                if (!dungeons.count(id))
                {
                    continue;
                }

                dungeons.erase(id);

                if (!entry)
                {
                    blacklist.push_back(LFGPackets::JoinResultBlacklist());
                    entry = &blacklist.back();
                    entry->guid = p->GetObjectGuid().GetRawValue();
                }

                LFGPackets::JoinResultBlacklistSlot slot;
                slot.slot = lock.slot;
                slot.reason = lock.reason;
                slot.subReason1 = lock.subReason1;
                slot.subReason2 = lock.subReason2;
                entry->slots.push_back(slot);
            }
        };

        if (pGroup)
        {
            for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                if (Player* pGroupPlr = itr->getSource())
                {
                    pruneLocks(pGroupPlr);
                }
            }
        }
        else
        {
            pruneLocks(plr);
        }

        if (!dungeons.empty())
        {
            blacklist.clear();       // dungeons remain -- TC parity, do not show the lock list
        }
        else
        {
            result = (pGroup) ? ERR_LFG_INTERNAL_ERROR : ERR_LFG_NO_SLOTS_PLAYER;
        }
    }

    // If our result is not ERR_LFG_OK, send join result now with err message
    if (result != ERR_LFG_OK)
    {
        plr->GetSession()->SendLfgJoinResult(result, 0, RideTicket(), blacklist);
        return;
    }

    RideTicket ticket;
    ticket.requesterGuid = plr->GetObjectGuid().GetRawValue();
    ticket.id = ++m_ticketId;
    ticket.type = RIDE_TYPE_LFG;
    ticket.time = int32(time(NULL));

    // dungeons is now the pruned CONCRETE set the queue matches on; the
    // client-facing selection (statuses, role check display) stays the
    // random entry the player actually clicked.
    std::set<uint32> displayDungeons = dungeons;
    if (isRandom)
    {
        displayDungeons.clear();
        displayDungeons.insert(randomDungeonID);
    }

    if (pGroup)
    {
        LFGRoleCheck roleCheck;
        roleCheck.state = LFG_ROLECHECK_INITIALITING;
        roleCheck.dungeonList = dungeons;
        roleCheck.randomDungeonID = randomDungeonID;
        roleCheck.leaderGuidRaw = pGroup->GetLeaderGuid().GetRawValue();
        roleCheck.waitForRoleTime = time_t(time(NULL) + LFG_TIME_ROLECHECK);

        // populate every member's role slot BEFORE the role check is stored,
        // otherwise PerformRoleCheck sees a lone (leader) entry and finishes
        // the check immediately (C4)
        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            if (Player* pGroupPlr = itr->getSource())
            {
                roleCheck.currentRoles[pGroupPlr->GetObjectGuid()] = 0;
            }
        }

        m_roleCheckMap[guid] = roleCheck;

        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            if (Player* pGroupPlr = itr->getSource())
            {
                ObjectGuid plrGuid = pGroupPlr->GetObjectGuid();

                LFGPlayerStatus overallStatus(LFG_STATE_ROLECHECK, LFG_UPDATE_JOIN, displayDungeons, comments);
                overallStatus.ticket = ticket;
                m_playerStatusMap[plrGuid] = overallStatus;

                SendLfgUpdate(plrGuid, overallStatus, true);
            }
        }

        // used later if they enter the queue
        LFGPlayers groupInfo(LFG_STATE_ROLECHECK, dungeons, roleCheck.currentRoles, comments, true, time(NULL), 0, 0, 0);
        m_playerData[guid] = groupInfo;

        PerformRoleCheck(plr, pGroup, uint8(roles));
    }
    else
    {
        // set up a role map and then an lfgplayer struct
        roleMap playerRole;
        playerRole[guid] = uint8(roles);

        m_playerData[guid] = LFGPlayers(LFG_STATE_QUEUED, dungeons, playerRole, comments, false, time(NULL), 0, 0, 0);

        // set up a status struct for client requests/updates
        LFGPlayerStatus plrStatus(LFG_STATE_NONE, LFG_UPDATE_JOIN_INITIAL, displayDungeons, comments);
        plrStatus.ticket = ticket;
        m_playerStatusMap[guid] = plrStatus;
        SendLfgUpdate(guid, plrStatus, false);

        plrStatus.state = LFG_STATE_QUEUED;
        plrStatus.updateType = LFG_UPDATE_ADDED_TO_QUEUE;
        m_playerStatusMap[guid] = plrStatus;
        SendLfgUpdate(guid, plrStatus, false);

        plr->GetSession()->SendLfgJoinResult(ERR_LFG_OK, 0, ticket, {});

        AddToQueue(guid);
        SendQueueStatusFor(guid);
    }
}

void LFGMgr::LeaveLFG(Player* plr, bool isGroup)
{
    if (!plr)
    {
        return;
    }

    if (isGroup)
    {
        Group* pGroup = plr->GetGroup();
        if (!pGroup)
        {
            return;
        }

        ObjectGuid grpGuid = pGroup->GetObjectGuid();

        // Leaving while the dungeon-ready popup is up counts as a decline
        // (tc-preservation LFGMgr.cpp:709-729).
        if (FailProposalForLeaver(plr->GetObjectGuid()))
        {
            return;
        }

        if (m_roleCheckMap.count(grpGuid))
        {
            PerformRoleCheck(NULL, pGroup, 0);   // aborts the check and notifies everyone
        }
        else
        {
            for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                if (Player* pGroupPlr = itr->getSource())
                {
                    ObjectGuid grpPlrGuid = pGroupPlr->GetObjectGuid();

                    LFGPlayerStatus grpPlrStatus = GetPlayerStatus(grpPlrGuid);
                    if (grpPlrStatus.state == LFG_STATE_QUEUED || grpPlrStatus.state == LFG_STATE_PROPOSAL)
                    {
                        grpPlrStatus.updateType = LFG_UPDATE_REMOVED_FROM_QUEUE;
                        grpPlrStatus.state = LFG_STATE_NONE;
                        grpPlrStatus.dungeonList.clear();
                        SendLfgUpdate(grpPlrGuid, grpPlrStatus, true);
                    }

                    m_playerData.erase(grpPlrGuid);
                    m_playerStatusMap.erase(grpPlrGuid);
                }
            }
        }

        m_queueSet.erase(grpGuid);
        m_playerData.erase(grpGuid);
    }
    else
    {
        ObjectGuid plrGuid = plr->GetObjectGuid();

        if (FailProposalForLeaver(plrGuid))
        {
            return;
        }

        LFGPlayerStatus plrStatus = GetPlayerStatus(plrGuid);
        if (plrStatus.state == LFG_STATE_QUEUED || plrStatus.state == LFG_STATE_PROPOSAL)
        {
            plrStatus.updateType = LFG_UPDATE_REMOVED_FROM_QUEUE;
            plrStatus.state = LFG_STATE_NONE;
            plrStatus.dungeonList.clear();
            SendLfgUpdate(plrGuid, plrStatus, false);
        }

        m_queueSet.erase(plrGuid);
        m_playerData.erase(plrGuid);
        m_playerStatusMap.erase(plrGuid);
    }
}

void LFGMgr::CancelQueueEntry(ObjectGuid unitGuid, LfgUpdateType reason)
{
    playerData::iterator itEntry = m_playerData.find(unitGuid);
    if (itEntry == m_playerData.end())
    {
        return;
    }

    for (roleMap::const_iterator itr = itEntry->second.currentRoles.begin();
         itr != itEntry->second.currentRoles.end(); ++itr)
    {
        ObjectGuid plrGuid = itr->first;

        Player* pPlayer = sPlayerRegistry.Find(plrGuid);
        bool isParty = pPlayer && pPlayer->GetGroup();

        LFGPlayerStatus status = GetPlayerStatus(plrGuid);
        status.updateType = reason;
        status.state = LFG_STATE_NONE;
        status.dungeonList.clear();
        SendLfgUpdate(plrGuid, status, isParty);

        m_playerStatusMap.erase(plrGuid);
    }

    m_playerData.erase(itEntry);
    m_queueSet.erase(unitGuid);
}

void LFGMgr::OnGroupMemberAdded(ObjectGuid groupGuid, ObjectGuid playerGuid)
{
    if (IsSuccessfulProposalMove(groupGuid) || IsSuccessfulProposalMove(playerGuid))
    {
        return;
    }

    // A roster change invalidates anything this group had pending.
    if (m_roleCheckMap.count(groupGuid))
    {
        if (Group* pGroup = sObjectMgr.GetGroupById(groupGuid.GetCounter()))
        {
            PerformRoleCheck(NULL, pGroup, 0);   // aborts the check and notifies everyone
        }
    }

    CancelQueueEntry(groupGuid, LFG_UPDATE_REMOVED_FROM_QUEUE);
    CancelQueueEntry(playerGuid, LFG_UPDATE_REMOVED_FROM_QUEUE);
}

void LFGMgr::OnGroupMemberRemoved(ObjectGuid groupGuid, ObjectGuid playerGuid)
{
    if (IsSuccessfulProposalMove(groupGuid) || IsSuccessfulProposalMove(playerGuid))
    {
        return;
    }

    // Mid-proposal: the leaver's whole unit declines (TC parity).
    FailProposalForLeaver(playerGuid);

    if (m_roleCheckMap.count(groupGuid))
    {
        if (Group* pGroup = sObjectMgr.GetGroupById(groupGuid.GetCounter()))
        {
            PerformRoleCheck(NULL, pGroup, 0);
        }
    }

    CancelQueueEntry(groupGuid, LFG_UPDATE_REMOVED_FROM_QUEUE);

    // In-dungeon LFD group bookkeeping.
    groupStatusMap::iterator itStatus = m_groupStatusMap.find(groupGuid);
    if (itStatus != m_groupStatusMap.end())
    {
        itStatus->second.playerRoles.erase(playerGuid);
        m_playerStatusMap.erase(playerGuid);

        if (itStatus->second.playerRoles.empty())
        {
            m_groupStatusMap.erase(itStatus);
            m_groupSet.erase(groupGuid);
        }
    }
}

void LFGMgr::OnGroupDisband(ObjectGuid groupGuid)
{
    if (IsSuccessfulProposalMove(groupGuid))
    {
        m_groupStatusMap.erase(groupGuid);
        m_groupSet.erase(groupGuid);
        return;
    }

    if (m_roleCheckMap.count(groupGuid))
    {
        // The group object is mid-teardown -- clean the check directly.
        LFGRoleCheck const& roleCheck = m_roleCheckMap.find(groupGuid)->second;
        for (roleMap::const_iterator itr = roleCheck.currentRoles.begin();
             itr != roleCheck.currentRoles.end(); ++itr)
        {
            SetPlayerState(itr->first, LFG_STATE_NONE);
            SetPlayerUpdateType(itr->first, LFG_UPDATE_GROUP_DISBAND);
            SendLfgUpdate(itr->first, GetPlayerStatus(itr->first), true);
            m_playerStatusMap.erase(itr->first);
        }
        m_roleCheckMap.erase(groupGuid);
    }

    CancelQueueEntry(groupGuid, LFG_UPDATE_GROUP_DISBAND);

    groupStatusMap::iterator itStatus = m_groupStatusMap.find(groupGuid);
    if (itStatus != m_groupStatusMap.end())
    {
        for (roleMap::const_iterator itr = itStatus->second.playerRoles.begin();
             itr != itStatus->second.playerRoles.end(); ++itr)
        {
            m_playerStatusMap.erase(itr->first);
        }

        m_groupStatusMap.erase(itStatus);
        m_groupSet.erase(groupGuid);
    }
}

void LFGMgr::OnGroupLeaderChanged(ObjectGuid groupGuid, ObjectGuid newLeaderGuid)
{
    roleCheckMap::iterator itCheck = m_roleCheckMap.find(groupGuid);
    if (itCheck != m_roleCheckMap.end())
    {
        itCheck->second.leaderGuidRaw = newLeaderGuid.GetRawValue();
    }

    groupStatusMap::iterator itStatus = m_groupStatusMap.find(groupGuid);
    if (itStatus != m_groupStatusMap.end())
    {
        itStatus->second.leaderGuid = newLeaderGuid;
    }
}

bool LFGMgr::OnPlayerLogout(Player* player)
{
    if (!player)
    {
        return false;
    }

    ObjectGuid const playerGuid = player->GetObjectGuid();

    if (Group* pGroup = player->GetGroup())
    {
        ObjectGuid const groupGuid = pGroup->GetObjectGuid();

        // An LFD group keeps its offline members (retail behavior); the
        // group-removal block in WorldSession::LogoutPlayer is skipped.
        groupStatusMap::iterator itStatus = m_groupStatusMap.find(groupGuid);
        if (itStatus != m_groupStatusMap.end() &&
            itStatus->second.playerRoles.count(playerGuid))
        {
            return true;
        }

        FailProposalForLeaver(playerGuid);

        if (m_roleCheckMap.count(groupGuid))
        {
            PerformRoleCheck(NULL, pGroup, 0);
        }

        CancelQueueEntry(groupGuid, LFG_UPDATE_GROUP_MEMBER_OFFLINE);
        return false;
    }

    FailProposalForLeaver(playerGuid);
    CancelQueueEntry(playerGuid, LFG_UPDATE_REMOVED_FROM_QUEUE);
    m_playerStatusMap.erase(playerGuid);
    return false;
}

LFGPlayers* LFGMgr::GetPlayerOrPartyData(ObjectGuid guid)
{
    playerData::iterator it = m_playerData.find(guid);
    if (it != m_playerData.end())
    {
        return &(it->second);
    }
    else
    {
        return NULL;
    }
}

LFGProposal* LFGMgr::GetProposalData(uint32 proposalID)
{
    proposalMap::iterator it = m_proposalMap.find(proposalID);
    if (it != m_proposalMap.end())
    {
        return &(it->second);
    }
    else
    {
        return NULL;
    }
}

LfgJoinResult LFGMgr::GetJoinResult(Player* plr)
{
    LfgJoinResult result = ERR_LFG_OK;
    Group* pGroup = plr->GetGroup();

    /* Reasons for not entering:
     *   Deserter spell
     *   Dungeon finder cooldown
     *   In a battleground
     *   In an arena
     *   Queued for battleground
     *   Too many members in group
     *   Group member disconnected
     *   Group member too low/high level
     *   Any group member cannot enter for x reason any other player can't
     */

    if (plr->HasAura(LFG_DESERTER_SPELL))
    {
        result = ERR_LFG_DESERTER_PLAYER;
    }
    else if (plr->InBattleGround() || plr->InBattleGroundQueue() || plr->InArena())
    {
        result = ERR_LFG_CANT_USE_DUNGEONS;
    }
    else if (plr->HasAura(LFG_COOLDOWN_SPELL))
    {
        result = ERR_LFG_RANDOM_COOLDOWN_PLAYER;
    }

    if (pGroup && result == ERR_LFG_OK)
    {
        if (pGroup->GetMembersCount() > 5)
        {
            result = ERR_LFG_TOO_MANY_MEMBERS;
        }
        else
        {
            uint8 currentMemberCount = 0;
            for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL && result == ERR_LFG_OK; itr = itr->next())
            {
                if (Player* pGroupPlr = itr->getSource())
                {
                    // check if the group members are level 15+ to use finder
                    if (pGroupPlr->getLevel() < 15)
                    {
                        result = ERR_LFG_NO_SLOTS_PLAYER;
                    }
                    else if (pGroupPlr->HasAura(LFG_DESERTER_SPELL))
                    {
                        result = ERR_LFG_DESERTER_PARTY;
                    }
                    else if (pGroupPlr->InBattleGround() || pGroupPlr->InBattleGroundQueue() || pGroupPlr->InArena())
                    {
                        result = ERR_LFG_CANT_USE_DUNGEONS;
                    }
                    else if (pGroupPlr->HasAura(LFG_COOLDOWN_SPELL))
                    {
                        result = ERR_LFG_RANDOM_COOLDOWN_PARTY;
                    }

                    ++currentMemberCount;
                }
            }

            if (result == ERR_LFG_OK && currentMemberCount != pGroup->GetMembersCount())
            {
                result = ERR_LFG_MEMBERS_NOT_PRESENT;
            }
        }
    }

    return result;
}

LFGPlayerStatus LFGMgr::GetPlayerStatus(ObjectGuid guid)
{
    LFGPlayerStatus status;

    playerStatusMap::iterator it = m_playerStatusMap.find(guid);
    if (it != m_playerStatusMap.end())
    {
        status = it->second;
    }

    return status;
}

void LFGMgr::SetPlayerComment(ObjectGuid guid, std::string comment)
{
    LFGPlayerStatus status = GetPlayerStatus(guid);
    status.comment = comment;

    m_playerStatusMap[guid] = status;
}

void LFGMgr::SetPlayerState(ObjectGuid guid, LFGState state)
{
    LFGPlayerStatus status = GetPlayerStatus(guid);
    status.state = state;

    m_playerStatusMap[guid] = status;
}

void LFGMgr::SetPlayerUpdateType(ObjectGuid guid, LfgUpdateType updateType)
{
    LFGPlayerStatus status = GetPlayerStatus(guid);
    status.updateType = updateType;

    m_playerStatusMap[guid] = status;
}
