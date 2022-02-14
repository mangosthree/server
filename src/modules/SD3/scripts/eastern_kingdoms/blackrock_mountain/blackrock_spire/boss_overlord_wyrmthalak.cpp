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
 * SDName:      Boss_Overlord_Wyrmthalak
 * SD%Complete: 100
 * SDComment:   None
 * SDCategory:  Blackrock Spire
 * EndScriptData
 */

#include "precompiled.h"

enum
{
    SPELL_BLASTWAVE            = 11130,
    SPELL_SHOUT                = 23511,
    SPELL_CLEAVE               = 20691,
    SPELL_KNOCKAWAY            = 20686,

    NPC_SPIRESTONE_WARLORD     = 9216,
    NPC_SMOLDERTHORN_BERSERKER = 9268,
    NPC_BLOODAXE_VETERAN       = 9583,

    // Overlord emote calls for help
    SAY_EMOTE                  = -1999926
};

const float afLocations[4][4] =
{
    { -51.6805f, -439.831f, 78.2874f, 4.657f},              // spawn points
    { -54.4554f, -439.679f, 78.2884f, 4.657f},
    { -39.355381f, -513.456482f, 88.472046f, 4.679872f},    // destination
    { -49.875881f, -511.896942f, 88.195160f, 4.613114f}
};

const uint32 uSummons[3] = {NPC_BLOODAXE_VETERAN, NPC_SMOLDERTHORN_BERSERKER, NPC_SPIRESTONE_WARLORD};

struct boss_overlordwyrmthalak : public CreatureScript
{
    boss_overlordwyrmthalak() : CreatureScript("boss_overlord_wyrmthalak") {}

    struct boss_overlordwyrmthalakAI : public ScriptedAI
    {
        boss_overlordwyrmthalakAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiBlastWaveTimer;
        uint32 m_uiShoutTimer;
        uint32 m_uiCleaveTimer;
        uint32 m_uiKnockawayTimer;
        bool m_bSummoned;

        void Reset() override
        {
            m_uiBlastWaveTimer = 20000;
            m_uiShoutTimer = 2000;
            m_uiCleaveTimer = 6000;
            m_uiKnockawayTimer = 12000;
            m_bSummoned = false;
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() != NPC_SPIRESTONE_WARLORD && pSummoned->GetEntry() != NPC_SMOLDERTHORN_BERSERKER)
            {
                return;
            }

            if (m_creature->getVictim())
            {
                Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0);
                pSummoned->AI()->AttackStart(pTarget ? pTarget : m_creature->getVictim());
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // BlastWave
            if (m_uiBlastWaveTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature, SPELL_BLASTWAVE);
                m_uiBlastWaveTimer = 20000;
            }
            else
            {
                m_uiBlastWaveTimer -= uiDiff;
            }

            // Shout
            if (m_uiShoutTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature, SPELL_SHOUT);
                m_uiShoutTimer = 10000;
            }
            else
            {
                m_uiShoutTimer -= uiDiff;
            }

            // Cleave
            if (m_uiCleaveTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE);
                m_uiCleaveTimer = 7000;
            }
            else
            {
                m_uiCleaveTimer -= uiDiff;
            }

            // Knockaway
            if (m_uiKnockawayTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature, SPELL_KNOCKAWAY);
                m_uiKnockawayTimer = 14000;
            }
            else
            {
                m_uiKnockawayTimer -= uiDiff;
            }

            // Summon two Beserks
            if (!m_bSummoned && m_creature->GetHealthPercent() < 51.0f)
            {
                DoScriptText(SAY_EMOTE, m_creature); // Calls for help
                Creature* pGuard1 = m_creature->SummonCreature(uSummons[urand(0,2)], afLocations[0][0], afLocations[0][1], afLocations[0][2], afLocations[0][3], TEMPSPAWN_TIMED_DESPAWN, 300000);
                Creature* pGuard2 = m_creature->SummonCreature(uSummons[urand(0,2)], afLocations[1][0], afLocations[1][1], afLocations[1][2], afLocations[1][3], TEMPSPAWN_TIMED_DESPAWN, 300000);
                if (pGuard1)
                {
                    pGuard1->SetWalk(false);
                    pGuard1->GetMotionMaster()->MovePoint(0, afLocations[2][0], afLocations[2][1], afLocations[2][2]);
                }
                if (pGuard2)
                {
                    pGuard1->SetWalk(false);
                    pGuard1->GetMotionMaster()->MovePoint(0, afLocations[3][0], afLocations[3][1], afLocations[3][2]);
                }

                m_bSummoned = true;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_overlordwyrmthalakAI(pCreature);
    }
};

void AddSC_boss_overlordwyrmthalak()
{
    Script *s;
    s = new boss_overlordwyrmthalak();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_overlord_wyrmthalak";
    //pNewScript->GetAI = &GetAI_boss_overlordwyrmthalak;
    //pNewScript->RegisterSelf();
}
