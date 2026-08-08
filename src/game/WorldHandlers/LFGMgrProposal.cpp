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
#include "DBCEnums.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "GameEventMgr.h"
#include "Group.h"
#include "LFGMgr.h"
#include "LFGProposalLogic.h"
#include "Log.h"
#include "Object.h"
#include "Player.h"
#include "PlayerRegistry.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"

/**
 * @file LFGMgrProposal.cpp
 * @brief Cohesion split of LFGMgr.cpp -- role check, dungeon proposal and in-dungeon flow: PerformRoleCheck, proposal send/update/decline, dungeon group create, teleport, boss-kill, kick/vote and LFG packet senders. Same LFGMgr class; no behaviour change. CMake file(GLOB) picks this file up automatically; LFGMgr.h is unchanged.
 */

void LFGMgr::BuildRoleCheckPacket(LFGRoleCheck const& roleCheck, LFGPackets::RoleCheckUpdate& out)
{
    std::set<uint32> dungeonBuff;
    if (roleCheck.randomDungeonID)
    {
        dungeonBuff.insert(roleCheck.randomDungeonID);
    }
    else
    {
        dungeonBuff = roleCheck.dungeonList;
    }

    out.status = uint32(roleCheck.state);
    out.isBeginning = roleCheck.state == LFG_ROLECHECK_INITIALITING;

    for (uint32 id : dungeonBuff)
    {
        out.dungeons.push_back(GetDungeonEntry(id));
    }

    ObjectGuid leaderGuid(roleCheck.leaderGuidRaw);
    roleMap::const_iterator leaderIt = roleCheck.currentRoles.find(leaderGuid);
    if (leaderIt != roleCheck.currentRoles.end())
    {
        LFGPackets::LFGRoleCheckMember member;
        member.guid = leaderIt->first.GetRawValue();
        member.roleCheckComplete = leaderIt->second > 0;
        member.rolesDesired = leaderIt->second;
        Player* pLeader = sPlayerRegistry.Find(leaderIt->first);
        member.level = uint8(pLeader ? pLeader->getLevel() : 0);
        out.members.push_back(member);
    }

    for (roleMap::const_iterator rItr = roleCheck.currentRoles.begin(); rItr != roleCheck.currentRoles.end(); ++rItr)
    {
        if (rItr->first == leaderGuid)
        {
            continue;   // already added first, above
        }

        LFGPackets::LFGRoleCheckMember member;
        member.guid = rItr->first.GetRawValue();
        member.roleCheckComplete = rItr->second > 0;
        member.rolesDesired = rItr->second;
        Player* pMember = sPlayerRegistry.Find(rItr->first);
        member.level = uint8(pMember ? pMember->getLevel() : 0);   // per-member level -- not the leader's (V10)
        out.members.push_back(member);
    }
}

// called each time a player selects their role
void LFGMgr::PerformRoleCheck(Player* pPlayer, Group* pGroup, uint8 roles)
{
    if (!pGroup)
    {
        return;
    }

    ObjectGuid groupGuid = pGroup->GetObjectGuid();

    roleCheckMap::iterator it = m_roleCheckMap.find(groupGuid);
    if (it == m_roleCheckMap.end())
    {
        return; // no role check map found
    }

    LFGRoleCheck& roleCheck = it->second;   // reference: mutations must stick (C3)
    ObjectGuid plrGuid = pPlayer ? pPlayer->GetObjectGuid() : ObjectGuid();
    bool roleChosen = roleCheck.state != LFG_ROLECHECK_DEFAULT && !plrGuid.IsEmpty();

    if (!plrGuid)
    {
        roleCheck.state = LFG_ROLECHECK_ABORTED;  // aborted if anyone cancels during role check
    }
    else if (roles < PLAYER_ROLE_TANK)            // kind of a sanity check- the client shouldn't allow this to happen
    {
        roleCheck.state = LFG_ROLECHECK_NO_ROLE;
    }
    else
    {
        roleCheck.currentRoles[plrGuid] = roles;

        bool allRolesChosen = true;
        for (roleMap::iterator rItr = roleCheck.currentRoles.begin(); rItr != roleCheck.currentRoles.end(); ++rItr)
        {
            if (rItr->second == PLAYER_ROLE_NONE)
            {
                allRolesChosen = false;
                break;
            }
        }

        if (allRolesChosen) // meaning that everyone confirmed their roles
        {
            roleCheck.state = ValidateGroupRoles(roleCheck.currentRoles) ? LFG_ROLECHECK_FINISHED : LFG_ROLECHECK_WRONG_ROLES;
        }
    }

    // Build ONE role-check packet for everyone in this check.
    LFGPackets::RoleCheckUpdate update;
    BuildRoleCheckPacket(roleCheck, update);

    for (roleMap::iterator itr = roleCheck.currentRoles.begin(); itr != roleCheck.currentRoles.end(); ++itr)
    {
        ObjectGuid guidBuff = itr->first;
        if (roleChosen)
        {
            SendRoleChosen(guidBuff, plrGuid, roles); // send SMSG_LFG_ROLE_CHOSEN to each player
        }

        // send SMSG_LFG_ROLE_CHECK_UPDATE
        SendRoleCheckUpdate(guidBuff, update);

        switch (roleCheck.state)
        {
            case LFG_ROLECHECK_INITIALITING:
                continue;
            case LFG_ROLECHECK_FINISHED:
                // set current plr's state to queued. then set their role in that struct
                // then send lfgupdate packet with UPDATETYPE_ADDED_TO_QUEUE
                SetPlayerState(guidBuff, LFG_STATE_QUEUED);
                SetPlayerUpdateType(guidBuff, LFG_UPDATE_ADDED_TO_QUEUE);
                SendLfgUpdate(guidBuff, GetPlayerStatus(guidBuff), true);
                break;
            default:
                if (roleCheck.leaderGuidRaw == guidBuff.GetRawValue())
                {
                    SendLfgJoinResult(guidBuff, ERR_LFG_ROLE_CHECK_FAILED, uint8(roleCheck.state),
                                      GetPlayerStatus(guidBuff).ticket, {});
                }
                SetPlayerState(guidBuff, LFG_STATE_NONE);
                SetPlayerUpdateType(guidBuff, LFG_UPDATE_ROLECHECK_FAILED);
                SendLfgUpdate(guidBuff, GetPlayerStatus(guidBuff), true);
                break;
        }
    }

    if (roleCheck.state == LFG_ROLECHECK_FINISHED)
    {
        LFGPlayers* queueInfo = GetPlayerOrPartyData(groupGuid);
        if (queueInfo)
        {
            queueInfo->currentState = LFG_STATE_QUEUED;
            queueInfo->currentRoles = roleCheck.currentRoles;
            queueInfo->joinedTime   = time(NULL);
        }

        m_roleCheckMap.erase(it);   // C3 fix: erase on the finished path too

        AddToQueue(groupGuid);
        SendQueueStatusFor(groupGuid);
    }
    else if (roleCheck.state != LFG_ROLECHECK_INITIALITING)
    {
        m_playerData.erase(groupGuid);
        m_roleCheckMap.erase(it);
    }
}

bool LFGMgr::ValidateGroupRoles(roleMap groupMap)
{
    if (groupMap.empty()) // sanity check
    {
        return false;
    }

    uint8 tankCount = 0, dpsCount = 0, healCount = 0;
    CountAssignedRoles(groupMap, tankCount, healCount, dpsCount);

    // Everyone must have been seated; anyone left over selected no role, or
    // only roles the group has already filled.
    return (tankCount + dpsCount + healCount == groupMap.size()) ? true : false;
}

//todo(7b): offer-continue proposals (isNew = false, encounters, groupRawGuid)
void LFGMgr::SendDungeonProposal(ObjectGuid queueGuid, LFGPlayers* lfgGroup)
{
    if (!lfgGroup || lfgGroup->dungeonList.empty() || lfgGroup->currentRoles.empty())
    {
        return;
    }

    ++m_proposalId;

    LFGProposal proposal;
    proposal.id = m_proposalId;
    proposal.dungeonID = *lfgGroup->dungeonList.begin();
    proposal.joinedQueue = lfgGroup->joinedTime;
    proposal.cancelTime = time(NULL) + LFG_TIME_PROPOSAL;
    proposal.queueEntryGuid = queueGuid;
    proposal.currentRoles = lfgGroup->currentRoles;

    for (roleMap::const_iterator itr = proposal.currentRoles.begin();
         itr != proposal.currentRoles.end(); ++itr)
    {
        ObjectGuid plrGuid = itr->first;
        proposal.answers[plrGuid] = LFG_ANSWER_PENDING;

        Player* pPlayer = sPlayerRegistry.Find(plrGuid);
        Group* pGroup = pPlayer ? pPlayer->GetGroup() : NULL;
        proposal.groups[plrGuid] = pGroup ? pGroup->GetObjectGuid() : ObjectGuid();
    }

    // Freeze the queue entry: no more matching, no more queue status.
    lfgGroup->currentState = LFG_STATE_PROPOSAL;
    m_queueSet.erase(queueGuid);

    m_proposalMap[proposal.id] = proposal;
    LFGProposal const& stored = m_proposalMap[proposal.id];

    for (roleMap::const_iterator itr = stored.currentRoles.begin();
         itr != stored.currentRoles.end(); ++itr)
    {
        ObjectGuid plrGuid = itr->first;
        bool isParty = !stored.groups.find(plrGuid)->second.IsEmpty();

        SetPlayerState(plrGuid, LFG_STATE_PROPOSAL);
        SetPlayerUpdateType(plrGuid, LFG_UPDATE_PROPOSAL_BEGIN);
        SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), isParty);
        SendProposalUpdateToPlayer(plrGuid, stored);
    }

    DEBUG_FILTER_LOG(LOG_FILTER_LFG,
        "LFGMgr: proposal %u sent for dungeon %u to %u players",
        stored.id, stored.dungeonID, uint32(stored.currentRoles.size()));
}

void LFGMgr::SendProposalUpdateToPlayer(ObjectGuid plrGuid, LFGProposal const& proposal)
{
    Player* pPlayer = sPlayerRegistry.Find(plrGuid);
    if (!pPlayer)
    {
        return;
    }

    LFGPlayerStatus status = GetPlayerStatus(plrGuid);

    // Show the recipient the dungeon they actually picked (their random
    // entry) when the concrete dungeon is not in their selection
    // (tc-preservation LFGHandler.cpp:594-599).
    uint32 dungeonId = proposal.dungeonID;
    if (!status.dungeonList.empty() &&
        status.dungeonList.find(dungeonId) == status.dungeonList.end())
    {
        dungeonId = *status.dungeonList.begin();
    }

    playerGroupMap::const_iterator selfItr = proposal.groups.find(plrGuid);
    ObjectGuid myGroup = (selfItr != proposal.groups.end())
        ? selfItr->second : ObjectGuid();

    LFGPackets::ProposalUpdate data;
    data.ticket = status.ticket;
    data.instanceId = 0;                        // TC parity; echoed back, unused
    data.proposalId = proposal.id;
    data.slot = GetDungeonEntry(dungeonId);
    data.state = int8(proposal.state);
    data.completedMask = proposal.encounters;
    data.proposalSilent = false;                // every 7a proposal is new (C4)

    for (roleMap::const_iterator itr = proposal.currentRoles.begin();
         itr != proposal.currentRoles.end(); ++itr)
    {
        ObjectGuid memberGuid = itr->first;

        playerGroupMap::const_iterator grpItr = proposal.groups.find(memberGuid);
        ObjectGuid memberGroup = (grpItr != proposal.groups.end())
            ? grpItr->second : ObjectGuid();

        proposalAnswerMap::const_iterator ansItr = proposal.answers.find(memberGuid);
        LFGProposalAnswer answer = (ansItr != proposal.answers.end())
            ? ansItr->second : LFG_ANSWER_PENDING;

        LFGPackets::ProposalUpdatePlayer member;
        member.roles = itr->second;             // leader bit kept (client isLeader)
        member.me = memberGuid == plrGuid;
        member.sameParty = !memberGroup.IsEmpty() && memberGroup == myGroup;
        // Wire bit 2 gates the client's "someone in your party did not
        // accept" message (spec C5); TC's proposal-group fill starves it.
        member.myParty = member.sameParty;
        member.responded = answer != LFG_ANSWER_PENDING;
        member.accepted = answer == LFG_ANSWER_AGREE;
        data.players.push_back(member);
    }

    pPlayer->GetSession()->SendLfgProposalUpdate(data);
}

bool LFGMgr::IsProposalSameGroup(LFGProposal const& proposal)
{
    bool firstLoop = true;
    bool isSameGroup = true;

    ObjectGuid priorGroupGuid;

    // when this is called we don't have the groups part filled, so iterate via role map
    for (roleMap::const_iterator it = proposal.currentRoles.begin(); it != proposal.currentRoles.end(); ++it)
    {
        ObjectGuid plrGuid = it->first;

        Player* pPlayer = sPlayerRegistry.Find(plrGuid);
        if (Group* pGroup = pPlayer->GetGroup())
        {
            ObjectGuid grpGuid = pGroup->GetObjectGuid();

            if (firstLoop)
            {
                priorGroupGuid = grpGuid;
                firstLoop = false;
            }
            else
            {
                if (isSameGroup)
                {
                    if (grpGuid != priorGroupGuid)
                    {
                        isSameGroup = false;
                    }
                }
            }
        }
    }
    return isSameGroup;
}

// From CMSG_LFG_PROPOSAL_RESULT
void LFGMgr::ProposalUpdate(uint32 proposalID, ObjectGuid plrGuid, bool accepted)
{
    proposalMap::iterator itProposal = m_proposalMap.find(proposalID);
    if (itProposal == m_proposalMap.end())
    {
        return;
    }

    LFGProposal& proposal = itProposal->second;

    proposalAnswerMap::iterator itAnswer = proposal.answers.find(plrGuid);
    if (itAnswer == proposal.answers.end())
    {
        return;                     // not a member of this proposal
    }

    itAnswer->second = accepted ? LFG_ANSWER_AGREE : LFG_ANSWER_DENY;

    DEBUG_FILTER_LOG(LOG_FILTER_LFG, "LFGMgr: proposal %u answer %u from %s",
        proposalID, uint32(accepted), plrGuid.GetString().c_str());

    if (!accepted)
    {
        RemoveProposal(itProposal, LFG_UPDATE_PROPOSAL_DECLINED);
        return;
    }

    bool allAgreed = true;
    for (proposalAnswerMap::const_iterator itr = proposal.answers.begin();
         itr != proposal.answers.end(); ++itr)
    {
        if (itr->second != LFG_ANSWER_AGREE)
        {
            allAgreed = false;
            break;
        }
    }

    if (!allAgreed)
    {
        // Everyone's popup shows the new answer.
        for (roleMap::const_iterator itr = proposal.currentRoles.begin();
             itr != proposal.currentRoles.end(); ++itr)
        {
            SendProposalUpdateToPlayer(itr->first, proposal);
        }

        return;
    }

    // Success: state 2, TC packet sequence, then wipe -- 7b replaces the
    // wipe with group formation + teleport (CreateDungeonGroup).
    bool sendUpdate = proposal.state != LFG_PROPOSAL_SUCCESS;
    proposal.state = LFG_PROPOSAL_SUCCESS;
    time_t now = time(NULL);

    for (roleMap::const_iterator itr = proposal.currentRoles.begin();
         itr != proposal.currentRoles.end(); ++itr)
    {
        ObjectGuid memberGuid = itr->first;
        uint8 role = itr->second;
        role &= ~PLAYER_ROLE_LEADER;

        if (sendUpdate)
        {
            SendProposalUpdateToPlayer(memberGuid, proposal);
        }

        UpdateWaitMap(LFGRoles(role), proposal.dungeonID,
                      time_t(now - proposal.joinedQueue));

        bool isParty = !proposal.groups.find(memberGuid)->second.IsEmpty();
        LFGPlayerStatus status = GetPlayerStatus(memberGuid);
        status.updateType = LFG_UPDATE_GROUP_FOUND;
        SendLfgUpdate(memberGuid, status, isParty);
    }

    sLog.outString("LFGMgr: proposal %u succeeded for dungeon %u -- "
                   "group formation lands in Phase 7b", proposal.id,
                   proposal.dungeonID);

    for (roleMap::const_iterator itr = proposal.currentRoles.begin();
         itr != proposal.currentRoles.end(); ++itr)
    {
        m_playerStatusMap.erase(itr->first);
    }

    m_playerData.erase(proposal.queueEntryGuid);
    m_proposalMap.erase(itProposal);
}

bool LFGMgr::HasLeaderFlag(roleMap const& roles)
{
    for (roleMap::const_iterator it = roles.begin(); it != roles.end(); ++it)
    {
        if (it->second & PLAYER_ROLE_LEADER)
        {
            return true;
        }
    }
    return false;
}

void LFGMgr::CreateDungeonGroup(LFGProposal* proposal)
{
    if (!proposal)
    {
        return;
    }

    Group* pGroup = nullptr;

    if (!proposal->groupRawGuid)
    {
        bool leaderIsSet = false;
        bool leaderRoleIsSet = HasLeaderFlag(proposal->currentRoles);
        ObjectGuid leaderGuid;

        pGroup = new Group();

        for (playerGroupMap::iterator it = proposal->groups.begin(); it != proposal->groups.end(); ++it)
        {
            // remove plr from group w/ guid it->second
            // set leader on first loop, then set leaderisset to true
            ObjectGuid pGroupPlrGuid = it->first;
            Player* pGroupPlr = sPlayerRegistry.Find(pGroupPlrGuid);

            if (pGroupPlr && it->second)
            {
                Group* existingGroup = pGroupPlr->GetGroup();
                if (existingGroup)
                {
                    existingGroup->RemoveMember(pGroupPlrGuid, 0);
                }
            }

            if (pGroupPlr && !leaderIsSet)
            {
                bool currentPlrIsLeader = false;
                if (leaderRoleIsSet)
                {
                    for (roleMap::iterator itr = proposal->currentRoles.begin(); itr != proposal->currentRoles.end(); ++itr)
                    {
                        if (itr->second & PLAYER_ROLE_LEADER)
                        {
                            leaderGuid = itr->first;
                            Player* leaderRef = sPlayerRegistry.Find(leaderGuid);

                            if (leaderRef)
                            {
                                pGroup->Create(leaderRef->GetObjectGuid(), leaderRef->GetName());
                                currentPlrIsLeader = (pGroupPlrGuid == leaderGuid);
                            }
                        }
                    }
                }
                else
                {
                    pGroup->Create(pGroupPlrGuid, pGroupPlr->GetName());
                }

                if (!currentPlrIsLeader)
                {
                    pGroup->AddMember(pGroupPlrGuid, pGroupPlr->GetName());
                }

                leaderIsSet = true;
            }
            else if (leaderIsSet && pGroupPlr && pGroupPlrGuid != leaderGuid)
            {
                pGroup->AddMember(pGroupPlrGuid, pGroupPlr->GetName());
            }
        }
        pGroup->SetAsLfgGroup();
    }
    else
    {
        Player* pGroupLeader = sPlayerRegistry.Find(ObjectGuid(proposal->groupLeaderGuid));

        // Check if the group leader was found before accessing their group
        if (pGroupLeader)
        {
            pGroup = pGroupLeader->GetGroup();
        }
        else
        {
            // Log that the group leader is missing and fall back to creating a new group
            // In the future, we should determine the right actions for this scenario.
            // LOG_ERROR("LFGMgr::CreateDungeonGroup", "Group leader with GUID %u not found. Creating new group.", proposal->groupLeaderGuid);

            // Attempt to create a new group using the first available player in the proposal group
            if (!proposal->groups.empty())
            {
                ObjectGuid fallbackLeaderGuid = proposal->groups.begin()->first;
                Player* fallbackLeader = sPlayerRegistry.Find(fallbackLeaderGuid);

                if (fallbackLeader)
                {
                    pGroup = new Group();
                    pGroup->Create(fallbackLeader->GetObjectGuid(), fallbackLeader->GetName());
                    pGroup->SetAsLfgGroup();

                    // Add remaining members to the new group
                    for (playerGroupMap::iterator it = proposal->groups.begin(); it != proposal->groups.end(); ++it)
                    {
                        ObjectGuid pGroupPlrGuid = it->first;
                        if (pGroupPlrGuid != fallbackLeaderGuid)
                        {
                            Player* pGroupPlr = sPlayerRegistry.Find(pGroupPlrGuid);
                            if (pGroupPlr)
                            {
                                pGroup->AddMember(pGroupPlrGuid, pGroupPlr->GetName());
                            }
                        }
                    }
                }
                else
                {
                    // If no valid players are found, we return without proceeding
                    // In the future, we should determine the right actions for this scenario.
                    // LOG_ERROR("LFGMgr::CreateDungeonGroup", "No valid players found to create a fallback group.");
                    return;
                }
            }
            else
            {
                // Log if there are no players in the proposal groups map
                // In the future, we should determine the right actions for this scenario.
                // LOG_ERROR("LFGMgr::CreateDungeonGroup", "Proposal groups map is empty, cannot create fallback group.");
                return;
            }
        }
    }

    // Set dungeon difficulty for group
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(proposal->dungeonID);
    if (!dungeon || !pGroup)
    {
        return;
    }

    pGroup->SetDungeonDifficulty(Difficulty(dungeon->difficulty));

    // Add group to our group set and group map, then teleport to the dungeon
    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus groupStatus(LFG_STATE_IN_DUNGEON, dungeon->ID, proposal->currentRoles, pGroup->GetLeaderGuid());

    m_groupSet.insert(groupGuid);
    m_groupStatusMap[groupGuid] = groupStatus;
    TeleportToDungeon(dungeon->ID, pGroup);

    pGroup->SendUpdate();
}

void LFGMgr::TeleportToDungeon(uint32 dungeonID, Group* pGroup)
{
    // if the group's leader is already in the dungeon, teleport anyone not in dungeon to them
    // if nobody is in the dungeon, teleport all to beginning of dungeon (sObjectMgr.GetMapEntranceTrigger(mapid [not dungeonid]))
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonID);
    if (!dungeon || !pGroup)
    {
        return;
    }

    uint32 mapID = (uint32)dungeon->mapID;
    float x, y, z, o;
    LFGTeleportError err = LFG_TELEPORTERROR_OK;

    Player* pGroupLeader = sPlayerRegistry.Find(pGroup->GetLeaderGuid());

    if (pGroupLeader && pGroupLeader->GetMapId() == mapID) // Already in the dungeon
    {
        // set teleport location to that of the group leader
        x = pGroupLeader->Where().X();
        y = pGroupLeader->Where().Y();
        z = pGroupLeader->Where().Z();
        o = pGroupLeader->Where().Facing();
    }
    else
    {
        if (AreaTrigger const* at = sObjectMgr.GetMapEntranceTrigger(mapID))
        {
            x = at->target_X;
            y = at->target_Y;
            z = at->target_Z;
            o = at->target_Orientation;
        }
        else
        {
            err = LFG_TELEPORTERROR_INVALID_LOCATION;
        }
    }

    dungeonForbidden lockedDungeons;
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            // further checks: player is dead, in vehicle, in battleground, on taxi, etc
            LFGTeleportError plrErr = LFG_TELEPORTERROR_OK;

            if (pGroupPlr->IsDead())
            {
                plrErr = LFG_TELEPORTERROR_PLAYER_DEAD;
            }
            if (pGroupPlr->IsFalling())
            {
                plrErr = LFG_TELEPORTERROR_FALLING;
            }
            if (pGroupPlr->GetVehicleInfo())
            {
                plrErr = LFG_TELEPORTERROR_IN_VEHICLE;
            }

            lockedDungeons = FindRandomDungeonsNotForPlayer(pGroupPlr);
            if (lockedDungeons.find(dungeon->Entry()) != lockedDungeons.end())
            {
                plrErr = LFG_TELEPORTERROR_INVALID_LOCATION;
            }

            if (err == LFG_TELEPORTERROR_OK && plrErr == LFG_TELEPORTERROR_OK && pGroupPlr->GetMapId() != mapID)
            {
                if (pGroupPlr->GetMap() && !pGroupPlr->GetMap()->IsDungeon() && !pGroupPlr->GetMap()->IsRaid() && !pGroupPlr->InBattleGround())
                {
                    pGroupPlr->SetBattleGroundEntryPoint(); // store current position and such
                }

                if (!pGroupPlr->TeleportTo(mapID, x, y, z, o))
                {
                    plrErr = LFG_TELEPORTERROR_INVALID_LOCATION;
                }
            }

            if (err != LFG_TELEPORTERROR_OK)
            {
                pGroupPlr->GetSession()->SendLfgTeleportError(err);
            }
            else if (plrErr != LFG_TELEPORTERROR_OK)
            {
                pGroupPlr->GetSession()->SendLfgTeleportError(plrErr);
            }
            else
            {
                SetPlayerState(pGroupPlr->GetObjectGuid(), LFG_STATE_IN_DUNGEON);
            }
        }
    }
}

void LFGMgr::TeleportPlayer(Player* pPlayer, bool out)
{
    // Fetch necessary data first
    Group* pGroup = pPlayer->GetGroup();
    if (!pGroup)
    {
        pPlayer->GetSession()->SendLfgTeleportError((uint8)LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }

    LFGGroupStatus* status = GetGroupStatus(pGroup->GetObjectGuid());
    if (!status)
    {
        pPlayer->GetSession()->SendLfgTeleportError((uint8)LFG_TELEPORTERROR_INVALID_LOCATION);
        return;
    }

    // Get dungeon info and then teleport the player out if applicable
    if (out)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(status->dungeonID);
        if (dungeon && pPlayer->GetMapId() == dungeon->mapID)
        {
            pPlayer->TeleportToBGEntryPoint();
        }
    }
}

LFGGroupStatus* LFGMgr::GetGroupStatus(ObjectGuid guid)
{
    groupStatusMap::iterator it = m_groupStatusMap.find(guid);
    if (it != m_groupStatusMap.end())
    {
        return &(it->second);
    }
    else
    {
        return NULL;
    }
}

proposalMap::iterator LFGMgr::RemoveProposal(proposalMap::iterator itProposal, LfgUpdateType type)
{
    LFGProposal& proposal = itProposal->second;
    proposal.state = LFG_PROPOSAL_FAILED;

    DEBUG_FILTER_LOG(LOG_FILTER_LFG, "LFGMgr: proposal %u failed, update type %u",
        proposal.id, uint32(type));

    // 1. Classify: decliners' whole units leave, everyone else re-queues.
    std::vector<LFGProposalLogic::Member> members;
    std::vector<ObjectGuid> memberGuids;
    members.reserve(proposal.currentRoles.size());
    memberGuids.reserve(proposal.currentRoles.size());

    for (roleMap::const_iterator itr = proposal.currentRoles.begin();
         itr != proposal.currentRoles.end(); ++itr)
    {
        ObjectGuid memberGuid = itr->first;
        ObjectGuid unitGuid = proposal.groups.find(memberGuid)->second;

        LFGProposalLogic::Member member;
        member.guid = memberGuid.GetRawValue();
        member.unitGuid = unitGuid.IsEmpty() ? member.guid : unitGuid.GetRawValue();
        member.answer = int(proposal.answers.find(memberGuid)->second);
        members.push_back(member);
        memberGuids.push_back(memberGuid);
    }

    std::vector<LFGProposalLogic::Disposition> dispositions;
    LFGProposalLogic::ResolveFailure(members, type == LFG_UPDATE_PROPOSAL_FAILED,
                                     dispositions);

    // Timeout flipped PENDING to DENY; reflect that in the packet's
    // responded/accepted bits.
    for (size_t i = 0; i < members.size(); ++i)
    {
        proposal.answers[memberGuids[i]] = LFGProposalAnswer(members[i].answer);
    }

    // 2. Notify. Proposal packet first, then the removed members' status
    //    (tc-preservation LFGMgr.cpp:1224-1258; survivors follow in step 3).
    for (size_t i = 0; i < members.size(); ++i)
    {
        ObjectGuid memberGuid = memberGuids[i];
        bool isParty = !proposal.groups.find(memberGuid)->second.IsEmpty();

        SendProposalUpdateToPlayer(memberGuid, proposal);

        if (dispositions[i] != LFGProposalLogic::DISPOSITION_REMOVE)
        {
            continue;
        }

        LFGPlayerStatus status = GetPlayerStatus(memberGuid);
        status.updateType = (members[i].answer == LFGProposalLogic::ANSWER_DENY)
            ? type : LFG_UPDATE_REMOVED_FROM_QUEUE;
        status.state = LFG_STATE_NONE;
        status.dungeonList.clear();
        SendLfgUpdate(memberGuid, status, isParty);

        m_playerStatusMap.erase(memberGuid);
    }

    // 3. Re-queue every survivor as one entry (m3 merged the originals
    //    away -- spec C11).
    uint64 newKeyRaw = LFGProposalLogic::PickRequeueKey(
        members, dispositions, proposal.queueEntryGuid.GetRawValue());

    playerData::iterator itEntry = m_playerData.find(proposal.queueEntryGuid);
    if (newKeyRaw == 0)
    {
        if (itEntry != m_playerData.end())
        {
            m_playerData.erase(itEntry);
        }
    }
    else
    {
        LFGPlayers entry;
        if (itEntry != m_playerData.end())
        {
            entry = itEntry->second;
            m_playerData.erase(itEntry);
        }

        entry.currentState = LFG_STATE_QUEUED;
        entry.currentRoles.clear();
        for (size_t i = 0; i < members.size(); ++i)
        {
            if (dispositions[i] == LFGProposalLogic::DISPOSITION_REQUEUE)
            {
                entry.currentRoles[memberGuids[i]] =
                    proposal.currentRoles.find(memberGuids[i])->second;
            }
        }

        ObjectGuid newKey = ObjectGuid(newKeyRaw);
        m_playerData[newKey] = entry;

        for (size_t i = 0; i < members.size(); ++i)
        {
            if (dispositions[i] != LFGProposalLogic::DISPOSITION_REQUEUE)
            {
                continue;
            }

            ObjectGuid memberGuid = memberGuids[i];
            bool isParty = !proposal.groups.find(memberGuid)->second.IsEmpty();

            SetPlayerState(memberGuid, LFG_STATE_QUEUED);
            SetPlayerUpdateType(memberGuid, LFG_UPDATE_ADDED_TO_QUEUE);
            SendLfgUpdate(memberGuid, GetPlayerStatus(memberGuid), isParty);
        }

        AddToQueue(newKey);
        SendQueueStatusFor(newKey);
    }

    return m_proposalMap.erase(itProposal);
}

bool LFGMgr::FailProposalForLeaver(ObjectGuid plrGuid)
{
    for (proposalMap::iterator itr = m_proposalMap.begin(); itr != m_proposalMap.end(); ++itr)
    {
        proposalAnswerMap::iterator itAnswer = itr->second.answers.find(plrGuid);
        if (itAnswer == itr->second.answers.end())
        {
            continue;
        }

        itAnswer->second = LFG_ANSWER_DENY;
        RemoveProposal(itr, LFG_UPDATE_PROPOSAL_DECLINED);
        return true;
    }

    return false;
}

void LFGMgr::UpdateWaitMap(LFGRoles role, uint32 dungeonID, time_t waitTime)
{
    if (!role || !dungeonID || !waitTime)
    {
        return;
    }

    switch (role)
    {
        case PLAYER_ROLE_TANK:
        {
            waitTimeMap::iterator it = m_tankWaitTime.find(dungeonID);
            if (it != m_tankWaitTime.end())
            {
                LFGWait wait = it->second;

                wait.previousTime = wait.time;
                wait.time = waitTime;
                wait.doAverage = true;

                m_tankWaitTime[dungeonID] = wait;
            }
        }
            break;
        case PLAYER_ROLE_HEALER:
        {
            waitTimeMap::iterator hIt = m_healerWaitTime.find(dungeonID);
            if (hIt != m_healerWaitTime.end())
            {
                LFGWait wait = hIt->second;

                wait.previousTime = wait.time;
                wait.time = waitTime;
                wait.doAverage = true;

                m_healerWaitTime[dungeonID] = wait;
            }
        }
            break;
        case PLAYER_ROLE_DAMAGE:
        {
            waitTimeMap::iterator dIt = m_dpsWaitTime.find(dungeonID);
            if (dIt != m_dpsWaitTime.end())
            {
                LFGWait wait = dIt->second;

                wait.previousTime = wait.time;
                wait.time = waitTime;
                wait.doAverage = true;

                m_dpsWaitTime[dungeonID] = wait;
            }
        }
            break;
        default:
        {
            waitTimeMap::iterator aIt = m_avgWaitTime.find(dungeonID);
            if (aIt != m_avgWaitTime.end())
            {
                LFGWait wait = aIt->second;

                wait.previousTime = wait.time;
                wait.time = waitTime;
                wait.doAverage = true;

                m_avgWaitTime[dungeonID] = wait;
            }
        }
            break;
    }

}

void LFGMgr::HandleBossKilled(Player* pPlayer)
{
    Group* pGroup = pPlayer->GetGroup();
    if (!pGroup)
    {
        return;
    }

    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    if (!status)
    {
        return;
    }

    // set each player's lfgstate to LFG_STATE_FINISHED_DUNGEON
    // fetch reward info, and if it's the first dungeon of the day (per player),
    //    give them 2x the xp (or 1x if it's not the first), and the reward item
    //    (special case for 2nd wotlk heroic and +). If no room in inventory, send
    //    via ingame mail.
    status->state = LFG_STATE_FINISHED_DUNGEON;

    DungeonTypes type = GetDungeonType(status->dungeonID);
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next()) //todo: check if we will need to use mail or not
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            SetPlayerState(pGroupPlr->GetObjectGuid(), LFG_STATE_FINISHED_DUNGEON);

            // check if player did a random dungeon
            uint32 randomDungeonId = 0;
            LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(status->dungeonID);
            if (dungeon->typeID == LFG_TYPE_RANDOM_DUNGEON || IsSeasonal(dungeon->flags))
            {
                randomDungeonId = dungeon->ID;
            }

            // get rewards
            uint32 groupPlrLevel = pGroupPlr->getLevel();
            const DungeonFinderRewards* rewards = sObjectMgr.GetDungeonFinderRewards(groupPlrLevel); // Fetch base xp/money reward
            ItemRewards itemRewards = GetDungeonItemRewards(status->dungeonID, type);                // fetch item reward

            int32 multiplier;                                                                        // base reward modifier
            bool hasDoneDaily = HasPlayerDoneDaily(pGroupPlr->GetGUIDLow(), type);                                 // first dungeon of the day?
            (hasDoneDaily) ? multiplier = 1 : multiplier = 2;

            uint32 xpReward = multiplier*rewards->baseXPReward;                                      // player's xp reward
            uint32 moneyReward = uint32(multiplier*rewards->baseMonetaryReward);                              // player's money reward

            uint32 itemReward = 0;                                                                   // reward item
            uint32 itemAmount = 0;                                                                   // amount of item
            if (hasDoneDaily && (type == DUNGEON_WOTLK_HEROIC))
            {
                itemReward = WOTLK_SPECIAL_HEROIC_ITEM;
                itemAmount = WOTLK_SPECIAL_HEROIC_AMNT;
            }
            else if (!hasDoneDaily)
            {
                itemReward = itemRewards.itemId;
                itemAmount = itemRewards.itemAmount;
            }

            // and then fill a structure corresponding to SMSG_LFG_PLAYER_REWARD and
            // send one of these to each player
            LFGRewards reward(randomDungeonId, status->dungeonID, hasDoneDaily, moneyReward, xpReward, itemReward, itemAmount);
            pGroupPlr->GetSession()->SendLfgRewards(reward);
        }
    }

    // now we can remove the group from our maps
    m_groupStatusMap.erase(groupGuid);
    m_groupSet.erase(groupGuid);
}

void LFGMgr::AttemptToKickPlayer(Group* pGroup, ObjectGuid guid, ObjectGuid kicker, std::string reason)
{
    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus* status = GetGroupStatus(groupGuid);

    bootStatusMap::iterator bIt = m_bootStatusMap.find(groupGuid);
    if (!status)
    {
        return;
    }

    status->state = LFG_STATE_BOOT;
    m_groupStatusMap[groupGuid] = *status;

    // This function is only called when a group is set/in a dungeon so we can go straight to the boot packets
    time_t now = time(NULL);
    proposalAnswerMap votes;

    // safe to say the person attempting to kick them will vote yes, the kick-ee will vote no
    votes[guid] = LFG_ANSWER_DENY;
    votes[kicker] = LFG_ANSWER_AGREE;

    // set group state to boot vote, same for player states until it's over
    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next()) //todo: check if we will need to use mail or not
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            ObjectGuid pGroupPlrGuid = pGroupPlr->GetObjectGuid();

            SetPlayerState(pGroupPlrGuid, LFG_STATE_BOOT);

            if ( (pGroupPlrGuid != guid) && (pGroupPlrGuid != kicker) )
            {
                votes[pGroupPlrGuid] = LFG_ANSWER_PENDING;
            }
        }
    }

    LFGBoot boot(true, guid, reason, votes, now);
    m_bootStatusMap[groupGuid] = boot;

    for (GroupReference* it = pGroup->GetFirstMember(); it != NULL; it = it->next())
    {
        if (Player* groupPlr = it->getSource())
        {
            groupPlr->GetSession()->SendLfgBootUpdate(boot);
        }
    }
}

void LFGMgr::CastVote(Player* pPlayer, bool vote)
{
    if (!pPlayer)
    {
        return;
    }

    Group* pGroup = pPlayer->GetGroup();
    ObjectGuid groupGuid = pGroup->GetObjectGuid();

    LFGGroupStatus* status = GetGroupStatus(groupGuid);

    if (!status || status->state != LFG_STATE_BOOT)
    {
        return;
    }

    bootStatusMap::iterator it = m_bootStatusMap.find(groupGuid);
    if (it == m_bootStatusMap.end())
    {
        return;
    }

    LFGBoot boot = it->second;
    boot.answers[pPlayer->GetObjectGuid()] = LFGProposalAnswer(vote);

    int32 yay = 0, nay = 0; // keep a count of votes
    for (proposalAnswerMap::iterator pIt = boot.answers.begin(); pIt != boot.answers.end(); ++pIt)
    {
        LFGProposalAnswer answer = pIt->second;
        if (answer == LFG_ANSWER_AGREE)
        {
            ++yay;
        }
        else if (answer == LFG_ANSWER_DENY)
        {
            ++nay;
        }
    }

    if (yay < REQUIRED_VOTES_FOR_BOOT && nay < REQUIRED_VOTES_FOR_BOOT)
    {
        m_bootStatusMap[groupGuid] = boot;
        return;
    }

    // if we dont have enough votes to kick or keep plr, don't send packet update
    // if else, set boot.inProgress to false, set plr + group states back to lfg-state-dungeon,
    // send packet update to group, kick plr if we had the votes, and then erase entry from boot map

    boot.inProgress = false;
    status->state = LFG_STATE_IN_DUNGEON;

    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            ObjectGuid plrGuid = pGroupPlr->GetObjectGuid();

            if (plrGuid != boot.playerVotedOn)
            {
                SetPlayerState(plrGuid, LFG_STATE_IN_DUNGEON);
                pGroupPlr->GetSession()->SendLfgBootUpdate(boot);
            }
        }
    }

    if (yay == REQUIRED_VOTES_FOR_BOOT)
    {
        // kick player from group
        if (pGroup->RemoveMember(boot.playerVotedOn, 1) <= 1)
        {
            // group->Disband(); already disbanded in RemoveMember
            sObjectMgr.RemoveGroup(pGroup);
            delete pGroup;
            // removemember sets the player's group pointer to NULL
        }
    }
}

void LFGMgr::SendRoleChosen(ObjectGuid plrGuid, ObjectGuid confirmedGuid, uint8 roles)
{
    Player* pPlayer = sPlayerRegistry.Find(plrGuid);

    if (pPlayer)
    {
        pPlayer->GetSession()->SendLfgRoleChosen(confirmedGuid.GetRawValue(), uint32(roles));
    }
}

void LFGMgr::SendRoleCheckUpdate(ObjectGuid plrGuid, LFGPackets::RoleCheckUpdate const& update)
{
    Player* pPlayer = sPlayerRegistry.Find(plrGuid);

    if (pPlayer)
    {
        pPlayer->GetSession()->SendLfgRoleCheckUpdate(update);
    }
}

void LFGMgr::SendLfgUpdate(ObjectGuid plrGuid, LFGPlayerStatus status, bool isGroup)
{
    Player* pPlayer = sPlayerRegistry.Find(plrGuid);

    if (!pPlayer)
    {
        return;
    }

    LFGPackets::UpdateStatus out;
    out.ticket = status.ticket;
    out.reason = uint8(status.updateType);
    out.comment = status.comment;
    out.isParty = isGroup;
    for (uint32 id : status.dungeonList)
    {
        out.slots.push_back(GetDungeonEntry(id));
    }

    // Needed-roles triple: real values when this player's queue entry exists.
    ObjectGuid queueGuid = pPlayer->GetGroup()
        ? pPlayer->GetGroup()->GetObjectGuid() : plrGuid;
    if (LFGPlayers* queueInfo = GetPlayerOrPartyData(queueGuid))
    {
        if (queueInfo->currentState == LFG_STATE_QUEUED)
        {
            out.needs[0] = queueInfo->neededTanks;
            out.needs[1] = queueInfo->neededHealers;
            out.needs[2] = queueInfo->neededDps;
        }
    }

    LFGPackets::UpdateFlags flags =
        LFGPackets::DeriveUpdateFlags(out.reason, uint8(status.state));
    out.joined = flags.joined;
    out.queued = flags.queued;
    out.lfgJoined = flags.lfgJoined;

    pPlayer->GetSession()->SendLfgUpdateStatus(out);
}

void LFGMgr::SendLfgJoinResult(ObjectGuid plrGuid, LfgJoinResult result, uint8 resultDetail,
                               RideTicket const& ticket,
                               std::vector<LFGPackets::JoinResultBlacklist> const& blacklist)
{
    Player* pPlayer = sPlayerRegistry.Find(plrGuid);

    if (pPlayer)
    {
        pPlayer->GetSession()->SendLfgJoinResult(result, resultDetail, ticket, blacklist);
    }
}

void LFGMgr::RemoveOldRoleChecks()
{
    for (roleCheckMap::iterator roleItr = m_roleCheckMap.begin(); roleItr != m_roleCheckMap.end();)
    {
        LFGRoleCheck roleCheck = roleItr->second;
        if ((roleCheck.waitForRoleTime - time(NULL)) <= 0) // no time left
        {
            roleCheck.state = LFG_ROLECHECK_MISSING_ROLE;   // TC parity (LFGMgr.cpp:310)

            LFGPackets::RoleCheckUpdate update;
            BuildRoleCheckPacket(roleCheck, update);

            for (roleMap::iterator roleMapItr = roleCheck.currentRoles.begin(); roleMapItr != roleCheck.currentRoles.end(); ++roleMapItr)
            {
                ObjectGuid plrGuid = roleMapItr->first;

                if (plrGuid.GetRawValue() == roleCheck.leaderGuidRaw)
                {
                    SendLfgJoinResult(plrGuid, ERR_LFG_ROLE_CHECK_FAILED, uint8(LFG_ROLECHECK_MISSING_ROLE),
                                      GetPlayerStatus(plrGuid).ticket, {});
                }

                SetPlayerState(plrGuid, LFG_STATE_NONE);
                SetPlayerUpdateType(plrGuid, LFG_UPDATE_ROLECHECK_FAILED);

                SendRoleCheckUpdate(plrGuid, update);                    // role check failed
                SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), true);  // not in lfg system anymore
            }

            m_playerData.erase(roleItr->first);
            roleItr = m_roleCheckMap.erase(roleItr);
        }
        else
        {
            ++roleItr;
        }
    }
}

void LFGMgr::RemoveOldProposals()
{
    time_t now = time(NULL);
    for (proposalMap::iterator itr = m_proposalMap.begin(); itr != m_proposalMap.end();)
    {
        if (itr->second.cancelTime <= now)
        {
            itr = RemoveProposal(itr, LFG_UPDATE_PROPOSAL_FAILED);
        }
        else
        {
            ++itr;
        }
    }
}
