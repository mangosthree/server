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

/* ScriptData
SDName: Instance_Mount_Hyjal
SD%Complete: 100
SDComment: Instance Data Scripts and functions to acquire mobs and set encounter status for use in various Hyjal Scripts
SDCategory: Caverns of Time, Mount Hyjal
EndScriptData */

#include "precompiled.h"
#include "hyjal.h"

/* Battle of Mount Hyjal encounters:
0 - Rage Winterchill event
1 - Anetheron event
2 - Kaz'rogal event
3 - Azgalor event
4 - Archimonde event
*/

static const float aArchimondeSpawnLoc[4] = { 5581.49f, -3445.63f, 1575.1f, 3.905f };

struct is_mount_hyjal : public InstanceScript
{
    is_mount_hyjal() : InstanceScript("instance_hyjal") {}

    class instance_mount_hyjal : public ScriptedInstance
    {
    public:
        instance_mount_hyjal(Map* pMap) : ScriptedInstance(pMap),
            m_uiTrashCount(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        bool IsEncounterInProgress() const override
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            if (m_auiEncounter[i] == IN_PROGRESS)
            {
                return true;
            }

            return false;
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            if (GetData(TYPE_AZGALOR) == DONE)
            {
                DoSpawnArchimonde();
            }
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_ARCHIMONDE)
            {
                m_mNpcEntryGuidStore[NPC_ARCHIMONDE] = pCreature->GetObjectGuid();
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            if (pGo->GetEntry() == GO_ANCIENT_GEM)
            {
                lAncientGemGUIDList.push_back(pGo->GetObjectGuid());
            }
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_WINTERCHILL: SetData(TYPE_WINTERCHILL, IN_PROGRESS); break;
            case NPC_ANETHERON:   SetData(TYPE_ANETHERON, IN_PROGRESS);   break;
            case NPC_KAZROGAL:    SetData(TYPE_KAZROGAL, IN_PROGRESS);    break;
            case NPC_AZGALOR:     SetData(TYPE_AZGALOR, IN_PROGRESS);     break;
            case NPC_ARCHIMONDE:  SetData(TYPE_ARCHIMONDE, IN_PROGRESS);  break;
            }
        }

        void OnCreatureEvade(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_WINTERCHILL: SetData(TYPE_WINTERCHILL, FAIL); break;
            case NPC_ANETHERON:   SetData(TYPE_ANETHERON, FAIL);   break;
            case NPC_KAZROGAL:    SetData(TYPE_KAZROGAL, FAIL);    break;
            case NPC_AZGALOR:     SetData(TYPE_AZGALOR, FAIL);     break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_WINTERCHILL: SetData(TYPE_WINTERCHILL, DONE); break;
            case NPC_ANETHERON:   SetData(TYPE_ANETHERON, DONE);   break;
            case NPC_KAZROGAL:    SetData(TYPE_KAZROGAL, DONE);    break;
            case NPC_AZGALOR:     SetData(TYPE_AZGALOR, DONE);     break;
            case NPC_ARCHIMONDE:  SetData(TYPE_ARCHIMONDE, DONE);  break;

                // Trash Mobs summoned in waves
            case NPC_NECRO:
            case NPC_ABOMI:
            case NPC_GHOUL:
            case NPC_BANSH:
            case NPC_CRYPT:
            case NPC_GARGO:
            case NPC_FROST:
            case NPC_GIANT:
            case NPC_STALK:
                // Decrease counter, and update world-state
                if (m_uiTrashCount)
                {
                    --m_uiTrashCount;
                    DoUpdateWorldState(WORLD_STATE_ENEMYCOUNT, m_uiTrashCount);
                }
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_WINTERCHILL:
            case TYPE_ANETHERON:
            case TYPE_KAZROGAL:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_AZGALOR:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoSpawnArchimonde();
                }
                break;
            case TYPE_ARCHIMONDE:
                m_auiEncounter[uiType] = uiData;
                break;

            case TYPE_TRASH_COUNT:
                m_uiTrashCount = uiData;
                DoUpdateWorldState(WORLD_STATE_ENEMYCOUNT, m_uiTrashCount);
                break;

            case TYPE_RETREAT:
                if (uiData == SPECIAL)
                {
                    if (!lAncientGemGUIDList.empty())
                    {
                        for (GuidList::const_iterator itr = lAncientGemGUIDList.begin(); itr != lAncientGemGUIDList.end(); ++itr)
                        {
                            // don't know how long it expected
                            DoRespawnGameObject(*itr, DAY);
                        }
                    }
                }
                break;
            }

            debug_log("SD3: Instance Hyjal: Instance data updated for event %u (Data=%u)", uiType, uiData);

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4];

                m_strSaveData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            switch (uiType)
            {
            case TYPE_WINTERCHILL:
            case TYPE_ANETHERON:
            case TYPE_KAZROGAL:
            case TYPE_AZGALOR:
            case TYPE_ARCHIMONDE:
                return m_auiEncounter[uiType];
            case TYPE_TRASH_COUNT:
                return m_uiTrashCount;
            default:
                return 0;
            }
        }

        const char* Save() const override { return m_strSaveData.c_str(); }
        void Load(const char* chrIn) override
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3] >> m_auiEncounter[4];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            if (m_auiEncounter[i] == IN_PROGRESS)               // Do not load an encounter as IN_PROGRESS - reset it instead.
            {
                m_auiEncounter[i] = NOT_STARTED;
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

    private:
        void DoSpawnArchimonde()
        {
            // Don't spawn if already killed
            if (GetData(TYPE_ARCHIMONDE) == DONE)
            {
                return;
            }

            // Don't spawn him twice
            if (GetSingleCreatureFromStorage(NPC_ARCHIMONDE, true))
            {
                return;
            }

            // Summon Archimonde
            if (Player* pPlayer = GetPlayerInMap())
            {
                pPlayer->SummonCreature(NPC_ARCHIMONDE, aArchimondeSpawnLoc[0], aArchimondeSpawnLoc[1], aArchimondeSpawnLoc[2], aArchimondeSpawnLoc[3], TEMPSPAWN_DEAD_DESPAWN, 0);
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strSaveData;

        GuidList lAncientGemGUIDList;

        uint32 m_uiTrashCount;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_mount_hyjal(pMap);
    }
};

void AddSC_instance_mount_hyjal()
{
    Script* s;

    s = new is_mount_hyjal();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_hyjal";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_mount_hyjal;
    //pNewScript->RegisterSelf();
}
