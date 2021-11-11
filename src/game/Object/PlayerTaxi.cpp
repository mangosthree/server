/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2021 MaNGOS <https://getmangos.eu>
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

#include "PlayerTaxi.h"

#include "Common.h"
#include "ObjectMgr.h"

PlayerTaxi::PlayerTaxi()
{
    // Taxi nodes
    memset(m_taximask, 0, sizeof(m_taximask));
}

void PlayerTaxi::InitTaxiNodes(uint32 race, uint32 chrClass, uint8 level)
{
    InitTaxiNodesForClass(chrClass);
    InitTaxiNodesForRace(race);
    InitTaxiNodesForFaction(Player::TeamForRace(race));
    InitTaxiNodesForLvl(level);
}

void PlayerTaxi::InitTaxiNodesForClass(uint32 chrClass)
{
    // class specific initial known nodes
    switch (chrClass)
    {
        case CLASS_DEATH_KNIGHT:
        {
            for (int i = 0; i < TaxiMaskSize; ++i)
            {
                m_taximask[i] |= sOldContinentsNodesMask[i];
            }
            break;
        }
    }
}

void PlayerTaxi::InitTaxiNodesForRace(uint32 race)
{
    // race specific initial known nodes: capital and taxi hub masks
    switch (race)
    {
        case RACE_HUMAN: // Human
            SetTaximaskNode(2);
            break;

        case RACE_ORC: // Orc
            SetTaximaskNode(23);
            break;

        case RACE_DWARF: // Dwarf
            SetTaximaskNode(6);
            break;

        case RACE_NIGHTELF: // Night Elf
            SetTaximaskNode(26);
            SetTaximaskNode(27);
            break;

        case RACE_UNDEAD: // Undead
            SetTaximaskNode(11);
            break;

        case RACE_TAUREN: // Tauren
            SetTaximaskNode(22);
            break;

        case RACE_GNOME: // Gnome
            SetTaximaskNode(6);
            break;

        case RACE_TROLL: // Troll
            SetTaximaskNode(23);
            break;

        case RACE_BLOODELF: // Blood Elf
            SetTaximaskNode(82);
            break;

        case RACE_DRAENEI: // Draenei
            SetTaximaskNode(94);
            break;
    }
}

void PlayerTaxi::InitTaxiNodesForFaction(uint32 faction)
{
    // new continent starting masks (It will be accessible only at new map)
    switch (faction)
    {
        case ALLIANCE:
            SetTaximaskNode(100); // Honor Hold
            SetTaximaskNode(245); // Valiance Keep
            break;

        case HORDE:
            SetTaximaskNode(99);  // Thrallmar
            SetTaximaskNode(257); // Warsong Hold
            break;

        default:
            break;
    }
}

void PlayerTaxi::InitTaxiNodesForLvl(uint8 level)
{
    // level dependent taxi hubs
    if (level >= 68) // Shattered Sun Staging Area
    {
        SetTaximaskNode(213);
    }

    if (level >= 78) // Dalaran
    {
        SetTaximaskNode(310);
    }
}

void PlayerTaxi::LoadTaxiMask(const char* data)
{
    Tokens tokens = StrSplit(data, " ");

    int index;
    Tokens::iterator iter;
    for (iter = tokens.begin(), index = 0; (index < TaxiMaskSize) && (iter != tokens.end()); ++iter, ++index)
    {
        // load and set bits only for existing taxi nodes
        m_taximask[index] = sTaxiNodesMask[index] & uint8(std::stoul((*iter).c_str()));
    }
}

void PlayerTaxi::AppendTaximaskTo(ByteBuffer& data, bool all)
{
    data << uint32(TaxiMaskSize);
    if (all)
    {
        for (uint8 i = 0; i < TaxiMaskSize; ++i)
        {
            data << uint8(sTaxiNodesMask[i]);               // all existing nodes
        }
    }
    else
    {
        for (uint8 i = 0; i < TaxiMaskSize; ++i)
        {
            data << uint8(m_taximask[i]);                   // known nodes
        }
    }
}

bool PlayerTaxi::LoadTaxiDestinationsFromString(const std::string& values, Team team)
{
    ClearTaxiDestinations();

    Tokens tokens = StrSplit(values, " ");

    for (Tokens::iterator iter = tokens.begin(); iter != tokens.end(); ++iter)
    {
        uint32 node = std::stoul(iter->c_str());
        AddTaxiDestination(node);
    }

    if (m_TaxiDestinations.empty())
    {
        return true;
    }

    // Check integrity
    if (m_TaxiDestinations.size() < 2)
    {
        return false;
    }

    for (size_t i = 1; i < m_TaxiDestinations.size(); ++i)
    {
        uint32 cost;
        uint32 path;
        sObjectMgr.GetTaxiPath(m_TaxiDestinations[i - 1], m_TaxiDestinations[i], path, cost);
        if (!path)
        {
            return false;
        }
    }

    // can't load taxi path without mount set (quest taxi path?)
    if (!sObjectMgr.GetTaxiMountDisplayId(GetTaxiSource(), team, true))
    {
        return false;
    }

    return true;
}

std::string PlayerTaxi::SaveTaxiDestinationsToString()
{
    if (m_TaxiDestinations.empty())
    {
        return "";
    }

    std::ostringstream ss;

    for (size_t i = 0; i < m_TaxiDestinations.size(); ++i)
    {
        ss << m_TaxiDestinations[i] << " ";
    }

    return ss.str();
}

uint32 PlayerTaxi::GetCurrentTaxiPath() const
{
    if (m_TaxiDestinations.size() < 2)
    {
        return 0;
    }

    uint32 path;
    uint32 cost;

    sObjectMgr.GetTaxiPath(m_TaxiDestinations[0], m_TaxiDestinations[1], path, cost);

    return path;
}

std::ostringstream& operator<< (std::ostringstream& ss, PlayerTaxi const& taxi)
{
    for (int i = 0; i < TaxiMaskSize; ++i)
    {
        ss << uint32(taxi.m_taximask[i]) << " ";    // cast to prevent conversion to char
    }
    return ss;
}
