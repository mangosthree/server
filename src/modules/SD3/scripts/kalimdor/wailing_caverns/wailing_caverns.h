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

#ifndef DEF_WAILING_CAVERNS_H
#define DEF_WAILING_CAVERNS_H

enum
{
    MAX_ENCOUNTER   = 6,

    TYPE_ANACONDRA  = 0,
    TYPE_COBRAHN    = 1,
    TYPE_PYTHAS     = 2,
    TYPE_SERPENTIS  = 3,
    TYPE_DISCIPLE   = 4,
    TYPE_MUTANUS    = 5,

    NPC_NARALEX     = 3679,
    NPC_DISCIPLE    = 3678,

    SAY_INTRO       = -1043000,                             // Say when the first 4 encounter are DONE

    GO_MYSTERIOUS_CHEST     = 180055,                       // used for quest 7944; spawns in the instance only if one of the players has the quest

    QUEST_FORTUNE_AWAITS    = 7944,
};

#endif
