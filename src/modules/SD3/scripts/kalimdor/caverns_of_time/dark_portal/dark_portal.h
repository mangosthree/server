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

#ifndef DEF_DARKPORTAL_H
#define DEF_DARKPORTAL_H

enum
{
    MAX_ENCOUNTER           = 6,

    TYPE_MEDIVH             = 0,
    TYPE_SHIELD             = 1,
    TYPE_TIME_RIFT          = 2,
    TYPE_CHRONO_LORD        = 3,
    TYPE_TEMPORUS           = 4,
    TYPE_AEONUS             = 5,

    TYPE_HANDLE_AREATRIGGER = MAX_ENCOUNTER,
    TYPE_RIFT_ID            = MAX_ENCOUNTER+1,

    WORLD_STATE_ID          = 2541,
    WORLD_STATE_SHIELD      = 2540,
    WORLD_STATE_RIFT        = 2784,

    QUEST_OPENING_PORTAL    = 10297,
    QUEST_MASTER_TOUCH      = 9836,

    // event controlers
    NPC_TIME_RIFT           = 17838,
    NPC_MEDIVH              = 15608,

    // main bosses
    NPC_CHRONO_LORD_DEJA    = 17879,
    NPC_TEMPORUS            = 17880,
    NPC_AEONUS              = 17881,

    // boss replacements for heroic
    NPC_CHRONO_LORD         = 21697,
    NPC_TIMEREAVER          = 21698,

    // portal guardians
    NPC_RIFT_KEEPER         = 21104,
    NPC_RIFT_LORD           = 17839,

    // portal summons
    NPC_ASSASSIN            = 17835,
    NPC_WHELP               = 21818,
    NPC_CHRONOMANCER        = 17892,
    NPC_EXECUTIONER         = 18994,
    NPC_VANQUISHER          = 18995,

    // additional npcs
    NPC_COUNCIL_ENFORCER    = 17023,
    NPC_TIME_KEEPER         = 17918,
    NPC_SAAT                = 20201,
    NPC_DARK_PORTAL_DUMMY   = 18625,            // cast spell 32564 on coordinates
    NPC_DARK_PORTAL_BEAM    = 18555,            // purple beams which travel from Medivh to the Dark Portal

    // event spells
    SPELL_RIFT_CHANNEL      = 31387,            // Aura channeled by the Time Rifts on the Rift Keepers

    SPELL_BANISH_HELPER     = 31550,            // used by the main bosses to banish the time keeprs
    SPELL_CORRUPT_AEONUS    = 37853,            // used by Aeonus to corrupt Medivh

    // cosmetic spells
    SPELL_PORTAL_CRYSTAL    = 32564,            // summons 18553 - Dark Portal Crystal stalker
    SPELL_BANISH_GREEN      = 32567,

    // yells during the event
    SAY_SAAT_WELCOME        = -1269019,

    SAY_MEDIVH_INTRO        = -1269021,
    SAY_MEDIVH_ENTER        = -1269020,
    SAY_MEDIVH_WIN          = -1269026,
    SAY_MEDIVH_WEAK75       = -1269022,
    SAY_MEDIVH_WEAK50       = -1269023,
    SAY_MEDIVH_WEAK25       = -1269024,
    SAY_ORCS_ENTER          = -1269027,
    SAY_ORCS_ANSWER         = -1269028,

    AREATRIGGER_MEDIVH      = 4288,
    AREATRIGGER_ENTER       = 4485,
};

#endif
