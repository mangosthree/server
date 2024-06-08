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
 * SDName:      Boss_Archaedas
 * SD%Complete: 100
 * SDComment:   None
 * SDCategory:  Uldaman
 * EndScriptData
 */

#include "precompiled.h"
#include "uldaman.h"

enum
{
    SPELL_ARCHAEDAS_AWAKEN_VISUAL   = 10347,
    SPELL_GROUND_TREMOR             = 6524,

    SPELL_AWAKEN_EARTHEN_GUARDIAN   = 10252,                // awaken all 7076 npcs
    SPELL_AWAKEN_VAULT_WARDER       = 10258,                // awaken 2 npcs 10120
    SPELL_AWAKEN_EARTHEN_DWARF      = 10259,                // awaken random npc 7309 or 7077

    SAY_AGGRO                       = -1070001,
    SAY_AWAKE_GUARDIANS             = -1070002,
    SAY_AWAKE_WARDERS               = -1070003,
    SAY_UNIT_SLAIN                  = -1070004,
    EMOTE_BREAKS_FREE               = -1070005,
};

struct boss_archaedas : public CreatureScript
{
    boss_archaedas() : CreatureScript("boss_archaedas") {}

    struct boss_archaedasAI : public ScriptedAI
    {
        boss_archaedasAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiAwakeningTimer;
        uint32 m_uiAwakeDwarfTimer;
        uint32 m_uiTremorTimer;
        uint8 m_uiSubevent;
        bool m_bDwarvesAwaken;

        uint8 m_uiHpPhaseCheck;

        void Reset() override
        {
            m_uiAwakeningTimer = 1000;
            m_uiSubevent = 0;
            m_uiAwakeDwarfTimer = 10000;
            m_uiTremorTimer = urand(7000, 14000);
            m_bDwarvesAwaken = false;
            m_uiHpPhaseCheck = 1;

            DoCastSpellIfCan(m_creature, SPELL_FREEZE_ANIM);
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_ARCHAEDAS, IN_PROGRESS);
            }
        }

        void KilledUnit(Unit* /*pVictim*/) override
        {
            DoScriptText(SAY_UNIT_SLAIN, m_creature);
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            // open door to vault (handled by instance script)
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_ARCHAEDAS, DONE);
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_ARCHAEDAS, FAIL);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // so many things are based in this script on instance data
            // so if we don't have access to it better do nothing
            if (!m_pInstance)
            {
                return;
            }

            // OOC Intro part triggered by Altar activation
            if (m_pInstance->GetData(TYPE_ARCHAEDAS) == SPECIAL)
            {
                if (m_uiAwakeningTimer <= uiDiff)
                {
                    switch (m_uiSubevent)
                    {
                    case 0:
                        DoCastSpellIfCan(m_creature, SPELL_ARCHAEDAS_AWAKEN_VISUAL);
                        m_uiAwakeningTimer = 2000;
                        break;
                    case 1:
                        DoScriptText(EMOTE_BREAKS_FREE, m_creature);
                        m_uiAwakeningTimer = 3000;
                        break;
                    case 2:
                        DoScriptText(SAY_AGGRO, m_creature);
                        m_creature->RemoveAurasDueToSpell(SPELL_FREEZE_ANIM);
                        m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                        // Attack player
                        if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_pInstance->GetGuid(DATA_EVENT_STARTER)))
                        {
                            AttackStart(pPlayer);
                        }
                        else
                        {
                            EnterEvadeMode();
                        }
                        break;
                    default:
                        break;
                    }

                    ++m_uiSubevent;
                }
                else
                {
                    m_uiAwakeningTimer -= uiDiff;
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Phase switch
            if (m_creature->GetHealthPercent() < 100.0f - 33.4f * (float)m_uiHpPhaseCheck)
            {
                if (DoCastSpellIfCan(m_creature, m_uiHpPhaseCheck == 1 ? SPELL_AWAKEN_EARTHEN_GUARDIAN : SPELL_AWAKEN_VAULT_WARDER) == CAST_OK)
                {
                    DoScriptText(m_uiHpPhaseCheck == 1 ? SAY_AWAKE_GUARDIANS : SAY_AWAKE_WARDERS, m_creature);
                    ++m_uiHpPhaseCheck;
                }
            }

            // Awake random Dwarf
            if (!m_bDwarvesAwaken && m_creature->GetHealthPercent() >= 33.0f)
            {
                if (m_uiAwakeDwarfTimer < uiDiff)
                {
                    m_pInstance->SetData64(TYPE_SIGNAL, m_creature->GetObjectGuid().GetRawValue());
                    if (Creature* pEarthen = m_pInstance->instance->GetCreature(ObjectGuid(m_pInstance->GetData64(TYPE_SIGNAL))))
                    {
                        if (DoCastSpellIfCan(pEarthen, SPELL_AWAKEN_EARTHEN_DWARF) == CAST_OK)
                        {
                            m_uiAwakeDwarfTimer = urand(9000, 12000);
                        }
                    }
                    else
                    {
                        m_bDwarvesAwaken = true;
                    }
                }
                else
                {
                    m_uiAwakeDwarfTimer -= uiDiff;
                }
            }

            if (m_uiTremorTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_GROUND_TREMOR) == CAST_OK)
                {
                    m_uiTremorTimer = urand(8000, 17000);
                }
            }
            else
            {
                m_uiTremorTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_archaedasAI(pCreature);
    }
};

struct spell_npc_vault_warder : public SpellScript
{
    spell_npc_vault_warder() : SpellScript("spell_npc_vault_warder") {}

    bool EffectDummy(Unit* /*pCaster*/, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pCreatureTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        // always check spellid and effectindex
        if (uiSpellId == SPELL_AWAKEN_VAULT_WARDER && uiEffIndex == EFFECT_INDEX_0)
        {
            if (pCreatureTarget->GetEntry() == NPC_VAULT_WARDER)
            {
                pCreatureTarget->ToCreature()->RemoveAurasDueToSpell(SPELL_STONED);

                ScriptedInstance* pInstance = (ScriptedInstance*)pCreatureTarget->ToCreature()->GetInstanceData();
                if (!pInstance)
                {
                    return true;
                }

                if (Creature* pArchaedas = pInstance->GetSingleCreatureFromStorage(NPC_ARCHAEDAS))
                {
                    pCreatureTarget->ToCreature()->AI()->AttackStart(pArchaedas->getVictim());
                }

                return true;
            }
        }

        return false;
    }
};

struct spell_aura_awaken_dwarf : public AuraScript
{
    spell_aura_awaken_dwarf() : AuraScript("spell_aura_awaken_dwarf") {}

    bool OnDummyApply(const Aura* pAura, bool bApply) override
    {
        if (bApply)
        {
            return true;
        }

        if ((pAura->GetId() == SPELL_AWAKEN_EARTHEN_DWARF || pAura->GetId() == SPELL_AWAKEN_EARTHEN_GUARDIAN) && pAura->GetEffIndex() == EFFECT_INDEX_0)
        {
            if (Creature* pTarget = (Creature*)pAura->GetTarget())
            {
                pTarget->RemoveAurasDueToSpell(SPELL_STONED);

                ScriptedInstance* pInstance = (ScriptedInstance*)pTarget->GetInstanceData();
                if (!pInstance)
                {
                    return true;
                }

                if (Creature* pArchaedas = pInstance->GetSingleCreatureFromStorage(NPC_ARCHAEDAS))
                {
                    pTarget->AI()->AttackStart(pArchaedas->getVictim());
                }
            }
        }

        return true;
    }
};

void AddSC_boss_archaedas()
{
    Script *s;
    s = new boss_archaedas();
    s->RegisterSelf();
    s = new spell_npc_vault_warder();
    s->RegisterSelf();
    s = new spell_aura_awaken_dwarf();
    s->RegisterSelf();

    //Script* pNewScript;

    //pNewScript = new Script;
    //pNewScript->Name = "boss_archaedas";
    //pNewScript->GetAI = &GetAI_boss_archaedas;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "mob_archaeras_add";
    //pNewScript->pEffectDummyNPC = &EffectDummyCreature_npc_vault_warder;
    //pNewScript->pEffectAuraDummy = &EffectAuraDummy_spell_aura_dummy_awaken_dwarf;
    //pNewScript->RegisterSelf();
}
