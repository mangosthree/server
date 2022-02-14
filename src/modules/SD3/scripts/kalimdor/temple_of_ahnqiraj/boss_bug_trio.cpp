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
 * SDName:      bug_trio
 * SD%Complete: 75
 * SDComment:   Summon Player spell NYI; Poison Cloud damage spell NYI; Timers need adjustments
 * SDCategory:  Temple of Ahn'Qiraj
 * EndScriptData
 */

#include "precompiled.h"
#include "temple_of_ahnqiraj.h"

enum
{
    // kri
    SPELL_THRASH            = 8876,
    SPELL_CLEAVE            = 26350,
    SPELL_TOXIC_VOLLEY      = 25812,
    SPELL_TOXIC_VAPORS      = 25786,

    // vem
    SPELL_CHARGE            = 26561,
    SPELL_VENGEANCE         = 25790,
    SPELL_KNOCKBACK         = 18670,
    SPELL_KNOCKDOWN         = 19128,

    // yauj
    SPELL_HEAL              = 25807,
    SPELL_FEAR              = 26580,
    SPELL_RAVAGE            = 3242,

    NPC_YAUJ_BROOD          = 15621
};

struct boss_kri : public CreatureScript
{
    boss_kri() : CreatureScript("boss_kri") {}

    struct boss_kriAI : public ScriptedAI
    {
        boss_kriAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();

            // Thrash
            DoCastSpellIfCan(m_creature, SPELL_THRASH, CAST_TRIGGERED | CAST_AURA_NOT_PRESENT);
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiCleaveTimer;
        uint32 m_uiToxicVolleyTimer;

        void Reset() override
        {
            m_uiCleaveTimer = urand(4000, 8000);
            m_uiToxicVolleyTimer = urand(6000, 12000);
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            // poison cloud on death
            DoCastSpellIfCan(m_creature, SPELL_TOXIC_VAPORS, CAST_TRIGGERED);

            if (!m_pInstance)
            {
                return;
            }

            // If the other 2 bugs are still alive, make unlootable
            if (m_pInstance->GetData(TYPE_BUG_TRIO) != DONE)
            {
                m_creature->SetLootRecipient(nullptr);
                m_pInstance->SetData(TYPE_BUG_TRIO, SPECIAL);

                if (Creature* pVem = m_pInstance->GetSingleCreatureFromStorage(NPC_VEM))
                {
                    m_creature->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, m_creature, pVem);
                }
                if (Creature* pYauj = m_pInstance->GetSingleCreatureFromStorage(NPC_YAUJ))
                {
                    m_creature->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, m_creature, pYauj);
                }
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_BUG_TRIO, FAIL);
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* pSender, Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            if (!(pSender->GetEntry() == NPC_VEM || pSender->GetEntry() == NPC_YAUJ))
            {
                return;
            }

            if (m_creature->IsInCombat())
            {
                m_creature->SetHealth(m_creature->GetMaxHealth());
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Cleave_Timer
            if (m_uiCleaveTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                {
                    m_uiCleaveTimer = urand(5000, 12000);
                }
            }
            else
            {
                m_uiCleaveTimer -= uiDiff;
            }

            // ToxicVolley_Timer
            if (m_uiToxicVolleyTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_TOXIC_VOLLEY) == CAST_OK)
                {
                    m_uiToxicVolleyTimer = urand(10000, 15000);
                }
            }
            else
            {
                m_uiToxicVolleyTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_kriAI(pCreature);
    }
};

struct boss_vem : public CreatureScript
{
    boss_vem() : CreatureScript("boss_vem") {}

    struct boss_vemAI : public ScriptedAI
    {
        boss_vemAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiChargeTimer;
        uint32 m_uiKnockBackTimer;
        uint32 m_uiKnockDownTimer;

        void Reset() override
        {
            m_uiChargeTimer = urand(15000, 27000);
            m_uiKnockBackTimer = urand(8000, 20000);
            m_uiKnockDownTimer = urand(10000, 23000);
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            // Enrage the other bugs
            DoCastSpellIfCan(m_creature, SPELL_VENGEANCE, CAST_TRIGGERED);

            if (!m_pInstance)
            {
                return;
            }

            // If the other 2 bugs are still alive, make unlootable
            if (m_pInstance->GetData(TYPE_BUG_TRIO) != DONE)
            {
                m_creature->SetLootRecipient(nullptr);
                m_pInstance->SetData(TYPE_BUG_TRIO, SPECIAL);

                if (Creature* pKri = m_pInstance->GetSingleCreatureFromStorage(NPC_KRI))
                {
                    m_creature->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, m_creature, pKri);
                }
                if (Creature* pYauj = m_pInstance->GetSingleCreatureFromStorage(NPC_YAUJ))
                {
                    m_creature->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, m_creature, pYauj);
                }
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_BUG_TRIO, FAIL);
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* pSender, Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            if (!(pSender->GetEntry() == NPC_KRI || pSender->GetEntry() == NPC_YAUJ))
            {
                return;
            }

            if (m_creature->IsInCombat())
            {
                m_creature->SetHealth(m_creature->GetMaxHealth());
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Charge_Timer
            if (m_uiChargeTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_CHARGE) == CAST_OK)
                    {
                        m_uiChargeTimer = urand(8000, 16000);
                    }
                }
            }
            else
            {
                m_uiChargeTimer -= uiDiff;
            }

            // KnockBack_Timer
            if (m_uiKnockBackTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_KNOCKBACK) == CAST_OK)
                {
                    if (m_creature->GetThreatManager().getThreat(m_creature->getVictim()))
                    {
                        m_creature->GetThreatManager().modifyThreatPercent(m_creature->getVictim(), -80);
                    }

                    m_uiKnockBackTimer = urand(15000, 25000);
                }
            }
            else
            {
                m_uiKnockBackTimer -= uiDiff;
            }

            // KnockDown_Timer
            if (m_uiKnockDownTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_KNOCKDOWN) == CAST_OK)
                {
                    m_uiKnockDownTimer = urand(15000, 25000);
                }
            }
            else
            {
                m_uiKnockDownTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_vemAI(pCreature);
    }
};

struct boss_yauj : public CreatureScript
{
    boss_yauj() : CreatureScript("boss_yauj") {}

    struct boss_yaujAI : public ScriptedAI
    {
        boss_yaujAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiHealTimer;
        uint32 m_uiFearTimer;
        uint32 m_uiRavageTimer;

        void Reset() override
        {
            m_uiHealTimer = urand(25000, 40000);
            m_uiFearTimer = urand(12000, 24000);
            m_uiRavageTimer = urand(18000, 20000);
        }

        void JustDied(Unit* /*Killer*/) override
        {
            // Spawn 10 yauj brood on death
            float fX, fY, fZ;
            for (int i = 0; i < 10; ++i)
            {
                m_creature->GetRandomPoint(m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 10.0f, fX, fY, fZ);
                m_creature->SummonCreature(NPC_YAUJ_BROOD, fX, fY, fZ, 0.0f, TEMPSPAWN_TIMED_OOC_DESPAWN, 30000);
            }

            if (!m_pInstance)
            {
                return;
            }

            // If the other 2 bugs are still alive, make unlootable
            if (m_pInstance->GetData(TYPE_BUG_TRIO) != DONE)
            {
                m_creature->SetLootRecipient(nullptr);
                m_pInstance->SetData(TYPE_BUG_TRIO, SPECIAL);

                if (Creature* pKri = m_pInstance->GetSingleCreatureFromStorage(NPC_KRI))
                {
                    m_creature->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, m_creature, pKri);
                }
                if (Creature* pVem = m_pInstance->GetSingleCreatureFromStorage(NPC_VEM))
                {
                    m_creature->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, m_creature, pVem);
                }
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_BUG_TRIO, FAIL);
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* pSender, Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            if (!(pSender->GetEntry() == NPC_KRI || pSender->GetEntry() == NPC_VEM))
            {
                return;
            }

            if (m_creature->IsInCombat())
            {
                m_creature->SetHealth(m_creature->GetMaxHealth());
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Fear_Timer
            if (m_uiFearTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FEAR) == CAST_OK)
                {
                    DoResetThreat();
                    m_uiFearTimer = 20000;
                }
            }
            else
            {
                m_uiFearTimer -= uiDiff;
            }

            // Heal
            if (m_uiHealTimer < uiDiff)
            {
                if (Unit* pTarget = DoSelectLowestHpFriendly(100.0f))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_HEAL) == CAST_OK)
                    {
                        m_uiHealTimer = urand(15000, 30000);
                    }
                }
            }
            else
            {
                m_uiHealTimer -= uiDiff;
            }

            // Ravage_Timer
            if (m_uiRavageTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_RAVAGE) == CAST_OK)
                {
                    m_uiRavageTimer = 16000;
                }
            }
            else
            {
                m_uiRavageTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_yaujAI(pCreature);
    }
};

void AddSC_bug_trio()
{
    Script* s;
    s = new boss_kri();
    s->RegisterSelf();
    s = new boss_vem();
    s->RegisterSelf();
    s = new boss_yauj();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_kri";
    //pNewScript->GetAI = &GetAI_boss_kri;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_vem";
    //pNewScript->GetAI = &GetAI_boss_vem;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_yauj";
    //pNewScript->GetAI = &GetAI_boss_yauj;
    //pNewScript->RegisterSelf();
}
