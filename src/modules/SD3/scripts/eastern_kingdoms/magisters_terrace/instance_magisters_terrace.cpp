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

/* ScriptData
SDName: Instance_Magisters_Terrace
SD%Complete: 80
SDComment:
SDCategory: Magister's Terrace
EndScriptData */

#include "precompiled.h"
#include "magisters_terrace.h"

/*
0  - Selin Fireheart
1  - Vexallus
2  - Priestess Delrissa
3  - Kael'thas Sunstrider
*/

static const int32 aDelrissaAddDeath[MAX_DELRISSA_ADDS] = { -1585013, -1585014, -1585015, -1585016 };

struct is_magisters_terrace : public InstanceScript
{
    is_magisters_terrace() : InstanceScript("instance_magisters_terrace") {}

    class instance_magisters_terrace : public ScriptedInstance
    {
    public:
        instance_magisters_terrace(Map* pMap) : ScriptedInstance(pMap),
            m_uiDelrissaDeathCount(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_SELIN_FIREHEART:
            case NPC_DELRISSA:
            case NPC_KALECGOS_DRAGON:
            case NPC_KAELTHAS:
                // insert Delrissa adds here, for better handling
            case NPC_KAGANI:
            case NPC_ELLRYS:
            case NPC_ERAMAS:
            case NPC_YAZZAI:
            case NPC_SALARIS:
            case NPC_GARAXXAS:
            case NPC_APOKO:
            case NPC_ZELFAN:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_FEL_CRYSTAL:
                m_lFelCrystalGuid.push_back(pCreature->GetObjectGuid());
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_VEXALLUS_DOOR:
                if (m_auiEncounter[TYPE_VEXALLUS] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_SELIN_DOOR:
                if (m_auiEncounter[TYPE_SELIN] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_DELRISSA_DOOR:
                if (m_auiEncounter[TYPE_DELRISSA] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_SELIN_ENCOUNTER_DOOR:
            case GO_KAEL_DOOR:
            case GO_ESCAPE_QUEL_DANAS:
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_KAGANI:
            case NPC_ELLRYS:
            case NPC_ERAMAS:
            case NPC_YAZZAI:
            case NPC_SALARIS:
            case NPC_GARAXXAS:
            case NPC_APOKO:
            case NPC_ZELFAN:
                ++m_uiDelrissaDeathCount;
                if (m_uiDelrissaDeathCount == MAX_DELRISSA_ADDS)
                {
                    SetData(TYPE_DELRISSA, SPECIAL);
                }
                // yell on summoned death
                if (Creature* pDelrissa = GetSingleCreatureFromStorage(NPC_DELRISSA))
                {
                    if (pDelrissa->IsAlive())
                    {
                        DoScriptText(aDelrissaAddDeath[m_uiDelrissaDeathCount - 1], pDelrissa);
                    }
                    else if (GetData(TYPE_DELRISSA) == SPECIAL)
                    {
                        SetData(TYPE_DELRISSA, DONE);
                        pDelrissa->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                    }
                }
                break;
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

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_SELIN:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_SELIN_DOOR);
                }
                if (uiData == FAIL)
                {
                    // Reset crystals - respawn and kill is handled by creature linking
                    for (GuidList::const_iterator itr = m_lFelCrystalGuid.begin(); itr != m_lFelCrystalGuid.end(); ++itr)
                    {
                        if (Creature* pTemp = instance->GetCreature(*itr))
                        {
                            if (!pTemp->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                            {
                                pTemp->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            }

                            if (pTemp->IsAlive())
                            {
                                pTemp->AI()->EnterEvadeMode();
                            }
                        }
                    }
                }
                if (uiData == IN_PROGRESS)
                {
                    // Stop channeling when the fight starts
                    for (GuidList::const_iterator itr = m_lFelCrystalGuid.begin(); itr != m_lFelCrystalGuid.end(); ++itr)
                    {
                        if (Creature* pTemp = instance->GetCreature(*itr))
                        {
                            pTemp->InterruptNonMeleeSpells(false);
                        }
                    }
                }
                DoUseDoorOrButton(GO_SELIN_ENCOUNTER_DOOR);
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_VEXALLUS:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_VEXALLUS_DOOR);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_DELRISSA:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_DELRISSA_DOOR);
                }
                if (uiData == IN_PROGRESS)
                {
                    m_uiDelrissaDeathCount = 0;
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_KAELTHAS:
                DoUseDoorOrButton(GO_KAEL_DOOR);
                if (uiData == DONE)
                {
                    DoToggleGameObjectFlags(GO_ESCAPE_QUEL_DANAS, GO_FLAG_NO_INTERACT, false);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " " << m_auiEncounter[3];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3];

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
        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiDelrissaDeathCount;

        GuidList m_lFelCrystalGuid;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_magisters_terrace(pMap);
    }
};

void AddSC_instance_magisters_terrace()
{
    Script* s;
    s = new is_magisters_terrace();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_magisters_terrace";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_magisters_terrace;
    //pNewScript->RegisterSelf();
}
