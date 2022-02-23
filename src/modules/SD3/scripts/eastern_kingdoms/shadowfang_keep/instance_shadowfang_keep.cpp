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
 * SDName:      Instance_Shadowfang_Keep
 * SD%Complete: 90
 * SDComment:   None
 * SDCategory:  Shadowfang Keep
 * EndScriptData
 */

#include "precompiled.h"
#include "shadowfang_keep.h"

struct is_shadowfang_keep : public InstanceScript
{
    is_shadowfang_keep() : InstanceScript("instance_shadowfang_keep") {}

    class instance_shadowfang_keep : public ScriptedInstance
    {
    public:
#if defined (CLASSIC) || defined (TBC)
        instance_shadowfang_keep(Map* pMap) : ScriptedInstance(pMap)
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
        instance_shadowfang_keep(Map* pMap) : ScriptedInstance(pMap), m_uiApothecaryDead(0)
#endif
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
            case NPC_ASH:
            case NPC_ADA:
            case NPC_FENRUS:
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
        case NPC_HUMMEL:
        case NPC_FRYE:
        case NPC_BAXTER:
        case NPC_APOTHECARY_GENERATOR:
        case NPC_VALENTINE_BOSS_MGR:
#endif
                break;
            case NPC_VINCENT:
                // If Arugal has done the intro, make Vincent dead!
                if (m_auiEncounter[4] == DONE)
                {
                    pCreature->SetStandState(UNIT_STAND_STATE_DEAD);
                }
                break;

            default:
                return;
            }
            m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_COURTYARD_DOOR:
                if (m_auiEncounter[0] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
                // For this we ignore voidwalkers, because if the server restarts
                // They won't be there, but Fenrus is dead so the door can't be opened!
            case GO_SORCERER_DOOR:
                if (m_auiEncounter[2] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_ARUGAL_DOOR:
                if (m_auiEncounter[3] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_ARUGAL_FOCUS:
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            case GO_APOTHECARE_VIALS:
            case GO_CHEMISTRY_SET:
#endif
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void DoSpeech()
        {
            Creature* pAda = GetSingleCreatureFromStorage(NPC_ADA);
            Creature* pAsh = GetSingleCreatureFromStorage(NPC_ASH);

            if (pAda && pAda->IsAlive() && pAsh && pAsh->IsAlive())
            {
                DoScriptText(SAY_BOSS_DIE_AD, pAda);
                DoScriptText(SAY_BOSS_DIE_AS, pAsh);
            }
        }

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
                // Remove lootable flag from Hummel
                // Instance data is set to SPECIAL because the encounter depends on multiple bosses
            case NPC_HUMMEL:
                pCreature->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                DoScriptText(SAY_HUMMEL_DEATH, pCreature);
                // no break;
            case NPC_FRYE:
            case NPC_BAXTER:
                SetData(TYPE_APOTHECARY, SPECIAL);
                break;
            }
        }

        void OnCreatureEvade(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_HUMMEL:
            case NPC_FRYE:
            case NPC_BAXTER:
                SetData(TYPE_APOTHECARY, FAIL);
                break;
            }
        }
#endif

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_FREE_NPC:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_COURTYARD_DOOR);
                }
                m_auiEncounter[0] = uiData;
                break;
            case TYPE_RETHILGORE:
                if (uiData == DONE)
                {
                    DoSpeech();
                }
                m_auiEncounter[1] = uiData;
                break;
            case TYPE_FENRUS:
                if (uiData == DONE)
                {
                    if (Creature* pFenrus = GetSingleCreatureFromStorage(NPC_FENRUS))
                    {
                        pFenrus->SummonCreature(NPC_ARCHMAGE_ARUGAL, -136.89f, 2169.17f, 136.58f, 2.794f, TEMPSPAWN_TIMED_DESPAWN, 30000);
                    }
                }
                m_auiEncounter[2] = uiData;
                break;
            case TYPE_NANDOS:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_ARUGAL_DOOR);
                }
                m_auiEncounter[3] = uiData;
                break;
            case TYPE_INTRO:
                m_auiEncounter[4] = uiData;
                break;
            case TYPE_VOIDWALKER:
                if (uiData == DONE)
                {
                    m_auiEncounter[5]++;
                    if (m_auiEncounter[5] > 3)
                    {
                        DoUseDoorOrButton(GO_SORCERER_DOOR);
                    }
                }
                break;
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
        case TYPE_APOTHECARY:
            // Reset apothecary counter on fail
            if (uiData == IN_PROGRESS)
            {
                m_uiApothecaryDead = 0;
            }
            if (uiData == SPECIAL)
            {
                ++m_uiApothecaryDead;

                // Set Hummel as lootable only when the others are dead
                if (m_uiApothecaryDead == MAX_APOTHECARY)
                {
                    if (Creature* pHummel = GetSingleCreatureFromStorage(NPC_HUMMEL))
                    {
                        pHummel->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                    }

                    SetData(TYPE_APOTHECARY, DONE);
                }
            }
            // We don't want to store the SPECIAL data
            else
            {
                m_auiEncounter[6] = uiData;
            }
            break;
#endif
    }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " " << m_auiEncounter[3]
#if defined (CLASSIC) || defined (TBC)
                    << " " << m_auiEncounter[4] << " " << m_auiEncounter[5];
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
                   << " " << m_auiEncounter[4] << " " << m_auiEncounter[5] << " " << m_auiEncounter[6];
#endif
                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            switch (uiType)
            {
            case TYPE_FREE_NPC:
                return m_auiEncounter[0];
            case TYPE_RETHILGORE:
                return m_auiEncounter[1];
            case TYPE_FENRUS:
                return m_auiEncounter[2];
            case TYPE_NANDOS:
                return m_auiEncounter[3];
            case TYPE_INTRO:
                return m_auiEncounter[4];
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            case TYPE_APOTHECARY:
                return m_auiEncounter[6];
#endif
            default:
                return 0;
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
#if defined (CLASSIC) || defined (TBC)
                >> m_auiEncounter[4] >> m_auiEncounter[5];
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
               >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6];
#endif
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

        uint8 m_uiApothecaryDead;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_shadowfang_keep(pMap);
    }
};

void AddSC_instance_shadowfang_keep()
{
    Script* s;
    s = new is_shadowfang_keep();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_shadowfang_keep";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_shadowfang_keep;
    //pNewScript->RegisterSelf();
}
