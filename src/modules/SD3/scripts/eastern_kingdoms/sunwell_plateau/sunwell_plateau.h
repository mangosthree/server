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

#ifndef DEF_SUNWELLPLATEAU_H
#define DEF_SUNWELLPLATEAU_H

enum
{
    MAX_ENCOUNTER               = 6,

    TYPE_KALECGOS               = 0,
    TYPE_BRUTALLUS              = 1,
    TYPE_FELMYST                = 2,
    TYPE_EREDAR_TWINS           = 3,
    TYPE_MURU                   = 4,
    TYPE_KILJAEDEN              = 5,
    TYPE_FELMYST_TRIGGER_RIGHT  = MAX_ENCOUNTER,
    TYPE_FELMYST_TRIGGER_LEFT   = MAX_ENCOUNTER+1,
    TYPE_FELMYST_TRIGGER        = MAX_ENCOUNTER+2,
    TYPE_KALECGOS_EJECT_SPECTRAL= MAX_ENCOUNTER+3,

    NPC_KALECGOS_DRAGON         = 24850,            // kalecgos blue dragon hostile
    NPC_KALECGOS_HUMAN          = 24891,            // kalecgos human form in spectral realm
    NPC_SATHROVARR              = 24892,
    NPC_MADRIGOSA               = 24895,
    NPC_FLIGHT_TRIGGER_LEFT     = 25357,            // Related to Felmyst flight path. Also the anchor to summon Madrigosa
    NPC_FLIGHT_TRIGGER_RIGHT    = 25358,            // related to Felmyst flight path
    NPC_WORLD_TRIGGER           = 22515,
    NPC_WORLD_TRIGGER_LARGE     = 23472,            // ground triggers spawned in Brutallus / Felmyst arena
    NPC_BRUTALLUS               = 24882,
    NPC_FELMYST                 = 25038,
    NPC_KALECGOS_MADRIGOSA      = 24844,            // kalecgos blue dragon; spawns after Felmyst
    NPC_ALYTHESS                = 25166,
    NPC_SACROLASH               = 25165,
    NPC_MURU                    = 25741,
    NPC_ENTROPIUS               = 25840,
    NPC_DECEIVER                = 25588,
    NPC_KILJAEDEN               = 25315,
    NPC_KILJAEDEN_CONTROLLER    = 25608,            // kiljaeden event controller
    NPC_ANVEENA                 = 26046,            // related to kiljaeden event
    NPC_KALECGOS                = 25319,            // related to kiljaeden event
    NPC_VELEN                   = 26246,
    NPC_LIADRIN                 = 26247,

    GO_FORCEFIELD               = 188421,           // kalecgos door + collisions
    GO_BOSS_COLLISION_1         = 188523,
    GO_BOSS_COLLISION_2         = 188524,
    GO_ICE_BARRIER              = 188119,           // used to block the players path during the Brutallus intro event
    GO_FIRE_BARRIER             = 188075,           // door after felmyst
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    GO_FIRST_GATE               = 187766,           // door between felmyst and eredar twins
    GO_SECOND_GATE              = 187764,           // door after eredar twins
#endif
    GO_MURU_ENTER_GATE          = 187990,           // muru gates
    GO_MURU_EXIT_GATE           = 188118,
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    GO_THIRD_GATE               = 187765,           // door after muru; why another?
#endif

    GO_ORB_BLUE_FLIGHT_1        = 188115,           // orbs used in the Kil'jaeden fight
    GO_ORB_BLUE_FLIGHT_2        = 188116,
    GO_ORB_BLUE_FLIGHT_3        = 187869,
    GO_ORB_BLUE_FLIGHT_4        = 188114,

    SAY_KALECGOS_OUTRO          = -1580043,
    SAY_TWINS_INTRO             = -1580044,

    // Kil'jaeden yells
    SAY_ORDER_1                 = -1580064,
    SAY_ORDER_2                 = -1580065,
    SAY_ORDER_3                 = -1580066,
    SAY_ORDER_4                 = -1580067,
    SAY_ORDER_5                 = -1580068,

    AREATRIGGER_TWINS           = 4937,

    // Kalec spectral realm spells
    SPELL_TELEPORT_NORMAL_REALM = 46020,
    SPELL_SPECTRAL_REALM_AURA   = 46021,
    SPELL_SPECTRAL_EXHAUSTION   = 44867,
    // Felmyst ouro spell
    SPELL_OPEN_BACK_DOOR        = 46650,            // Opens the fire barrier - script effect for 46652
    // used by both muru and entropius
    SPELL_MURU_BERSERK          = 26662,
    // visuals for Kiljaeden encounter
    SPELL_ANVEENA_DRAIN         = 46410,

    MAX_DECEIVERS               = 3
};

struct EventLocations
{
    float m_fX, m_fY, m_fZ, m_fO;
};

static const EventLocations aMadrigosaLoc[] =
{
    {1463.82f, 661.212f, 19.79f, 4.88f},            // reload spawn loc - the place where to spawn Felmyst
#if defined (CLASSIC) || defined (TBC)
    {1463.82f, 661.212f, 39.234f},                  // fly loc during the cinematig
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    {1463.82f, 661.212f, 39.234f, 0}                // fly loc during the cinematig
#endif
};
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
static const float afMuruSpawnLoc[4] = { 1816.25f, 625.484f, 69.603f, 5.624f };
#endif
#endif
