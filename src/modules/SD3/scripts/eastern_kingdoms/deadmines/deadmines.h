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

#ifndef DEF_DEADMINES_H
#define DEF_DEADMINES_H

enum
{
    MAX_ENCOUNTER           = 4,

    TYPE_RHAHKZOR           = 0,
    TYPE_SNEED              = 1,
    TYPE_GILNID             = 2,
    TYPE_IRON_CLAD_DOOR     = 3,

    INST_SAY_ALARM1         = -1036000,
    INST_SAY_ALARM2         = -1036001,

    GO_FACTORY_DOOR         = 13965,                        // rhahk'zor
    GO_MAST_ROOM_DOOR       = 16400,                        // sneed
    GO_FOUNDRY_DOOR         = 16399,                        // gilnid
    GO_HEAVY_DOOR_1         = 17153,                        // to sneed
    GO_HEAVY_DOOR_2         = 17154,                        // to gilnid
    GO_IRON_CLAD_DOOR       = 16397,
    GO_DEFIAS_CANNON        = 16398,
    GO_SMITE_CHEST          = 144111,                       // use to get correct location of mr.smites equipment changes
    GO_MYSTERIOUS_CHEST     = 180024,                       // used for quest 7938; spawns in the instance only if one of the players has the quest

    NPC_RHAHKZOR            = 644,
    NPC_SNEED               = 643,
    NPC_GILNID              = 1763,
    NPC_MR_SMITE            = 646,
    NPC_PIRATE              = 657,
    NPC_SQUALLSHAPER        = 1732,

    QUEST_FORTUNE_AWAITS    = 7938,
};

#endif
