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

#ifndef DEF_ZULAMAN_H
#define DEF_ZULAMAN_H

enum InstanceZA
{
    MAX_ENCOUNTER           = 8,
    MAX_VENDOR              = 2,
    MAX_CHESTS              = 4,
    MAX_BEAR_WAVES          = 4,

    SAY_INST_RELEASE        = -1568067,                     // TODO Event NYI
    SAY_INST_BEGIN          = -1568068,
    SAY_INST_PROGRESS_1     = -1568069,
    SAY_INST_PROGRESS_2     = -1568070,
    SAY_INST_PROGRESS_3     = -1568071,
    SAY_INST_WARN_1         = -1568072,
    SAY_INST_WARN_2         = -1568073,
    SAY_INST_WARN_3         = -1568074,
    SAY_INST_WARN_4         = -1568075,
    SAY_INST_SACRIF1        = -1568076,
    SAY_INST_SACRIF2        = -1568077,
    SAY_INST_COMPLETE       = -1568078,

    // Bear event yells
    SAY_WAVE1_AGGRO         = -1568010,
    SAY_WAVE2_STAIR1        = -1568011,
    SAY_WAVE3_STAIR2        = -1568012,
    SAY_WAVE4_PLATFORM      = -1568013,

    WORLD_STATE_ID          = 3104,
    WORLD_STATE_COUNTER     = 3106,

    TYPE_EVENT_RUN          = 0,
    TYPE_AKILZON            = 1,
    TYPE_NALORAKK           = 2,
    TYPE_JANALAI            = 3,
    TYPE_HALAZZI            = 4,
    TYPE_MALACRASS          = 5,
    TYPE_ZULJIN             = 6,
    TYPE_RUN_EVENT_TIME     = 7,                            // Must be MAX_ENCOUNTER -1

    TYPE_RAND_VENDOR_1      = 8,
    TYPE_RAND_VENDOR_2      = 9,
    TYPE_BEAR_PHASE         = 10,

    NPC_AKILZON             = 23574,
    NPC_NALORAKK            = 23576,
    NPC_JANALAI             = 23578,
    NPC_HALAZZI             = 23577,
    NPC_MALACRASS           = 24239,
    NPC_ZULJIN              = 23863,

    // Narolakk event npcs
    NPC_MEDICINE_MAN        = 23581,
    NPC_TRIBES_MAN          = 23582,
    NPC_AXETHROWER          = 23542,
    NPC_WARBRINGER          = 23580,

    // Malacrass companions
    NPC_ALYSON              = 24240,
    NPC_THURG               = 24241,
    NPC_SLITHER             = 24242,
    NPC_RADAAN              = 24243,
    NPC_GAZAKROTH           = 24244,
    NPC_FENSTALKER          = 24245,
    NPC_DARKHEART           = 24246,
    NPC_KORAGG              = 24247,

    NPC_HARRISON            = 24358,
    // Time Run Event NPCs
    NPC_TANZAR              = 23790,                        // at bear
    NPC_KRAZ                = 24024,                        // at phoenix
    NPC_ASHLI               = 24001,                        // at lynx
    NPC_HARKOR              = 23999,                        // at eagle
    // unused (TODO or TODO with DB-tools)
    NPC_TANZAR_CORPSE       = 24442,
    NPC_KRAZ_CORPSE         = 24444,
    NPC_ASHIL_CORPSE        = 24441,
    NPC_HARKOR_CORPSE       = 24443,

    // Zul'jin event spirits
    NPC_BEAR_SPIRIT         = 23878,                        // They should all have aura 42466
    NPC_EAGLE_SPIRIT        = 23880,
    NPC_LYNX_SPIRIT         = 23877,
    NPC_DRAGONHAWK_SPIRIT   = 23879,

    GO_STRANGE_GONG         = 187359,
    GO_MASSIVE_GATE         = 186728,
    GO_WIND_DOOR            = 186858,
    GO_LYNX_TEMPLE_ENTRANCE = 186304,
    GO_LYNX_TEMPLE_EXIT     = 186303,
    GO_HEXLORD_ENTRANCE     = 186305,
    GO_WOODEN_DOOR          = 186306,
    GO_FIRE_DOOR            = 186859,

    GO_TANZARS_TRUNK        = 186648,
    GO_KRAZS_PACKAGE        = 186667,
    GO_ASHLIS_BAG           = 186672,
    GO_HARKORS_SATCHEL      = 187021,
};

struct NalorakkBearEventInfo
{
    int iYellId;
    float fX, fY, fZ, fO, fAggroDist;
};

static const NalorakkBearEventInfo aBearEventInfo[MAX_BEAR_WAVES] =
{
    {SAY_WAVE1_AGGRO,    0, 0, 0, 0, 45.0f},
    {SAY_WAVE2_STAIR1,   -54.948f, 1419.772f, 27.303f, 0.03f, 37.0f},
    {SAY_WAVE3_STAIR2,   -80.303f, 1372.622f, 40.764f, 1.67f, 35.0f},
    {SAY_WAVE4_PLATFORM, -77.495f, 1294.760f, 48.487f, 1.66f, 60.0f}
};

#endif
