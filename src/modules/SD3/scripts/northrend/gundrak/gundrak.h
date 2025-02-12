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

#ifndef DEF_GUNDRAK_H
#define DEF_GUNDRAK_H
/* Encounters
 * Slad'ran          = 0
 * Moorabi           = 1
 * Drakkari Colossus = 2
 * Gal'darah         = 3
 * Eck the Ferocious = 4
*/
enum
{
    MAX_ENCOUNTER          = 5,
    MIN_LOVE_SHARE_PLAYERS = 5,

    TYPE_SLADRAN           = 0,
    TYPE_MOORABI           = 1,
    TYPE_COLOSSUS          = 2,
    TYPE_GALDARAH          = 3,
    TYPE_ECK               = 4,

    // Used to handle achievements
    TYPE_ACHIEV_WHY_SNAKES = 5,
    TYPE_ACHIEV_SHARE_LOVE = 6,
    TYPE_ACHIEV_LESS_RABI  = 7,

    NPC_SLADRAN            = 29304,
    NPC_MOORABI            = 29305,
    NPC_COLOSSUS           = 29307,
    NPC_ELEMENTAL          = 29573,
    NPC_LIVIN_MOJO         = 29830,
    NPC_GALDARAH           = 29306,
    NPC_ECK                = 29932,
    NPC_INVISIBLE_STALKER  = 30298,                         // Caster and Target for visual spells on altar use
    NPC_SLADRAN_SUMMON_T   = 29682,

    GO_ECK_DOOR            = 192632,
    GO_ECK_UNDERWATER_DOOR = 192569,
    GO_GALDARAH_DOOR       = 192568,
    GO_EXIT_DOOR_L         = 193208,
    GO_EXIT_DOOR_R         = 193209,

    GO_ALTAR_OF_SLADRAN    = 192518,
    GO_ALTAR_OF_MOORABI    = 192519,
    GO_ALTAR_OF_COLOSSUS   = 192520,

    GO_SNAKE_KEY           = 192564,
    GO_TROLL_KEY           = 192567,
    GO_MAMMOTH_KEY         = 192565,
    GO_RHINO_KEY           = 192566,

    GO_BRIDGE              = 193188,
    GO_COLLISION           = 192633,

    SPELL_BEAM_MAMMOTH     = 57068,
    SPELL_BEAM_SNAKE       = 57071,
    SPELL_BEAM_ELEMENTAL   = 57072,

    TIMER_VISUAL_ALTAR     = 3000,
    TIMER_VISUAL_BEAM      = 2500,
    TIMER_VISUAL_KEY       = 2000,

    ACHIEV_CRIT_LESS_RABI  = 7319,              // Moorabi achiev 2040
    ACHIEV_CRIT_WHY_SNAKES = 7363,              // Sladran achiev 2058
    ACHIEV_CRIT_SHARE_LOVE = 7583,              // Galdarah achiev 2152
};

#endif
