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
 * SDName:      Boss_Mr_Smite
 * SD%Complete: 100
 * SDComment:   None
 * SDCategory:  Deadmines
 * EndScriptData
 */

#include "precompiled.h"
#include "deadmines.h"

enum
{
    SAY_PHASE_2                     = -1036002,
    SAY_PHASE_3                     = -1036003,

    EQUIP_ID_SWORD                  = 2179,                 // default equipment
    EQUIP_ID_AXE                    = 2183,
    EQUIP_ID_HAMMER                 = 10756,

    SPELL_NIBLE_REFLEXES            = 6433,                 // removed after phase 1
    SPELL_SMITE_SLAM                = 6435,                 // only casted in phase 3
    SPELL_SMITE_STOMP               = 6432,
    SPELL_SMITE_HAMMER              = 6436,                 // unclear, not casted in sniff
    SPELL_THRASH                    = 12787,                // only casted in phase 2; unclear, 3391 directly casted in sniff

    PHASE_1                         = 1,
    PHASE_2                         = 2,
    PHASE_3                         = 3,
    PHASE_EQUIP_NULL                = 4,
    PHASE_EQUIP_START               = 5,
    PHASE_EQUIP_PROCESS             = 6,
    PHASE_EQUIP_END                 = 7,
};

struct boss_mr_smite : public CreatureScript
{
    boss_mr_smite() : CreatureScript("boss_mr_smite") {}

    struct boss_mr_smiteAI : public ScriptedAI
    {
        boss_mr_smiteAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiPhase;
        uint32 m_uiEquipTimer;
        uint32 m_uiSlamTimer;

        void Reset() override
        {
            m_uiPhase = PHASE_1;
            m_uiEquipTimer = 0;
            m_uiSlamTimer = 9000;

            DoCastSpellIfCan(m_creature, SPELL_NIBLE_REFLEXES, CAST_TRIGGERED);

            // must assume database has the default equipment set
            SetEquipmentSlots(false, EQUIP_ID_SWORD, EQUIP_UNEQUIP);
        }

        void AttackedBy(Unit* pAttacker) override
        {
            if (m_creature->getVictim())
            {
                return;
            }

            if (m_uiPhase > PHASE_3)
            {
                return;
            }

            AttackStart(pAttacker);
        }

        void AttackStart(Unit* pWho) override
        {
            if (m_uiPhase > PHASE_3)
            {
                return;
            }

            if (m_creature->Attack(pWho, true))
            {
                m_creature->AddThreat(pWho);
                m_creature->SetInCombatWith(pWho);
                pWho->SetInCombatWith(m_creature);

                m_creature->GetMotionMaster()->MoveChase(pWho);
            }
        }

        void MovementInform(uint32 uiMotionType, uint32 /*uiPointId*/) override
        {
            if (uiMotionType != POINT_MOTION_TYPE)
            {
                return;
            }

            m_creature->SetSheath(SHEATH_STATE_UNARMED);
            m_creature->SetStandState(UNIT_STAND_STATE_KNEEL);

            m_uiEquipTimer = 3000;
            m_uiPhase = PHASE_EQUIP_PROCESS;
        }

        void PhaseEquipStart()
        {
            ScriptedInstance* pInstance = (ScriptedInstance*)m_creature->GetInstanceData();

            if (!pInstance)
            {
                return;
            }

            GameObject* pChest = pInstance->GetSingleGameObjectFromStorage(GO_SMITE_CHEST);

            if (!pChest)
            {
                return;
            }

            m_uiPhase = PHASE_EQUIP_NULL;

            float fX, fY, fZ;
            pChest->GetContactPoint(m_creature, fX, fY, fZ, CONTACT_DISTANCE);

            m_creature->GetMotionMaster()->Clear();
            m_creature->SetFacingToObject(pChest);
            m_creature->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
        }

        void PhaseEquipProcess()
        {
            if (m_creature->GetHealthPercent() < 33.0f)
            {
                // It's Hammer, go Hammer!
                SetEquipmentSlots(false, EQUIP_ID_HAMMER, EQUIP_UNEQUIP);
                DoCastSpellIfCan(m_creature, SPELL_SMITE_HAMMER);
            }
            else
            {
                SetEquipmentSlots(false, EQUIP_ID_AXE, EQUIP_ID_AXE);
            }

            m_creature->SetStandState(UNIT_STAND_STATE_STAND);
            m_uiPhase = PHASE_EQUIP_END;
            m_uiEquipTimer = 1000;
        }

        void PhaseEquipEnd()
        {
            // We don't have getVictim, so select from threat list
            Unit* pVictim = m_creature->SelectAttackingTarget(ATTACKING_TARGET_TOPAGGRO, 0);

            if (!pVictim)
            {
                EnterEvadeMode();
                return;
            }

            m_creature->SetSheath(SHEATH_STATE_MELEE);

            m_uiPhase = m_creature->GetHealthPercent() < 33.0f ? PHASE_3 : PHASE_2;

            if (m_uiPhase == PHASE_2)
            {
                DoCastSpellIfCan(m_creature, SPELL_THRASH, CAST_TRIGGERED);
            }

            AttackStart(pVictim);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                if (m_uiEquipTimer)
                {
                    // decrease the cooldown in between equipment change phases
                    if (m_uiEquipTimer > uiDiff)
                    {
                        m_uiEquipTimer -= uiDiff;
                        return;
                    }
                    else
                    {
                        m_uiEquipTimer = 0;
                    }
                }

                switch (m_uiPhase)
                {
                case PHASE_EQUIP_START:
                    PhaseEquipStart();
                    break;
                case PHASE_EQUIP_PROCESS:
                    PhaseEquipProcess();
                    break;
                case PHASE_EQUIP_END:
                    PhaseEquipEnd();
                    break;
                }

                return;
            }

            // the normal combat phases
            switch (m_uiPhase)
            {
            case PHASE_1:
                if (m_creature->GetHealthPercent() < 66.0f)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_SMITE_STOMP) == CAST_OK)
                    {
                        DoScriptText(m_creature->GetHealthPercent() < 33.0f ? SAY_PHASE_3 : SAY_PHASE_2, m_creature);
                        m_uiPhase = PHASE_EQUIP_START;
                        m_uiEquipTimer = 2500;

                        // will clear getVictim (m_attacking)
                        m_creature->AttackStop(true);
                        m_creature->RemoveAurasDueToSpell(SPELL_NIBLE_REFLEXES);
                    }
                    return;
                }
                break;
            case PHASE_2:
                if (m_creature->GetHealthPercent() < 33.0f)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_SMITE_STOMP) == CAST_OK)
                    {
                        DoScriptText(SAY_PHASE_3, m_creature);
                        m_uiPhase = PHASE_EQUIP_START;
                        m_uiEquipTimer = 2500;

                        // will clear getVictim (m_attacking)
                        m_creature->AttackStop(true);
                        m_creature->RemoveAurasDueToSpell(SPELL_THRASH);
                    }
                    return;
                }
                break;
            case PHASE_3:
                if (m_uiSlamTimer < uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SMITE_SLAM) == CAST_OK)
                    {
                        m_uiSlamTimer = 11000;
                    }
                }
                else
                {
                    m_uiSlamTimer -= uiDiff;
                }
                break;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_mr_smiteAI(pCreature);
    }
};

void AddSC_boss_mr_smite()
{
    Script *s;
    s = new boss_mr_smite();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_mr_smite";
    //pNewScript->GetAI = &GetAI_boss_mr_smite;
    //pNewScript->RegisterSelf();
}
