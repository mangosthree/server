/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2023 MaNGOS <https://getmangos.eu>
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

#ifndef DEF_BLACKROCK_SPIRE_H
#define DEF_BLACKROCK_SPIRE_H

enum
{
    MAX_ENCOUNTER               = 6,
    MAX_ROOMS                   = 7,
    DO_ACTIVATE_RUNES           = 0,
    DO_SORT_MOBS                = 1,
    DO_EMBERSEER_EVENT          = 2,
    DO_FLAMEWREATH_EVENT        = 3,

    TYPE_ROOM_EVENT             = 0,
    TYPE_EMBERSEER              = 1,
    TYPE_FLAMEWREATH            = 2,                        // Only summon once per instance
    TYPE_STADIUM                = 3,
    TYPE_DRAKKISATH             = 4,
    TYPE_VALTHALAK              = 5,                        // Only summon once per instance

    NPC_SCARSHIELD_INFILTRATOR  = 10299,
    NPC_BLACKHAND_SUMMONER      = 9818,
    NPC_BLACKHAND_VETERAN       = 9819,
    NPC_PYROGUARD_EMBERSEER     = 9816,
    NPC_SOLAKAR_FLAMEWREATH     = 10264,
    NPC_BLACKHAND_INCARCERATOR  = 10316,
    NPC_LORD_VICTOR_NEFARIUS    = 10162,
    NPC_REND_BLACKHAND          = 10429,
    NPC_GYTH                    = 10339,
    NPC_DRAKKISATH              = 10363,
    NPC_CHROMATIC_WHELP         = 10442,                    // related to Gyth arena event
    NPC_CHROMATIC_DRAGON        = 10447,
    NPC_BLACKHAND_HANDLER       = 10742,
    NPC_THE_BEAST               = 10430,

    // Doors
    GO_EMBERSEER_IN             = 175244,
    GO_DOORS                    = 175705,
    GO_EMBERSEER_OUT            = 175153,
    GO_FATHER_FLAME             = 175245,
    GO_GYTH_ENTRY_DOOR          = 164726,
    GO_GYTH_COMBAT_DOOR         = 175185,
    GO_GYTH_EXIT_DOOR           = 175186,
    GO_DRAKKISATH_DOOR_1        = 175946,
    GO_DRAKKISATH_DOOR_2        = 175947,


#if defined (CLASSIC)
    // upper spire entrance
    GO_DRAGONSPINE              = 164725,
    GO_BRAZIER_1                = 175528,
    GO_BRAZIER_2                = 175529,
    GO_BRAZIER_3                = 175530,
    GO_BRAZIER_4                = 175531,
    GO_BRAZIER_5                = 175532,
    GO_BRAZIER_6                = 175533,
#endif

    GO_ROOM_7_RUNE              = 175194,
    GO_ROOM_3_RUNE              = 175195,
    GO_ROOM_6_RUNE              = 175196,
    GO_ROOM_1_RUNE              = 175197,
    GO_ROOM_5_RUNE              = 175198,
    GO_ROOM_2_RUNE              = 175199,
    GO_ROOM_4_RUNE              = 175200,

    GO_ROOKERY_EGG              = 175124,

    GO_EMBERSEER_RUNE_1         = 175266,
    GO_EMBERSEER_RUNE_2         = 175267,
    GO_EMBERSEER_RUNE_3         = 175268,
    GO_EMBERSEER_RUNE_4         = 175269,
    GO_EMBERSEER_RUNE_5         = 175270,
    GO_EMBERSEER_RUNE_6         = 175271,
    GO_EMBERSEER_RUNE_7         = 175272,

#if defined (CLASSIC)
    ITEM_SEAL_OF_ASCENSION      = 12344,
#endif

    MAX_STADIUM_WAVES           = 7,
    MAX_STADIUM_MOBS_PER_WAVE   = 5,

    FACTION_BLACK_DRAGON        = 103,

    SPELL_ENCAGE_EMBERSEER      = 15281,    // cast by Blackhand Incarcerator
    SPELL_FINKLE_IS_EINHORN     = 16710     // cast by The Beast when skinned. Summons Finkle Einhorn.
};

struct SpawnLocation
{
    float m_fX, m_fY, m_fZ, m_fO;
};

static const SpawnLocation aStadiumLocs[7] =
{
    {210.00f, -420.30f, 110.94f, 3.14f},                    // dragons summon location
    {210.14f, -397.54f, 111.1f, 0.0f},                      // Gyth summon location
    {163.62f, -420.33f, 110.47f, 0.0f},                     // center of the stadium location (for movement)
    {164.63f, -444.04f, 121.97f, 3.22f},                    // Lord Nefarius summon position
    {161.01f, -443.73f, 121.97f, 6.26f},                    // Rend summon position
    {164.64f, -443.30f, 121.97f, 1.61f},                    // Nefarius move position
    {165.74f, -466.46f, 116.80f, 0.0f},                     // Rend move position
};

// Stadium event description
static const uint32 aStadiumEventNpcs[MAX_STADIUM_WAVES][5] =
{
    {NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_DRAGON, 0},
    {NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_DRAGON, 0},
    {NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_DRAGON, NPC_BLACKHAND_HANDLER, 0},
    {NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_DRAGON, NPC_BLACKHAND_HANDLER, 0},
    {NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_DRAGON, NPC_BLACKHAND_HANDLER},
    {NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_DRAGON, NPC_CHROMATIC_DRAGON, NPC_BLACKHAND_HANDLER},
    {NPC_CHROMATIC_WHELP, NPC_CHROMATIC_WHELP, NPC_CHROMATIC_DRAGON, NPC_CHROMATIC_DRAGON, NPC_BLACKHAND_HANDLER},
};

#endif
