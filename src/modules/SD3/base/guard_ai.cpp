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
SDName: Guard_AI
SD%Complete: 90
SDComment:
SDCategory: Guards
EndScriptData */

#include "precompiled.h"
#include "guard_ai.h"

// This script is for use within every single guard to save coding time
/**
 * @brief Constructor for guardAI.
 *
 * This constructor initializes the guardAI object with the given creature pointer.
 * It also sets the initial values for the global cooldown and buff timer.
 *
 * @param pCreature Pointer to the creature this AI is associated with.
 */
guardAI::guardAI(Creature* pCreature) : ScriptedAI(pCreature),
    m_uiGlobalCooldown(0),  // Initialize global cooldown to 0
    m_uiBuffTimer(0)        // Initialize buff timer to 0
{}

/**
 * @brief Resets the guard's AI state.
 *
 * This method resets the global cooldown and buff timer to their initial values.
 * It is typically called when the guard is respawned or reset.
 */
void guardAI::Reset()
{
    m_uiGlobalCooldown = 0;  // Reset global cooldown to 0
    m_uiBuffTimer = 0;       // Reset buff timer to 0, allowing immediate rebuff
}

/**
 * @brief Handles the guard's aggro behavior.
 *
 * This method is called when the guard aggroes on a unit. If the guard is a Cenarion Infantry,
 * it will randomly choose one of three aggro sayings to shout. The guard will then attempt to
 * cast a spell on the aggroed unit if it is within range.
 *
 * @param pWho Pointer to the unit being aggroed.
 */
void guardAI::Aggro(Unit* pWho)
{
    // Check if the guard is a Cenarion Infantry
    if (m_creature->GetEntry() == NPC_CENARION_INFANTRY)
    {
        // Randomly choose one of three aggro sayings to shout
        switch (urand(0, 2))
        {
            case 0:
                DoScriptText(SAY_GUARD_SIL_AGGRO1, m_creature, pWho);
                break;
            case 1:
                DoScriptText(SAY_GUARD_SIL_AGGRO2, m_creature, pWho);
                break;
            case 2:
                DoScriptText(SAY_GUARD_SIL_AGGRO3, m_creature, pWho);
                break;
        }
    }

    // Attempt to cast a spell on the aggroed unit if it is within range
    if (const SpellEntry* pSpellInfo = m_creature->ReachWithSpellAttack(pWho))
    {
        DoCastSpell(pWho, pSpellInfo);
    }
}

/**
 * @brief Handles the guard's death behavior.
 *
 * This method is called when the guard dies. It sends a "Zone Under Attack" message to the
 * LocalDefense and WorldDefense channels if the killer is a player or controlled by a player.
 *
 * @param pKiller Pointer to the unit that killed the guard.
 */
void guardAI::JustDied(Unit* pKiller)
{
    // Check if the killer is a player or controlled by a player
    if (Player* pPlayer = pKiller->GetCharmerOrOwnerPlayerOrPlayerItself())
    {
        // Send a "Zone Under Attack" message to the LocalDefense and WorldDefense channels
        m_creature->SendZoneUnderAttackMessage(pPlayer);
    }
}

void guardAI::UpdateAI(const uint32 uiDiff)
{
    // Always decrease our global cooldown first
    if (m_uiGlobalCooldown > uiDiff)
    {
        m_uiGlobalCooldown -= uiDiff;
    }
    else
    {
        m_uiGlobalCooldown = 0;
    }

    // Buff timer (only buff when we are alive and not in combat
    if (m_creature->IsAlive() && !m_creature->IsInCombat())
    {
        if (m_uiBuffTimer < uiDiff)
        {
            // Find a spell that targets friendly and applies an aura (these are generally buffs)
            const SpellEntry* pSpellInfo = SelectSpell(m_creature, -1, -1, SELECT_TARGET_ANY_FRIEND, 0, 0, 0, 0, SELECT_EFFECT_AURA);

            if (pSpellInfo && !m_uiGlobalCooldown)
            {
                // Cast the buff spell
                DoCastSpell(m_creature, pSpellInfo);

                // Set our global cooldown
                m_uiGlobalCooldown = GENERIC_CREATURE_COOLDOWN;

                // Set our timer to 10 minutes before rebuff
                m_uiBuffTimer = 600000;
            }                                               // Try again in 30 seconds
            else
            {
                m_uiBuffTimer = 30000;
            }
        }
        else
        {
            m_uiBuffTimer -= uiDiff;
        }
    }

    // Return since we have no target
    if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
    {
        return;
    }

    // Make sure our attack is ready and we arn't currently casting
    if (m_creature->isAttackReady() && !m_creature->IsNonMeleeSpellCasted(false))
    {
        // If we are within range melee the target
        if (m_creature->CanReachWithMeleeAttack(m_creature->getVictim()))
        {
            bool bHealing = false;
            const SpellEntry* pSpellInfo = nullptr;

            // Select a healing spell if less than 30% hp
            if (m_creature->GetHealthPercent() < 30.0f)
            {
                pSpellInfo = SelectSpell(m_creature, -1, -1, SELECT_TARGET_ANY_FRIEND, 0, 0, 0, 0, SELECT_EFFECT_HEALING);
            }

            // No healing spell available, select a hostile spell
            if (pSpellInfo)
            {
                bHealing = true;
            }
            else
            {
                pSpellInfo = SelectSpell(m_creature->getVictim(), -1, -1, SELECT_TARGET_ANY_ENEMY, 0, 0, 0, 0, SELECT_EFFECT_DONTCARE);
            }

            // 20% chance to replace our white hit with a spell
            if (pSpellInfo && !urand(0, 4) && !m_uiGlobalCooldown)
            {
                // Cast the spell
                if (bHealing)
                {
                    DoCastSpell(m_creature, pSpellInfo);
                }
                else
                {
                    DoCastSpell(m_creature->getVictim(), pSpellInfo);
                }

                // Set our global cooldown
                m_uiGlobalCooldown = GENERIC_CREATURE_COOLDOWN;
            }
            else
            {
                m_creature->AttackerStateUpdate(m_creature->getVictim());
            }

            m_creature->resetAttackTimer();
        }
    }
    else
    {
        // Only run this code if we arn't already casting
        if (!m_creature->IsNonMeleeSpellCasted(false))
        {
            bool bHealing = false;
            const SpellEntry* pSpellInfo = nullptr;

            // Select a healing spell if less than 30% hp ONLY 33% of the time
            if (m_creature->GetHealthPercent() < 30.0f && !urand(0, 2))
            {
                pSpellInfo = SelectSpell(m_creature, -1, -1, SELECT_TARGET_ANY_FRIEND, 0, 0, 0, 0, SELECT_EFFECT_HEALING);
            }

            // No healing spell available, See if we can cast a ranged spell (Range must be greater than ATTACK_DISTANCE)
            if (pSpellInfo)
            {
                bHealing = true;
            }
            else
            {
                pSpellInfo = SelectSpell(m_creature->getVictim(), -1, -1, SELECT_TARGET_ANY_ENEMY, 0, 0, ATTACK_DISTANCE, 0, SELECT_EFFECT_DONTCARE);
            }

            // Found a spell, check if we arn't on cooldown
            if (pSpellInfo && !m_uiGlobalCooldown)
            {
                // If we are currently moving stop us and set the movement generator
                if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != IDLE_MOTION_TYPE)
                {
                    m_creature->GetMotionMaster()->Clear(false);
                    m_creature->GetMotionMaster()->MoveIdle();
                }

                // Cast spell
                if (bHealing)
                {
                    DoCastSpell(m_creature, pSpellInfo);
                }
                else
                {
                    DoCastSpell(m_creature->getVictim(), pSpellInfo);
                }

                // Set our global cooldown
                m_uiGlobalCooldown = GENERIC_CREATURE_COOLDOWN;
            }                                               // If no spells available and we arn't moving run to target
            else if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != CHASE_MOTION_TYPE)
            {
                // Cancel our current spell and then mutate new movement generator
                m_creature->InterruptNonMeleeSpells(false);
                m_creature->GetMotionMaster()->Clear(false);
                m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());
            }
        }
    }
}

/**
 * @brief Responds to text emotes directed at the guard.
 *
 * This method handles the guard's response to specific text emotes from players.
 * Depending on the emote received, the guard will perform a corresponding emote action.
 *
 * @param uiTextEmote The text emote ID.
 */
void guardAI::DoReplyToTextEmote(uint32 uiTextEmote)
{
    switch (uiTextEmote)
    {
        case TEXTEMOTE_KISS:
            m_creature->HandleEmote(EMOTE_ONESHOT_BOW);  // Respond with a bow
            break;
        case TEXTEMOTE_WAVE:
            m_creature->HandleEmote(EMOTE_ONESHOT_WAVE);  // Respond with a wave
            break;
        case TEXTEMOTE_SALUTE:
            m_creature->HandleEmote(EMOTE_ONESHOT_SALUTE);  // Respond with a salute
            break;
        case TEXTEMOTE_SHY:
            m_creature->HandleEmote(EMOTE_ONESHOT_FLEX);  // Respond with a flex
            break;
        case TEXTEMOTE_RUDE:
        case TEXTEMOTE_CHICKEN:
            m_creature->HandleEmote(EMOTE_ONESHOT_POINT);  // Respond with a point
            break;
    }
}

/**
 * @brief Handles emotes received by Orgrimmar guards.
 *
 * This method is called when an Orgrimmar guard receives an emote from a player.
 * If the player is a member of the Horde, the guard will respond to the emote.
 *
 * @param pPlayer Pointer to the player sending the emote.
 * @param uiTextEmote The text emote ID.
 */
void guardAI_orgrimmar::ReceiveEmote(Player* pPlayer, uint32 uiTextEmote)
{
    if (pPlayer->GetTeam() == HORDE)
    {
        DoReplyToTextEmote(uiTextEmote);  // Respond to the emote if the player is Horde
    }
}

/**
 * @brief Handles emotes received by Stormwind guards.
 *
 * This method is called when a Stormwind guard receives an emote from a player.
 * If the player is a member of the Alliance, the guard will respond to the emote.
 *
 * @param pPlayer Pointer to the player sending the emote.
 * @param uiTextEmote The text emote ID.
 */
void guardAI_stormwind::ReceiveEmote(Player* pPlayer, uint32 uiTextEmote)
{
    if (pPlayer->GetTeam() == ALLIANCE)
    {
        DoReplyToTextEmote(uiTextEmote);  // Respond to the emote if the player is Alliance
    }
}
