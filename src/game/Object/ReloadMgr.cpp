/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

#include "ReloadMgr.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "DisableMgr.h"
#include "World.h"
#include "MapManager.h"
#include "CreatureEventAIMgr.h"
#include "BattleGroundMgr.h"
#include "SkillExtraItems.h"
#include "SkillDiscovery.h"
#include "ItemEnchantmentMgr.h"
#include "LootMgr.h"
#include "ScriptMgr.h"
#include "CommandMgr.h"
#include "SQLStorages.h"

ReloadMgr& ReloadMgr::instance()
{
    static ReloadMgr mgr;
    return mgr;
}

ReloadMgr::ReloadMgr()
{
    m_worker = std::thread(&ReloadMgr::WorkerThread, this);
}

ReloadMgr::~ReloadMgr()
{
    {
        std::lock_guard<std::mutex> lock(m_queueLock);
        m_stop = true;
    }
    m_cv.notify_one();
    if (m_worker.joinable())
        m_worker.join();
}

void ReloadMgr::StartFullReload(void (*printFn)(void*, char const*), void* printArg, bool forceRespawn)
{
    ReloadTask task;
    task.printFn = printFn;
    task.printArg = printArg;
    task.forceRespawn = forceRespawn;

    {
        std::lock_guard<std::mutex> lock(m_queueLock);
        m_queue.push(task);
    }
    m_cv.notify_one();
}

void ReloadMgr::WorkerThread()
{
    while (true)
    {
        ReloadTask task;

        {
            std::unique_lock<std::mutex> lock(m_queueLock);
            m_cv.wait(lock, [this] { return m_stop || !m_queue.empty(); });
            if (m_stop && m_queue.empty())
                return;
            if (m_queue.empty())
                continue;
            task = m_queue.front();
            m_queue.pop();
        }

        m_running = true;

        sWorld.QueueDelayedTask([forceRespawn = task.forceRespawn]() {
            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 1/10: Шаблоны (атомарный обмен)...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 1/10: Template swap (atomic, zero downtime)...");
            sCreatureStorage.ReloadAtomic();
            sGOStorage.ReloadAtomic();
            sItemStorage.ReloadAtomic();
            sCreatureModelStorage.ReloadAtomic();
            sEquipmentStorage.ReloadAtomic();
            sCreatureDataAddonStorage.ReloadAtomic();
            sCreatureInfoAddonStorage.ReloadAtomic();
            sCreatureTemplateSpellsStorage.ReloadAtomic();
            sVehicleAccessoryStorage.ReloadAtomic();
            sSpellScriptTargetStorage.ReloadAtomic();
            sPageTextStore.ReloadAtomic();
            sInstanceTemplate.ReloadAtomic();
            sConditionStorage.ReloadAtomic();

            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 2/10: Таблицы лута...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 2/10: Loot tables...");
            LoadLootTables();

            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 3/10: Квесты...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 3/10: Quests & relations...");
            sObjectMgr.LoadQuests();
            sObjectMgr.LoadQuestPOI();
            sObjectMgr.LoadQuestAreaTriggers();
            sObjectMgr.LoadQuestRelations();
            sObjectMgr.LoadGameobjectQuestRelations();
            sObjectMgr.LoadGameobjectInvolvedRelations();
            sObjectMgr.LoadCreatureQuestRelations();
            sObjectMgr.LoadCreatureInvolvedRelations();
            sObjectMgr.LoadGameObjectForQuests();

            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 4/10: NPC и торговцы...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 4/10: NPCs & Gossip...");
            sObjectMgr.LoadTrainerTemplates();
            sObjectMgr.LoadTrainers();
            sObjectMgr.LoadVendorTemplates();
            sObjectMgr.LoadVendors();
            sObjectMgr.LoadPointsOfInterest();
            sObjectMgr.LoadNPCSpellClickSpells();
            sObjectMgr.LoadMangosStrings();
            sObjectMgr.LoadGossipMenus();
            sObjectMgr.LoadGossipText();

            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 5/10: Заклинания и условия...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 5/10: Spells, conditions, disables...");
            sSpellMgr.LoadSpellAreas();
            sSpellMgr.LoadSpellBonuses();
            sSpellMgr.LoadSpellChains();
            sSpellMgr.LoadSpellElixirs();
            sSpellMgr.LoadSpellLearnSpells();
            sSpellMgr.LoadSpellProcEvents();
            sSpellMgr.LoadSpellProcItemEnchant();
            sSpellMgr.LoadSpellScriptTarget();
            sSpellMgr.LoadSpellTargetPositions();
            sSpellMgr.LoadSpellThreats();
            sSpellMgr.LoadSpellPetAuras();
            sObjectMgr.LoadConditions();
            DisableMgr::LoadDisables();
            DisableMgr::CheckQuestDisables();

            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 6/10: Скрипты поведения (DB Scripts)...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 6/10: DB Scripts...");
            sScriptMgr.LoadDbScripts(DBS_ON_GOSSIP);
            sScriptMgr.LoadDbScripts(DBS_ON_CREATURE_MOVEMENT);
            sScriptMgr.LoadDbScripts(DBS_ON_QUEST_START);
            sScriptMgr.LoadDbScripts(DBS_ON_QUEST_END);
            sScriptMgr.LoadDbScripts(DBS_ON_SPELL);
            sScriptMgr.LoadDbScripts(DBS_ON_GO_USE);
            sScriptMgr.LoadDbScripts(DBS_ON_GOT_USE);
            sScriptMgr.LoadDbScripts(DBS_ON_EVENT);
            sScriptMgr.LoadDbScripts(DBS_ON_CREATURE_DEATH);
            sScriptMgr.LoadDbScriptStrings();

            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 7/10: Зоны, события, настройки...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 7/10: Areas, events, misc...");
            sObjectMgr.LoadAreaTriggerTeleports();
            sObjectMgr.LoadTavernAreaTriggers();
            sObjectMgr.LoadGraveyardZones();
            sObjectMgr.LoadFishingBaseSkillLevel();
            sObjectMgr.LoadMailLevelRewards();
            LoadSkillDiscoveryTable();
            LoadSkillExtraItemTable();
            LoadRandomEnchantmentsTable();
            sObjectMgr.LoadItemConverts();
            sObjectMgr.LoadItemExpireConverts();
            sObjectMgr.LoadItemRequiredTarget();
            sObjectMgr.LoadReputationRewardRate();
            sObjectMgr.LoadReputationOnKill();
            sObjectMgr.LoadReputationSpilloverTemplate();
            sObjectMgr.LoadGameTele();
            sObjectMgr.LoadReservedPlayersNames();
            sObjectMgr.LoadDungeonFinderRequirements();
            sObjectMgr.LoadDungeonFinderRewards();
            sObjectMgr.LoadDungeonFinderItems();
            sObjectMgr.LoadHotfixData();
            sWorld.LoadBroadcastStrings();

            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 8/10: Пересборка кэшей...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 8/10: Rebuild caches from templates...");
            sObjectMgr.LoadCreatureModelInfo();
            sObjectMgr.LoadEquipmentTemplates();
            sObjectMgr.LoadCreatureAddons();
            sObjectMgr.LoadSpellTemplate();
            sObjectMgr.LoadCreatureTemplateSpells();
            sObjectMgr.LoadVehicleAccessory();

            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 9/10: Локализация...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 9/10: Localized text...");
            sObjectMgr.LoadCreatureLocales();
            sObjectMgr.LoadGameObjectLocales();
            sObjectMgr.LoadItemLocales();
            sObjectMgr.LoadQuestLocales();
            sObjectMgr.LoadGossipTextLocales();
            sObjectMgr.LoadPageTextLocales();
            sObjectMgr.LoadGossipMenuItemsLocales();
            sObjectMgr.LoadPointOfInterestLocales();

            sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Phase 10/10: Достижения, скрипты...", SEC_PLAYER);
            sLog.outString("[Reload] Phase 10/10: Achievements, EventAI, Events...");
            sAchievementMgr.LoadAchievementCriteriaRequirements();
            sAchievementMgr.LoadRewards();
            sAchievementMgr.LoadRewardLocales();
            sEventAIMgr.LoadCreatureEventAI_Texts(true);
            sEventAIMgr.LoadCreatureEventAI_Summons(true);
            sEventAIMgr.LoadCreatureEventAI_Scripts();
            sBattleGroundMgr.LoadBattleEventIndexes();

            sLog.outString("[Reload] Reloading config settings...");
            sWorld.LoadConfigSettings(true);

            if (forceRespawn)
            {
                sWorld.SendGlobalSysMessage("|cFFFFCC00[Reload]|r Респаун существ...", SEC_PLAYER);
                sLog.outString("[Reload] Forcing creature respawn...");
                sMapMgr.ForceRespawnAllCreatures();
            }

            sWorld.SendGlobalSysMessage("|cff00ff00[Reload] Перезагрузка завершена успешно!|r", SEC_PLAYER);
            sLog.outString("[Reload] All major DB tables reloaded successfully!");
        });

        m_running = false;
    }
}
