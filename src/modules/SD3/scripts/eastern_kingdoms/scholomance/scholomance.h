/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2022 MaNGOS <https://getmangos.eu>
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

#ifndef DEF_SCHOLOMANCE_H
#define DEF_SCHOLOMANCE_H

enum
{
    MAX_ENCOUNTER           = 10,
    TYPE_SIGNAL             = MAX_ENCOUNTER,
    MAX_EVENTS              = 6,

    TYPE_KIRTONOS           = 0,
    TYPE_RATTLEGORE         = 1,
    TYPE_RAS_FROSTWHISPER   = 2,
    TYPE_MALICIA            = 3,
    TYPE_THEOLEN            = 4,
    TYPE_POLKELT            = 5,
    TYPE_RAVENIAN           = 6,
    TYPE_ALEXEI_BAROV       = 7,
    TYPE_ILLUCIA_BAROV      = 8,
    TYPE_GANDLING           = 9,

    NPC_KIRTONOS            = 10506,
    NPC_RATTLEGORE          = 11622,
    NPC_RAS_FROSTWHISPER    = 10508,
    NPC_THEOLEN_KRASTINOV   = 11261,
    NPC_LOREKEEPER_POLKELT  = 10901,
    NPC_RAVENIAN            = 10507,
    NPC_ILLUCIA_BAROV       = 10502,
    NPC_ALEXEI_BAROV        = 10504,
    NPC_INSTRUCTOR_MALICIA  = 10505,
    NPC_DARKMASTER_GANDLING = 1853,
    NPC_BONE_MINION         = 16119,                        // summoned in random rooms by gandling

    GO_GATE_KIRTONOS        = 175570,
    GO_VIEWING_ROOM_DOOR    = 175167,                       // Must be opened in reload case
    GO_GATE_RAS             = 177370,
    GO_GATE_MALICIA         = 177375,
    GO_GATE_THEOLEN         = 177377,
    GO_GATE_POLKELT         = 177376,
    GO_GATE_RAVENIAN        = 177372,
    GO_GATE_BAROV           = 177373,
    GO_GATE_ILLUCIA         = 177371,
    GO_GATE_GANDLING        = 177374,

    // Because the shadow portal teleport coordinates are guesswork (taken from old script) these IDs might be randomized
    // TODO Syncronise with correct DB coordinates when they will be known
    EVENT_ID_POLKELT        = 5618,
    EVENT_ID_THEOLEN        = 5619,
    EVENT_ID_MALICIA        = 5620,
    EVENT_ID_ILLUCIA        = 5621,
    EVENT_ID_BAROV          = 5622,
    EVENT_ID_RAVENIAN       = 5623,

    SAY_GANDLING_SPAWN      = -1289000,
};

struct SpawnLocation
{
    float m_fX, m_fY, m_fZ, m_fO;
};

static const SpawnLocation aGandlingSpawnLocs[1] =
{
    {180.771f, -5.4286f, 75.5702f, 1.29154f}
};

struct GandlingEventData
{
    GandlingEventData() : m_bIsActive(false) {}
    bool m_bIsActive;
    ObjectGuid m_doorGuid;
    std::set<uint32> m_sAddGuids;
};

static const uint32 aGandlingEvents[MAX_EVENTS] = {EVENT_ID_POLKELT, EVENT_ID_THEOLEN, EVENT_ID_MALICIA, EVENT_ID_ILLUCIA, EVENT_ID_BAROV, EVENT_ID_RAVENIAN};

typedef std::map<uint32, GandlingEventData> GandlingEventMap;

#endif
