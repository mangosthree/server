/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

#include "Unit.h"
#include "Log.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "ObjectGuid.h"
#include "SpellMgr.h"
#include "QuestDef.h"
#include "Player.h"
#include "Creature.h"
#include "Spell.h"
#include "Group.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "TemporarySummon.h"
#include "Formulas.h"
#include "Pet.h"
#include "Util.h"
#include "Totem.h"
#include "Vehicle.h"
#include "BattleGround/BattleGround.h"
#include "InstanceData.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "MapPersistentStateMgr.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "VMapFactory.h"
#include "CreatureLinkingMgr.h"
#include "GameTime.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#include "ElunaConfig.h"
#include "ElunaEventMgr.h"
#endif /* ENABLE_ELUNA */

/**
 * @file UnitMeleeDamage.cpp
 * @brief Cohesion split of Unit.cpp -- melee damage: CalculateMeleeDamage (hit outcome + damage roll) and DealMeleeDamage (apply damage, durability, on-hit side effects). Same Unit class; no behaviour change. CMake file(GLOB) picks this file up automatically; Unit.h is unchanged.
 */

// TODO for melee need create structure as in
void Unit::CalculateMeleeDamage(Unit* pVictim, CalcDamageInfo* damageInfo, WeaponAttackType attackType /*= BASE_ATTACK*/)
{
    damageInfo->attacker         = this;
    damageInfo->target           = pVictim;
    damageInfo->damageSchoolMask = GetMeleeDamageSchoolMask();
    damageInfo->attackType       = attackType;
    damageInfo->damage           = 0;
    damageInfo->cleanDamage      = 0;
    damageInfo->absorb           = 0;
    damageInfo->resist           = 0;
    damageInfo->blocked_amount   = 0;

    damageInfo->TargetState      = VICTIMSTATE_UNAFFECTED;
    damageInfo->HitInfo          = HITINFO_NORMALSWING;
    damageInfo->procAttacker     = PROC_FLAG_NONE;
    damageInfo->procVictim       = PROC_FLAG_NONE;
    damageInfo->procEx           = PROC_EX_NONE;
    damageInfo->hitOutCome       = MELEE_HIT_EVADE;

    if (!this || !pVictim)
    {
        return;
    }
    if (!this->IsAlive() || !pVictim->IsAlive())
    {
        return;
    }

    // Select HitInfo/procAttacker/procVictim flag based on attack type
    switch (attackType)
    {
        case BASE_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_SUCCESSFUL_MELEE_HIT;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_MELEE_HIT;
            damageInfo->HitInfo      = HITINFO_NORMALSWING2;
            break;
        case OFF_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_SUCCESSFUL_MELEE_HIT | PROC_FLAG_SUCCESSFUL_OFFHAND_HIT;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_MELEE_HIT;//|PROC_FLAG_TAKEN_OFFHAND_HIT // not used
            damageInfo->HitInfo = HITINFO_LEFTSWING;
            break;
        case RANGED_ATTACK:
            damageInfo->procAttacker = PROC_FLAG_SUCCESSFUL_RANGED_HIT;
            damageInfo->procVictim   = PROC_FLAG_TAKEN_RANGED_HIT;
            damageInfo->HitInfo = HITINFO_UNK3;             // test (dev note: test what? HitInfo flag possibly not confirmed.)
            break;
        default:
            break;
    }

    // Physical Immune check
    if (damageInfo->target->IsImmunedToDamage(damageInfo->damageSchoolMask))
    {
        damageInfo->HitInfo       |= HITINFO_NORMALSWING;
        damageInfo->TargetState    = VICTIMSTATE_IS_IMMUNE;

        damageInfo->procEx |= PROC_EX_IMMUNE;
        damageInfo->damage         = 0;
        damageInfo->cleanDamage    = 0;
        return;
    }
    uint32 damage = CalculateDamage(damageInfo->attackType, false);
    // Add melee damage bonus
    damage = MeleeDamageBonusDone(damageInfo->target, damage, damageInfo->attackType);
    damage = damageInfo->target->MeleeDamageBonusTaken(this, damage, damageInfo->attackType);

    // Calculate armor reduction
    uint32 armor_affected_damage = CalcNotIgnoreDamageReduction(damage, damageInfo->damageSchoolMask);
    damageInfo->damage = damage - armor_affected_damage + CalcArmorReducedDamage(damageInfo->target, armor_affected_damage);
    damageInfo->cleanDamage += damage - damageInfo->damage;

    damageInfo->hitOutCome = RollMeleeOutcomeAgainst(damageInfo->target, damageInfo->attackType);

    // Disable parry or dodge for ranged attack
    if (damageInfo->attackType == RANGED_ATTACK)
    {
        if (damageInfo->hitOutCome == MELEE_HIT_PARRY)
        {
            damageInfo->hitOutCome = MELEE_HIT_NORMAL;
        }
        if (damageInfo->hitOutCome == MELEE_HIT_DODGE)
        {
            damageInfo->hitOutCome = MELEE_HIT_MISS;
        }
    }

    switch (damageInfo->hitOutCome)
    {
        case MELEE_HIT_EVADE:
        {
            damageInfo->HitInfo    |= HITINFO_MISS | HITINFO_SWINGNOHITSOUND;
            damageInfo->TargetState = VICTIMSTATE_EVADES;

            damageInfo->procEx |= PROC_EX_EVADE;
            damageInfo->damage = 0;
            damageInfo->cleanDamage = 0;
            return;
        }
        case MELEE_HIT_MISS:
        {
            damageInfo->HitInfo    |= HITINFO_MISS;
            damageInfo->TargetState = VICTIMSTATE_UNAFFECTED;

            damageInfo->procEx |= PROC_EX_MISS;
            damageInfo->damage = 0;
            damageInfo->cleanDamage = 0;
            break;
        }
        case MELEE_HIT_NORMAL:
            damageInfo->TargetState = VICTIMSTATE_NORMAL;
            damageInfo->procEx |= PROC_EX_NORMAL_HIT;
            break;
        case MELEE_HIT_CRIT:
        {
            damageInfo->HitInfo     |= HITINFO_CRITICALHIT;
            damageInfo->TargetState  = VICTIMSTATE_NORMAL;

            damageInfo->procEx |= PROC_EX_CRITICAL_HIT;
            // Crit bonus calc
            damageInfo->damage += damageInfo->damage;

            // Apply SPELL_AURA_MOD_CRIT_DAMAGE_BONUS modifier first
            const int32 bonus = GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS, SPELL_SCHOOL_MASK_NORMAL);
            damageInfo->damage += int32((damageInfo->damage) * float(bonus / 100.0f));

            int32 mod = 0;
            // Apply SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE or SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE
            if (damageInfo->attackType == RANGED_ATTACK)
            {
                mod += damageInfo->target->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE);
            }
            else
            {
                mod += damageInfo->target->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE);
            }

            if (mod != 0)
            {
                damageInfo->damage = int32((damageInfo->damage) * float((100.0f + mod) / 100.0f));
            }

            // Cata 4.0.1: no separate crit-damage resilience reduction; the
            // single GetDamageReduction pass at the "// only from players"
            // block below handles crit damage too.
            break;
        }
        case MELEE_HIT_PARRY:
            damageInfo->TargetState  = VICTIMSTATE_PARRY;
            damageInfo->procEx      |= PROC_EX_PARRY;
            damageInfo->cleanDamage += damageInfo->damage;
            damageInfo->damage = 0;
            break;

        case MELEE_HIT_DODGE:
            damageInfo->TargetState  = VICTIMSTATE_DODGE;
            damageInfo->procEx      |= PROC_EX_DODGE;
            damageInfo->cleanDamage += damageInfo->damage;
            damageInfo->damage = 0;
            break;
        case MELEE_HIT_BLOCK:
        {
            damageInfo->TargetState = VICTIMSTATE_NORMAL;
            damageInfo->HitInfo    |= HITINFO_BLOCK;
            damageInfo->procEx     |= PROC_EX_BLOCK;
            damageInfo->blocked_amount = damageInfo->target->GetShieldBlockDamageValue() * damageInfo->damage / 100.0f;

            // Target has a chance to double the blocked amount if it has SPELL_AURA_MOD_BLOCK_CRIT_CHANCE
            if (roll_chance_i(pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_CRIT_CHANCE)))
            {
                damageInfo->blocked_amount *= 2;
            }

            if (damageInfo->blocked_amount >= damageInfo->damage)
            {
                damageInfo->TargetState = VICTIMSTATE_BLOCKS;
                damageInfo->blocked_amount = damageInfo->damage;
                damageInfo->procEx |= PROC_EX_FULL_BLOCK;
            }
            else
            {
                damageInfo->procEx |= PROC_EX_NORMAL_HIT;    // Partial blocks can still cause attacker procs
            }

            damageInfo->damage      -= damageInfo->blocked_amount;
            damageInfo->cleanDamage += damageInfo->blocked_amount;
            break;
        }
        case MELEE_HIT_GLANCING:
        {
            damageInfo->HitInfo |= HITINFO_GLANCING;
            damageInfo->TargetState = VICTIMSTATE_NORMAL;
            damageInfo->procEx |= PROC_EX_NORMAL_HIT;

            // Cata 4.0.1: flat 10%-per-level-diff reduction, capped at 3 levels (boss = -30%).
            // Class- and weapon-skill-agnostic; the random damage window was removed in 4.0.1.
            int32 leveldif = int32(damageInfo->target->getLevel()) - int32(getLevel());
            if (leveldif > 3)
            {
                leveldif = 3;
            }
            float reducePercent = 1.0f - leveldif * 0.1f;

            damageInfo->cleanDamage += damageInfo->damage - uint32(reducePercent * damageInfo->damage);
            damageInfo->damage = uint32(reducePercent * damageInfo->damage);
            break;
        }
        case MELEE_HIT_CRUSHING:
        {
            damageInfo->HitInfo     |= HITINFO_CRUSHING;
            damageInfo->TargetState  = VICTIMSTATE_NORMAL;
            damageInfo->procEx |= PROC_EX_NORMAL_HIT;
            // 150% normal damage
            damageInfo->damage += (damageInfo->damage / 2);
            break;
        }
        default:

            break;
    }

    // only from players
    if (GetTypeId() == TYPEID_PLAYER)
    {
        uint32 reduction_affected_damage = CalcNotIgnoreDamageReduction(damageInfo->damage, damageInfo->damageSchoolMask);
        uint32 resilienceReduction = pVictim->GetDamageReduction(reduction_affected_damage);
        damageInfo->damage      -= resilienceReduction;
        damageInfo->cleanDamage += resilienceReduction;
    }

    // Calculate absorb resist
    if (int32(damageInfo->damage) > 0)
    {
        damageInfo->procVictim |= PROC_FLAG_TAKEN_ANY_DAMAGE;

        // Calculate absorb & resists
        uint32 absorb_affected_damage = CalcNotIgnoreAbsorbDamage(damageInfo->damage, damageInfo->damageSchoolMask);
        damageInfo->target->CalculateDamageAbsorbAndResist(this, damageInfo->damageSchoolMask, DIRECT_DAMAGE, absorb_affected_damage, &damageInfo->absorb, &damageInfo->resist, true);
        damageInfo->damage -= damageInfo->absorb + damageInfo->resist;
        if (damageInfo->absorb)
        {
            damageInfo->HitInfo |= HITINFO_ABSORB;
            damageInfo->procEx |= PROC_EX_ABSORB;
        }
        if (damageInfo->resist)
        {
            damageInfo->HitInfo |= HITINFO_RESIST;
        }
    }
    else // Umpossible get negative result but....
    {
        damageInfo->damage = 0;
    }
}

/**
 * @brief Applies prepared melee damage and related proc logic.
 *
 * @param damageInfo The prepared melee damage information.
 * @param durabilityLoss True to apply durability loss rules.
 */
void Unit::DealMeleeDamage(CalcDamageInfo* damageInfo, bool durabilityLoss)
{
    if (damageInfo == 0)
    {
        return;
    }
    Unit* pVictim = damageInfo->target;

    if (!this || !pVictim)
    {
        return;
    }

    if (!pVictim->IsAlive() || pVictim->IsTaxiFlying() || (pVictim->GetTypeId() == TYPEID_UNIT && ((Creature*)pVictim)->IsInEvadeMode()))
    {
        return;
    }

    // You don't lose health from damage taken from another player while in a sanctuary
    // You still see it in the combat log though
    if (!IsAllowedDamageInArea(pVictim))
    {
        return;
    }

    // Hmmmm dont like this emotes client must by self do all animations
    if (damageInfo->HitInfo & HITINFO_CRITICALHIT)
    {
        pVictim->HandleEmoteCommand(EMOTE_ONESHOT_WOUNDCRITICAL);
    }
    if (damageInfo->blocked_amount && damageInfo->TargetState != VICTIMSTATE_BLOCKS)
    {
        pVictim->HandleEmoteCommand(EMOTE_ONESHOT_PARRYSHIELD);
    }

    // This seems to reduce the victims time until next attack if your attack was parried
    if (damageInfo->TargetState == VICTIMSTATE_PARRY)
    {
        if (pVictim->GetTypeId() != TYPEID_UNIT ||
            !(((Creature*)pVictim)->GetCreatureInfo()->ExtraFlags & CREATURE_FLAG_EXTRA_NO_PARRY_HASTEN))
        {
            // Get attack timers
            float offtime = float(pVictim->getAttackTimer(OFF_ATTACK));
            float basetime = float(pVictim->getAttackTimer(BASE_ATTACK));
            // Reduce attack time
            if (pVictim->haveOffhandWeapon() && offtime < basetime)
            {
                float percent20 = pVictim->GetAttackTime(OFF_ATTACK) * 0.20f;
                float percent60 = 3.0f * percent20;
                if (offtime > percent20 && offtime <= percent60)
                {
                    pVictim->setAttackTimer(OFF_ATTACK, uint32(percent20));
                }
                else if (offtime > percent60)
                {
                    offtime -= 2.0f * percent20;
                    pVictim->setAttackTimer(OFF_ATTACK, uint32(offtime));
                }
            }
            else
            {
                float percent20 = pVictim->GetAttackTime(BASE_ATTACK) * 0.20f;
                float percent60 = 3.0f * percent20;
                if (basetime > percent20 && basetime <= percent60)
                {
                    pVictim->setAttackTimer(BASE_ATTACK, uint32(percent20));
                }
                else if (basetime > percent60)
                {
                    basetime -= 2.0f * percent20;
                    pVictim->setAttackTimer(BASE_ATTACK, uint32(basetime));
                }
            }
        }
    }

    // Call default DealDamage
    CleanDamage cleanDamage(damageInfo->cleanDamage, damageInfo->attackType, damageInfo->hitOutCome);
    DealDamage(pVictim, damageInfo->damage, &cleanDamage, DIRECT_DAMAGE, damageInfo->damageSchoolMask, NULL, durabilityLoss);

    // If this is a creature and it attacks from behind it has a probability to daze it's victim
    if ((damageInfo->hitOutCome == MELEE_HIT_CRIT || damageInfo->hitOutCome == MELEE_HIT_CRUSHING || damageInfo->hitOutCome == MELEE_HIT_NORMAL || damageInfo->hitOutCome == MELEE_HIT_GLANCING) &&
        GetTypeId() != TYPEID_PLAYER && !((Creature*)this)->GetCharmerOrOwnerGuid() && !pVictim->HasInArc(M_PI_F, this))
    {
        // -probability is between 0% and 40%
        // 20% base chance
        float Probability = 20.0f;

        // there is a newbie protection, at level 10 just 7% base chance; assuming linear function
        if (pVictim->getLevel() < 30)
        {
            Probability = 0.65f * pVictim->getLevel() + 0.5f;
        }

        uint32 VictimDefense = pVictim->GetMaxSkillValueForLevel(this);
        uint32 AttackerMeleeSkill = GetMaxSkillValueForLevel();

        Probability *= AttackerMeleeSkill / (float)VictimDefense;

        if (Probability > 40.0f)
        {
            Probability = 40.0f;
        }

        if (roll_chance_f(Probability))
        {
            CastSpell(pVictim, 1604, true);
        }
    }

    // If not miss
    if (!(damageInfo->HitInfo & HITINFO_MISS))
    {
        // on weapon hit casts
        if (GetTypeId() == TYPEID_PLAYER && pVictim->IsAlive())
        {
            ((Player*)this)->CastItemCombatSpell(pVictim, damageInfo->attackType);
        }

        // victim's damage shield
        std::set<Aura*> alreadyDone;
        AuraList const& vDamageShields = pVictim->GetAurasByType(SPELL_AURA_DAMAGE_SHIELD);
        for (AuraList::const_iterator i = vDamageShields.begin(); i != vDamageShields.end();)
        {
            if (alreadyDone.find(*i) == alreadyDone.end())
            {
                alreadyDone.insert(*i);
                uint32 damage = (*i)->GetModifier()->m_amount;
                SpellEntry const* i_spellProto = (*i)->GetSpellProto();

                // Calculate absorb and resist for damage shield
                uint32 absorb = 0;
                uint32 resist = 0;
                CalculateDamageAbsorbAndResist(pVictim, GetSpellSchoolMask(i_spellProto), SPELL_DIRECT_DAMAGE, damage, &absorb, &resist);
                damage -= std::min(damage, absorb + resist);

                pVictim->DealDamageMods(this, damage, NULL);

                uint32 targetHealth = GetHealth();
                uint32 overkill = damage > targetHealth ? damage - targetHealth : 0;

                WorldPacket data(SMSG_SPELLDAMAGESHIELD, (8 + 8 + 4 + 4 + 4 + 4));
                data << pVictim->GetObjectGuid();
                data << GetObjectGuid();
                data << uint32(i_spellProto->Id);
                data << uint32(damage);                  // Damage
                data << uint32(overkill);                // Overkill
                data << uint32(i_spellProto->SchoolMask);
                data << uint32(resist);                  // Resist
                pVictim->SendMessageToSet(&data, true);

                pVictim->DealDamage(this, damage, 0, SPELL_DIRECT_DAMAGE, GetSpellSchoolMask(i_spellProto), i_spellProto, true);

                i = vDamageShields.begin();
            }
            else
            {
                ++i;
            }
        }
    }
}
