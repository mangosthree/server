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

#ifndef SC_GRIDSEARCH_H
#define SC_GRIDSEARCH_H

#include "Object.h"
class GameObject;
class Creature;

/**
 * @struct ObjectDistanceOrder
 * @brief Comparator for sorting WorldObjects by distance from a source Unit.
 */
struct ObjectDistanceOrder
{
    const Unit* m_pSource; ///< Source unit for distance comparison

    /**
     * @brief Constructor for ObjectDistanceOrder.
     * @param pSource Pointer to the source unit.
     */
    ObjectDistanceOrder(const Unit* pSource) : m_pSource(pSource) {}

    /**
     * @brief Comparator function for sorting WorldObjects by distance.
     * @param pLeft Pointer to the left WorldObject.
     * @param pRight Pointer to the right WorldObject.
     * @return True if pLeft is closer to the source than pRight, false otherwise.
     */
    bool operator()(const WorldObject* pLeft, const WorldObject* pRight) const
    {
        return m_pSource->GetDistanceOrder(pLeft, pRight);
    }
};

/**
 * @struct ObjectDistanceOrderReversed
 * @brief Comparator for sorting WorldObjects by distance from a source Unit in reverse order.
 */
struct ObjectDistanceOrderReversed
{
    const Unit* m_pSource; ///< Source unit for distance comparison

    /**
     * @brief Constructor for ObjectDistanceOrderReversed.
     * @param pSource Pointer to the source unit.
     */
    ObjectDistanceOrderReversed(const Unit* pSource) : m_pSource(pSource) {}

    /**
     * @brief Comparator function for sorting WorldObjects by distance in reverse order.
     * @param pLeft Pointer to the left WorldObject.
     * @param pRight Pointer to the right WorldObject.
     * @return True if pLeft is farther from the source than pRight, false otherwise.
     */
    bool operator()(const WorldObject* pLeft, const WorldObject* pRight) const
    {
        return !m_pSource->GetDistanceOrder(pLeft, pRight);
    }
};

/**
 * @brief Gets the closest GameObject with a specific entry within a given range.
 * @param pSource Pointer to the source WorldObject.
 * @param uiEntry Entry ID of the GameObject.
 * @param fMaxSearchRange Maximum search range.
 * @return Pointer to the closest GameObject, or nullptr if not found.
 */
GameObject* GetClosestGameObjectWithEntry(WorldObject* pSource, uint32 uiEntry, float fMaxSearchRange);

/**
 * @brief Gets the closest Creature with a specific entry within a given range.
 * @param pSource Pointer to the source WorldObject.
 * @param uiEntry Entry ID of the Creature.
 * @param fMaxSearchRange Maximum search range.
 * @param bOnlyAlive If true, only alive creatures will be considered.
 * @param bOnlyDead If true, only dead creatures will be considered.
 * @param bExcludeSelf If true, the source creature will be excluded from the search.
 * @return Pointer to the closest Creature, or nullptr if not found.
 */
Creature* GetClosestCreatureWithEntry(WorldObject* pSource, uint32 uiEntry, float fMaxSearchRange, bool bOnlyAlive = true, bool bOnlyDead = false, bool bExcludeSelf = false);

/**
 * @brief Gets a list of GameObjects with a specific entry within a given range.
 * @param lList Reference to the list to store the found GameObjects.
 * @param pSource Pointer to the source WorldObject.
 * @param uiEntry Entry ID of the GameObject.
 * @param fMaxSearchRange Maximum search range.
 */
void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& lList , WorldObject* pSource, uint32 uiEntry, float fMaxSearchRange);

/**
 * @brief Gets a list of Creatures with a specific entry within a given range.
 * @param lList Reference to the list to store the found Creatures.
 * @param pSource Pointer to the source WorldObject.
 * @param uiEntry Entry ID of the Creature.
 * @param fMaxSearchRange Maximum search range.
 */
void GetCreatureListWithEntryInGrid(std::list<Creature*>& lList, WorldObject* pSource, uint32 uiEntry, float fMaxSearchRange);

#endif
