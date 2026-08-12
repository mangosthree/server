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

#include <set>
#include "DBCEnums.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include "GameEventMgr.h"
#include "Group.h"
#include "LFGLockReason.h"
#include "LFGMgr.h"
#include "LFGRoleAssignment.h"
#include "Log.h"
#include "Object.h"
#include "Player.h"
#include "PlayerRegistry.h"
#include "ObjectMgr.h"
#include "SharedDefines.h"
#include "WorldSession.h"


LFGMgr::LFGMgr()
{
    m_proposalId = 0;
    m_ticketId = 0;
}

LFGMgr::~LFGMgr()
{
    m_dailyAny.clear();
    m_dailyTBCHeroic.clear();
    m_dailyLKNormal.clear();
    m_dailyLKHeroic.clear();

    m_playerData.clear();
    m_queueSet.clear();

    m_playerStatusMap.clear();
    m_groupStatusMap.clear();
    m_groupSet.clear();
    m_proposalMap.clear();

    m_roleCheckMap.clear();

    m_bootStatusMap.clear();

    m_tankWaitTime.clear();
    m_healerWaitTime.clear();
    m_dpsWaitTime.clear();
    m_avgWaitTime.clear();
}

void LFGMgr::Update()
{
    //todo: remove old queues

    // remove old role checks
    RemoveOldRoleChecks();

    // remove expired dungeon proposals (45 s -- LFG_TIME_PROPOSAL)
    RemoveOldProposals();

    // fail expired boot votes (120 s -- LFG_TIME_BOOT)
    RemoveOldBoots();

    // go through a waitTimeMap::iterator for each wait map and update times based on player count
    for (waitTimeMap::iterator tankItr = m_tankWaitTime.begin(); tankItr != m_tankWaitTime.end(); ++tankItr)
    {
        LFGWait waitInfo = tankItr->second;
        if (waitInfo.doAverage)
        {
            int32 lastTime = waitInfo.previousTime;
            int32 thisTime = waitInfo.time;

            // average of the two join times
            waitInfo.time = (thisTime + lastTime) / 2;

            // now set what was just the current wait time to the previous time for a later calculation
            waitInfo.previousTime = thisTime;
            waitInfo.doAverage = false;

            tankItr->second = waitInfo;
        }
    }
    for (waitTimeMap::iterator healItr = m_healerWaitTime.begin(); healItr != m_healerWaitTime.end(); ++healItr)
    {
        LFGWait waitInfo = healItr->second;
        if (waitInfo.doAverage)
        {
            int32 lastTime = waitInfo.previousTime;
            int32 thisTime = waitInfo.time;

            // average of the two join times
            waitInfo.time = (thisTime + lastTime) / 2;

            // now set what was just the current wait time to the previous time for a later calculation
            waitInfo.previousTime = thisTime;
            waitInfo.doAverage = false;

            healItr->second = waitInfo;
        }
    }
    for (waitTimeMap::iterator dpsItr = m_dpsWaitTime.begin(); dpsItr != m_dpsWaitTime.end(); ++dpsItr)
    {
        LFGWait waitInfo = dpsItr->second;
        if (waitInfo.doAverage)
        {
            int32 lastTime = waitInfo.previousTime;
            int32 thisTime = waitInfo.time;

            // average of the two join times
            waitInfo.time = (thisTime + lastTime) / 2;

            // now set what was just the current wait time to the previous time for a later calculation
            waitInfo.previousTime = thisTime;
            waitInfo.doAverage = false;

            dpsItr->second = waitInfo;
        }
    }
    for (waitTimeMap::iterator avgItr = m_avgWaitTime.begin(); avgItr != m_avgWaitTime.end(); ++avgItr)
    {
        LFGWait waitInfo = avgItr->second;
        if (waitInfo.doAverage)
        {
            int32 lastTime = waitInfo.previousTime;
            int32 thisTime = waitInfo.time;

            // average of the two join times
            waitInfo.time = (thisTime + lastTime) / 2;

            // now set what was just the current wait time to the previous time for a later calculation
            waitInfo.previousTime = thisTime;
            waitInfo.doAverage = false;

            avgItr->second = waitInfo;
        }
    }

    // Queue System
    FindQueueMatches();
    SendQueueStatus();
}


ItemRewards LFGMgr::GetDungeonItemRewards(uint32 dungeonId, DungeonTypes type)
{
    ItemRewards rewards;
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonId);
    if (dungeon)
    {
        uint32 minLevel = dungeon->minLevel;
        uint32 maxLevel = dungeon->maxLevel;
        uint32 avgLevel = (minLevel+maxLevel)/2; // otherwise there are issues

        DungeonFinderItemsMap const& itemBuffer = sObjectMgr.GetDungeonFinderItemsMap();
        for (DungeonFinderItemsMap::const_iterator it = itemBuffer.begin(); it != itemBuffer.end(); ++it)
        {
            DungeonFinderItems itemCache = it->second;

            // should only be one row matching this band and tier
            if (LFGRewardLogic::ItemRewardMatches(itemCache.minLevel, itemCache.maxLevel,
                                                  itemCache.dungeonType, avgLevel, type))
            {
                rewards.itemId = itemCache.itemReward;
                rewards.itemAmount = itemCache.itemAmount;
                return rewards;
            }
        }
    }
    return rewards;
}

DungeonTypes LFGMgr::GetDungeonType(uint32 dungeonId)
{
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(dungeonId);
    if (!dungeon)
    {
        return DUNGEON_UNKNOWN;
    }

    return LFGRewardLogic::ClassifyDungeon(dungeon->expansionLevel, dungeon->difficulty);
}

void LFGMgr::RegisterPlayerDaily(uint32 guidLow, DungeonTypes dungeon)
{
    switch (dungeon)
    {
        case DUNGEON_CLASSIC:
        case DUNGEON_TBC:
            m_dailyAny.insert(guidLow);
            break;
        case DUNGEON_TBC_HEROIC:
            m_dailyTBCHeroic.insert(guidLow);
            break;
        case DUNGEON_WOTLK:
            m_dailyLKNormal.insert(guidLow);
            break;
        case DUNGEON_WOTLK_HEROIC:
            m_dailyLKHeroic.insert(guidLow);
            break;
        case DUNGEON_CATACLYSM:
            m_dailyCataNormal.insert(guidLow);
            break;
        // DUNGEON_CATACLYSM_HEROIC has no daily set: its cadence is the
        // weekly currency cap, not a first-of-day bonus.
        default:
            break;
    }
}

bool LFGMgr::HasPlayerDoneDaily(uint32 guidLow, DungeonTypes dungeon)
{
    switch (dungeon)
    {
        case DUNGEON_CLASSIC:
        case DUNGEON_TBC:
            return (m_dailyAny.find(guidLow) != m_dailyAny.end()) ? true : false;
        case DUNGEON_TBC_HEROIC:
            return (m_dailyTBCHeroic.find(guidLow) != m_dailyTBCHeroic.end()) ? true : false;
        case DUNGEON_WOTLK:
            return (m_dailyLKNormal.find(guidLow) != m_dailyLKNormal.end()) ? true : false;
        case DUNGEON_WOTLK_HEROIC:
            return (m_dailyLKHeroic.find(guidLow) != m_dailyLKHeroic.end()) ? true : false;
        case DUNGEON_CATACLYSM:
            return (m_dailyCataNormal.find(guidLow) != m_dailyCataNormal.end()) ? true : false;
        default:
            return false;
    }
    return false;
}

void LFGMgr::ResetDailyRecords()
{
    m_dailyAny.clear();
    m_dailyTBCHeroic.clear();
    m_dailyLKNormal.clear();
    m_dailyLKHeroic.clear();
    m_dailyCataNormal.clear();
}

void LFGMgr::ResetWeeklyRecords()
{
    m_weeklyCataNormal.clear();
}

bool LFGMgr::IsSeasonActive(uint32 dungeonId)
{
    switch (dungeonId)
    {
        case 285:
            return IsHolidayActive(HOLIDAY_HALLOWS_END);
        case 286:
            return IsHolidayActive(HOLIDAY_FIRE_FESTIVAL);
        case 287:
            return IsHolidayActive(HOLIDAY_BREWFEST);
        case 288:
            return IsHolidayActive(HOLIDAY_LOVE_IS_IN_THE_AIR);
        default:
            return false;
    }
    return false;
}

dungeonEntries LFGMgr::FindRandomDungeonsForPlayer(uint32 level, uint8 expansion)
{
    dungeonEntries randomDungeons;

    // go through the dungeon dbc and select the applicable dungeons
    for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(id);
        if (dungeon)
        {
            if ( (dungeon->typeID == LFG_TYPE_RANDOM_DUNGEON)
                || (IsSeasonal(dungeon->flags) && IsSeasonActive(dungeon->ID)) )
                if ((uint8)dungeon->expansionLevel <= expansion && dungeon->minLevel <= level
                    && dungeon->maxLevel >= level)
                    randomDungeons[dungeon->ID] = dungeon->Entry();
        }
    }
    return randomDungeons;
}

void LFGMgr::BuildRandomDungeonRewards(Player* pPlayer,
                                       std::vector<LFGPackets::LFGRandomDungeonEntry>& out)
{
    if (!pPlayer)
    {
        return;
    }

    uint32 const level = pPlayer->getLevel();
    bool const atMaxLevel = (level >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));

    DungeonFinderRewards const* rewards = sObjectMgr.GetDungeonFinderRewards(level);

    dungeonEntries const randomDungeons =
        FindRandomDungeonsForPlayer(level, pPlayer->GetSession()->Expansion());

    for (dungeonEntries::const_iterator itr = randomDungeons.begin();
         itr != randomDungeons.end(); ++itr)
    {
        DungeonTypes const type = GetDungeonType(itr->first);

        LFGPackets::LFGRandomDungeonEntry entry;
        entry.slot = itr->second;

        // Random slots carry no map of their own, so there is no bound
        // instance to report encounters from.
        entry.completedMask = 0;

        if (type == DUNGEON_CATACLYSM_HEROIC)
        {
            // Valor, capped weekly by the currency itself. The panel works
            // the "N more times this week" line out of these numbers.
            CurrencyTypesEntry const* valor =
                sCurrencyTypesStore.LookupEntry(LFGRewardLogic::LFG_CURRENCY_VALOR);

            uint32 const weekCount =
                pPlayer->GetCurrencyWeekCount(LFGRewardLogic::LFG_CURRENCY_VALOR);
            uint32 const weekCap = valor ? pPlayer->GetCurrencyWeekCap(valor) : 0;

            entry.firstReward = (weekCount == 0);
            entry.completionCurrencyId = LFGRewardLogic::LFG_CURRENCY_VALOR;
            entry.completionQuantity = LFGRewardLogic::CATA_HEROIC_VALOR;
            entry.completionLimit =
                LFGRewardLogic::CATA_HEROIC_VALOR *
                LFGRewardLogic::CATA_NORMAL_JUSTICE_RUNS_PER_WEEK;
            entry.specificQuantity = weekCount;
            entry.specificLimit = entry.completionLimit;
            entry.overallQuantity = weekCount;
            entry.overallLimit = entry.completionLimit;
            entry.purseWeeklyQuantity = weekCount;
            entry.purseWeeklyLimit = weekCap;
            entry.purseQuantity =
                pPlayer->GetCurrencyCount(LFGRewardLogic::LFG_CURRENCY_VALOR);
            entry.purseLimit = valor ? pPlayer->GetCurrencyTotalCap(valor) : 0;
            entry.quantity = entry.completionQuantity;

            LFGRewardItem item;
            item.id = LFGRewardLogic::LFG_CURRENCY_VALOR;
            item.quantity = LFGRewardLogic::CATA_HEROIC_VALOR;
            item.isCurrency = true;
            entry.items.push_back(item);
        }
        else if (type == DUNGEON_CATACLYSM &&
                 level >= LFGRewardLogic::CATA_NORMAL_JUSTICE_MIN_LEVEL)
        {
            // Justice has no weekly cap of its own, so the allowance is
            // ours to report. The purse fields stay zero: the client skips
            // a zero purse limit when it works out what is left.
            std::unordered_map<uint32, uint8>::const_iterator weekItr =
                m_weeklyCataNormal.find(pPlayer->GetGUIDLow());
            uint32 const runsThisWeek =
                (weekItr != m_weeklyCataNormal.end()) ? weekItr->second : 0;

            entry.firstReward = !HasPlayerDoneDaily(pPlayer->GetGUIDLow(), type);
            entry.completionCurrencyId = LFGRewardLogic::LFG_CURRENCY_JUSTICE;
            entry.completionQuantity = LFGRewardLogic::CATA_NORMAL_JUSTICE;
            entry.completionLimit =
                LFGRewardLogic::CATA_NORMAL_JUSTICE *
                LFGRewardLogic::CATA_NORMAL_JUSTICE_RUNS_PER_WEEK;
            entry.specificQuantity = runsThisWeek * LFGRewardLogic::CATA_NORMAL_JUSTICE;
            entry.specificLimit = entry.completionLimit;
            entry.overallQuantity = entry.specificQuantity;
            entry.overallLimit = entry.completionLimit;
            entry.quantity = entry.completionQuantity;

            LFGRewardItem item;
            item.id = LFGRewardLogic::LFG_CURRENCY_JUSTICE;
            item.quantity = LFGRewardLogic::CATA_NORMAL_JUSTICE;
            item.isCurrency = true;
            entry.items.push_back(item);
        }
        else
        {
            // No currency: the panel drops to its flat "You will receive
            // this reward:" text, which is what a levelling random pays.
            entry.firstReward = !HasPlayerDoneDaily(pPlayer->GetGUIDLow(), type);

            ItemRewards const itemRewards = GetDungeonItemRewards(itr->first, type);
            if (entry.firstReward && itemRewards.itemId && itemRewards.itemAmount)
            {
                if (ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(itemRewards.itemId))
                {
                    LFGRewardItem item;
                    item.id = itemRewards.itemId;
                    item.displayId = pProto->DisplayInfoID;
                    item.quantity = itemRewards.itemAmount;
                    entry.items.push_back(item);
                }
            }
        }

        // The preview is always the full first-completion value: the panel
        // has its own "already done" wording for the reduced run.
        if (rewards)
        {
            uint32 const multiplier =
                (type == DUNGEON_CATACLYSM_HEROIC)
                    ? 1u : LFGRewardLogic::FirstRewardMultiplier(false);

            entry.rewardMoney = uint32(rewards->baseMonetaryReward) * multiplier;
            entry.rewardXp = atMaxLevel ? 0 : (rewards->baseXPReward * multiplier);
        }

        out.push_back(entry);
    }
}

dungeonForbidden LFGMgr::FindRandomDungeonsNotForPlayer(Player* plr)
{
    dungeonForbidden randomDungeons;

    for (LFGLockedDungeon const& locked : GetPlayerLockList(plr))
    {
        randomDungeons[locked.slot] = locked.reason;
    }

    return randomDungeons;
}

std::vector<LFGLockedDungeon> LFGMgr::GetPlayerLockList(Player* plr)
{
    std::vector<LFGLockedDungeon> lockList;

    uint32 level = plr->getLevel();
    uint8 expansion = plr->GetSession()->Expansion();
    bool isAlliance = plr->GetTeam() == ALLIANCE;
    uint32 currentItemLevel = plr->GetEquipGearScore(false, false);

    for (uint32 id = 0; id < sLfgDungeonsStore.GetNumRows(); ++id)
    {
        LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(id);
        if (!dungeon)
        {
            continue;
        }

        LFGLockReason::CheckInput in;
        in.playerLevel = level;
        in.playerExpansion = expansion;
        in.isAlliance = isAlliance;
        in.dungeonExpansionLevel = dungeon->expansionLevel;
        in.dungeonTypeID = dungeon->typeID;
        in.dungeonMinLevel = dungeon->minLevel;
        in.dungeonMaxLevel = dungeon->maxLevel;
        in.isSeasonal = IsSeasonal(dungeon->flags);
        in.seasonActive = in.isSeasonal && IsSeasonActive(dungeon->ID);

        // LFGDungeons.dbc carries 49 rows with mapID == -1
        // (LFD_CATA_ANALYSIS.md section 4.2 / Phase 5's trap). Guard here
        // rather than casting -1 to uint32 and handing 0xFFFFFFFF to
        // GetDungeonFinderRequirements.
        DungeonFinderRequirements const* req = (dungeon->mapID >= 0)
            ? sObjectMgr.GetDungeonFinderRequirements(
                  (uint32)dungeon->mapID, dungeon->difficulty)
            : NULL;

        if (req)
        {
            in.hasRequirements = true;
            in.requiredItemLevel = req->minItemLevel;
            in.currentItemLevel = currentItemLevel;

            in.requiresAchievement = req->achievement != 0;
            in.hasAchievement = !in.requiresAchievement
                || plr->GetAchievementMgr().HasAchievement(req->achievement);

            in.requiresAllianceQuest = req->allianceQuestId != 0;
            in.hasCompletedAllianceQuest = !in.requiresAllianceQuest
                || plr->GetQuestRewardStatus(req->allianceQuestId);

            in.requiresHordeQuest = req->hordeQuestId != 0;
            in.hasCompletedHordeQuest = !in.requiresHordeQuest
                || plr->GetQuestRewardStatus(req->hordeQuestId);

            in.requiresItem1 = req->item != 0;
            in.hasItem1 = !in.requiresItem1
                || plr->HasItemCount(req->item, 1);

            in.requiresItem2 = req->item2 != 0;
            in.hasItem2 = !in.requiresItem2
                || plr->HasItemCount(req->item2, 1);
        }

        LFGLockReason::Result result = LFGLockReason::Compute(in);
        if (result.reason != LFGLockReason::REASON_NONE)
        {
            LFGLockedDungeon locked;
            locked.slot = dungeon->Entry();
            locked.reason = result.reason;
            locked.subReason1 = result.subReason1;
            locked.subReason2 = result.subReason2;
            lockList.push_back(locked);
        }
    }

    return lockList;
}

void LFGMgr::CountAssignedRoles(roleMap const& roles, uint8& tanks,
                                uint8& healers, uint8& dps)
{
    std::vector<uint8> masks;
    masks.reserve(roles.size());
    for (roleMap::const_iterator it = roles.begin(); it != roles.end(); ++it)
    {
        masks.push_back(it->second);
    }

    LFGRoleAssignment::Counts const seated = LFGRoleAssignment::Assign(
        masks, NORMAL_TANK_OR_HEALER_COUNT, NORMAL_TANK_OR_HEALER_COUNT,
        NORMAL_DAMAGE_COUNT);

    tanks = seated.tanks;
    healers = seated.healers;
    dps = seated.dps;
}

void LFGMgr::AssignRoles(roleMap const& selected, roleMap& assigned)
{
    std::vector<ObjectGuid> guids;
    std::vector<uint8> masks;
    guids.reserve(selected.size());
    masks.reserve(selected.size());

    for (roleMap::const_iterator it = selected.begin(); it != selected.end(); ++it)
    {
        guids.push_back(it->first);
        masks.push_back(it->second);
    }

    std::vector<uint8> seats;
    LFGRoleAssignment::Assign(masks, NORMAL_TANK_OR_HEALER_COUNT,
                              NORMAL_TANK_OR_HEALER_COUNT, NORMAL_DAMAGE_COUNT,
                              &seats);

    assigned.clear();
    for (size_t i = 0; i < guids.size(); ++i)
    {
        // Carry the leader bit through -- the client reads it for the crown.
        uint8 leaderBit = masks[i] & PLAYER_ROLE_LEADER;
        assigned[guids[i]] = uint8(seats[i] | leaderBit);
    }
}

void LFGMgr::UpdateNeededRoles(ObjectGuid guid, LFGPlayers* information)
{
    if (information->dungeonList.empty())
    {
        return;
    }

    uint8 tankCount = 0, dpsCount = 0, healCount = 0;
    CountAssignedRoles(information->currentRoles, tankCount, healCount, dpsCount);

    std::set<uint32>::iterator itr = information->dungeonList.begin();

    // check dungeon type for max of each role [normal heroic etc.]
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(*itr);
    if (dungeon)
    {
        // All 5-man content (normal and heroic alike) wants 1/1/3; raids are
        // Phase 10 and never reach the queue in this phase.
        information->neededTanks = NORMAL_TANK_OR_HEALER_COUNT - tankCount;
        information->neededHealers = NORMAL_TANK_OR_HEALER_COUNT - healCount;
        information->neededDps = NORMAL_DAMAGE_COUNT - dpsCount;
    }

    m_playerData[guid] = *information;
}

void LFGMgr::AddToQueue(ObjectGuid guid)
{
    LFGPlayers* information = GetPlayerOrPartyData(guid);
    if (!information)
    {
        return;
    }

    // This will be necessary for finding matches in the queue
    UpdateNeededRoles(guid, information);

    // put info into wait time maps for starters, keyed by what each member
    // SELECTED (the random entry for random queuers) -- wait stats answer
    // "how long for Random Cataclysm Heroic", not per concrete dungeon.
    for (roleMap::iterator it = information->currentRoles.begin(); it != information->currentRoles.end(); ++it)
    {
        LFGPlayerStatus const memberStatus = GetPlayerStatus(it->first);
        AddToWaitMap(it->second, memberStatus.dungeonList.empty()
                     ? information->dungeonList : memberStatus.dungeonList);
    }

    // just in case someone's already been in the queue.
    queueSet::iterator qItr = m_queueSet.find(guid);
    if (qItr == m_queueSet.end())
    {
        m_queueSet.insert(guid);
    }

    // A complete party (1/1/3 seated) skips matching entirely -- this is
    // how a full premade proposes the moment its role check passes.
    if ((information->neededTanks == 0) && (information->neededHealers == 0) &&
        (information->neededDps == 0))
    {
        SendDungeonProposal(guid, information);
    }
}

void LFGMgr::RemoveFromQueue(ObjectGuid guid)
{
    m_queueSet.erase(guid);

    //todo - might need to implement a removefromwaitmap function
}

/// Counts one more player waiting for each of the given dungeons.
static void AddWaiterTo(waitTimeMap& target, std::set<uint32> const& dungeons)
{
    for (std::set<uint32>::const_iterator itr = dungeons.begin();
         itr != dungeons.end(); ++itr)
    {
        waitTimeMap::iterator it = target.find(*itr);
        if (it != target.end())
        {
            // Increment current player count by one
            ++it->second.playerCount;
        }
        else
        {
            LFGWait waitInfo(QUEUE_DEFAULT_TIME, -1, 1, false);
            target[*itr] = waitInfo;
        }
    }
}

void LFGMgr::AddToWaitMap(uint8 role, std::set<uint32> dungeons)
{
    uint8 withoutLeader = role;
    withoutLeader &= ~PLAYER_ROLE_LEADER;

    // Someone queued for several roles is waiting in each of those queues, so
    // test the bits instead of matching the mask exactly.
    if (withoutLeader & PLAYER_ROLE_TANK)
    {
        AddWaiterTo(m_tankWaitTime, dungeons);
    }
    if (withoutLeader & PLAYER_ROLE_HEALER)
    {
        AddWaiterTo(m_healerWaitTime, dungeons);
    }
    if (withoutLeader & PLAYER_ROLE_DAMAGE)
    {
        AddWaiterTo(m_dpsWaitTime, dungeons);
    }

    // insert the average time regardless of role
    for (std::set<uint32>::iterator itr = dungeons.begin(); itr != dungeons.end(); ++itr)
    {
        waitTimeMap::iterator it = m_avgWaitTime.find(*itr);
        if (it != m_avgWaitTime.end())
        {
            ++it->second.playerCount;
        }
        else
        {
            LFGWait waitInfo(QUEUE_DEFAULT_TIME, -1, 1, false);
            m_avgWaitTime[*itr] = waitInfo;
        }
    }
}

void LFGMgr::FindQueueMatches()
{
    // Fetch information on all the queued players/groups. Iterate a snapshot:
    // a merge retires the absorbed entry from m_queueSet, which would
    // invalidate a live iterator here.
    queueSet const snapshot = m_queueSet;
    for (queueSet::const_iterator itr = snapshot.begin(); itr != snapshot.end(); ++itr)
    {
        FindSpecificQueueMatches(*itr);
    }
}

void LFGMgr::FindSpecificQueueMatches(ObjectGuid guid)
{
    uint64 rawGuid = guid.GetRawValue();
    LFGPlayers* queueInfo = GetPlayerOrPartyData(guid);
    if (queueInfo)
    {
        // compare to everyone else in queue for compatibility
        // after a match is found call UpdateNeededRoles
        // Use the roleMap to store player guid/role information; merge into queueInfo struct & delete other struct/map entry
        // Snapshot for the same reason as FindQueueMatches: MergeGroups
        // retires the absorbed guid from m_queueSet.
        queueSet const candidates = m_queueSet;
        for (queueSet::const_iterator itr = candidates.begin(); itr != candidates.end(); ++itr)
        {
            if (*itr == guid)
            {
                continue;
            }

            LFGPlayers* matchInfo = GetPlayerOrPartyData(*itr);
            if (matchInfo)
            {
                // 1. iterate through queueInfo's dungeon set and search the matchInfo for a matching entry.
                // 2. if an(y) entry is found, great and proceed!
                // 2a. if an entry is found and the amounts of players-to-roles are compatible, make
                //     a new map of only the inter-compatible dungeons and use that if the other checks pass
                // 3. Regardless of outcome, after the end of calculations send a LFGQueueStatus packet
                bool fullyCompatible = false;
                std::set<uint32> compatibleDungeons;

                for (std::set<uint32>::iterator dItr = matchInfo->dungeonList.begin(); dItr != matchInfo->dungeonList.end(); ++dItr)
                {
                    if (queueInfo->dungeonList.find(*dItr) != queueInfo->dungeonList.end())
                    {
                        compatibleDungeons.insert(*dItr);
                    }
                }

                if (!compatibleDungeons.empty())
                {
                    // check for player / role count and also team compatibility
                    // if function returns true, then merge groups into one
                    if (RoleMapsAreCompatible(queueInfo, matchInfo) && MatchesAreOfSameTeam(queueInfo, matchInfo))
                    {
                        MergeGroups(guid, *itr, compatibleDungeons);
                    }
                }
            }
        }
    }
}

bool LFGMgr::RoleMapsAreCompatible(LFGPlayers* groupOne, LFGPlayers* groupTwo)
{
    // When this is called we already know that the dungeons match, so just focus on roles
    // compare: neededX(role) from each struct and the amount of people per role in the roleMap
    if ((groupOne->currentRoles.size() + groupTwo->currentRoles.size()) > NORMAL_TOTAL_ROLE_COUNT)
    {
        return false;
    }

    // Seat the two selections together rather than adding up what each side
    // separately decided it needed. Someone who picked several roles is only
    // pinned to one of them once the rest of the group is known, so comparing
    // the pre-computed counts would let a flexible player squat the tank slot
    // and lock out a dedicated tank who could still have joined.
    roleMap combined = groupOne->currentRoles;
    for (roleMap::const_iterator it = groupTwo->currentRoles.begin();
         it != groupTwo->currentRoles.end(); ++it)
    {
        combined[it->first] = it->second;
    }

    uint8 tanks = 0, healers = 0, dps = 0;
    CountAssignedRoles(combined, tanks, healers, dps);

    // Compatible only when everyone still gets a seat; anyone left over means
    // the two sides are carrying roles the group cannot use.
    return (uint32(tanks) + uint32(healers) + uint32(dps)) == combined.size();
}

bool LFGMgr::MatchesAreOfSameTeam(LFGPlayers* groupOne, LFGPlayers* groupTwo)
{
    // we should safely be able to compare any two players from each struct to
    // determine compatibility
    roleMap::iterator it1 = groupOne->currentRoles.begin();
    roleMap::iterator it2 = groupTwo->currentRoles.begin();

    // now we find the players from the maps
    Player* pPlayer1 = sPlayerRegistry.Find(it1->first);
    Player* pPlayer2 = sPlayerRegistry.Find(it2->first);

    if (!pPlayer1 || !pPlayer2)
    {
        return false;
    }

    // todo: disable this if a config option is set
    if (pPlayer1->GetTeamId() == pPlayer2->GetTeamId())
    {
        return true;
    }

    return false;
}

void LFGMgr::MergeGroups(ObjectGuid guidOne, ObjectGuid guidTwo, std::set<uint32> compatibleDungeons)
{
    // merge into the entry for rawGuidOne, then see if they are
    // able to enter the dungeon at this point or not
    LFGPlayers* mainGroup   = GetPlayerOrPartyData(guidOne);
    LFGPlayers* bufferGroup = GetPlayerOrPartyData(guidTwo);

    if (!mainGroup || !bufferGroup)
    {
        return;
    }

    // update the dungeon selection with the compatible ones
    mainGroup->dungeonList.clear();
    mainGroup->dungeonList = compatibleDungeons;

    // move players / roles into a single roleMap
    for (roleMap::iterator it = bufferGroup->currentRoles.begin(); it != bufferGroup->currentRoles.end(); ++it)
    {
        mainGroup->currentRoles[it->first] = it->second;
    }

    // update the role count / needed role info
    UpdateNeededRoles(guidOne, mainGroup);

    // being safe
    //mainGroup = GetPlayerOrPartyData(rawGuidOne);

    // Then do the following:
    if ((mainGroup->neededTanks == 0) && (mainGroup->neededHealers == 0) &&
        (mainGroup->neededDps == 0))
    {
        SendDungeonProposal(guidOne, mainGroup);
    }

    // guidTwo's players now live in guidOne's entry, so retire its queue slot
    // as well -- leaving it behind makes every later tick look it up, find no
    // data and skip it.
    m_playerData.erase(guidTwo);
    m_queueSet.erase(guidTwo);
}

void LFGMgr::SendQueueStatus()
{
    for (queueSet::iterator itr = m_queueSet.begin(); itr != m_queueSet.end(); ++itr)
    {
        SendQueueStatusFor(*itr);
    }
}

void LFGMgr::SendQueueStatusFor(ObjectGuid guid)
{
    LFGPlayers* queueInfo = GetPlayerOrPartyData(guid);
    if (!queueInfo || queueInfo->currentState != LFG_STATE_QUEUED ||
        queueInfo->dungeonList.empty())
    {
        return;
    }

    time_t timeNow = time(NULL);

    for (roleMap::iterator rItr = queueInfo->currentRoles.begin();
         rItr != queueInfo->currentRoles.end(); ++rItr)
    {
        Player* pPlayer = sPlayerRegistry.Find(rItr->first);
        if (!pPlayer)
        {
            continue;
        }

        LFGPlayerStatus const memberStatus = GetPlayerStatus(rItr->first);
        uint32 dungeonId = memberStatus.dungeonList.empty()
            ? *queueInfo->dungeonList.begin() : *memberStatus.dungeonList.begin();

        LFGPackets::QueueStatus status;
        status.ticket = memberStatus.ticket;
        status.slot = GetDungeonEntry(dungeonId);
        status.queuedTime = uint32(timeNow - queueInfo->joinedTime);
        status.lastNeeded[0] = queueInfo->neededTanks;
        status.lastNeeded[1] = queueInfo->neededHealers;
        status.lastNeeded[2] = queueInfo->neededDps;
        status.avgWaitTimeByRole[0] = m_tankWaitTime[dungeonId].time;
        status.avgWaitTimeByRole[1] = m_healerWaitTime[dungeonId].time;
        status.avgWaitTimeByRole[2] = m_dpsWaitTime[dungeonId].time;
        status.avgWaitTime = m_avgWaitTime[dungeonId].time;

        // strip leader flag from role
        uint8 withoutLeader = rItr->second;
        withoutLeader &= ~PLAYER_ROLE_LEADER;
        switch (withoutLeader)
        {
            case PLAYER_ROLE_TANK:
                status.avgWaitTimeMe = status.avgWaitTimeByRole[0];
                break;
            case PLAYER_ROLE_HEALER:
                status.avgWaitTimeMe = status.avgWaitTimeByRole[1];
                break;
            case PLAYER_ROLE_DAMAGE:
                status.avgWaitTimeMe = status.avgWaitTimeByRole[2];
                break;
            default:
                status.avgWaitTimeMe = status.avgWaitTime;
                break;
        }

        pPlayer->GetSession()->SendLfgQueueStatus(status);
    }
}

uint32 LFGMgr::GetDungeonEntry(uint32 ID) const
{
    LfgDungeonsEntry const* dungeon = sLfgDungeonsStore.LookupEntry(ID);
    if (dungeon)
    {
        return dungeon->Entry();
    }
    else
    {
        return 0;
    }
}

void LFGMgr::SendStatusUpdate(Player* plr)
{
    ObjectGuid guid = plr->GetObjectGuid();
    LFGPlayerStatus status = GetPlayerStatus(guid);

    if (status.state == LFG_STATE_NONE)
    {
        return;                          // not using LFG
    }

    status.updateType = LFG_UPDATE_STATUS;

    LFGPlayerStatus cleared = status;
    cleared.dungeonList.clear();

    if (plr->GetGroup())
    {
        SendLfgUpdate(guid, status, true);
        SendLfgUpdate(guid, cleared, false);
    }
    else
    {
        SendLfgUpdate(guid, status, false);
        SendLfgUpdate(guid, cleared, true);
    }
}

bool LFGMgr::CanPerformSelectedRoles(uint8 playerClass, uint8 roles)
{
    if ((roles & PLAYER_ROLE_TANK) &&
        playerClass != CLASS_WARRIOR && playerClass != CLASS_PALADIN &&
        playerClass != CLASS_DEATH_KNIGHT && playerClass != CLASS_DRUID)
    {
        return false;
    }

    if ((roles & PLAYER_ROLE_HEALER) &&
        playerClass != CLASS_PALADIN && playerClass != CLASS_PRIEST &&
        playerClass != CLASS_SHAMAN && playerClass != CLASS_DRUID)
    {
        return false;
    }

    return true;
}

