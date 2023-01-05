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

/**
 * ScriptData
 * SDName:      Boss_Vaelastrasz
 * SD%Complete: 75
 * SDComment:   Burning Adrenaline not correctly implemented in core
 * SDCategory:  Blackwing Lair
 * EndScriptData
 */

#include "precompiled.h"
#include "blackwing_lair.h"

enum
{
    SAY_LINE_1                  = -1469026,
    SAY_LINE_2                  = -1469027,
    SAY_LINE_3                  = -1469028,
    SAY_HALFLIFE                = -1469029,
    SAY_KILLTARGET              = -1469030,
    SAY_NEFARIUS_CORRUPT_1      = -1469006,                 // When he corrupts Vaelastrasz
    SAY_NEFARIUS_CORRUPT_2      = -1469032,
    SAY_TECHNICIAN_RUN          = -1469034,

    SPELL_ESSENCE_OF_THE_RED    = 23513,
    SPELL_FLAME_BREATH          = 23461,
    SPELL_FIRE_NOVA             = 23462,
    SPELL_TAIL_SWEEP            = 15847,
    SPELL_BURNING_ADRENALINE    = 23620,
    SPELL_CLEAVE                = 20684,                    // Chain cleave is most likely named something different and contains a dummy effect

    SPELL_NEFARIUS_CORRUPTION   = 23642,

    GOSSIP_ITEM_VAEL_1          = -3469003,
    GOSSIP_ITEM_VAEL_2          = -3469004,
    // Vael Gossip texts might be 7156 and 7256; At the moment are missing from DB
    // For the moment add the default values
    GOSSIP_TEXT_VAEL_1          = 384,
    GOSSIP_TEXT_VAEL_2          = 384,

    FACTION_HOSTILE             = 14,

    AREATRIGGER_VAEL_INTRO      = 3626,
};

struct boss_vaelastrasz : public CreatureScript
{
    boss_vaelastrasz() : CreatureScript("boss_vaelastrasz") {}

    struct boss_vaelastraszAI : public ScriptedAI
    {
        boss_vaelastraszAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();

            // Set stand state to dead before the intro event
            m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
        }

        ScriptedInstance* m_pInstance;

        ObjectGuid m_nefariusGuid;
        uint32 m_uiIntroTimer;
        uint8 m_uiIntroPhase;

        ObjectGuid m_playerGuid;
        uint32 m_uiSpeechTimer;
        uint8 m_uiSpeechNum;

        uint32 m_uiCleaveTimer;
        uint32 m_uiFlameBreathTimer;
        uint32 m_uiFireNovaTimer;
        uint32 m_uiBurningAdrenalineCasterTimer;
        uint32 m_uiBurningAdrenalineTankTimer;
        uint32 m_uiTailSweepTimer;
        bool m_bHasYelled;

        void Reset() override
        {
            m_playerGuid.Clear();

            m_uiIntroTimer = 0;
            m_uiIntroPhase = 0;
            m_uiSpeechTimer = 0;
            m_uiSpeechNum = 0;
            m_uiCleaveTimer = 8000;            // These times are probably wrong
            m_uiFlameBreathTimer = 11000;
            m_uiBurningAdrenalineCasterTimer = 15000;
            m_uiBurningAdrenalineTankTimer = 45000;
            m_uiFireNovaTimer = 5000;
            m_uiTailSweepTimer = 20000;
            m_bHasYelled = false;

            // Creature should have only 1/3 of hp
            m_creature->SetHealth(uint32(m_creature->GetMaxHealth()*0.3f));
        }

        void BeginIntro()
        {
            // Start Intro delayed
            m_uiIntroTimer = 1000;

            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_VAELASTRASZ, SPECIAL);
            }
        }

        void BeginSpeech(Player* pTarget)
        {
            // Stand up and begin speach
            m_playerGuid = pTarget->GetObjectGuid();

            // 10 seconds
            DoScriptText(SAY_LINE_1, m_creature);

            // Make boss stand
            m_creature->SetStandState(UNIT_STAND_STATE_STAND);

            m_uiSpeechTimer = 10000;
            m_uiSpeechNum = 0;
        }

        void KilledUnit(Unit* pVictim) override
        {
            if (urand(0, 4))
            {
                return;
            }

            DoScriptText(SAY_KILLTARGET, m_creature, pVictim);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_VAELASTRASZ, IN_PROGRESS);
            }

            // Buff players on aggro
            DoCastSpellIfCan(m_creature, SPELL_ESSENCE_OF_THE_RED);
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_VAELASTRASZ, DONE);
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_VAELASTRASZ, FAIL);
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_LORD_VICTOR_NEFARIUS)
            {
                // Set not selectable, so players won't interact with it
                pSummoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                m_nefariusGuid = pSummoned->GetObjectGuid();
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* pSender, Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            if (eventType == AI_EVENT_START_EVENT && pSender == m_creature && pInvoker->GetTypeId() == TYPEID_PLAYER)
            {
                BeginIntro();
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiIntroTimer)
            {
                if (m_uiIntroTimer <= uiDiff)
                {
                    switch (m_uiIntroPhase)
                    {
                    case 0:
                        m_creature->SummonCreature(NPC_LORD_VICTOR_NEFARIUS, aNefariusSpawnLoc[0], aNefariusSpawnLoc[1], aNefariusSpawnLoc[2], aNefariusSpawnLoc[3], TEMPSPAWN_TIMED_DESPAWN, 25000);
                        m_uiIntroTimer = 1000;
                        break;
                    case 1:
                        if (Creature* pNefarius = m_creature->GetMap()->GetCreature(m_nefariusGuid))
                        {
                            pNefarius->CastSpell(m_creature, SPELL_NEFARIUS_CORRUPTION, true);
                            DoScriptText(SAY_NEFARIUS_CORRUPT_1, pNefarius);
                        }
                        m_uiIntroTimer = 16000;
                        break;
                    case 2:
                        if (Creature* pNefarius = m_creature->GetMap()->GetCreature(m_nefariusGuid))
                        {
                            DoScriptText(SAY_NEFARIUS_CORRUPT_2, pNefarius);
                        }

                        // Set npc flags now
                        m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                        m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                        m_uiIntroTimer = 0;
                        break;
                    }
                    ++m_uiIntroPhase;
                }
                else
                {
                    m_uiIntroTimer -= uiDiff;
                }
            }

            // Speech
            if (m_uiSpeechTimer)
            {
                if (m_uiSpeechTimer <= uiDiff)
                {
                    switch (m_uiSpeechNum)
                    {
                    case 0:
                        // 16 seconds till next line
                        DoScriptText(SAY_LINE_2, m_creature);
                        m_uiSpeechTimer = 16000;
                        ++m_uiSpeechNum;
                        break;
                    case 1:
                        // This one is actually 16 seconds but we only go to 10 seconds because he starts attacking after he says "I must fight this!"
                        DoScriptText(SAY_LINE_3, m_creature);
                        m_uiSpeechTimer = 10000;
                        ++m_uiSpeechNum;
                        break;
                    case 2:
                        m_creature->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_RESPAWN);

                        if (m_playerGuid)
                        {
                            if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
                            {
                                AttackStart(pPlayer);
                            }
                        }
                        m_uiSpeechTimer = 0;
                        break;
                    }
                }
                else
                {
                    m_uiSpeechTimer -= uiDiff;
                }
            }

            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Yell if hp lower than 15%
            if (m_creature->GetHealthPercent() < 15.0f && !m_bHasYelled)
            {
                DoScriptText(SAY_HALFLIFE, m_creature);
                m_bHasYelled = true;
            }

            // Cleave Timer
            if (m_uiCleaveTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                {
                    m_uiCleaveTimer = 15000;
                }
            }
            else
            {
                m_uiCleaveTimer -= uiDiff;
            }

            // Flame Breath Timer
            if (m_uiFlameBreathTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FLAME_BREATH) == CAST_OK)
                {
                    m_uiFlameBreathTimer = urand(4000, 8000);
                }
            }
            else
            {
                m_uiFlameBreathTimer -= uiDiff;
            }

            // Burning Adrenaline Caster Timer
            if (m_uiBurningAdrenalineCasterTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_BURNING_ADRENALINE, SELECT_FLAG_PLAYER | SELECT_FLAG_POWER_MANA))
                {
                    pTarget->CastSpell(pTarget, SPELL_BURNING_ADRENALINE, true, nullptr, nullptr, m_creature->GetObjectGuid());
                    m_uiBurningAdrenalineCasterTimer = 15000;
                }
            }
            else
            {
                m_uiBurningAdrenalineCasterTimer -= uiDiff;
            }

            // Burning Adrenaline Tank Timer
            if (m_uiBurningAdrenalineTankTimer < uiDiff)
            {
                // have the victim cast the spell on himself otherwise the third effect aura will be applied
                // to Vael instead of the player
                m_creature->getVictim()->CastSpell(m_creature->getVictim(), SPELL_BURNING_ADRENALINE, true, nullptr, nullptr, m_creature->GetObjectGuid());

                m_uiBurningAdrenalineTankTimer = 45000;
            }
            else
            {
                m_uiBurningAdrenalineTankTimer -= uiDiff;
            }

            // Fire Nova Timer
            if (m_uiFireNovaTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FIRE_NOVA) == CAST_OK)
                {
                    m_uiFireNovaTimer = 5000;
                }
            }
            else
            {
                m_uiFireNovaTimer -= uiDiff;
            }

            // Tail Sweep Timer
            if (m_uiTailSweepTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_TAIL_SWEEP) == CAST_OK)
                {
                    m_uiTailSweepTimer = 20000;
                }
            }
            else
            {
                m_uiTailSweepTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch (uiAction)
        {
        case GOSSIP_ACTION_INFO_DEF + 1:
            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_VAEL_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXT_VAEL_2, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 2:
            pPlayer->CLOSE_GOSSIP_MENU();
            if (boss_vaelastraszAI* pVaelAI = dynamic_cast<boss_vaelastraszAI*>(pCreature->AI()))
            {
                pVaelAI->BeginSpeech(pPlayer);
            }
            break;
        }

        return true;
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pCreature->IsQuestGiver())
        {
            pPlayer->PrepareQuestMenu(pCreature->GetObjectGuid());
        }

        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_VAEL_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXT_VAEL_1, pCreature->GetObjectGuid());

        return true;
    }

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_vaelastraszAI(pCreature);
    }
};

struct at_vaelastrasz : public AreaTriggerScript
{
    at_vaelastrasz() : AreaTriggerScript("at_vaelastrasz") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (pAt->id == AREATRIGGER_VAEL_INTRO)
        {
            if (pPlayer->isGameMaster() || pPlayer->IsDead())
            {
                return false;
            }

            if (ScriptedInstance* pInstance = (ScriptedInstance*)pPlayer->GetInstanceData())
            {
                // Handle intro event
                if (pInstance->GetData(TYPE_VAELASTRASZ) == NOT_STARTED)
                {
                    if (Creature* pVaelastrasz = pInstance->GetSingleCreatureFromStorage(NPC_VAELASTRASZ))
                        if (CreatureAI* pVaelAI = pVaelastrasz->AI())
                        {
                            pVaelAI->SendAIEvent(AI_EVENT_START_EVENT, pPlayer, pVaelastrasz);
                        }
                }

                // ToDo: make goblins flee
            }
        }

        return false;
    }
};

void AddSC_boss_vaelastrasz()
{
    Script* s;
    s = new boss_vaelastrasz();
    s->RegisterSelf();
    s = new at_vaelastrasz();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_vaelastrasz";
    //pNewScript->GetAI = &GetAI_boss_vaelastrasz;
    //pNewScript->pGossipHello = &GossipHello_boss_vaelastrasz;
    //pNewScript->pGossipSelect = &GossipSelect_boss_vaelastrasz;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_vaelastrasz";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_vaelastrasz;
    //pNewScript->RegisterSelf();
}
