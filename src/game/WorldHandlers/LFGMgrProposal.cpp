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
#include "Item.h"
#include "LFGMgr.h"
#include "Mail.h"
#include "LFGBootLogic.h"
#include "LFGProposalLogic.h"
#include "LFGDungeonResolution.h"
#include "Log.h"
#include "Object.h"
#include "Player.h"
#include "PlayerRegistry.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "Util.h"
#include "WorldSession.h"

/**
 * @file LFGMgrProposal.cpp
 * @brief Cohesion split of LFGMgr.cpp -- role check, dungeon proposal and in-dungeon flow: PerformRoleCheck, proposal send/update/decline, dungeon group create, teleport, boss-kill, kick/vote and LFG packet senders. Same LFGMgr class; no behaviour change. CMake file(GLOB) picks this file up automatically; LFGMgr.h is unchanged.
 */

void LFGMgr::BuildRoleCheckPacket(LFGRoleCheck const& roleCheck, bool isBeginning,
                                  LFGPackets::RoleCheckUpdate& out)
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
    out.isBeginning = isBeginning;

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

    // The client treats a set flag as "the check just started": it prints
    // ERR_LFG_ROLE_CHECK_INITIATED, plays 17318 and replays ROLE_CHOSEN for
    // every member who already answered. Exactly one packet may carry it.
    bool const isBeginning = !roleCheck.beginningSent &&
                             roleCheck.state == LFG_ROLECHECK_INITIALITING;
    roleCheck.beginningSent = true;

    // The isBeginning packet already replays ROLE_CHOSEN for everyone who has
    // answered, so sending it here too prints the leader's line twice.
    bool const roleChosen = !isBeginning && !plrGuid.IsEmpty();

    // Build ONE role-check packet for everyone in this check.
    LFGPackets::RoleCheckUpdate update;
    BuildRoleCheckPacket(roleCheck, isBeginning, update);

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
    // The entry's dungeonList is already the pruned concrete candidate set
    // (JoinLFG expanded randoms at join; merges intersected) -- every member
    // can enter whichever one we draw. Uniform pick, TC parity.
    proposal.dungeonID = LFGDungeonResolution::Pick(lfgGroup->dungeonList,
        urand(0, uint32(lfgGroup->dungeonList.size() - 1)));
    proposal.joinedQueue = lfgGroup->joinedTime;
    proposal.cancelTime = time(NULL) + LFG_TIME_PROPOSAL;
    proposal.queueEntryGuid = queueGuid;
    proposal.currentRoles = lfgGroup->currentRoles;

    // Pin each player to the single role they will fill. The selected masks
    // stay in currentRoles so a declined proposal re-queues them just as
    // flexible as they were; the packet and 7b's group both want the seat.
    AssignRoles(proposal.currentRoles, proposal.assignedRoles);

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

        // The client silently drops SMSG_LFG_UPDATE_STATUS when IsParty
        // disagrees with its own current group state (spec 7c, section 1.5b)
        // -- derive it from live membership, not from the proposal snapshot.
        Player* pPlayer = sPlayerRegistry.Find(plrGuid);
        bool const isParty = pPlayer && pPlayer->GetGroup();

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

        // The seat, not the selection: sending the raw mask draws an icon per
        // bit, so a player who offered to tank and heal shows up as a second
        // tank beside the real one. Fall back to the mask if unseated.
        roleMap::const_iterator seatItr = proposal.assignedRoles.find(memberGuid);
        uint8 seat = (seatItr != proposal.assignedRoles.end() &&
                      (seatItr->second & ~uint8(PLAYER_ROLE_LEADER)) != PLAYER_ROLE_NONE)
            ? seatItr->second : itr->second;

        LFGPackets::ProposalUpdatePlayer member;
        member.roles = seat;                    // leader bit kept (client isLeader)
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
        RemoveProposal(proposalID, LFG_UPDATE_PROPOSAL_DECLINED);
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

    // Success: state 2, rebroadcast, then FORM THE GROUP before any
    // UPDATE_STATUS -- the client only hides the ready popup on
    // GROUP_FOUND when it is already in a party (spec C6 / 7a C2).
    bool sendUpdate = proposal.state != LFG_PROPOSAL_SUCCESS;
    proposal.state = LFG_PROPOSAL_SUCCESS;
    time_t now = time(NULL);

    for (roleMap::const_iterator itr = proposal.currentRoles.begin();
         itr != proposal.currentRoles.end(); ++itr)
    {
        if (sendUpdate)
        {
            SendProposalUpdateToPlayer(itr->first, proposal);
        }
    }

    if (!CreateDungeonGroup(&proposal))
    {
        // Formation failed (someone vanished / group calls failed):
        // everyone agreed, so RemoveProposal re-queues all of them.
        sLog.outString("LFGMgr: proposal %u group formation FAILED, re-queueing",
                       proposal.id);
        RemoveProposal(proposalID, LFG_UPDATE_PROPOSAL_FAILED);
        return;
    }

    for (roleMap::const_iterator itr = proposal.currentRoles.begin();
         itr != proposal.currentRoles.end(); ++itr)
    {
        ObjectGuid memberGuid = itr->first;

        roleMap::const_iterator seatItr = proposal.assignedRoles.find(memberGuid);
        uint8 role = (seatItr != proposal.assignedRoles.end())
            ? seatItr->second : itr->second;
        role &= ~PLAYER_ROLE_LEADER;

        LFGPlayerStatus status = GetPlayerStatus(memberGuid);
        uint32 waitKey = status.dungeonList.empty()
            ? proposal.dungeonID : *status.dungeonList.begin();
        UpdateWaitMap(LFGRoles(role), waitKey,
                      time_t(now - proposal.joinedQueue));

        // CreateDungeonGroup has already run: every proposal player, premade
        // and former solo queuer alike, is now in the formed group -- the
        // pre-formation proposal.groups snapshot no longer reflects that.
        Player* pMember = sPlayerRegistry.Find(memberGuid);
        bool const isParty = pMember && pMember->GetGroup();
        status.updateType = LFG_UPDATE_GROUP_FOUND;
        SendLfgUpdate(memberGuid, status, isParty);
    }

    sLog.outString("LFGMgr: proposal %u formed an LFD group for dungeon %u",
                   proposal.id, proposal.dungeonID);

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

void LFGMgr::DetachFromGroup(Group* pGroup, ObjectGuid plrGuid)
{
    if (!pGroup)
    {
        return;
    }

    // RemoveMember disbands itself when the pre-removal count is <= 2, so the
    // caller must decide ownership before the call, never after it. Disband
    // never unregisters from ObjectMgr or deletes itself (Group.cpp:914-1009),
    // so this is the only teardown -- not a double-free of a self-disbanded
    // group.
    bool const willDisband = pGroup->GetMembersCount() <= 2;

    pGroup->RemoveMember(plrGuid, 0);

    if (willDisband)
    {
        sObjectMgr.RemoveGroup(pGroup);
        delete pGroup;
    }
}

ObjectGuid LFGMgr::PickHostGroup(LFGProposal const& proposal)
{
    std::vector<LFGProposalLogic::PlayerGroupPair> playerToGroup;
    playerToGroup.reserve(proposal.groups.size());
    for (playerGroupMap::const_iterator itr = proposal.groups.begin();
         itr != proposal.groups.end(); ++itr)
    {
        playerToGroup.push_back(LFGProposalLogic::PlayerGroupPair(
            itr->first.GetRawValue(), itr->second.GetRawValue()));
    }

    return ObjectGuid(LFGProposalLogic::PickHostGroup(playerToGroup));
}

//todo(7c): offer-continue formation (proposal->groupRawGuid != 0)
bool LFGMgr::CreateDungeonGroup(LFGProposal* proposal)
{
    if (!proposal || proposal->currentRoles.empty() ||
        proposal->currentRoles.size() > NORMAL_TOTAL_ROLE_COUNT)
    {
        return false;
    }

    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(proposal->dungeonID);
    MapEntry const* map = (dungeon && dungeon->mapID > 0)
        ? sMapStore.LookupEntry(uint32(dungeon->mapID)) : NULL;
    if (!dungeon || !map || !map->IsNonRaidDungeon() ||
        (dungeon->typeID != LFG_TYPE_DUNGEON && dungeon->typeID != LFG_TYPE_HEROIC_DUNGEON))
    {
        return false;
    }

    // Everyone must still be online; an offline member is the timeout's job.
    for (roleMap::const_iterator itr = proposal->currentRoles.begin();
         itr != proposal->currentRoles.end(); ++itr)
    {
        if (!sPlayerRegistry.Find(itr->first))
        {
            return false;
        }
    }

    // Prefer the pre-existing party contributing the most members: keep its
    // Group object and leader instead of tearing the premade down and
    // rebuilding it.
    Group* pGroup = NULL;
    ObjectGuid const hostGuid = PickHostGroup(*proposal);
    if (!hostGuid.IsEmpty())
    {
        pGroup = sObjectMgr.GetGroupById(hostGuid.GetCounter());
    }

    if (!pGroup)
    {
        // No premade in this proposal: build from scratch, as before.
        // Leader: the seat that carries the leader bit; lowest guid otherwise.
        ObjectGuid leaderGuid;
        for (roleMap::const_iterator itr = proposal->assignedRoles.begin();
             itr != proposal->assignedRoles.end(); ++itr)
        {
            if (itr->second & PLAYER_ROLE_LEADER)
            {
                leaderGuid = itr->first;
                break;
            }

            if (leaderGuid.IsEmpty() || itr->first < leaderGuid)
            {
                leaderGuid = itr->first;
            }
        }

        Player* pLeader = sPlayerRegistry.Find(leaderGuid);
        if (!pLeader)
        {
            return false;
        }

        pGroup = new Group();
        if (!pGroup->Create(leaderGuid, pLeader->GetName()))
        {
            delete pGroup;
            return false;
        }
        sObjectMgr.AddGroup(pGroup);
    }

    // From here on, lifecycle hooks fired by the removes/adds below must not
    // react -- IsSuccessfulProposalMove covers this proposal's guids. The
    // host party keeps its own leader; only outsiders move.
    for (roleMap::const_iterator itr = proposal->currentRoles.begin();
         itr != proposal->currentRoles.end(); ++itr)
    {
        ObjectGuid const memberGuid = itr->first;
        if (pGroup->IsMember(memberGuid))
        {
            continue;
        }

        Player* pMember = sPlayerRegistry.Find(memberGuid);
        if (!pMember)
        {
            return false;
        }

        DetachFromGroup(pMember->GetGroup(), memberGuid);

        if (!pGroup->AddMember(memberGuid, pMember->GetName()))
        {
            return false;
        }
    }

    // Seat everyone (leader bit kept -- the client draws the crown/roles
    // from these bytes in SMSG_GROUP_LIST), then flag and configure.
    for (roleMap::const_iterator itr = proposal->assignedRoles.begin();
         itr != proposal->assignedRoles.end(); ++itr)
    {
        pGroup->SetLfgRoles(itr->first, itr->second);
    }

    pGroup->SetAsLfgGroup();
    pGroup->SetDungeonDifficulty(Difficulty(dungeon->difficulty));

    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus groupStatus(LFG_STATE_IN_DUNGEON, dungeon->ID,
                               proposal->assignedRoles, pGroup->GetLeaderGuid());

    // Capture what each member queued for while their selection is still
    // around. By completion time the group holds a concrete dungeon and the
    // random slot they clicked is unrecoverable -- and members of one group
    // can have arrived from different random slots.
    for (roleMap::const_iterator itr = proposal->assignedRoles.begin();
         itr != proposal->assignedRoles.end(); ++itr)
    {
        groupStatus.queuedSlots[itr->first] = GetQueuedRandomID(itr->first);
    }

    m_groupSet.insert(groupGuid);
    m_groupStatusMap[groupGuid] = groupStatus;

    // Roster broadcast carrying the filled LFG block (state / concrete dungeon
    // entry / aborted) -- and preceding GROUP_FOUND. This has to go out BEFORE
    // the teleport: a far teleport is deferred behind the world-port ack, so a
    // group list sent afterwards reaches the client mid-loading-screen and the
    // last roster it actually applied stays the pre-SetAsLfgGroup one from
    // AddMember -- leader crown instead of the guide, and raid conversion still
    // offered.
    pGroup->SendUpdate();

    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        if (Player* pGroupPlr = itr->getSource())
        {
            // Being placed in a random group starts the 15 minute cooldown
            // on queueing for another random. Picking a dungeon by name
            // starts nothing, and completing this one lifts it early.
            queuedSlotMap::const_iterator slotItr =
                groupStatus.queuedSlots.find(pGroupPlr->GetObjectGuid());
            if (slotItr != groupStatus.queuedSlots.end() && slotItr->second != 0)
            {
                pGroupPlr->CastSpell(pGroupPlr, LFG_COOLDOWN_SPELL, true);
            }

            TeleportPlayer(pGroupPlr, false, true);
        }
    }

    return true;
}

bool LFGMgr::TeleportPlayer(Player* pPlayer, bool out, bool automatic, uint32 dungeonID)
{
    if (!pPlayer)
    {
        return false;
    }

    Group* pGroup = pPlayer->GetGroup();

    // An explicit dungeon is for callers that have already detached the
    // player from the group -- a boot -- and so cannot look one up.
    if (!dungeonID)
    {
        if (LFGGroupStatus* status = pGroup ? GetGroupStatus(pGroup->GetObjectGuid()) : NULL)
        {
            dungeonID = status->dungeonID;
        }
    }

    LfgDungeonsEntry const* dungeon = dungeonID
        ? sLfgDungeonsStore.LookupEntry(dungeonID) : NULL;
    if (!dungeon || dungeon->mapID <= 0)
    {
        pPlayer->GetSession()->SendLfgTeleportError(uint8(LFG_TELEPORTERROR_INVALID_LOCATION));
        return false;
    }

    uint32 mapID = uint32(dungeon->mapID);

    if (out)
    {
        if (pPlayer->GetMapId() != mapID)
        {
            pPlayer->GetSession()->SendLfgTeleportError(uint8(LFG_TELEPORTERROR_INVALID_LOCATION));
            return false;
        }

        // A never-stored entry point is the zero location -- teleporting
        // there would drop the player into the map-0 ocean (spec C10).
        WorldLocation const& entryPoint = pPlayer->GetBattleGroundEntryPoint();
        if (entryPoint.mapid == 0 && entryPoint.coord_x == 0.0f &&
            entryPoint.coord_y == 0.0f && entryPoint.coord_z == 0.0f)
        {
            pPlayer->GetSession()->SendLfgTeleportError(uint8(LFG_TELEPORTERROR_INVALID_LOCATION));
            return false;
        }

        if (!pPlayer->TeleportToBGEntryPoint())
        {
            pPlayer->GetSession()->SendLfgTeleportError(uint8(LFG_TELEPORTERROR_INVALID_LOCATION));
            return false;
        }

        return true;
    }

    if (pPlayer->GetMapId() == mapID)
    {
        return true;                     // already inside
    }

    LFGTeleportError err = LFG_TELEPORTERROR_OK;
    if (pPlayer->IsDead())
    {
        err = LFG_TELEPORTERROR_PLAYER_DEAD;
    }
    else if (pPlayer->IsFalling())
    {
        err = LFG_TELEPORTERROR_FALLING;
    }
    else if (pPlayer->GetVehicleInfo())
    {
        err = LFG_TELEPORTERROR_IN_VEHICLE;
    }
    else if (!pPlayer->GetCharmerGuid().IsEmpty())
    {
        err = LFG_TELEPORTERROR_CHARMING;
    }
    else if (pPlayer->IsInCombat() || pPlayer->IsTaxiFlying())
    {
        err = LFG_TELEPORTERROR_INVALID_LOCATION;
    }

    if (err == LFG_TELEPORTERROR_OK)
    {
        dungeonForbidden const lockedDungeons = FindRandomDungeonsNotForPlayer(pPlayer);
        if (lockedDungeons.find(dungeon->Entry()) != lockedDungeons.end())
        {
            err = LFG_TELEPORTERROR_INVALID_LOCATION;
        }
    }

    if (err == LFG_TELEPORTERROR_OK)
    {
        Map* pCurrentMap = pPlayer->GetMap();
        if (!pCurrentMap || pCurrentMap->IsDungeon() || pCurrentMap->IsRaid() ||
            pPlayer->InBattleGround())
        {
            err = LFG_TELEPORTERROR_INVALID_LOCATION;
        }
    }

    if (err != LFG_TELEPORTERROR_OK)
    {
        pPlayer->GetSession()->SendLfgTeleportError(uint8(err));
        return false;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float o = 0.0f;
    bool destinationFound = false;

    // Formation-time: land beside a member who is already inside.
    if (automatic)
    {
        Player* pInside = sPlayerRegistry.Find(pGroup->GetLeaderGuid());
        if (!pInside || pInside == pPlayer || pInside->GetMapId() != mapID)
        {
            pInside = NULL;
            for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* pMember = itr->getSource();
                if (pMember && pMember != pPlayer && pMember->GetMapId() == mapID)
                {
                    pInside = pMember;
                    break;
                }
            }
        }

        if (pInside)
        {
            x = pInside->Where().X();
            y = pInside->Where().Y();
            z = pInside->Where().Z();
            o = pInside->Where().Facing();
            destinationFound = true;
        }
    }

    if (!destinationFound)
    {
        // Multi-wing override first (lfg_dungeon_entrances), then the
        // generic map entrance areatrigger.
        if (ObjectMgr::LfgDungeonEntrance const* entrance =
            sObjectMgr.GetLfgDungeonEntrance(dungeonID))
        {
            x = entrance->x;
            y = entrance->y;
            z = entrance->z;
            o = entrance->o;
        }
        else if (AreaTrigger const* at = sObjectMgr.GetMapEntranceTrigger(mapID))
        {
            x = at->target_X;
            y = at->target_Y;
            z = at->target_Z;
            o = at->target_Orientation;
        }
        else
        {
            pPlayer->GetSession()->SendLfgTeleportError(uint8(LFG_TELEPORTERROR_INVALID_LOCATION));
            return false;
        }
    }

    pPlayer->SetBattleGroundEntryPoint();    // current map is validated outdoor

    if (!pPlayer->TeleportTo(mapID, x, y, z, o))
    {
        pPlayer->GetSession()->SendLfgTeleportError(uint8(LFG_TELEPORTERROR_INVALID_LOCATION));
        return false;
    }

    SetPlayerState(pPlayer->GetObjectGuid(), LFG_STATE_IN_DUNGEON);
    return true;
}

bool LFGMgr::IsSuccessfulProposalMove(ObjectGuid guid) const
{
    for (proposalMap::const_iterator itr = m_proposalMap.begin();
         itr != m_proposalMap.end(); ++itr)
    {
        LFGProposal const& proposal = itr->second;
        if (proposal.state != LFG_PROPOSAL_SUCCESS)
        {
            continue;
        }

        if (proposal.queueEntryGuid == guid)
        {
            return true;
        }

        // The VALUE arm is what covers a preserved host group: AddMember on
        // a kept premade fires OnGroupMemberAdded(hostGroupGuid, ...), and
        // the host's guid appears as a value here for each of its own
        // members -- do not "simplify" this to the KEY arm alone.
        for (playerGroupMap::const_iterator grpItr = proposal.groups.begin();
             grpItr != proposal.groups.end(); ++grpItr)
        {
            if (grpItr->first == guid || grpItr->second == guid)
            {
                return true;
            }
        }
    }

    return false;
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

bool LFGMgr::GetGroupUpdateData(ObjectGuid groupGuid, ObjectGuid playerGuid,
                                LFGGroupUpdateData& data) const
{
    groupStatusMap::const_iterator itr = m_groupStatusMap.find(groupGuid);
    if (itr == m_groupStatusMap.end())
    {
        return false;                    // caller keeps the value-initialized zeros
    }

    LFGGroupStatus const& status = itr->second;

    roleMap::const_iterator roleItr = status.playerRoles.find(playerGuid);
    data.role = (roleItr != status.playerRoles.end()) ? roleItr->second : uint8(PLAYER_ROLE_NONE);
    // MyLfgFlags: 2 = dungeon finished (kills the client's backfill offer);
    // the concrete dungeon's Entry() drives IsPartyLFG/IsInLFGDungeon (C7).
    data.state = uint8((status.state == LFG_STATE_FINISHED_DUNGEON) ? 2 : 0);
    data.dungeonEntry = GetDungeonEntry(status.dungeonID);
    return true;
}

void LFGMgr::RemoveProposal(uint32 proposalId, LfgUpdateType type)
{
    proposalMap::iterator itProposal = m_proposalMap.find(proposalId);
    if (itProposal == m_proposalMap.end())
    {
        return;
    }

    // Work on a copy and retire the map entry up front. Re-queueing a survivor
    // set below can reach SendDungeonProposal, and its insert into
    // m_proposalMap may rehash and invalidate every iterator into the map.
    // Taking the id rather than an iterator keeps that off every caller too.
    LFGProposal proposal = itProposal->second;
    m_proposalMap.erase(itProposal);

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

        if (itr->second.state != LFG_PROPOSAL_INITIATING)
        {
            return false;                // success is being formed; boots come later
        }

        itAnswer->second = LFG_ANSWER_DENY;
        RemoveProposal(itr->first, LFG_UPDATE_PROPOSAL_DECLINED);
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

namespace
{
    /// Grants a currency and reports what actually landed. CurrencyMgr
    /// clamps against the weekly and total caps, so the request and the
    /// grant are different numbers on the run that fills the week.
    uint32 GrantLfgCurrency(Player* pPlayer, uint32 currencyId, uint32 amount)
    {
        uint32 const before = pPlayer->GetCurrencyCount(currencyId);
        pPlayer->ModifyCurrencyCount(currencyId, int32(amount));
        uint32 const after = pPlayer->GetCurrencyCount(currencyId);

        return (after > before) ? (after - before) : 0;
    }

    /// Adds a granted currency to the reward packet. Quantities stay in the
    /// hundredths CurrencyMgr stores: the client divides a currency flagged
    /// 0x08 by 100 before display, so 15000 shows as 150.
    void AppendCurrencyReward(LFGPackets::PlayerReward& reward, uint32 currencyId,
                              uint32 quantity)
    {
        LFGRewardItem item;
        item.id = currencyId;
        item.displayId = 0;
        item.quantity = quantity;
        item.isCurrency = true;
        reward.items.push_back(item);
    }

    /// Puts the reward item in the player's bags, mailing it if they are
    /// full so a full inventory never eats the reward.
    void GiveRewardItem(Player* pPlayer, uint32 itemId, uint32 amount)
    {
        ItemPosCountVec dest;
        if (pPlayer->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, amount) == EQUIP_ERR_OK)
        {
            Item* pItem = pPlayer->StoreNewItem(dest, itemId, true,
                                                Item::GenerateItemRandomPropertyId(itemId));
            pPlayer->SendNewItem(pItem, amount, true, false);
            return;
        }

        Item* pItem = Item::CreateItem(itemId, amount, pPlayer);
        if (!pItem)
        {
            return;
        }

        // Save before sending, so a failed send does not lose the item.
        pItem->SaveToDB();

        ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(itemId);

        // Subject is the item's own name, as the auction house does it --
        // no new mangos_string row for one fallback path.
        MailDraft draft(pProto ? pProto->Name1 : "", "");
        draft.AddItem(pItem);
        draft.SendMailTo(pPlayer, MailSender(MAIL_NORMAL, pPlayer->GetGUIDLow(),
                                             MAIL_STATIONERY_GM));
    }
}

uint32 LFGMgr::GetQueuedRandomID(ObjectGuid plrGuid)
{
    // The queue keeps the player's own selection: a random join carries the
    // one random row they clicked, never the concrete set it expanded to.
    LFGPlayerStatus const status = GetPlayerStatus(plrGuid);
    for (std::set<uint32>::const_iterator itr = status.dungeonList.begin();
         itr != status.dungeonList.end(); ++itr)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(*itr);
        if (dungeon && (dungeon->typeID == LFG_TYPE_RANDOM_DUNGEON ||
                        IsSeasonal(dungeon->flags)))
        {
            return dungeon->ID;
        }
    }

    return 0;
}

void LFGMgr::RewardDungeonCompletion(Player* pPlayer, LFGGroupStatus const& status,
                                     DungeonTypes type)
{
    ObjectGuid const plrGuid = pPlayer->GetObjectGuid();

    queuedSlotMap::const_iterator slotItr = status.queuedSlots.find(plrGuid);
    uint32 const queuedID = (slotItr != status.queuedSlots.end()) ? slotItr->second : 0;

    LFGPackets::PlayerReward reward;
    reward.actualSlot = GetDungeonEntry(status.dungeonID);
    reward.queuedSlot = queuedID ? GetDungeonEntry(queuedID) : reward.actualSlot;

    // Only a random (or seasonal) queue pays. Picking a dungeon by name and
    // clearing it is its own reward -- the era reward quests are all
    // "Random ..." quests, and the client's own preview only ever names a
    // random slot.
    if (!queuedID)
    {
        return;
    }

    DungeonTypes const randomType = GetDungeonType(queuedID);
    uint32 const level = pPlayer->getLevel();
    bool const atMaxLevel =
        (level >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));

    // Max-level heroic randoms are paid in currency on a weekly cadence;
    // everything else pays gold and experience, doubled on the first run of
    // the day for that tier.
    bool const currencyRegime = (randomType == DUNGEON_CATACLYSM_HEROIC);

    DungeonFinderRewards const* rewards = sObjectMgr.GetDungeonFinderRewards(level);
    uint32 money = 0;
    uint32 xp = 0;
    bool hasDoneDaily = true;

    if (rewards)
    {
        if (currencyRegime)
        {
            // Repeatable, so the stored (subsequent-run) value stands as is.
            money = uint32(rewards->baseMonetaryReward);
        }
        else
        {
            hasDoneDaily = HasPlayerDoneDaily(pPlayer->GetGUIDLow(), randomType);

            uint32 const multiplier = LFGRewardLogic::FirstRewardMultiplier(hasDoneDaily);
            money = uint32(rewards->baseMonetaryReward) * multiplier;
            xp = rewards->baseXPReward * multiplier;

            RegisterPlayerDaily(pPlayer->GetGUIDLow(), randomType);
        }
    }
    else
    {
        // No row for this level: pay nothing rather than dereference one.
        sLog.outErrorDb("LFGMgr: no dungeonfinder_rewards row for level %u, "
                        "player %s completed dungeon %u unpaid",
                        level, plrGuid.GetString().c_str(), status.dungeonID);
    }

    if (atMaxLevel)
    {
        xp = 0;
    }

    // Currency. The granted amount is measured, never assumed: CurrencyMgr
    // clamps against the weekly and total caps, and the packet has to carry
    // what the player actually received.
    if (currencyRegime)
    {
        uint32 granted = GrantLfgCurrency(pPlayer, LFGRewardLogic::LFG_CURRENCY_VALOR,
                                          LFGRewardLogic::CATA_HEROIC_VALOR);
        if (granted > 0)
        {
            AppendCurrencyReward(reward, LFGRewardLogic::LFG_CURRENCY_VALOR, granted);
        }
        else
        {
            // Valor week spent -- the run still pays, in Justice.
            granted = GrantLfgCurrency(pPlayer, LFGRewardLogic::LFG_CURRENCY_JUSTICE,
                                       LFGRewardLogic::CATA_HEROIC_JUSTICE_FALLBACK);
            if (granted > 0)
            {
                AppendCurrencyReward(reward, LFGRewardLogic::LFG_CURRENCY_JUSTICE, granted);
            }
        }
    }
    else if (randomType == DUNGEON_CATACLYSM &&
             level >= LFGRewardLogic::CATA_NORMAL_JUSTICE_MIN_LEVEL)
    {
        // Slot 300 spans levels 80-85, but its Justice only starts with the
        // 83+ reward quests; below that it is a gold and experience run.
        uint8& runsThisWeek = m_weeklyCataNormal[pPlayer->GetGUIDLow()];
        if (runsThisWeek < LFGRewardLogic::CATA_NORMAL_JUSTICE_RUNS_PER_WEEK)
        {
            uint32 const granted = GrantLfgCurrency(pPlayer,
                                                    LFGRewardLogic::LFG_CURRENCY_JUSTICE,
                                                    LFGRewardLogic::CATA_NORMAL_JUSTICE);
            if (granted > 0)
            {
                ++runsThisWeek;
                AppendCurrencyReward(reward, LFGRewardLogic::LFG_CURRENCY_JUSTICE, granted);
            }
        }
    }

    // Item, first run of the day only.
    if (!currencyRegime && !hasDoneDaily)
    {
        ItemRewards const itemRewards = GetDungeonItemRewards(status.dungeonID, type);
        if (itemRewards.itemId && itemRewards.itemAmount)
        {
            GiveRewardItem(pPlayer, itemRewards.itemId, itemRewards.itemAmount);

            if (ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(itemRewards.itemId))
            {
                LFGRewardItem item;
                item.id = itemRewards.itemId;
                item.displayId = pProto->DisplayInfoID;
                item.quantity = itemRewards.itemAmount;
                item.isCurrency = false;
                reward.items.push_back(item);
            }
        }
    }

    if (money > 0)
    {
        pPlayer->ModifyMoney(int64(money));
    }

    if (xp > 0)
    {
        pPlayer->GiveXP(xp, NULL);
    }

    reward.rewardMoney = money;
    reward.addedXp = xp;

    pPlayer->GetSession()->SendLfgPlayerReward(reward);
}

void LFGMgr::OnDungeonEncounterComplete(Map* pMap)
{
    if (!pMap)
    {
        return;
    }

    // The groups standing on this instance. A player may be grouped without
    // that group being an LFD one -- the status lookup below filters those.
    std::set<ObjectGuid> groupGuids;
    Map::PlayerList const& players = pMap->GetPlayers();
    for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
    {
        Player* pPlayer = itr->getSource();
        if (!pPlayer)
        {
            continue;
        }

        if (Group* pGroup = pPlayer->GetGroup())
        {
            groupGuids.insert(pGroup->GetObjectGuid());
        }
    }

    for (std::set<ObjectGuid>::const_iterator itr = groupGuids.begin();
         itr != groupGuids.end(); ++itr)
    {
        LFGGroupStatus* status = GetGroupStatus(*itr);
        if (!status || status->state != LFG_STATE_IN_DUNGEON)
        {
            continue;
        }

        // The dungeon the group queued for is the authority on what was just
        // completed; instance_encounters only says "this was a last boss".
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(status->dungeonID);
        if (!dungeon || dungeon->mapID != int32(pMap->GetId()) ||
            Difficulty(dungeon->difficulty) != pMap->GetDifficulty())
        {
            continue;
        }

        Group* pGroup = sObjectMgr.GetGroupById(itr->GetCounter());
        if (!pGroup)
        {
            continue;
        }

        // Flip state before paying anyone: a second last-boss credit finds
        // the group finished and skips it.
        status->state = LFG_STATE_FINISHED_DUNGEON;

        DungeonTypes const type = GetDungeonType(status->dungeonID);

        for (GroupReference* ref = pGroup->GetFirstMember(); ref != NULL; ref = ref->next())
        {
            Player* pGroupPlr = ref->getSource();

            // Only members who were there for the kill. Nobody is paid for
            // walking in afterwards.
            if (!pGroupPlr || pGroupPlr->GetMapId() != pMap->GetId())
            {
                continue;
            }

            RewardDungeonCompletion(pGroupPlr, *status, type);

            // Finishing lifts the random cooldown early.
            pGroupPlr->RemoveAurasDueToSpell(LFG_COOLDOWN_SPELL);

            ObjectGuid const plrGuid = pGroupPlr->GetObjectGuid();
            SetPlayerState(plrGuid, LFG_STATE_FINISHED_DUNGEON);
            SetPlayerUpdateType(plrGuid, LFG_UPDATE_DUNGEON_FINISHED);
            SendLfgUpdate(plrGuid, GetPlayerStatus(plrGuid), true);
        }
    }

    // The group status stays, now FINISHED: it is what suppresses the
    // client's backfill offer and what tells a later leave that the run was
    // already over. OnGroupDisband clears it when the group dissolves.
}

void LFGMgr::AttemptToKickPlayer(Group* pGroup, ObjectGuid guid, ObjectGuid kicker, std::string reason)
{
    if (!pGroup || !LFGBootLogic::ShouldVoteKick(guid.GetRawValue(), kicker.GetRawValue()) ||
        !pGroup->IsMember(guid) || !pGroup->IsMember(kicker))
    {
        return;
    }

    ObjectGuid groupGuid = pGroup->GetObjectGuid();
    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    if (!status || status->state != LFG_STATE_IN_DUNGEON)
    {
        return;
    }

    if (m_bootStatusMap.find(groupGuid) != m_bootStatusMap.end())
    {
        return;                     // a boot is already running for this group
    }

    LFGState const previousState = status->state;
    status->state = LFG_STATE_BOOT;   // mutate through the pointer -- no write-back copy

    uint32 const votesNeeded = LFGBootLogic::RequiredVotes(pGroup->isLFRGroup());

    // Seed votes from playerRoles (not GetFirstMember()) so offline members
    // stay in the tally: victim DENY, kicker AGREE, everyone else PENDING.
    proposalAnswerMap votes;
    for (roleMap::const_iterator itr = status->playerRoles.begin();
         itr != status->playerRoles.end(); ++itr)
    {
        ObjectGuid const memberGuid = itr->first;

        if (memberGuid == guid)
        {
            votes[memberGuid] = LFG_ANSWER_DENY;
        }
        else if (memberGuid == kicker)
        {
            votes[memberGuid] = LFG_ANSWER_AGREE;
        }
        else
        {
            votes[memberGuid] = LFG_ANSWER_PENDING;
        }

        SetPlayerState(memberGuid, LFG_STATE_BOOT);
    }

    LFGBoot boot(true, false, previousState, guid, reason, votes, time(NULL));
    m_bootStatusMap[groupGuid] = boot;

    // Include the victim: their myVoteCompleted suppresses the popup
    // (section 1.1), but voteInProgress must be set so the terminating
    // packet can clear it later.
    for (proposalAnswerMap::const_iterator itr = boot.answers.begin();
         itr != boot.answers.end(); ++itr)
    {
        if (Player* pMember = sPlayerRegistry.Find(itr->first))
        {
            pMember->GetSession()->SendLfgBootProposalUpdate(boot, votesNeeded);
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
    if (!pGroup)
    {
        return;
    }

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

    LFGBoot& boot = it->second;   // a reference: mutations must stick

    proposalAnswerMap::iterator ansItr = boot.answers.find(pPlayer->GetObjectGuid());
    if (ansItr == boot.answers.end() || ansItr->second != LFG_ANSWER_PENDING)
    {
        return;                     // not a voter, or already voted
    }

    ansItr->second = LFGProposalAnswer(vote);

    uint32 const votesNeeded = LFGBootLogic::RequiredVotes(pGroup->isLFRGroup());

    LFGBootLogic::BootTally tally;
    for (proposalAnswerMap::const_iterator pIt = boot.answers.begin(); pIt != boot.answers.end(); ++pIt)
    {
        if (pIt->second == LFG_ANSWER_AGREE)
        {
            ++tally.agree;
            ++tally.total;
        }
        else if (pIt->second == LFG_ANSWER_DENY)
        {
            ++tally.deny;
            ++tally.total;
        }
    }

    LFGBootLogic::BootResolution const resolution = LFGBootLogic::Resolve(tally, votesNeeded);
    if (resolution == LFGBootLogic::BOOT_PASSED)
    {
        FinishBootVote(groupGuid, true);
        return;
    }

    if (resolution == LFGBootLogic::BOOT_FAILED)
    {
        FinishBootVote(groupGuid, false);
        return;
    }

    // Still pending: re-broadcast. Safe on the voter's own client too --
    // noCancelOnReuse means a fresh update never fires a spurious OnCancel.
    for (proposalAnswerMap::const_iterator pIt = boot.answers.begin(); pIt != boot.answers.end(); ++pIt)
    {
        if (Player* pMember = sPlayerRegistry.Find(pIt->first))
        {
            pMember->GetSession()->SendLfgBootProposalUpdate(boot, votesNeeded);
        }
    }
}

/// Resolves one boot vote, notifies the group and applies the outcome.
void LFGMgr::FinishBootVote(ObjectGuid groupGuid, bool succeeded)
{
    bootStatusMap::iterator it = m_bootStatusMap.find(groupGuid);
    if (it == m_bootStatusMap.end())
    {
        return;
    }

    // Copy-then-erase-then-act: RemoveMember below re-enters through
    // OnGroupMemberRemoved -> CancelBootVote, and the entry must already be
    // gone for that recursive call to be a no-op (section 1.7 item 4).
    LFGBoot boot = it->second;
    m_bootStatusMap.erase(it);

    boot.inProgress = false;
    boot.votePassed = succeeded;

    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    if (status)
    {
        status->state = boot.previousState;
        for (proposalAnswerMap::const_iterator ansItr = boot.answers.begin();
             ansItr != boot.answers.end(); ++ansItr)
        {
            SetPlayerState(ansItr->first, boot.previousState);
        }
    }

    Group* pGroup = sObjectMgr.GetGroupById(groupGuid.GetCounter());
    uint32 const votesNeeded = LFGBootLogic::RequiredVotes(pGroup && pGroup->isLFRGroup());

    // Broadcast BEFORE touching the group: this is what closes the popup on
    // every client (the only thing that does) and what prints the pass/fail
    // line while the victim's guid still resolves. The victim is included --
    // see OQ-8.1 in the PR body for the TrinityCore divergence and why.
    for (proposalAnswerMap::const_iterator ansItr = boot.answers.begin();
         ansItr != boot.answers.end(); ++ansItr)
    {
        if (Player* pMember = sPlayerRegistry.Find(ansItr->first))
        {
            pMember->GetSession()->SendLfgBootProposalUpdate(boot, votesNeeded);
        }
    }

    if (!succeeded)
    {
        if (pGroup)
        {
            pGroup->SendUpdate();
        }
        return;
    }

    if (status)
    {
        status->kicksLeft = (status->kicksLeft > 0) ? status->kicksLeft - 1 : 0;
    }

    if (pGroup && pGroup->IsMember(boot.playerVotedOn))
    {
        Player* pVictim = sPlayerRegistry.Find(boot.playerVotedOn);
        if (pVictim)
        {
            // A booted player gets their random dungeon cooldown back.
            // RemoveMember's hook does this too, but only while the group
            // survives -- a boot that takes it below three disbands
            // instead, and this is the only clearing on that path.
            // Whether the victim also earns Deserter is the hook's call,
            // under LFG.Deserter.OnVoteKick (exempt by default).
            pVictim->RemoveAurasDueToSpell(LFG_COOLDOWN_SPELL);
        }

        // Read the dungeon before the removal takes the group away, so the
        // teleport below still knows where the victim is being pulled out of.
        uint32 const bootedFrom = status ? status->dungeonID : 0;
        bool const victimInDungeon =
            pVictim && pVictim->GetMap() && pVictim->GetMap()->IsDungeon();

        // Remove BEFORE teleporting. RemoveMember is what tells the victim
        // they are out -- SMSG_GROUP_UNINVITE plus an empty SMSG_GROUP_LIST
        // -- and a far teleport puts them behind a loading screen that
        // discards both, leaving them staring at a group they already left
        // while everyone else sees them gone. The world-port ack cannot
        // repair it either: its roster re-send only fires for a player who
        // still has a group.
        bool const groupSurvived = pGroup->RemoveMember(boot.playerVotedOn, 1) > 1;

        if (victimInDungeon)
        {
            TeleportPlayer(pVictim, true, true, bootedFrom);
        }

        if (!groupSurvived)
        {
            // group->Disband(); already disbanded in RemoveMember
            sObjectMgr.RemoveGroup(pGroup);
            delete pGroup;
            // RemoveMember sets the player's group pointer to NULL
        }

        // Guarded against the group having just been deleted above.
        if (groupSurvived && status && status->state != LFG_STATE_FINISHED_DUNGEON)
        {
            if (Player* pLeader = sPlayerRegistry.Find(pGroup->GetLeaderGuid()))
            {
                pLeader->GetSession()->SendLfgOfferContinue(
                    GetDungeonEntry(status->dungeonID));
            }
        }
    }
}

/// Fails boot votes that have run past LFG_TIME_BOOT.
void LFGMgr::RemoveOldBoots()
{
    time_t const now = time(NULL);

    // Collect first: FinishBootVote erases from the map being walked, so a
    // single-phase loop is UB (mirrors RemoveOldProposals above).
    std::vector<ObjectGuid> expired;
    for (bootStatusMap::const_iterator itr = m_bootStatusMap.begin();
         itr != m_bootStatusMap.end(); ++itr)
    {
        if (itr->second.startTime + LFG_TIME_BOOT <= now)
        {
            expired.push_back(itr->first);
        }
    }

    for (std::vector<ObjectGuid>::const_iterator itr = expired.begin();
         itr != expired.end(); ++itr)
    {
        FinishBootVote(*itr, false);
    }
}

/// Cancel any in-flight boot for this group without penalty -- called from
/// the group lifecycle seam (OnGroupMemberRemoved / OnGroupDisband) when a
/// member is already leaving. Must not route through FinishBootVote's
/// removal path.
void LFGMgr::CancelBootVote(ObjectGuid groupGuid)
{
    bootStatusMap::iterator it = m_bootStatusMap.find(groupGuid);
    if (it == m_bootStatusMap.end())
    {
        return;
    }

    LFGBoot boot = it->second;
    m_bootStatusMap.erase(it);

    boot.inProgress = false;
    boot.votePassed = false;

    LFGGroupStatus* status = GetGroupStatus(groupGuid);
    if (status)
    {
        status->state = boot.previousState;
        for (proposalAnswerMap::const_iterator ansItr = boot.answers.begin();
             ansItr != boot.answers.end(); ++ansItr)
        {
            SetPlayerState(ansItr->first, boot.previousState);
        }
    }

    Group* pGroup = sObjectMgr.GetGroupById(groupGuid.GetCounter());
    uint32 const votesNeeded = LFGBootLogic::RequiredVotes(pGroup && pGroup->isLFRGroup());

    for (proposalAnswerMap::const_iterator ansItr = boot.answers.begin();
         ansItr != boot.answers.end(); ++ansItr)
    {
        if (Player* pMember = sPlayerRegistry.Find(ansItr->first))
        {
            pMember->GetSession()->SendLfgBootProposalUpdate(boot, votesNeeded);
        }
    }
}

bool LFGMgr::IsBootVoteActive(ObjectGuid groupGuid) const
{
    return m_bootStatusMap.find(groupGuid) != m_bootStatusMap.end();
}

uint8 LFGMgr::GetKicksLeft(ObjectGuid groupGuid) const
{
    groupStatusMap::const_iterator it = m_groupStatusMap.find(groupGuid);
    return (it != m_groupStatusMap.end()) ? it->second.kicksLeft : 0;
}

LFGState LFGMgr::GetGroupLfgState(ObjectGuid groupGuid) const
{
    groupStatusMap::const_iterator it = m_groupStatusMap.find(groupGuid);
    return (it != m_groupStatusMap.end()) ? it->second.state : LFG_STATE_NONE;
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
            BuildRoleCheckPacket(roleCheck, false, update);

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

    // Collect first: removing a proposal re-queues its survivors, which can
    // insert a fresh proposal and rehash the map out from under a live walk.
    std::vector<uint32> expired;
    for (proposalMap::const_iterator itr = m_proposalMap.begin();
         itr != m_proposalMap.end(); ++itr)
    {
        if (itr->second.cancelTime <= now)
        {
            expired.push_back(itr->first);
        }
    }

    for (std::vector<uint32>::const_iterator itr = expired.begin();
         itr != expired.end(); ++itr)
    {
        RemoveProposal(*itr, LFG_UPDATE_PROPOSAL_FAILED);
    }
}
