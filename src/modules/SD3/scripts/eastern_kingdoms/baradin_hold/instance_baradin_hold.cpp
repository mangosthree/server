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

/* ScriptData
SDName: instance_baradin_hold
SD%Complete: 0%
SDComment:
SDCategory: Baradin Hold
EndScriptData */

#include "precompiled.h"
#include "baradin_hold.h"

struct is_baradin_hold : public InstanceScript
{
    is_baradin_hold() : InstanceScript("instance_baradin_hold") {}

    class instance_baradin_hold : public ScriptedInstance
    {
    public:
        instance_baradin_hold(Map* pMap) : ScriptedInstance(pMap)
        {
            Initialize();
        }

        ~instance_baradin_hold() {}

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

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

        void OnPlayerEnter(Player* pPlayer) override
        {

        }

        void OnObjectCreate(GameObject* pGo) override
        {

        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_ARGALOTH:
            case NPC_OCCUTHAR:
            case NPC_ALIZABAL:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {

        }

        void OnCreatureEvade(Creature* pCreature)
        {

        }

        void SetData(uint32 uiType, uint32 uiData) override
        {

        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
            {
                return m_auiEncounter[uiType];
            }

            return 0;
        }

        void Update(uint32 uiDiff) override
        {
            // DialogueUpdate(uiDiff);

        }

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* in) override
        {
            if (!in)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(in);

            std::istringstream loadStream(in);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }
    protected:
        bool m_bFirstSpecialDone;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_baradin_hold(pMap);
    }
};

struct at_baradin_hold : public AreaTriggerScript
{
    at_baradin_hold() : AreaTriggerScript("at_baradin_hold") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        return false;
    }
};

void AddSC_instance_baradin_hold()
{
    Script* s;

    s = new is_baradin_hold();
    s->RegisterSelf();
    s = new at_baradin_hold();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_baradin_hold";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_baradin_hold;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_baradin_hold";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_baradin_hold;
    //pNewScript->RegisterSelf();
}
