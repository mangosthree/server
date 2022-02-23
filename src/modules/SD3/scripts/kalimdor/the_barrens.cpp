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
 * SDName:      The_Barrens
 * SD%Complete: 90
 * SDComment:   Quest support: 863, 898, 1719, 2458, 4021, 4921.
 * SDCategory:  Barrens
 * EndScriptData
 */

/**
 * ContentData
 * npc_beaten_corpse
 * npc_gilthares
 * npc_taskmaster_fizzule
 * npc_twiggy_flathead
 * at_twiggy_flathead
 * npc_wizzlecrank_shredder
 * npc_regthar_deathgate
 * npc_horde_defender
 * EndContentData
 */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_beaten_corpse
######*/

enum
{
    QUEST_LOST_IN_BATTLE    = 4921
};

struct npc_beaten_corpse : public CreatureScript
{
    npc_beaten_corpse() : CreatureScript("npc_beaten_corpse") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (pPlayer->GetQuestStatus(QUEST_LOST_IN_BATTLE) == QUEST_STATUS_INCOMPLETE ||
            pPlayer->GetQuestStatus(QUEST_LOST_IN_BATTLE) == QUEST_STATUS_COMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Examine corpse in detail...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        pPlayer->SEND_GOSSIP_MENU(3557, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            pPlayer->SEND_GOSSIP_MENU(3558, pCreature->GetObjectGuid());
            pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetObjectGuid());
        }
        return true;
    }
};

/*######
# npc_gilthares
######*/

enum
{
    SAY_GIL_START               = -1000370,
    SAY_GIL_AT_LAST             = -1000371,
    SAY_GIL_PROCEED             = -1000372,
    SAY_GIL_FREEBOOTERS         = -1000373,
    SAY_GIL_AGGRO_1             = -1000374,
    SAY_GIL_AGGRO_2             = -1000375,
    SAY_GIL_AGGRO_3             = -1000376,
    SAY_GIL_AGGRO_4             = -1000377,
    SAY_GIL_ALMOST              = -1000378,
    SAY_GIL_SWEET               = -1000379,
    SAY_GIL_FREED               = -1000380,

    QUEST_FREE_FROM_HOLD        = 898,
    AREA_MERCHANT_COAST         = 391
};

struct npc_gilthares : public CreatureScript
{
    npc_gilthares() : CreatureScript("npc_gilthares") {}

    struct npc_giltharesAI : public npc_escortAI
    {
        npc_giltharesAI(Creature* pCreature) : npc_escortAI(pCreature) { }

        void Reset() override { }

        void WaypointReached(uint32 uiPointId) override
        {
            Player* pPlayer = GetPlayerForEscort();

            if (!pPlayer)
            {
                return;
            }

            switch (uiPointId)
            {
            case 16:
                DoScriptText(SAY_GIL_AT_LAST, m_creature, pPlayer);
                break;
            case 17:
                DoScriptText(SAY_GIL_PROCEED, m_creature, pPlayer);
                break;
            case 18:
                DoScriptText(SAY_GIL_FREEBOOTERS, m_creature, pPlayer);
                break;
            case 37:
                DoScriptText(SAY_GIL_ALMOST, m_creature, pPlayer);
                break;
            case 47:
                DoScriptText(SAY_GIL_SWEET, m_creature, pPlayer);
                break;
            case 53:
                DoScriptText(SAY_GIL_FREED, m_creature, pPlayer);
                pPlayer->GroupEventHappens(QUEST_FREE_FROM_HOLD, m_creature);
                break;
            }
        }

        void Aggro(Unit* pWho) override
        {
            // not always use
            if (urand(0, 3))
            {
                return;
            }

            // only aggro text if not player and only in this area
            if (pWho->GetTypeId() != TYPEID_PLAYER && m_creature->GetAreaId() == AREA_MERCHANT_COAST)
            {
                // appears to be pretty much random (possible only if escorter not in combat with pWho yet?)
                switch (urand(0, 3))
                {
                case 0:
                    DoScriptText(SAY_GIL_AGGRO_1, m_creature, pWho);
                    break;
                case 1:
                    DoScriptText(SAY_GIL_AGGRO_2, m_creature, pWho);
                    break;
                case 2:
                    DoScriptText(SAY_GIL_AGGRO_3, m_creature, pWho);
                    break;
                case 3:
                    DoScriptText(SAY_GIL_AGGRO_4, m_creature, pWho);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_giltharesAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_FREE_FROM_HOLD)
        {
            pCreature->SetFactionTemporary(FACTION_ESCORT_H_NEUTRAL_ACTIVE, TEMPFACTION_RESTORE_RESPAWN);
            pCreature->SetStandState(UNIT_STAND_STATE_STAND);

            DoScriptText(SAY_GIL_START, pCreature, pPlayer);

            if (npc_giltharesAI* pEscortAI = dynamic_cast<npc_giltharesAI*>(pCreature->AI()))
            {
                pEscortAI->Start(false, pPlayer, pQuest);
            }
        }
        return true;
    }
};

/*######
## npc_taskmaster_fizzule
######*/

enum
{
    FACTION_FRIENDLY_F  = 35,
    SPELL_FLARE         = 10113,
    SPELL_FOLLY         = 10137,
};

struct npc_taskmaster_fizzule : public CreatureScript
{
    npc_taskmaster_fizzule() : CreatureScript("npc_taskmaster_fizzule") {}

    struct npc_taskmaster_fizzuleAI : public ScriptedAI
    {
        npc_taskmaster_fizzuleAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiResetTimer;
        uint8 m_uiFlareCount;

        void Reset() override
        {
            m_uiResetTimer = 0;
            m_uiFlareCount = 0;
        }

        void EnterEvadeMode() override
        {
            if (m_uiResetTimer)
            {
                m_creature->RemoveAllAurasOnEvade();
                m_creature->DeleteThreatList();
                m_creature->CombatStop(true);
                m_creature->LoadCreatureAddon(true);

                m_creature->SetLootRecipient(nullptr);

                m_creature->SetFactionTemporary(FACTION_FRIENDLY_F, TEMPFACTION_RESTORE_REACH_HOME);
                m_creature->HandleEmote(EMOTE_ONESHOT_SALUTE);
            }
            else
            {
                ScriptedAI::EnterEvadeMode();
            }
        }

        void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
        {
            if (pCaster->GetTypeId() == TYPEID_PLAYER && (pSpell->Id == SPELL_FLARE || pSpell->Id == SPELL_FOLLY))
            {
                ++m_uiFlareCount;

                if (m_uiFlareCount >= 2 && m_creature->getFaction() != FACTION_FRIENDLY_F)
                {
                    m_uiResetTimer = 120000;
                }
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiResetTimer)
            {
                if (m_uiResetTimer <= uiDiff)
                {
                    m_uiResetTimer = 0;
                    EnterEvadeMode();
                }
                else
                {
                    m_uiResetTimer -= uiDiff;
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }

        void ReceiveEmote(Player* /*pPlayer*/, uint32 uiTextEmote) override
        {
            if (uiTextEmote == TEXTEMOTE_SALUTE)
            {
                if (m_uiFlareCount >= 2 && m_creature->getFaction() != FACTION_FRIENDLY_F)
                {
                    EnterEvadeMode();
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_taskmaster_fizzuleAI(pCreature);
    }
};

/*#####
## npc_twiggy_flathead
#####*/

enum
{
    SAY_BIG_WILL_READY                  = -1000123,
    SAY_TWIGGY_BEGIN                    = -1000124,
    SAY_TWIGGY_FRAY                     = -1000125,
    SAY_TWIGGY_DOWN                     = -1000126,
    SAY_TWIGGY_OVER                     = -1000127,

    NPC_TWIGGY                          = 6248,
    NPC_BIG_WILL                        = 6238,
    NPC_AFFRAY_CHALLENGER               = 6240,

    QUEST_AFFRAY                        = 1719,

    FACTION_FRIENDLY                    = 35,
    FACTION_HOSTILE_WILL                = 32,
    FACTION_HOSTILE_CHALLENGER          = 14,

    MAX_CHALLENGERS                     = 6,
};

static const float aAffrayChallengerLoc[8][4] =
{
    { -1683.0f, -4326.0f, 2.79f, 0.00f},
    { -1682.0f, -4329.0f, 2.79f, 0.00f},
    { -1683.0f, -4330.0f, 2.79f, 0.00f},
    { -1680.0f, -4334.0f, 2.79f, 1.49f},
    { -1674.0f, -4326.0f, 2.79f, 3.49f},
    { -1677.0f, -4334.0f, 2.79f, 1.66f},
    { -1713.79f, -4342.09f, 6.05f, 6.15f},          // Big Will spawn loc
    { -1682.31f, -4329.68f, 2.78f, 0.0f},           // Big Will move loc
};

struct npc_twiggy_flathead : public CreatureScript
{
    npc_twiggy_flathead() : CreatureScript("npc_twiggy_flathead") {}

    struct npc_twiggy_flatheadAI : public ScriptedAI
    {
        npc_twiggy_flatheadAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        bool m_bIsEventInProgress;

        uint32 m_uiEventTimer;
        uint32 m_uiChallengerCount;
        uint8 m_uiStep;

        ObjectGuid m_playerGuid;
        ObjectGuid m_bigWillGuid;
        GuidVector m_vAffrayChallengerGuidsVector;

        void Reset() override
        {
            m_bIsEventInProgress = false;

            m_uiEventTimer = 1000;
            m_uiChallengerCount = 0;
            m_uiStep = 0;

            m_playerGuid.Clear();
            m_bigWillGuid.Clear();
            m_vAffrayChallengerGuidsVector.clear();
        }

        bool CanStartEvent(Player* pPlayer)
        {
            if (!m_bIsEventInProgress)
            {
                DoScriptText(SAY_TWIGGY_BEGIN, m_creature, pPlayer);
                m_playerGuid = pPlayer->GetObjectGuid();
                m_bIsEventInProgress = true;

                return true;
            }

            debug_log("SD3: npc_twiggy_flathead event already in progress, need to wait.");
            return false;
        }

        void SetChallengers()
        {
            m_vAffrayChallengerGuidsVector.reserve(MAX_CHALLENGERS);

            for (uint8 i = 0; i < MAX_CHALLENGERS; ++i)
            {
                m_creature->SummonCreature(NPC_AFFRAY_CHALLENGER, aAffrayChallengerLoc[i][0], aAffrayChallengerLoc[i][1], aAffrayChallengerLoc[i][2], aAffrayChallengerLoc[i][3], TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, 600000);
            }
        }

        void SetChallengerReady(Creature* pChallenger)
        {
            pChallenger->setFaction(FACTION_HOSTILE_CHALLENGER);
            pChallenger->HandleEmote(EMOTE_ONESHOT_ROAR);
            ++m_uiChallengerCount;

            if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
            {
                pChallenger->AI()->AttackStart(pPlayer);
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_BIG_WILL)
            {
                m_bigWillGuid = pSummoned->GetObjectGuid();
                pSummoned->setFaction(FACTION_FRIENDLY);
                pSummoned->SetWalk(false);
                pSummoned->GetMotionMaster()->MovePoint(1, aAffrayChallengerLoc[7][0], aAffrayChallengerLoc[7][1], aAffrayChallengerLoc[7][2]);
            }
            else
            {
                pSummoned->setFaction(FACTION_FRIENDLY);
                pSummoned->HandleEmote(EMOTE_ONESHOT_ROAR);
                m_vAffrayChallengerGuidsVector.push_back(pSummoned->GetObjectGuid());
            }
        }

        void SummonedMovementInform(Creature* pSummoned, uint32 uiMoveType, uint32 uiPointId) override
        {
            if (uiMoveType != POINT_MOTION_TYPE || !uiPointId || pSummoned->GetEntry() != NPC_BIG_WILL)
            {
                return;
            }

            pSummoned->setFaction(FACTION_HOSTILE_WILL);

            if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
            {
                DoScriptText(SAY_BIG_WILL_READY, pSummoned, pPlayer);
                pSummoned->SetFacingToObject(pPlayer);
            }
        }

        void SummonedCreatureJustDied(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_BIG_WILL)
            {
                DoScriptText(SAY_TWIGGY_OVER, m_creature);
                EnterEvadeMode();
            }
            else
            {
                DoScriptText(SAY_TWIGGY_DOWN, m_creature);
                m_uiEventTimer = 15000;
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* pSender, Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            if (eventType == AI_EVENT_CUSTOM_A && pSender == m_creature && pInvoker->GetTypeId() == TYPEID_PLAYER)
            {
                CanStartEvent(pInvoker->ToPlayer());
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_bIsEventInProgress)
            {
                return;
            }

            if (m_uiEventTimer < uiDiff)
            {
                Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid);

                if (!pPlayer || !pPlayer->IsAlive())
                {
                    EnterEvadeMode();
                }

                switch (m_uiStep)
                {
                case 0:
                    SetChallengers();
                    m_uiEventTimer = 5000;
                    ++m_uiStep;
                    break;
                case 1:
                    DoScriptText(SAY_TWIGGY_FRAY, m_creature);
                    if (Creature* pChallenger = m_creature->GetMap()->GetCreature(m_vAffrayChallengerGuidsVector[m_uiChallengerCount]))
                    {
                        SetChallengerReady(pChallenger);
                    }
                    else
                    {
                        EnterEvadeMode();
                    }

                    if (m_uiChallengerCount == MAX_CHALLENGERS)
                    {
                        ++m_uiStep;
                        m_uiEventTimer = 5000;
                    }
                    else
                    {
                        m_uiEventTimer = 25000;
                    }
                    break;
                case 2:
                    m_creature->SummonCreature(NPC_BIG_WILL, aAffrayChallengerLoc[6][0], aAffrayChallengerLoc[6][1], aAffrayChallengerLoc[6][2], aAffrayChallengerLoc[6][3], TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, 300000);
                    m_uiEventTimer = 15000;
                    ++m_uiStep;
                    break;
                default:
                    m_uiEventTimer = 5000;
                    break;
                }
            }
            else
            {
                m_uiEventTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_twiggy_flatheadAI(pCreature);
    }
};

struct at_twiggy_flathead : public AreaTriggerScript
{
    at_twiggy_flathead() : AreaTriggerScript("at_twiggy_flathead") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* /*pAt*/) override
    {
        if (pPlayer->IsAlive() && !pPlayer->isGameMaster() && pPlayer->GetQuestStatus(QUEST_AFFRAY) == QUEST_STATUS_INCOMPLETE)
        {
            Creature* pCreature = GetClosestCreatureWithEntry(pPlayer, NPC_TWIGGY, 30.0f);
            if (!pCreature)
            {
                return true;
            }

            if (CreatureAI* pTwiggyAI = pCreature->AI())
            {
                pTwiggyAI->SendAIEvent(AI_EVENT_CUSTOM_A, pPlayer, pCreature);
            }

            return false;   //this will let the trigger pop up several times until the quest is complete, but who cares
        }
        return true;
    }
};

/*#####
## npc_wizzlecranks_shredder
#####*/

enum
{
    SAY_START           = -1000298,
    SAY_STARTUP1        = -1000299,
    SAY_STARTUP2        = -1000300,
    SAY_MERCENARY       = -1000301,
    SAY_PROGRESS_1      = -1000302,
    SAY_PROGRESS_2      = -1000303,
    SAY_PROGRESS_3      = -1000304,
    SAY_END             = -1000305,

    QUEST_ESCAPE        = 863,
    FACTION_RATCHET     = 637,
    NPC_PILOT_WIZZ      = 3451,
    NPC_MERCENARY       = 3282
};

struct npc_wizzlecranks_shredder : public CreatureScript
{
    npc_wizzlecranks_shredder() : CreatureScript("npc_wizzlecranks_shredder") {}

    struct npc_wizzlecranks_shredderAI : public npc_escortAI
    {
        npc_wizzlecranks_shredderAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
            m_bIsPostEvent = false; //TODO check this
            m_uiPostEventTimer = 1000;
            m_uiPostEventCount = 0;
        }

        bool m_bIsPostEvent;
        uint32 m_uiPostEventTimer;
        uint32 m_uiPostEventCount;

        void Reset() override
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                if (m_creature->getStandState() == UNIT_STAND_STATE_DEAD)
                {
                    m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                }

                m_bIsPostEvent = false;
                m_uiPostEventTimer = 1000;
                m_uiPostEventCount = 0;
            }
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 0:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_STARTUP1, m_creature, pPlayer);
                }
                break;
            case 9:
                SetRun(false);
                break;
            case 17:
                if (Creature* pTemp = m_creature->SummonCreature(NPC_MERCENARY, 1128.489f, -3037.611f, 92.701f, 1.472f, TEMPSPAWN_TIMED_OOC_DESPAWN, 120000))
                {
                    DoScriptText(SAY_MERCENARY, pTemp);
                    m_creature->SummonCreature(NPC_MERCENARY, 1160.172f, -2980.168f, 97.313f, 3.690f, TEMPSPAWN_TIMED_OOC_DESPAWN, 120000);
                }
                break;
            case 24:
                m_bIsPostEvent = true;
                break;
            }
        }

        void WaypointStart(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 9:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_STARTUP2, m_creature, pPlayer);
                }
                break;
            case 18:
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    DoScriptText(SAY_PROGRESS_1, m_creature, pPlayer);
                }
                SetRun();
                break;
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_PILOT_WIZZ)
            {
                m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
            }

            if (pSummoned->GetEntry() == NPC_MERCENARY)
            {
                pSummoned->AI()->AttackStart(m_creature);
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                if (m_bIsPostEvent)
                {
                    if (m_uiPostEventTimer < uiDiff)
                    {
                        switch (m_uiPostEventCount)
                        {
                        case 0:
                            DoScriptText(SAY_PROGRESS_2, m_creature);
                            break;
                        case 1:
                            DoScriptText(SAY_PROGRESS_3, m_creature);
                            break;
                        case 2:
                            DoScriptText(SAY_END, m_creature);
                            break;
                        case 3:
                            if (Player* pPlayer = GetPlayerForEscort())
                            {
                                pPlayer->GroupEventHappens(QUEST_ESCAPE, m_creature);
                                m_creature->SummonCreature(NPC_PILOT_WIZZ, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSPAWN_TIMED_DESPAWN, 180000);
                            }
                            break;
                        }

                        ++m_uiPostEventCount;
                        m_uiPostEventTimer = 5000;
                    }
                    else
                    {
                        m_uiPostEventTimer -= uiDiff;
                    }
                }

                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_ESCAPE)
        {
            DoScriptText(SAY_START, pCreature);
            pCreature->SetFactionTemporary(FACTION_RATCHET, TEMPFACTION_RESTORE_RESPAWN);

            if (npc_wizzlecranks_shredderAI* pEscortAI = dynamic_cast<npc_wizzlecranks_shredderAI*>(pCreature->AI()))
            {
                pEscortAI->Start(true, pPlayer, pQuest);
            }
        }
        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_wizzlecranks_shredderAI(pCreature);
    }
};

/*#####
## npc_regthar_deathgate
#####*/

enum
{
    QUEST_COUNTERATTACK        = 4021,

    SAY_START_REGTHAR          = -1000891,
    SAY_DEFENDER               = -1000892,
    YELL_RETREAT               = -1000893,
    YELL_STRONGEST             = -1000894,

    NPC_REGTHAR_DEATHGATE      = 3389,
    NPC_WARLORD_KROMZAR        = 9456,
    NPC_HORDE_DEFENDER         = 9457,
    NPC_HORDE_AXE_THROWER      = 9458,
    NPC_KOLKAR_INVADER         = 9524,
    NPC_KOLKAR_STORMSEER       = 9523

};

struct SpawnPoint
{
    float fX;
    float fY;
    float fZ;
    float fO;
};

SpawnPoint SpawnPointsHorde[] =
{
    { -307.23f, -1919.84f, 91.66f, 0.64f},
    { -295.84f, -1913.58f, 91.66f, 1.86f},
    { -281.15f, -1906.39f, 91.66f, 1.88f},
    { -270.09f, -1901.58f, 91.66f, 1.88f},
    { -226.65f, -1927.87f, 93.24f, 0.41f},
    { -221.61f, -1936.54f, 94.00f, 0.41f},
    { -275.24f, -1901.96f, 91.66f, 1.90f},
};

SpawnPoint SpawnPointsKromzar[] =
{
    { -281.19f, -1855.54f, 92.58f, 4.85f},
    { -283.66f, -1858.45f, 92.47f, 4.85f},
    { -286.50f, -1856.18f, 92.44f, 4.85f}
};

SpawnPoint SpawnPointsKolkar[] =
{
    { -290.26f, -1860.85f, 92.48f, 3.68f},
    { -311.46f, -1871.16f, 92.64f, 6.06f},
    { -321.75f, -1868.48f, 93.73f, 0.30f},
    { -338.12f, -1852.10f, 94.09f, 0.01f},
    { -311.46f, -1847.78f, 94.93f, 6.15f},
    { -292.02f, -1840.00f, 93.06f, 3.51f},
    { -267.14f, -1853.97f, 93.24f, 3.52f},
    { -267.68f, -1832.77f, 92.69f, 3.52f},
    { -279.70f, -1845.67f, 92.81f, 3.83f},
    { -338.99f, -1868.68f, 93.50f, 0.17f},
    { -210.28f, -1916.22f, 92.87f, 4.89f},
    { -209.09f, -1922.44f, 93.26f, 3.87f},
    { -206.64f, -1924.83f, 93.78f, 2.41f},
    { -283.57f, -1883.91f, 92.60f, 3.53f},
    { -197.50f, -1929.31f, 94.03f, 3.51f}
};

struct npc_regthar_deathgate : public CreatureScript
{
    npc_regthar_deathgate() : CreatureScript("npc_regthar_deathgate") {}

    struct npc_regthar_deathgateAI : public ScriptedAI
    {
        npc_regthar_deathgateAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        std::list<Creature*> lCreatureList;
        std::list<uint32> lSpawnList;
        std::list<Creature*>::iterator itr;

        std::list<Creature*> lCreatureListHorde;
        std::list<uint32> lSpawnListHorde;
        std::list<Creature*>::iterator itrh;

        uint64 m_uiPlayerGUID;
        uint64 m_uiEventTimer;
        uint32 m_uiSummonCountInvader;
        uint32 m_uiSummonCountStormseer;
        uint32 m_uiSummonCountHorde;
        uint32 m_uiWaitSummonTimer;
        uint32 m_uiWaitSummonTimerHorde;
        uint32 m_uiSpawnPosition;
        uint32 m_uiKillCount;
        uint32 m_uiCreatureCount;
        uint32 m_uiPhaseCount;
        bool   m_bEventStarted;

        void Reset() {}

        void JustRespawned()
        {
            FinishEvent();
            Reset();
        }

        void StartEvent(uint64 uiPlayerGUID)
        {
            if (!m_bEventStarted)
            {
                m_bEventStarted = true;
                m_uiPlayerGUID = uiPlayerGUID;
                m_uiEventTimer = 1200000;
                m_uiSummonCountInvader = 0;
                m_uiSummonCountStormseer = 0;
                m_uiSummonCountHorde = 0;
                m_uiWaitSummonTimer = 0;
                m_uiWaitSummonTimerHorde = 0;
                m_uiSpawnPosition = 0;
                m_uiKillCount = 0;
                m_uiCreatureCount = 0;
                m_uiPhaseCount = 1;
                lCreatureList.clear();
                lSpawnList.clear();
                lCreatureListHorde.clear();
                lSpawnListHorde.clear();
            }
        }

        void FinishEvent()
        {
            m_uiPlayerGUID = 0;
            m_bEventStarted = false;
            m_uiKillCount = 0;
        }

        void JustSummoned(Creature* pSummoned)
        {
            SendAIEventAround(AI_EVENT_CUSTOM_A, pSummoned->ToUnit(), 0, 40.0f);

            if (pSummoned->GetEntry() == NPC_HORDE_DEFENDER) //replace died creature from list with new spawned one
            {
                if (m_uiPhaseCount == 2)
                {
                    itrh = lCreatureListHorde.begin();
                    advance(itrh, lSpawnListHorde.front());
                    *itrh = pSummoned;
                    lSpawnListHorde.pop_front(); //Drop spawned creature from spawn list
                }

                if (m_uiPhaseCount == 1)
                {
                    lCreatureListHorde.push_back(pSummoned);
                }
            }

            if (pSummoned->GetEntry() == NPC_HORDE_AXE_THROWER) //replace died creature from list with new spawned one
            {
                if (m_uiPhaseCount == 2)
                {
                    itrh = lCreatureListHorde.begin();
                    advance(itrh, lSpawnListHorde.front());
                    *itrh = pSummoned;
                    lSpawnListHorde.pop_front(); //Drop spawned creature from spawn list
                }

                if (m_uiPhaseCount == 1)
                {
                    lCreatureListHorde.push_back(pSummoned);
                }
            }

            if (pSummoned->GetEntry() == NPC_KOLKAR_INVADER) //replace died creature from list with new spawned one
            {
                if (m_uiPhaseCount == 2)
                {
                    itr = lCreatureList.begin();
                    advance(itr, lSpawnList.front());
                    *itr = pSummoned;
                    lSpawnList.pop_front(); //Drop spawned creature from spawn list
                }

                if (m_uiPhaseCount == 1)
                {
                    lCreatureList.push_back(pSummoned);
                }
            }

            if (pSummoned->GetEntry() == NPC_KOLKAR_STORMSEER) //Insert each spawn into list until 10 spawned
            {
                if (m_uiPhaseCount == 2)
                {
                    itr = lCreatureList.begin();
                    advance(itr, lSpawnList.front());
                    *itr = pSummoned;
                    lSpawnList.pop_front(); //Drop spawned creature from spawn list
                }

                if (m_uiPhaseCount == 1)
                {
                    lCreatureList.push_back(pSummoned);
                }
            }
            if (pSummoned->GetEntry() == NPC_WARLORD_KROMZAR)
            {
                DoScriptText(YELL_STRONGEST, m_creature);
            }
        }

        void SummonedCreatureJustDied(Creature* pKilled)
        {
            if (pKilled->GetEntry() == NPC_HORDE_DEFENDER) //find spawnpoint of died creature spawnpoint = position in creature list
            {
                --m_uiSummonCountHorde;
                m_uiWaitSummonTimerHorde = 12000;

                m_uiSpawnPosition = 0;
                for (std::list<Creature*>::iterator itrh = lCreatureListHorde.begin(); itrh != lCreatureListHorde.end(); ++itrh, ++m_uiSpawnPosition)
                {
                    if ((*itrh) == pKilled)
                    {
                        lSpawnListHorde.push_back(m_uiSpawnPosition);  //put spawnpoint in list
                        (*itrh) = nullptr;
                    }
                }
                if (m_uiWaitSummonTimerHorde < 1000)
                {
                    m_uiWaitSummonTimerHorde = 1000;
                }

                if (urand(0, 1))
                {
                    DoScriptText(SAY_DEFENDER, m_creature);
                }
            }

            if (pKilled->GetEntry() == NPC_HORDE_AXE_THROWER) //find spawnpoint of died creature spawnpoint = position in creature list
            {
                --m_uiSummonCountHorde;
                m_uiSpawnPosition = 0;
                for (std::list<Creature*>::iterator itrh = lCreatureListHorde.begin(); itrh != lCreatureListHorde.end(); ++itrh, ++m_uiSpawnPosition)
                {
                    if ((*itrh) == pKilled)
                    {
                        lSpawnListHorde.push_back(m_uiSpawnPosition);  //put spawnpoint in list
                        (*itrh) = nullptr;
                    }
                }
                if (m_uiWaitSummonTimerHorde < 1000)
                {
                    m_uiWaitSummonTimerHorde = 1000;
                }

                if (urand(0, 1))
                {
                    DoScriptText(SAY_DEFENDER, m_creature);
                }
            }

            if (pKilled->GetEntry() == NPC_WARLORD_KROMZAR)
            {
                DoScriptText(YELL_RETREAT, m_creature);
                m_uiPhaseCount = 4;
            }

            if (pKilled->GetEntry() == NPC_KOLKAR_INVADER) //find spawnpoint of died creature, spawnpoint = position in creature list
            {
                --m_uiSummonCountInvader;
                ++m_uiKillCount;
                m_uiWaitSummonTimer = 10000;

                m_uiSpawnPosition = 0;
                for (std::list<Creature*>::iterator itr = lCreatureList.begin(); itr != lCreatureList.end(); ++itr, ++m_uiSpawnPosition)
                {
                    if ((*itr) == pKilled)
                    {
                        lSpawnList.push_back(m_uiSpawnPosition);  //put spawnpoint in list
                        (*itr) = nullptr;
                    }
                }
            }

            if (pKilled->GetEntry() == NPC_KOLKAR_STORMSEER) //find spawnpoint of died creature, spawnpoint = position in creature list
            {
                --m_uiSummonCountStormseer;
                ++m_uiKillCount;
                m_uiWaitSummonTimer = 10000;

                m_uiSpawnPosition = 0;
                for (std::list<Creature*>::iterator itr = lCreatureList.begin(); itr != lCreatureList.end(); ++itr, ++m_uiSpawnPosition)
                {
                    if ((*itr) == pKilled)
                    {
                        lSpawnList.push_back(m_uiSpawnPosition);  //put spawnpoint in list
                        (*itr) = nullptr;
                    }
                }
            }
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (m_uiPhaseCount <= 2 && m_uiEventTimer < uiDiff)
            {
                m_uiPhaseCount = 4;
            }

            if (m_uiPhaseCount == 4)
            {
                for (std::list<Creature*>::iterator itrh = lCreatureListHorde.begin(); itrh != lCreatureListHorde.end(); ++itrh)
                {
                    if ((*itrh) != nullptr)
                    {
                        (*itrh)->ForcedDespawn();
                    }
                }
                m_uiPhaseCount = 5;
            }

            if (m_uiPhaseCount == 5)
            {
                for (std::list<Creature*>::iterator itr = lCreatureList.begin(); itr != lCreatureList.end(); ++itr)
                {
                    if ((*itr) != nullptr)
                    {
                        (*itr)->ForcedDespawn();
                    }
                }
                m_uiPhaseCount = 6;
                FinishEvent();
            }
            if (m_uiPhaseCount == 2 && m_uiKillCount >= 20)
            {
                m_uiPhaseCount = 3;
                m_creature->SummonCreature(NPC_KOLKAR_INVADER, SpawnPointsKromzar[0].fX, SpawnPointsKromzar[0].fY, SpawnPointsKromzar[0].fZ, SpawnPointsKromzar[0].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                m_creature->SummonCreature(NPC_WARLORD_KROMZAR, SpawnPointsKromzar[1].fX, SpawnPointsKromzar[1].fY, SpawnPointsKromzar[1].fZ, SpawnPointsKromzar[1].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                m_creature->SummonCreature(NPC_KOLKAR_STORMSEER, SpawnPointsKromzar[2].fX, SpawnPointsKromzar[2].fY, SpawnPointsKromzar[2].fZ, SpawnPointsKromzar[2].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
            }

            if (m_uiPhaseCount <= 2 && m_uiWaitSummonTimerHorde < uiDiff && m_uiSummonCountHorde < 7)
            {
                if (m_uiPhaseCount == 2)
                    switch (urand(0, 1))
                {
                    case 0:
                        m_creature->SummonCreature(NPC_HORDE_DEFENDER, SpawnPointsHorde[lSpawnListHorde.front()].fX, SpawnPointsHorde[lSpawnListHorde.front()].fY, SpawnPointsHorde[lSpawnListHorde.front()].fZ, SpawnPointsHorde[lSpawnListHorde.front()].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                        ++m_uiSummonCountHorde;
                        if (m_uiSummonCountHorde <= 4)
                        {
                            m_uiWaitSummonTimerHorde = 1000;
                        }
                        else
                        {
                            m_uiWaitSummonTimerHorde = 12000;
                        }
                        break;
                    case 1:
                        m_creature->SummonCreature(NPC_HORDE_AXE_THROWER, SpawnPointsHorde[lSpawnListHorde.front()].fX, SpawnPointsHorde[lSpawnListHorde.front()].fY, SpawnPointsHorde[lSpawnListHorde.front()].fZ, SpawnPointsHorde[lSpawnListHorde.front()].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                        ++m_uiSummonCountHorde;
                        if (m_uiSummonCountHorde <= 4)
                        {
                            m_uiWaitSummonTimerHorde = 1000;
                        }
                        else
                        {
                            m_uiWaitSummonTimerHorde = 12000;
                        }
                        break;
                }

                if (m_uiPhaseCount == 1)
                    switch (urand(0, 1))
                {
                    case 0:
                        m_creature->SummonCreature(NPC_HORDE_DEFENDER, SpawnPointsHorde[m_uiSummonCountHorde].fX, SpawnPointsHorde[m_uiSummonCountHorde].fY, SpawnPointsHorde[m_uiSummonCountHorde].fZ, SpawnPointsHorde[m_uiSummonCountHorde].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                        ++m_uiSummonCountHorde;
                        break;
                    case 1:
                        m_creature->SummonCreature(NPC_HORDE_AXE_THROWER, SpawnPointsHorde[m_uiSummonCountHorde].fX, SpawnPointsHorde[m_uiSummonCountHorde].fY, SpawnPointsHorde[m_uiSummonCountHorde].fZ, SpawnPointsHorde[m_uiSummonCountHorde].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                        ++m_uiSummonCountHorde;
                        break;
                }
            }

            if (m_uiPhaseCount <= 2 && m_uiWaitSummonTimer < uiDiff && (m_uiSummonCountInvader + m_uiSummonCountStormseer) < 15)
            {

                if (m_uiSummonCountInvader < 8)
                {
                    ++m_uiSummonCountInvader;

                    if (m_uiPhaseCount == 2)
                    {
                        m_creature->SummonCreature(NPC_KOLKAR_INVADER, SpawnPointsKolkar[lSpawnList.front()].fX, SpawnPointsKolkar[lSpawnList.front()].fY, SpawnPointsKolkar[lSpawnList.front()].fZ, SpawnPointsKolkar[lSpawnList.front()].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                        m_uiWaitSummonTimer = 10000;
                    }

                    if (m_uiPhaseCount == 1)
                    {
                        m_creature->SummonCreature(NPC_KOLKAR_INVADER, SpawnPointsKolkar[m_uiCreatureCount].fX, SpawnPointsKolkar[m_uiCreatureCount].fY, SpawnPointsKolkar[m_uiCreatureCount].fZ, SpawnPointsKolkar[m_uiCreatureCount].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                        ++m_uiCreatureCount;
                    }
                }

                if (m_uiSummonCountStormseer < 7)
                {
                    ++m_uiSummonCountStormseer;
                    if (m_uiPhaseCount == 2)
                    {
                        m_creature->SummonCreature(NPC_KOLKAR_STORMSEER, SpawnPointsKolkar[lSpawnList.front()].fX, SpawnPointsKolkar[lSpawnList.front()].fY, SpawnPointsKolkar[lSpawnList.front()].fZ, SpawnPointsKolkar[lSpawnList.front()].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                        m_uiWaitSummonTimer = 10000;
                    }

                    if (m_uiPhaseCount == 1)
                    {
                        m_creature->SummonCreature(NPC_KOLKAR_STORMSEER, SpawnPointsKolkar[m_uiCreatureCount].fX, SpawnPointsKolkar[m_uiCreatureCount].fY, SpawnPointsKolkar[m_uiCreatureCount].fZ, SpawnPointsKolkar[m_uiCreatureCount].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
                        ++m_uiCreatureCount;
                    }
                }

                if (m_uiPhaseCount == 1 && m_uiCreatureCount == 15) //set initial spawn done when 15 enemys spawned
                {
                    m_uiPhaseCount = 2;
                }
            }

            else
            {
                m_uiWaitSummonTimer -= uiDiff;
                m_uiWaitSummonTimerHorde -= uiDiff;
                m_uiEventTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_regthar_deathgateAI(pCreature);
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (pCreature->IsQuestGiver())
        {
            pPlayer->PrepareQuestMenu(pCreature->GetObjectGuid());
        }

        if (npc_regthar_deathgateAI* pRegtharAI = dynamic_cast<npc_regthar_deathgateAI*>(pCreature->AI()))
        {
            if (!pRegtharAI->m_bEventStarted && (pPlayer->GetQuestStatus(QUEST_COUNTERATTACK) == QUEST_STATUS_INCOMPLETE))
            {
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Where is warlord Krom'zar?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            }
              //pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "督军克罗姆扎在哪里？", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }
        pPlayer->SEND_GOSSIP_MENU(2533, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            DoScriptText(SAY_START_REGTHAR, pCreature, pPlayer);
            //pPlayer->CLOSE_GOSSIP_MENU();
            pPlayer->SEND_GOSSIP_MENU(2534, pCreature->GetObjectGuid());
            if (npc_regthar_deathgateAI* pRegtharAI = dynamic_cast<npc_regthar_deathgateAI*>(pCreature->AI()))
            {
                pRegtharAI->StartEvent(pPlayer->GetObjectGuid().GetRawValue());
            }
        }
        return true;
    }
};

/*#####
## npc_horde_defender
#####*/

enum
{
    SAY_AGGRO_1        = -1000895,
    SAY_AGGRO_2        = -1000896,
    SAY_AGGRO_3        = -1000897,
};

struct horde_defender : public CreatureScript
{
    horde_defender() : CreatureScript("npc_horde_defender") {}

    struct npc_horde_defenderAI : public ScriptedAI
    {
        npc_horde_defenderAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint64 i_victimGuid;
        bool m_bCreatureFound;

        void Reset()
        {
        }

        void EnterEvadeMode() override
        {
            m_creature->RemoveAllAurasOnEvade();
//            m_creature->DeleteThreatList();
            m_creature->SetLootRecipient(nullptr);
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* pSender, Unit* pInvoker, uint32 /*miscValue*/)
        {
            if (eventType == AI_EVENT_CUSTOM_A)
            {
                m_creature->AddThreat(pInvoker, 0.0f);
                pSender->AddThreat(m_creature->ToUnit(), 0.0f);
            }
        }

        void Aggro(Unit* /*pWho*/) override
        {
            switch (urand(0, 2))
            {
            case 0:
                DoScriptText(SAY_AGGRO_1, m_creature);
                break;
            case 1:
                DoScriptText(SAY_AGGRO_2, m_creature);
                break;
            case 2:
                DoScriptText(SAY_AGGRO_3, m_creature);
                break;
            }
        }

        void MoveInLineOfSight(Unit* u) override
        {
            if (m_creature->CanInitiateAttack() && u->IsTargetableForAttack() &&
                m_creature->IsHostileTo(u) && u->isInAccessablePlaceFor(m_creature))
            {
                float attackRadius = 38.0f;
                if (m_creature->IsWithinDistInMap(u, attackRadius) && m_creature->IsWithinLOSInMap(u))
                {
                    if (!m_creature->getVictim())
                    {
                        AttackStart(u);

                    }
                }
            }
        }

        void UpdateAI(const uint32 /*uiDiff*/)
        {
            if (!m_creature->getVictim())
            {
                Creature* pCreature = GetClosestCreatureWithEntry(m_creature, NPC_KOLKAR_INVADER, 38.0f);
                if (!pCreature && m_bCreatureFound)
                {
                    m_creature->GetMotionMaster()->MoveTargetedHome();
                    m_bCreatureFound = false;
                }
                if (pCreature)
                {
                    AttackStart(pCreature);
                    m_bCreatureFound = true;
                }
            }
            DoMeleeAttackIfReady();
        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_horde_defenderAI(pCreature);
    }
};

/*#####
## npc_kolkar_invader
#####*/

struct kolkar_invader : public CreatureScript
{
    kolkar_invader() : CreatureScript("npc_kolkar_invader") {}

    struct npc_kolkar_invaderAI : public ScriptedAI
    {
        npc_kolkar_invaderAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint64 i_victimGuid;
        bool m_bCreatureFound;

        void Reset()
        {
        }

        void JustRespawned()
        {
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* pSender, Unit* pInvoker, uint32 /*miscValue*/)
        {
            if (eventType == AI_EVENT_CUSTOM_A)
            {
                m_creature->AddThreat(pInvoker, 0.0f);
                pSender->AddThreat(m_creature->ToUnit(), 0.0f);
            }
        }

        void EnterEvadeMode() override
        {
            m_creature->RemoveAllAurasOnEvade();
            //m_creature->DeleteThreatList();
            m_creature->SetLootRecipient(nullptr);
        }

        void MoveInLineOfSight(Unit* u) override
        {
            if (m_creature->CanInitiateAttack() && u->IsTargetableForAttack() &&
                m_creature->IsHostileTo(u) && u->isInAccessablePlaceFor(m_creature))
            {
                float attackRadius = 38.0f;
                if (m_creature->IsWithinDistInMap(u, attackRadius) && m_creature->IsWithinLOSInMap(u))
                {
                    if (!m_creature->getVictim())
                    {
                        AttackStart(u);

                    }
                }
            }
        }



        void UpdateAI(const uint32 /*uiDiff*/)
        {
            if (!m_creature->getVictim())
            {
                Creature* pCreature = GetClosestCreatureWithEntry(m_creature, NPC_HORDE_DEFENDER |  NPC_HORDE_AXE_THROWER , 38.0f);
                if (!pCreature && m_bCreatureFound)
                {
                    m_creature->GetMotionMaster()->MoveTargetedHome();
                    m_bCreatureFound = false;
                }
                if (pCreature)
                {
                    AttackStart(pCreature);
                    m_bCreatureFound = true;
                }
            }
            DoMeleeAttackIfReady();
        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_kolkar_invaderAI(pCreature);
    }
};

/*#####
## NPC_WARLORD_KROMZAR
#####*/

struct warlord_kromzar : public CreatureScript
{
    warlord_kromzar() : CreatureScript("npc_warlord_kromzar") {}

    struct npc_warlord_kromzarAI : public npc_escortAI
    {
        npc_warlord_kromzarAI(Creature* pCreature) : npc_escortAI(pCreature) {}

        void Reset() {}

        void WaypointReached(uint32 uiPointId)
        {

        }

        void JustDied(Unit* /**/)
        {
            m_creature->CastSpell(m_creature->ToUnit(), 13965, true);
        }

        void JustRespawned() override
        {
            DoScriptText(YELL_STRONGEST, m_creature);
            m_creature->SummonCreature(NPC_KOLKAR_INVADER, SpawnPointsKromzar[0].fX, SpawnPointsKromzar[0].fY, SpawnPointsKromzar[0].fZ, SpawnPointsKromzar[0].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
            m_creature->SummonCreature(NPC_KOLKAR_STORMSEER, SpawnPointsKromzar[2].fX, SpawnPointsKromzar[2].fY, SpawnPointsKromzar[2].fZ, SpawnPointsKromzar[2].fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 40000);
            Start(false, nullptr, nullptr, false, true);
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_warlord_kromzarAI(pCreature);
    }
};

void AddSC_the_barrens()
{
    Script* s;
    s = new npc_beaten_corpse();
    s->RegisterSelf();
    s = new npc_gilthares();
    s->RegisterSelf();
    s = new npc_taskmaster_fizzule();
    s->RegisterSelf();
    s = new npc_twiggy_flathead();
    s->RegisterSelf();
    s = new at_twiggy_flathead();
    s->RegisterSelf();
    s = new npc_wizzlecranks_shredder();
    s->RegisterSelf();
    s = new npc_regthar_deathgate();
    s->RegisterSelf();
    s = new horde_defender();
    s->RegisterSelf();
    s = new kolkar_invader();
    s->RegisterSelf();
    s = new warlord_kromzar();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_beaten_corpse";
    //pNewScript->pGossipHello = &GossipHello_npc_beaten_corpse;
    //pNewScript->pGossipSelect = &GossipSelect_npc_beaten_corpse;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_gilthares";
    //pNewScript->GetAI = &GetAI_npc_gilthares;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_gilthares;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_taskmaster_fizzule";
    //pNewScript->GetAI = &GetAI_npc_taskmaster_fizzule;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_twiggy_flathead";
    //pNewScript->GetAI = &GetAI_npc_twiggy_flathead;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_twiggy_flathead";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_twiggy_flathead;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_wizzlecranks_shredder";
    //pNewScript->GetAI = &GetAI_npc_wizzlecranks_shredder;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_wizzlecranks_shredder;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_regthar_deathgate";
    //pNewScript->pGossipHello = &GossipHello_npc_regthar_deathgate;
    //pNewScript->pGossipSelect = &GossipSelect_npc_regthar_deathgate;
    //pNewScript->GetAI = &GetAI_npc_regthar_deathgate;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_horde_defender";
    //pNewScript->GetAI = &GetAI_npc_horde_defender;
    //pNewScript->RegisterSelf();
}
