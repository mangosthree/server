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

#ifndef DEF_UTG_KEEP_H
#define DEF_UTG_KEEP_H

enum
{
    MAX_ENCOUNTER               = 6,

    TYPE_KELESETH               = 0,
    TYPE_SKARVALD_DALRONN       = 1,
    TYPE_INGVAR                 = 2,
    TYPE_BELLOW_1               = 3,
    TYPE_BELLOW_2               = 4,
    TYPE_BELLOW_3               = 5,

    NPC_KELESETH                = 23953,
    NPC_SKARVALD                = 24200,
    NPC_DALRONN                 = 24201,
    NPC_INGVAR                  = 23954,

    NPC_FROST_TOMB              = 23965,

    GO_BELLOW_1                 = 186688,
    GO_BELLOW_2                 = 186689,
    GO_BELLOW_3                 = 186690,
    GO_FORGEFIRE_1              = 186692,
    GO_FORGEFIRE_2              = 186693,
    GO_FORGEFIRE_3              = 186691,
    GO_PORTCULLIS_COMBAT        = 186612,
    GO_PORTCULLIS_EXIT_1        = 186694,
    GO_PORTCULLIS_EXIT_2        = 186756,

    ACHIEV_CRIT_ON_THE_ROCKS    = 7231,
};
#endif
