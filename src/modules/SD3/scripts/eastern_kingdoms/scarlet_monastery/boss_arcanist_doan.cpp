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
 * SDName:      Boss_Arcanist_Doan
 * SD%Complete: 100
 * SDComment:   None
 * SDCategory:  Scarlet Monastery
 * EndScriptData
 */

#include "precompiled.h"

enum
{
    SAY_AGGRO                   = -1189019,
    SAY_SPECIALAE               = -1189020,

    SPELL_POLYMORPH             = 13323,
    SPELL_SILENCE               = 8988,
    SPELL_ARCANE_EXPLOSION      = 9433,
    SPELL_DETONATION            = 9435,
    SPELL_ARCANE_BUBBLE         = 9438,
};

struct boss_arcanist_doan : public CreatureScript
{
    boss_arcanist_doan() : CreatureScript("boss_arcanist_doan") {}

    struct boss_arcanist_doanAI : public ScriptedAI
    {
        boss_arcanist_doanAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiPolymorphTimer;
        uint32 m_uiSilenceTimer;
        uint32 m_uiArcaneExplosionTimer;
        uint32 m_uiDetonationTimer;
        bool bShielded;

        void Reset() override
        {
            m_uiPolymorphTimer = 15000;
            m_uiSilenceTimer = 7500;
            m_uiArcaneExplosionTimer = urand(1000, 3000);
            m_uiDetonationTimer = 0;
            bShielded = false;
        }

        void Aggro(Unit* /*pWho*/) override
        {
            DoScriptText(SAY_AGGRO, m_creature);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_uiDetonationTimer)
            {
                if (m_uiDetonationTimer <= uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_DETONATION) == CAST_OK)
                    {
                        DoScriptText(SAY_SPECIALAE, m_creature);
                        m_uiDetonationTimer = 0;
                    }
                }
                else
                {
                    m_uiDetonationTimer -= uiDiff;
                }
            }

            // Do not attack while having the bubble active
            if (m_creature->HasAura(SPELL_ARCANE_BUBBLE))
            {
                return;
            }

            // If we are <50% hp cast Arcane Bubble
            if (!bShielded && m_creature->GetHealthPercent() <= 50.0f)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_ARCANE_BUBBLE) == CAST_OK)
                {
                    m_uiDetonationTimer = 1000;
                    bShielded = true;
                }
            }

            if (m_uiPolymorphTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1))
                {
                    if (DoCastSpellIfCan(pTarget, SPELL_POLYMORPH) == CAST_OK)
                    {
                        m_uiPolymorphTimer = 20000;
                    }
                }
            }
            else
            {
                m_uiPolymorphTimer -= uiDiff;
            }

            // Silence_Timer
            if (m_uiSilenceTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SILENCE) == CAST_OK)
                {
                    m_uiSilenceTimer = urand(15000, 22000);
                }
            }
            else
            {
                m_uiSilenceTimer -= uiDiff;
            }

            // ArcaneExplosion_Timer
            if (m_uiArcaneExplosionTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_ARCANE_EXPLOSION) == CAST_OK)
                {
                    m_uiArcaneExplosionTimer = urand(2500, 8500);
                }
            }
            else
            {
                m_uiArcaneExplosionTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_arcanist_doanAI(pCreature);
    }
};

void AddSC_boss_arcanist_doan()
{
    Script *s;
    s = new boss_arcanist_doan();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_arcanist_doan";
    //pNewScript->GetAI = &GetAI_boss_arcanist_doan;
    //pNewScript->RegisterSelf();
}
