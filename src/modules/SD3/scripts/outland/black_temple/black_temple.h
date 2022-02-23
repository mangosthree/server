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

#ifndef DEF_BLACK_TEMPLE_H
#define DEF_BLACK_TEMPLE_H

enum
{
    MAX_ENCOUNTER                   = 9,

    TYPE_NAJENTUS                   = 0,
    TYPE_SUPREMUS                   = 1,
    TYPE_SHADE                      = 2,
    TYPE_GOREFIEND                  = 3,
    TYPE_BLOODBOIL                  = 4,
    TYPE_RELIQUIARY                 = 5,
    TYPE_SHAHRAZ                    = 6,
    TYPE_COUNCIL                    = 7,
    TYPE_ILLIDAN                    = 8,
    TYPE_GLAIVE_1                   = MAX_ENCOUNTER,
    TYPE_GLAIVE_2                   = MAX_ENCOUNTER+1,
    TYPE_CHANNELERS                 = MAX_ENCOUNTER+2,

    // NPC_WARLORD_NAJENTUS         = 22887,
    // NPC_SUPREMUS                 = 22898,
    NPC_SHADE_OF_AKAMA              = 22841,
    NPC_AKAMA_SHADE                 = 22990,
    NPC_RELIQUARY_OF_SOULS          = 22856,
    NPC_ILLIDARI_COUNCIL            = 23426,
    NPC_COUNCIL_VOICE               = 23499,
    NPC_LADY_MALANDE                = 22951,
    NPC_ZEREVOR                     = 22950,
    NPC_GATHIOS                     = 22949,
    NPC_VERAS                       = 22952,
    NPC_AKAMA                       = 23089,
    NPC_MAIEV_SHADOWSONG            = 23197,
    NPC_ILLIDAN_STORMRAGE           = 22917,

    NPC_ASH_CHANNELER               = 23421,
    NPC_ASH_BROKEN                  = 23319,
    NPC_CREATURE_GENERATOR          = 23210,
    NPC_ILLIDAN_DOOR_TRIGGER        = 23412,
    NPC_GLAIVE_TARGET               = 23448,
    NPC_SPIRIT_OF_OLUM              = 23411,
    NPC_SPIRIT_OF_UDALO             = 23410,

    GO_NAJENTUS_GATE                = 185483,
    GO_SUPREMUS_DOORS               = 185882,
    GO_SHADE_OF_AKAMA               = 185478,
    GO_GOREFIEND_DOOR               = 186153,
    GO_GURTOGG_DOOR                 = 185892,
    GO_PRE_SHAHRAZ_DOOR             = 185479,
    GO_POST_SHAHRAZ_DOOR            = 185482,
    GO_PRE_COUNCIL_DOOR             = 185481,
    GO_COUNCIL_DOOR                 = 186152,
    GO_ILLIDAN_GATE                 = 185905,
    GO_ILLIDAN_DOOR_R               = 186261,
    GO_ILLIDAN_DOOR_L               = 186262,
};

struct Location
{
    float m_fX, m_fY, m_fZ;
};

static const Location afBrokenSpawnLoc[] =
{
    { 541.375916f, 401.439575f, 112.784f },       // The place where Akama channels
    { 534.130005f, 352.394531f, 112.784f },       // Behind a 'pillar' which is behind the east alcove
};

#endif
