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

#ifndef DEF_NEXUS_H
#define DEF_NEXUS_H

enum
{
    MAX_ENCOUNTER                  = 4,
    MAX_SPECIAL_ACHIEV_CRITS       = 2,

    TYPE_TELESTRA                  = 0,
    TYPE_ANOMALUS                  = 1,
    TYPE_ORMOROK                   = 2,
    TYPE_KERISTRASZA               = 3,
    TYPE_INTENSE_COLD_FAILED       = 4,

    TYPE_ACHIEV_CHAOS_THEORY       = 5,     //order to be met in ScriptedInstance
    TYPE_ACHIEV_SPLIT_PERSONALITY  = 6,

    NPC_TELESTRA                   = 26731,
    NPC_ANOMALUS                   = 26763,
    NPC_ORMOROK                    = 26794,
    NPC_KERISTRASZA                = 26723,

    GO_CONTAINMENT_SPHERE_TELESTRA = 188526,
    GO_CONTAINMENT_SPHERE_ANOMALUS = 188527,
    GO_CONTAINMENT_SPHERE_ORMOROK  = 188528,

    SPELL_FROZEN_PRISON             = 47854,

    ACHIEV_CRIT_CHAOS_THEORY        = 7316,                 // Anomalus, achiev 2037
    ACHIEV_CRIT_INTENSE_COLD        = 7315,                 // Keristrasza, achiev 2036
    ACHIEV_CRIT_SPLIT_PERSONALITY   = 7577,                 // Telestra, achiev 2150
};
#endif
