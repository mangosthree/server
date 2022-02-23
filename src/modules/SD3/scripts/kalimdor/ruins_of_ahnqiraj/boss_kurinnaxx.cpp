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
 * SDName:      Boss_Kurinnaxx
 * SD%Complete: 90
 * SDComment:   Summon Player ability NYI
 * SDCategory:  Ruins of Ahn'Qiraj
 * EndScriptData
 */

#include "precompiled.h"

enum
{
    SPELL_TRASH             = 3391,
    SPELL_WIDE_SLASH        = 25814,
    SPELL_MORTAL_WOUND      = 25646,
    SPELL_SANDTRAP          = 25648,        // summons gameobject 180647
    SPELL_ENRAGE            = 26527,
    SPELL_SUMMON_PLAYER     = 26446,

    GO_SAND_TRAP            = 180647,
};

struct boss_kurinnaxx : public CreatureScript
{
    boss_kurinnaxx() : CreatureScript("boss_kurinnaxx") {}

    struct boss_kurinnaxxAI : public ScriptedAI
    {
        boss_kurinnaxxAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiMortalWoundTimer;
        uint32 m_uiSandTrapTimer;
        uint32 m_uiTrashTimer;
        uint32 m_uiWideSlashTimer;
        uint32 m_uiTrapTriggerTimer;
        bool m_bEnraged;

        ObjectGuid m_sandtrapGuid;

        void Reset() override
        {
            m_bEnraged = false;

            m_uiMortalWoundTimer = urand(8000, 10000);
            m_uiSandTrapTimer = urand(5000, 10000);
            m_uiTrashTimer = urand(1000, 5000);
            m_uiWideSlashTimer = urand(10000, 15000);
            m_uiTrapTriggerTimer = 0;
        }

        void JustSummoned(GameObject* pGo) override
        {
            if (pGo->GetEntry() == GO_SAND_TRAP)
            {
                m_uiTrapTriggerTimer = 3000;
                m_sandtrapGuid = pGo->GetObjectGuid();
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // If we are belowe 30% HP cast enrage
            if (!m_bEnraged && m_creature->GetHealthPercent() <= 30.0f)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK)
                {
                    m_bEnraged = true;
                }
            }

            // Mortal Wound
            if (m_uiMortalWoundTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_MORTAL_WOUND) == CAST_OK)
                {
                    m_uiMortalWoundTimer = urand(8000, 10000);
                }
            }
            else
            {
                m_uiMortalWoundTimer -= uiDiff;
            }

            // Sand Trap
            if (m_uiSandTrapTimer < uiDiff)
            {
                Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 1);
                if (!pTarget)
                {
                    pTarget = m_creature->getVictim();
                }

                pTarget->CastSpell(pTarget, SPELL_SANDTRAP, true, nullptr, nullptr, m_creature->GetObjectGuid());
                m_uiSandTrapTimer = urand(10000, 15000);
            }
            else
            {
                m_uiSandTrapTimer -= uiDiff;
            }

            // Trigger the sand trap in 3 secs after spawn
            if (m_uiTrapTriggerTimer)
            {
                if (m_uiTrapTriggerTimer <= uiDiff)
                {
                    if (GameObject* pTrap = m_creature->GetMap()->GetGameObject(m_sandtrapGuid))
                    {
                        pTrap->Use(m_creature);
                    }
                    m_uiTrapTriggerTimer = 0;
                }
                else
                {
                    m_uiTrapTriggerTimer -= uiDiff;
                }
            }

            // Wide Slash
            if (m_uiWideSlashTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_WIDE_SLASH) == CAST_OK)
                {
                    m_uiWideSlashTimer = urand(12000, 15000);
                }
            }
            else
            {
                m_uiWideSlashTimer -= uiDiff;
            }

            // Trash
            if (m_uiTrashTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_TRASH) == CAST_OK)
                {
                    m_uiTrashTimer = urand(12000, 17000);
                }
            }
            else
            {
                m_uiTrashTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_kurinnaxxAI(pCreature);
    }
};

void AddSC_boss_kurinnaxx()
{
    Script* s;
    s = new boss_kurinnaxx();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_kurinnaxx";
    //pNewScript->GetAI = &GetAI_boss_kurinnaxx;
    //pNewScript->RegisterSelf();
}
