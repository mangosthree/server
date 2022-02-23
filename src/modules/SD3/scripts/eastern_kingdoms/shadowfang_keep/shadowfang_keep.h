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

#ifndef DEF_SHADOWFANG_H
#define DEF_SHADOWFANG_H

enum
{
#if defined (CLASSIC) || defined (TBC)
    MAX_ENCOUNTER           = 6,
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    MAX_ENCOUNTER           = 7,
#endif

    TYPE_FREE_NPC           = 1,
    TYPE_RETHILGORE         = 2,
    TYPE_FENRUS             = 3,
    TYPE_NANDOS             = 4,
    TYPE_INTRO              = 5,
    TYPE_VOIDWALKER         = 6,
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    TYPE_APOTHECARY         = 7,
#endif

    SAY_BOSS_DIE_AD         = -1033007,
    SAY_BOSS_DIE_AS         = -1033008,

    NPC_ASH                 = 3850,
    NPC_ADA                 = 3849,
    //  NPC_ARUGAL              = 10000,                    //"Arugal" says intro text, not used
    NPC_ARCHMAGE_ARUGAL     = 4275,                         //"Archmage Arugal" does Fenrus event
    NPC_FENRUS              = 4274,                         // used to summon Arugal in Fenrus event
    NPC_VINCENT             = 4444,                         // Vincent should be "dead" is Arugal is done the intro already

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    NPC_HUMMEL              = 36296,                        // Love is in the Air event
    NPC_FRYE                = 36272,
    NPC_BAXTER              = 36565,
    NPC_VALENTINE_BOSS_MGR  = 36643,                        // controller npc for the apothecary event
    NPC_APOTHECARY_GENERATOR = 36212,                       // the npc which summons the crazed apothecary
#endif
    GO_COURTYARD_DOOR       = 18895,                        // door to open when talking to NPC's
    GO_SORCERER_DOOR        = 18972,                        // door to open when Fenrus the Devourer
    GO_ARUGAL_DOOR          = 18971,                        // door to open when Wolf Master Nandos
    GO_ARUGAL_FOCUS         = 18973,                        // this generates the lightning visual in the Fenrus event
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    GO_APOTHECARE_VIALS     = 190678,                       // move position for Baxter
    GO_CHEMISTRY_SET        = 200335,                       // move position for Frye

    SAY_HUMMEL_DEATH        = -1033025,

    MAX_APOTHECARY          = 3,
#endif
};

#endif
