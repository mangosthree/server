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
SDName: Instance_Dark_Portal
SD%Complete: 75
SDComment: Quest support: 9836, 10297. Some visuals for the event are missing; Event epilogue NYI.
SDCategory: Caverns of Time, The Dark Portal
EndScriptData */

#include "precompiled.h"
#include "dark_portal.h"

struct PortalData
{
    float fX, fY, fZ, fOrient;
};

static const PortalData afPortalLocation[] =
{
    { -2030.832f, 7024.944f, 23.07182f, 3.141593f },
    { -1961.734f, 7029.528f, 21.8114f, 2.129302f },
    { -1887.695f, 7106.557f, 22.0495f, 4.956735f },
    { -1930.911f, 7183.597f, 23.00764f, 3.595378f }
};

// Dark Crystal summon location
static const float fDarkPortalCrystalLoc[3] = { -2024.31f, 7127.75f, 22.65419f };

static const int32 uiMedivhWeakYell[3] = { SAY_MEDIVH_WEAK75, SAY_MEDIVH_WEAK50, SAY_MEDIVH_WEAK25 };

struct is_dark_portal : public InstanceScript
{
    is_dark_portal() : InstanceScript("instance_dark_portal") {}

    class instance_dark_portal : public ScriptedInstance
    {
    public:
        instance_dark_portal(Map* pMap) : ScriptedInstance(pMap),
            m_uiWorldState(0),
            m_uiWorldStateRiftCount(0),
            m_uiWorldStateShieldCount(100),

            m_bHasIntroYelled(false),
            m_uiMedivhYellCount(1),

            m_uiNextPortalTimer(0),
            m_uiCurrentRiftId(0)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            UpdateWorldState(m_auiEncounter[TYPE_MEDIVH] == IN_PROGRESS ? true : false);
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_MEDIVH:
            case NPC_SAAT:
            case NPC_DARK_PORTAL_DUMMY:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            }
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_CHRONO_LORD_DEJA:
            case NPC_CHRONO_LORD:
                SetData(TYPE_CHRONO_LORD, IN_PROGRESS);
                break;
            case NPC_TEMPORUS:
            case NPC_TIMEREAVER:
                SetData(TYPE_TEMPORUS, IN_PROGRESS);
                break;
            case NPC_AEONUS:
                SetData(TYPE_AEONUS, IN_PROGRESS);
                // no break
            case NPC_ASSASSIN:
            case NPC_WHELP:
            case NPC_CHRONOMANCER:
            case NPC_EXECUTIONER:
            case NPC_VANQUISHER:
                pCreature->InterruptNonMeleeSpells(false);
                break;
            }
        }

        void OnCreatureEvade(Creature* pCreature)
        {
            switch (pCreature->GetEntry())
            {
            case NPC_CHRONO_LORD_DEJA:
            case NPC_CHRONO_LORD:
                SetData(TYPE_CHRONO_LORD, FAIL);
                break;
            case NPC_TEMPORUS:
            case NPC_TIMEREAVER:
                SetData(TYPE_TEMPORUS, FAIL);
                break;
            case NPC_AEONUS:
                SetData(TYPE_AEONUS, FAIL);
                // no break;
                // Allow these guys to go and finish off Medivh
            case NPC_ASSASSIN:
            case NPC_WHELP:
            case NPC_CHRONOMANCER:
            case NPC_EXECUTIONER:
            case NPC_VANQUISHER:
                if (Creature* pMedivh = GetSingleCreatureFromStorage(NPC_MEDIVH))
                {
                    float fX, fY, fZ;
                    pMedivh->GetNearPoint(pMedivh, fX, fY, fZ, 0, 20.0f, pMedivh->GetAngle(pCreature));
                    pCreature->GetMotionMaster()->MovePoint(1, fX, fY, fZ);
                }
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_CHRONO_LORD_DEJA:
            case NPC_CHRONO_LORD:
                SetData(TYPE_CHRONO_LORD, DONE);
                break;
            case NPC_TEMPORUS:
            case NPC_TIMEREAVER:
                SetData(TYPE_TEMPORUS, DONE);
                break;
            case NPC_AEONUS:
                SetData(TYPE_AEONUS, DONE);
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_MEDIVH:
                if (uiData == IN_PROGRESS)
                {
                    if (Creature* pMedivh = GetSingleCreatureFromStorage(NPC_MEDIVH))
                    {
                        if (pMedivh->IsAlive())
                        {
                            DoScriptText(SAY_MEDIVH_ENTER, pMedivh);
                        }
                        // If Medivh is not available then do not store the uiData;
                        else
                        {
                            return;
                        }
                    }

                    // TODO:
                    // Start the Portal Crystal casting - by the Dark Portal Dumm Npc
                    // Also Start summoning the Dark Portal Beams

                    UpdateWorldState();
                    m_uiNextPortalTimer = 3000;
                }
                else if (uiData == DONE)
                {
                    // Yell for event finished
                    if (Creature* pMedivh = GetSingleCreatureFromStorage(NPC_MEDIVH))
                    {
                        DoScriptText(SAY_MEDIVH_WIN, pMedivh);
                        pMedivh->SetFacingTo(6.15f);
                        pMedivh->InterruptNonMeleeSpells(false);
                        pMedivh->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                    }

                    // this may be completed further out in the post-event
                    Map::PlayerList const& players = instance->GetPlayers();

                    if (!players.isEmpty())
                    {
                        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                        {
                            if (Player* pPlayer = itr->getSource())
                            {
                                if (pPlayer->GetQuestStatus(QUEST_OPENING_PORTAL) == QUEST_STATUS_INCOMPLETE)
                                {
                                    pPlayer->AreaExploredOrEventHappens(QUEST_OPENING_PORTAL);
                                }

                                if (pPlayer->GetQuestStatus(QUEST_MASTER_TOUCH) == QUEST_STATUS_INCOMPLETE)
                                {
                                    pPlayer->AreaExploredOrEventHappens(QUEST_MASTER_TOUCH);
                                }
                            }
                        }
                    }
                }
                else if (uiData == FAIL)
                {
                    DoResetEvent();
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_SHIELD:
                if (uiData == SPECIAL)
                {
                    --m_uiWorldStateShieldCount;
                    DoUpdateWorldState(WORLD_STATE_SHIELD, m_uiWorldStateShieldCount);

                    // Yell at 75%, 50% and 25% shield
                    if (m_uiWorldStateShieldCount < 100 - 25 * m_uiMedivhYellCount)
                    {
                        if (Creature* pMedivh = GetSingleCreatureFromStorage(NPC_MEDIVH))
                        {
                            DoScriptText(uiMedivhWeakYell[m_uiMedivhYellCount - 1], pMedivh);
                            ++m_uiMedivhYellCount;
                        }
                    }

                    // Kill the npc when the shield is broken
                    if (!m_uiWorldStateShieldCount)
                    {
                        if (Creature* pMedivh = GetSingleCreatureFromStorage(NPC_MEDIVH))
                        {
                            if (pMedivh->IsAlive())
                            {
                                pMedivh->DealDamage(pMedivh, pMedivh->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
                            }
                        }
                    }
                }
                m_auiEncounter[uiType] = uiData;
                return;
            case TYPE_TIME_RIFT:
                // Set the delay to the next time rift from the point the rift despawns
                if (uiData == DONE)
                {
                    m_uiNextPortalTimer = IsBossTimeRift() ? 125000 : 3000;
                }
                // Set the delay to the next time rift from the point the rift summons it's guardian
                // ToDo: research if these timers are correct
                else if (uiData == SPECIAL)
                {
                    m_uiNextPortalTimer = IsBossTimeRift() ? 0 : m_uiWorldStateRiftCount > 12 ? 90000 : 2 * MINUTE * IN_MILLISECONDS;
                }

                m_auiEncounter[uiType] = uiData;
                return;
            case TYPE_CHRONO_LORD:
            case TYPE_TEMPORUS:
                if (m_auiEncounter[uiType] != DONE)             // Keep the DONE-information stored
                {
                    m_auiEncounter[uiType] = uiData;
                }
                break;
            case TYPE_AEONUS:
                if (uiData == DONE)
                {
                    UpdateWorldState(false);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_HANDLE_AREATRIGGER:
                DoHandleAreaTrigger(uiData);
                return;
            default:
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5];

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

            if (uiType == TYPE_RIFT_ID)
            {
                return m_uiWorldStateRiftCount;
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
                >> m_auiEncounter[4] >> m_auiEncounter[5];

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
            if (GetData(TYPE_MEDIVH) != IN_PROGRESS)
            {
                return;
            }

            if (m_uiNextPortalTimer)
            {
                if (m_uiNextPortalTimer <= uiDiff)
                {
                    DoUpdateWorldState(WORLD_STATE_RIFT, ++m_uiWorldStateRiftCount);

                    DoSpawnNextPortal();
                    m_uiNextPortalTimer = 0;
                }
                else
                {
                    m_uiNextPortalTimer -= uiDiff;
                }
            }
        }

    private:
        bool IsBossTimeRift() { return m_uiWorldStateRiftCount == 6 || m_uiWorldStateRiftCount == 12; }
        void UpdateWorldState(bool bEnable = true)
        {
            m_uiWorldState = bEnable ? 1 : 0;

            DoUpdateWorldState(WORLD_STATE_ID, m_uiWorldState);
            DoUpdateWorldState(WORLD_STATE_SHIELD, m_uiWorldStateShieldCount);
            DoUpdateWorldState(WORLD_STATE_RIFT, m_uiWorldStateRiftCount);
        }

        void DoSpawnNextPortal()
        {
            if (Creature* pMedivh = GetSingleCreatureFromStorage(NPC_MEDIVH))
            {
                // Randomize portal locations
                uint8 uiTmp = urand(0, 2);

                if (uiTmp >= m_uiCurrentRiftId)
                {
                    ++uiTmp;
                }

                debug_log("SD3: instance_dark_portal: SetRiftId %u, old was id %u.", uiTmp, m_uiCurrentRiftId);

                m_uiCurrentRiftId = uiTmp;

                // Summon next portal
                pMedivh->SummonCreature(NPC_TIME_RIFT, afPortalLocation[uiTmp].fX, afPortalLocation[uiTmp].fY, afPortalLocation[uiTmp].fZ, afPortalLocation[uiTmp].fOrient, TEMPSPAWN_CORPSE_DESPAWN, 0);
            }
        }

        void DoResetEvent()
        {
            UpdateWorldState(false);

            m_uiWorldStateShieldCount = 100;
            m_uiWorldStateRiftCount = 0;

            m_uiCurrentRiftId = 0;
            m_uiNextPortalTimer = 0;
            m_uiMedivhYellCount = 1;
        }

        void DoHandleAreaTrigger(uint32 uiTriggerId)
        {
            if (uiTriggerId == AREATRIGGER_ENTER)
            {
                // Yell at instance entrance
                if (!m_bHasIntroYelled)
                {
                    if (Creature* pSaat = GetSingleCreatureFromStorage(NPC_SAAT))
                    {
                        DoScriptText(SAY_SAAT_WELCOME, pSaat);
                    }
                    m_bHasIntroYelled = true;
                }
            }
            else if (uiTriggerId == AREATRIGGER_MEDIVH)
            {
                // Start Dark Portal event
                if (GetData(TYPE_MEDIVH) == NOT_STARTED || GetData(TYPE_MEDIVH) == FAIL)
                {
                    SetData(TYPE_MEDIVH, IN_PROGRESS);
                }
                // Start Epilogue
                else if (GetData(TYPE_AEONUS) == DONE && GetData(TYPE_MEDIVH) != DONE)
                {
                    SetData(TYPE_MEDIVH, DONE);
                }
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiWorldState;
        uint32 m_uiWorldStateRiftCount;
        uint32 m_uiWorldStateShieldCount;

        bool m_bHasIntroYelled;
        uint32 m_uiMedivhYellCount;

        uint32 m_uiNextPortalTimer;
        uint8 m_uiCurrentRiftId;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_dark_portal(pMap);
    }
};

struct at_dark_portal : public AreaTriggerScript
{
    at_dark_portal() : AreaTriggerScript("at_dark_portal") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (pAt->id == AREATRIGGER_MEDIVH || pAt->id == AREATRIGGER_ENTER)
        {
            if (pPlayer->isGameMaster() || pPlayer->IsDead())
            {
                return false;
            }

            if (InstanceData* pInstance = pPlayer->GetInstanceData())
            {
                pInstance->SetData(TYPE_HANDLE_AREATRIGGER, pAt->id);
            }
        }

        return false;
    }
};

void AddSC_instance_dark_portal()
{
    Script* s;

    s = new is_dark_portal();
    s->RegisterSelf();
    s = new at_dark_portal();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_dark_portal";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_dark_portal;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_dark_portal";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_dark_portal;
    //pNewScript->RegisterSelf();
}
