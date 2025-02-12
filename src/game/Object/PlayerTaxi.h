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

#ifndef PLAYERTAXI_H
#define PLAYERTAXI_H

#include "Player.h"

#include "Common.h"
#include "DBCEnums.h"

class ByteBuffer;
struct FactionTemplateEntry;

class PlayerTaxi
{
    public:
        PlayerTaxi() : m_flightMasterFactionId(0) { memset(m_taximask, 0, sizeof(m_taximask)); }
        ~PlayerTaxi() { }

        // Nodes
        void InitTaxiNodes(uint32 race, uint32 chrClass, uint8 level);
        void InitTaxiNodesForClass(uint32 chrClass);
        void InitTaxiNodesForRace(uint32 race);
        void InitTaxiNodesForFaction(uint32 faction);
        void InitTaxiNodesForLvl(uint8 level);

        void LoadTaxiMask(const char* data);

        bool IsTaximaskNodeKnown(uint32 nodeidx) const
        {
            uint8 field   = uint8((nodeidx - 1) / 8);
            uint8 submask = 1 << ((nodeidx - 1) % 8);
            return (m_taximask[field] & submask) == submask;
        }

        bool SetTaximaskNode(uint32 nodeidx)
        {
            uint8 field   = uint8((nodeidx - 1) / 8);
            uint8 submask = 1 << ((nodeidx - 1) % 8);
            if ((m_taximask[field] & submask) != submask)
            {
                m_taximask[field] |= submask;
                return true;
            }
            else
            {
                return false;
            }
        }

        void AppendTaximaskTo(ByteBuffer& data, bool all);

        // Destinations
        bool LoadTaxiDestinationsFromString(const std::string& values, Team team);
        std::string SaveTaxiDestinationsToString();

        void ClearTaxiDestinations()
        {
            m_TaxiDestinations.clear();
        }

        void AddTaxiDestination(uint32 dest)
        {
            m_TaxiDestinations.push_back(dest);
        }

        uint32 GetTaxiSource() const
        {
            return m_TaxiDestinations.empty() ? 0 : m_TaxiDestinations.front();
        }

        uint32 GetTaxiDestination() const
        {
            return m_TaxiDestinations.size() < 2 ? 0 : m_TaxiDestinations[1];
        }

        uint32 GetCurrentTaxiPath() const;

        uint32 NextTaxiDestination()
        {
            m_TaxiDestinations.pop_front();
            return GetTaxiDestination();
        }

        bool empty() const
        {
            return m_TaxiDestinations.empty();
        }

        FactionTemplateEntry const* GetFlightMasterFactionTemplate() const;
        void SetFlightMasterFactionTemplateId(uint32 factionTemplateId)
        {
            m_flightMasterFactionId = factionTemplateId;
        }

        friend std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi);

    private:
        TaxiMask m_taximask;
        std::deque<uint32> m_TaxiDestinations;
        uint32 m_flightMasterFactionId;
};

std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi);

#endif
