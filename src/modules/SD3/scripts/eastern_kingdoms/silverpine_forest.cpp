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
 * SDName:      Silverpine_Forest
 * SD%Complete: 100
 * SDComment:   Quest support: 435, 452.
 * SDCategory:  Silverpine Forest
 * EndScriptData
 */

/**
 * ContentData
 * npc_deathstalker_erland
 * npc_deathstalker_faerleia
 * EndContentData
 */

#include "precompiled.h"
#include "escort_ai.h"

/*#####
## npc_deathstalker_erland
#####*/

enum
{
    SAY_START_1         = -1000306,
    SAY_START_2         = -1000307,
    SAY_AGGRO_1         = -1000308,
    SAY_AGGRO_2         = -1000309,
    SAY_AGGRO_3         = -1000310,
    SAY_PROGRESS        = -1000311,
    SAY_END             = -1000312,
    SAY_RANE            = -1000313,
    SAY_RANE_REPLY      = -1000314,
    SAY_CHECK_NEXT      = -1000315,
    SAY_QUINN           = -1000316,
    SAY_QUINN_REPLY     = -1000317,
    SAY_BYE             = -1000318,

    QUEST_ERLAND        = 435,
    NPC_RANE            = 1950,
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    NPC_QUINN           = 1951
#endif
#if defined (CLASSIC) || defined (TBC)
    NPC_QUINN           = 1951,

    PHASE_RANE          = 0,
    PHASE_QUINN         = 1
#endif
};

struct npc_deathstalker_erland : public CreatureScript
{
    npc_deathstalker_erland() : CreatureScript("npc_deathstalker_erland") {}

    struct npc_deathstalker_erlandAI : public npc_escortAI
    {
        npc_deathstalker_erlandAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
            Reset();
        }
#if defined (CLASSIC) || defined (TBC)
        std::vector<Creature*> lCreatureList;
        uint32 m_uiPhase;
        uint32 m_uiPhaseCounter;
        uint32 m_uiGlobalTimer;
#endif

#if defined (CLASSIC) || defined (TBC)
        void MoveInLineOfSight(Unit* pWho) override
        {
            if (HasEscortState(STATE_ESCORT_ESCORTING) && (pWho->GetEntry() == NPC_QUINN || NPC_RANE))
            {
                lCreatureList.push_back((Creature*)pWho);
            }

            npc_escortAI::MoveInLineOfSight(pWho);
        }

        Creature* GetCreature(uint32 uiCreatureEntry)
        {
            if (!lCreatureList.empty())
            {
                for (std::vector<Creature*>::iterator itr = lCreatureList.begin(); itr != lCreatureList.end(); ++itr)
                {
                    if ((*itr)->GetEntry() == uiCreatureEntry && (*itr)->IsAlive())
                    {
                        return (*itr);
                    }
                }
            }

            return nullptr;
        }
#endif

        void WaypointReached(uint32 i) override
        {
            Player* pPlayer = GetPlayerForEscort();

            if (!pPlayer)
            {
                return;
            }

            switch (i)
            {
            case 0:
                DoScriptText(SAY_START_2, m_creature, pPlayer);
                break;
            case 13:
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
                DoScriptText(SAY_END, m_creature, pPlayer);
#endif
#if defined (CLASSIC) || defined (TBC)
                switch (urand(0, 1))
                {
                case 0:
                    DoScriptText(SAY_END, m_creature, pPlayer);
                    break;
                case 1:
                    DoScriptText(SAY_PROGRESS, m_creature);
                    break;
                }
#endif
                pPlayer->GroupEventHappens(QUEST_ERLAND, m_creature);
#if defined (CLASSIC) || defined (TBC)
                m_creature->SetWalk(false);
#endif
                break;
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            case 14:
                if (Creature* pRane = GetClosestCreatureWithEntry(m_creature, NPC_RANE, 45.0f))
                {
                    DoScriptText(SAY_RANE, pRane, m_creature);
                }
                break;
            case 15:
                DoScriptText(SAY_RANE_REPLY, m_creature);
                break;
#endif
            case 16:
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
                DoScriptText(SAY_CHECK_NEXT, m_creature);
#endif
#if defined (CLASSIC) || defined (TBC)
                m_creature->SetWalk(true);
                SetEscortPaused(true);
#endif
                break;
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            case 24:
                DoScriptText(SAY_QUINN, m_creature);
                break;
#endif
            case 25:
#if defined (CLASSIC) || defined (TBC)
                SetEscortPaused(true);
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
                if (Creature* pQuinn = GetClosestCreatureWithEntry(m_creature, NPC_QUINN, 45.0f))
                {
                    DoScriptText(SAY_QUINN_REPLY, pQuinn, m_creature);
                }
#endif
                break;
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            case 26:
                DoScriptText(SAY_BYE, m_creature);
                break;
#endif
            }
        }

#if defined (CLASSIC) || defined (TBC)
        void UpdateEscortAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                if (HasEscortState(STATE_ESCORT_PAUSED))
                {
                    if (m_uiGlobalTimer < uiDiff)
                    {
                        m_uiGlobalTimer = 5000;

                        switch (m_uiPhase)
                        {
                        case PHASE_RANE:
                            switch (m_uiPhaseCounter)
                            {
                            case 0:
                                if (Creature* pRane = GetCreature(NPC_RANE))
                                {
                                    DoScriptText(SAY_RANE, pRane);
                                }
                                break;
                            case 1:
                                DoScriptText(SAY_RANE_REPLY, m_creature);
                                break;
                            case 2:
                                DoScriptText(SAY_CHECK_NEXT, m_creature);
                                break;
                            case 3:
                                SetEscortPaused(false);
                                m_uiPhase = PHASE_QUINN;
                                break;
                            }
                            break;
                        case PHASE_QUINN:
                            switch (m_uiPhaseCounter)
                            {
                            case 4:
                                DoScriptText(SAY_QUINN, m_creature);
                                break;
                            case 5:
                                if (Creature* pQuinn = GetCreature(NPC_QUINN))
                                {
                                    DoScriptText(SAY_QUINN_REPLY, pQuinn);
                                }
                                break;
                            case 6:
                                DoScriptText(SAY_BYE, m_creature);
                                break;
                            case 7:
                                SetEscortPaused(false);
                                break;
                            }
                            break;
                        }
                        ++m_uiPhaseCounter;
                    }
                    else
                    {
                        m_uiGlobalTimer -= uiDiff;
                    }
                }

                return;
            }

            DoMeleeAttackIfReady();
        }
#endif

        void Reset() override
        {
#if defined (CLASSIC) || defined (TBC)
            lCreatureList.clear();
            m_uiPhase = 0;
            m_uiPhaseCounter = 0;
            m_uiGlobalTimer = 5000;
#endif
        }

        void Aggro(Unit* pWho) override
        {
            switch (urand(0, 2))
            {
            case 0:
                DoScriptText(SAY_AGGRO_1, m_creature, pWho);
                break;
            case 1:
                DoScriptText(SAY_AGGRO_2, m_creature);
                break;
            case 2:
                DoScriptText(SAY_AGGRO_3, m_creature, pWho);
                break;
            }
        }
    };

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_ERLAND)
        {
            DoScriptText(SAY_START_1, pCreature);

            if (npc_deathstalker_erlandAI* pEscortAI = dynamic_cast<npc_deathstalker_erlandAI*>(pCreature->AI()))
            {
                pEscortAI->Start(false, pPlayer, pQuest);
            }
        }
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_deathstalker_erlandAI(pCreature);
    }
};

/*#####
## npc_deathstalker_faerleia
#####*/

enum
{
    QUEST_PYREWOOD_AMBUSH      = 452,

    // cast it after every wave
    SPELL_DRINK_POTION         = 3359,

    SAY_START                  = -1000553,
    SAY_COMPLETED              = -1000554,

    // 1st wave
    NPC_COUNCILMAN_SMITHERS    = 2060,
    // 2nd wave
    NPC_COUNCILMAN_THATHER     = 2061,
    NPC_COUNCILMAN_HENDRICKS   = 2062,
    // 3rd wave
    NPC_COUNCILMAN_WILHELM     = 2063,
    NPC_COUNCILMAN_HARTIN      = 2064,
    NPC_COUNCILMAN_HIGARTH     = 2066,
    // final wave
    NPC_COUNCILMAN_COOPER      = 2065,
    NPC_COUNCILMAN_BRUNSWICK   = 2067,
    NPC_LORD_MAYOR_MORRISON    = 2068
};

struct SpawnPoint
{
    float fX;
    float fY;
    float fZ;
    float fO;
};

SpawnPoint SpawnPoints[] =
{
#if defined (CLASSIC) || defined (TBC)
    { -397.39f, 1509.78f, 18.87f, 4.73f},
    { -396.30f, 1511.68f, 18.87f, 4.76f},
    { -398.26f, 1511.56f, 18.87f, 4.74f}
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    { -397.45f, 1509.56f, 18.87f, 4.73f},
    { -398.35f, 1510.75f, 18.87f, 4.76f},
    { -396.41f, 1511.06f, 18.87f, 4.74f}
#endif
};

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
static float m_afMoveCoords[] = { -410.69f, 1498.04f, 19.77f};
#endif

#if defined (CLASSIC) || defined (TBC)
struct MovePoints
{
    float fX;
    float fY;
    float fZ;
};

MovePoints MovePointspy[] =   // Set Movementpoints for Waves
{
    { -396.97f, 1494.43f, 19.77f},
    { -396.21f, 1495.97f, 19.77f},
    { -398.30f, 1495.97f, 19.77f}
};
#endif
struct npc_deathstalker_faerleia : public CreatureScript
{
    npc_deathstalker_faerleia() : CreatureScript("npc_deathstalker_faerleia") {}

    struct npc_deathstalker_faerleiaAI : public ScriptedAI
    {
        npc_deathstalker_faerleiaAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        void Reset()
        {
        }

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    ObjectGuid m_playerGuid;
#endif
#if defined (CLASSIC) || defined (TBC)
        uint64 m_uiPlayerGUID;
#endif
        uint32 m_uiWaveTimer;
        uint32 m_uiSummonCount;
#if defined (CLASSIC) || defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
        uint32 m_uiRunbackTimer;
#endif
        uint8  m_uiWaveCount;
#if defined (CLASSIC) || defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
        uint8  m_uiMoveCount;
#endif
        bool   m_bEventStarted;
#if defined (CLASSIC) || defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
        bool   m_bWaveDied;
#endif

        void JustRespawned()
        {
            m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER); // Reseting flags on respawn in case questgiver died durin event
            Reset();
        }

#if defined (CLASSIC) || defined (TBC)
        void StartEvent(uint64 uiPlayerGUID)
        {
            m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);

            m_uiPlayerGUID = uiPlayerGUID;
            m_bEventStarted = true;
            m_bWaveDied = false;
            m_uiWaveTimer = 10000;
            m_uiSummonCount = 0;
            m_uiWaveCount = 0;
            m_uiRunbackTimer = 0;
            m_uiMoveCount = 0;
        }
#endif

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    void StartEvent(Player* pPlayer)
    {
        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);

        m_playerGuid  = pPlayer->GetObjectGuid();
        m_bEventStarted  = true;
        m_uiWaveTimer    = 10000;
        m_uiSummonCount  = 0;
        m_uiWaveCount    = 0;
    }
#endif

        void FinishEvent()
        {
#if defined (CLASSIC) || defined (TBC)
            m_uiPlayerGUID = 0;
            m_bEventStarted = false;
            m_bWaveDied = false;
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
        m_playerGuid.Clear();
        m_bEventStarted = false;
        m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
#endif
        }

        void JustDied(Unit* /*pKiller*/)
        {
#if defined (CLASSIC) || defined (TBC)
            if (Player* pPlayer = (m_creature->GetMap()->GetPlayer(m_uiPlayerGUID)))
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
        if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
#endif
            {
                pPlayer->SendQuestFailed(QUEST_PYREWOOD_AMBUSH);
            }

            FinishEvent();
        }

#if defined (CLASSIC) || defined (TBC)
        void JustSummoned(Creature* pSummoned)
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    void JustSummoned(Creature* pSummoned) override
#endif
        {
            ++m_uiSummonCount;

#if defined (CLASSIC) || defined (TBC)
            // Get waypoint for each creature
            pSummoned->GetMotionMaster()->MovePoint(0, MovePointspy[m_uiMoveCount].fX, MovePointspy[m_uiMoveCount].fY, MovePointspy[m_uiMoveCount].fZ);

            ++m_uiMoveCount;
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
        // put them on correct waypoints later on
        float fX, fY, fZ;
        pSummoned->GetRandomPoint(m_afMoveCoords[0], m_afMoveCoords[1], m_afMoveCoords[2], 10.0f, fX, fY, fZ);
        pSummoned->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
#endif
        }

#if defined (CLASSIC) || defined (TBC)
        void SummonedCreatureJustDied(Creature* /*pKilled*/)
        {
            --m_uiSummonCount;

            if (!m_uiSummonCount)
            {
                m_uiRunbackTimer = 3000;  //without timer creature is in evade state running to start point and do not drink potion
                m_bWaveDied = true;
            }
        }
#endif

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    void SummonedCreatureJustDied(Creature* /*pKilled*/) override
    {
        --m_uiSummonCount;

        if (!m_uiSummonCount)
        {
            DoCastSpellIfCan(m_creature, SPELL_DRINK_POTION);

            // final wave
            if (m_uiWaveCount == 4)
            {
                DoScriptText(SAY_COMPLETED, m_creature);

                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
                {
                    pPlayer->GroupEventHappens(QUEST_PYREWOOD_AMBUSH, m_creature);
                }

                FinishEvent();
            }

        }
    }
#endif

#if defined (CLASSIC) || defined (TBC)
        void UpdateAI(const uint32 uiDiff)
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    void UpdateAI(const uint32 uiDiff) override
#endif
        {
#if defined (CLASSIC) || defined (TBC)
            if (m_bEventStarted && m_bWaveDied && m_uiRunbackTimer < uiDiff && m_uiWaveCount == 4)
            {
                DoScriptText(SAY_COMPLETED, m_creature);
                m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                if (Player* pPlayer = (m_creature->GetMap()->GetPlayer(m_uiPlayerGUID)))
                {
                    pPlayer->GroupEventHappens(QUEST_PYREWOOD_AMBUSH, m_creature);
                }

                FinishEvent();
            }


            if (m_bEventStarted && m_bWaveDied && m_uiRunbackTimer < uiDiff && m_uiWaveCount <= 3)
            {
                DoCastSpellIfCan(m_creature, SPELL_DRINK_POTION);
                m_bWaveDied = false;
            }
#endif

            if (m_bEventStarted && !m_uiSummonCount)
            {
                if (m_uiWaveTimer < uiDiff)
                {
                    switch (m_uiWaveCount)
                    {
                    case 0:
#if defined (CLASSIC) || defined (TBC)
                        m_creature->SummonCreature(NPC_COUNCILMAN_SMITHERS, SpawnPoints[0].fX, SpawnPoints[0].fY, SpawnPoints[0].fZ, SpawnPoints[0].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
                        m_creature->SummonCreature(NPC_COUNCILMAN_SMITHERS,  SpawnPoints[1].fX, SpawnPoints[1].fY, SpawnPoints[1].fZ, SpawnPoints[1].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
#endif
                        m_uiWaveTimer = 10000;
#if defined (CLASSIC) || defined (TBC)
                        m_uiMoveCount = 0;
#endif
                        break;
                    case 1:
                        m_creature->SummonCreature(NPC_COUNCILMAN_THATHER, SpawnPoints[2].fX, SpawnPoints[2].fY, SpawnPoints[2].fZ, SpawnPoints[2].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
                        m_creature->SummonCreature(NPC_COUNCILMAN_HENDRICKS, SpawnPoints[1].fX, SpawnPoints[1].fY, SpawnPoints[1].fZ, SpawnPoints[1].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
                        m_uiWaveTimer = 10000;
#if defined (CLASSIC) || defined (TBC)
                        m_uiMoveCount = 0;
#endif
                        break;
                    case 2:
                        m_creature->SummonCreature(NPC_COUNCILMAN_HARTIN, SpawnPoints[0].fX, SpawnPoints[0].fY, SpawnPoints[0].fZ, SpawnPoints[0].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
                        m_creature->SummonCreature(NPC_COUNCILMAN_WILHELM, SpawnPoints[1].fX, SpawnPoints[1].fY, SpawnPoints[1].fZ, SpawnPoints[1].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
                        m_creature->SummonCreature(NPC_COUNCILMAN_HIGARTH, SpawnPoints[2].fX, SpawnPoints[2].fY, SpawnPoints[2].fZ, SpawnPoints[2].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
                        m_uiWaveTimer = 8000;
#if defined (CLASSIC) || defined (TBC)
                        m_uiMoveCount = 0;
#endif
                        break;
                    case 3:
                        m_creature->SummonCreature(NPC_LORD_MAYOR_MORRISON, SpawnPoints[0].fX, SpawnPoints[0].fY, SpawnPoints[0].fZ, SpawnPoints[0].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
                        m_creature->SummonCreature(NPC_COUNCILMAN_COOPER, SpawnPoints[1].fX, SpawnPoints[1].fY, SpawnPoints[1].fZ, SpawnPoints[1].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
                        m_creature->SummonCreature(NPC_COUNCILMAN_BRUNSWICK, SpawnPoints[2].fX, SpawnPoints[2].fY, SpawnPoints[2].fZ, SpawnPoints[2].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 20000);
#if defined (CLASSIC) || defined (TBC)
                        m_uiMoveCount = 0;
#endif
                        break;
#if defined (CLASSIC) || defined (TBC)
                    case 4:
                        m_uiRunbackTimer -= uiDiff;
                        return;
#endif
                    }

                    ++m_uiWaveCount;
                }
                else
                {
                    m_uiWaveTimer -= uiDiff;
                }
#if defined (CLASSIC) || defined (TBC)
                m_uiRunbackTimer -= uiDiff;
#endif
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_PYREWOOD_AMBUSH)
        {
            DoScriptText(SAY_START, pCreature, pPlayer);

            if (npc_deathstalker_faerleiaAI* pFaerleiaAI = dynamic_cast<npc_deathstalker_faerleiaAI*>(pCreature->AI()))
            {
#if defined (CLASSIC) || defined (TBC)
                pFaerleiaAI->StartEvent(pPlayer->GetObjectGuid().GetRawValue());
                return true;
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            pFaerleiaAI->StartEvent(pPlayer);
#endif
            }
        }
        return false;
    }

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_deathstalker_faerleiaAI(pCreature);
    }
};

void AddSC_silverpine_forest()
{
    Script* s;
    s = new npc_deathstalker_erland();
    s->RegisterSelf();
    s = new npc_deathstalker_faerleia();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_deathstalker_erland";
    //pNewScript->GetAI = &GetAI_npc_deathstalker_erland;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_deathstalker_erland;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_deathstalker_faerleia";
    //pNewScript->GetAI = &GetAI_npc_deathstalker_faerleia;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_deathstalker_faerleia;
    //pNewScript->RegisterSelf();
}
