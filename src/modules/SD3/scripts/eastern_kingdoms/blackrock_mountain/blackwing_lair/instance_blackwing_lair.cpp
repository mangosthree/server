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
 * SDName:      Instance_Blackwing_Lair
 * SD%Complete: 90
 * SDComment:   None
 * SDCategory:  Blackwing Lair
 * EndScriptData
 */

#include "precompiled.h"
#include "blackwing_lair.h"

static const uint32 aRazorgoreSpawns[MAX_EGGS_DEFENDERS] = { NPC_BLACKWING_LEGIONNAIRE, NPC_BLACKWING_MAGE, NPC_DRAGONSPAWN, NPC_DRAGONSPAWN };

struct is_blackwing_lair : public InstanceScript
{
    is_blackwing_lair() : InstanceScript("instance_blackwing_lair") {}

    class instance_blackwing_lair : public ScriptedInstance
    {
    public:
        instance_blackwing_lair(Map* pMap) : ScriptedInstance(pMap),
            m_uiResetTimer(0),
            m_uiDefenseTimer(0)
        {
            Initialize();
        }

        ~instance_blackwing_lair() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        bool IsEncounterInProgress() const override
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    return true;
                }
            }
            return false;
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_BLACKWING_TECHNICIAN:
                // Sort creatures so we can get only the ones near Vaelastrasz
                if (pCreature->IsWithinDist2d(aNefariusSpawnLoc[0], aNefariusSpawnLoc[1], 50.0f))
                {
                    m_lTechnicianGuids.push_back(pCreature->GetObjectGuid());
                }
                break;
            case NPC_MONSTER_GENERATOR:
                m_vGeneratorGuids.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_BLACKWING_LEGIONNAIRE:
            case NPC_BLACKWING_MAGE:
            case NPC_DRAGONSPAWN:
                m_lDefendersGuids.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_RAZORGORE:
            case NPC_NEFARIANS_TROOPS:
            case NPC_BLACKWING_ORB_TRIGGER:
            case NPC_VAELASTRASZ:
            case NPC_LORD_VICTOR_NEFARIUS:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_DOOR_RAZORGORE_ENTER:
            case GO_ORB_OF_DOMINATION:
            case GO_DOOR_NEFARIAN:
                break;
            case GO_DOOR_RAZORGORE_EXIT:
                if (m_auiEncounter[TYPE_RAZORGORE] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_DOOR_CHROMAGGUS_EXIT:
                if (m_auiEncounter[TYPE_CHROMAGGUS] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_DOOR_VAELASTRASZ:
                if (m_auiEncounter[TYPE_VAELASTRASZ] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_DOOR_LASHLAYER:
                if (m_auiEncounter[TYPE_LASHLAYER] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_BLACK_DRAGON_EGG:
                m_lDragonEggsGuids.push_back(pGo->GetObjectGuid());
                return;
            case GO_DRAKONID_BONES:
                m_lDrakonidBonesGuids.push_back(pGo->GetObjectGuid());
                return;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_GRETHOK_CONTROLLER)
            {
                SetData(TYPE_RAZORGORE, IN_PROGRESS);
                m_uiDefenseTimer = 40000;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_GRETHOK_CONTROLLER)
            {
                // Allow orb to be used
                DoToggleGameObjectFlags(GO_ORB_OF_DOMINATION, GO_FLAG_NO_INTERACT, false);

                if (Creature* pOrbTrigger = GetSingleCreatureFromStorage(NPC_BLACKWING_ORB_TRIGGER))
                {
                    pOrbTrigger->InterruptNonMeleeSpells(false);
                }
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_RAZORGORE:
                m_auiEncounter[uiType] = uiData;
                if (uiData != SPECIAL)
                {
                    DoUseDoorOrButton(GO_DOOR_RAZORGORE_ENTER);
                }
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_DOOR_RAZORGORE_EXIT);
                }
                else if (uiData == FAIL)
                {
                    m_uiResetTimer = 30000;

                    // Reset the Orb of Domination and the eggs
                    DoToggleGameObjectFlags(GO_ORB_OF_DOMINATION, GO_FLAG_NO_INTERACT, true);

                    // Reset defenders
                    for (GuidList::const_iterator itr = m_lDefendersGuids.begin(); itr != m_lDefendersGuids.end(); ++itr)
                    {
                        if (Creature* pDefender = instance->GetCreature(*itr))
                        {
                            pDefender->ForcedDespawn();
                        }
                    }

                    m_lUsedEggsGuids.clear();
                    m_lDefendersGuids.clear();
                }
                break;
            case TYPE_VAELASTRASZ:
                m_auiEncounter[uiType] = uiData;
                // Prevent the players from running back to the first room; use if the encounter is not special
                if (uiData != SPECIAL)
                {
                    DoUseDoorOrButton(GO_DOOR_RAZORGORE_EXIT);
                }
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_DOOR_VAELASTRASZ);
                }
                break;
            case TYPE_LASHLAYER:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_DOOR_LASHLAYER);
                }
                break;
            case TYPE_FIREMAW:
            case TYPE_EBONROC:
            case TYPE_FLAMEGOR:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_CHROMAGGUS:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_DOOR_CHROMAGGUS_EXIT);
                }
                break;
            case TYPE_NEFARIAN:
                // Don't store the same thing twice
                if (m_auiEncounter[uiType] == uiData)
                {
                    break;
                }
                if (uiData == SPECIAL)
                {
                    // handle missing spell 23362
                    Creature* pNefarius = GetSingleCreatureFromStorage(NPC_LORD_VICTOR_NEFARIUS);
                    if (!pNefarius)
                    {
                        break;
                    }

                    for (GuidList::const_iterator itr = m_lDrakonidBonesGuids.begin(); itr != m_lDrakonidBonesGuids.end(); ++itr)
                    {
                        // The Go script will handle the missing spell 23361
                        if (GameObject* pGo = instance->GetGameObject(*itr))
                        {
                            pGo->Use(pNefarius);
                        }
                    }
                    // Don't store special data
                    break;
                }
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_DOOR_NEFARIAN);
                // Cleanup the drakonid bones
                if (uiData == FAIL)
                {
                    for (GuidList::const_iterator itr = m_lDrakonidBonesGuids.begin(); itr != m_lDrakonidBonesGuids.end(); ++itr)
                    {
                        if (GameObject* pGo = instance->GetGameObject(*itr))
                        {
                            pGo->SetLootState(GO_JUST_DEACTIVATED);
                        }
                    }

                    m_lDrakonidBonesGuids.clear();
                }
                break;
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

        void SetData64(uint32 uiData, uint64 uiGuid) override
        {
            if (uiData == DATA_DRAGON_EGG)
            {
                if (GameObject* pEgg = instance->GetGameObject(ObjectGuid(uiGuid)))
                {
                    m_lUsedEggsGuids.push_back(pEgg->GetObjectGuid());
                }

                // If all eggs are destroyed, then allow Razorgore to be attacked
                if (m_lUsedEggsGuids.size() == m_lDragonEggsGuids.size())
                {
                    SetData(TYPE_RAZORGORE, SPECIAL);
                    DoToggleGameObjectFlags(GO_ORB_OF_DOMINATION, GO_FLAG_NO_INTERACT, true);

                    // Emote for the start of the second phase
                    if (Creature* pTrigger = GetSingleCreatureFromStorage(NPC_NEFARIANS_TROOPS))
                    {
                        DoScriptText(EMOTE_ORB_SHUT_OFF, pTrigger);
                        DoScriptText(EMOTE_TROOPS_FLEE, pTrigger);
                    }

                    // Break mind control and set max health
                    if (Creature* pRazorgore = GetSingleCreatureFromStorage(NPC_RAZORGORE))
                    {
                        pRazorgore->RemoveAllAuras();
                        pRazorgore->SetHealth(pRazorgore->GetMaxHealth());
                    }

                    // All defenders evade and despawn
                    for (GuidList::const_iterator itr = m_lDefendersGuids.begin(); itr != m_lDefendersGuids.end(); ++itr)
                    {
                        if (Creature* pDefender = instance->GetCreature(*itr))
                        {
                            pDefender->AI()->EnterEvadeMode();
                            pDefender->ForcedDespawn(10000);
                        }
                    }
                }
            }
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

        void Update(uint32 uiDiff) override
        {
            // Reset Razorgore in case of wipe
            if (m_uiResetTimer)
            {
                if (m_uiResetTimer <= uiDiff)
                {
                    // Respawn Razorgore
                    if (Creature* pRazorgore = GetSingleCreatureFromStorage(NPC_RAZORGORE))
                    {
                        if (!pRazorgore->IsAlive())
                        {
                            pRazorgore->Respawn();
                        }
                    }

                    // Respawn the Dragon Eggs
                    for (GuidList::const_iterator itr = m_lDragonEggsGuids.begin(); itr != m_lDragonEggsGuids.end(); ++itr)
                    {
                        if (GameObject* pEgg = instance->GetGameObject(*itr))
                        {
                            if (!pEgg->isSpawned())
                            {
                                pEgg->Respawn();
                            }
                        }
                    }

                    m_uiResetTimer = 0;
                }
                else
                {
                    m_uiResetTimer -= uiDiff;
                }
            }

            if (GetData(TYPE_RAZORGORE) != IN_PROGRESS)
            {
                return;
            }

            if (m_uiDefenseTimer < uiDiff)
            {
                // Allow Razorgore to spawn the defenders
                Creature* pRazorgore = GetSingleCreatureFromStorage(NPC_RAZORGORE);
                if (!pRazorgore)
                {
                    return;
                }

                // Randomize generators
                std::random_shuffle(m_vGeneratorGuids.begin(), m_vGeneratorGuids.end());

                // Spawn the defenders
                for (uint8 i = 0; i < MAX_EGGS_DEFENDERS; ++i)
                {
                    Creature* pGenerator = instance->GetCreature(m_vGeneratorGuids[i]);
                    if (!pGenerator)
                    {
                        return;
                    }

                    pRazorgore->SummonCreature(aRazorgoreSpawns[i], pGenerator->GetPositionX(), pGenerator->GetPositionY(), pGenerator->GetPositionZ(), pGenerator->GetOrientation(), TEMPSPAWN_DEAD_DESPAWN, 0);
                }

                m_uiDefenseTimer = 20000;
            }
            else
            {
                m_uiDefenseTimer -= uiDiff;
            }
        }

    protected:
        std::string m_strInstData;
        uint32 m_auiEncounter[MAX_ENCOUNTER];

        uint32 m_uiResetTimer;
        uint32 m_uiDefenseTimer;

        GuidList m_lTechnicianGuids;
        GuidList m_lDragonEggsGuids;
        GuidList m_lDrakonidBonesGuids;
        GuidList m_lDefendersGuids;
        GuidList m_lUsedEggsGuids;
        GuidVector m_vGeneratorGuids;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_blackwing_lair(pMap);
    }
};

void AddSC_instance_blackwing_lair()
{
    Script* s;
    s = new is_blackwing_lair();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_blackwing_lair";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_blackwing_lair;
    //pNewScript->RegisterSelf();
}
