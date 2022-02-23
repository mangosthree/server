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
SDName: Instance_Mechanar
SD%Complete: 70
SDComment: Elevator needs core support
SDCategory: Mechanar
EndScriptData */

#include "precompiled.h"
#include "mechanar.h"

struct SpawnLocation
{
    uint32 m_uiSpawnEntry;
    float m_fX, m_fY, m_fZ, m_fO;
};

static const SpawnLocation aBridgeEventLocs[MAX_BRIDGE_LOCATIONS][4] =
{
    {
        { NPC_ASTROMAGE, 243.9323f, -24.53621f, 26.3284f, 0 },
        { NPC_ASTROMAGE, 240.5847f, -21.25438f, 26.3284f, 0 },
        { NPC_PHYSICIAN, 238.4178f, -25.92982f, 26.3284f, 0 },
        { NPC_CENTURION, 237.1122f, -19.14261f, 26.3284f, 0 },
    },
    {
        { NPC_FORGE_DESTROYER, 199.945f, -22.85885f, 24.95783f, 0 },
        { 0, 0, 0, 0, 0 },
    },
    {
        { NPC_ENGINEER, 179.8642f, -25.84609f, 24.8745f, 0 },
        { NPC_ENGINEER, 181.9983f, -17.56084f, 24.8745f, 0 },
        { NPC_PHYSICIAN, 183.4078f, -22.46612f, 24.8745f, 0 },
        { 0, 0, 0, 0, 0 },
    },
    {
        { NPC_ENGINEER, 141.0496f, 37.86048f, 24.87399f, 4.65f },
        { NPC_ASTROMAGE, 137.6626f, 34.89631f, 24.8742f, 4.65f },
        { NPC_PHYSICIAN, 135.3587f, 38.03816f, 24.87417f, 4.65f },
        { 0, 0, 0, 0, 0 },
    },
    {
        { NPC_FORGE_DESTROYER, 137.8275f, 53.18128f, 24.95783f, 4.65f },
        { 0, 0, 0, 0, 0 },
    },
    {
        { NPC_PHYSICIAN, 134.3062f, 109.1506f, 26.45663f, 4.65f },
        { NPC_ASTROMAGE, 135.3307f, 99.96439f, 26.45663f, 4.65f },
        { NPC_NETHERBINDER, 141.3976f, 102.7863f, 26.45663f, 4.65f },
        { NPC_ENGINEER, 140.8281f, 112.0363f, 26.45663f, 4.65f },
    },
    {
        { NPC_PATHALEON, 139.5425f, 149.3192f, 25.65904f, 4.63f },
        { 0, 0, 0, 0, 0 },
    },
};

struct is_mechanar : public InstanceScript
{
    is_mechanar() : InstanceScript("instance_mechanar") {}

    class instance_mechanar : public ScriptedInstance
    {
    public:
        instance_mechanar(Map* pMap) : ScriptedInstance(pMap),
            m_uiBridgeEventTimer(0),
            m_uiBridgeEventPhase(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            // Check encounter states
            if (GetData(TYPE_SEPETHREA) != DONE || GetData(TYPE_PATHALEON) == DONE)
            {
                return;
            }

            // Check if already summoned
            if (GetSingleCreatureFromStorage(NPC_PATHALEON, true))
            {
                return;
            }

            pPlayer->SummonCreature(aBridgeEventLocs[6][0].m_uiSpawnEntry, aBridgeEventLocs[6][0].m_fX, aBridgeEventLocs[6][0].m_fY, aBridgeEventLocs[6][0].m_fZ, aBridgeEventLocs[6][0].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0);
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            if (pGo->GetEntry() == GO_FACTORY_ELEVATOR)
            {
                // ToDo: activate elevator if TYPE_GYRO_KILL && TYPE_IRON_HAND && TYPE_CAPACITUS are DONE
                m_mGoEntryGuidStore[GO_FACTORY_ELEVATOR] = pGo->GetObjectGuid();
            }
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_PATHALEON:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_ASTROMAGE:
            case NPC_PHYSICIAN:
            case NPC_CENTURION:
            case NPC_ENGINEER:
            case NPC_NETHERBINDER:
            case NPC_FORGE_DESTROYER:
                if (pCreature->IsTemporarySummon())
                {
                    m_sBridgeTrashGuidSet.insert(pCreature->GetObjectGuid());
                }
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_GYRO_KILL:      SetData(TYPE_GYRO_KILL, DONE); break;
            case NPC_IRON_HAND:      SetData(TYPE_IRON_HAND, DONE); break;
            case NPC_LORD_CAPACITUS: SetData(TYPE_CAPACITUS, DONE); break;

            case NPC_ASTROMAGE:
            case NPC_PHYSICIAN:
            case NPC_CENTURION:
            case NPC_ENGINEER:
            case NPC_NETHERBINDER:
            case NPC_FORGE_DESTROYER:
                if (m_sBridgeTrashGuidSet.find(pCreature->GetObjectGuid()) != m_sBridgeTrashGuidSet.end())
                {
                    m_sBridgeTrashGuidSet.erase(pCreature->GetObjectGuid());

                    if (m_sBridgeTrashGuidSet.empty())
                    {
                        // After the 3rd wave wait 10 seconds
                        if (m_uiBridgeEventPhase == 3)
                        {
                            m_uiBridgeEventTimer = 10000;
                        }
                        else
                        {
                            DoSpawnBridgeWave();
                        }
                    }
                }
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_GYRO_KILL:
            case TYPE_IRON_HAND:
            case TYPE_CAPACITUS:
                m_auiEncounter[uiType] = uiData;
                // ToDo: Activate the Elevator when all these 3 are done
                break;
            case TYPE_SEPETHREA:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    m_uiBridgeEventTimer = 10000;
                }
                break;
            case TYPE_PATHALEON:
                m_auiEncounter[uiType] = uiData;
                break;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;

                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4];

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
                >> m_auiEncounter[4];

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
            if (m_uiBridgeEventTimer)
            {
                if (m_uiBridgeEventTimer <= uiDiff)
                {
                    DoSpawnBridgeWave();
                    m_uiBridgeEventTimer = 0;
                }
                else
                {
                    m_uiBridgeEventTimer -= uiDiff;
                }
            }
        }

    private:
        void DoSpawnBridgeWave()
        {
            if (Player* pPlayer = GetPlayerInMap(true, false))
            {
                for (uint8 i = 0; i < MAX_BRIDGE_TRASH; ++i)
                {
                    // Skip the blank entries
                    if (aBridgeEventLocs[m_uiBridgeEventPhase][i].m_uiSpawnEntry == 0)
                    {
                        break;
                    }

                    if (Creature* pTemp = pPlayer->SummonCreature(aBridgeEventLocs[m_uiBridgeEventPhase][i].m_uiSpawnEntry, aBridgeEventLocs[m_uiBridgeEventPhase][i].m_fX, aBridgeEventLocs[m_uiBridgeEventPhase][i].m_fY, aBridgeEventLocs[m_uiBridgeEventPhase][i].m_fZ, aBridgeEventLocs[m_uiBridgeEventPhase][i].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0))
                    {
                        pTemp->CastSpell(pTemp, SPELL_ETHEREAL_TELEPORT, false);

                        switch (m_uiBridgeEventPhase)
                        {
                        case 1:                                 // These waves should attack the player directly
                        case 2:
                        case 4:
                        case 5:
                            pTemp->AI()->AttackStart(pPlayer);
                            break;
                        case 6:                                 // Pathaleon
                            DoScriptText(SAY_PATHALEON_INTRO, pTemp);
                            break;
                        }
                    }
                }
            }
            ++m_uiBridgeEventPhase;
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiBridgeEventTimer;
        uint8 m_uiBridgeEventPhase;

        GuidSet m_sBridgeTrashGuidSet;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_mechanar(pMap);
    }
};

void AddSC_instance_mechanar()
{
    Script* s;

    s = new is_mechanar();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_mechanar";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_mechanar;
    //pNewScript->RegisterSelf();
}
