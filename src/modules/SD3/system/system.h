/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013 ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2025 MaNGOS <https://www.getmangos.eu>
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

#ifndef SC_SYSTEM_H
#define SC_SYSTEM_H

#include <unordered_map>

#define TEXT_SOURCE_RANGE -1000000                          // the amount of entries each text source has available

#define TEXT_SOURCE_TEXT_START      TEXT_SOURCE_RANGE
#define TEXT_SOURCE_TEXT_END        TEXT_SOURCE_RANGE*2 + 1

#define TEXT_SOURCE_CUSTOM_START    TEXT_SOURCE_RANGE*2
#define TEXT_SOURCE_CUSTOM_END      TEXT_SOURCE_RANGE*3 + 1

#define TEXT_SOURCE_GOSSIP_START    TEXT_SOURCE_RANGE*3
#define TEXT_SOURCE_GOSSIP_END      TEXT_SOURCE_RANGE*4 + 1

/**
 * @struct ScriptPointMove
 * @brief Structure to hold waypoint data for a creature.
 */
struct ScriptPointMove
{
    uint32 uiCreatureEntry;  ///< Entry ID of the creature
    uint32 uiPointId;        ///< ID of the waypoint
    float  fX;               ///< X coordinate of the waypoint
    float  fY;               ///< Y coordinate of the waypoint
    float  fZ;               ///< Z coordinate of the waypoint
    uint32 uiWaitTime;       ///< Wait time at the waypoint
};

/**
 * @class SystemMgr
 * @brief Manager class for handling script texts, gossip texts, and waypoints.
 */
class SystemMgr
{
    public:
        /**
         * @brief Constructor for SystemMgr.
         */
        SystemMgr();
        ~SystemMgr() {}

        /**
         * @brief Gets the singleton instance of SystemMgr.
         * @return Reference to the singleton instance.
         */
        static SystemMgr& Instance();

        typedef std::unordered_map<uint32, std::vector<ScriptPointMove> > PointMoveMap;

        /**
         * @brief Loads script texts from the database.
         */
        void LoadScriptTexts();

        /**
         * @brief Loads custom script texts from the database.
         */
        void LoadScriptTextsCustom();

        /**
         * @brief Loads script gossip texts from the database.
         */
        void LoadScriptGossipTexts();

        /**
         * @brief Loads script waypoints from the database.
         */
        void LoadScriptWaypoints();

        /**
         * @brief Gets the list of waypoints for a given creature entry.
         * @param uiCreatureEntry Entry ID of the creature.
         * @return Vector of ScriptPointMove containing the waypoints.
         */
        std::vector<ScriptPointMove> const& GetPointMoveList(uint32 uiCreatureEntry) const
        {
            static std::vector<ScriptPointMove> vEmpty;

            PointMoveMap::const_iterator itr = m_mPointMoveMap.find(uiCreatureEntry);

            if (itr == m_mPointMoveMap.end())
            {
                return vEmpty;
            }

            return itr->second;
        }

    protected:
        PointMoveMap    m_mPointMoveMap;                    // Map of creature entry IDs to their waypoints
};

#define pSystemMgr SystemMgr::Instance()

#endif
