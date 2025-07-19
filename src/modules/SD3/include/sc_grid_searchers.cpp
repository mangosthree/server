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

#include "precompiled.h"

#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

/**
 * @brief Returns the closest GameObject with a specific entry within a given range.
 * 
 * This function searches for the closest GameObject with the specified entry ID within the given range from the source object.
 * 
 * @param pSource Pointer to the source WorldObject.
 * @param uiEntry Entry ID of the GameObject.
 * @param fMaxSearchRange Maximum search range.
 * @return Pointer to the closest GameObject, or nullptr if not found.
 */
GameObject* GetClosestGameObjectWithEntry(WorldObject* pSource, uint32 uiEntry, float fMaxSearchRange)
{
    GameObject* pGo = nullptr;

    MaNGOS::NearestGameObjectEntryInObjectRangeCheck go_check(*pSource, uiEntry, fMaxSearchRange);
    MaNGOS::GameObjectLastSearcher<MaNGOS::NearestGameObjectEntryInObjectRangeCheck> searcher(pGo, go_check);

    Cell::VisitGridObjects(pSource, searcher, fMaxSearchRange);

    return pGo;
}

/**
 * @brief Returns the closest Creature with a specific entry within a given range.
 * 
 * This function searches for the closest Creature with the specified entry ID within the given range from the source object.
 * 
 * @param pSource Pointer to the source WorldObject.
 * @param uiEntry Entry ID of the Creature.
 * @param fMaxSearchRange Maximum search range.
 * @param bOnlyAlive If true, only alive creatures will be considered.
 * @param bOnlyDead If true, only dead creatures will be considered.
 * @param bExcludeSelf If true, the source creature will be excluded from the search.
 * @return Pointer to the closest Creature, or nullptr if not found.
 */
Creature* GetClosestCreatureWithEntry(WorldObject* pSource, uint32 uiEntry, float fMaxSearchRange, bool bOnlyAlive/*=true*/, bool bOnlyDead/*=false*/, bool bExcludeSelf/*=false*/)
{
    Creature* pCreature = nullptr;

    MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck creature_check(*pSource, uiEntry, bOnlyAlive, bOnlyDead, fMaxSearchRange, bExcludeSelf);
    MaNGOS::CreatureLastSearcher<MaNGOS::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(pCreature, creature_check);

    Cell::VisitGridObjects(pSource, searcher, fMaxSearchRange);

    return pCreature;
}

/**
 * @brief Gets a list of GameObjects with a specific entry within a given range.
 * 
 * This function searches for all GameObjects with the specified entry ID within the given range from the source object and stores them in the provided list.
 * 
 * @param lList Reference to the list to store the found GameObjects.
 * @param pSource Pointer to the source WorldObject.
 * @param uiEntry Entry ID of the GameObject.
 * @param fMaxSearchRange Maximum search range.
 */
void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& lList , WorldObject* pSource, uint32 uiEntry, float fMaxSearchRange)
{
    MaNGOS::GameObjectEntryInPosRangeCheck check(*pSource, uiEntry, pSource->GetPositionX(), pSource->GetPositionY(), pSource->GetPositionZ(), fMaxSearchRange);
    MaNGOS::GameObjectListSearcher<MaNGOS::GameObjectEntryInPosRangeCheck> searcher(lList, check);

    Cell::VisitGridObjects(pSource, searcher, fMaxSearchRange);
}

/**
 * @brief Gets a list of Creatures with a specific entry within a given range.
 * 
 * This function searches for all Creatures with the specified entry ID within the given range from the source object and stores them in the provided list.
 * 
 * @param lList Reference to the list to store the found Creatures.
 * @param pSource Pointer to the source WorldObject.
 * @param uiEntry Entry ID of the Creature.
 * @param fMaxSearchRange Maximum search range.
 */
void GetCreatureListWithEntryInGrid(std::list<Creature*>& lList, WorldObject* pSource, uint32 uiEntry, float fMaxSearchRange)
{
    MaNGOS::AllCreaturesOfEntryInRangeCheck check(pSource, uiEntry, fMaxSearchRange);
    MaNGOS::CreatureListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(lList, check);

    Cell::VisitGridObjects(pSource, searcher, fMaxSearchRange);
}
