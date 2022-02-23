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
 * SDName:      Boss_Noxxion
 * SD%Complete: 100
 * SDComment:   None
 * SDCategory:  Maraudon
 * EndScriptData
 */

#include "precompiled.h"

enum
{
    SPELL_TOXICVOLLEY           = 21687,
    SPELL_UPPERCUT              = 22916,
    SPELL_NOXXION_SPAWNS_AURA   = 21708,
    SPELL_NOXXION_SPAWNS_SUMMON = 21707,
};

struct boss_noxxion : public CreatureScript
{
    boss_noxxion() : CreatureScript("boss_noxxion") {}

    struct boss_noxxionAI : public ScriptedAI
    {
        boss_noxxionAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiToxicVolleyTimer;
        uint32 m_uiUppercutTimer;
        uint32 m_uiSummonTimer;

        void Reset() override
        {
            m_uiToxicVolleyTimer = 7000;
            m_uiUppercutTimer = 16000;
            m_uiSummonTimer = 19000;
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                pSummoned->AI()->AttackStart(pTarget);
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_uiToxicVolleyTimer < diff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_TOXICVOLLEY) == CAST_OK)
                {
                    m_uiToxicVolleyTimer = 9000;
                }
            }
            else
            {
                m_uiToxicVolleyTimer -= diff;
            }

            if (m_uiUppercutTimer < diff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_UPPERCUT) == CAST_OK)
                {
                    m_uiUppercutTimer = 12000;
                }
            }
            else
            {
                m_uiUppercutTimer -= diff;
            }

            if (m_uiSummonTimer < diff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_NOXXION_SPAWNS_AURA) == CAST_OK)
                {
                    m_uiSummonTimer = 40000;
                }
            }
            else
            {
                m_uiSummonTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_noxxionAI(pCreature);
    }
};

struct aura_dummy_noxxion_spawns : public AuraScript
{
    aura_dummy_noxxion_spawns() : AuraScript("aura_dummy_noxxion_spawns") {}

    bool OnDummyApply(const Aura* pAura, bool bApply) override
    {
        if (pAura->GetId() == SPELL_NOXXION_SPAWNS_AURA && pAura->GetEffIndex() == EFFECT_INDEX_0)
        {
            if (Creature* pTarget = (Creature*)pAura->GetTarget())
            {
                if (bApply)
                {
                    pTarget->CastSpell(pTarget, SPELL_NOXXION_SPAWNS_SUMMON, true);
                    pTarget->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
                else
                {
                    pTarget->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
            }
        }
        return true;
    }
};

void AddSC_boss_noxxion()
{
    Script* s;
    s = new boss_noxxion();
    s->RegisterSelf();
    s = new aura_dummy_noxxion_spawns();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_noxxion";
    //pNewScript->GetAI = &GetAI_boss_noxxion;
    //pNewScript->pEffectAuraDummy = &EffectAuraDummy_spell_aura_dummy_noxxion_spawns;
    //pNewScript->RegisterSelf();
}
