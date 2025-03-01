/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013 ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2025 MaNGOS <https://www.getmangos.eu>
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

/* ScriptData
SDName: boss_alizabal
SD%Complete: 93%
SDComment: [TODO] Blade Dance root needs to be cast after charging to player.
SDCategory: Baradin Hold
EndScriptData */

#include "precompiled.h"
// #include "baradin_hold.h"

enum Creatures
{
    NPC_ALIZABAL                      = 55869
};

enum Yells
{
    YELL_ALIZABAL_INTRO               = -1999927,
    YELL_ALIZABAL_AGGRO               = -1999928,
    YELL_ALIZABAL_BLADE_DANCE_1       = -1999929,
    YELL_ALIZABAL_BLADE_DANCE_2       = -1999930,
    YELL_ALIZABAL_SEETHING_HATE_1     = -1999931,
    YELL_ALIZABAL_SEETHING_HATE_2     = -1999932,
    YELL_ALIZABAL_SEETHING_HATE_3     = -1999933,
    YELL_ALIZABAL_SKEWER_1            = -1999934,
    YELL_ALIZABAL_SKEWER_2            = -1999935,
    YELL_ALIZABAL_KILL_PLAYER_1       = -1999936,
    YELL_ALIZABAL_KILL_PLAYER_2       = -1999937,
    YELL_ALIZABAL_KILL_PLAYER_3       = -1999938,
    YELL_ALIZABAL_KILL_PLAYER_4       = -1999939,
    YELL_ALIZABAL_WIPE                = -1999940,
    YELL_ALIZABAL_DEATH               = -1999941
};

enum Spells
{
    SPELL_SKEWER                      = 104936,
    SPELL_SEETHING_HATE               = 105067,

    // These codes came from Trinity Core [[ https://github.com/The-Cataclysm-Preservation-Project/TrinityCore/blob/master/src/server/scripts/EasternKingdoms/BaradinHold/boss_alizabal.cpp]]
    // SPELL_BLADE_DANCE                 = 104995,
    // SPELL_BLADE_DANCE_SPIN            = 105828,
    // SPELL_BLADE_DANCE_CHARGE          = 105726,
    // SPELL_BLADE_DANCE_ROOT            = 105784,

    // These codes came from WowHead [[ https://www.wowhead.com/npc=55869/alizabal#comments:id=1532708 ]]
    // SPELL_BLADE_DANCE_AURA            = 104995,      // https://www.wowhead.com/spell=104995/blade-dance
    // SPELL_BLADE_DANCE_UNKNOWN         = 104994,      // https://www.wowhead.com/spell=104994/blade-dance , this is MUCH weaker than the one above.
    SPELL_BLADE_DANCE_ROOT            = 105784,         // https://www.wowhead.com/spell=105784/blade-dance
    // SPELL_BLADE_DANCE_INVINCIBLE      = 105738,      // https://www.wowhead.com/spell=105738/blade-dance
    // SPELL_BLADE_DANCE_UNKNOWN2        = 106248,      // https://www.wowhead.com/spell=106248/blade-dance
    SPELL_BLADE_DANCE_CHARGE          = 105726,         // https://www.wowhead.com/spell=105726/blade-dance
    SPELL_BLADE_DANCE_AURA2           = 105828,         // https://www.wowhead.com/spell=105828/blade-dance , causes Alizabal to spin for 22 seconds.


    SPELL_BERSERK                     = 47008,
};

/*
    Unused at the moment - but we'll need to close the door at time of encounter.
*/
enum GameObjects
{
    GAMEOBJECT_TOLBARAD_DOOR_01       = 207849
};

struct boss_alizabal : public CreatureScript
{
    boss_alizabal() : CreatureScript("boss_alizabal") {}

    struct boss_alizabalAI : public ScriptedAI
    {
        boss_alizabalAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        // Timers
        uint32 m_uiEnrageTimer;
        uint32 m_uiSpecialTimer;
        uint32 m_uiBladeDanceTimer;
        uint32 m_uiBladeDanceRandomTimer;
        uint32 m_uiBladeDanceEndTimer;

        // Data Storage
        uint8 m_uiSpecial;                                                          // 0 = Seething Hate, 1 = Skewer
        bool m_bFirstSpecialDone;                                                   // True = Second Special is next, False = First Special is next

        void Reset() override
        {
            DoScriptText(YELL_ALIZABAL_WIPE, m_creature);

            m_uiSpecialTimer = urand(6,8) * IN_MILLISECONDS;                        // First special starts between 6-8 seconds after pull.
            m_uiBladeDanceTimer = 25 * IN_MILLISECONDS;                             // Blade Dance starts at 25 seconds after pull.
            m_uiBladeDanceRandomTimer = 5 * IN_MILLISECONDS;
            m_uiBladeDanceEndTimer = 0;
            m_uiEnrageTimer = 5 * MINUTE * IN_MILLISECONDS;                         // Berserk starts at 5 minutes after pull.

            m_uiSpecial = 0;
            m_bFirstSpecialDone = false;

            m_creature->SetSpeedRate(MOVE_RUN, 2.0f);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            DoScriptText(YELL_ALIZABAL_AGGRO, m_creature);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Shared Timer for Seething Hate and Skewer
            if (m_uiSpecialTimer < uiDiff)
            {
                if (!m_bFirstSpecialDone) // First Special has not been completed, so we need to randomize the first special.
                {
                    m_uiSpecial = urand(0,1);
                }

                switch (m_uiSpecial)
                {
                case 0:  // Seething Hate is cast onto random target.
                    if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0, SPELL_SEETHING_HATE, SELECT_FLAG_PLAYER))
                    {
                        if (DoCastSpellIfCan(pTarget, SPELL_SEETHING_HATE) == CAST_OK)
                        {
                            switch (urand(0,2))
                            {
                                case 0: DoScriptText(YELL_ALIZABAL_SEETHING_HATE_1, m_creature); break;
                                case 1: DoScriptText(YELL_ALIZABAL_SEETHING_HATE_2, m_creature); break;
                                case 2: DoScriptText(YELL_ALIZABAL_SEETHING_HATE_3, m_creature); break;
                            }

                            m_uiSpecial = 1; // This makes sure that the next time a special is done it will be the other one.
                        }
                    }
                    break;


                case 1:   // Skewer is cast onto current target.
                    if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_SKEWER) == CAST_OK)
                    {
                        DoScriptText(urand(0, 1) ? YELL_ALIZABAL_SKEWER_1 : YELL_ALIZABAL_SKEWER_2, m_creature);

                        m_uiSpecial = 0; // This makes sure that the next time a special is done it will be the other one.
                    }
                    break;
                }

                if (!m_bFirstSpecialDone)                         // True = Second Special is next, False = First Special is next
                {
                    m_bFirstSpecialDone = true;                   // Next time we run SpecialTimer we will NOT randomize the special.
                    m_uiSpecialTimer = 8 * IN_MILLISECONDS;
                }
                else
                {
                    m_uiSpecialTimer = 12 * IN_MILLISECONDS;
                }
            }
            else
            {
                m_uiSpecialTimer -= uiDiff;
            }

            // Blade Dance Timer
            if (m_uiBladeDanceEndTimer)                          // Is in BladeDance
            {
                // While BladeDance, switch to random targets often
                if (m_uiBladeDanceRandomTimer < uiDiff)
                {
                    if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    {
                        m_creature->FixateTarget(pTarget);
                        if (DoCastSpellIfCan(pTarget, SPELL_BLADE_DANCE_CHARGE) == CAST_OK)
                        {
                            DoCastSpellIfCan(m_creature, SPELL_BLADE_DANCE_ROOT, CAST_TRIGGERED);
                        }
                    }

                    m_uiBladeDanceRandomTimer = 5 * IN_MILLISECONDS;
                }
                else
                {
                    m_uiBladeDanceRandomTimer -= uiDiff;
                }

                // End BladeDance Phase
                if (m_uiBladeDanceEndTimer <= uiDiff)
                {
                    m_creature->SetSpeedRate(MOVE_RUN, 2.0f);
                    m_creature->FixateTarget(nullptr);
                    m_uiBladeDanceEndTimer = 0;
                    m_uiBladeDanceTimer = 60 * IN_MILLISECONDS;
                }
                else
                {
                    m_uiBladeDanceEndTimer -= uiDiff;
                }
            }
            else // if (!m_uiBladeDanceEndTimer)                 // Is not in BladeDance
            {
                // Enter BladeDance Phase
                if (m_uiBladeDanceTimer < uiDiff)
                {
                    if (DoCastSpellIfCan(m_creature, SPELL_BLADE_DANCE_AURA2) == CAST_OK)
                    {
                        m_creature->SetSpeedRate(MOVE_RUN, 4.0f);

                        DoScriptText(urand(0, 1) ? YELL_ALIZABAL_BLADE_DANCE_1 : YELL_ALIZABAL_BLADE_DANCE_2, m_creature);
                        m_uiBladeDanceEndTimer = 15 * IN_MILLISECONDS;
                        m_uiBladeDanceRandomTimer = 5 * IN_MILLISECONDS;

                        if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                        {
                            m_creature->FixateTarget(pTarget);
                        if (DoCastSpellIfCan(pTarget, SPELL_BLADE_DANCE_CHARGE) == CAST_OK)
                        {
                            DoCastSpellIfCan(m_creature, SPELL_BLADE_DANCE_ROOT, CAST_TRIGGERED);
                        }
                        }
                    }
                }
                else
                {
                    m_uiBladeDanceTimer -= uiDiff;
                }
            }

            // Berserk Timer
            if (m_uiEnrageTimer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_BERSERK) == CAST_OK)
                {
                    m_uiEnrageTimer = 5 * MINUTE * IN_MILLISECONDS;
                }
            }
            else
            {
                m_uiEnrageTimer -= uiDiff;
            }

            // No melee damage while in BladeDance
            if (!m_uiBladeDanceEndTimer)
            {
                DoMeleeAttackIfReady();
            }
        }

        void KilledUnit(Unit* pVictim) override
        {
            if (pVictim->GetTypeId() != TYPEID_PLAYER)
            {
                return;
            }

            switch (urand(0, 3))
            {
                case 0: DoScriptText(YELL_ALIZABAL_KILL_PLAYER_1, m_creature); break;
                case 1: DoScriptText(YELL_ALIZABAL_KILL_PLAYER_2, m_creature); break;
                case 2: DoScriptText(YELL_ALIZABAL_KILL_PLAYER_3, m_creature); break;
                case 3: DoScriptText(YELL_ALIZABAL_KILL_PLAYER_4, m_creature); break;
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            DoScriptText(YELL_ALIZABAL_DEATH, m_creature);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new boss_alizabalAI(pCreature);
    }
};

void AddSC_boss_alizabal()
{
    Script* s;

    s = new boss_alizabal();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "boss_alizabal";
    //pNewScript->GetAI = &GetAI_boss_alizabal;
    //pNewScript->RegisterSelf();
}
