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

#ifndef DEF_FORGE_OF_SOULS_H
#define DEF_FORGE_OF_SOULS_H

enum
{
    MAX_ENCOUNTER               = 2,
    TYPE_BRONJAHM               = 1,
    TYPE_DEVOURER_OF_SOULS      = 2,
    TYPE_ACHIEV_PHANTOM_BLAST   = 3,

    DATA_SOULFRAGMENT_REMOVE    = 4,                        // on Death and on Use

    NPC_BRONJAHM                = 36497,
    NPC_DEVOURER_OF_SOULS       = 36502,
    NPC_CORRUPTED_SOUL_FRAGMENT = 36535,

    // Event NPCs
    NPC_SILVANA_BEGIN           = 37596,
    NPC_SILVANA_END             = 38161,
    NPC_JAINA_BEGIN             = 37597,
    NPC_JAINA_END               = 38160,
    NPC_ARCHMAGE_ELANDRA        = 37774,
    NPC_ARCHMAGE_KORELN         = 37582,
    NPC_DARK_RANGER_KALIRA      = 37583,
    NPC_DARK_RANGER_LORALEN     = 37779,
    NPC_COLISEUM_CHAMPION_A_P   = 37498,                    // Alliance Paladin
    NPC_COLISEUM_CHAMPION_A_F   = 37496,                    // Alliance Footman
    NPC_COLISEUM_CHAMPION_A_M   = 37497,                    // Alliance Mage
    NPC_COLISEUM_CHAMPION_H_F   = 37584,                    // Horde Footman
    NPC_COLISEUM_CHAMPION_H_T   = 37587,                    // Horde Taure
    NPC_COLISEUM_CHAMPION_H_M   = 37588,                    // Horde Mage

    ACHIEV_CRIT_SOUL_POWER      = 12752,
    ACHIEV_CRIT_PHANTOM_BLAST   = 12976,
};
#endif
