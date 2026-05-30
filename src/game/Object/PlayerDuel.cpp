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

#include "Player.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "SkillDiscovery.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "UpdateData.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Pet.h"
#include "Util.h"
#include "Transports.h"
#include "Weather.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGroundAV.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "ArenaTeam.h"
#include "Chat.h"
#include "revision_data.h"
#include "Database/DatabaseImpl.h"
#include "Spell.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "AchievementMgr.h"
#include "Mail.h"
#include "SpellAuras.h"
#include "DBCStores.h"
#include "DB2Stores.h"
#include "SQLStorages.h"
#include "Vehicle.h"
#include "Calendar.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

#include <cmath>
// If players are too far way of duel flag... then player loose the duel
void Player::CheckDuelDistance(time_t currTime)
{
    if (!duel)
    {
        return;
    }

    GameObject* obj = GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER));
    if (!obj)
    {
        // player not at duel start map
        DuelComplete(DUEL_FLED);
        return;
    }

    if (duel->outOfBound == 0)
    {
        if (!IsWithinDistInMap(obj, 50))
        {
            duel->outOfBound = currTime;

            WorldPacket data(SMSG_DUEL_OUTOFBOUNDS, 0);
            GetSession()->SendPacket(&data);
        }
    }
    else
    {
        if (IsWithinDistInMap(obj, 40))
        {
            duel->outOfBound = 0;

            WorldPacket data(SMSG_DUEL_INBOUNDS, 0);
            GetSession()->SendPacket(&data);
        }
        else if (currTime >= (duel->outOfBound + 10))
        {
            DuelComplete(DUEL_FLED);
        }
    }
}

/**
 * @brief Finalizes an active duel and cleans up all duel state.
 *
 * @param type The duel completion result.
 */
void Player::DuelComplete(DuelCompleteType type)
{
    // duel not requested
    if (!duel)
    {
        return;
    }

    Player* opponent = duel->opponent;

    // opponent may have been removed (disconnect during duel)
    if (!opponent)
    {
        // Remove Duel Flag object
        if (GameObject* obj = GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER)))
        {
            duel->initiator->RemoveGameObject(obj, true);
        }

        SetGuidValue(PLAYER_DUEL_ARBITER, ObjectGuid());
        SetUInt32Value(PLAYER_DUEL_TEAM, 0);
        delete duel;
        duel = NULL;
        return;
    }

    WorldPacket data(SMSG_DUEL_COMPLETE, (1));
    data << (uint8)((type != DUEL_INTERRUPTED) ? 1 : 0);
    GetSession()->SendPacket(&data);
    opponent->GetSession()->SendPacket(&data);

    if (type != DUEL_INTERRUPTED)
    {
        data.Initialize(SMSG_DUEL_WINNER, (1 + 20));          // we guess size
        data << (uint8)((type == DUEL_WON) ? 0 : 1);          // 0 = just won; 1 = fled
        data << opponent->GetName();
        data << GetName();
        SendMessageToSet(&data, true);
    }

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnDuelEnd(opponent, this, type);
    }
#endif /* ENABLE_ELUNA */

    if (type == DUEL_WON)
    {
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL, 1);
        opponent->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL, 1);
    }

    // Remove Duel Flag object
    if (GameObject* obj = GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER)))
    {
        duel->initiator->RemoveGameObject(obj, true);
    }

    /* remove auras */
    // TODO: Needs a simpler method
    std::vector<uint32> auras2remove;
    SpellAuraHolderMap const& vAuras = opponent->GetSpellAuraHolderMap();
    for (SpellAuraHolderMap::const_iterator i = vAuras.begin(); i != vAuras.end(); ++i)
    {
        SpellAuraHolder const* aura = i->second;
        if (!aura->IsPositive() && aura->GetCasterGuid() == GetObjectGuid() && aura->GetAuraApplyTime() >= duel->startTime)
        {
            auras2remove.push_back(aura->GetId());
        }
    }

    for (size_t i = 0; i < auras2remove.size(); ++i)
    {
        opponent->RemoveAurasDueToSpell(auras2remove[i]);
    }

    auras2remove.clear();
    SpellAuraHolderMap const& auras = GetSpellAuraHolderMap();
    for (SpellAuraHolderMap::const_iterator i = auras.begin(); i != auras.end(); ++i)
    {
        SpellAuraHolder const* aura = i->second;
        if (!aura->IsPositive() && aura->GetCasterGuid() == opponent->GetObjectGuid() && aura->GetAuraApplyTime() >= duel->startTime)
        {
            auras2remove.push_back(aura->GetId());
        }
    }
    for (size_t i = 0; i < auras2remove.size(); ++i)
    {
        RemoveAurasDueToSpell(auras2remove[i]);
    }

    // cleanup combo points
    if (GetComboTargetGuid() == opponent->GetObjectGuid())
    {
        ClearComboPoints();
    }
    else if (GetComboTargetGuid() == opponent->GetPetGuid())
    {
        ClearComboPoints();
    }

    if (opponent->GetComboTargetGuid() == GetObjectGuid())
    {
        opponent->ClearComboPoints();
    }
    else if (opponent->GetComboTargetGuid() == GetPetGuid())
    {
        opponent->ClearComboPoints();
    }

    // cleanups
    SetGuidValue(PLAYER_DUEL_ARBITER, ObjectGuid());
    SetUInt32Value(PLAYER_DUEL_TEAM, 0);
    opponent->SetGuidValue(PLAYER_DUEL_ARBITER, ObjectGuid());
    opponent->SetUInt32Value(PLAYER_DUEL_TEAM, 0);

    delete opponent->duel;
    opponent->duel = NULL;
    delete duel;
    duel = NULL;
}

void Player::SendDuelCountdown(uint32 counter)
{
    WorldPacket data(SMSG_DUEL_COUNTDOWN, 4);
    data << uint32(counter);                                // seconds
    GetSession()->SendPacket(&data);
}
