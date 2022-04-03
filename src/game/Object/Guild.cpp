/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2022 MaNGOS <https://getmangos.eu>
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

#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Player.h"
#include "Opcodes.h"
#include "ObjectMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Chat.h"
#include "SocialMgr.h"
#include "Util.h"
#include "Language.h"
#include "World.h"
#include "Calendar.h"
#include "Log.h"

#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

#define MAX_GUILD_BANK_TAB_TEXT_LEN 500
#define EMBLEM_PRICE 10 * GOLD

inline uint32 _GetGuildBankTabPrice(uint8 tabId)
{
    switch (tabId)
    {
        case 0: return 100;
        case 1: return 250;
        case 2: return 500;
        case 3: return 1000;
        case 4: return 2500;
        case 5: return 5000;
        default: return 0;       
    }
}

void Guild::SendCommandResult(WorldSession* session, GuildCommandType type, GuildCommandError errCode, const std::string& param)
{
    WorldPacket data(SMSG_GUILD_COMMAND_RESULT, 8 + param.size() + 1);
    data << uint32(type);
    data << param;
    data << uint32(errCode);

    session->SendPacket(&data);

    sLog.outDebug("World: Sent SMSG_GUILD_COMMAND_RESULT");
}

void Guild::SendSaveEmblemResult(WorldSession* session, GuildEmblemError errCode)
{
    WorldPacket data(MSG_SAVE_GUILD_EMBLEM, 4);
    data << uint32(errCode);

    session->SendPacket(&data);

    sLog.outDebug("World: Sent MSG_SAVE_GUILD_EMBLEM");
}

/* Log Holder */
Guild::LogHolder::~LogHolder()
{
    /* clean up */
    for (GuildLog::iterator itr = m_log.begin(); itr != m_log.end(); ++itr)
    {
        delete (*itr);
    }
}

/* Adds event loaded from database to collection */
inline void Guild::LogHolder::LoadEvent(LogEntry* entry)
{
    if (m_nextGUID == uint32(GUILD_EVENT_LOG_GUID_UNDEFINED))
    {
        m_nextGUID = entry->GetGUID();
    }

    m_log.push_front(entry);
}

inline void Guild::LogHolder::AddEvent(SqlTransaction& trans, LogEntry* entry)
{
    if (m_log.size() >= m_maxRecords)
    {
        LogEntry* oldEntry = m_log.front();
        delete oldEntry;

        m_log.pop_front();
    }

    /* add event to list */
    m_log.push_back(entry);

    /* save to database */
    entry->SaveToDB(trans);
}

inline void Guild::LogHolder::WritePacket(WorldPacket& data) const
{
    ByteBuffer buffer;
    data.WriteBits(m_log.size(), 23);

    for (GuildLog::const_iterator itr = m_log.begin(); itr != m_log.end(); ++itr)
    {
        (*itr)->WritePacket(data, buffer);
    }

    data.FlushBits();
    data.append(buffer);
}
