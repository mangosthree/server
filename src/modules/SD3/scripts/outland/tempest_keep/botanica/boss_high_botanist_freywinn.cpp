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
SDName: Boss_High_Botanist_Freywinn
SD%Complete: 90
SDComment: Timers may need some fine adjustments
SDCategory: Tempest Keep, The Botanica
EndScriptData */

#include "precompiled.h"

enum
{
    SAY_AGGRO                   = -1553000,
    SAY_KILL_1                  = -1553001,
    SAY_KILL_2                  = -1553002,
    SAY_TREE_1                  = -1553003,
    SAY_TREE_2                  = -1553004,
    SAY_DEATH                   = -1553005,

    SPELL_TRANQUILITY           = 34550,
    SPELL_TREE_FORM             = 34551,
    SPELL_SUMMON_FRAYER         = 34557,
    SPELL_PLANT_WHITE           = 34759,
    SPELL_PLANT_GREEN           = 34761,
    SPELL_PLANT_BLUE            = 34762,
    SPELL_PLANT_RED             = 34763,

    NPC_FRAYER_PROTECTOR        = 19953,
};

struct boss_high_botanist_freywinn : public CreatureScript
{
    boss_high_botanist_freywinn() : CreatureScript("boss_high_botanist_freywinn") {}

    struct boss_high_botanist_freywinnAI : public ScriptedAI
    {
        boss_high_botanist_freywinnAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiSummonSeedlingTimer;
        uint32 m_uiTreeFormTimer;
        uint32 m_uiFrayerTimer;
        uint32 m_uiTreeFormEndTimer;
        uint8 m_uiFrayerAddsCount;
        bool m_bCanMoveFree;

        void Reset() override
        {
            m_uiSummonSeedlingTimer = 6000;
            m_uiTreeFormTimer = 30000;
            m_uiTreeFormEndTimer = 0;
            m_uiFrayerAddsCount = 0;
            m_uiFrayerTimer = 0;
            m_bCanMoveFree = true;
        }

        void Aggro(Unit* /*pWho*/) override
        {
            DoScriptText(SAY_AGGRO, m_creature);
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_FRAYER_PROTECTOR)
            {
                ++m_uiFrayerAddsCount;
            }

            // Attack players
            if (m_creature->getVictim())
            {
                pSummoned->AI()->AttackStart(m_creature->getVictim());
            }
        }

        void SummonedCreatureJustDied(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_FRAYER_PROTECTOR)
            {
                --m_uiFrayerAddsCount;

                // When all 3 Frayers are killed stop the tree form action (if not done this already)
                if (!m_uiFrayerAddsCount && !m_bCanMoveFree)
                {
                    m_uiTreeFormEndTimer = 0;

                    if (m_creature->getVictim())
                    {
                        m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());
                    }

                    // Interrupt all spells and remove auras
                    m_creature->InterruptNonMeleeSpells(true);
                    m_creature->RemoveAllAuras();
                    m_bCanMoveFree = true;
                }
            }
        }

        // Wrapper to summon one seedling
        void DoSummonSeedling()
        {
            switch (urand(0, 3))
            {
            case 0: DoCastSpellIfCan(m_creature, SPELL_PLANT_WHITE); break;
            case 1: DoCastSpellIfCan(m_creature, SPELL_PLANT_GREEN); break;
            case 2: DoCastSpellIfCan(m_creature, SPELL_PLANT_BLUE);  break;
            case 3: DoCastSpellIfCan(m_creature, SPELL_PLANT_RED);   break;
            }
        }

        void KilledUnit(Unit* /*pVictim*/) override
        {
            DoScriptText(urand(0, 1) ? SAY_KILL_1 : SAY_KILL_2, m_creature);
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            DoScriptText(SAY_DEATH, m_creature);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_uiTreeFormTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_TRANQUILITY) == CAST_OK)
                {
                    // Note: This should remove only negative auras
                    m_creature->RemoveAllAuras();

                    DoCastSpellIfCan(m_creature, SPELL_TREE_FORM, CAST_TRIGGERED);
                    DoScriptText(urand(0, 1) ? SAY_TREE_1 : SAY_TREE_2, m_creature);

                    m_creature->GetMotionMaster()->MoveIdle();
                    m_bCanMoveFree = false;
                    m_uiFrayerTimer = 1000;
                    m_uiTreeFormEndTimer = 45000;
                    m_uiTreeFormTimer = 75000;
                }
            }
            else
            {
                m_uiTreeFormTimer -= uiDiff;
            }

            // The Frayer is summoned after one second in the tree phase
            if (m_uiFrayerTimer)
            {
                if (m_uiFrayerTimer <= uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_SUMMON_FRAYER, CAST_TRIGGERED) == CAST_OK)
                    {
                        m_uiFrayerTimer = 0;
                    }
                }
                else
                {
                    m_uiFrayerTimer -= uiDiff;
                }
            }

            // Tree phase will be removed when the timer expires;
            if (m_uiTreeFormEndTimer)
            {
                if (m_uiTreeFormEndTimer <= uiDiff)
                {
                    if (m_creature->getVictim())
                    {
                        m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());
                    }
                    m_bCanMoveFree = true;
                    m_uiTreeFormEndTimer = 0;
                }
                else
                {
                    m_uiTreeFormEndTimer -= uiDiff;
                }
            }

            // Don't do any other actions during tree form
            if (!m_bCanMoveFree)
            {
                return;
            }

            // one random seedling every 5 secs, but not in tree form
            if (m_uiSummonSeedlingTimer < uiDiff)
            {
                DoSummonSeedling();
                m_uiSummonSeedlingTimer = 6000;
            }
            else
            {
                m_uiSummonSeedlingTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_high_botanist_freywinnAI(pCreature);
    }
};

void AddSC_boss_high_botanist_freywinn()
{
    Script* s;

    s = new boss_high_botanist_freywinn();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_high_botanist_freywinn";
    //pNewScript->GetAI = &GetAI_boss_high_botanist_freywinn;
    //pNewScript->RegisterSelf();
}
