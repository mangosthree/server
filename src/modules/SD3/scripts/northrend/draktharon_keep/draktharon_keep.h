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

#ifndef DEF_DRAKTHARON_KEEP_H
#define DEF_DRAKTHARON_KEEP_H

enum
{
    MAX_ENCOUNTER                   = 4,

    TYPE_TROLLGORE                  = 0,
    TYPE_NOVOS                      = 1,
    TYPE_KING_DRED                  = 2,
    TYPE_THARONJA                   = 3,
    TYPE_DATA_NOVOS_CRYSTAL_INDEX   = MAX_ENCOUNTER,
    TYPE_DO_TROLLGORE               = MAX_ENCOUNTER + 1,

    DATA64_NOVOS_SUMMON_DUMMY       = 0,
    DATA64_NOVOS_CRYSTAL_HANDLER    = 1,

    NPC_NOVOS                       = 26631,
    NPC_KING_DRED                   = 27483,
    NPC_TROLLGORE                   = 26630,

    // Adds of King Dred Encounter - deaths counted for achievement
    NPC_DRAKKARI_GUTRIPPER          = 26641,
    NPC_DRAKKARI_SCYTHECLAW         = 26628,
    NPC_WORLD_TRIGGER               = 22515,

    // Novos Encounter
    SPELL_BEAM_CHANNEL              = 52106,
    SPELL_CRYSTAL_HANDLER_DEATH_1   = 47336,
    SPELL_CRYSTAL_HANDLER_DEATH_2   = 55801,
    SPELL_CRYSTAL_HANDLER_DEATH_3   = 55803,
    SPELL_CRYSTAL_HANDLER_DEATH_4   = 55805,

    MAX_CRYSTALS                    = 4,
    NPC_CRYSTAL_CHANNEL_TARGET      = 26712,
    GO_CRYSTAL_SW                   = 189299,
    GO_CRYSTAL_NE                   = 189300,
    GO_CRYSTAL_NW                   = 189301,
    GO_CRYSTAL_SE                   = 189302,
};

static const uint32 aCrystalHandlerDeathSpells[MAX_CRYSTALS] =
{
    SPELL_CRYSTAL_HANDLER_DEATH_1, SPELL_CRYSTAL_HANDLER_DEATH_2, SPELL_CRYSTAL_HANDLER_DEATH_3, SPELL_CRYSTAL_HANDLER_DEATH_4
};

#endif