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
 * SDName:      Gilneas City
 * SD%Complete: 0 Work in progress...
 * SDComment:
 * SDCategory:  Gilneas City
 * EndScriptData
 */

/**
 * ContentData
 * npc_prince_liam_greymane_phase1     - Fully Scripted
 * npc_prince_liam_greymane_phase2     - Fully Scripted
 * rampaging_worgen                    - Enrage Spell Scripted
 * frightened_citizen_quest
 * EndContentData
 */

#include "precompiled.h"
#include "escort_ai.h"
#include <cmath>

/*######
# npc_prince_liam_greymane_phase1
######*/

enum {
    SAY_STORYLINE1        = -1654001,
    SAY_STORYLINE2        = -1654002,
    SAY_STORYLINE3        = -1654003
};

struct npc_prince_liam_greymane_phase1 : public CreatureScript
{
    npc_prince_liam_greymane_phase1() : CreatureScript("npc_prince_liam_greymane_phase1") {}

    struct npc_prince_liam_greymane_phase1AI : public ScriptedAI
    {
        npc_prince_liam_greymane_phase1AI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_uiNpcFlags = pCreature->GetUInt32Value(UNIT_NPC_FLAGS);
            Reset();
        }

        ObjectGuid m_casterGuid;
        uint32 m_uiNpcFlags;
        uint32 m_uiSayStoryDelay;
        uint32 m_uiSayStoryTimer;
        int m_uiSayStoryLast;

        bool m_bCanSayStory;

        void Reset() override
        {
            m_casterGuid.Clear();

            m_uiSayStoryDelay = 15000;
            m_uiSayStoryTimer = m_uiSayStoryDelay;
            m_uiSayStoryLast = 0;

            m_bCanSayStory = true;

            m_creature->SetUInt32Value(UNIT_NPC_FLAGS, m_uiNpcFlags);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_bCanSayStory)
            {
                m_uiSayStoryTimer = m_uiSayStoryDelay;

                // Random switch between 3 texts
                switch (m_uiSayStoryLast)
                {
                    case 0: DoScriptText(SAY_STORYLINE1, m_creature); m_uiSayStoryLast++; break;
                    case 1: DoScriptText(SAY_STORYLINE2, m_creature); m_uiSayStoryLast++; break;
                    case 2: DoScriptText(SAY_STORYLINE3, m_creature); m_uiSayStoryLast = 0; m_uiSayStoryTimer = 60000; break;
                    //case 3: DoScriptText(SAY_HELP4, m_creature); break;
                }

                m_bCanSayStory = false;
            }

            if (m_uiSayStoryTimer < uiDiff)
            {
                m_bCanSayStory = true;
                m_uiSayStoryTimer = m_uiSayStoryDelay;
            }
            else m_uiSayStoryTimer -= uiDiff;
        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prince_liam_greymane_phase1AI(pCreature);
    }
};







/*######
# npc_prince_liam_greymane_phase2
######*/

enum {
    SAY_STORYLINE4        = -1654004,
    SAY_STORYLINE5        = -1654005,
    SAY_STORYLINE6        = -1654006,
    SAY_STORYLINE7        = -1654007,
    SAY_STORYLINE8        = -1654008,

    SAY_STORY_DELAY        = 30000,
    SPELL_SHOOT            = 50092,
    SPELL_CNOCKING        = 67869,

    QUEST_EVAQUATE_THE_MERCHANT_SQUARE = 14098
};

struct npc_prince_liam_greymane_phase2 : public CreatureScript
{
    npc_prince_liam_greymane_phase2() : CreatureScript("npc_prince_liam_greymane_phase2") {}

    struct npc_prince_liam_greymane_phase2AI : public ScriptedAI
    {
        npc_prince_liam_greymane_phase2AI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            Reset();
        }

        uint32 m_uiNpcFlags;
        uint32 m_uiSayStoryTimer;

        Unit* lastVictim;

        int m_uiSayStoryLast;

        bool m_bCanSayStory;
        bool IsSelfRooted;

        void Reset() override
        {
            m_uiSayStoryTimer = SAY_STORY_DELAY;
            m_uiSayStoryLast = 0;
            lastVictim = nullptr;

            m_bCanSayStory = true;
            IsSelfRooted = false;

        }

        void Aggro(Unit* who) override
        {
            if (!m_creature->CanReachWithMeleeAttack(who) && !m_creature->CanUseEquippedWeapon(RANGED_ATTACK))
            {
                IsSelfRooted = true;
            }
        }

        void AttackedBy(Unit* pAttacker) override
        {
            // Check if Liam is attacking who attack he and don't jump on multiple attackers
            if (m_creature->getVictim() && (m_creature->getVictim() == pAttacker || lastVictim == pAttacker))
            {
                return;
            }

            if (m_creature->IsFriendlyTo(pAttacker))
            {
                return;
            }

            lastVictim = pAttacker;
            AttackStart(pAttacker);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Ready to say emote
            if (m_bCanSayStory)
            {
                m_uiSayStoryTimer = SAY_STORY_DELAY;

                // Sequence switch between 3 texts
                switch (m_uiSayStoryLast)
                {
                    case 0: DoScriptText(SAY_STORYLINE4, m_creature); m_uiSayStoryLast++; break;
                    case 1: DoScriptText(SAY_STORYLINE5, m_creature); m_uiSayStoryLast++; break;
                    case 2: DoScriptText(SAY_STORYLINE6, m_creature); m_uiSayStoryLast++; break;
                    case 3: DoScriptText(SAY_STORYLINE7, m_creature); m_uiSayStoryLast++; break;
                    case 4: DoScriptText(SAY_STORYLINE8, m_creature); m_uiSayStoryLast = 0; break;
                    //case 3: DoScriptText(SAY_HELP4, m_creature); break;
                }

                m_bCanSayStory = false;
            }

            if (m_uiSayStoryTimer < uiDiff)
            {
                m_bCanSayStory = true;
                m_uiSayStoryTimer = SAY_STORY_DELAY;
            }
            else m_uiSayStoryTimer -= uiDiff;


            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Equipped ranged weapon usable and not close to victim
            if (m_creature->GetCombatDistance(m_creature->getVictim(), false) > 0 && m_creature->CanUseEquippedWeapon(RANGED_ATTACK) )
            {
                // Make sure our attack is ready
                if (m_creature->isAttackReady(RANGED_ATTACK)){
                    // Use spell instead of normal ranged attack that seem not working. TODO: check ranged attack!
                    DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHOOT);
                    //m_creature->AttackerStateUpdate(m_creature->getVictim(),RANGED_ATTACK);
                    m_creature->resetAttackTimer(RANGED_ATTACK);
                }

            }
            else if (m_creature->CanReachWithMeleeAttack(m_creature->getVictim()))
            {
                // If we are within range melee the target
                // Make sure our attack is ready
                if (m_creature->isAttackReady())
                {
                    printf("---> %s melee is ready %s! \n",m_creature->GetName(), m_creature->getVictim()->GetName());
                    m_creature->AttackerStateUpdate(m_creature->getVictim());

                    m_creature->resetAttackTimer();
                }

            }
            else if (IsSelfRooted)
            {
                // Cancel our current spell and then allow movement again
                m_creature->InterruptNonMeleeSpells(false);
                IsSelfRooted = false;
            }

        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prince_liam_greymane_phase2AI(pCreature);
    }
};


/*######
# rampaging_worgen
######*/


enum {
    SPELL_ENRAGE          = 8599,
    SPELL_ENRAGE_DELAY    = 120000
};

struct rampaging_worgen : public CreatureScript
{
    rampaging_worgen() : CreatureScript("rampaging_worgen") {}

    struct rampaging_worgenAI : public ScriptedAI
    {
        rampaging_worgenAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 spellCooldown;      // This variable acts like the global cooldown that players have (2 minute)
        bool IsSelfRooted;

        void Reset() override
        {
            spellCooldown = 0;
            IsSelfRooted = false;
        }

        void Aggro(Unit* who) override
        {
            if (!m_creature->CanReachWithMeleeAttack(who))
            {
                IsSelfRooted = true;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            // Always decrease our spell cooldown first
            if (spellCooldown > diff)
            {
                spellCooldown -= diff;
            }
            else spellCooldown = 0;

            // Enrage timer (only enrage when we are alive and  in combat)
            if (m_creature->IsInCombat() && m_creature->IsAlive())
            {
                // Cast enrage spell if less than 70% hp ONLY and not casted already
                if (m_creature->GetHealthPercent() < 70.0f && !spellCooldown) {
                {
                    DoCastSpellIfCan(m_creature, SPELL_ENRAGE);
                }
                    spellCooldown = SPELL_ENRAGE_DELAY;
                }

            }

            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Return if we already cast a spell
            if (m_creature->IsNonMeleeSpellCasted(false))
            {
                return;
            }

            // If we are within range melee the target
            if (m_creature->CanReachWithMeleeAttack(m_creature->getVictim()))
            {
                // Make sure our attack is ready
                if (m_creature->isAttackReady())
                {
                    m_creature->AttackerStateUpdate(m_creature->getVictim());

                    m_creature->resetAttackTimer();
                }
            }
            else if (IsSelfRooted)
            {
                // Cancel our current spell and then allow movement again
                m_creature->InterruptNonMeleeSpells(false);
                IsSelfRooted = false;
            }

        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new rampaging_worgenAI(pCreature);
    }
};


/*######
## creature_frightened_citizen_quest
######*/
enum {
    SAY_ON_ESCAPE1             = -1654009,
    SAY_ON_ESCAPE2             = -1654010,
    SAY_ON_ESCAPE3             = -1654011,
    SAY_ON_ESCAPE4             = -1654012,
    SAY_ON_ESCAPE5             = -1654013,
    SAY_ON_ESCAPE6             = -1654014,
    SAY_ON_ESCAPE7             = -1654015,
    SAY_ON_ESCAPE_DELAY        = 12000
};


struct frightened_citizen_quest : public CreatureScript
{
    frightened_citizen_quest() : CreatureScript("frightened_citizen_quest") {}

    struct frightened_citizen_questAI : public ScriptedAI
    {
        frightened_citizen_questAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiSayOnEscapeTimer;
        bool m_bCanSayOnEscape;

        void Reset() override
        {
            m_uiSayOnEscapeTimer = SAY_ON_ESCAPE_DELAY;
            m_bCanSayOnEscape = true;
        }


        void UpdateAI(const uint32 uiDiff) override
        {
            // Ready to say emote
            if (m_bCanSayOnEscape)
            {
                m_uiSayOnEscapeTimer = SAY_ON_ESCAPE_DELAY;

                // switch between 7 texts randomly
                switch (urand(0, 6))
                {
                    case 0: DoScriptText(SAY_ON_ESCAPE1, m_creature); break;
                    case 1: DoScriptText(SAY_ON_ESCAPE2, m_creature); break;
                    case 2: DoScriptText(SAY_ON_ESCAPE3, m_creature); break;
                    case 3: DoScriptText(SAY_ON_ESCAPE4, m_creature); break;
                    case 4: DoScriptText(SAY_ON_ESCAPE5, m_creature); break;
                    case 5: DoScriptText(SAY_ON_ESCAPE6, m_creature); break;
                    case 6: DoScriptText(SAY_ON_ESCAPE7, m_creature); break;
                }

                m_bCanSayOnEscape = false;
            }

            if (m_uiSayOnEscapeTimer < uiDiff)
            {
                m_bCanSayOnEscape = true;
                m_uiSayOnEscapeTimer = SAY_ON_ESCAPE_DELAY;
            }
            else m_uiSayOnEscapeTimer -= uiDiff;

        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new frightened_citizen_questAI(pCreature);
    }
};

void AddSC_gilneas_city()
{
    Script* s;
    s = new npc_prince_liam_greymane_phase1();
    s->RegisterSelf();
    s = new npc_prince_liam_greymane_phase2();
    s->RegisterSelf();
    s = new rampaging_worgen();
    s->RegisterSelf();
    s = new frightened_citizen_quest();
    s->RegisterSelf();
}
