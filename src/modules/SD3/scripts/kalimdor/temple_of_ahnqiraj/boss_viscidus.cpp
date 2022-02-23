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
 * SD%Complete: 90
 * SDComment: Server side spells implementation need to be checked.
 * SDComment:   Only basic spells - freeze and explode events require more research
 * SDCategory:  Temple of Ahn'Qiraj
 * EndScriptData
 */

#include "precompiled.h"
#include "temple_of_ahnqiraj.h"

enum
{
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    // Emotes
    EMOTE_SLOW                  = -1531041,
    EMOTE_FREEZE                = -1531042,
    EMOTE_FROZEN                = -1531043,
    EMOTE_CRACK                 = -1531044,
    EMOTE_SHATTER               = -1531045,
    EMOTE_EXPLODE               = -1531046,
#endif

    // Timer spells
    SPELL_POISON_SHOCK          = 25993,
    SPELL_POISONBOLT_VOLLEY     = 25991,
    SPELL_TOXIN                 = 26575,                    // Triggers toxin cloud - 25989

    // Debuffs gained by the boss on frost damage
    SPELL_VISCIDUS_SLOWED       = 26034,
    SPELL_VISCIDUS_SLOWED_MORE  = 26036,
    SPELL_VISCIDUS_FREEZE       = 25937,

    // When frost damage exceeds a certain limit, then boss explodes
    SPELL_REJOIN_VISCIDUS       = 25896,
    SPELL_VISCIDUS_EXPLODE      = 25938,
    SPELL_VISCIDUS_SUICIDE      = 26003,                    // cast when boss explodes and is below 5% Hp - should trigger 26002
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    SPELL_DESPAWN_GLOBS         = 26608,
#endif

#if defined (CLASSIC)
    SPELL_MEMBRANE_VISCIDUS     = 25994,                    // damage reduction spell
#endif
    // SPELL_VISCIDUS_WEAKNESS   = 25926,                   // aura which procs at damage - should trigger the slow spells
    // SPELL_VISCIDUS_SHRINKS    = 25893,                   // removed from DBC
    // SPELL_VISCIDUS_SHRINKS_2  = 27934,                   // removed from DBC
    // SPELL_VISCIDUS_GROWS      = 25897,                   // removed from DBC
    // SPELL_SUMMON_GLOBS        = 25885,                   // summons npc 15667 using spells from 25865 to 25884; All spells have target coords - removed from DBC
    // SPELL_VISCIDUS_TELEPORT   = 25904,                   // removed from DBC
    // SPELL_SUMMONT_TRIGGER     = 26564,                   // summons 15992 - removed from DBC

#if defined (CLASSIC) || defined (TBC)
    NPC_GLOB_OF_VISCIDUS        = 15667
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    NPC_GLOB_OF_VISCIDUS        = 15667,
    NPC_VISCIDUS_TRIGGER      = 15922,                      // handles aura 26575

    MAX_VISCIDUS_GLOBS          = 20,                       // there are 20 summoned globs; each glob = 5% hp

    // hitcounts
    HITCOUNT_SLOW               = 100,
    HITCOUNT_SLOW_MORE          = 150,
    HITCOUNT_FREEZE             = 200,
    HITCOUNT_CRACK              = 50,
    HITCOUNT_SHATTER            = 100,
    HITCOUNT_EXPLODE            = 150,

    // phases
    PHASE_NORMAL                = 1,
    PHASE_FROZEN                = 2,
    PHASE_EXPLODED              = 3,
#endif
};

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
static const uint32 auiGlobSummonSpells[MAX_VISCIDUS_GLOBS] = { 25865, 25866, 25867, 25868, 25869, 25870, 25871, 25872, 25873, 25874, 25875, 25876, 25877, 25878, 25879, 25880, 25881, 25882, 25883, 25884 };
#endif


struct boss_viscidus : public CreatureScript
{
    boss_viscidus() : CreatureScript("boss_viscidus") {}

    struct boss_viscidusAI : public ScriptedAI
    {
        boss_viscidusAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
            Reset();
        }

        ScriptedInstance* m_pInstance;

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
        uint8 m_uiPhase;

        uint32 m_uiHitCount;
        uint32 m_uiToxinTimer;
        uint32 m_uiExplodeDelayTimer;
#endif
        uint32 m_uiPoisonShockTimer;
        uint32 m_uiPoisonBoltVolleyTimer;

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
        GuidList m_lGlobesGuidList;
#endif

        void Reset() override
        {
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            m_uiPhase                 = PHASE_NORMAL;
            m_uiHitCount              = 0;

            m_uiExplodeDelayTimer     = 0;
            m_uiToxinTimer            = 30000;
#endif
            m_uiPoisonShockTimer = urand(7000, 12000);
            m_uiPoisonBoltVolleyTimer = urand(10000, 15000);

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            SetCombatMovement(true);
            m_creature->SetVisibility(VISIBILITY_ON);
            m_creature->SetStandState(UNIT_STAND_STATE_STAND);
            m_creature->SetObjectScale(DEFAULT_OBJECT_SCALE);
#endif
#if defined (CLASSIC)
            DoCastSpellIfCan(m_creature, SPELL_MEMBRANE_VISCIDUS);
#endif
        }

        void Aggro(Unit* /*pWho*/) override
        {
            DoCastSpellIfCan(m_creature, SPELL_TOXIN);

            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_VISCIDUS, IN_PROGRESS);
            }
        }

        void JustReachedHome() override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_VISCIDUS, FAIL);

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
                DoCastSpellIfCan(m_creature, SPELL_DESPAWN_GLOBS, CAST_TRIGGERED);
#endif
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_VISCIDUS, DONE);
            }
        }

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_GLOB_OF_VISCIDUS)
            {
                float fX, fY, fZ;
                m_creature->GetRespawnCoord(fX, fY, fZ);
                pSummoned->GetMotionMaster()->MovePoint(1, fX, fY, fZ);
                m_lGlobesGuidList.push_back(pSummoned->GetObjectGuid());
            }
            else if (pSummoned->GetEntry() == NPC_VISCIDUS_TRIGGER)
            {
                pSummoned->CastSpell(pSummoned, SPELL_TOXIN, true);
            }
        }

        void SummonedCreatureJustDied(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_GLOB_OF_VISCIDUS)
            {
                // shrink - modify scale
                m_creature->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, float(-4), true);
                m_creature->UpdateModelData();
                m_creature->SetHealth(m_creature->GetHealth() - (m_creature->GetMaxHealth() * 0.05f));
                m_lGlobesGuidList.remove(pSummoned->GetObjectGuid());

                // suicide if required
                if (m_creature->GetHealthPercent() < 5.0f)
                {
                    m_creature->SetVisibility(VISIBILITY_ON);

                    if (DoCastSpellIfCan(m_creature, SPELL_VISCIDUS_SUICIDE, CAST_TRIGGERED) == CAST_OK)
                    {
                        m_creature->DealDamage(m_creature, m_creature->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NONE, nullptr, false);
                    }
                }
                else if (m_lGlobesGuidList.empty())
                {
                    m_creature->SetVisibility(VISIBILITY_ON);
                    m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                    m_uiPhase = PHASE_NORMAL;

                    SetCombatMovement(true);
                    DoStartMovement(m_creature->getVictim());
                }
            }
        }

        void SummonedMovementInform(Creature* pSummoned, uint32 uiType, uint32 uiPointId) override
        {
            if (pSummoned->GetEntry() != NPC_GLOB_OF_VISCIDUS || uiType != POINT_MOTION_TYPE || !uiPointId)
            {
                return;
            }

            m_lGlobesGuidList.remove(pSummoned->GetObjectGuid());
            pSummoned->CastSpell(m_creature, SPELL_REJOIN_VISCIDUS, true);
            pSummoned->ForcedDespawn(1000);

            if (m_lGlobesGuidList.empty())
            {
                m_creature->SetVisibility(VISIBILITY_ON);
                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                m_uiPhase = PHASE_NORMAL;

                SetCombatMovement(true);
                DoStartMovement(m_creature->getVictim());
            }
        }

        void DamageTaken(Unit* pDealer, uint32& uiDamage) override
        {
            // apply missing aura: 50% damage reduction;
            uiDamage = uiDamage * 0.5f;

            if (m_uiPhase != PHASE_FROZEN)
            {
                return;
            }

            ++m_uiHitCount;

            // only count melee attacks
            if (pDealer->hasUnitState(UNIT_STAT_MELEE_ATTACKING) && m_uiHitCount >= HITCOUNT_EXPLODE)
            {
                if (m_creature->GetHealthPercent() <= 5.0f)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_VISCIDUS_SUICIDE, CAST_TRIGGERED) == CAST_OK)
                    {
                        m_creature->DealDamage(m_creature, m_creature->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NONE, nullptr, false);
                    }
                }
                else if (DoCastSpellIfCan(m_creature, SPELL_VISCIDUS_EXPLODE, CAST_TRIGGERED | CAST_INTERRUPT_PREVIOUS) == CAST_OK)
                {
                    DoScriptText(EMOTE_EXPLODE, m_creature);
                    m_uiPhase = PHASE_EXPLODED;
                    m_uiHitCount = 0;
                    m_lGlobesGuidList.clear();
                    uint32 uiGlobeCount = m_creature->GetHealthPercent() / 5.0f;

                    for (uint8 i = 0; i < uiGlobeCount; ++i)
                    {
                        DoCastSpellIfCan(m_creature, auiGlobSummonSpells[i], CAST_TRIGGERED);
                    }

                    m_creature->RemoveAurasDueToSpell(SPELL_VISCIDUS_FREEZE);
                    m_uiExplodeDelayTimer = 2000;

                    SetCombatMovement(false);
                    m_creature->GetMotionMaster()->MoveIdle();
                    m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
                }
            }
            else if (m_uiHitCount == HITCOUNT_SHATTER)
            {
                DoScriptText(EMOTE_SHATTER, m_creature);
            }
            else if (m_uiHitCount == HITCOUNT_CRACK)
            {
                DoScriptText(EMOTE_CRACK, m_creature);
            }
        }

        void SpellHit(Unit* /*pCaster*/, const SpellEntry* pSpell) override
        {
            if (m_uiPhase != PHASE_NORMAL)
            {
                return;
            }

            // only count frost damage
#if defined(MISTS)
            if (pSpell->GetSchoolMask() == SPELL_SCHOOL_MASK_FROST)
#else
            if (pSpell->SchoolMask == SPELL_SCHOOL_MASK_FROST)
#endif
            {
                ++m_uiHitCount;

                if (m_uiHitCount >= HITCOUNT_FREEZE)
                {
                    m_uiPhase = PHASE_FROZEN;
                    m_uiHitCount = 0;

                    DoScriptText(EMOTE_FROZEN, m_creature);
                    m_creature->RemoveAurasDueToSpell(SPELL_VISCIDUS_SLOWED_MORE);
                    DoCastSpellIfCan(m_creature, SPELL_VISCIDUS_FREEZE, CAST_TRIGGERED);
                }
                else if (m_uiHitCount >= HITCOUNT_SLOW_MORE)
                {
                    DoScriptText(EMOTE_FREEZE, m_creature);
                    m_creature->RemoveAurasDueToSpell(SPELL_VISCIDUS_SLOWED);
                    DoCastSpellIfCan(m_creature, SPELL_VISCIDUS_SLOWED_MORE, CAST_TRIGGERED);
                }
                else if (m_uiHitCount >= HITCOUNT_SLOW)
                {
                    DoScriptText(EMOTE_SLOW, m_creature);
                    DoCastSpellIfCan(m_creature, SPELL_VISCIDUS_SLOWED, CAST_TRIGGERED);
                }
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
        {
            if (eventType == AI_EVENT_CUSTOM_A)
            {
                if (m_uiPhase == PHASE_EXPLODED)
                {
                    return;
                }

                // reset phase if not already exploded
                m_uiPhase = PHASE_NORMAL;
                m_uiHitCount = 0;
            }
        }
#endif

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            if (m_uiExplodeDelayTimer)
            {
                if (m_uiExplodeDelayTimer <= uiDiff)
                {
                    // Make invisible
                    m_creature->SetVisibility(VISIBILITY_OFF);

                    // Teleport to room center
                    float fX, fY, fZ, fO;
                    m_creature->GetRespawnCoord(fX, fY, fZ, &fO);
                    m_creature->NearTeleportTo(fX, fY, fZ, fO);
                    m_uiExplodeDelayTimer = 0;
                }
                else
                {
                    m_uiExplodeDelayTimer -= uiDiff;
                }
            }

            if (m_uiPhase != PHASE_NORMAL)
            {
                return;
            }
#endif

            if (m_uiPoisonShockTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_POISON_SHOCK) == CAST_OK)
                {
                    m_uiPoisonShockTimer = urand(7000, 12000);
                }
            }
            else
            {
                m_uiPoisonShockTimer -= uiDiff;
            }

            if (m_uiPoisonBoltVolleyTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_POISONBOLT_VOLLEY) == CAST_OK)
                {
                    m_uiPoisonBoltVolleyTimer = urand(10000, 15000);
                }
            }
            else
            {
                m_uiPoisonBoltVolleyTimer -= uiDiff;
            }

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            if (m_uiToxinTimer < uiDiff)
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    m_creature->SummonCreature(NPC_VISCIDUS_TRIGGER, pTarget->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ(), 0, TEMPSPAWN_DEAD_DESPAWN, 0);
                }
                m_uiToxinTimer = 30000;
            }
            else
            {
                m_uiToxinTimer -= uiDiff;
            }
#endif
            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_viscidusAI(pCreature);
    }
};

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
struct aura_viscidus_freeze : public AuraScript
{
    aura_viscidus_freeze() : AuraScript("aura_viscidus_freeze") {}

    bool OnDummyApply(const Aura* pAura, bool bApply) override
    {
        // On Aura removal inform the boss
        if (pAura->GetId() == SPELL_VISCIDUS_FREEZE && pAura->GetEffIndex() == EFFECT_INDEX_1 && !bApply)
        {
            if (Creature* pTarget = (Creature*)pAura->GetTarget())
            {
                pTarget->AI()->SendAIEvent(AI_EVENT_CUSTOM_A, pTarget, pTarget);
            }
        }
        return true;
    }
};

// TODO Remove this 'script' when combat can be proper prevented from core-side
struct npc_glob_of_viscidus : public CreatureScript
{
    npc_glob_of_viscidus() : CreatureScript("npc_glob_of_viscidus") {}

    struct  npc_glob_of_viscidusAI : public ScriptedAI
    {
        npc_glob_of_viscidusAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        void Reset() override { }

        void AttackStart(Unit* /*pWho*/) override { }
        void MoveInLineOfSight(Unit* /*pWho*/) override { }
        void UpdateAI(const uint32 /*uiDiff*/) override { }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_glob_of_viscidusAI(pCreature);
    }
};
#endif

void AddSC_boss_viscidus()
{
    Script* s;

    s = new boss_viscidus();
    s->RegisterSelf();

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new npc_glob_of_viscidus();
    s->RegisterSelf();

    s = new aura_viscidus_freeze();
    s->RegisterSelf();
#endif

    //pNewScript = new Script;
    //pNewScript->Name = "boss_viscidus";
    //pNewScript->GetAI = &GetAI_boss_viscidus;
    //pNewScript->RegisterSelf();
    //
    //pNewScript = new Script;
    //pNewScript->Name = "npc_glob_of_viscidus";
    //pNewScript->GetAI = &GetAI_npc_glob_of_viscidus;
    //pNewScript->RegisterSelf();
}
