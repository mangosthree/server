/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file BattleGroundMgr.cpp
 * @brief Implementation of the battleground manager and queue system.
 *
 * This file contains the implementation of the BattleGroundMgr singleton class and
 * the BattleGroundQueue class, which handle:
 * - Battleground instance creation and management
 * - Player queue management and matching
 * - Team balancing for battleground invitations
 * - Average wait time calculations
 * - Bracket-based queue organization
 * - Premade group matching
 */

#include "Common.h"
#include "SharedDefines.h"
#include "Player.h"
#include "BattleGroundMgr.h"
#include "BattleGroundAV.h"
#include "BattleGroundAB.h"
#include "BattleGroundEY.h"
#include "BattleGroundWS.h"
#include "BattleGroundNA.h"
#include "BattleGroundBE.h"
#include "BattleGroundAA.h"
#include "BattleGroundRL.h"
#include "BattleGroundSA.h"
#include "BattleGroundDS.h"
#include "BattleGroundRV.h"
#include "BattleGroundIC.h"
#include "BattleGroundRB.h"
#include "MapManager.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "ProgressBar.h"
#include "Chat.h"
#include "ArenaTeam.h"
#include "World.h"
#include "WorldPacket.h"
#include "GameEventMgr.h"
#include "DisableMgr.h"
#include "GameTime.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

#include "Policies/Singleton.h"

INSTANTIATE_SINGLETON_1(BattleGroundMgr);


/*********************************************************/
/***            BATTLEGROUND MANAGER                   ***/
/*********************************************************/

/**
 * @brief Constructor for BattleGroundMgr.
 *
 * Initializes all battleground containers and sets testing mode to false.
 */
BattleGroundMgr::BattleGroundMgr() : m_AutoDistributionTimeChecker(0), m_ArenaTesting(false)
{
    for (uint8 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
    {
        m_BattleGrounds[i].clear();
    }
    m_NextRatingDiscardUpdate = sWorld.getConfig(CONFIG_UINT32_ARENA_RATING_DISCARD_TIMER);
    m_Testing = false;
}

/**
 * @brief Destructor for BattleGroundMgr.
 *
 * Cleans up all active and template battlegrounds.
 */
BattleGroundMgr::~BattleGroundMgr()
{
    DeleteAllBattleGrounds();
}

/**
 * @brief Deletes all battleground instances.
 *
 * Safely removes all active battlegrounds and template battlegrounds from memory.
 * This includes template battlegrounds that are only used as templates for creating instances.
 */
void BattleGroundMgr::DeleteAllBattleGrounds()
{
    // will also delete template bgs:
    for (uint8 i = BATTLEGROUND_TYPE_NONE; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
    {
        for (BattleGroundSet::iterator itr = m_BattleGrounds[i].begin(); itr != m_BattleGrounds[i].end();)
        {
            BattleGround* bg = itr->second;
            ++itr;                                          // step from invalidate iterator pos in result element remove in ~BattleGround call
            delete bg;
        }
    }
}

/**
 * @brief Updates all active battlegrounds and processes queue operations.
 *
 * Performs the main update loop for all active battleground instances, processes
 * scheduled queue updates based on the update scheduler, and removes finished
 * battlegrounds from memory. Called once per world tick.
 *
 * @param diff The time elapsed since the last update in milliseconds (unused).
 */
void BattleGroundMgr::Update(uint32 diff)
{
    // update scheduled queues
    if (!m_QueueUpdateScheduler.empty())
    {
        std::vector<uint64> scheduled;
        {
            // create mutex
            // ACE_Guard<ACE_Thread_Mutex> guard(SchedulerLock);
            // copy vector and clear the other
            scheduled = std::vector<uint64>(m_QueueUpdateScheduler);
            m_QueueUpdateScheduler.clear();
            // release lock
        }

        for (uint8 i = 0; i < scheduled.size(); ++i)
        {
            uint32 arenaRating = scheduled[i] >> 32;
            ArenaType arenaType = ArenaType(scheduled[i] >> 24 & 255);
            BattleGroundQueueTypeId bgQueueTypeId = BattleGroundQueueTypeId(scheduled[i] >> 16 & 255);
            BattleGroundTypeId bgTypeId = BattleGroundTypeId((scheduled[i] >> 8) & 255);
            BattleGroundBracketId bracket_id = BattleGroundBracketId(scheduled[i] & 255);
            m_BattleGroundQueues[bgQueueTypeId].Update(bgTypeId, bracket_id, arenaType, arenaRating > 0, arenaRating);
        }
    }

    // if rating difference counts, maybe force-update queues
    if (sWorld.getConfig(CONFIG_UINT32_ARENA_MAX_RATING_DIFFERENCE) && sWorld.getConfig(CONFIG_UINT32_ARENA_RATING_DISCARD_TIMER))
    {
        // it's time to force update
        if (m_NextRatingDiscardUpdate < diff)
        {
            // forced update for rated arenas (scan all, but skipped non rated)
            DEBUG_LOG("BattleGroundMgr: UPDATING ARENA QUEUES");
            for (uint8 qtype = BATTLEGROUND_QUEUE_2v2; qtype <= BATTLEGROUND_QUEUE_5v5; ++qtype)
            {
                for (uint8 bracket = BG_BRACKET_ID_FIRST; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
                {
                    m_BattleGroundQueues[qtype].Update(
                        BATTLEGROUND_AA, BattleGroundBracketId(bracket),
                        BattleGroundMgr::BGArenaType(BattleGroundQueueTypeId(qtype)), true, 0);
                }
            }

            m_NextRatingDiscardUpdate = sWorld.getConfig(CONFIG_UINT32_ARENA_RATING_DISCARD_TIMER);
        }
        else
        {
            m_NextRatingDiscardUpdate -= diff;
        }
    }
}

/**
 * @brief Builds a battlefield status packet for sending to the player.
 *
 * Constructs the network packet for SMSG_BATTLEFIELD_STATUS that informs the player
 * of their queue status, position, estimated wait time, and other relevant information.
 * Handles different status types: waiting in queue, invited to join, and in progress.
 *
 * @param data Pointer to the WorldPacket to write data to.
 * @param bg Pointer to the battleground (may be NULL for status clear).
 * @param player
 * @param QueueSlot The queue slot index (0-2, player can be in multiple queues).
 * @param StatusID The status identifier (0=clear, STATUS_WAIT_QUEUE, STATUS_WAIT_JOIN, STATUS_IN_PROGRESS).
 * @param Time1 Status-specific time value (wait time, invitation timeout, or auto-leave time).
 * @param Time2 Secondary time value (queue time or elapsed battle time).
 * @param ArenaType
 */
void BattleGroundMgr::BuildBattleGroundStatusPacket(WorldPacket* data, BattleGround* bg, Player* player, uint8 QueueSlot, uint8 StatusID, uint32 Time1, uint32 Time2, ArenaType arenatype)
{
    // we can be in 2 queues in same time...
    if (!bg)
    {
        StatusID = STATUS_NONE;
    }

    ObjectGuid playerGuid = player->GetObjectGuid();
    ObjectGuid bgGuid = bg ? bg->GetObjectGuid() : ObjectGuid();

    DEBUG_LOG("BattleGroundMgr::BuildBattleGroundStatusPacket bgGuid %s, playerguid %s, queueSlot %u, "
        "statusid %u time1 %u time2 %u arenatype %u",
        bgGuid.GetString().c_str(), playerGuid.GetString().c_str(), QueueSlot, StatusID, Time1, Time2, arenatype);

    switch (StatusID)
    {
        case STATUS_NONE:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS);

            data->WriteGuidMask<0, 4, 7, 1, 6, 3, 5, 2>(playerGuid);
            data->WriteGuidBytes<5, 6, 7, 2>(playerGuid);
            *data << uint32(QueueSlot);                 // not queue slot
            data->WriteGuidBytes<3, 1>(playerGuid);
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(bg->GetStartTime());
            data->WriteGuidBytes<0, 4>(playerGuid);
            break;
        }
        case STATUS_WAIT_QUEUE:                             // status_in_queue
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS_QUEUED);

            data->WriteGuidMask<3, 0>(playerGuid);
            data->WriteGuidMask<3>(bgGuid);
            data->WriteGuidMask<2>(playerGuid);
            data->WriteBit(1);                          // eligible in queue
            data->WriteBit(0);                          // Join Failed
            data->WriteGuidMask<2>(bgGuid);
            data->WriteGuidMask<1>(playerGuid);
            data->WriteGuidMask<0, 6, 4>(bgGuid);
            data->WriteGuidMask<6, 7>(playerGuid);
            data->WriteGuidMask<7, 5>(bgGuid);
            data->WriteGuidMask<4, 5>(playerGuid);
            data->WriteBit(bg->isRated());              // is rated bg or arena. if israted => cant exit (wtf?)
            data->WriteBit(0);                          // Waiting On Other Activity 4
            data->WriteGuidMask<1>(bgGuid);

            data->WriteGuidBytes<0>(playerGuid);
            *data << uint32(QueueSlot);                 // unk 5 - received in battlefield port then
            data->WriteGuidBytes<5>(bgGuid);
            data->WriteGuidBytes<3>(playerGuid);
            *data << uint32(Time1);                     // Estimated Wait Time 6 time1
            data->WriteGuidBytes<7, 1, 2>(bgGuid);
            *data << uint8(0);                          // unk param
            data->WriteGuidBytes<4>(bgGuid);
            data->WriteGuidBytes<2>(playerGuid);
            //if (...)
            //    *data << uint8(playerCount);          // player count (XvX) > 0 && rated: Rated XvX, !rated: Skirmish XvX. if ==0 >= Bg type
            //else
                *data << uint8(arenatype);
            data->WriteGuidBytes<6>(bgGuid);
            data->WriteGuidBytes<7>(playerGuid);
            data->WriteGuidBytes<3>(bgGuid);
            data->WriteGuidBytes<6>(playerGuid);
            data->WriteGuidBytes<0>(bgGuid);
            *data << uint32(0);                         // Time
            *data << uint32(QueueSlot);                 // queueslot
            *data << uint8(0);                          // maxlevel? seen 85 only in sniffs
            *data << uint32(Time2);                     // unk 10 Time since started time2
            data->WriteGuidBytes<1, 5>(playerGuid);
            *data << uint32(bg->GetClientInstanceID()); // client instance id
            data->WriteGuidBytes<4>(playerGuid);
            break;
        }
        case STATUS_WAIT_JOIN:                              // status_invite
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS_NEEDCONFIRMATION, 44);

            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID
            *data << uint32(Time1);                     // Time until closed
            //*data << uint8(bg->GetPlayersCountByTeam(bg->GetPlayerTeam(playerGuid)));
            *data << uint8(0);                          // unk 0
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(0);                         // Time
            *data << uint8(0);                          // Max Level
            *data << uint32(QueueSlot);                 // not queueslot
            *data << uint32(bg->GetMapId());            // Map Id
            //if (...)
            //    *data << uint8(playerCount);          // player count (XvX) > 0 && rated: Rated XvX, !rated: Skirmish XvX. if ==0 >= Bg type
            //else
                *data << uint8(arenatype);

            data->WriteGuidMask<5, 2, 1>(playerGuid);
            data->WriteGuidMask<2>(bgGuid);
            data->WriteGuidMask<4>(playerGuid);
            data->WriteGuidMask<6, 3>(bgGuid);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteGuidMask<7, 3>(playerGuid);
            data->WriteGuidMask<7, 0, 4>(bgGuid);
            data->WriteGuidMask<6>(playerGuid);
            data->WriteGuidMask<5, 1>(bgGuid);
            data->WriteGuidMask<0>(playerGuid);

            data->WriteGuidBytes<6, 5, 7, 2>(bgGuid);
            data->WriteGuidBytes<0, 7>(playerGuid);
            data->WriteGuidBytes<4>(bgGuid);
            data->WriteGuidBytes<1>(playerGuid);
            data->WriteGuidBytes<0>(bgGuid);
            data->WriteGuidBytes<4>(playerGuid);
            data->WriteGuidBytes<1>(bgGuid);
            data->WriteGuidBytes<5>(playerGuid);
            data->WriteGuidBytes<3>(bgGuid);
            data->WriteGuidBytes<6, 2, 3>(playerGuid);
            break;
        }
        case STATUS_IN_PROGRESS:
        {
            data->Initialize(SMSG_BATTLEFIELD_STATUS_ACTIVE, 49);

            data->WriteGuidMask<2, 7>(playerGuid);
            data->WriteGuidMask<7, 1>(bgGuid);
            data->WriteGuidMask<5>(playerGuid);
            data->WriteBit(bg->GetPlayerTeam(playerGuid) == ALLIANCE);  // Bg faction bit
            data->WriteGuidMask<0>(bgGuid);
            data->WriteGuidMask<1>(playerGuid);
            data->WriteGuidMask<3>(bgGuid);
            data->WriteGuidMask<6>(playerGuid);
            data->WriteGuidMask<5>(bgGuid);
            data->WriteBit(bg->isRated());              // Unk Bit 64
            data->WriteGuidMask<4>(playerGuid);
            data->WriteGuidMask<6, 4, 2>(bgGuid);
            data->WriteGuidMask<3, 0>(playerGuid);

            data->WriteGuidBytes<4, 5>(bgGuid);
            data->WriteGuidBytes<5>(playerGuid);
            data->WriteGuidBytes<1, 6, 3, 7>(bgGuid);
            data->WriteGuidBytes<6>(playerGuid);

            *data << uint32(0);                         // Time
            *data << uint8(0);                          // unk

            data->WriteGuidBytes<4, 1>(playerGuid);

            *data << uint32(QueueSlot);                 // Queue slot
            //if (...)
            //    *data << uint8(playerCount);          // player count (XvX) > 0 && rated: Rated XvX, !rated: Skirmish XvX. if ==0 >= Bg type
            //else
                *data << uint8(arenatype);

            *data << uint32(QueueSlot);                 // not queue slot
            *data << uint32(bg->GetMapId());            // Map Id
            *data << uint8(0);                          // Max Level? seen 85
            *data << uint32(Time2);                     // Time since started

            data->WriteGuidBytes<2>(playerGuid);

            *data << uint32(Time1);                     // Time until closed

            data->WriteGuidBytes<0, 3>(playerGuid);
            data->WriteGuidBytes<2>(bgGuid);

            *data << uint32(bg->GetClientInstanceID()); // Client Instance ID

            data->WriteGuidBytes<0>(bgGuid);
            data->WriteGuidBytes<7>(playerGuid);
            break;
        }
        case STATUS_WAIT_LEAVE:
        {
            // not used currently and not checked
            data->Initialize(SMSG_BATTLEFIELD_STATUS_WAITFORGROUPS, 48);

            *data << uint8(0);                          // unk
            *data << uint32(QueueSlot);                 // not queueSlot
            *data << uint32(QueueSlot);                 // Queue slot
            *data << uint32(bg->GetEndTime());          // Time until closed
            *data << uint32(0);                         // unk
            *data << uint8(0);                          // unk
            *data << uint8(0);                          // unk
            *data << uint8(bg->GetMinLevel());          // Min Level
            *data << uint8(0);                          // unk
            *data << uint8(0);                          // unk
            *data << uint32(bg->GetMapId());            // Map Id
            *data << uint32(0);                         // Time
            *data << uint8(0);                          // unk

            data->WriteGuidMask<0, 1, 7>(bgGuid);
            data->WriteGuidMask<7, 0>(playerGuid);
            data->WriteGuidMask<4>(bgGuid);
            data->WriteGuidMask<6, 2, 3>(playerGuid);
            data->WriteGuidMask<3>(bgGuid);
            data->WriteGuidMask<4>(playerGuid);
            data->WriteGuidMask<5>(bgGuid);
            data->WriteGuidMask<5>(playerGuid);
            data->WriteGuidMask<2>(bgGuid);
            data->WriteBit(bg->isRated());              // Is Rated
            data->WriteGuidMask<1>(playerGuid);
            data->WriteGuidMask<6>(bgGuid);

            data->WriteGuidBytes<0>(playerGuid);
            data->WriteGuidBytes<4>(bgGuid);
            data->WriteGuidBytes<3>(playerGuid);
            data->WriteGuidBytes<1, 0, 2>(bgGuid);
            data->WriteGuidBytes<2>(playerGuid);
            data->WriteGuidBytes<7>(bgGuid);
            data->WriteGuidBytes<1, 6>(playerGuid);
            data->WriteGuidBytes<6, 5>(bgGuid);
            data->WriteGuidBytes<5, 4, 7>(playerGuid);
            data->WriteGuidBytes<3>(bgGuid);
            break;
        }
        default:
            sLog.outError("Unknown BG status %u!", StatusID);
            break;
    }
}

/**
 * @brief Builds a PvP log data packet with player statistics.
 *
 * Constructs the network packet for MSG_PVP_LOG_DATA that contains the battleground
 * statistics for all players, including scores, kills, deaths, and battleground-specific
 * objective data. Indicates whether the battleground has finished.
 *
 * @param data Pointer to the WorldPacket to write data to.
 * @param bg Pointer to the battleground instance.
 */
void BattleGroundMgr::BuildPvpLogDataPacket(WorldPacket* data, BattleGround* bg)
{
    ByteBuffer buffer;

    // last check on 4.3.4
    data->Initialize(SMSG_PVP_LOG_DATA, (1 + 1 + 4 + 40 * bg->GetPlayerScoresSize()));
    data->WriteBit(bg->isArena());
    data->WriteBit(bg->isRated());

    if (bg->isArena())
    {
        // it seems this must be according to BG_WINNER_A/H and _NOT_ BG_TEAM_A/H
        for (int8 i = 0; i < PVP_TEAM_COUNT; ++i)
        {
            if (ArenaTeam* at = sObjectMgr.GetArenaTeamById(bg->m_ArenaTeamIds[i]))
            {
                data->WriteBits(at->GetName().length(), 8);
            }
            else
            {
                data->WriteBits(0, 8);
            }
        }
    }

    data->WriteBits(bg->GetPlayerScoresSize(), 21);

    for (BattleGround::BattleGroundScoreMap::const_iterator itr = bg->GetPlayerScoresBegin(); itr != bg->GetPlayerScoresEnd(); ++itr)
    {
        ObjectGuid memberGuid = itr->first;
        Player* player = sObjectMgr.GetPlayer(itr->first);
        const BattleGroundScore* score = itr->second;

        data->WriteBit(0);                  // unk1
        data->WriteBit(0);                  // unk2
        data->WriteGuidMask<2>(memberGuid);
        data->WriteBit(!bg->isArena());
        data->WriteBit(0);                  // unk4
        data->WriteBit(0);                  // unk5
        data->WriteBit(0);                  // unk6
        data->WriteGuidMask<3, 0, 5, 1, 6>(memberGuid);
        Team team = bg->GetPlayerTeam(itr->first);
        if (!team && player)
        {
            team = player->GetTeam();
        }
        data->WriteBit(team == ALLIANCE);   // unk7
        data->WriteGuidMask<7>(memberGuid);

        buffer << uint32(score->HealingDone);         // healing done
        buffer << uint32(score->DamageDone);          // damage done

        if (!bg->isArena())
        {
            buffer << uint32(score->BonusHonor);
            buffer << uint32(score->Deaths);
            buffer << uint32(score->HonorableKills);
        }

        buffer.WriteGuidBytes<4>(memberGuid);
        buffer << uint32(score->KillingBlows);
        // if (unk5) << uint32() unk
        buffer.WriteGuidBytes<5>(memberGuid);
        // if (unk6) << uint32() unk
        // if (unk2) << uint32() unk
        buffer.WriteGuidBytes<1, 6>(memberGuid);
        // TODO: store this in player score
        if (player)
        {
            buffer << uint32(player->GetPrimaryTalentTree(player->GetActiveSpec()));
        }
        else
        {
            buffer << uint32(0);
        }

        switch (bg->GetTypeID())                            // battleground specific things
        {
            case BATTLEGROUND_AV:
                data->WriteBits(5, 24);                     // count of next fields
                buffer << uint32(((BattleGroundAVScore*)score)->GraveyardsAssaulted); // GraveyardsAssaulted
                buffer << uint32(((BattleGroundAVScore*)score)->GraveyardsDefended);  // GraveyardsDefended
                buffer << uint32(((BattleGroundAVScore*)score)->TowersAssaulted);     // TowersAssaulted
                buffer << uint32(((BattleGroundAVScore*)score)->TowersDefended);      // TowersDefended
                buffer << uint32(((BattleGroundAVScore*)score)->SecondaryObjectives); // SecondaryObjectives - free some of the Lieutnants
                break;
            case BATTLEGROUND_WS:
                data->WriteBits(2, 24);                     // count of next fields
                buffer << uint32(((BattleGroundWGScore*)score)->FlagCaptures);        // flag captures
                buffer << uint32(((BattleGroundWGScore*)score)->FlagReturns);         // flag returns
                break;
            case BATTLEGROUND_AB:
                data->WriteBits(2, 24);                     // count of next fields
                buffer << uint32(((BattleGroundABScore*)score)->BasesAssaulted);      // bases asssulted
                buffer << uint32(((BattleGroundABScore*)score)->BasesDefended);       // bases defended
                break;
            case BATTLEGROUND_EY:
                data->WriteBits(1, 24);                     // count of next fields
                buffer << uint32(((BattleGroundEYScore*)score)->FlagCaptures);        // flag captures
                break;
            case BATTLEGROUND_NA:
            case BATTLEGROUND_BE:
            case BATTLEGROUND_AA:
            case BATTLEGROUND_RL:
            case BATTLEGROUND_SA:                           // wotlk
            case BATTLEGROUND_DS:                           // wotlk
            case BATTLEGROUND_RV:                           // wotlk
            case BATTLEGROUND_IC:                           // wotlk
            case BATTLEGROUND_RB:                           // wotlk
                data->WriteBits(0, 24);                     // count of next fields
                break;
            default:
                sLog.outError("Unhandled SMSG_PVP_LOG_DATA for BG id %u", bg->GetTypeID());
                data->WriteBits(0, 24);                     // count of next fields
                break;
        }

        data->WriteGuidMask<4>(memberGuid);

        buffer.WriteGuidBytes<0, 3>(memberGuid);
        // if (unk4) << uint32() unk
        buffer.WriteGuidBytes<7, 2>(memberGuid);
    }

    data->WriteBit(bg->GetStatus() == STATUS_WAIT_LEAVE);   // If Ended

    if (bg->isRated())                                      // arena
    {
        for (int8 i = 0; i < PVP_TEAM_COUNT; ++i)
        {
            uint32 pointsLost = bg->m_ArenaTeamRatingChanges[i] < 0 ? abs(bg->m_ArenaTeamRatingChanges[i]) : 0;
            uint32 pointsGained = bg->m_ArenaTeamRatingChanges[i] > 0 ? bg->m_ArenaTeamRatingChanges[i] : 0;

            *data << uint32(0);                             // Matchmaking Value
            *data << uint32(pointsLost);                    // Rating Lost
            *data << uint32(pointsGained);                  // Rating gained
            DEBUG_LOG("rating change: %d", bg->m_ArenaTeamRatingChanges[i]);
        }
    }

    data->FlushBits();
    data->append(buffer);

    if (bg->isArena())
    {
        for (int8 i = 0; i < PVP_TEAM_COUNT; ++i)
        {
            if (ArenaTeam* at = sObjectMgr.GetArenaTeamById(bg->m_ArenaTeamIds[i]))
            {
                data->append(at->GetName().data(), at->GetName().length());
            }
        }
    }

    *data << uint8(bg->GetPlayersCountByTeam(HORDE));

    if (bg->GetStatus() == STATUS_WAIT_LEAVE)
    {
        *data << uint8(bg->GetWinner() == ALLIANCE);        // who win
    }

    *data << uint8(bg->GetPlayersCountByTeam(ALLIANCE));
}

/**
 * @brief Builds the group battleground join result packet.
 *
 * Writes the battleground join status code returned to grouped players after a
 * join request is processed.
 *
 * @param data Pointer to the packet being filled.
 * @param status The battleground group join status code.
 */
void BattleGroundMgr::BuildBattleGroundStatusFailedPacket(WorldPacket* data, BattleGround* bg, Player* player, uint8 QueueSlot, GroupJoinBattlegroundResult result)
{
    ObjectGuid bgGuid = bg->GetObjectGuid();
    ObjectGuid unkGuid2 = ObjectGuid();
    ObjectGuid playerGuid = player ? player->GetObjectGuid() : ObjectGuid(); // player who caused the error

    DEBUG_LOG("BattleGroundMgr::BuildBattleGroundStatusFailedPacket slot %u result %u bgstatus %u player %s bg %s",
        QueueSlot, result, bg->GetStatus(), playerGuid.GetString().c_str(), bgGuid.GetString().c_str());

    data->Initialize(SMSG_BATTLEFIELD_STATUS_FAILED);

    data->WriteGuidMask<3>(bgGuid);
    data->WriteGuidMask<3>(playerGuid);
    data->WriteGuidMask<3>(unkGuid2);
    data->WriteGuidMask<0>(playerGuid);
    data->WriteGuidMask<6>(bgGuid);
    data->WriteGuidMask<5, 6, 4, 2>(unkGuid2);

    data->WriteGuidMask<1>(playerGuid);
    data->WriteGuidMask<1>(bgGuid);
    data->WriteGuidMask<5, 6>(playerGuid);
    data->WriteGuidMask<1>(unkGuid2);
    data->WriteGuidMask<7>(bgGuid);
    data->WriteGuidMask<4>(playerGuid);

    data->WriteGuidMask<2, 5>(bgGuid);
    data->WriteGuidMask<7>(playerGuid);
    data->WriteGuidMask<4, 0>(bgGuid);
    data->WriteGuidMask<0>(unkGuid2);
    data->WriteGuidMask<2>(playerGuid);
    data->WriteGuidMask<7>(unkGuid2);

    data->WriteGuidBytes<1>(bgGuid);

    *data << uint32(QueueSlot);         // not queue slot
    *data << uint32(QueueSlot);         // Queue slot

    data->WriteGuidBytes<6, 3, 7, 4>(unkGuid2);
    data->WriteGuidBytes<0>(bgGuid);
    data->WriteGuidBytes<5>(unkGuid2);
    data->WriteGuidBytes<7, 6, 2>(bgGuid);
    data->WriteGuidBytes<6, 3>(playerGuid);
    data->WriteGuidBytes<1>(unkGuid2);
    data->WriteGuidBytes<3>(bgGuid);
    data->WriteGuidBytes<0, 1, 4>(playerGuid);
    data->WriteGuidBytes<0>(unkGuid2);
    data->WriteGuidBytes<5>(bgGuid);
    data->WriteGuidBytes<7>(playerGuid);
    data->WriteGuidBytes<4>(bgGuid);
    data->WriteGuidBytes<2>(unkGuid2);

    *data << uint32(result);            // Result

    data->WriteGuidBytes<2>(playerGuid);

    *data << uint32(0);                 // unk Time

    data->WriteGuidBytes<5>(playerGuid);
}

/**
 * @brief Builds a world state update packet.
 *
 * Populates a packet with a world state field identifier and its new value so
 * clients can refresh battleground UI state.
 *
 * @param data Pointer to the packet being filled.
 * @param field The world state field identifier.
 * @param value The value to assign to the field.
 */
void BattleGroundMgr::BuildUpdateWorldStatePacket(WorldPacket* data, uint32 field, uint32 value)
{
    data->Initialize(SMSG_UPDATE_WORLD_STATE, 4 + 4);
    *data << uint32(field);
    *data << uint32(value);
    *data << uint8(0);
}

/**
 * @brief Builds a packet to play a sound effect.
 *
 * Constructs the SMSG_PLAY_SOUND packet that instructs clients to play a specific sound.
 *
 * @param data Pointer to the WorldPacket to write data to.
 * @param soundid The sound ID to play.
 */
void BattleGroundMgr::BuildPlaySoundPacket(WorldPacket* data, uint32 soundid)
{
    data->Initialize(SMSG_PLAY_SOUND, 4);
    *data << uint32(soundid);
    *data << uint64(0);
}

/**
 * @brief Builds a packet for when a player leaves a battleground.
 *
 * Constructs the SMSG_BATTLEGROUND_PLAYER_LEFT packet that notifies other players
 * about a player leaving the battleground.
 *
 * @param data Pointer to the WorldPacket to write data to.
 * @param guid The GUID of the player who left.
 */
void BattleGroundMgr::BuildPlayerLeftBattleGroundPacket(WorldPacket* data, ObjectGuid guid)
{
    data->Initialize(SMSG_BATTLEGROUND_PLAYER_LEFT, 8);
    data->WriteGuidMask<7, 6, 2, 4, 5, 1, 3, 0>(guid);
    data->WriteGuidBytes<4, 2, 5, 7, 0, 6, 1, 3>(guid);
}

/**
 * @brief Builds a packet for when a player joins a battleground.
 *
 * Constructs the SMSG_BATTLEGROUND_PLAYER_JOINED packet that notifies other players
 * about a new player joining the battleground.
 *
 * @param data Pointer to the WorldPacket to write data to.
 * @param plr Pointer to the player who joined.
 */
void BattleGroundMgr::BuildPlayerJoinedBattleGroundPacket(WorldPacket* data, Player* plr)
{
    data->Initialize(SMSG_BATTLEGROUND_PLAYER_JOINED, 8);
    data->WriteGuidMask<0, 4, 3, 5, 7, 6, 2, 1>(plr->GetObjectGuid());
    data->WriteGuidBytes<1, 5, 3, 2, 0, 7, 4, 6>(plr->GetObjectGuid());
}

/**
 * @brief Retrieves a battleground instance by client instance ID.
 *
 * Searches for a battleground instance using the client-side instance ID that was
 * sent in the SMSG_BATTLEFIELD_LIST packet. This is used when a player joins from the UI.
 *
 * @param instanceId The client-side instance ID.
 * @param bgTypeId The battleground type to search in.
 * @return Pointer to the battleground instance, or NULL if not found.
 */
BattleGround* BattleGroundMgr::GetBattleGroundThroughClientInstance(uint32 instanceId, BattleGroundTypeId bgTypeId)
{
    // cause at HandleBattleGroundJoinOpcode the clients sends the instanceid he gets from
    // SMSG_BATTLEFIELD_LIST we need to find the battleground with this clientinstance-id
    BattleGround* bg = GetBattleGroundTemplate(bgTypeId);
    if (!bg)
    {
        return NULL;
    }

    if (bg->isArena())
    {
        return GetBattleGround(instanceId, bgTypeId);
    }

    for (BattleGroundSet::iterator itr = m_BattleGrounds[bgTypeId].begin(); itr != m_BattleGrounds[bgTypeId].end(); ++itr)
    {
        if (itr->second->GetClientInstanceID() == instanceId)
        {
            return itr->second;
        }
    }
    return NULL;
}

/**
 * @brief Retrieves a battleground instance by instance ID.
 *
 * Searches for an active battleground instance by its server instance ID. If bgTypeId
 * is BATTLEGROUND_TYPE_NONE, searches across all battleground types.
 *
 * @param InstanceID The server instance ID.
 * @param bgTypeId The battleground type to search in, or BATTLEGROUND_TYPE_NONE for all types.
 * @return Pointer to the battleground instance, or NULL if not found.
 */
BattleGround* BattleGroundMgr::GetBattleGround(uint32 InstanceID, BattleGroundTypeId bgTypeId)
{
    // search if needed
    BattleGroundSet::iterator itr;
    if (bgTypeId == BATTLEGROUND_TYPE_NONE)
    {
        for (uint8 i = BATTLEGROUND_AV; i < MAX_BATTLEGROUND_TYPE_ID; ++i)
        {
            itr = m_BattleGrounds[i].find(InstanceID);
            if (itr != m_BattleGrounds[i].end())
            {
                return itr->second;
            }
        }
        return NULL;
    }
    itr = m_BattleGrounds[bgTypeId].find(InstanceID);
    return ((itr != m_BattleGrounds[bgTypeId].end()) ? itr->second : NULL);
}

/**
 * @brief Retrieves the template battleground for a given type.
 *
 * Returns the template battleground for the specified type. The template is the lowest-ID
 * battleground in the container and is used as a reference for creating new instances.
 *
 * @param bgTypeId The battleground type.
 * @return Pointer to the template battleground, or NULL if none exists.
 */
BattleGround* BattleGroundMgr::GetBattleGroundTemplate(BattleGroundTypeId bgTypeId)
{
    // map is sorted and we can be sure that lowest instance id has only BG template
    return m_BattleGrounds[bgTypeId].empty() ? NULL : m_BattleGrounds[bgTypeId].begin()->second;
}

/**
 * @brief Creates a unique client-visible instance ID for a battleground.
 *
 * Generates a new unique client-facing instance ID for the specified battleground type and bracket.
 * Client IDs are sequential starting from 1, filling any gaps in the ID sequence. These IDs are
 * sent to clients in the battleground list packet and used when players join via the UI.
 *
 * @param bgTypeId The battleground type.
 * @param bracket_id The bracket level.
 * @return A unique client-visible instance ID.
 */
uint32 BattleGroundMgr::CreateClientVisibleInstanceId(BattleGroundTypeId bgTypeId, BattleGroundBracketId bracket_id)
{
    if (IsArenaType(bgTypeId))
    {
        return 0; // arenas don't have client-instanceids
    }

    // we create here an instanceid, which is just for
    // displaying this to the client and without any other use..
    // the client-instanceIds are unique for each battleground-type
    // the instance-id just needs to be as low as possible, beginning with 1
    // the following works, because std::set is default ordered with "<"
    // the optimalization would be to use as bitmask std::vector<uint32> - but that would only make code unreadable
    uint32 lastId = 0;
    ClientBattleGroundIdSet& ids = m_ClientBattleGroundIds[bgTypeId][bracket_id];
    for (ClientBattleGroundIdSet::const_iterator itr = ids.begin(); itr != ids.end();)
    {
        if ((++lastId) != *itr)                             // if there is a gap between the ids, we will break..
        {
            break;
        }
        lastId = *itr;
    }
    ids.insert(lastId + 1);
    return lastId + 1;
}

/**
 * @brief Creates a new battleground instance.
 *
 * Creates a new playable battleground instance by copying the template and initializing
 * it with a new instance ID, bracket ID, and game map. The new battleground is placed in
 * queue waiting for players to join.
 *
 * @param bgTypeId The type of battleground to create.
 * @param bracketEntry
 * @param arenaType
 * @param isRated
 * @return Pointer to the newly created battleground, or NULL if creation failed.
 */
BattleGround* BattleGroundMgr::CreateNewBattleGround(BattleGroundTypeId bgTypeId, PvPDifficultyEntry const* bracketEntry, ArenaType arenaType, bool isRated)
{
    // get the template BG
    BattleGround* bg_template = GetBattleGroundTemplate(bgTypeId);
    if (!bg_template)
    {
        sLog.outError("BattleGround: CreateNewBattleGround - bg template not found for %u", bgTypeId);
        return NULL;
    }

    // for arenas there is random map used
    if (bg_template->isArena())
    {
        BattleGroundTypeId arenas[] = { BATTLEGROUND_NA, BATTLEGROUND_BE, BATTLEGROUND_RL/*, BATTLEGROUND_DS, BATTLEGROUND_RV*/ };
        bgTypeId = arenas[urand(0, countof(arenas) - 1)];
        bg_template = GetBattleGroundTemplate(bgTypeId);
        if (!bg_template)
        {
            sLog.outError("BattleGround: CreateNewBattleGround - bg template not found for %u", bgTypeId);
            return NULL;
        }
    }

    BattleGround* bg = NULL;
    // create a copy of the BG template
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV:
            bg = new BattleGroundAV(*(BattleGroundAV*)bg_template);
            break;
        case BATTLEGROUND_WS:
            bg = new BattleGroundWS(*(BattleGroundWS*)bg_template);
            break;
        case BATTLEGROUND_AB:
            bg = new BattleGroundAB(*(BattleGroundAB*)bg_template);
            break;
        case BATTLEGROUND_NA:
            bg = new BattleGroundNA(*(BattleGroundNA*)bg_template);
            break;
        case BATTLEGROUND_BE:
            bg = new BattleGroundBE(*(BattleGroundBE*)bg_template);
            break;
        case BATTLEGROUND_AA:
            bg = new BattleGroundAA(*(BattleGroundAA*)bg_template);
            break;
        case BATTLEGROUND_EY:
            bg = new BattleGroundEY(*(BattleGroundEY*)bg_template);
            break;
        case BATTLEGROUND_RL:
            bg = new BattleGroundRL(*(BattleGroundRL*)bg_template);
            break;
        case BATTLEGROUND_SA:
            bg = new BattleGroundSA(*(BattleGroundSA*)bg_template);
            break;
        case BATTLEGROUND_DS:
            bg = new BattleGroundDS(*(BattleGroundDS*)bg_template);
            break;
        case BATTLEGROUND_RV:
            bg = new BattleGroundRV(*(BattleGroundRV*)bg_template);
            break;
        case BATTLEGROUND_IC:
            bg = new BattleGroundIC(*(BattleGroundIC*)bg_template);
            break;
        case BATTLEGROUND_RB:
            bg = new BattleGroundRB(*(BattleGroundRB*)bg_template);
            break;
        default:
            // error, but it is handled few lines above
            return 0;
    }

    // set before Map creating for let use proper difficulty
    bg->SetBracket(bracketEntry);

    // will also set m_bgMap, instanceid
    sMapMgr.CreateBgMap(bg->GetMapId(), bg);

    bg->SetClientInstanceID(CreateClientVisibleInstanceId(bgTypeId, bracketEntry->GetBracketId()));

    // reset the new bg (set status to status_wait_queue from status_none)
    bg->Reset();

    // start the joining of the bg
    bg->SetStatus(STATUS_WAIT_JOIN);
    bg->SetArenaType(arenaType);
    bg->SetRated(isRated);
    bg->SetRandomTypeID(bgTypeId);

    return bg;
}

/**
 * @brief Creates a battleground template.
 *
 * Creates a template battleground that serves as the prototype for all instances of this type.
 * The template stores configuration like player limits, level requirements, and spawn locations.
 * New instances are created by copying this template.
 *
 * @param bgTypeId The battleground type.
 * @param IsArena Whether the template is an arena.
 * @param MinPlayersPerTeam Minimum players required per team.
 * @param MaxPlayersPerTeam Maximum players allowed per team.
 * @param LevelMin Minimum level to queue for this battleground.
 * @param LevelMax Maximum level for this battleground.
 * @param BattleGroundName The name of the battleground.
 * @param MapID The map ID for this battleground.
 * @param Team1StartLocX Alliance spawn location X coordinate.
 * @param Team1StartLocY Alliance spawn location Y coordinate.
 * @param Team1StartLocZ Alliance spawn location Z coordinate.
 * @param Team1StartLocO Alliance spawn location orientation.
 * @param Team2StartLocX Horde spawn location X coordinate.
 * @param Team2StartLocY Horde spawn location Y coordinate.
 * @param Team2StartLocZ Horde spawn location Z coordinate.
 * @param Team2StartLocO Horde spawn location orientation.
 * @param StartMaxDist Maximum distance from spawn location for initial positioning.
 * @return The instance ID of the created template battleground.
 */
uint32 BattleGroundMgr::CreateBattleGround(BattleGroundTypeId bgTypeId, bool IsArena, uint32 MinPlayersPerTeam, uint32 MaxPlayersPerTeam, uint32 LevelMin, uint32 LevelMax, char const* BattleGroundName, uint32 MapID, float Team1StartLocX, float Team1StartLocY, float Team1StartLocZ, float Team1StartLocO, float Team2StartLocX, float Team2StartLocY, float Team2StartLocZ, float Team2StartLocO, float StartMaxDist)
{
    // Create the BG
    BattleGround* bg = NULL;
    switch (bgTypeId)
    {
        case BATTLEGROUND_AV: bg = new BattleGroundAV; break;
        case BATTLEGROUND_WS: bg = new BattleGroundWS; break;
        case BATTLEGROUND_AB: bg = new BattleGroundAB; break;
        case BATTLEGROUND_NA: bg = new BattleGroundNA; break;
        case BATTLEGROUND_BE: bg = new BattleGroundBE; break;
        case BATTLEGROUND_AA: bg = new BattleGroundAA; break;
        case BATTLEGROUND_EY: bg = new BattleGroundEY; break;
        case BATTLEGROUND_RL: bg = new BattleGroundRL; break;
        case BATTLEGROUND_SA: bg = new BattleGroundSA; break;
        case BATTLEGROUND_DS: bg = new BattleGroundDS; break;
        case BATTLEGROUND_RV: bg = new BattleGroundRV; break;
        case BATTLEGROUND_IC: bg = new BattleGroundIC; break;
        case BATTLEGROUND_RB: bg = new BattleGroundRB; break;
        default:              bg = new BattleGround;   break;                           // placeholder for non implemented BG
    }

    bg->SetMapId(MapID);
    bg->SetTypeID(bgTypeId);
    bg->SetArenaorBGType(IsArena);
    bg->SetMinPlayersPerTeam(MinPlayersPerTeam);
    bg->SetMaxPlayersPerTeam(MaxPlayersPerTeam);
    bg->SetMinPlayers(MinPlayersPerTeam * 2);
    bg->SetMaxPlayers(MaxPlayersPerTeam * 2);
    bg->SetName(BattleGroundName);
    bg->SetTeamStartLoc(ALLIANCE, Team1StartLocX, Team1StartLocY, Team1StartLocZ, Team1StartLocO);
    bg->SetTeamStartLoc(HORDE,    Team2StartLocX, Team2StartLocY, Team2StartLocZ, Team2StartLocO);
    bg->SetStartMaxDist(StartMaxDist);
    bg->SetLevelRange(LevelMin, LevelMax);

    // add bg to update list
    AddBattleGround(bg->GetInstanceID(), bg->GetTypeID(), bg);

    // return some not-null value, bgTypeId is good enough for me
    return bgTypeId;
}

/**
 * @brief Creates initial battleground templates from the database.
 *
 * Loads battleground template configurations from the database table `battleground_template`
 * and creates the template instances for each configured battleground type. These templates
 * are used as prototypes for all new battleground instances.
 */
void BattleGroundMgr::CreateInitialBattleGrounds()
{
    uint32 count = 0;

    //                                                 0     1                   2                   3                  4                5               6              7
    QueryResult* result = WorldDatabase.Query("SELECT `id`, `MinPlayersPerTeam`,`MaxPlayersPerTeam`,`AllianceStartLoc`,`AllianceStartO`,`HordeStartLoc`,`HordeStartO`, `StartMaxDist` FROM `battleground_template`");

    if (!result)
    {
        BarGoLink bar(1);
        bar.step();
        sLog.outErrorDb(">> Loaded 0 battlegrounds. DB table `battleground_template` is empty.");
        sLog.outString();
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();
        bar.step();

        uint32 bgTypeID_ = fields[0].GetUInt32();

        // can be overwrite by values from DB
        BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(bgTypeID_);
        if (!bl)
        {
            sLog.outError("Battleground ID %u not found in BattlemasterList.dbc. Battleground not created.", bgTypeID_);
            continue;
        }

        if (DisableMgr::IsDisabledFor(DISABLE_TYPE_BATTLEGROUND, bgTypeID_))
        {
            continue;
        }

        BattleGroundTypeId bgTypeID = BattleGroundTypeId(bgTypeID_);

        bool IsArena = (bl->type == TYPE_ARENA);
        uint32 MinPlayersPerTeam = fields[1].GetUInt32();
        uint32 MaxPlayersPerTeam = fields[2].GetUInt32();

        // check values from DB
        if (MaxPlayersPerTeam == 0)
        {
            sLog.outErrorDb("Table `battleground_template` for id %u have wrong min/max players per team settings. BG not created.", bgTypeID);
            continue;
        }

        if (MinPlayersPerTeam > MaxPlayersPerTeam)
        {
            MinPlayersPerTeam = MaxPlayersPerTeam;
            sLog.outErrorDb("Table `battleground_template` for id %u has min players > max players per team settings. Min players will use same value as max players.", bgTypeID);
        }

        float AStartLoc[4];
        float HStartLoc[4];

        uint32 start1 = fields[3].GetUInt32();

        WorldSafeLocsEntry const* start = sWorldSafeLocsStore.LookupEntry(start1);
        if (start)
        {
            AStartLoc[0] = start->x;
            AStartLoc[1] = start->y;
            AStartLoc[2] = start->z;
            AStartLoc[3] = fields[4].GetFloat();
        }
        else if (bgTypeID == BATTLEGROUND_AA || bgTypeID == BATTLEGROUND_RB)
        {
            AStartLoc[0] = 0;
            AStartLoc[1] = 0;
            AStartLoc[2] = 0;
            AStartLoc[3] = fields[4].GetFloat();
        }
        else
        {
            sLog.outErrorDb("Table `battleground_template` for id %u have nonexistent WorldSafeLocs.dbc id %u in field `AllianceStartLoc`. BG not created.", bgTypeID, start1);
            continue;
        }

        uint32 start2 = fields[5].GetUInt32();

        start = sWorldSafeLocsStore.LookupEntry(start2);
        if (start)
        {
            HStartLoc[0] = start->x;
            HStartLoc[1] = start->y;
            HStartLoc[2] = start->z;
            HStartLoc[3] = fields[6].GetFloat();
        }
        else if (bgTypeID == BATTLEGROUND_AA || bgTypeID == BATTLEGROUND_RB)
        {
            HStartLoc[0] = 0;
            HStartLoc[1] = 0;
            HStartLoc[2] = 0;
            HStartLoc[3] = fields[6].GetFloat();
        }
        else
        {
            sLog.outErrorDb("Table `battleground_template` for id %u have nonexistent WorldSafeLocs.dbc id %u in field `HordeStartLoc`. BG not created.", bgTypeID, start2);
            continue;
        }

        float startMaxDist = fields[7].GetFloat();
        // sLog.outDetail("Creating battleground %s, %u-%u", bl->name[sWorld.GetDBClang()], MinLvl, MaxLvl);
        if (!CreateBattleGround(bgTypeID, IsArena, MinPlayersPerTeam, MaxPlayersPerTeam, bl->minLevel, bl->maxLevel, bl->name[sWorld.GetDefaultDbcLocale()], bl->mapid[0], AStartLoc[0], AStartLoc[1], AStartLoc[2], AStartLoc[3], HStartLoc[0], HStartLoc[1], HStartLoc[2], HStartLoc[3], startMaxDist))
        {
            continue;
        }

        ++count;
    }
    while (result->NextRow());

    delete result;

    sLog.outString(">> Loaded %u battlegrounds", count);
    sLog.outString();
}

void BattleGroundMgr::InitAutomaticArenaPointDistribution()
{
    if (sWorld.getConfig(CONFIG_BOOL_ARENA_AUTO_DISTRIBUTE_POINTS))
    {
        DEBUG_LOG("Initializing Automatic Arena Point Distribution");
        QueryResult* result = CharacterDatabase.Query("SELECT `NextArenaPointDistributionTime` FROM `saved_variables`");
        if (!result)
        {
            DEBUG_LOG("Battleground: Next arena point distribution time not found in SavedVariables, reseting it now.");
            m_NextAutoDistributionTime = time_t(sWorld.GetGameTime() + BATTLEGROUND_ARENA_POINT_DISTRIBUTION_DAY * sWorld.getConfig(CONFIG_UINT32_ARENA_AUTO_DISTRIBUTE_INTERVAL_DAYS));
            CharacterDatabase.PExecute("INSERT INTO `saved_variables` (`NextArenaPointDistributionTime`) VALUES ('" UI64FMTD "')", uint64(m_NextAutoDistributionTime));
        }
        else
        {
            m_NextAutoDistributionTime = time_t((*result)[0].GetUInt64());
            delete result;
        }
        DEBUG_LOG("Automatic Arena Point Distribution initialized.");
    }
}

/*
 *  there does not appear to be a way to do this in Three
void BattleGroundMgr::DistributeArenaPoints()
{
    // used to distribute arena points based on last week's stats
    sWorld.SendWorldText(LANG_DIST_ARENA_POINTS_START);

    sWorld.SendWorldText(LANG_DIST_ARENA_POINTS_ONLINE_START);

    // temporary structure for storing maximum points to add values for all players
    std::map<uint32, uint32> PlayerPoints;

    // at first update all points for all team members
    for (ObjectMgr::ArenaTeamMap::iterator team_itr = sObjectMgr.GetArenaTeamMapBegin(); team_itr != sObjectMgr.GetArenaTeamMapEnd(); ++team_itr)
    {
        if (ArenaTeam* at = team_itr->second)
        {
            at->UpdateArenaPointsHelper(PlayerPoints);
        }
    }

    // cycle that gives points to all players
    for (std::map<uint32, uint32>::iterator plr_itr = PlayerPoints.begin(); plr_itr != PlayerPoints.end(); ++plr_itr)
    {
        // update to database
        CharacterDatabase.PExecute("UPDATE `characters` SET `arenaPoints` = `arenaPoints` + '%u' WHERE `guid` = '%u'", plr_itr->second, plr_itr->first);
        // add points if player is online
        if (Player* pl = sObjectMgr.GetPlayer(ObjectGuid(HIGHGUID_PLAYER, plr_itr->first)))
        {
            pl->ModifyArenaPoints(plr_itr->second);
        }
    }

    PlayerPoints.clear();

    sWorld.SendWorldText(LANG_DIST_ARENA_POINTS_ONLINE_END);

    sWorld.SendWorldText(LANG_DIST_ARENA_POINTS_TEAM_START);
    for (ObjectMgr::ArenaTeamMap::iterator titr = sObjectMgr.GetArenaTeamMapBegin(); titr != sObjectMgr.GetArenaTeamMapEnd(); ++titr)
    {
        if (ArenaTeam* at = titr->second)
        {
            at->FinishWeek();                              // set played this week etc values to 0 in memory, too
            at->SaveToDB();                                // save changes
            at->NotifyStatsChanged();                      // notify the players of the changes
        }
    }

    sWorld.SendWorldText(LANG_DIST_ARENA_POINTS_TEAM_END);

    sWorld.SendWorldText(LANG_DIST_ARENA_POINTS_END);
}
*/

/**
 * @brief Builds the battleground instance list packet for a player.
 *
 * Enumerates the client-visible battleground instances available for the player's
 * bracket and writes them into the battlefield list response.
 *
 * @param data Pointer to the packet being filled.
 * @param guid The battlemaster GUID associated with the request.
 * @param plr The player receiving the list.
 * @param bgTypeId The battleground type being listed.
 */
void BattleGroundMgr::BuildBattleGroundListPacket(WorldPacket* data, ObjectGuid guid, Player* plr, BattleGroundTypeId bgTypeId)
{
    if (!plr)
    {
        return;
    }

    BattleGround* bgTemplate = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);

    data->Initialize(SMSG_BATTLEFIELD_LIST);
    *data << uint32(0);                     // 4.3.4 winConquest weekend
    *data << uint32(0);                     // 4.3.4 winConquest random
    *data << uint32(0);                     // 4.3.4 lossHonor weekend
    *data << uint32(bgTypeId);              // battleground id
    *data << uint32(0);                     // 4.3.4 lossHonor random
    *data << uint32(0);                     // 4.3.4 winHonor random
    *data << uint32(0);                     // 4.3.4 winHonor weekend
    *data << uint8(bgTemplate->GetMaxLevel()); // max level
    *data << uint8(bgTemplate->GetMinLevel()); // min level
    data->WriteGuidMask<0, 1, 7>(guid);
    data->WriteBit(false);                  // has holiday bg currency bonus ??
    data->WriteBit(false);                  // has random bg currency bonus ??

    uint32 count = 0;
    ByteBuffer buf;
    if (bgTemplate)
    {
        // expected bracket entry
        if (PvPDifficultyEntry const* bracketEntry = GetBattlegroundBracketByLevel(bgTemplate->GetMapId(), plr->getLevel()))
        {
            BattleGroundBracketId bracketId = bracketEntry->GetBracketId();
            ClientBattleGroundIdSet const& ids = m_ClientBattleGroundIds[bgTypeId][bracketId];
            for (ClientBattleGroundIdSet::const_iterator itr = ids.begin(); itr != ids.end(); ++itr)
            {
                buf << uint32(*itr);
                ++count;
            }
        }
    }

    data->WriteBits(count, 24);
    data->WriteGuidMask<6, 4, 2, 3>(guid);
    data->WriteBit(false);                   // unk
    data->WriteGuidMask<5>(guid);
    data->WriteBit(true);                   // hide battleground list window

    data->WriteGuidBytes<6, 1, 7, 5>(guid);
    data->FlushBits();
    if (count)
    {
        data->append(buf);
    }
    data->WriteGuidBytes<0, 2, 4, 3>(guid);
}

/**
 * @brief Teleports a player to their assigned battleground location.
 *
 * Moves the player to the battleground map and their team's spawn location. Handles
 * retrieving the correct start location for the player's team.
 *
 * @param pl Pointer to the player to teleport.
 * @param instanceId The battleground instance ID.
 * @param bgTypeId The battleground type.
 */
void BattleGroundMgr::SendToBattleGround(Player* pl, uint32 instanceId, BattleGroundTypeId bgTypeId)
{
    BattleGround* bg = GetBattleGround(instanceId, bgTypeId);
    if (bg)
    {
        uint32 mapid = bg->GetMapId();
        float x, y, z, O;
        Team team = pl->GetBGTeam();
        if (team == 0)
        {
            team = pl->GetTeam();
        }
        bg->GetTeamStartLoc(team, x, y, z, O);

        DETAIL_LOG("BATTLEGROUND: Sending %s to map %u, X %f, Y %f, Z %f, O %f", pl->GetName(), mapid, x, y, z, O);
        pl->TeleportTo(mapid, x, y, z, O);
    }
    else
    {
        sLog.outError("player %u trying to port to nonexistent bg instance %u", pl->GetGUIDLow(), instanceId);
    }
}

