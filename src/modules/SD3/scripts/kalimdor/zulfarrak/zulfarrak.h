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

#ifndef DEF_ZULFARRAK_H
#define DEF_ZULFARRAK_H

enum
{
    MAX_ENCOUNTER                   = 9,
    TYPE_SIGNAL                     = MAX_ENCOUNTER + 1,

    TYPE_VELRATHA                   = 0,
    TYPE_GAHZRILLA                  = 1,
    TYPE_ANTUSUL                    = 2,
    TYPE_THEKA                      = 3,
    TYPE_ZUMRAH                     = 4,
    TYPE_NEKRUM                     = 5,
    TYPE_SEZZZIZ                    = 6,
    TYPE_CHIEF_SANDSCALP            = 7,
    TYPE_PYRAMID_EVENT              = 8,

    NPC_VELRATHA                    = 7795,
    NPC_GAHZRILLA                   = 7273,
    NPC_ANTUSUL                     = 8127,
    NPC_THEKA                       = 7272,
    NPC_ZUMRAH                      = 7271,
    NPC_NEKRUM                      = 7796,
    NPC_SEZZZIZ                     = 7275,
    NPC_CHIEF_SANDSCALP             = 7267,

    NPC_SERGEANT_BLY                = 7604,
    NPC_SANDFURY_SLAVE              = 7787,
    NPC_SANDFURY_DRUDGE             = 7788,
    NPC_SANDFURY_CRETIN             = 7789,
    NPC_SANDFURY_ACOLYTE            = 8876,
    NPC_SANDFURY_ZEALOT             = 8877,

    GO_SHALLOW_GRAVE                = 128403,
    GO_END_DOOR                     = 146084,

    // EVENT_ID_GONG_ZULFARRAK      = 2488,                 // go 141832
    // EVENT_ID_UNLOCKING           = 2609,                 // spell 10738

    AREATRIGGER_ANTUSUL             = 1447,
};

#endif
