/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2014  MaNGOS project <http://getmangos.eu>
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

/// \addtogroup realmd
/// @{
/// \file

#ifndef MANGOS_H_REALMLIST
#define MANGOS_H_REALMLIST

#include "Common.h"

/**
 * @brief
 *
 */
struct RealmBuildInfo
{
    int build; /**< TODO */
    int major_version; /**< TODO */
    int minor_version; /**< TODO */
    int bugfix_version; /**< TODO */
    int hotfix_version; /**< TODO */
};

enum RealmVersion
{
    REALM_VERSION_VANILLA = 0,
    REALM_VERSION_TBC     = 1,
    REALM_VERSION_WOTLK   = 2,
    REALM_VERSION_CATA    = 3,
    REALM_VERSION_MOP     = 4,
    REALM_VERSION_COUNT   = 5
};

/**
 * This is used to make a link between build number and actual wow version that
 * it belongs to. To get the connection between them, ie turn a build into a version
 * one would use \ref RealmList::BelongsToVersion the other way around is not available
 * as it does not make sense and isn't needed.
 */
RealmBuildInfo const* FindBuildInfo(uint16 _build);

/**
 * @brief
 *
 */
typedef std::set<uint32> RealmBuilds;

/// Storage object for a realm
/**
 * @brief
 *
 */
struct Realm
{
    std::string address;
    uint8 icon;
    RealmFlags realmflags;                                  // realmflags
    uint8 timezone;
    uint32 m_ID;
    AccountTypes allowedSecurityLevel;                      // current allowed join security level (show as locked for not fit accounts)
    float populationLevel;
    RealmBuilds realmbuilds;                                // list of supported builds (updated in DB by mangosd)
    RealmBuildInfo realmBuildInfo;                          // build info for show version in list
};

/**
 * @brief Storage object for the list of realms on the server
 *
 */
class RealmList
{
    public:
        /**
         * @brief
         *
         */
        typedef std::map<std::string, Realm> RealmMap;

        /**
         * @brief
         *
         * @return RealmList
         */
        static RealmList& Instance();

        /**
         * @brief
         *
         */
        RealmList();
        /**
         * @brief
         *
         */
        ~RealmList() {};

        /**
         * @brief
         *
         * @param updateInterval
         */
        void Initialize(uint32 updateInterval);
        /** 
         * Initializes a map holding a link from build number to a version.
         * \see RealmVersion
         */
        void UpdateIfNeed();
        
        /** 
         * Get's the iterators for all realms supporting the given version as a pair,
         * the first member is a iterator to the begin() and the second is an iterator
         * to the end().
         * @param build the build number to fetch the iterators for
         * @return iterators to the begin() and end() part of the realms supporting
         * the given build, if there is no matching build iterators are given to end()
         * and end() of a list.
         */
        RealmMap::const_iterator begin() const { return m_realms.begin(); }

        /** 
         * Returns how many realms we have available for the current build
         * @param build the build we want to know number of available realms for
         * @return the number of available realms
         */
        RealmMap::const_iterator end() const { return m_realms.end(); }
        /**
         * @brief
         *
         * @return uint32
         */
        uint32 size() const { return m_realms.size(); };
    private:
        /**
         * @brief
         *
         * @param init
         */
        void UpdateRealms(bool init);
        /**
         * @brief
         *
         * @param ID
         * @param name
         * @param address
         * @param port
         * @param icon
         * @param realmflags
         * @param timezone
         * @param allowedSecurityLevel
         * @param popu
         * @param builds
         */
        void UpdateRealm(uint32 ID, const std::string& name, const std::string& address, uint32 port, uint8 icon, RealmFlags realmflags, uint8 timezone, AccountTypes allowedSecurityLevel, float popu, const std::string& builds);
    private:
        RealmMap m_realms;                                    ///< Internal map of realms
        uint32   m_UpdateInterval; /**< TODO */
        time_t   m_NextUpdateTime; /**< TODO */
};

#define sRealmList RealmList::Instance()

#endif
/// @}
