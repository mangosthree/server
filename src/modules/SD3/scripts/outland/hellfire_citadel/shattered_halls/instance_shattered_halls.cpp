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
SDName: Instance_Shattered_Halls
SD%Complete: 50
SDComment: currently missing info about door. instance not complete
SDCategory: Hellfire Citadel, Shattered Halls
EndScriptData */

#include "precompiled.h"
#include "shattered_halls.h"

struct SpawnLocation
{
    uint32 m_uiAllianceEntry, m_uiHordeEntry;
    float m_fX, m_fY, m_fZ, m_fO;
};

const float afExecutionerLoc[4] = { 151.443f, -84.439f, 1.938f, 6.283f };

static SpawnLocation aSoldiersLocs[] =
{
    { 0, NPC_SOLDIER_HORDE_1, 119.609f, 256.127f, -45.254f, 5.133f },
    { NPC_SOLDIER_ALLIANCE_1, 0, 131.106f, 254.520f, -45.236f, 3.951f },
    { NPC_SOLDIER_ALLIANCE_3, NPC_SOLDIER_HORDE_3, 151.040f, -91.558f, 1.936f, 1.559f },
    { NPC_SOLDIER_ALLIANCE_2, NPC_SOLDIER_HORDE_2, 150.669f, -77.015f, 1.933f, 4.705f },
    { NPC_OFFICER_ALLIANCE, NPC_OFFICER_HORDE, 138.241f, -84.198f, 1.907f, 0.055f }
};

struct is_shattered_halls : public InstanceScript
{
    is_shattered_halls() : InstanceScript("instance_shattered_halls") {}

    class instance_shattered_halls : public ScriptedInstance
    {
    public:
        instance_shattered_halls(Map* pMap) : ScriptedInstance(pMap),
            m_uiExecutionTimer(55 * MINUTE* IN_MILLISECONDS),
            m_uiTeam(0),
            m_uiExecutionStage(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            // Only on heroic
            if (instance->IsRegularDifficulty() || m_uiTeam)
            {
                return;
            }

            m_uiTeam = pPlayer->GetTeam();

            if (m_uiTeam == ALLIANCE)
            {
                pPlayer->SummonCreature(aSoldiersLocs[1].m_uiAllianceEntry, aSoldiersLocs[1].m_fX, aSoldiersLocs[1].m_fY, aSoldiersLocs[1].m_fZ, aSoldiersLocs[1].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0);
            }
            else
            {
                pPlayer->SummonCreature(aSoldiersLocs[0].m_uiHordeEntry, aSoldiersLocs[0].m_fX, aSoldiersLocs[0].m_fY, aSoldiersLocs[0].m_fZ, aSoldiersLocs[0].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0);
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_NETHEKURSE_DOOR:
                if (m_auiEncounter[TYPE_NETHEKURSE] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_NETHEKURSE_ENTER_DOOR:
                if (m_auiEncounter[TYPE_NETHEKURSE] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;

            default:
                return;
            }

            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_NETHEKURSE:
            case NPC_KARGATH_BLADEFIST:
            case NPC_EXECUTIONER:
            case NPC_SOLDIER_ALLIANCE_2:
            case NPC_SOLDIER_ALLIANCE_3:
            case NPC_OFFICER_ALLIANCE:
            case NPC_SOLDIER_HORDE_2:
            case NPC_SOLDIER_HORDE_3:
            case NPC_OFFICER_HORDE:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_EXECUTIONER)
            {
                SetData(TYPE_EXECUTION, DONE);
            }
        }

        void OnCreatureEvade(Creature* pCreature) override
        {
            // If npc evades continue the counting
            if (pCreature->GetEntry() == NPC_EXECUTIONER)
            {
                SetData(TYPE_EXECUTION, IN_PROGRESS);
            }
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            // Set data to special in order to pause the event timer
            // This is according to the blizz comments which say that it is possible to complete the event if you engage the npc while you have only a few seconds left
            if (pCreature->GetEntry() == NPC_EXECUTIONER)
            {
                SetData(TYPE_EXECUTION, SPECIAL);
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_NETHEKURSE:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_NETHEKURSE_DOOR);
                    DoUseDoorOrButton(GO_NETHEKURSE_ENTER_DOOR);
                }
                break;
            case TYPE_OMROGG:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_BLADEFIST:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    // Make executioner attackable only after the final boss is dead
                    if (Creature* pExecutioner = GetSingleCreatureFromStorage(NPC_EXECUTIONER, true))
                    {
                        pExecutioner->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
                break;
            case TYPE_EXECUTION:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS && !GetSingleCreatureFromStorage(NPC_EXECUTIONER, true))
                {
                    if (Player* pPlayer = GetPlayerInMap())
                    {
                        // summon the 3 npcs for execution
                        for (uint8 i = 2; i < 5; ++i)
                        {
                            pPlayer->SummonCreature(m_uiTeam == ALLIANCE ? aSoldiersLocs[i].m_uiAllianceEntry : aSoldiersLocs[i].m_uiHordeEntry, aSoldiersLocs[i].m_fX, aSoldiersLocs[i].m_fY, aSoldiersLocs[i].m_fZ, aSoldiersLocs[i].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0);
                        }

                        // Summon the executioner; Note: according to wowhead he shouldn't be targetable until Kargath encounter is finished
                        if (Creature* pExecutioner = pPlayer->SummonCreature(NPC_EXECUTIONER, afExecutionerLoc[0], afExecutionerLoc[1], afExecutionerLoc[2], afExecutionerLoc[3], TEMPSPAWN_DEAD_DESPAWN, 0, true))
                        {
                            pExecutioner->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                        }

                        // cast the execution spell
                        DoCastGroupDebuff(SPELL_KARGATH_EXECUTIONER_1);
                    }
                }
                if (uiData == DONE)
                {
                    // If the officer is already killed, then skip the quest completion
                    if (m_uiExecutionStage)
                    {
                        break;
                    }

                    // Complete quest 9524 or 9525
                    if (Creature* pOfficer = GetSingleCreatureFromStorage(m_uiTeam == ALLIANCE ? NPC_OFFICER_ALLIANCE : NPC_OFFICER_HORDE))
                    {
                        Map::PlayerList const& lPlayers = instance->GetPlayers();

                        for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
                        {
                            if (Player* pPlayer = itr->getSource())
                            {
                                pPlayer->KilledMonsterCredit(pOfficer->GetEntry(), pOfficer->GetObjectGuid());
                            }
                        }
                    }
                }
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

        bool CheckConditionCriteriaMeet(Player const* pPlayer, uint32 uiInstanceConditionId, WorldObject const* pConditionSource, uint32 conditionSourceType) const override
        {
            switch (uiInstanceConditionId)
            {
            case INSTANCE_CONDITION_ID_NORMAL_MODE:             // No soldier alive
            case INSTANCE_CONDITION_ID_HARD_MODE:               // One soldier alive
            case INSTANCE_CONDITION_ID_HARD_MODE_2:             // Two soldier alive
            case INSTANCE_CONDITION_ID_HARD_MODE_3:             // Three soldier alive
                return uiInstanceConditionId == uint32(INSTANCE_CONDITION_ID_HARD_MODE_3 - m_uiExecutionStage);
            }

            script_error_log("instance_shattered_halls::CheckConditionCriteriaMeet called with unsupported Id %u. Called with param plr %s, src %s, condition source type %u",
                uiInstanceConditionId, pPlayer ? pPlayer->GetGuidStr().c_str() : "nullptr", pConditionSource ? pConditionSource->GetGuidStr().c_str() : "nullptr", conditionSourceType);
            return false;
        }

        void Update(uint32 uiDiff) override
        {
            if (m_auiEncounter[TYPE_EXECUTION] != IN_PROGRESS)
            {
                return;
            }

            if (m_uiExecutionTimer < uiDiff)
            {
                switch (m_uiExecutionStage)
                {
                case 0:
                    // Kill the officer
                    if (Creature* pSoldier = GetSingleCreatureFromStorage(m_uiTeam == ALLIANCE ? NPC_OFFICER_ALLIANCE : NPC_OFFICER_HORDE))
                    {
                        pSoldier->DealDamage(pSoldier, pSoldier->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
                    }

                    // Make Kargath yell
                    DoOrSimulateScriptTextForThisInstance(m_uiTeam == ALLIANCE ? SAY_KARGATH_EXECUTE_ALLY : SAY_KARGATH_EXECUTE_HORDE, NPC_KARGATH_BLADEFIST);

                    // Set timer for the next execution
                    DoCastGroupDebuff(SPELL_KARGATH_EXECUTIONER_2);
                    m_uiExecutionTimer = 10 * MINUTE * IN_MILLISECONDS;
                    break;
                case 1:
                    if (Creature* pSoldier = GetSingleCreatureFromStorage(m_uiTeam == ALLIANCE ? NPC_SOLDIER_ALLIANCE_2 : NPC_SOLDIER_HORDE_2))
                    {
                        pSoldier->DealDamage(pSoldier, pSoldier->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
                    }

                    DoCastGroupDebuff(SPELL_KARGATH_EXECUTIONER_3);
                    m_uiExecutionTimer = 15 * MINUTE * IN_MILLISECONDS;
                    break;
                case 2:
                    if (Creature* pSoldier = GetSingleCreatureFromStorage(m_uiTeam == ALLIANCE ? NPC_SOLDIER_ALLIANCE_3 : NPC_SOLDIER_HORDE_3))
                    {
                        pSoldier->DealDamage(pSoldier, pSoldier->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
                    }

                    SetData(TYPE_EXECUTION, FAIL);
                    m_uiExecutionTimer = 0;
                    break;
                }
                ++m_uiExecutionStage;
            }
            else
            {
                m_uiExecutionTimer -= uiDiff;
            }
        }

    private:
        // Add debuff to all players in the instance
        void DoCastGroupDebuff(uint32 uiSpellId)
        {
            Map::PlayerList const& lPlayers = instance->GetPlayers();

            for (Map::PlayerList::const_iterator itr = lPlayers.begin(); itr != lPlayers.end(); ++itr)
            {
                Player* pPlayer = itr->getSource();
                if (pPlayer && !pPlayer->HasAura(uiSpellId))
                {
                    pPlayer->CastSpell(pPlayer, uiSpellId, true);
                }
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiExecutionTimer;
        uint32 m_uiTeam;
        uint8 m_uiExecutionStage;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_shattered_halls(pMap);
    }
};

struct at_shattered_halls : public AreaTriggerScript
{
    at_shattered_halls() : AreaTriggerScript("at_shattered_halls") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* /*pAt*/) override
    {
        if (pPlayer->isGameMaster() || !pPlayer->IsAlive())
        {
            return false;
        }

        InstanceData* pInstance = pPlayer->GetInstanceData();

        if (!pInstance)
        {
            return false;
        }

        // Only on heroic
        if (pInstance->instance->IsRegularDifficulty())
        {
            return false;
        }

        // Don't allow players to cheat
        if (pInstance->GetData(TYPE_BLADEFIST) == DONE || pInstance->GetData(TYPE_OMROGG) == DONE)
        {
            return false;
        }

        if (pInstance->GetData(TYPE_EXECUTION) == NOT_STARTED)
        {
            pInstance->SetData(TYPE_EXECUTION, IN_PROGRESS);
        }

        return true;
    }
};

void AddSC_instance_shattered_halls()
{
    Script* s;

    s = new is_shattered_halls();
    s->RegisterSelf();
    s = new at_shattered_halls();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_shattered_halls";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_shattered_halls;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_shattered_halls";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_shattered_halls;
    //pNewScript->RegisterSelf();
}
