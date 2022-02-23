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
 * SDName:      Boss_Onyxia
 * SD%Complete: 85
 * SDComment:   Phase 3 need additional code. The spawning Whelps need GO-Support.
 * SDCategory:  Onyxia's Lair
 * EndScriptData
 */

#include "precompiled.h"
#include "onyxias_lair.h"

enum
{
    SAY_AGGRO                   = -1249000,
    SAY_KILL                    = -1249001,
    SAY_PHASE_2_TRANS           = -1249002,
    SAY_PHASE_3_TRANS           = -1249003,
    EMOTE_BREATH                = -1249004,

    SPELL_WINGBUFFET            = 18500,
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
    SPELL_WINGBUFFET_H          = 69293,
#endif
    SPELL_FLAMEBREATH           = 18435,
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
    SPELL_FLAMEBREATH_H         = 68970,
#endif
#if defined (CLASSIC) || defined (TBC)
    SPELL_CLEAVE                = 19983,
    SPELL_TAILSWEEP             = 15847,
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
    SPELL_CLEAVE                = 68868,
    SPELL_TAILSWEEP             = 68867,
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
    SPELL_TAILSWEEP_H           = 69286,
#endif
    SPELL_KNOCK_AWAY            = 19633,
    SPELL_FIREBALL              = 18392,
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
    SPELL_FIREBALL_H            = 68926,
#endif

    // Not much choise about these. We have to make own defintion on the direction/start-end point
    SPELL_BREATH_NORTH_TO_SOUTH = 17086,                    // 20x in "array"
    SPELL_BREATH_SOUTH_TO_NORTH = 18351,                    // 11x in "array"

    SPELL_BREATH_EAST_TO_WEST   = 18576,                    // 7x in "array"
    SPELL_BREATH_WEST_TO_EAST   = 18609,                    // 7x in "array"

    SPELL_BREATH_SE_TO_NW       = 18564,                    // 12x in "array"
    SPELL_BREATH_NW_TO_SE       = 18584,                    // 12x in "array"
    SPELL_BREATH_SW_TO_NE       = 18596,                    // 12x in "array"
    SPELL_BREATH_NE_TO_SW       = 18617,                    // 12x in "array"

    SPELL_VISUAL_BREATH_A       = 4880,                     // Only and all of the above Breath spells (and their triggered spells) have these visuals
    SPELL_VISUAL_BREATH_B       = 4919,

    SPELL_BREATH_ENTRANCE       = 21131,                    // 8x in "array", different initial cast than the other arrays

    SPELL_BELLOWINGROAR         = 18431,
    SPELL_HEATED_GROUND         = 22191,                    // Prevent players from hiding in the tunnels when it is time for Onyxia's breath

    SPELL_SUMMONWHELP           = 17646,                    // TODO this spell is only a summon spell, but would need a spell to activate the eggs
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
    SPELL_SUMMON_LAIR_GUARD     = 68968,
#endif

    MAX_WHELPS_PER_PACK         = 40,

    POINT_ID_NORTH              = 0,
    POINT_ID_SOUTH              = 4,
    NUM_MOVE_POINT              = 8,
    POINT_ID_LIFTOFF            = 1 + NUM_MOVE_POINT,
    POINT_ID_IN_AIR             = 2 + NUM_MOVE_POINT,
    POINT_ID_INIT_NORTH         = 3 + NUM_MOVE_POINT,
    POINT_ID_LAND               = 4 + NUM_MOVE_POINT,

    PHASE_START                 = 1,                        // Health above 65%, normal ground abilities
    PHASE_BREATH                = 2,                        // Breath phase (while health above 40%)
    PHASE_END                   = 3,                        // normal ground abilities + some extra abilities
    PHASE_BREATH_POST           = 4,                        // Landing and initial fearing
    PHASE_TO_LIFTOFF            = 5,                        // Movement to south-entrance of room and liftoff there
    PHASE_BREATH_PRE            = 6,                        // lifting off + initial flying to north side (summons also first pack of whelps)

};

struct OnyxiaMove
{
    uint32 uiSpellId;
    float fX, fY, fZ;
};

static const OnyxiaMove aMoveData[NUM_MOVE_POINT] =
{
    {SPELL_BREATH_NORTH_TO_SOUTH,  24.16332f, -216.0808f, -58.98009f},  // north (coords verified in wotlk)
    {SPELL_BREATH_NE_TO_SW,        10.2191f,  -247.912f,  -60.896f},    // north-east
    {SPELL_BREATH_EAST_TO_WEST,   -15.00505f, -244.4841f, -60.40087f},  // east (coords verified in wotlk)
    {SPELL_BREATH_SE_TO_NW,       -63.5156f,  -240.096f,  -60.477f},    // south-east
    {SPELL_BREATH_SOUTH_TO_NORTH, -66.3589f,  -215.928f,  -64.23904f},  // south (coords verified in wotlk)
    {SPELL_BREATH_SW_TO_NE,       -58.2509f,  -189.020f,  -60.790f},    // south-west
    {SPELL_BREATH_WEST_TO_EAST,   -16.70134f, -181.4501f, -61.98513f},  // west (coords verified in wotlk)
    {SPELL_BREATH_NW_TO_SE,        12.26687f, -181.1084f, -60.23914f},  // north-west (coords verified in wotlk)
};

static const float afSpawnLocations[3][3] =
{
    { -30.127f, -254.463f, -89.440f},                       // whelps
    { -30.817f, -177.106f, -89.258f},                       // whelps
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
    { -126.57f, -214.609f, -71.446f}                        // guardians
#endif
};

struct boss_onyxia : public CreatureScript
{
    boss_onyxia() : CreatureScript("boss_onyxia") {}

    struct boss_onyxiaAI : public ScriptedAI
    {
        boss_onyxiaAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
            m_bIsRegularMode = pCreature->GetMap()->IsRegularDifficulty();
#endif
            Reset();
        }

        ScriptedInstance* m_pInstance;
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
        bool m_bIsRegularMode;
        uint32 m_uiSummonGuardTimer;
#endif

        uint8 m_uiPhase;

        uint32 m_uiFlameBreathTimer;
        uint32 m_uiCleaveTimer;
        uint32 m_uiTailSweepTimer;
        uint32 m_uiWingBuffetTimer;
        uint32 m_uiCheckInLairTimer;
        uint32 m_uiKnockTimer;

        uint32 m_uiMovePoint;
        uint32 m_uiMovementTimer;

        uint32 m_uiFireballTimer;
        uint32 m_uiSummonWhelpsTimer;
        uint32 m_uiBellowingRoarTimer;
        uint32 m_uiWhelpTimer;

        uint8 m_uiSummonCount;

        bool m_bIsSummoningWhelps;

        uint32 m_uiPhaseTimer;

        void Reset() override
        {
            if (!IsCombatMovement())
            {
                SetCombatMovement(true);
            }

            m_uiPhase = PHASE_START;

            m_uiFlameBreathTimer = urand(10000, 20000);
            m_uiTailSweepTimer = urand(15000, 20000);
            m_uiCleaveTimer = urand(2000, 5000);
            m_uiWingBuffetTimer = urand(10000, 20000);
            m_uiCheckInLairTimer = 3000;
            m_uiKnockTimer = urand(10000, 15000);
            m_uiMovePoint = POINT_ID_NORTH;                     // First point reached by the flying Onyxia
            m_uiMovementTimer = 25000;

            m_uiFireballTimer = 1000;
            m_uiSummonWhelpsTimer = 60000;
            m_uiBellowingRoarTimer = 30000;
            m_uiWhelpTimer = 1000;
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
            m_uiSummonGuardTimer = 15000;
#endif

            m_uiSummonCount = 0;

            m_bIsSummoningWhelps = false;

            m_uiPhaseTimer = 0;
        }

        void Aggro(Unit* /*pWho*/) override
        {
            DoScriptText(SAY_AGGRO, m_creature);

            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_ONYXIA, IN_PROGRESS);
            }
        }

        void JustReachedHome() override
        {
            // in case evade in phase 2, see comments for hack where phase 2 is set
            m_creature->SetLevitate(false);
            m_creature->SetByteFlag(UNIT_FIELD_BYTES_1, 3, 0);

            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_ONYXIA, FAIL);
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            if (m_pInstance)
            {
                m_pInstance->SetData(TYPE_ONYXIA, DONE);
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (!m_pInstance)
            {
                return;
            }

            if (Creature* pTrigger = m_pInstance->GetSingleCreatureFromStorage(NPC_ONYXIA_TRIGGER))
            {
                // Get some random point near the center
                float fX, fY, fZ;
                pSummoned->GetRandomPoint(pTrigger->GetPositionX(), pTrigger->GetPositionY(), pTrigger->GetPositionZ(), 20.0f, fX, fY, fZ);
                pSummoned->GetMotionMaster()->MovePoint(1, fX, fY, fZ);
            }
            else
            {
                pSummoned->SetInCombatWithZone();
            }

            if (pSummoned->GetEntry() == NPC_ONYXIA_WHELP)
            {
                ++m_uiSummonCount;
            }
        }

        void SummonedMovementInform(Creature* pSummoned, uint32 uiMoveType, uint32 uiPointId) override
        {
            if (uiMoveType != POINT_MOTION_TYPE || uiPointId != 1 || !m_creature->getVictim())
            {
                return;
            }

            pSummoned->SetInCombatWithZone();
        }

        void KilledUnit(Unit* /*pVictim*/) override
        {
            DoScriptText(SAY_KILL, m_creature);
        }

        void SpellHit(Unit* /*pCaster*/, const SpellEntry* pSpell) override
        {
            if (pSpell->Id == SPELL_BREATH_EAST_TO_WEST ||
                pSpell->Id == SPELL_BREATH_WEST_TO_EAST ||
                pSpell->Id == SPELL_BREATH_SE_TO_NW ||
                pSpell->Id == SPELL_BREATH_NW_TO_SE ||
                pSpell->Id == SPELL_BREATH_SW_TO_NE ||
                pSpell->Id == SPELL_BREATH_NE_TO_SW ||
                pSpell->Id == SPELL_BREATH_SOUTH_TO_NORTH ||
                pSpell->Id == SPELL_BREATH_NORTH_TO_SOUTH)
            {
                // This was sent with SendMonsterMove - which resulted in better speed than now
                m_creature->GetMotionMaster()->MovePoint(m_uiMovePoint, aMoveData[m_uiMovePoint].fX, aMoveData[m_uiMovePoint].fY, aMoveData[m_uiMovePoint].fZ);
                DoCastSpellIfCan(m_creature, SPELL_HEATED_GROUND, CAST_TRIGGERED);
            }
        }

        void MovementInform(uint32 uiMoveType, uint32 uiPointId) override
        {
            if (uiMoveType != POINT_MOTION_TYPE || !m_pInstance)
            {
                return;
            }

            switch (uiPointId)
            {
                case POINT_ID_IN_AIR:
                    // sort of a hack, it is unclear how this really work but the values are valid
#if defined (CLASSIC) || defined (TBC)
                    m_creature->SetByteValue(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND);
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
                    m_creature->SetByteValue(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_FLY_ANIM);
#endif
                    m_creature->SetLevitate(true);
                    m_uiPhaseTimer = 1000;                          // Movement to Initial North Position is delayed
                    return;
                case POINT_ID_LAND:
                    // undo flying
                    m_creature->SetByteValue(UNIT_FIELD_BYTES_1, 3, 0);
                    m_creature->SetLevitate(false);
                    m_uiPhaseTimer = 500;                           // Start PHASE_END shortly delayed
                    return;
                case POINT_ID_LIFTOFF:
                    m_uiPhaseTimer = 500;                           // Start Flying shortly delayed
                    break;
                case POINT_ID_INIT_NORTH:                           // Start PHASE_BREATH
                    m_uiPhase = PHASE_BREATH;
                    m_uiSummonCount = 0;
                    break;
            }

            if (Creature* pTrigger = m_pInstance->GetSingleCreatureFromStorage(NPC_ONYXIA_TRIGGER))
            {
                m_creature->SetFacingToObject(pTrigger);
            }
        }

        void AttackStart(Unit* pWho) override
        {
            if (m_uiPhase == PHASE_START || m_uiPhase == PHASE_END)
            {
                ScriptedAI::AttackStart(pWho);
            }
        }

        bool DidSummonWhelps(const uint32 uiDiff)
        {
            if (m_uiSummonCount >= MAX_WHELPS_PER_PACK)
            {
                return true;
            }

            if (m_uiWhelpTimer < uiDiff)
            {
                m_creature->SummonCreature(NPC_ONYXIA_WHELP, afSpawnLocations[0][0], afSpawnLocations[0][1], afSpawnLocations[0][2], 0.0f, TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, MINUTE * IN_MILLISECONDS);
                m_creature->SummonCreature(NPC_ONYXIA_WHELP, afSpawnLocations[1][0], afSpawnLocations[1][1], afSpawnLocations[1][2], 0.0f, TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, MINUTE * IN_MILLISECONDS);
                m_uiWhelpTimer = 500;
            }
            else
            {
                m_uiWhelpTimer -= uiDiff;
            }
            return false;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            switch (m_uiPhase)
            {
                case PHASE_END:                                 // Here is room for additional summoned whelps and Erruption
                    if (m_uiBellowingRoarTimer < uiDiff)
                    {
                        if (DoCastSpellIfCan(m_creature, SPELL_BELLOWINGROAR) == CAST_OK)
                        {
                            m_uiBellowingRoarTimer = 30000;
                        }
                    }
                    else
                    {
                        m_uiBellowingRoarTimer -= uiDiff;
                    }
                // no break, phase 3 will use same abilities as in 1
                case PHASE_START:
                {
                    if (m_uiFlameBreathTimer < uiDiff)
                    {
#if defined (CLASSIC) || defined (TBC)
                        if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_FLAMEBREATH) == CAST_OK)
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
                        if (DoCastSpellIfCan(m_creature->getVictim(), m_bIsRegularMode ? SPELL_FLAMEBREATH : SPELL_FLAMEBREATH_H) == CAST_OK)
#endif
                        {
                            m_uiFlameBreathTimer = urand(10000, 20000);
                        }
                    }
                    else
                    {
                        m_uiFlameBreathTimer -= uiDiff;
                    }

                    if (m_uiTailSweepTimer < uiDiff)
                    {
#if defined (CLASSIC) || defined (TBC)
                        if (DoCastSpellIfCan(m_creature, SPELL_TAILSWEEP) == CAST_OK)
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
                        if (DoCastSpellIfCan(m_creature, m_bIsRegularMode ? SPELL_TAILSWEEP : SPELL_TAILSWEEP_H) == CAST_OK)
#endif
                        {
                            m_uiTailSweepTimer = urand(15000, 20000);
                        }
                    }
                    else
                    {
                        m_uiTailSweepTimer -= uiDiff;
                    }

                    if (m_uiKnockTimer < uiDiff)
                    {
                        if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_KNOCK_AWAY) == CAST_OK)
                        {
                            m_uiKnockTimer = urand(10000, 15000);
                        }
                    }
                    else
                    {
                        m_uiKnockTimer -= uiDiff;
                    }

                    if (m_uiCleaveTimer < uiDiff)
                    {
                        if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CLEAVE) == CAST_OK)
                        {
                            m_uiCleaveTimer = urand(2000, 5000);
                        }
                    }
                    else
                    {
                        m_uiCleaveTimer -= uiDiff;
                    }

                    if (m_uiWingBuffetTimer < uiDiff)
                    {
#if defined (CLASSIC) || defined (TBC)
                        if (DoCastSpellIfCan(m_creature, SPELL_WINGBUFFET) == CAST_OK)
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
                        if (DoCastSpellIfCan(m_creature, m_bIsRegularMode ? SPELL_WINGBUFFET : SPELL_WINGBUFFET_H) == CAST_OK)
#endif
                        {
                            m_uiWingBuffetTimer = urand(15000, 30000);
                        }
                    }
                    else
                    {
                        m_uiWingBuffetTimer -= uiDiff;
                    }

                    if (m_uiCheckInLairTimer < uiDiff)
                    {
                        if (m_pInstance)
                        {
                            Creature* pOnyTrigger = m_pInstance->GetSingleCreatureFromStorage(NPC_ONYXIA_TRIGGER);
                            if (pOnyTrigger && !m_creature->IsWithinDistInMap(pOnyTrigger, 90.0f, false))
                            {
                                DoCastSpellIfCan(m_creature, SPELL_BREATH_ENTRANCE);
                            }
                        }
                        m_uiCheckInLairTimer = 3000;
                    }
                    else
                    {
                        m_uiCheckInLairTimer -= uiDiff;
                    }

                    if (m_uiPhase == PHASE_START && m_creature->GetHealthPercent() < 65.0f)
                    {
                        m_uiPhase = PHASE_TO_LIFTOFF;
                        DoScriptText(SAY_PHASE_2_TRANS, m_creature);
                        SetCombatMovement(false);
                        m_creature->GetMotionMaster()->MoveIdle();
                        m_creature->SetTargetGuid(ObjectGuid());

#if defined (CLASSIC) || defined (TBC)
                        float fGroundZ = m_creature->GetMap()->GetHeight(aMoveData[POINT_ID_SOUTH].fX, aMoveData[POINT_ID_SOUTH].fY, aMoveData[POINT_ID_SOUTH].fZ);
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
                        float fGroundZ = m_creature->GetMap()->GetHeight(m_creature->GetPhaseMask(), aMoveData[POINT_ID_SOUTH].fX, aMoveData[POINT_ID_SOUTH].fY, aMoveData[POINT_ID_SOUTH].fZ);
#endif
                        m_creature->GetMotionMaster()->MovePoint(POINT_ID_LIFTOFF, aMoveData[POINT_ID_SOUTH].fX, aMoveData[POINT_ID_SOUTH].fY, fGroundZ);
                        return;
                    }

                    DoMeleeAttackIfReady();
                    break;
                }
                case PHASE_BREATH:
                {
                    if (m_creature->GetHealthPercent() < 40.0f)
                    {
                        m_uiPhase = PHASE_BREATH_POST;
                        DoScriptText(SAY_PHASE_3_TRANS, m_creature);

#if defined (CLASSIC) || defined (TBC)
                        float fGroundZ = m_creature->GetMap()->GetHeight(m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ());
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
                        float fGroundZ = m_creature->GetMap()->GetHeight(m_creature->GetPhaseMask(), m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ());
#endif
                        m_creature->GetMotionMaster()->MoveFlyOrLand(POINT_ID_LAND, m_creature->GetPositionX(), m_creature->GetPositionY(), fGroundZ, false);
                        return;
                    }

                    if (m_uiMovementTimer < uiDiff)
                    {
                         // 3 possible actions
                         switch (urand(0, 2))
                         {
                             case 0:                             // breath
                                 DoScriptText(EMOTE_BREATH, m_creature);
                                 DoCastSpellIfCan(m_creature, aMoveData[m_uiMovePoint].uiSpellId, CAST_INTERRUPT_PREVIOUS);
                                 m_uiMovePoint += NUM_MOVE_POINT / 2;
                                 m_uiMovePoint %= NUM_MOVE_POINT;
                                 m_uiMovementTimer = 25000;
                                 return;
                              case 1:                             // a point on the left side
                              {
                                  // C++ is stupid, so add -1 with +7
                                  m_uiMovePoint += NUM_MOVE_POINT - 1;
                                  m_uiMovePoint %= NUM_MOVE_POINT;
                                  break;
                              }
                              case 2:                             // a point on the right side
                                   ++m_uiMovePoint %= NUM_MOVE_POINT;
                                   break;
                          }

                          m_uiMovementTimer = urand(15000, 25000);
                          m_creature->GetMotionMaster()->MovePoint(m_uiMovePoint, aMoveData[m_uiMovePoint].fX, aMoveData[m_uiMovePoint].fY, aMoveData[m_uiMovePoint].fZ);
                      }
                      else
                      {
                          m_uiMovementTimer -= uiDiff;
                      }

                      if (m_uiFireballTimer < uiDiff)
                      {
                          if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                          {
#if defined (CLASSIC) || defined (TBC)
                              if (DoCastSpellIfCan(pTarget, SPELL_FIREBALL) == CAST_OK)
#endif
#if defined (WOTLK) || defined (CATA) || defined (MISTS)
                              if (DoCastSpellIfCan(pTarget, m_bIsRegularMode ? SPELL_FIREBALL : SPELL_FIREBALL_H) == CAST_OK)
#endif
                              {
                                  m_uiFireballTimer = urand(3000, 5000);
                              }
                          }
                      }
                      else
                      {
                          m_uiFireballTimer -= uiDiff;    // engulfingflames is supposed to be activated by a fireball but haven't come by
                      }

                      if (m_bIsSummoningWhelps)
                      {
                          if (DidSummonWhelps(uiDiff))
                          {
                              m_bIsSummoningWhelps = false;
                              m_uiSummonCount = 0;
                              m_uiSummonWhelpsTimer = 80000;      // 90s - 10s for summoning
                          }
                      }
                      else
                      {
                          if (m_uiSummonWhelpsTimer < uiDiff)
                          {
                              m_bIsSummoningWhelps = true;
                          }
                          else
                          {
                              m_uiSummonWhelpsTimer -= uiDiff;
                          }
                      }

#if defined (WOTLK) || defined (CATA) || defined (MISTS)
                      if (m_uiSummonGuardTimer < uiDiff)
                      {
                          if (!m_creature->IsNonMeleeSpellCasted(false))
                          {
                              m_creature->CastSpell(afSpawnLocations[2][0], afSpawnLocations[2][1], afSpawnLocations[2][2], SPELL_SUMMON_LAIR_GUARD, true);
                              m_uiSummonGuardTimer = 30000;
                          }
                      }
                      else
                      {
                          m_uiSummonGuardTimer -= uiDiff;
                      }
#endif

                      break;
                }
                case PHASE_BREATH_PRE:                          // Summon first rounds of whelps
                    DidSummonWhelps(uiDiff);
                    // no break here
                default:                                        // Phase-switching phases
                    if (!m_uiPhaseTimer)
                    {
                        break;
                    }
                    if (m_uiPhaseTimer <= uiDiff)
                    {
                        switch (m_uiPhase)
                        {
                            case PHASE_TO_LIFTOFF:
                                m_uiPhase = PHASE_BREATH_PRE;
                                if (m_pInstance)
                                {
                                    m_pInstance->SetData(TYPE_ONYXIA, DATA_LIFTOFF);
                                }
                                m_creature->GetMotionMaster()->MoveFlyOrLand(POINT_ID_IN_AIR, aMoveData[POINT_ID_SOUTH].fX, aMoveData[POINT_ID_SOUTH].fY, aMoveData[POINT_ID_SOUTH].fZ, true);
                                break;
                            case PHASE_BREATH_PRE:
                                m_creature->GetMotionMaster()->MovePoint(POINT_ID_INIT_NORTH, aMoveData[POINT_ID_NORTH].fX, aMoveData[POINT_ID_NORTH].fY, aMoveData[POINT_ID_NORTH].fZ);
                                break;
                            case PHASE_BREATH_POST:
                                m_uiPhase = PHASE_END;
                                m_creature->SetTargetGuid(m_creature->getVictim()->GetObjectGuid());
                                SetCombatMovement(true, true);
                                DoCastSpellIfCan(m_creature, SPELL_BELLOWINGROAR);
                                break;
                        }
                        m_uiPhaseTimer = 0;
                    }
                    else
                    {
                        m_uiPhaseTimer -= uiDiff;
                    }
                    break;
            }
        }

#if defined (WOTLK) || defined (CATA)
    void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell) override
    {
        // Check if players are hit by Onyxia's Deep Breath
        if (pTarget->GetTypeId() != TYPEID_PLAYER || !m_pInstance)
        {
            return;
        }

        // All and only the Onyxia Deep Breath Spells have these visuals
        if (pSpell->SpellVisual[0] == SPELL_VISUAL_BREATH_A || pSpell->SpellVisual[0] == SPELL_VISUAL_BREATH_B)
        {
            m_pInstance->SetData(TYPE_ONYXIA, DATA_PLAYER_TOASTED);
        }
    }
#elif defined (MISTS)
    void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell) override
    {
        // Check if players are hit by Onyxia's Deep Breath
        if (pTarget->GetTypeId() != TYPEID_PLAYER || !m_pInstance)
        {
            return;
        }

        // All and only the Onyxia Deep Breath Spells have these visuals
        if (pSpell->GetSpellVisual(0) == SPELL_VISUAL_BREATH_A || pSpell->GetSpellVisual(0) == SPELL_VISUAL_BREATH_B)
        {
            m_pInstance->SetData(TYPE_ONYXIA, DATA_PLAYER_TOASTED);
        }
    }
#endif
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_onyxiaAI(pCreature);
    }
};

void AddSC_boss_onyxia()
{
    Script* s;
    s = new boss_onyxia();
    s->RegisterSelf();
}
