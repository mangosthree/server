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
 * SDName:      Boss_Shazzrah
 * SD%Complete: 75
 * SDComment:   Teleport NYI (need core support, remove hack here when implemented)
 * SDCategory:  Molten Core
 * EndScriptData
 */

#include "precompiled.h"
#include "molten_core.h"

enum
{
    SPELL_ARCANE_EXPLOSION          = 19712,
    SPELL_SHAZZRAH_CURSE            = 19713,
    SPELL_MAGIC_GROUNDING           = 19714,
    SPELL_COUNTERSPELL              = 19715,
    SPELL_GATE_OF_SHAZZRAH          = 23138                 // effect spell: 23139
};

struct boss_shazzrah : public CreatureScript
{
    boss_shazzrah() : CreatureScript("boss_shazzrah") {}

    struct boss_shazzrahAI : public ScriptedAI
    {
        boss_shazzrahAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        }

        ScriptedInstance* m_pInstance;

        uint32 m_uiArcaneExplosionTimer;
        uint32 m_uiShazzrahCurseTimer;
        uint32 m_uiMagicGroundingTimer;
        uint32 m_uiCounterspellTimer;
        uint32 m_uiBlinkTimer;

        void Reset() override
        {
            m_uiArcaneExplosionTimer = 6000;
            m_uiShazzrahCurseTimer = 10000;
            m_uiMagicGroundingTimer = 24000;
            m_uiCounterspellTimer = 15000;
            m_uiBlinkTimer = 30000;
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_SHAZZRAH, IN_PROGRESS);
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_SHAZZRAH, DONE);
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_SHAZZRAH, NOT_STARTED);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Arcane Explosion Timer
            if (m_uiArcaneExplosionTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_ARCANE_EXPLOSION) == CAST_OK)
                {
                    m_uiArcaneExplosionTimer = urand(5000, 9000);
                }
            }
            else
            {
                m_uiArcaneExplosionTimer -= uiDiff;
            }

            // Shazzrah Curse Timer
            if (m_uiShazzrahCurseTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_SHAZZRAH_CURSE) == CAST_OK)
                {
                    m_uiShazzrahCurseTimer = 20000;
                }
            }
            else
            {
                m_uiShazzrahCurseTimer -= uiDiff;
            }

            // Magic Grounding Timer
            if (m_uiMagicGroundingTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_MAGIC_GROUNDING) == CAST_OK)
                {
                    m_uiMagicGroundingTimer = 35000;
                }
            }
            else
            {
                m_uiMagicGroundingTimer -= uiDiff;
            }

            // Counterspell Timer
            if (m_uiCounterspellTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_COUNTERSPELL) == CAST_OK)
                {
                    m_uiCounterspellTimer = urand(16000, 20000);
                }
            }
            else
            {
                m_uiCounterspellTimer -= uiDiff;
            }

            // Blink Timer
            if (m_uiBlinkTimer < uiDiff)
            {
                // Teleporting him to a random gamer and casting Arcane Explosion after that.
                if (DoCastSpellIfCan(m_creature, SPELL_GATE_OF_SHAZZRAH) == CAST_OK)
                {
                    // manual, until added effect of dummy properly -- TODO REMOVE HACK
                    if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    {
                        m_creature->NearTeleportTo(pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), m_creature->GetOrientation());
                    }
                    DoResetThreat();

                    DoCastSpellIfCan(m_creature, SPELL_ARCANE_EXPLOSION, CAST_TRIGGERED);

                    m_uiBlinkTimer = 45000;
                }
            }
            else
            {
                m_uiBlinkTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_shazzrahAI(pCreature);
    }
};

void AddSC_boss_shazzrah()
{
    Script* s;
    s = new boss_shazzrah();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_shazzrah";
    //pNewScript->GetAI = &GetAI_boss_shazzrah;
    //pNewScript->RegisterSelf();
}
