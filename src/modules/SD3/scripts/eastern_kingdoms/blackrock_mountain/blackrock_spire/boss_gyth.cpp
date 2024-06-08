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
 * SDName:      Boss_Gyth
 * SD%Complete: 100
 * SDComment:   Timers may need adjustments
 * SDCategory:  Blackrock Spire
 * EndScriptData
 */

#include "precompiled.h"
#include "blackrock_spire.h"

enum
{
    SAY_NEFARIUS_BUFF_GYTH  = -1229017,
    EMOTE_KNOCKED_OFF       = -1229019,

    SPELL_CHROMATIC_CHAOS   = 16337,                // casted by Nefarius at 50%
    SPELL_REND_MOUNTS       = 16167,
    SPELL_SUMMON_REND       = 16328,
    SPELL_CORROSIVE_ACID    = 16359,
    SPELL_FREEZE            = 16350,
    SPELL_FLAME_BREATH      = 16390,
    SPELL_KNOCK_AWAY        = 10101,
};

struct boss_gyth : public CreatureScript
{
    boss_gyth() : CreatureScript("boss_gyth") {}

    struct boss_gythAI : public ScriptedAI
    {
        boss_gythAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        uint32 uiCorrosiveAcidTimer;
        uint32 uiFreezeTimer;
        uint32 uiFlamebreathTimer;

        bool m_bSummonedRend;
        bool m_bHasChromaticChaos;

        void Reset() override
        {
            uiCorrosiveAcidTimer = 8000;
            uiFreezeTimer = 11000;
            uiFlamebreathTimer = 4000;
            m_bSummonedRend = false;
            m_bHasChromaticChaos = false;

            DoCastSpellIfCan(m_creature, SPELL_REND_MOUNTS);
        }

        void JustSummoned(Creature* pSummoned) override
        {
            DoScriptText(EMOTE_KNOCKED_OFF, pSummoned);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Chromatic Chaos at 50%
            if (!m_bHasChromaticChaos && m_creature->GetHealthPercent() < 50.0f)
            {
                if (m_pInstance)
                {
                    if (Creature* pNefarius = m_pInstance->GetSingleCreatureFromStorage(NPC_LORD_VICTOR_NEFARIUS))
                    {
                        pNefarius->CastSpell(m_creature, SPELL_CHROMATIC_CHAOS, true);
                        DoScriptText(SAY_NEFARIUS_BUFF_GYTH, pNefarius);
                        m_bHasChromaticChaos = true;
                    }
                }
            }

            // CorrosiveAcid_Timer
            if (uiCorrosiveAcidTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_CORROSIVE_ACID) == CAST_OK)
                {
                    uiCorrosiveAcidTimer = 7000;
                }
            }
            else
            {
                uiCorrosiveAcidTimer -= uiDiff;
            }

            // Freeze_Timer
            if (uiFreezeTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FREEZE) == CAST_OK)
                {
                    uiFreezeTimer = 16000;
                }
            }
            else
            {
                uiFreezeTimer -= uiDiff;
            }

            // Flamebreath_Timer
            if (uiFlamebreathTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_FLAME_BREATH) == CAST_OK)
                {
                    uiFlamebreathTimer = 10500;
                }
            }
            else
            {
                uiFlamebreathTimer -= uiDiff;
            }

            // Summon Rend
            if (!m_bSummonedRend && m_creature->GetHealthPercent() < 11.0f)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SUMMON_REND) == CAST_OK)
                {
                    m_creature->RemoveAurasDueToSpell(SPELL_REND_MOUNTS);
                    m_bSummonedRend = true;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_gythAI(pCreature);
    }
};

void AddSC_boss_gyth()
{
    Script *s;
    s = new boss_gyth();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_gyth";
    //pNewScript->GetAI = &GetAI_boss_gyth;
    //pNewScript->RegisterSelf();
}
