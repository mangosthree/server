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

#ifndef DEF_ICECROWN_PIT_H
#define DEF_ICECROWN_PIT_H

enum
{
    MAX_ENCOUNTER                   = 4,
    MAX_SPECIAL_ACHIEV_CRITS        = 2,

    TYPE_GARFROST                   = 0,
    TYPE_KRICK                      = 1,
    TYPE_TYRANNUS                   = 2,
    TYPE_AMBUSH                     = 3,
    TYPE_DATA_PLAYER_TEAM           = MAX_ENCOUNTER,
    TYPE_ACHIEV_DOESNT_GO_ELEVEN    = MAX_ENCOUNTER + 1,    //order to be met in ScriptedInstance
    TYPE_ACHIEV_DONT_LOOK_UP        = MAX_ENCOUNTER + 2,

    // Bosses
    NPC_TYRANNUS_INTRO              = 36794,
    NPC_GARFROST                    = 36494,
    NPC_KRICK                       = 36477,
    NPC_ICK                         = 36476,
    NPC_TYRANNUS                    = 36658,
    NPC_RIMEFANG                    = 36661,
    NPC_SINDRAGOSA                  = 37755,

    // Intro part npcs
    NPC_SYLVANAS_PART1              = 36990,
    NPC_SYLVANAS_PART2              = 38189,
    NPC_JAINA_PART1                 = 36993,
    NPC_JAINA_PART2                 = 38188,
    NPC_KILARA                      = 37583,
    NPC_ELANDRA                     = 37774,
    NPC_LORALEN                     = 37779,
    NPC_KORELN                      = 37582,
    NPC_CHAMPION_1_HORDE            = 37584,
    NPC_CHAMPION_2_HORDE            = 37587,
    NPC_CHAMPION_3_HORDE            = 37588,
    NPC_CHAMPION_1_ALLIANCE         = 37496,
    NPC_CHAMPION_2_ALLIANCE         = 37497,
    NPC_CHAMPION_3_ALLIANCE         = 37498,
    NPC_CORRUPTED_CHAMPION          = 36796,

    // Enslaved npcs
    NPC_IRONSKULL_PART1             = 37592,
    NPC_IRONSKULL_PART2             = 37581,
    NPC_VICTUS_PART1                = 37591,
    NPC_VICTUS_PART2                = 37580,
    NPC_FREE_HORDE_SLAVE_1          = 37577,
    NPC_FREE_HORDE_SLAVE_2          = 37578,
    NPC_FREE_HORDE_SLAVE_3          = 37579,
    NPC_FREE_ALLIANCE_SLAVE_1       = 37572,
    NPC_FREE_ALLIANCE_SLAVE_2       = 37575,
    NPC_FREE_ALLIANCE_SLAVE_3       = 37576,

    // Ambush npcs
    NPC_YMIRJAR_DEATHBRINGER        = 36892,
    NPC_YMIRJAR_WRATHBRINGER        = 36840,
    NPC_YMIRJAR_FLAMEBEARER         = 36893,
    NPC_FALLEN_WARRIOR              = 36841,
    NPC_COLDWRAITH                  = 36842,
    NPC_STALKER                     = 32780,
    NPC_GENERAL_BUNNY               = 24110,

    GO_ICEWALL                      = 201885,               // open after gafrost/krick
    GO_HALLS_OF_REFLECT_PORT        = 201848,               // unlocked by jaina/sylvanas at last outro

    AREATRIGGER_ID_TUNNEL_START     = 5578,
    AREATRIGGER_ID_TUNNEL_END       = 5581,

    ACHIEV_CRIT_DOESNT_GO_ELEVEN    = 12993,                // Garfrost, achiev 4524
    ACHIEV_CRIT_DONT_LOOK_UP        = 12994,                // Gauntlet, achiev 4525
};
#endif
