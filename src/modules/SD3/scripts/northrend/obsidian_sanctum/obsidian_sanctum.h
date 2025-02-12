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

#ifndef DEF_OBSIDIAN_SANCTUM_H
#define DEF_OBSIDIAN_SANCTUM_H

enum
{
    MAX_ENCOUNTER               = 1,

    TYPE_SARTHARION_EVENT       = 1,
    // internal used types for achievement
    TYPE_ALIVE_DRAGONS          = 2,
    TYPE_VOLCANO_BLOW_FAILED    = 3,

    TYPE_DATA_PORTAL_ON         = 4,
    TYPE_DATA_PORTAL_OFF        = 5,
    TYPE_DATA_PORTAL_STATUS     = 6,

    DATA64_FIRE_CYCLONE         = 0,

    MAX_TWILIGHT_DRAGONS        = 3,

    TYPE_PORTAL_TENEBRON        = 0,
    TYPE_PORTAL_SHADRON         = 1,
    TYPE_PORTAL_VESPERON        = 2,

    NPC_SARTHARION              = 28860,
    NPC_TENEBRON                = 30452,
    NPC_SHADRON                 = 30451,
    NPC_VESPERON                = 30449,

    NPC_FIRE_CYCLONE            = 30648,

    GO_TWILIGHT_PORTAL          = 193988,

    // Achievement related
    ACHIEV_CRIT_VOLCANO_BLOW_N  = 7326,                     // achievs 2047, 2048 (Go When the Volcano Blows) -- This is individual achievement!
    ACHIEV_CRIT_VOLCANO_BLOW_H  = 7327,
    ACHIEV_DRAGONS_ALIVE_1_N    = 7328,                     // achievs 2049, 2052 (Twilight Assist)
    ACHIEV_DRAGONS_ALIVE_1_H    = 7331,
    ACHIEV_DRAGONS_ALIVE_2_N    = 7329,                     // achievs 2050, 2053 (Twilight Duo)
    ACHIEV_DRAGONS_ALIVE_2_H    = 7332,
    ACHIEV_DRAGONS_ALIVE_3_N    = 7330,                     // achievs 2051, 2054 (The Twilight Zone)
    ACHIEV_DRAGONS_ALIVE_3_H    = 7333,
};

#endif
