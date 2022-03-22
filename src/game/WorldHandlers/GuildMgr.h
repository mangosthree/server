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

#ifndef _GUILDMGR_H
#define _GUILDMGR_H

#include "Common.h"
#include "Policies/Singleton.h"

class Guild;
class ObjectGuid;

class GuildMgr
{
    private:
        GuildMgr();
        ~GuildMgr();

    public:
        typedef UNORDERED_MAP<uint32, Guild*> GuildContainer;

        void AddGuild(Guild* guild);
        void RemoveGuild(uint32 guildId);
        void RemoveGuild(ObjectGuid guildGuid);

        Guild* GetGuildById(uint32 guildId) const;
        Guild* GetGuildByGuid(ObjectGuid guildGuid) const;
        Guild* GetGuildByName(std::string const& name) const;
        Guild* GetGuildByLeader(ObjectGuid const& guid) const;
        std::string GetGuildNameById(uint32 guildId) const;
        std::string GetGuildNameByGuid(ObjectGuid guildGuid) const;

        void LoadGuildXpForLevel();
        void LoadGuildRewards();

        void LoadGuilds();

        void ResetExperienceCaps();
        void ResetReputationCaps();

    protected:
        uint32 NextGuildId;
        GuildContainer GuildMap;
        std::vector<uint64> GuildXPperLevel;
};

#define sGuildMgr MaNGOS::Singleton<GuildMgr>::Instance()

#endif // _GUILDMGR_H
