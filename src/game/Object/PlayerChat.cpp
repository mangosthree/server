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

/**
 * @brief Sends a say chat message from the player to nearby listeners.
 *
 * @param text The message text.
 * @param language The language used for the chat packet.
 */
void Player::Say(const std::string& text, const uint32 language)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SAY, text.c_str(), Language(language), GetChatTag(), GetObjectGuid(), GetName());
    SendMessageToSetInRange(&data, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_SAY), true);
}

/**
 * @brief Sends a yell chat message from the player to nearby listeners.
 *
 * @param text The message text.
 * @param language The language used for the chat packet.
 */
void Player::Yell(const std::string& text, const uint32 language)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_YELL, text.c_str(), Language(language), GetChatTag(), GetObjectGuid(), GetName());
    SendMessageToSetInRange(&data, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_YELL), true);
}

/**
 * @brief Sends a text emote message from the player to nearby listeners.
 *
 * @param text The emote text.
 */
void Player::TextEmote(const std::string& text)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_EMOTE, text.c_str(), LANG_UNIVERSAL, GetChatTag(), GetObjectGuid(), GetName());
    SendMessageToSetInRange(&data, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_TEXTEMOTE), true, !sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHAT));
}

/**
 * @brief Logs a whisper to the database when whisper logging is enabled.
 *
 * @param text The whisper text.
 * @param receiver The recipient player GUID.
 */
void Player::LogWhisper(const std::string& text, ObjectGuid receiver)
{
    WhisperLoggingLevels loggingLevel = WhisperLoggingLevels(sWorld.getConfig(CONFIG_UINT32_LOG_WHISPERS));

    if (loggingLevel == WHISPER_LOGGING_NONE)
    {
        return;
    }

    //Try to find ticket by either this player or the receiver
    GMTicket* ticket = sTicketMgr.GetGMTicket(GetObjectGuid());
    if (!ticket)
    {
        ticket = sTicketMgr.GetGMTicket(receiver);
    }

    uint32 ticketId = 0;
    if (ticket)
    {
        ticketId = ticket->GetId();
    }

    bool isSomeoneGM = false;

    //Find out if at least one of them is a GM for ticket logging
    if (GetSession()->GetSecurity() >= SEC_GAMEMASTER)
    {
        isSomeoneGM = true;
    }
    else
    {
        Player* pRecvPlayer = sObjectMgr.GetPlayer(receiver);
        if (pRecvPlayer && pRecvPlayer->GetSession()->GetSecurity() >= SEC_GAMEMASTER)
        {
            isSomeoneGM = true;
        }
    }

    if ((loggingLevel == WHISPER_LOGGING_TICKETS && ticket && isSomeoneGM)
        || loggingLevel == WHISPER_LOGGING_EVERYTHING)
    {
        static SqlStatementID wlog;
        SqlStatement stmt = CharacterDatabase.CreateStatement(wlog, "INSERT INTO `character_whispers` (`to_guid`, `from_guid`, `message`, `regarding_ticket_id`) VALUES (?, ?, ?, ?)");
        stmt.addUInt32(receiver.GetCounter());          // to_guid
        stmt.addUInt32(GetObjectGuid().GetCounter());   // from_guid
        stmt.addString(text.c_str());                   // message
        stmt.addUInt32(ticketId);                       // regarding_ticket_id
        stmt.Execute();
    }
}

/**
 * @brief Sends a whisper to another player and handles local response messages.
 *
 * @param text The whisper text.
 * @param language The requested chat language.
 * @param receiver The recipient player GUID.
 */
void Player::Whisper(const std::string& text, uint32 language, ObjectGuid receiver)
{
    Player* rPlayer = sObjectMgr.GetPlayer(receiver);

    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, text.c_str(), Language(language), GetChatTag(), GetObjectGuid(), GetName());
    rPlayer->GetSession()->SendPacket(&data);

    // do not send confirmations, afk, dnd or system notifications for addon messages
    if (language == LANG_ADDON)
    {
        data.clear();
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, text.c_str(), Language(language), CHAT_TAG_NONE, rPlayer->GetObjectGuid());
        LogWhisper(text, receiver);
        GetSession()->SendPacket(&data);
    }

    data.clear();
    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER_INFORM, text.c_str(), Language(language), CHAT_TAG_NONE, rPlayer->GetObjectGuid());
    GetSession()->SendPacket(&data);

    if (!isAcceptWhispers())
    {
        SetAcceptWhispers(true);
        ChatHandler(this).SendSysMessage(LANG_COMMAND_WHISPERON);
    }

    // announce afk or dnd message
    if (rPlayer->isAFK() || rPlayer->isDND())
    {
        const ChatMsg msgtype = rPlayer->isAFK() ? CHAT_MSG_AFK : CHAT_MSG_DND;
        data.clear();
        ChatHandler::BuildChatPacket(data, msgtype, rPlayer->autoReplyMsg.c_str(), LANG_UNIVERSAL, CHAT_TAG_NONE, rPlayer->GetObjectGuid());
        GetSession()->SendPacket(&data);
    }
}
