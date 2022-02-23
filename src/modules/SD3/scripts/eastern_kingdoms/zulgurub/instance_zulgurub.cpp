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

/**
 * ScriptData
 * SDName:      Instance_ZulGurub
 * SD%Complete: 80
 * SDComment:   Missing reset function after killing a boss for Ohgan, Thekal.
 * SDCategory:  Zul'Gurub
 * EndScriptData
 */

#include "precompiled.h"
#include "zulgurub.h"

struct is_zulgurub : public InstanceScript
{
    is_zulgurub() : InstanceScript("instance_zulgurub") {}

    class instance_zulgurub : public ScriptedInstance
    {
    public:
        instance_zulgurub(Map* pMap) : ScriptedInstance(pMap),
            m_bHasIntroYelled(false),
            m_bHasAltarYelled(false)
        {
            Initialize();
        }

        ~instance_zulgurub() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        // IsEncounterInProgress() const override { return false; }  // not active in Zul'Gurub

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_LORKHAN:
            case NPC_ZATH:
            case NPC_THEKAL:
            case NPC_JINDO:
            case NPC_HAKKAR:
            case NPC_BLOODLORD_MANDOKIR:
            case NPC_MARLI:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_PANTHER_TRIGGER:
                if (pCreature->GetPositionY() < -1626)
                {
                    m_lLeftPantherTriggerGUIDList.push_back(pCreature->GetObjectGuid());
                }
                else
                {
                    m_lRightPantherTriggerGUIDList.push_back(pCreature->GetObjectGuid());
                }
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_GONG_OF_BETHEKK:
            case GO_FORCEFIELD:
                break;
            case GO_SPIDER_EGG:
                m_lSpiderEggGUIDList.push_back(pGo->GetObjectGuid());
                return;
            }

            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_JEKLIK:
            case TYPE_VENOXIS:
            case TYPE_THEKAL:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoLowerHakkarHitPoints();
                }
                break;
            case TYPE_MARLI:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoLowerHakkarHitPoints();
                }
                if (uiData == FAIL)
                {
                    for (GuidList::const_iterator itr = m_lSpiderEggGUIDList.begin(); itr != m_lSpiderEggGUIDList.end(); ++itr)
                    {
                        if (GameObject* pEgg = instance->GetGameObject(*itr))
                        {
                            // Note: this type of Gameobject needs to be respawned manually
                            pEgg->SetRespawnTime(2 * DAY);
                            pEgg->Respawn();
                        }
                    }
                }
                break;
            case TYPE_ARLOKK:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_FORCEFIELD);
                if (uiData == DONE)
                {
                    DoLowerHakkarHitPoints();
                }
                if (uiData == FAIL)
                {
                    // Note: this gameobject should change flags - currently it despawns which isn't correct
                    if (GameObject* pGong = GetSingleGameObjectFromStorage(GO_GONG_OF_BETHEKK))
                    {
                        pGong->SetRespawnTime(2 * DAY);
                        pGong->Respawn();
                    }
                }
                break;
            case TYPE_OHGAN:
                // Note: SPECIAL instance data is set via ACID!
                if (uiData == SPECIAL)
                {
                    if (Creature* pMandokir = GetSingleCreatureFromStorage(NPC_BLOODLORD_MANDOKIR))
                    {
                        pMandokir->SetWalk(false);
                        pMandokir->GetMotionMaster()->MovePoint(1, aMandokirDownstairsPos[0], aMandokirDownstairsPos[1], aMandokirDownstairsPos[2]);
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_LORKHAN:
            case TYPE_ZATH:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_SIGNAL_1:
                DoYellAtTriggerIfCan(uiData);
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5] << " "
                    << m_auiEncounter[6] << " " << m_auiEncounter[7];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
            {
                return m_auiEncounter[uiType];
            }

            return 0;
        }

        uint64 GetData64(uint32 type) const override
        {
            switch (type)
            {
            case TYPE_SIGNAL_2:
            case TYPE_SIGNAL_3:
                if (Creature *p = (const_cast<instance_zulgurub*>(this))->SelectRandomPantherTrigger(type == TYPE_SIGNAL_2))
                {
                    return p->GetObjectGuid().GetRawValue();
                }
                break;
            default:
                break;
            }

            return 0;
        }

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3]
                >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6] >> m_auiEncounter[7];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

    private:
        void DoLowerHakkarHitPoints()
        {
            if (Creature* pHakkar = GetSingleCreatureFromStorage(NPC_HAKKAR))
            {
                if (pHakkar->IsAlive() && pHakkar->GetMaxHealth() > HP_LOSS_PER_PRIEST)
                {
                    pHakkar->SetMaxHealth(pHakkar->GetMaxHealth() - HP_LOSS_PER_PRIEST);
                    pHakkar->SetHealth(pHakkar->GetHealth() - HP_LOSS_PER_PRIEST);
                }
            }
        }

        void DoYellAtTriggerIfCan(uint32 uiTriggerId)
        {
            if (uiTriggerId == AREATRIGGER_ENTER && !m_bHasIntroYelled)
            {
                DoOrSimulateScriptTextForThisInstance(SAY_HAKKAR_PROTECT, NPC_HAKKAR);
                m_bHasIntroYelled = true;
            }
            else if (uiTriggerId == AREATRIGGER_ALTAR && !m_bHasAltarYelled)
            {
                DoOrSimulateScriptTextForThisInstance(SAY_MINION_DESTROY, NPC_HAKKAR);
                m_bHasAltarYelled = true;
            }
        }

        Creature* SelectRandomPantherTrigger(bool bIsLeft)
        {
            GuidList* plTempList = bIsLeft ? &m_lLeftPantherTriggerGUIDList : &m_lRightPantherTriggerGUIDList;
            std::vector<Creature*> vTriggers;
            vTriggers.reserve(plTempList->size());

            for (GuidList::const_iterator itr = plTempList->begin(); itr != plTempList->end(); ++itr)
            {
                if (Creature* pTemp = instance->GetCreature(*itr))
                {
                    vTriggers.push_back(pTemp);
                }
            }

            if (vTriggers.empty())
            {
                return nullptr;
            }

            return vTriggers[urand(0, vTriggers.size() - 1)];
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        GuidList m_lRightPantherTriggerGUIDList;
        GuidList m_lLeftPantherTriggerGUIDList;
        GuidList m_lSpiderEggGUIDList;

        bool m_bHasIntroYelled;
        bool m_bHasAltarYelled;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_zulgurub(pMap);
    }
};

struct at_zulgurub : public AreaTriggerScript
{
    at_zulgurub() : AreaTriggerScript("at_zulgurub") {}

    bool AreaTrigger_at_zulgurub(Player* pPlayer, AreaTriggerEntry const* pAt)
    {
        if (pAt->id == AREATRIGGER_ENTER || pAt->id == AREATRIGGER_ALTAR)
        {
            if (pPlayer->isGameMaster() || pPlayer->IsDead())
            {
                return false;
            }

            if (ScriptedInstance* pInstance = (ScriptedInstance*)pPlayer->GetInstanceData())
            {
                pInstance->SetData(TYPE_SIGNAL_1, pAt->id);
            }
        }

        return false;
    }
};

void AddSC_instance_zulgurub()
{
    Script* s;
    s = new is_zulgurub();
    s->RegisterSelf();
    s = new at_zulgurub();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_zulgurub";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_zulgurub;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_zulgurub";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_zulgurub;
    //pNewScript->RegisterSelf();
}
