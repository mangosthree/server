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

#include "SpellMgr.h"
#include "ObjectMgr.h"
#include "SpellAuraDefines.h"
#include "ProgressBar.h"
#include "DBCStores.h"
#include "SQLStorages.h"
#include "World.h"
#include "Chat.h"
#include "Spell.h"
#include "BattleGround/BattleGroundMgr.h"
#include "MapManager.h"
#include "Unit.h"

/**
 * @brief Checks whether a skill line is a primary profession.
 *
 * @param skill The skill line id.
 * @return true if the skill belongs to the profession category; otherwise false.
 */
bool IsPrimaryProfessionSkill(uint32 skill)
{
    SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(skill);
    if (!pSkill)
    {
        return false;
    }

    if (pSkill->categoryId != SKILL_CATEGORY_PROFESSION)
    {
        return false;
    }

    return true;
}

/**
 * @brief Initializes the spell manager.
 */
SpellMgr::SpellMgr()
{
}

/**
 * @brief Destroys the spell manager.
 */
SpellMgr::~SpellMgr()
{
}

/**
 * @brief Returns the global SpellMgr singleton instance.
 *
 * @return Reference to the shared spell manager.
 */
SpellMgr& SpellMgr::Instance()
{
    static SpellMgr spellMgr;
    return spellMgr;
}

/**
 * @brief Returns the base duration of a spell.
 *
 * @param spellInfo The spell entry.
 * @return The base duration in milliseconds, or 0 if unavailable.
 */
int32 GetSpellDuration(SpellEntry const* spellInfo)
{
    if (!spellInfo)
    {
        return 0;
    }
    SpellDurationEntry const* du = sSpellDurationStore.LookupEntry(spellInfo->DurationIndex);
    if (!du)
    {
        return 0;
    }
    return (du->Duration[0] == -1) ? -1 : abs(du->Duration[0]);
}

/**
 * @brief Returns the maximum duration of a spell.
 *
 * @param spellInfo The spell entry.
 * @return The maximum duration in milliseconds, or 0 if unavailable.
 */
int32 GetSpellMaxDuration(SpellEntry const* spellInfo)
{
    if (!spellInfo)
    {
        return 0;
    }
    SpellDurationEntry const* du = sSpellDurationStore.LookupEntry(spellInfo->DurationIndex);
    if (!du)
    {
        return 0;
    }
    return (du->Duration[2] == -1) ? -1 : abs(du->Duration[2]);
}

/**
 * @brief Calculates the effective spell duration for a caster.
 *
 * @param spellInfo The spell entry.
 * @param caster The unit casting the spell.
 * @return The adjusted duration in milliseconds.
 */
int32 CalculateSpellDuration(SpellEntry const* spellInfo, Unit const* caster)
{
    int32 duration = GetSpellDuration(spellInfo);

    if (duration != -1 && caster)
    {
        int32 maxduration = GetSpellMaxDuration(spellInfo);

        if (duration != maxduration && caster->GetTypeId() == TYPEID_PLAYER)
        {
            duration += int32((maxduration - duration) * ((Player*)caster)->GetComboPoints() / 5);
        }

        if (Player* modOwner = caster->GetSpellModOwner())
        {
            modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_DURATION, duration);

            if (duration < 0)
            {
                duration = 0;
            }
        }
    }

    return duration;
}

/**
 * @brief Returns the effective cast time of a spell.
 *
 * @param spellInfo The spell entry.
 * @param spell The spell instance, if available.
 * @return The cast time in milliseconds.
 */
uint32 GetSpellCastTime(SpellEntry const* spellInfo, Spell const* spell)
{
    if (spell)
    {
        // some triggered spells have data only usable for client
        if (spell->IsTriggeredSpellWithRedundentCastTime())
        {
            return 0;
        }

        // spell targeted to non-trading trade slot item instant at trade success apply
        if (spell->GetCaster()->GetTypeId() == TYPEID_PLAYER)
            if (TradeData* my_trade = ((Player*)(spell->GetCaster()))->GetTradeData())
                if (Item* nonTrade = my_trade->GetTraderData()->GetItem(TRADE_SLOT_NONTRADED))
                    if (nonTrade == spell->m_targets.getItemTarget())
                    {
                        return 0;
                    }
    }

    uint32 castTime = 0;
    SpellScalingEntry const* spellScalingEntry = spellInfo->GetSpellScaling();
    if (spell && spellScalingEntry && (spell->GetCaster()->GetTypeId() == TYPEID_PLAYER || spell->GetCaster()->GetObjectGuid().IsPet()))
    {
        uint32 level = spell->GetCaster()->getLevel();
        if (level == 1)
        {
            castTime = uint32(spellScalingEntry->castTimeMin);
        }
        else if (level < uint32(spellScalingEntry->castScalingMaxLevel))
            castTime = uint32(spellScalingEntry->castTimeMin + float(level - 1) *
                (spellScalingEntry->castTimeMax - spellScalingEntry->castTimeMin) / (spellScalingEntry->castScalingMaxLevel - 1));
        else
        {
            castTime = uint32(spellScalingEntry->castTimeMax);
        }
    }
    else if (SpellCastTimesEntry const* spellCastTimeEntry = sSpellCastTimesStore.LookupEntry(spellInfo->CastingTimeIndex))
    {
        if (spell)
        {
            uint32 level = spell->GetCaster()->getLevel();
            if (SpellLevelsEntry const* levelsEntry = spellInfo->GetSpellLevels())
            {
                if (levelsEntry->maxLevel)
                {
                    level = std::min(level, levelsEntry->maxLevel);
                }
                level = std::max(level, levelsEntry->baseLevel) - levelsEntry->baseLevel;
            }

            // currently only profession spells have CastTimePerLevel data filled, always negative
            castTime = uint32(spellCastTimeEntry->CastTime + spellCastTimeEntry->CastTimePerLevel * level);
        }
        else
        {
            castTime = uint32(spellCastTimeEntry->CastTime);
        }

        if (castTime < uint32(spellCastTimeEntry->MinCastTime))
        {
            castTime = uint32(spellCastTimeEntry->MinCastTime);
        }
    }
    else
        // not all spells have cast time index and this is all is pasiive abilities
        return 0;

    if (spell)
    {
        if (Player* modOwner = spell->GetCaster()->GetSpellModOwner())
        {
            modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_CASTING_TIME, castTime, spell);
        }

        if (!spellInfo->HasAttribute(SPELL_ATTR_ABILITY) && !spellInfo->HasAttribute(SPELL_ATTR_TRADESPELL))
        {
            castTime = uint32(castTime * spell->GetCaster()->GetFloatValue(UNIT_MOD_CAST_SPEED));
        }
        else
        {
            if (spell->IsRangedSpell() && !spell->IsAutoRepeat())
            {
                castTime = uint32(castTime * spell->GetCaster()->m_modAttackSpeedPct[RANGED_ATTACK]);
            }
        }
    }

    if (spellInfo->HasAttribute(SPELL_ATTR_RANGED) && (!spell || !spell->IsAutoRepeat()))
    {
        castTime += 500;
    }

    return (castTime > 0) ? uint32(castTime) : 0;
}

/**
 * @brief Calculates the cast time value used for spell bonus coefficients.
 *
 * @param spellProto The spell entry.
 * @param damagetype The damage effect type being evaluated.
 * @return The normalized cast time in milliseconds.
 */
uint32 GetSpellCastTimeForBonus(SpellEntry const* spellProto, DamageEffectType damagetype)
{
    uint32 CastingTime = !IsChanneledSpell(spellProto) ? GetSpellCastTime(spellProto) : GetSpellDuration(spellProto);

    if (CastingTime > 7000)
    {
        CastingTime = 7000;
    }
    if (CastingTime < 1500)
    {
        CastingTime = 1500;
    }

    if (damagetype == DOT && !IsChanneledSpell(spellProto))
    {
        CastingTime = 3500;
    }

    int32 overTime    = 0;
    uint8 effects     = 0;
    bool DirectDamage = false;
    bool AreaEffect   = false;

    for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        SpellEffectEntry const* spellEffect = spellProto->GetSpellEffect(SpellEffectIndex(i));
        if (!spellEffect)
        {
            continue;
        }
        if (IsAreaEffectTarget(Targets(spellEffect->EffectImplicitTargetA)) || IsAreaEffectTarget(Targets(spellEffect->EffectImplicitTargetB)))
        {
            AreaEffect = true;
        }
    }

    for (uint32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        SpellEffectEntry const* spellEffect = spellProto->GetSpellEffect(SpellEffectIndex(i));
        if (!spellEffect)
        {
            continue;
        }
        switch (spellEffect->Effect)
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_POWER_DRAIN:
            case SPELL_EFFECT_HEALTH_LEECH:
            case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
            case SPELL_EFFECT_POWER_BURN:
            case SPELL_EFFECT_HEAL:
                DirectDamage = true;
                break;
            case SPELL_EFFECT_APPLY_AURA:
                switch (spellEffect->EffectApplyAuraName)
                {
                    case SPELL_AURA_PERIODIC_DAMAGE:
                    case SPELL_AURA_PERIODIC_HEAL:
                    case SPELL_AURA_PERIODIC_LEECH:
                        if (GetSpellDuration(spellProto))
                        {
                            overTime = GetSpellDuration(spellProto);
                        }
                        break;
                        // Penalty for additional effects
                    case SPELL_AURA_DUMMY:
                        ++effects;
                        break;
                    case SPELL_AURA_MOD_DECREASE_SPEED:
                        ++effects;
                        break;
                    case SPELL_AURA_MOD_CONFUSE:
                    case SPELL_AURA_MOD_STUN:
                    case SPELL_AURA_MOD_ROOT:
                        // -10% per effect
                        effects += 2;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    // Combined Spells with Both Over Time and Direct Damage
    if (overTime > 0 && CastingTime > 0 && DirectDamage)
    {
        // mainly for DoTs which are 3500 here otherwise
        uint32 OriginalCastTime = GetSpellCastTime(spellProto);
        if (OriginalCastTime > 7000)
        {
            OriginalCastTime = 7000;
        }
        if (OriginalCastTime < 1500)
        {
            OriginalCastTime = 1500;
        }
        // Portion to Over Time
        float PtOT = (overTime / 15000.0f) / ((overTime / 15000.0f) + (OriginalCastTime / 3500.0f));

        if (damagetype == DOT)
        {
            CastingTime = uint32(CastingTime * PtOT);
        }
        else if (PtOT < 1.0f)
        {
            CastingTime  = uint32(CastingTime * (1 - PtOT));
        }
        else
        {
            CastingTime = 0;
        }
    }

    // Area Effect Spells receive only half of bonus
    if (AreaEffect)
    {
        CastingTime /= 2;
    }

    // 50% for damage and healing spells for leech spells from damage bonus and 0% from healing
    for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
    {
        SpellEffectEntry const* spellEffect = spellProto->GetSpellEffect(SpellEffectIndex(j));
        if (!spellEffect)
        {
            continue;
        }
        if (spellEffect->Effect == SPELL_EFFECT_HEALTH_LEECH ||
            spellEffect->Effect == SPELL_EFFECT_APPLY_AURA && spellEffect->EffectApplyAuraName == SPELL_AURA_PERIODIC_LEECH)
        {
            CastingTime /= 2;
            break;
        }
    }

    // -5% of total per any additional effect (multiplicative)
    for (int i = 0; i < effects; ++i)
    {
        CastingTime *= 0.95f;
    }

    return CastingTime;
}

/**
 * @brief Calculates the maximum number of periodic ticks for a spell.
 *
 * @param spellInfo The spell entry.
 * @return The maximum tick count.
 */
uint16 GetSpellAuraMaxTicks(SpellEntry const* spellInfo)
{
    int32 DotDuration = GetSpellDuration(spellInfo);
    if (DotDuration == 0)
    {
        return 1;
    }

    // 200% limit
    if (DotDuration > 30000)
    {
        DotDuration = 30000;
    }

    for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
    {
        SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(SpellEffectIndex(j));
        if (!spellEffect)
        {
            continue;
        }
        if (spellEffect->Effect == SPELL_EFFECT_APPLY_AURA && (
            spellEffect->EffectApplyAuraName == SPELL_AURA_PERIODIC_DAMAGE ||
            spellEffect->EffectApplyAuraName == SPELL_AURA_PERIODIC_HEAL ||
            spellEffect->EffectApplyAuraName == SPELL_AURA_PERIODIC_LEECH) )
        {
            if (spellEffect->EffectAmplitude != 0)
            {
                return DotDuration / spellEffect->EffectAmplitude;
            }
            break;
        }
    }

    return 6;
}

/**
 * @brief Calculates the maximum number of periodic ticks for a spell id.
 *
 * @param spellId The spell id.
 * @return The maximum tick count.
 */
uint16 GetSpellAuraMaxTicks(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        sLog.outError("GetSpellAuraMaxTicks: Spell %u not exist!", spellId);
        return 1;
    }

    return GetSpellAuraMaxTicks(spellInfo);
}

/**
 * @brief Calculates the default bonus coefficient for a spell effect.
 *
 * @param spellProto The spell entry.
 * @param damagetype The damage effect type being evaluated.
 * @return The default coefficient value.
 */
float CalculateDefaultCoefficient(SpellEntry const* spellProto, DamageEffectType const damagetype)
{
    // Damage over Time spells bonus calculation
    float DotFactor = 1.0f;
    if (damagetype == DOT)
    {
        if (!IsChanneledSpell(spellProto))
        {
            DotFactor = GetSpellDuration(spellProto) / 15000.0f;
        }

        if (uint16 DotTicks = GetSpellAuraMaxTicks(spellProto))
        {
            DotFactor /= DotTicks;
        }
    }

    // Distribute Damage over multiple effects, reduce by AoE
    float coeff = GetSpellCastTimeForBonus(spellProto, damagetype) / 3500.0f;

    return coeff * DotFactor;
}

/**
 * @brief Determines which weapon attack type a spell uses.
 *
 * @param spellInfo The spell entry.
 * @return The associated weapon attack type.
 */
WeaponAttackType GetWeaponAttackType(SpellEntry const* spellInfo)
{
    if (!spellInfo)
    {
        return BASE_ATTACK;
    }

    switch (spellInfo->GetDmgClass())
    {
        case SPELL_DAMAGE_CLASS_MELEE:
            if (spellInfo->HasAttribute(SPELL_ATTR_EX3_REQ_OFFHAND))
            {
                return OFF_ATTACK;
            }
            else
            {
                return BASE_ATTACK;
            }
            break;
        case SPELL_DAMAGE_CLASS_RANGED:
            return RANGED_ATTACK;
            break;
        default:
            // Wands
            if (spellInfo->HasAttribute(SPELL_ATTR_EX2_AUTOREPEAT_FLAG))
            {
                return RANGED_ATTACK;
            }
            else
            {
                return BASE_ATTACK;
            }
            break;
    }
}

/**
 * @brief Checks whether a spell id is passive.
 *
 * @param spellId The spell id.
 * @return true if the spell is passive; otherwise false.
 */
bool IsPassiveSpell(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        return false;
    }
    return IsPassiveSpell(spellInfo);
}

/**
 * @brief Checks whether a spell entry is passive.
 *
 * @param spellInfo The spell entry.
 * @return true if the spell is passive; otherwise false.
 */
bool IsPassiveSpell(SpellEntry const* spellInfo)
{
    return spellInfo->HasAttribute(SPELL_ATTR_PASSIVE);
}

/**
 * @brief Checks whether two spells cannot stack because of matching aura data.
 *
 * @param spellId_1 The first spell id.
 * @param spellId_2 The second spell id.
 * @return true if the aura data conflicts; otherwise false.
 */
bool IsNoStackAuraDueToAura(uint32 spellId_1, uint32 spellId_2)
{
    SpellEntry const* spellInfo_1 = sSpellStore.LookupEntry(spellId_1);
    SpellEntry const* spellInfo_2 = sSpellStore.LookupEntry(spellId_2);
    if (!spellInfo_1 || !spellInfo_2)
    {
        return false;
    }
    if (spellInfo_1->Id == spellId_2)
    {
        return false;
    }

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        SpellEffectEntry const* effect_1 = spellInfo_1->GetSpellEffect(SpellEffectIndex(i));

        for (int32 j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            SpellEffectEntry const* effect_2 = spellInfo_2->GetSpellEffect(SpellEffectIndex(j));
            if (!effect_1 || !effect_2)
            {
                continue;
            }
            if (effect_1->Effect == effect_2->Effect
                && effect_1->EffectApplyAuraName == effect_2->EffectApplyAuraName
                && effect_1->EffectMiscValue == effect_2->EffectMiscValue
                && effect_1->EffectItemType == effect_2->EffectItemType
                && (effect_1->Effect != 0 || effect_1->EffectApplyAuraName != 0 ||
                    effect_1->EffectMiscValue != 0 || effect_1->EffectItemType != 0))
                return true;
        }
    }

    return false;
}

/**
 * @brief Compares the effect strength of two aura ranks.
 *
 * @param spellId_1 The first spell id.
 * @param spellId_2 The second spell id.
 * @return A positive or negative difference value, or 0 if not comparable.
 */
int32 CompareAuraRanks(uint32 spellId_1, uint32 spellId_2)
{
    SpellEntry const* spellInfo_1 = sSpellStore.LookupEntry(spellId_1);
    SpellEntry const* spellInfo_2 = sSpellStore.LookupEntry(spellId_2);
    if (!spellInfo_1 || !spellInfo_2)
    {
        return 0;
    }
    if (spellId_1 == spellId_2)
    {
        return 0;
    }

    for (int32 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        SpellEffectEntry const* spellEffect_1 = spellInfo_1->GetSpellEffect(SpellEffectIndex(i));
        SpellEffectEntry const* spellEffect_2 = spellInfo_2->GetSpellEffect(SpellEffectIndex(i));
        if (!spellEffect_1 || !spellEffect_2)
        {
            continue;
        }

        if (spellEffect_1->Effect != 0 && spellEffect_2->Effect != 0 && spellEffect_1->Effect == spellEffect_2->Effect)
        {
            int32 diff = spellEffect_1->EffectBasePoints - spellEffect_2->EffectBasePoints;
            if (spellInfo_1->CalculateSimpleValue(SpellEffectIndex(i)) < 0 && spellInfo_2->CalculateSimpleValue(SpellEffectIndex(i)) < 0)
            {
                return -diff;
            }
            else
            {
                return diff;
            }
        }
    }
    return 0;
}

/**
 * @brief Classifies a spell into a spell-specific category.
 *
 * @param spellId The spell id.
 * @return The derived spell-specific classification.
 */
SpellSpecific GetSpellSpecific(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        return SPELL_NORMAL;
    }

    SpellClassOptionsEntry const* classOpt = spellInfo->GetSpellClassOptions();
    SpellInterruptsEntry const* interrupts = spellInfo->GetSpellInterrupts();

    switch(spellInfo->GetSpellFamilyName())
    {
        case SPELLFAMILY_GENERIC:
        {
            // Food / Drinks (mostly)
            if (interrupts && interrupts->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
            {
                bool food = false;
                bool drink = false;
                for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
                {
                    SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(SpellEffectIndex(i));
                    if (!spellEffect)
                    {
                        continue;
                    }

                    switch(spellEffect->EffectApplyAuraName)
                    {
                            // Food
                        case SPELL_AURA_MOD_REGEN:
                        case SPELL_AURA_OBS_MOD_HEALTH:
                            food = true;
                            break;
                            // Drink
                        case SPELL_AURA_MOD_POWER_REGEN:
                        case SPELL_AURA_OBS_MOD_MANA:
                            drink = true;
                            break;
                        default:
                            break;
                    }
                }

                if (food && drink)
                {
                    return SPELL_FOOD_AND_DRINK;
                }
                else if (food)
                {
                    return SPELL_FOOD;
                }
                else if (drink)
                {
                    return SPELL_DRINK;
                }
            }
            else
            {
                // Well Fed buffs (must be exclusive with Food / Drink replenishment effects, or else Well Fed will cause them to be removed)
                // SpellIcon 2560 is Spell 46687, does not have this flag
                if (spellInfo->HasAttribute(SPELL_ATTR_EX2_FOOD_BUFF) || spellInfo->SpellIconID == 2560)
                {
                    return SPELL_WELL_FED;
                }
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            // family flags 18(Molten), 25(Frost/Ice), 28(Mage)
            if (classOpt && classOpt->SpellFamilyFlags & UI64LIT(0x12040000))
            {
                return SPELL_MAGE_ARMOR;
            }

            SpellEffectEntry const* mageSpellEffect = spellInfo->GetSpellEffect(EFFECT_INDEX_0);
            if (classOpt && (classOpt->SpellFamilyFlags & UI64LIT(0x1000000)) && mageSpellEffect && mageSpellEffect->EffectApplyAuraName == SPELL_AURA_MOD_CONFUSE)
            {
                return SPELL_MAGE_POLYMORPH;
            }

            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            if (classOpt && classOpt->SpellFamilyFlags & UI64LIT(0x00008000010000))
            {
                return SPELL_POSITIVE_SHOUT;
            }

            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // only warlock curses have this
            if (spellInfo->GetDispel() == DISPEL_CURSE)
            {
                return SPELL_CURSE;
            }

            // Warlock (Demon Armor | Demon Skin | Fel Armor)
            if (spellInfo->IsFitToFamilyMask(UI64LIT(0x2000002000000000), 0x00000010))
            {
                return SPELL_WARLOCK_ARMOR;
            }

            // Unstable Affliction | Immolate
            if (spellInfo->IsFitToFamilyMask(UI64LIT(0x0000010000000004)))
            {
                return SPELL_UA_IMMOLATE;
            }
            break;
        }
        // Need Fix
        case SPELLFAMILY_PRIEST:
        {
            // "Well Fed" buff from Blessed Sunfruit, Blessed Sunfruit Juice, Alterac Spring Water
            if (spellInfo->HasAttribute(SPELL_ATTR_CASTABLE_WHILE_SITTING) &&
                (interrupts && interrupts->InterruptFlags & SPELL_INTERRUPT_FLAG_AUTOATTACK) &&
                (spellInfo->SpellIconID == 52 || spellInfo->SpellIconID == 79))
                {
                    return SPELL_WELL_FED;
                }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // only hunter stings have this
            if (spellInfo->GetDispel() == DISPEL_POISON)
            {
                return SPELL_STING;
            }

            // only hunter aspects have this
            if (spellInfo->IsFitToFamilyMask(UI64LIT(0x0044000000380000), 0x00001010))
            {
                return SPELL_ASPECT;
            }

            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            if (IsSealSpell(spellInfo))
            {
                return SPELL_SEAL;
            }

            if (spellInfo->IsFitToFamilyMask(UI64LIT(0x0000000011010002)))
            {
                return SPELL_BLESSING;
            }

            if (spellInfo->IsFitToFamilyMask(UI64LIT(0x0000000000002190)))
            {
                return SPELL_HAND;
            }

            // skip Heart of the Crusader that have also same spell family mask
            if (spellInfo->IsFitToFamilyMask(UI64LIT(0x00000820180400)) && spellInfo->HasAttribute(SPELL_ATTR_EX3_UNK9) && (spellInfo->SpellIconID != 237))
            {
                return SPELL_JUDGEMENT;
            }

            // only paladin auras have this (for palaldin class family)
            if (spellInfo->IsFitToFamilyMask(UI64LIT(0x0000000000000000), 0x00000020))
            {
                return SPELL_AURA;
            }

            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            if (IsElementalShield(spellInfo))
            {
                return SPELL_ELEMENTAL_SHIELD;
            }

            break;
        }

        case SPELLFAMILY_POTION:
            return sSpellMgr.GetSpellElixirSpecific(spellInfo->Id);

        case SPELLFAMILY_DEATHKNIGHT:
            if (spellInfo->GetCategory() == 47)
            {
                return SPELL_PRESENCE;
            }
            break;
    }

    // Tracking spells (exclude Well Fed, some other always allowed cases)
    if ((IsSpellHaveAura(spellInfo, SPELL_AURA_TRACK_CREATURES) ||
            IsSpellHaveAura(spellInfo, SPELL_AURA_TRACK_RESOURCES)  ||
            IsSpellHaveAura(spellInfo, SPELL_AURA_TRACK_STEALTHED)) &&
            (spellInfo->HasAttribute(SPELL_ATTR_EX_UNK17) || spellInfo->HasAttribute(SPELL_ATTR_EX6_UNK12)))
         {
             return SPELL_TRACKER;
         }

    // elixirs can have different families, but potion most ofc.
    if (SpellSpecific sp = sSpellMgr.GetSpellElixirSpecific(spellInfo->Id))
    {
        return sp;
    }

    return SPELL_NORMAL;
}

// target not allow have more one spell specific from same caster
bool IsSingleFromSpellSpecificPerTargetPerCaster(SpellSpecific spellSpec1, SpellSpecific spellSpec2)
{
    switch (spellSpec1)
    {
        case SPELL_BLESSING:
        case SPELL_AURA:
        case SPELL_STING:
        case SPELL_CURSE:
        case SPELL_ASPECT:
        case SPELL_POSITIVE_SHOUT:
        case SPELL_JUDGEMENT:
        case SPELL_HAND:
        case SPELL_UA_IMMOLATE:
            return spellSpec1 == spellSpec2;
        default:
            return false;
    }
}

// target not allow have more one ranks from spell from spell specific per target
bool IsSingleFromSpellSpecificSpellRanksPerTarget(SpellSpecific spellSpec1, SpellSpecific spellSpec2)
{
    switch (spellSpec1)
    {
        case SPELL_BLESSING:
        case SPELL_AURA:
        case SPELL_CURSE:
        case SPELL_ASPECT:
        case SPELL_HAND:
            return spellSpec1 == spellSpec2;
        default:
            return false;
    }
}

// target not allow have more one spell specific per target from any caster
bool IsSingleFromSpellSpecificPerTarget(SpellSpecific spellSpec1, SpellSpecific spellSpec2)
{
    switch (spellSpec1)
    {
        case SPELL_SEAL:
        case SPELL_TRACKER:
        case SPELL_WARLOCK_ARMOR:
        case SPELL_MAGE_ARMOR:
        case SPELL_ELEMENTAL_SHIELD:
        case SPELL_MAGE_POLYMORPH:
        case SPELL_PRESENCE:
        case SPELL_WELL_FED:
            return spellSpec1 == spellSpec2;
        case SPELL_BATTLE_ELIXIR:
            return spellSpec2 == SPELL_BATTLE_ELIXIR
                   || spellSpec2 == SPELL_FLASK_ELIXIR;
        case SPELL_GUARDIAN_ELIXIR:
            return spellSpec2 == SPELL_GUARDIAN_ELIXIR
                   || spellSpec2 == SPELL_FLASK_ELIXIR;
        case SPELL_FLASK_ELIXIR:
            return spellSpec2 == SPELL_BATTLE_ELIXIR
                   || spellSpec2 == SPELL_GUARDIAN_ELIXIR
                   || spellSpec2 == SPELL_FLASK_ELIXIR;
        case SPELL_FOOD:
            return spellSpec2 == SPELL_FOOD
                   || spellSpec2 == SPELL_FOOD_AND_DRINK;
        case SPELL_DRINK:
            return spellSpec2 == SPELL_DRINK
                   || spellSpec2 == SPELL_FOOD_AND_DRINK;
        case SPELL_FOOD_AND_DRINK:
            return spellSpec2 == SPELL_FOOD
                   || spellSpec2 == SPELL_DRINK
                   || spellSpec2 == SPELL_FOOD_AND_DRINK;
        default:
            return false;
    }
}

/**
 * @brief Determines whether a target pair is considered positive.
 *
 * @param targetA The primary implicit target type.
 * @param targetB The secondary implicit target type.
 * @return true if the target selection is positive; otherwise false.
 */
bool IsPositiveTarget(uint32 targetA, uint32 targetB)
{
    switch (targetA)
    {
            // non-positive targets
        case TARGET_CHAIN_DAMAGE:
        case TARGET_ALL_ENEMY_IN_AREA:
        case TARGET_ALL_ENEMY_IN_AREA_INSTANT:
        case TARGET_IN_FRONT_OF_CASTER:
        case TARGET_ALL_ENEMY_IN_AREA_CHANNELED:
        case TARGET_CURRENT_ENEMY_COORDINATES:
        case TARGET_SINGLE_ENEMY:
        case TARGET_IN_FRONT_OF_CASTER_30:
            return false;
            // positive or dependent
        case TARGET_CASTER_COORDINATES:
            return (targetB == TARGET_ALL_PARTY || targetB == TARGET_ALL_FRIENDLY_UNITS_AROUND_CASTER);
        default:
            break;
    }
    if (targetB)
    {
        return IsPositiveTarget(targetB, 0);
    }
    return true;
}

/**
 * @brief Checks whether a target type is explicitly positive.
 *
 * @param targetA The implicit target type.
 * @return true if the target requires an explicit positive target; otherwise false.
 */
bool IsExplicitPositiveTarget(uint32 targetA)
{
    // positive targets that in target selection code expect target in m_targers, so not that auto-select target by spell data by m_caster and etc
    switch (targetA)
    {
        case TARGET_SINGLE_FRIEND:
        case TARGET_SINGLE_PARTY:
        case TARGET_CHAIN_HEAL:
        case TARGET_SINGLE_FRIEND_2:
        case TARGET_AREAEFFECT_PARTY_AND_CLASS:
            return true;
        default:
            break;
    }
    return false;
}

/**
 * @brief Checks whether a target type is explicitly negative.
 *
 * @param targetA The implicit target type.
 * @return true if the target requires an explicit negative target; otherwise false.
 */
bool IsExplicitNegativeTarget(uint32 targetA)
{
    // non-positive targets that in target selection code expect target in m_targers, so not that auto-select target by spell data by m_caster and etc
    switch (targetA)
    {
        case TARGET_CHAIN_DAMAGE:
        case TARGET_CURRENT_ENEMY_COORDINATES:
        case TARGET_SINGLE_ENEMY:
            return true;
        default:
            break;
    }
    return false;
}

/**
 * @brief Determines whether a spell effect should be treated as positive.
 *
 * @param spellproto The spell entry.
 * @param effIndex The effect index to evaluate.
 * @return true if the effect is positive; otherwise false.
 */
bool IsPositiveEffect(SpellEntry const* spellproto, SpellEffectIndex effIndex)
{
    SpellEffectEntry const* spellEffect = spellproto->GetSpellEffect(effIndex);

    switch(spellproto->GetSpellEffectIdByIndex(effIndex))
    {
        case SPELL_EFFECT_DUMMY:
            // some explicitly required dummy effect sets
            switch (spellproto->Id)
            {
                case 28441:                                 // AB Effect 000
                    return false;
                case 10258:                                 // Awaken Vault Warder
                case 18153:                                 // Kodo Kombobulator
                case 32312:                                 // Move 1
                case 37388:                                 // Move 2
                case 45863:                                 // Cosmetic - Incinerate to Random Target
                case 49634:                                 // Sergeant's Flare
                case 54530:                                 // Opening
                case 56099:                                 // Throw Ice
                case 58533:                                 // Return to Stormwind
                case 58552:                                 // Return to Orgrimmar
                case 62105:                                 // To'kini's Blowgun
                case 63745:                                 // Sara's Blessing
                case 63747:                                 // Sara's Fervor
                case 64402:                                 // Rocket Strike
                    return true;
                default:
                    break;
            }
            break;
        case SPELL_EFFECT_SCRIPT_EFFECT:
            // some explicitly required script effect sets
            switch (spellproto->Id)
            {
                case 42436:                                 // Drink!
                case 42492:                                 // Cast Energized
                case 46650:                                 // Open Brutallus Back Door
                case 62488:                                 // Activate Construct
                case 64503:                                 // Water
                    return true;
                default:
                    break;
            }
            break;
            // always positive effects (check before target checks that provided non-positive result in some case for positive effects)
        case SPELL_EFFECT_HEAL:
        case SPELL_EFFECT_LEARN_SPELL:
        case SPELL_EFFECT_SKILL_STEP:
        case SPELL_EFFECT_HEAL_PCT:
        case SPELL_EFFECT_ENERGIZE_PCT:
        case SPELL_EFFECT_QUEST_COMPLETE:
        case SPELL_EFFECT_KILL_CREDIT_PERSONAL:
        case SPELL_EFFECT_KILL_CREDIT_GROUP:
            return true;

            // non-positive aura use
        case SPELL_EFFECT_APPLY_AURA:
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
        {
            switch(spellEffect->EffectApplyAuraName)
            {
                case SPELL_AURA_DUMMY:
                {
                    // dummy aura can be positive or negative dependent from casted spell
                    switch (spellproto->Id)
                    {
                        case 13139:                         // net-o-matic special effect
                        case 23182:                         // Mark of Frost
                        case 23445:                         // evil twin
                        case 25040:                         // Mark of Nature
                        case 35679:                         // Protectorate Demolitionist
                        case 37695:                         // Stanky
                        case 38637:                         // Nether Exhaustion (red)
                        case 38638:                         // Nether Exhaustion (green)
                        case 38639:                         // Nether Exhaustion (blue)
                        case 11196:                         // Recently Bandaged
                        case 44689:                         // Relay Race Accept Hidden Debuff - DND
                        case 58600:                         // Restricted Flight Area
                            return false;
                            // some spells have unclear target modes for selection, so just make effect positive
                        case 27184:
                        case 27190:
                        case 27191:
                        case 27201:
                        case 27202:
                        case 27203:
                        case 47669:
                        case 64996:                         // Reorigination
                            return true;
                        default:
                            break;
                    }
                }   break;
                case SPELL_AURA_MOD_DAMAGE_DONE:            // dependent from base point sign (negative -> negative)
                case SPELL_AURA_MOD_RESISTANCE:
                case SPELL_AURA_MOD_STAT:
                case SPELL_AURA_MOD_SKILL:
                case SPELL_AURA_MOD_DODGE_PERCENT:
                case SPELL_AURA_MOD_HEALING_PCT:
                case SPELL_AURA_MOD_HEALING_DONE:
                    if (spellEffect->CalculateSimpleValue() < 0)
                    {
                        return false;
                    }
                    break;
                case SPELL_AURA_MOD_DAMAGE_TAKEN:           // dependent from bas point sign (positive -> negative)
                case SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN:
                    if (spellEffect->CalculateSimpleValue() < 0)
                    {
                        return true;
                    }
                    // let check by target modes (for Amplify Magic cases/etc)
                    break;
                case SPELL_AURA_MOD_SPELL_CRIT_CHANCE:
                case SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT:
                case SPELL_AURA_MOD_DAMAGE_PERCENT_DONE:
                    if (spellEffect->CalculateSimpleValue() > 0)
                    {
                        return true;                        // some expected positive spells have SPELL_ATTR_NEGATIVE or unclear target modes
                    }
                    break;
                case SPELL_AURA_ADD_TARGET_TRIGGER:
                    return true;
                case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
                    if (spellproto->Id != spellEffect->EffectTriggerSpell)
                    {
                        uint32 spellTriggeredId = spellEffect->EffectTriggerSpell;
                        SpellEntry const* spellTriggeredProto = sSpellStore.LookupEntry(spellTriggeredId);

                        if (spellTriggeredProto)
                        {
                            // non-positive targets of main spell return early
                            for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
                            {
                                SpellEffectEntry const* triggerSpellEffect = spellTriggeredProto->GetSpellEffect(SpellEffectIndex(i));
                                if (!triggerSpellEffect)
                                {
                                    continue;
                                }
                                // if non-positive trigger cast targeted to positive target this main cast is non-positive
                                // this will place this spell auras as debuffs
                                if (triggerSpellEffect->Effect &&
                                    IsPositiveTarget(triggerSpellEffect->EffectImplicitTargetA,triggerSpellEffect->EffectImplicitTargetB) &&
                                    !IsPositiveEffect(spellTriggeredProto, SpellEffectIndex(i)))
                                {
                                        return false;
                                }
                            }
                        }
                    }
                    break;
                case SPELL_AURA_PROC_TRIGGER_SPELL:
                    // many positive auras have negative triggered spells at damage for example and this not make it negative (it can be canceled for example)
                    break;
                case SPELL_AURA_MOD_STUN:                   // have positive and negative spells, we can't sort its correctly at this moment.
                    if (effIndex == EFFECT_INDEX_0 && spellproto->GetSpellEffectIdByIndex(EFFECT_INDEX_1) == 0 && spellproto->GetSpellEffectIdByIndex(EFFECT_INDEX_2) == 0)
                    {
                        return false;                        // but all single stun aura spells is negative
                    }

                    // Petrification
                    if (spellproto->Id == 17624)
                    {
                        return false;
                    }
                    break;
                case SPELL_AURA_MOD_PACIFY_SILENCE:
                    switch (spellproto->Id)
                    {
                        case 24740:                         // Wisp Costume
                        case 47585:                         // Dispersion
                            return true;
                        default: break;
                    }
                    return false;
                case SPELL_AURA_MOD_ROOT:
                case SPELL_AURA_MOD_SILENCE:
                case SPELL_AURA_GHOST:
                case SPELL_AURA_PERIODIC_LEECH:
                case SPELL_AURA_MOD_STALKED:
                case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
                case SPELL_AURA_PREVENT_RESURRECTION:
                    return false;
                case SPELL_AURA_PERIODIC_DAMAGE:            // used in positive spells also.
                    // part of negative spell if casted at self (prevent cancel)
                    if (spellEffect->EffectImplicitTargetA == TARGET_SELF ||
                        spellEffect->EffectImplicitTargetA == TARGET_SELF2)
                    {
                        return false;
                    }
                    break;
                case SPELL_AURA_MOD_DECREASE_SPEED:         // used in positive spells also
                    if (spellproto->Id == 37830)            // Repolarized Magneto Sphere
                    {
                        return true;
                    }
                    // part of positive spell if casted at self
                    if ((spellEffect->EffectImplicitTargetA == TARGET_SELF ||
                        spellEffect->EffectImplicitTargetA == TARGET_SELF2) &&
                        spellproto->GetSpellFamilyName() == SPELLFAMILY_GENERIC)
                        return false;
                    // but not this if this first effect (don't found better check)
                    if (spellproto->HasAttribute(SPELL_ATTR_NEGATIVE) && effIndex == EFFECT_INDEX_0)
                    {
                        return false;
                    }
                    break;
                case SPELL_AURA_TRANSFORM:
                    // some spells negative
                    switch (spellproto->Id)
                    {
                        case 36897:                         // Transporter Malfunction (race mutation to horde)
                        case 36899:                         // Transporter Malfunction (race mutation to alliance)
                        case 37097:                         // Crate Disguise
                            return false;
                    }
                    break;
                case SPELL_AURA_MOD_SCALE:
                    // some spells negative
                    switch (spellproto->Id)
                    {
                        case 802:                           // Mutate Bug, wrongly negative by target modes
                        case 38449:                         // Blessing of the Tides
                        case 50312:                         // Unholy Frenzy
                            return true;
                        case 36900:                         // Soul Split: Evil!
                        case 36901:                         // Soul Split: Good
                        case 36893:                         // Transporter Malfunction (decrease size case)
                        case 36895:                         // Transporter Malfunction (increase size case)
                            return false;
                    }
                    break;
                case SPELL_AURA_MECHANIC_IMMUNITY:
                {
                    // non-positive immunities
                    switch(spellEffect->EffectMiscValue)
                    {
                        case MECHANIC_BANDAGE:
                        case MECHANIC_SHIELD:
                        case MECHANIC_MOUNT:
                        case MECHANIC_INVULNERABILITY:
                            return false;
                        default:
                            break;
                    }
                }   break;
                case SPELL_AURA_ADD_FLAT_MODIFIER:          // mods
                case SPELL_AURA_ADD_PCT_MODIFIER:
                {
                    // non-positive mods
                    switch(spellEffect->EffectMiscValue)
                    {
                        case SPELLMOD_COST:                 // dependent from bas point sign (negative -> positive)
                            if (spellproto->CalculateSimpleValue(effIndex) > 0)
                            {
                                return false;
                            }
                            break;
                        default:
                            break;
                    }
                }   break;
                case SPELL_AURA_MOD_MELEE_HASTE:
                {
                    switch (spellproto->Id)
                    {
                        case 38449:                         // Blessing of the Tides
                            return true;
                        default:
                            break;
                    }
                    break;
                }
                case SPELL_AURA_FORCE_REACTION:
                {
                    switch (spellproto->Id)
                    {
                        case 42792:                         // Recently Dropped Flag (prevent cancel)
                        case 46221:                         // Animal Blood
                            return false;
                        default:
                            break;
                    }
                    break;
                }
                case SPELL_AURA_PHASE:
                {
                    switch (spellproto->Id)
                    {
                        case 57508:                         // Insanity (16)
                        case 57509:                         // Insanity (32)
                        case 57510:                         // Insanity (64)
                        case 57511:                         // Insanity (128)
                        case 57512:                         // Insanity (256)
                            return false;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    // non-positive targets
    if (spellEffect && !IsPositiveTarget(spellEffect->EffectImplicitTargetA,spellEffect->EffectImplicitTargetB))
    {
        return false;
    }

    // AttributesEx check
    if (spellproto->HasAttribute(SPELL_ATTR_NEGATIVE))
    {
        return false;
    }

    // ok, positive
    return true;
}

/**
 * @brief Determines whether a spell id is positive.
 *
 * @param spellId The spell id.
 * @return true if all active effects are positive; otherwise false.
 */
bool IsPositiveSpell(uint32 spellId)
{
    SpellEntry const* spellproto = sSpellStore.LookupEntry(spellId);
    if (!spellproto)
    {
        return false;
    }

    return IsPositiveSpell(spellproto);
}

/**
 * @brief Determines whether a spell entry is positive.
 *
 * @param spellproto The spell entry.
 * @return true if all active effects are positive; otherwise false.
 */
bool IsPositiveSpell(SpellEntry const* spellproto)
{
    // spells with at least one negative effect are considered negative
    // some self-applied spells have negative effects but in self casting case negative check ignored.
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (spellproto->GetSpellEffectIdByIndex(SpellEffectIndex(i)) && !IsPositiveEffect(spellproto, SpellEffectIndex(i)))
        {
            return false;
        }
    return true;
}

/**
 * @brief Checks whether a spell is treated as single-target.
 *
 * @param spellInfo The spell entry.
 * @return true if the spell is single-target; otherwise false.
 */
bool IsSingleTargetSpell(SpellEntry const* spellInfo)
{
    // all other single target spells have if it has AttributesEx5
    if (spellInfo->HasAttribute(SPELL_ATTR_EX5_SINGLE_TARGET_SPELL))
    {
        return true;
    }

    // TODO - need found Judgements rule
    switch (GetSpellSpecific(spellInfo->Id))
    {
        case SPELL_JUDGEMENT:
            return true;
        default:
            break;
    }
    // single target triggered spell.
    // Not real client side single target spell, but it' not triggered until prev. aura expired.
    // This is allow store it in single target spells list for caster for spell proc checking
    if (spellInfo->Id == 38324)                             // Regeneration (triggered by 38299 (HoTs on Heals))
    {
        return true;
    }

    return false;
}

/**
 * @brief Checks whether two spells belong to the same single-target family.
 *
 * @param spellInfo1 The first spell entry.
 * @param spellInfo2 The second spell entry.
 * @return true if the spells are treated as equivalent single-target spells; otherwise false.
 */
bool IsSingleTargetSpells(SpellEntry const* spellInfo1, SpellEntry const* spellInfo2)
{
    // TODO - need better check
    // Equal icon and spellfamily
    if ( spellInfo1->GetSpellFamilyName() == spellInfo2->GetSpellFamilyName() &&
        spellInfo1->SpellIconID == spellInfo2->SpellIconID )
    {
        return true;
    }

    SpellSpecific spec1 = GetSpellSpecific(spellInfo1->Id);
    // spell with single target specific types
    switch (spec1)
    {
        case SPELL_JUDGEMENT:
        case SPELL_MAGE_POLYMORPH:
            if (GetSpellSpecific(spellInfo2->Id) == spec1)
            {
                return true;
            }
            break;
        default:
            break;
    }

    return false;
}

/**
 * @brief Returns the cast error produced by the current shapeshift form.
 *
 * @param spellInfo The spell entry.
 * @param form The shapeshift form id.
 * @return The spell cast result for the stance check.
 */
SpellCastResult GetErrorAtShapeshiftedCast(SpellEntry const* spellInfo, uint32 form)
{
    // talents that learn spells can have stance requirements that need ignore
    // (this requirement only for client-side stance show in talent description)
    if ( GetTalentSpellCost(spellInfo->Id) > 0 &&
        (spellInfo->GetSpellEffectIdByIndex(EFFECT_INDEX_0) == SPELL_EFFECT_LEARN_SPELL || spellInfo->GetSpellEffectIdByIndex(EFFECT_INDEX_1) == SPELL_EFFECT_LEARN_SPELL || spellInfo->GetSpellEffectIdByIndex(EFFECT_INDEX_2) == SPELL_EFFECT_LEARN_SPELL) )
    {
        return SPELL_CAST_OK;
    }

    uint32 stanceMask = (form ? 1 << (form - 1) : 0);

    SpellShapeshiftEntry const* shapeShift = spellInfo->GetSpellShapeshift();

    if (shapeShift && stanceMask & shapeShift->StancesNot)  // can explicitly not be casted in this stance
    {
        return SPELL_FAILED_NOT_SHAPESHIFT;
    }

    if (shapeShift && stanceMask & shapeShift->Stances)     // can explicitly be casted in this stance
    {
        return SPELL_CAST_OK;
    }

    bool actAsShifted = false;
    if (form > 0)
    {
        SpellShapeshiftFormEntry const* shapeInfo = sSpellShapeshiftFormStore.LookupEntry(form);
        if (!shapeInfo)
        {
            sLog.outError("GetErrorAtShapeshiftedCast: unknown shapeshift %u", form);
            return SPELL_CAST_OK;
        }
        actAsShifted = !(shapeInfo->flags1 & 1);            // shapeshift acts as normal form for spells
    }

    if (actAsShifted)
    {
        if (spellInfo->HasAttribute(SPELL_ATTR_NOT_SHAPESHIFT)) // not while shapeshifted
        {
            return SPELL_FAILED_NOT_SHAPESHIFT;
        }
        else if (shapeShift && shapeShift->Stances != 0)    // needs other shapeshift
        {
            return SPELL_FAILED_ONLY_SHAPESHIFT;
        }
    }
    else
    {
        // needs shapeshift
        if (!(spellInfo->AttributesEx2 & SPELL_ATTR_EX2_NOT_NEED_SHAPESHIFT) && shapeShift && shapeShift->Stances != 0)
        {
            return SPELL_FAILED_ONLY_SHAPESHIFT;
        }
    }

    return SPELL_CAST_OK;
}

/**
 * @brief Checks whether a proc event definition can be triggered by a proc context.
 *
 * @param spellProcEvent The proc event definition to evaluate.
 * @param EventProcFlag The event flag being tested.
 * @param procSpell The spell that caused the proc, if any.
 * @param procFlags The proc flags of the current event.
 * @param procExtra Additional proc result flags.
 * @return true if the proc event can trigger; otherwise, false.
 */
bool SpellMgr::IsSpellProcEventCanTriggeredBy(SpellProcEventEntry const* spellProcEvent, uint32 EventProcFlag, SpellEntry const* procSpell, uint32 procFlags, uint32 procExtra)
{
    // No extra req need
    uint32 procEvent_procEx = PROC_EX_NONE;

    // check prockFlags for condition
    if ((procFlags & EventProcFlag) == 0)
    {
        return false;
    }

    // Always trigger for this
    if (EventProcFlag & (PROC_FLAG_KILLED | PROC_FLAG_KILL | PROC_FLAG_ON_TRAP_ACTIVATION))
    {
        return true;
    }

    if (spellProcEvent)     // Exist event data
    {
        // Store extra req
        procEvent_procEx = spellProcEvent->procEx;

        // For melee triggers
        if (procSpell == NULL)
        {
            // Check (if set) for school (melee attack have Normal school)
            if (spellProcEvent->schoolMask && (spellProcEvent->schoolMask & SPELL_SCHOOL_MASK_NORMAL) == 0)
            {
                return false;
            }
        }
        else // For spells need check school/spell family/family mask
        {
            // Check (if set) for school
            if (spellProcEvent->schoolMask && (spellProcEvent->schoolMask & procSpell->SchoolMask) == 0)
            {
                return false;
            }

            SpellClassOptionsEntry const* spellClassOptions = procSpell->GetSpellClassOptions();

            // Check (if set) for spellFamilyName
            if (spellProcEvent->spellFamilyName && ((!spellClassOptions && spellProcEvent->spellFamilyName != SPELLFAMILY_GENERIC) || (spellClassOptions && spellProcEvent->spellFamilyName != spellClassOptions->SpellFamilyName)))
            {
                return false;
            }
        }
    }

    // Check for extra req (if none) and hit/crit
    if (procEvent_procEx == PROC_EX_NONE)
    {
        // Don't allow proc from periodic heal if no extra requirement is defined
        if (EventProcFlag & (PROC_FLAG_ON_DO_PERIODIC | PROC_FLAG_ON_TAKE_PERIODIC) && (procExtra & PROC_EX_PERIODIC_POSITIVE))
        {
            return false;
        }

        // No extra req, so can trigger for (damage/healing present) and cast end/hit/crit
        if (procExtra & (PROC_EX_CAST_END | PROC_EX_NORMAL_HIT | PROC_EX_CRITICAL_HIT))
        {
            return true;
        }
    }
    else // all spells hits here only if resist/reflect/immune/evade
    {
        // Exist req for PROC_EX_EX_TRIGGER_ALWAYS
        if (procEvent_procEx & PROC_EX_EX_TRIGGER_ALWAYS)
        {
            return true;
        }
        // Check Extra Requirement like (hit/crit/miss/resist/parry/dodge/block/immune/reflect/absorb and other)
        if (procEvent_procEx & procExtra)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Checks whether one spell is a rank of another spell.
 *
 * @param spellInfo_1 The first spell entry.
 * @param spellId_2 The second spell identifier.
 * @return true if both spells belong to the same rank chain; otherwise, false.
 */
bool SpellMgr::IsRankSpellDueToSpell(SpellEntry const* spellInfo_1, uint32 spellId_2) const
{
    SpellEntry const* spellInfo_2 = sSpellStore.LookupEntry(spellId_2);
    if (!spellInfo_1 || !spellInfo_2)
    {
        return false;
    }
    if (spellInfo_1->Id == spellId_2)
    {
        return false;
    }

    return GetFirstSpellInChain(spellInfo_1->Id) == GetFirstSpellInChain(spellId_2);
}

/**
 * @brief Checks whether ranked spells may coexist in the spell book.
 *
 * @param spellInfo The spell entry to inspect.
 * @return true if multiple ranks may stack in the spell book; otherwise, false.
 */
bool SpellMgr::canStackSpellRanksInSpellBook(SpellEntry const* spellInfo) const
{
    if (IsPassiveSpell(spellInfo))                          // ranked passive spell
    {
        return false;
    }
    if (spellInfo->powerType != POWER_MANA && spellInfo->powerType != POWER_HEALTH)
    {
        return false;
    }
    if (IsProfessionOrRidingSpell(spellInfo->Id))
    {
        return false;
    }

    if (IsSkillBonusSpell(spellInfo->Id))
    {
        return false;
    }

    // All stance spells. if any better way, change it.
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(SpellEffectIndex(i));
        if (!spellEffect)
        {
            continue;
        }
        switch(spellInfo->GetSpellFamilyName())
        {
            case SPELLFAMILY_PALADIN:
                {
                    // Paladin aura Spell
                    if (spellEffect->Effect == SPELL_EFFECT_APPLY_AREA_AURA_RAID)
                    {
                        return false;
                    }
                    // Seal of Righteousness, 2 version of same rank
                    SpellClassOptionsEntry const* classOptions = spellInfo->GetSpellClassOptions();
                    if (classOptions && (classOptions->SpellFamilyFlags & UI64LIT(0x0000000008000000)) && spellInfo->SpellIconID == 25)
                    {
                        return false;
                    }
                }
                break;
            case SPELLFAMILY_DRUID:
                // Druid form Spell
                if (spellEffect->Effect == SPELL_EFFECT_APPLY_AURA &&
                    spellEffect->EffectApplyAuraName == SPELL_AURA_MOD_SHAPESHIFT)
                {
                    return false;
                }
                break;
            case SPELLFAMILY_ROGUE:
                // Rogue Stealth
                if (spellEffect->Effect == SPELL_EFFECT_APPLY_AURA &&
                    spellEffect->EffectApplyAuraName == SPELL_AURA_MOD_SHAPESHIFT)
                {
                    return false;
                }
                break;
        }
    }
    return true;
}

/**
 * @brief Checks whether two spells should not stack.
 *
 * @param spellId_1 The first spell identifier.
 * @param spellId_2 The second spell identifier.
 * @return true if the spells should not stack; otherwise, false.
 */
bool SpellMgr::IsNoStackSpellDueToSpell(uint32 spellId_1, uint32 spellId_2) const
{
    SpellEntry const* spellInfo_1 = sSpellStore.LookupEntry(spellId_1);
    SpellEntry const* spellInfo_2 = sSpellStore.LookupEntry(spellId_2);

    if (!spellInfo_1 || !spellInfo_2)
    {
        return false;
    }

    SpellClassOptionsEntry const* classOptions1 = spellInfo_1->GetSpellClassOptions();
    SpellClassOptionsEntry const* classOptions2 = spellInfo_2->GetSpellClassOptions();

    // Resurrection sickness
    if ((spellInfo_1->Id == SPELL_ID_PASSIVE_RESURRECTION_SICKNESS) != (spellInfo_2->Id == SPELL_ID_PASSIVE_RESURRECTION_SICKNESS))
    {
        return false;
    }

    // Allow stack passive and not passive spells
    if (spellInfo_1->HasAttribute(SPELL_ATTR_PASSIVE) != spellInfo_2->HasAttribute(SPELL_ATTR_PASSIVE))
    {
        return false;
    }

    // Specific spell family spells
    switch(spellInfo_1->GetSpellFamilyName())
    {
        case SPELLFAMILY_GENERIC:
            switch(spellInfo_2->GetSpellFamilyName())
            {
                case SPELLFAMILY_GENERIC:                   // same family case
                {
                    // Thunderfury
                    if ((spellInfo_1->Id == 21992 && spellInfo_2->Id == 27648) ||
                        (spellInfo_2->Id == 21992 && spellInfo_1->Id == 27648))
                    {
                        return false;
                    }

                    // Lightning Speed (Mongoose) and Fury of the Crashing Waves (Tsunami Talisman)
                    if ((spellInfo_1->Id == 28093 && spellInfo_2->Id == 42084) ||
                        (spellInfo_2->Id == 28093 && spellInfo_1->Id == 42084))
                    {
                        return false;
                    }

                    // Soulstone Resurrection and Twisting Nether (resurrector)
                    if (spellInfo_1->SpellIconID == 92 && spellInfo_2->SpellIconID == 92 && (
                                (spellInfo_1->SpellVisual[0] == 99 && spellInfo_2->SpellVisual[0] == 0) ||
                                (spellInfo_2->SpellVisual[0] == 99 && spellInfo_1->SpellVisual[0] == 0)))
                                {
                                    return false;
                                }

                    // Heart of the Wild, Agility and various Idol Triggers
                    if (spellInfo_1->SpellIconID == 240 && spellInfo_2->SpellIconID == 240)
                    {
                        return false;
                    }

                    // Personalized Weather (thunder effect should overwrite rainy aura)
                    if (spellInfo_1->SpellIconID == 2606 && spellInfo_2->SpellIconID == 2606)
                    {
                        return false;
                    }

                    // Mirrored Soul (FoS - Devourer) - and other Boss spells
                    if (spellInfo_1->SpellIconID == 3176 && spellInfo_2->SpellIconID == 3176)
                    {
                        return false;
                    }

                    // Brood Affliction: Bronze
                    if ((spellInfo_1->Id == 23170 && spellInfo_2->Id == 23171) ||
                        (spellInfo_2->Id == 23170 && spellInfo_1->Id == 23171))
                    {
                        return false;
                    }

                    // Male Shadowy Disguise
                    if ((spellInfo_1->Id == 32756 && spellInfo_2->Id == 38080) ||
                            (spellInfo_2->Id == 32756 && spellInfo_1->Id == 38080))
                    {
                        return false;
                    }

                    // Female Shadowy Disguise
                    if ((spellInfo_1->Id == 32756 && spellInfo_2->Id == 38081) ||
                            (spellInfo_2->Id == 32756 && spellInfo_1->Id == 38081))
                    {
                        return false;
                    }

                    // Cool Down (See PeriodicAuraTick())
                    if ((spellInfo_1->Id == 52441 && spellInfo_2->Id == 52443) ||
                            (spellInfo_2->Id == 52441 && spellInfo_1->Id == 52443))
                    {
                        return false;
                    }

                    // See Chapel Invisibility and See Noth Invisibility
                    if ((spellInfo_1->Id == 52950 && spellInfo_2->Id == 52707) ||
                            (spellInfo_2->Id == 52950 && spellInfo_1->Id == 52707))
                    {
                        return false;
                    }

                    // Regular and Night Elf Ghost
                    if ((spellInfo_1->Id == 8326 && spellInfo_2->Id == 20584) ||
                        (spellInfo_2->Id == 8326 && spellInfo_1->Id == 20584))
                    {
                        return false;
                    }

                    // Aura of Despair auras
                    if ((spellInfo_1->Id == 64848 && spellInfo_2->Id == 62692) ||
                            (spellInfo_2->Id == 64848 && spellInfo_1->Id == 62692))
                    {
                        return false;
                    }

                    // Blood Fury and Rage of the Unraveller
                    if (spellInfo_1->SpellIconID == 1662 && spellInfo_2->SpellIconID == 1662)
                    {
                        return false;
                    }

                    // Kindred Spirits
                    if (spellInfo_1->SpellIconID == 3559 && spellInfo_2->SpellIconID == 3559)
                    {
                        return false;
                    }

                    // Vigilance and Damage Reduction (Vigilance triggered spell)
                    if (spellInfo_1->SpellIconID == 2834 && spellInfo_2->SpellIconID == 2834)
                    {
                        return false;
                    }

                    // Unstable Sphere Timer and Unstable Sphere Passive
                    if ((spellInfo_1->Id == 50758 && spellInfo_2->Id == 50756) ||
                            (spellInfo_2->Id == 50758 && spellInfo_1->Id == 50756))
                    {
                        return false;
                    }

                    // Arcane Beam Periodic and Arcane Beam Visual
                    if ((spellInfo_1->Id == 51019 && spellInfo_2->Id == 51024) ||
                            (spellInfo_2->Id == 51019 && spellInfo_1->Id == 51024))
                    {
                        return false;
                    }

                    // Crystal Spike Pre-visual and Crystal Spike aura
                    if ((spellInfo_1->Id == 50442 && spellInfo_2->Id == 47941) ||
                            (spellInfo_2->Id == 50442 && spellInfo_1->Id == 47941))
                    {
                        return false;
                    }

                    // Impale aura and Submerge
                    if ((spellInfo_1->Id == 53456 && spellInfo_2->Id == 53421) ||
                            (spellInfo_2->Id == 53456 && spellInfo_1->Id == 53421))
                    {
                        return false;
                    }

                    // Summon Anub'ar Champion Periodic and Summon Anub'ar Necromancer Periodic
                    if ((spellInfo_1->Id == 53035 && spellInfo_2->Id == 53036) ||
                            (spellInfo_2->Id == 53035 && spellInfo_1->Id == 53036))
                    {
                        return false;
                    }

                    // Summon Anub'ar Necromancer Periodic and Summon Anub'ar Crypt Fiend Periodic
                    if ((spellInfo_1->Id == 53036 && spellInfo_2->Id == 53037) ||
                            (spellInfo_2->Id == 53036 && spellInfo_1->Id == 53037))
                    {
                        return false;
                    }

                    // Summon Anub'ar Crypt Fiend Periodic and Summon Anub'ar Champion Periodic
                    if ((spellInfo_1->Id == 53037 && spellInfo_2->Id == 53035) ||
                            (spellInfo_2->Id == 53037 && spellInfo_1->Id == 53035))
                    {
                        return false;
                    }

                    // Possess visual and Possess
                    if ((spellInfo_1->Id == 23014 && spellInfo_2->Id == 19832) ||
                        (spellInfo_2->Id == 23014 && spellInfo_1->Id == 19832))
                    {
                        return false;
                    }

                    // Shade Soul Channel and Akama Soul Channel
                    if ((spellInfo_1->Id == 40401 && spellInfo_2->Id == 40447) ||
                            (spellInfo_2->Id == 40401 && spellInfo_1->Id == 40447))
                    {
                        return false;
                    }

                    // Eye Blast visual and Eye Blast
                    if ((spellInfo_1->Id == 39908 && spellInfo_2->Id == 40017) ||
                            (spellInfo_2->Id == 39908 && spellInfo_1->Id == 40017))
                    {
                        return false;
                    }

                    // Encapsulate and Encapsulate (channeled)
                    if ((spellInfo_1->Id == 45665 && spellInfo_2->Id == 45661) ||
                            (spellInfo_2->Id == 45665 && spellInfo_1->Id == 45661))
                    {
                        return false;
                    }

                    // Flame Tsunami Visual and Flame Tsunami Damage Aura
                    if ((spellInfo_1->Id == 57494 && spellInfo_2->Id == 57492) ||
                            (spellInfo_2->Id == 57494 && spellInfo_1->Id == 57492))
                    {
                        return false;
                    }

                    // Cyclone Aura 2 and Cyclone Aura
                    if ((spellInfo_1->Id == 57598 && spellInfo_2->Id == 57560) ||
                            (spellInfo_2->Id == 57598 && spellInfo_1->Id == 57560))
                    {
                        return false;
                    }

                    // Shard of Flame and Mote of Flame
                    if ((spellInfo_1->SpellIconID == 2302 && spellInfo_1->SpellVisual[0] == 0) ||
                            (spellInfo_2->SpellIconID == 2302 && spellInfo_2->SpellVisual[0] == 0))
                    {
                        return false;
                    }

                    // Felblaze Visual and Fog of Corruption
                    if ((spellInfo_1->Id == 45068 && spellInfo_2->Id == 45582) ||
                            (spellInfo_2->Id == 45068 && spellInfo_1->Id == 45582))
                    {
                        return false;
                    }

                    // Simon Game START timer, (DND) and Simon Game Pre-game timer
                    if ((spellInfo_1->Id == 39993 && spellInfo_2->Id == 40041) ||
                            (spellInfo_2->Id == 39993 && spellInfo_1->Id == 40041))
                    {
                        return false;
                    }

                    // Karazhan - Chess: Is Square OCCUPIED aura Karazhan - Chess: Create Move Marker
                    if ((spellInfo_1->Id == 39400 && spellInfo_2->Id == 32261) ||
                            (spellInfo_2->Id == 39400 && spellInfo_1->Id == 32261))
                    {
                        return false;
                    }

                    // Black Hole (damage) and Black Hole (phase)
                    if ((spellInfo_1->Id == 62169 && spellInfo_2->Id == 62168) ||
                            (spellInfo_2->Id == 62169 && spellInfo_1->Id == 62168))
                    {
                        return false;
                    }

                    // Black Hole (damage) and Worm Hole (phase)
                    if ((spellInfo_1->Id == 62169 && spellInfo_2->Id == 65250) ||
                            (spellInfo_2->Id == 62169 && spellInfo_1->Id == 65250))
                    {
                        return false;
                    }

                    // Black Hole (damage) and Phase Punch (phase)
                    if ((spellInfo_1->Id == 62169 && spellInfo_2->Id == 64417) ||
                            (spellInfo_2->Id == 62169 && spellInfo_1->Id == 64417))
                    {
                        return false;
                    }

                    // Auto Grow and Healthy Spore Visual
                    if ((spellInfo_1->Id == 62559 && spellInfo_2->Id == 62538) ||
                            (spellInfo_2->Id == 62559 && spellInfo_1->Id == 62538))
                    {
                        return false;
                    }

                    // Phase 2 Transform and Shadowy Barrier
                    if ((spellInfo_1->Id == 65157 && spellInfo_2->Id == 64775) ||
                        (spellInfo_2->Id == 65157 && spellInfo_1->Id == 64775))
                    {
                        return false;
                    }

                    // Empowered (dummy) and Empowered
                    if ((spellInfo_1->Id == 64161 && spellInfo_2->Id == 65294) ||
                        (spellInfo_2->Id == 64161 && spellInfo_1->Id == 65294))
                    {
                        return false;
                    }

                    // Spectral Realm (reaction) and Spectral Realm (invisibility)
                    if ((spellInfo_1->Id == 44852 && spellInfo_2->Id == 46021) ||
                        (spellInfo_2->Id == 44852 && spellInfo_1->Id == 46021))
                    {
                        return false;
                    }

                    // Halls of Reflection Clone
                    if (spellInfo_1->SpellIconID == 692 && spellInfo_2->SpellIconID == 692)
                    {
                        return false;
                    }
                    break;
                }
                case SPELLFAMILY_MAGE:
                    // Arcane Intellect and Insight
                    if (spellInfo_2->SpellIconID == 125 && spellInfo_1->Id == 18820)
                    {
                        return false;
                    }
                    break;
                case SPELLFAMILY_WARRIOR:
                {
                    // Scroll of Protection and Defensive Stance (multi-family check)
                    if (spellInfo_1->SpellIconID == 276 && spellInfo_1->SpellVisual[0] == 196 && spellInfo_2->Id == 71)
                    {
                        return false;
                    }

                    // Improved Hamstring -> Hamstring (multi-family check)
                    if (classOptions2 && (classOptions2->SpellFamilyFlags & UI64LIT(0x2)) && spellInfo_1->Id == 23694 )
                    {
                        return false;
                    }

                    break;
                }
                case SPELLFAMILY_DRUID:
                {
                    // Scroll of Stamina and Leader of the Pack (multi-family check)
                    if (spellInfo_1->SpellIconID == 312 && spellInfo_1->SpellVisual[0] == 216 && spellInfo_2->Id == 24932)
                    {
                        return false;
                    }

                    // Dragonmaw Illusion (multi-family check)
                    if (spellId_1 == 40216 && spellId_2 == 42016)
                    {
                        return false;
                    }

                    break;
                }
                case SPELLFAMILY_ROGUE:
                {
                    // Garrote-Silence -> Garrote (multi-family check)
                    if (spellInfo_1->SpellIconID == 498 && spellInfo_1->SpellVisual[0] == 0 && spellInfo_2->SpellIconID == 498)
                    {
                        return false;
                    }

                    break;
                }
                case SPELLFAMILY_HUNTER:
                {
                    // Concussive Shot and Imp. Concussive Shot (multi-family check)
                    if (spellInfo_1->Id == 19410 && spellInfo_2->Id == 5116)
                    {
                        return false;
                    }

                    // Improved Wing Clip -> Wing Clip (multi-family check)
                    if (classOptions2 && (classOptions2->SpellFamilyFlags & UI64LIT(0x40)) && spellInfo_1->Id == 19229 )
                    {
                        return false;
                    }
                    break;
                }
                case SPELLFAMILY_PALADIN:
                {
                    // Unstable Currents and other -> *Sanctity Aura (multi-family check)
                    if (spellInfo_2->SpellIconID == 502 && spellInfo_1->SpellIconID == 502 && spellInfo_1->SpellVisual[0] == 969)
                    {
                        return false;
                    }

                    // *Band of Eternal Champion and Seal of Command(multi-family check)
                    if (spellId_1 == 35081 && spellInfo_2->SpellIconID == 561 && spellInfo_2->SpellVisual[0] == 7992)
                    {
                        return false;
                    }

                    // Blessing of Sanctuary (multi-family check, some from 16 spell icon spells)
                    if (spellInfo_1->Id == 67480 && spellInfo_2->Id == 20911)
                    {
                        return false;
                    }

                    break;
                }
            }
            // Dragonmaw Illusion, Blood Elf Illusion, Human Illusion, Illidari Agent Illusion, Scarlet Crusade Disguise
            if (spellInfo_1->SpellIconID == 1691 && spellInfo_2->SpellIconID == 1691)
            {
                return false;
            }
            break;
        case SPELLFAMILY_MAGE:
            if (classOptions2 && classOptions2->SpellFamilyName == SPELLFAMILY_MAGE )
            {
                // Blizzard & Chilled (and some other stacked with blizzard spells
                if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x80)) && (classOptions2->SpellFamilyFlags & UI64LIT(0x100000)) ||
                    (classOptions2->SpellFamilyFlags & UI64LIT(0x80)) && (classOptions1->SpellFamilyFlags & UI64LIT(0x100000)) )
                {
                    return false;
                }

                // Blink & Improved Blink
                if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x0000000000010000)) && (spellInfo_2->SpellVisual[0] == 72 && spellInfo_2->SpellIconID == 1499) ||
                    (classOptions2->SpellFamilyFlags & UI64LIT(0x0000000000010000)) && (spellInfo_1->SpellVisual[0] == 72 && spellInfo_1->SpellIconID == 1499) )
                {
                    return false;
                }

                // Fingers of Frost effects
                if (spellInfo_1->SpellIconID == 2947 && spellInfo_2->SpellIconID == 2947)
                {
                    return false;
                }

                // Living Bomb & Ignite (Dots)
                if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x2000000000000)) && (classOptions2->SpellFamilyFlags & UI64LIT(0x8000000)) ||
                    (classOptions2->SpellFamilyFlags & UI64LIT(0x2000000000000)) && (classOptions1->SpellFamilyFlags & UI64LIT(0x8000000)) )
                    return false;

                // Fireball & Pyroblast (Dots)
                if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x1)) && (classOptions2->SpellFamilyFlags & UI64LIT(0x400000)) ||
                    (classOptions2->SpellFamilyFlags & UI64LIT(0x1)) && (classOptions1->SpellFamilyFlags & UI64LIT(0x400000)) )
                {
                    return false;
                }
            }
            // Detect Invisibility and Mana Shield (multi-family check)
            if (spellInfo_2->Id == 132 && spellInfo_1->SpellIconID == 209 && spellInfo_1->SpellVisual[0] == 968)
            {
                return false;
            }

            // Combustion and Fire Protection Aura (multi-family check)
            if (spellInfo_1->Id == 11129 && spellInfo_2->SpellIconID == 33 && spellInfo_2->SpellVisual[0] == 321)
            {
                return false;
            }

            // Arcane Intellect and Insight
            if (spellInfo_1->SpellIconID == 125 && spellInfo_2->Id == 18820)
            {
                return false;
            }

            break;
        case SPELLFAMILY_WARLOCK:
            if ( classOptions2 && classOptions2->SpellFamilyName == SPELLFAMILY_WARLOCK )
            {
                // Siphon Life and Drain Life
                if ((spellInfo_1->SpellIconID == 152 && spellInfo_2->SpellIconID == 546) ||
                    (spellInfo_2->SpellIconID == 152 && spellInfo_1->SpellIconID == 546))
                {
                    return false;
                }

                // Corruption & Seed of corruption
                if ((spellInfo_1->SpellIconID == 313 && spellInfo_2->SpellIconID == 1932) ||
                    (spellInfo_2->SpellIconID == 313 && spellInfo_1->SpellIconID == 1932))
                    if (spellInfo_1->SpellVisual[0] != 0 && spellInfo_2->SpellVisual[0] != 0)
                    {
                        return true;                         // can't be stacked
                    }

                // Corruption and Unstable Affliction
                if ((spellInfo_1->SpellIconID == 313 && spellInfo_2->SpellIconID == 2039) ||
                        (spellInfo_2->SpellIconID == 313 && spellInfo_1->SpellIconID == 2039))
                {
                    return false;
                }

                // (Corruption or Unstable Affliction) and (Curse of Agony or Curse of Doom)
                if (((spellInfo_1->SpellIconID == 313 || spellInfo_1->SpellIconID == 2039) && (spellInfo_2->SpellIconID == 544  || spellInfo_2->SpellIconID == 91)) ||
                        ((spellInfo_2->SpellIconID == 313 || spellInfo_2->SpellIconID == 2039) && (spellInfo_1->SpellIconID == 544  || spellInfo_1->SpellIconID == 91)))
                {
                    return false;
                }

                // Shadowflame and Curse of Agony
                if ((spellInfo_1->SpellIconID == 544 && spellInfo_2->SpellIconID == 3317) ||
                        (spellInfo_2->SpellIconID == 544 && spellInfo_1->SpellIconID == 3317))
                {
                    return false;
                }

                // Shadowflame and Curse of Doom
                if ((spellInfo_1->SpellIconID == 91 && spellInfo_2->SpellIconID == 3317) ||
                        (spellInfo_2->SpellIconID == 91 && spellInfo_1->SpellIconID == 3317))
                {
                    return false;
                }

                // Metamorphosis, diff effects
                if (spellInfo_1->SpellIconID == 3314 && spellInfo_2->SpellIconID == 3314)
                {
                    return false;
                }
            }
            // Detect Invisibility and Mana Shield (multi-family check)
            if (spellInfo_1->Id == 132 && spellInfo_2->SpellIconID == 209 && spellInfo_2->SpellVisual[0] == 968)
            {
                return false;
            }
            break;
        case SPELLFAMILY_WARRIOR:
            if (classOptions2 && classOptions1->SpellFamilyName == SPELLFAMILY_WARRIOR )
            {
                // Rend and Deep Wound
                if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x20)) && (classOptions2->SpellFamilyFlags & UI64LIT(0x1000000000)) ||
                    (classOptions2->SpellFamilyFlags & UI64LIT(0x20)) && (classOptions1->SpellFamilyFlags & UI64LIT(0x1000000000)) )
                {
                    return false;
                }

                // Battle Shout and Rampage
                if ((spellInfo_1->SpellIconID == 456 && spellInfo_2->SpellIconID == 2006) ||
                    (spellInfo_2->SpellIconID == 456 && spellInfo_1->SpellIconID == 2006))
                {
                    return false;
                }

                // Glyph of Revenge (triggered), and Sword and Board (triggered)
                if ((spellInfo_1->SpellIconID == 856 && spellInfo_2->SpellIconID == 2780) ||
                        (spellInfo_2->SpellIconID == 856 && spellInfo_1->SpellIconID == 2780))
                {
                    return false;
                }

                // Defensive/Berserker/Battle stance aura can not stack (needed for dummy auras)
                if (((classOptions1->SpellFamilyFlags & UI64LIT(0x800000)) && (classOptions2->SpellFamilyFlags & UI64LIT(0x800000))) ||
                    ((classOptions2->SpellFamilyFlags & UI64LIT(0x800000)) && (classOptions1->SpellFamilyFlags & UI64LIT(0x800000))))
                {
                    return true;
                }
            }

            // Hamstring -> Improved Hamstring (multi-family check)
            if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x2)) && spellInfo_2->Id == 23694 )
            {
                return false;
            }

            // Defensive Stance and Scroll of Protection (multi-family check)
            if (spellInfo_1->Id == 71 && spellInfo_2->SpellIconID == 276 && spellInfo_2->SpellVisual[0] == 196)
            {
                return false;
            }

            // Bloodlust and Bloodthirst (multi-family check)
            if (spellInfo_2->Id == 2825 && spellInfo_1->SpellIconID == 38 && spellInfo_1->SpellVisual[0] == 0)
            {
                return false;
            }

            break;
        case SPELLFAMILY_PRIEST:
            if (classOptions2 && classOptions2->SpellFamilyName == SPELLFAMILY_PRIEST )
            {
                // Devouring Plague and Shadow Vulnerability
                if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x2000000)) && (classOptions2->SpellFamilyFlags & UI64LIT(0x800000000)) ||
                    (classOptions2->SpellFamilyFlags & UI64LIT(0x2000000)) && (classOptions1->SpellFamilyFlags & UI64LIT(0x800000000)))
                {
                    return false;
                }

                // StarShards and Shadow Word: Pain
                if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x200000)) && (classOptions2->SpellFamilyFlags & UI64LIT(0x8000)) ||
                    (classOptions2->SpellFamilyFlags & UI64LIT(0x200000)) && (classOptions1->SpellFamilyFlags & UI64LIT(0x8000)))
                {
                    return false;
                }

                // Dispersion
                if ((spellInfo_1->Id == 47585 && spellInfo_2->Id == 60069) ||
                        (spellInfo_2->Id == 47585 && spellInfo_1->Id == 60069))
                {
                    return false;
                }
            }
            break;
        case SPELLFAMILY_DRUID:
            if (classOptions2 && classOptions2->SpellFamilyName == SPELLFAMILY_DRUID )
            {
                // Omen of Clarity and Blood Frenzy
                if ((classOptions1 && (classOptions1->SpellFamilyFlags == UI64LIT(0x0) && spellInfo_1->SpellIconID == 108) && (classOptions2->SpellFamilyFlags & UI64LIT(0x20000000000000))) ||
                    ((classOptions2->SpellFamilyFlags == UI64LIT(0x0) && spellInfo_2->SpellIconID == 108) && (classOptions1->SpellFamilyFlags & UI64LIT(0x20000000000000))))
                {
                    return false;
                }

                //  Tree of Life (Shapeshift) and 34123 Tree of Life (Passive)
                if ((spellId_1 == 33891 && spellId_2 == 34123) ||
                        (spellId_2 == 33891 && spellId_1 == 34123))
                {
                    return false;
                }

                // Lifebloom and Wild Growth
                if ((spellInfo_1->SpellIconID == 2101 && spellInfo_2->SpellIconID == 2864) ||
                        (spellInfo_2->SpellIconID == 2101 && spellInfo_1->SpellIconID == 2864))
                {
                    return false;
                }

                //  Innervate and Glyph of Innervate and some other spells
                if (spellInfo_1->SpellIconID == 62 && spellInfo_2->SpellIconID == 62)
                {
                    return false;
                }

                // Wrath of Elune and Nature's Grace
                if ((spellInfo_1->Id == 16886 && spellInfo_2->Id == 46833) ||
                    (spellInfo_2->Id == 16886 && spellInfo_1->Id == 46833))
                {
                    return false;
                }

                // Bear Rage (Feral T4 (2)) and Omen of Clarity
                if ((spellInfo_1->Id == 16864 && spellInfo_2->Id == 37306) ||
                    (spellInfo_2->Id == 16864 && spellInfo_1->Id == 37306))
                {
                    return false;
                }

                // Cat Energy (Feral T4 (2)) and Omen of Clarity
                if ((spellInfo_1->Id == 16864 && spellInfo_2->Id == 37311) ||
                    (spellInfo_2->Id == 16864 && spellInfo_1->Id == 37311))
                {
                    return false;
                }

                // Survival Instincts and Survival Instincts
                if ((spellInfo_1->Id == 61336 && spellInfo_2->Id == 50322) ||
                        (spellInfo_2->Id == 61336 && spellInfo_1->Id == 50322))
                {
                    return false;
                }

                // Savage Roar and Savage Roar (triggered)
                if (spellInfo_1->SpellIconID == 2865 && spellInfo_2->SpellIconID == 2865)
                {
                    return false;
                }

                // Frenzied Regeneration and Savage Defense
                if ((spellInfo_1->Id == 22842 && spellInfo_2->Id == 62606) ||
                        (spellInfo_2->Id == 22842 && spellInfo_1->Id == 62606))
                {
                    return false;
                }
            }

            // Leader of the Pack and Scroll of Stamina (multi-family check)
            if (spellInfo_1->Id == 24932 && spellInfo_2->SpellIconID == 312 && spellInfo_2->SpellVisual[0] == 216)
            {
                return false;
            }

            // Dragonmaw Illusion (multi-family check)
            if (spellId_1 == 42016 && spellId_2 == 40216)
            {
                return false;
            }

            break;
        case SPELLFAMILY_ROGUE:
            if (classOptions2 && classOptions2->SpellFamilyName == SPELLFAMILY_ROGUE )
            {
                // Master of Subtlety
                if ((spellId_1 == 31665 && spellId_2 == 31666) ||
                        (spellId_1 == 31666 && spellId_2 == 31665))
                {
                    return false;
                }

                // Sprint & Sprint (waterwalk)
                if (spellInfo_1->SpellIconID == 516 && spellInfo_2->SpellIconID == 516 &&
                    ((spellInfo_1->GetCategory() == 44 && spellInfo_2->GetCategory() == 0) ||
                    (spellInfo_2->GetCategory() == 44 && spellInfo_1->GetCategory() == 0)))
                {
                    return false;
                }
            }

            // Overkill
            if (spellInfo_1->SpellIconID == 2285 && spellInfo_2->SpellIconID == 2285)
            {
                return false;
            }

            // Garrote -> Garrote-Silence (multi-family check)
            if (spellInfo_1->SpellIconID == 498 && spellInfo_2->SpellIconID == 498 && spellInfo_2->SpellVisual[0] == 0)
            {
                return false;
            }
            break;
        case SPELLFAMILY_HUNTER:
            if (classOptions2 && classOptions2->SpellFamilyName == SPELLFAMILY_HUNTER )
            {
                // Rapid Fire & Quick Shots
                if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x20)) && (classOptions2->SpellFamilyFlags & UI64LIT(0x20000000000)) ||
                    (classOptions2->SpellFamilyFlags & UI64LIT(0x20)) && (classOptions1->SpellFamilyFlags & UI64LIT(0x20000000000)) )
                {
                    return false;
                }

                // Serpent Sting & (Immolation/Explosive Trap Effect)
                if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x4)) && (classOptions2->SpellFamilyFlags & UI64LIT(0x00000004000)) ||
                    (classOptions2->SpellFamilyFlags & UI64LIT(0x4)) && (classOptions1->SpellFamilyFlags & UI64LIT(0x00000004000)) )
                {
                    return false;
                }

                // Deterrence
                if (spellInfo_1->SpellIconID == 83 && spellInfo_2->SpellIconID == 83)
                {
                    return false;
                }

                // Bestial Wrath
                if (spellInfo_1->SpellIconID == 1680 && spellInfo_2->SpellIconID == 1680)
                {
                    return false;
                }

                // Aspect of the Viper & Vicious Viper
                if (spellInfo_1->SpellIconID == 2227 && spellInfo_2->SpellIconID == 2227)
                {
                    return false;
                }
            }

            // Wing Clip -> Improved Wing Clip (multi-family check)
            if (classOptions1 && (classOptions1->SpellFamilyFlags & UI64LIT(0x40)) && spellInfo_2->Id == 19229 )
            {
                return false;
            }

            // Concussive Shot and Imp. Concussive Shot (multi-family check)
            if (spellInfo_2->Id == 19410 && spellInfo_1->Id == 5116)
            {
                return false;
            }
            break;
        case SPELLFAMILY_PALADIN:
            if (classOptions2 && classOptions2->SpellFamilyName == SPELLFAMILY_PALADIN )
            {
                // Paladin Seals
                if (IsSealSpell(spellInfo_1) && IsSealSpell(spellInfo_2))
                {
                    return true;
                }

                // Swift Retribution / Improved Devotion Aura (talents) and Paladin Auras
                if ((spellInfo_1->IsFitToFamilyMask(UI64LIT(0x0), 0x00000020) && (spellInfo_2->SpellIconID == 291 || spellInfo_2->SpellIconID == 3028)) ||
                        (spellInfo_2->IsFitToFamilyMask(UI64LIT(0x0), 0x00000020) && (spellInfo_1->SpellIconID == 291 || spellInfo_1->SpellIconID == 3028)))
                {
                    return false;
                }

                // Beacon of Light and Light's Beacon
                if ((spellInfo_1->SpellIconID == 3032) && (spellInfo_2->SpellIconID == 3032))
                {
                    return false;
                }

                // Concentration Aura and Improved Concentration Aura and Aura Mastery
                if ((spellInfo_1->SpellIconID == 1487) && (spellInfo_2->SpellIconID == 1487))
                {
                    return false;
                }

                // Seal of Corruption (caster/target parts stacking allow, other stacking checked by spell specs)
                if (spellInfo_1->SpellIconID == 2292 && spellInfo_2->SpellIconID == 2292)
                {
                    return false;
                }

                // Divine Sacrifice and Divine Guardian
                if (spellInfo_1->SpellIconID == 3837 && spellInfo_2->SpellIconID == 3837)
                {
                    return false;
                }

                // Blood Corruption, Holy Vengeance, Righteous Vengeance
                if ((spellInfo_1->SpellIconID == 2292 && spellInfo_2->SpellIconID == 3025) ||
                        (spellInfo_2->SpellIconID == 2292 && spellInfo_1->SpellIconID == 3025))
                {
                    return false;
                }
            }

            // Blessing of Sanctuary (multi-family check, some from 16 spell icon spells)
            if (spellInfo_2->Id == 67480 && spellInfo_1->Id == 20911)
            {
                return false;
            }

            // Combustion and Fire Protection Aura (multi-family check)
            if (spellInfo_2->Id == 11129 && spellInfo_1->SpellIconID == 33 && spellInfo_1->SpellVisual[0] == 321)
            {
                return false;
            }

            // *Sanctity Aura -> Unstable Currents and other (multi-family check)
            if (spellInfo_1->SpellIconID==502 && classOptions2->SpellFamilyName == SPELLFAMILY_GENERIC && spellInfo_2->SpellIconID==502 && spellInfo_2->SpellVisual[0]==969 )
            {
                return false;
            }

            // *Seal of Command and Band of Eternal Champion (multi-family check)
            if (spellInfo_1->SpellIconID == 561 && spellInfo_1->SpellVisual[0] == 7992 && spellId_2 == 35081)
            {
                return false;
            }
            break;
        case SPELLFAMILY_SHAMAN:
            if (classOptions2 && classOptions2->SpellFamilyName == SPELLFAMILY_SHAMAN )
            {
                // Windfury weapon
                if (spellInfo_1->SpellIconID == 220 && spellInfo_2->SpellIconID == 220 &&
                    !classOptions1->IsFitToFamilyMask(classOptions2->SpellFamilyFlags))
                    {
                        return false;
                    }

                // Ghost Wolf
                if (spellInfo_1->SpellIconID == 67 && spellInfo_2->SpellIconID == 67)
                {
                    return false;
                }

                // Totem of Wrath (positive/negative), ranks checked early
                if (spellInfo_1->SpellIconID == 2019 && spellInfo_2->SpellIconID == 2019)
                {
                    return false;
                }
            }
            // Bloodlust and Bloodthirst (multi-family check)
            if (spellInfo_1->Id == 2825 && spellInfo_2->SpellIconID == 38 && spellInfo_2->SpellVisual[0] == 0)
            {
                return false;
            }
            break;
        case SPELLFAMILY_DEATHKNIGHT:
            if (classOptions2 && classOptions2->SpellFamilyName == SPELLFAMILY_DEATHKNIGHT)
            {
                // Lichborne  and Lichborne (triggered)
                if (spellInfo_1->SpellIconID == 61 && spellInfo_2->SpellIconID == 61)
                {
                    return false;
                }

                // Frost Presence and Frost Presence (triggered)
                if (spellInfo_1->SpellIconID == 2632 && spellInfo_2->SpellIconID == 2632)
                {
                    return false;
                }

                // Unholy Presence and Unholy Presence (triggered)
                if (spellInfo_1->SpellIconID == 2633 && spellInfo_2->SpellIconID == 2633)
                {
                    return false;
                }

                // Blood Presence and Blood Presence (triggered)
                if (spellInfo_1->SpellIconID == 2636 && spellInfo_2->SpellIconID == 2636)
                {
                    return false;
                }
            }
            break;
        default:
            break;
    }

    // more generic checks
    if (spellInfo_1->SpellIconID == spellInfo_2->SpellIconID &&
        spellInfo_1->SpellIconID != 0 && spellInfo_2->SpellIconID != 0)
    {
        bool isModifier = false;
        for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        {
            SpellEffectEntry const* spellEffect1 = spellInfo_1->GetSpellEffect(SpellEffectIndex(i));
            SpellEffectEntry const* spellEffect2 = spellInfo_2->GetSpellEffect(SpellEffectIndex(i));
            if (!spellEffect1 || !spellEffect2)
            {
                continue;
            }
            if (spellEffect1->EffectApplyAuraName == SPELL_AURA_ADD_FLAT_MODIFIER ||
                spellEffect1->EffectApplyAuraName == SPELL_AURA_ADD_PCT_MODIFIER  ||
                spellEffect2->EffectApplyAuraName == SPELL_AURA_ADD_FLAT_MODIFIER ||
                spellEffect2->EffectApplyAuraName == SPELL_AURA_ADD_PCT_MODIFIER )
            {
                isModifier = true;
            }
        }

        if (!isModifier)
        {
            return true;
        }
    }

    if (IsRankSpellDueToSpell(spellInfo_1, spellId_2))
    {
        return true;
    }

    if (!classOptions1 || classOptions1->SpellFamilyName == 0 || !classOptions2 || classOptions2->SpellFamilyName == 0)
    {
        return false;
    }

    if (classOptions1->SpellFamilyName != classOptions2->SpellFamilyName)
    {
        return false;
    }

    bool dummy_only = true;
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        SpellEffectEntry const* spellEffect1 = spellInfo_1->GetSpellEffect(SpellEffectIndex(i));
        SpellEffectEntry const* spellEffect2 = spellInfo_2->GetSpellEffect(SpellEffectIndex(i));

        if (!spellEffect1 && !spellEffect2)
        {
            continue;
        }

        if (!spellEffect1 || !spellEffect2)
        {
            return false;
        }

        if (spellEffect1->Effect != spellEffect2->Effect ||
            spellEffect1->EffectItemType != spellEffect2->EffectItemType ||
            spellEffect1->EffectMiscValue != spellEffect2->EffectMiscValue ||
            spellEffect1->EffectApplyAuraName != spellEffect2->EffectApplyAuraName)
            return false;

        // ignore dummy only spells
        if (spellEffect1->Effect && spellEffect1->Effect != SPELL_EFFECT_DUMMY && spellEffect1->EffectApplyAuraName != SPELL_AURA_DUMMY)
        {
            dummy_only = false;
        }
    }
    if (dummy_only)
    {
        return false;
    }

    return true;
}

bool SpellMgr::IsSpellCanAffectSpell(SpellEntry const* spellInfo_1, SpellEntry const* spellInfo_2) const
{
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        ClassFamilyMask mask = spellInfo_1->GetEffectSpellClassMask(SpellEffectIndex(i));
        if (spellInfo_2->IsFitToFamilyMask(mask))
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Checks whether a spell teaches a profession or riding skill.
 *
 * @param spellId The spell identifier.
 * @return true if the spell teaches a profession or riding skill; otherwise, false.
 */
bool SpellMgr::IsProfessionOrRidingSpell(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        return false;
    }

    if (spellInfo->GetSpellEffectIdByIndex(EFFECT_INDEX_1) != SPELL_EFFECT_SKILL)
    {
        return false;
    }

    uint32 skill = spellInfo->GetEffectMiscValue(EFFECT_INDEX_1);

    return IsProfessionOrRidingSkill(skill);
}

/**
 * @brief Checks whether a spell teaches a profession skill.
 *
 * @param spellId The spell identifier.
 * @return true if the spell teaches a profession skill; otherwise, false.
 */
bool SpellMgr::IsProfessionSpell(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        return false;
    }

    if (spellInfo->GetSpellEffectIdByIndex(EFFECT_INDEX_1) != SPELL_EFFECT_SKILL)
    {
        return false;
    }

    uint32 skill = spellInfo->GetEffectMiscValue(EFFECT_INDEX_1);

    return IsProfessionSkill(skill);
}

/**
 * @brief Checks whether a spell teaches a primary profession skill.
 *
 * @param spellId The spell identifier.
 * @return true if the spell teaches a primary profession; otherwise, false.
 */
bool SpellMgr::IsPrimaryProfessionSpell(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        return false;
    }

    if (spellInfo->GetSpellEffectIdByIndex(EFFECT_INDEX_1) != SPELL_EFFECT_SKILL)
    {
        return false;
    }

    uint32 skill = spellInfo->GetEffectMiscValue(EFFECT_INDEX_1);

    return IsPrimaryProfessionSkill(skill);
}

uint32 SpellMgr::GetProfessionSpellMinLevel(uint32 spellId)
{
    uint32 s2l[8][3] =
    {
        // 0 - gather 1 - non-gather 2 - fish
        /*0*/ { 0,   5,  5 },
        /*1*/ { 0,   5,  5 },
        /*2*/ { 0,  10, 10 },
        /*3*/ { 10, 20, 10 },
        /*4*/ { 25, 35, 10 },
        /*5*/ { 40, 50, 10 },
        /*6*/ { 55, 65, 10 },
        /*7*/ { 75, 75, 10 },
    };

    uint32 rank = GetSpellRank(spellId);
    if (rank >= 8)
    {
        return 0;
    }

    SkillLineAbilityMapBounds bounds = GetSkillLineAbilityMapBounds(spellId);
    if (bounds.first == bounds.second)
    {
        return 0;
    }

    switch (bounds.first->second->skillId)
    {
        case SKILL_FISHING:
            return s2l[rank][2];
        case SKILL_HERBALISM:
        case SKILL_MINING:
        case SKILL_SKINNING:
            return s2l[rank][0];
        default:
            return s2l[rank][1];
    }
}

/**
 * @brief Checks whether a spell is the first rank of a primary profession.
 *
 * @param spellId The spell identifier.
 * @return true if the spell is the first rank of a primary profession; otherwise, false.
 */
bool SpellMgr::IsPrimaryProfessionFirstRankSpell(uint32 spellId) const
{
    return IsPrimaryProfessionSpell(spellId) && GetSpellRank(spellId) == 1;
}

/**
 * @brief Checks whether a spell grants a profession skill bonus tier.
 *
 * @param spellId The spell identifier.
 * @return true if the spell is a skill-bonus spell; otherwise, false.
 */
bool SpellMgr::IsSkillBonusSpell(uint32 spellId) const
{
    SkillLineAbilityMapBounds bounds = GetSkillLineAbilityMapBounds(spellId);

    for (SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
    {
        SkillLineAbilityEntry const* pAbility = _spell_idx->second;
        if (!pAbility || pAbility->learnOnGetSkill != ABILITY_LEARNED_ON_GET_PROFESSION_SKILL)
        {
            continue;
        }

        if (pAbility->req_skill_value > 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Selects the most suitable positive aura rank for a target level.
 *
 * @param spellInfo The reference spell entry.
 * @param level The target unit level.
 * @return Pointer to the chosen rank spell entry.
 */
SpellEntry const* SpellMgr::SelectAuraRankForLevel(SpellEntry const* spellInfo, uint32 level) const
{
    // fast case
    if (level + 10 >= spellInfo->GetSpellLevel())
    {
        return spellInfo;
    }

    // ignore selection for passive spells
    if (IsPassiveSpell(spellInfo))
    {
        return spellInfo;
    }

    bool needRankSelection = false;
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(SpellEffectIndex(i));
        if (!spellEffect)
        {
            continue;
        }
        // for simple aura in check apply to any non caster based targets, in rank search mode to any explicit targets
        if (((spellEffect->Effect == SPELL_EFFECT_APPLY_AURA &&
            (IsExplicitPositiveTarget(spellEffect->EffectImplicitTargetA) ||
            IsAreaEffectPossitiveTarget(Targets(spellEffect->EffectImplicitTargetA)))) ||
            spellEffect->Effect == SPELL_EFFECT_APPLY_AREA_AURA_PARTY ||
            spellEffect->Effect == SPELL_EFFECT_APPLY_AREA_AURA_RAID) &&
            IsPositiveEffect(spellInfo, SpellEffectIndex(i)))
        {
            needRankSelection = true;
            break;
        }
    }

    // not required (rank check more slow so check it here)
    if (!needRankSelection || GetSpellRank(spellInfo->Id) == 0)
    {
        return spellInfo;
    }

    for (uint32 nextSpellId = spellInfo->Id; nextSpellId != 0; nextSpellId = GetPrevSpellInChain(nextSpellId))
    {
        SpellEntry const* nextSpellInfo = sSpellStore.LookupEntry(nextSpellId);
        if (!nextSpellInfo)
        {
            break;
        }

        // if found appropriate level
        if (level + 10 >= nextSpellInfo->GetSpellLevel())
        {
            return nextSpellInfo;
        }

        // one rank less then
    }

    // not found
    return NULL;
}

/// Some checks for spells, to prevent adding deprecated/broken spells for trainers, spell book, etc
bool SpellMgr::IsSpellValid(SpellEntry const* spellInfo, Player* pl, bool msg)
{
    // not exist
    if (!spellInfo)
    {
        return false;
    }

    bool need_check_reagents = false;

    // check effects
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(SpellEffectIndex(i));
        if (!spellEffect)
        {
            continue;
        }

        switch(spellEffect->Effect)
        {
            case SPELL_EFFECT_NONE:
                continue;

                // craft spell for crafting nonexistent item (break client recipes list show)
            case SPELL_EFFECT_CREATE_ITEM:
            case SPELL_EFFECT_CREATE_ITEM_2:
            {
                if (spellEffect->EffectItemType == 0)
                {
                    // skip auto-loot crafting spells, its not need explicit item info (but have special fake items sometime)
                    if (!IsLootCraftingSpell(spellInfo))
                    {
                        if (msg)
                        {
                            if (pl)
                            {
                                ChatHandler(pl).PSendSysMessage("Craft spell %u not have create item entry.", spellInfo->Id);
                            }
                            else
                            {
                                sLog.outErrorDb("Craft spell %u not have create item entry.", spellInfo->Id);
                            }
                        }
                        return false;
                    }
                }
                // also possible IsLootCraftingSpell case but fake item must exist anyway
                else if (!ObjectMgr::GetItemPrototype( spellEffect->EffectItemType ))
                {
                    if (msg)
                    {
                        if (pl)
                        {
                            ChatHandler(pl).PSendSysMessage("Craft spell %u create item (Entry: %u) but item does not exist in item_template.",spellInfo->Id,spellEffect->EffectItemType);
                        }
                        else
                        {
                            sLog.outErrorDb("Craft spell %u create item (Entry: %u) but item does not exist in item_template.",spellInfo->Id,spellEffect->EffectItemType);
                        }
                    }
                    return false;
                }

                need_check_reagents = true;
                break;
            }
            case SPELL_EFFECT_LEARN_SPELL:
            {
                SpellEntry const* spellInfo2 = sSpellStore.LookupEntry(spellEffect->EffectTriggerSpell);
                if (!IsSpellValid(spellInfo2, pl, msg))
                {
                    if (msg)
                    {
                        if (pl)
                        {
                            ChatHandler(pl).PSendSysMessage("Spell %u learn to broken spell %u, and then...",spellInfo->Id,spellEffect->EffectTriggerSpell);
                        }
                        else
                        {
                            sLog.outErrorDb("Spell %u learn to invalid spell %u, and then...",spellInfo->Id,spellEffect->EffectTriggerSpell);
                        }
                    }
                    return false;
                }
                break;
            }
        }
    }

    if (need_check_reagents)
    {
        SpellReagentsEntry const* spellReagents = spellInfo->GetSpellReagents();
        if (spellReagents)
        {
            for(int j = 0; j < MAX_SPELL_REAGENTS; ++j)
            {
                if (spellReagents->Reagent[j] > 0 && !ObjectMgr::GetItemPrototype( spellReagents->Reagent[j] ))
                {
                    if (msg)
                    {
                        if (pl)
                        {
                            ChatHandler(pl).PSendSysMessage("Craft spell %u requires reagent item (Entry: %u) but item does not exist in item_template.",spellInfo->Id,spellReagents->Reagent[j]);
                        }
                        else
                        {
                            sLog.outErrorDb("Craft spell %u requires reagent item (Entry: %u) but item does not exist in item_template.",spellInfo->Id,spellReagents->Reagent[j]);
                        }
                    }
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 * @brief Checks whether a spell is allowed in a given location.
 *
 * @param spellInfo The spell entry to validate.
 * @param map_id The current map identifier.
 * @param zone_id The current zone identifier.
 * @param area_id The current area identifier.
 * @param player The player attempting the cast, if any.
 * @return The spell cast failure code, or SPELL_CAST_OK when allowed.
 */
SpellCastResult SpellMgr::GetSpellAllowedInLocationError(SpellEntry const* spellInfo, uint32 map_id, uint32 zone_id, uint32 area_id, Player const* player)
{
    // normal case
    int32 areaGroupId = spellInfo->GetAreaGroupId();
    if (areaGroupId > 0)
    {
        bool found = false;
        AreaGroupEntry const* groupEntry = sAreaGroupStore.LookupEntry(areaGroupId);
        while (groupEntry)
        {
            for (uint32 i = 0; i < 6; ++i)
                if (groupEntry->AreaId[i] == zone_id || groupEntry->AreaId[i] == area_id)
                {
                    found = true;
                }
            if (found || !groupEntry->nextGroup)
            {
                break;
            }
            // Try search in next group
            groupEntry = sAreaGroupStore.LookupEntry(groupEntry->nextGroup);
        }

        if (!found)
        {
            return SPELL_FAILED_INCORRECT_AREA;
        }
    }

    // continent limitation (virtual continent), ignore for GM
    if (spellInfo->HasAttribute(SPELL_ATTR_EX4_CAST_ONLY_IN_OUTLAND) && !(player && player->isGameMaster()))
    {
        uint32 v_map = GetVirtualMapForMapAndZone(map_id, zone_id);
        MapEntry const* mapEntry = sMapStore.LookupEntry(v_map);
        if (!mapEntry || mapEntry->addon < 1 || !mapEntry->IsContinent())
        {
            return SPELL_FAILED_INCORRECT_AREA;
        }
    }

    // raid instance limitation
    if (spellInfo->HasAttribute(SPELL_ATTR_EX6_NOT_IN_RAID_INSTANCE))
    {
        MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
        if (!mapEntry || mapEntry->IsRaid())
        {
            return SPELL_FAILED_NOT_IN_RAID_INSTANCE;
        }
    }

    // DB base check (if non empty then must fit at least single for allow)
    SpellAreaMapBounds saBounds = GetSpellAreaMapBounds(spellInfo->Id);
    if (saBounds.first != saBounds.second)
    {
        for (SpellAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
        {
            if (itr->second.IsFitToRequirements(player, zone_id, area_id))
            {
                return SPELL_CAST_OK;
            }
        }
        return SPELL_FAILED_INCORRECT_AREA;
    }

    // bg spell checks

    // do not allow spells to be cast in arenas
    // - with SPELL_ATTR_EX4_NOT_USABLE_IN_ARENA flag
    // - with greater than 10 min CD
    if (spellInfo->HasAttribute(SPELL_ATTR_EX4_NOT_USABLE_IN_ARENA) ||
            (GetSpellRecoveryTime(spellInfo) > 10 * MINUTE * IN_MILLISECONDS && !spellInfo->HasAttribute(SPELL_ATTR_EX4_USABLE_IN_ARENA)))
        if (player && player->InArena())
        {
            return SPELL_FAILED_NOT_IN_ARENA;
        }

    // Spell casted only on battleground
    if (spellInfo->HasAttribute(SPELL_ATTR_EX3_BATTLEGROUND))
        if (!player || !player->InBattleGround())
        {
            return SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }

    switch (spellInfo->Id)
    {
            // a trinket in alterac valley allows to teleport to the boss
        case 22564:                                         // recall
        case 22563:                                         // recall
        {
            if (!player)
            {
                return SPELL_FAILED_REQUIRES_AREA;
            }
            BattleGround* bg = player->GetBattleGround();
            return map_id == 30 && bg
                   && bg->GetStatus() != STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        }
        case 23333:                                         // Warsong Flag
        case 23335:                                         // Silverwing Flag
            return map_id == 489 && player && player->InBattleGround() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        case 34976:                                         // Netherstorm Flag
            return map_id == 566 && player && player->InBattleGround() ? SPELL_CAST_OK : SPELL_FAILED_REQUIRES_AREA;
        case 2584:                                          // Waiting to Resurrect
        case 42792:                                         // Recently Dropped Flag
        case 43681:                                         // Inactive
        {
            return player && player->InBattleGround() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }
        case 22011:                                         // Spirit Heal Channel
        case 22012:                                         // Spirit Heal
        case 24171:                                         // Resurrection Impact Visual
        case 44535:                                         // Spirit Heal (mana)
        {
            MapEntry const* mapEntry = sMapStore.LookupEntry(map_id);
            if (!mapEntry)
            {
                return SPELL_FAILED_INCORRECT_AREA;
            }
            return mapEntry->IsBattleGround() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }
        case 44521:                                         // Preparation
        {
            if (!player)
            {
                return SPELL_FAILED_REQUIRES_AREA;
            }

            BattleGround* bg = player->GetBattleGround();
            return bg && bg->GetStatus() == STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }
        case 32724:                                         // Gold Team (Alliance)
        case 32725:                                         // Green Team (Alliance)
        case 35774:                                         // Gold Team (Horde)
        case 35775:                                         // Green Team (Horde)
        {
            return player && player->InArena() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_IN_ARENA;
        }
        case 32727:                                         // Arena Preparation
        {
            if (!player)
            {
                return SPELL_FAILED_REQUIRES_AREA;
            }
            if (!player->InArena())
            {
                return SPELL_FAILED_REQUIRES_AREA;
            }

            BattleGround* bg = player->GetBattleGround();
            return bg && bg->GetStatus() == STATUS_WAIT_JOIN ? SPELL_CAST_OK : SPELL_FAILED_ONLY_IN_ARENA;
        }
        case 74410:                                         // Arena - Dampening
            return player && player->InArena() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_IN_ARENA;
        case 74411:                                         // Battleground - Dampening
        {
            if (!player)
            {
                return SPELL_FAILED_ONLY_BATTLEGROUNDS;
            }

            BattleGround* bg = player->GetBattleGround();
            return bg && !bg->isArena() ? SPELL_CAST_OK : SPELL_FAILED_ONLY_BATTLEGROUNDS;
        }
    }

    return SPELL_CAST_OK;
}

/**
 * @brief Validates spell references used by a custom spell-related database table.
 *
 * @param table The table name to inspect.
 */
void SpellMgr::CheckUsedSpells(char const* table)
{
    uint32 countSpells = 0;
    uint32 countMasks = 0;

    //                                                  0         1                 2                  3                  4           5             6               7            8            9           10     11
    QueryResult* result = WorldDatabase.PQuery("SELECT `spellid`,`SpellFamilyName`,`SpellFamilyMaskA`,`SpellFamilyMaskB`,`SpellIcon`,`SpellVisual`,`SpellCategory`,`EffectType`,`EffectAura`,`EffectIdx`,`Name`,`Code` FROM `%s`", table);

    if (!result)
    {
        BarGoLink bar(1);

        bar.step();

        sLog.outString();
        sLog.outErrorDb("`%s` table is empty!", table);
        return;
    }

    BarGoLink bar(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        bar.step();

        uint32 spell       = fields[0].GetUInt32();
        int32  family      = fields[1].GetInt32();
        uint64 familyMaskA = fields[2].GetUInt64();
        uint32 familyMaskB = fields[3].GetUInt32();
        int32  spellIcon   = fields[4].GetInt32();
        int32  spellVisual = fields[5].GetInt32();
        int32  category    = fields[6].GetInt32();
        int32  effectType  = fields[7].GetInt32();
        int32  auraType    = fields[8].GetInt32();
        int32  effectIdx   = fields[9].GetInt32();
        std::string name   = fields[10].GetCppString();
        std::string code   = fields[11].GetCppString();

        // checks of correctness requirements itself

        if (family < -1 || family > SPELLFAMILY_PET)
        {
            sLog.outError("Table '%s' for spell %u have wrong SpellFamily value(%u), skipped.", table, spell, family);
            continue;
        }

        // TODO: spellIcon check need dbc loading
        if (spellIcon < -1)
        {
            sLog.outError("Table '%s' for spell %u have wrong SpellIcon value(%u), skipped.", table, spell, spellIcon);
            continue;
        }

        // TODO: spellVisual check need dbc loading
        if (spellVisual < -1)
        {
            sLog.outError("Table '%s' for spell %u have wrong SpellVisual value(%u), skipped.", table, spell, spellVisual);
            continue;
        }

        // TODO: for spellCategory better check need dbc loading
        if (category < -1 || (category >= 0 && sSpellCategoryStore.find(category) == sSpellCategoryStore.end()))
        {
            sLog.outError("Table '%s' for spell %u have wrong SpellCategory value(%u), skipped.", table, spell, category);
            continue;
        }

        if (effectType < -1 || effectType >= TOTAL_SPELL_EFFECTS)
        {
            sLog.outError("Table '%s' for spell %u have wrong SpellEffect type value(%u), skipped.", table, spell, effectType);
            continue;
        }

        if (auraType < -1 || auraType >= TOTAL_AURAS)
        {
            sLog.outError("Table '%s' for spell %u have wrong SpellAura type value(%u), skipped.", table, spell, auraType);
            continue;
        }

        if (effectIdx < -1 || effectIdx >= 3)
        {
            sLog.outError("Table '%s' for spell %u have wrong EffectIdx value(%u), skipped.", table, spell, effectIdx);
            continue;
        }

        // now checks of requirements

        if (spell)
        {
            ++countSpells;

            SpellEntry const* spellEntry = sSpellStore.LookupEntry(spell);
            if (!spellEntry)
            {
                sLog.outError("Spell %u '%s' not exist but used in %s.", spell, name.c_str(), code.c_str());
                continue;
            }

            SpellClassOptionsEntry const* classOptions = spellEntry->GetSpellClassOptions();

            if (family >= 0 && classOptions && classOptions->SpellFamilyName != uint32(family))
            {
                sLog.outError("Spell %u '%s' family(%u) <> %u but used in %s.",spell,name.c_str(),classOptions->SpellFamilyName,family,code.c_str());
                continue;
            }

            if (familyMaskA != UI64LIT(0xFFFFFFFFFFFFFFFF) || familyMaskB != 0xFFFFFFFF)
            {
                if (familyMaskA == UI64LIT(0x0000000000000000) && familyMaskB == 0x00000000)
                {
                    if (classOptions && classOptions->SpellFamilyFlags)
                    {
                        sLog.outError("Spell %u '%s' not fit to (" I64FMT "," I32FMT ") but used in %s.",
                                      spell, name.c_str(), familyMaskA, familyMaskB, code.c_str());
                        continue;
                    }
                }
                else
                {
                    if (!spellEntry->IsFitToFamilyMask(familyMaskA, familyMaskB))
                    {
                        sLog.outError("Spell %u '%s' not fit to (" I64FMT "," I32FMT ") but used in %s.", spell, name.c_str(), familyMaskA, familyMaskB, code.c_str());
                        continue;
                    }
                }
            }

            if (spellIcon >= 0 && spellEntry->SpellIconID != uint32(spellIcon))
            {
                sLog.outError("Spell %u '%s' icon(%u) <> %u but used in %s.", spell, name.c_str(), spellEntry->SpellIconID, spellIcon, code.c_str());
                continue;
            }

            if (spellVisual >= 0 && spellEntry->SpellVisual[0] != uint32(spellVisual))
            {
                sLog.outError("Spell %u '%s' visual(%u) <> %u but used in %s.", spell, name.c_str(), spellEntry->SpellVisual[0], spellVisual, code.c_str());
                continue;
            }

            if (category >= 0 && spellEntry->GetCategory() != uint32(category))
            {
                sLog.outError("Spell %u '%s' category(%u) <> %u but used in %s.",spell,name.c_str(),spellEntry->GetCategory(),category,code.c_str());
                continue;
            }

            if (effectIdx >= EFFECT_INDEX_0)
            {
                SpellEffectEntry const* spellEffect = spellEntry->GetSpellEffect(SpellEffectIndex(effectIdx));

                if (effectType >= 0 && spellEffect && spellEffect->Effect != uint32(effectType))
                {
                    sLog.outError("Spell %u '%s' effect%d <> %u but used in %s.", spell, name.c_str(), effectIdx + 1, effectType, code.c_str());
                    continue;
                }

                if (auraType >= 0 && spellEffect && spellEffect->EffectApplyAuraName != uint32(auraType))
                {
                    sLog.outError("Spell %u '%s' aura%d <> %u but used in %s.", spell, name.c_str(), effectIdx + 1, auraType, code.c_str());
                    continue;
                }
            }
            else
            {
                if (effectType >= 0 && !IsSpellHaveEffect(spellEntry, SpellEffects(effectType)))
                {
                    sLog.outError("Spell %u '%s' not have effect %u but used in %s.", spell, name.c_str(), effectType, code.c_str());
                    continue;
                }

                if (auraType >= 0 && !IsSpellHaveAura(spellEntry, AuraType(auraType)))
                {
                    sLog.outError("Spell %u '%s' not have aura %u but used in %s.", spell, name.c_str(), auraType, code.c_str());
                    continue;
                }
            }
        }
        else
        {
            ++countMasks;

            bool found = false;
            for (uint32 spellId = 1; spellId < sSpellStore.GetNumRows(); ++spellId)
            {
                SpellEntry const* spellEntry = sSpellStore.LookupEntry(spellId);
                if (!spellEntry)
                {
                    continue;
                }

                SpellClassOptionsEntry const* classOptions = spellEntry->GetSpellClassOptions();

                if (family >=0 && classOptions && classOptions->SpellFamilyName != uint32(family))
                {
                    continue;
                }

                if (familyMaskA != UI64LIT(0xFFFFFFFFFFFFFFFF) || familyMaskB != 0xFFFFFFFF)
                {
                    if (familyMaskA == UI64LIT(0x0000000000000000) && familyMaskB == 0x00000000)
                    {
                        if (classOptions && classOptions->SpellFamilyFlags)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if (!spellEntry->IsFitToFamilyMask(familyMaskA, familyMaskB))
                        {
                            continue;
                        }
                    }
                }

                if (spellIcon >= 0 && spellEntry->SpellIconID != uint32(spellIcon))
                {
                    continue;
                }

                if (spellVisual >= 0 && spellEntry->SpellVisual[0] != uint32(spellVisual))
                {
                    continue;
                }

                if (category >= 0 && spellEntry->GetCategory() != uint32(category))
                {
                    continue;
                }

                if (effectIdx >= 0)
                {
                    SpellEffectEntry const* spellEffect = spellEntry->GetSpellEffect(SpellEffectIndex(effectIdx));

                    if (effectType >=0 && spellEffect && spellEffect->Effect != uint32(effectType))
                    {
                        continue;
                    }

                    if (auraType >=0 && spellEffect && spellEffect->EffectApplyAuraName != uint32(auraType))
                    {
                        continue;
                    }
                }
                else
                {
                    if (effectType >= 0 && !IsSpellHaveEffect(spellEntry, SpellEffects(effectType)))
                    {
                        continue;
                    }

                    if (auraType >= 0 && !IsSpellHaveAura(spellEntry, AuraType(auraType)))
                    {
                        continue;
                    }
                }

                found = true;
                break;
            }

            if (!found)
            {
                if (effectIdx >= 0)
                    sLog.outError("Spells '%s' not found for family %i (" I64FMT "," I32FMT ") icon(%i) visual(%i) category(%i) effect%d(%i) aura%d(%i) but used in %s",
                                  name.c_str(), family, familyMaskA, familyMaskB, spellIcon, spellVisual, category, effectIdx + 1, effectType, effectIdx + 1, auraType, code.c_str());
                else
                    sLog.outError("Spells '%s' not found for family %i (" I64FMT "," I32FMT ") icon(%i) visual(%i) category(%i) effect(%i) aura(%i) but used in %s",
                                  name.c_str(), family, familyMaskA, familyMaskB, spellIcon, spellVisual, category, effectType, auraType, code.c_str());
                continue;
            }
        }
    }
    while (result->NextRow());

    delete result;

    sLog.outString();
    sLog.outString(">> Checked %u spells and %u spell masks", countSpells, countMasks);
}

/**
 * @brief Returns the diminishing returns group for a spell.
 *
 * @param spellproto The spell entry.
 * @param triggered Whether the spell was triggered instead of directly cast.
 * @return The diminishing returns group.
 */
DiminishingGroup GetDiminishingReturnsGroupForSpell(SpellEntry const* spellproto, bool triggered)
{
    // Explicit Diminishing Groups
    SpellClassOptionsEntry const* classOptions = spellproto->GetSpellClassOptions();

    switch(spellproto->GetSpellFamilyName())
    {
        case SPELLFAMILY_GENERIC:
            // some generic arena related spells have by some strange reason MECHANIC_TURN
            if  (spellproto->GetMechanic() == MECHANIC_TURN)
            {
                return DIMINISHING_NONE;
            }
            break;
        case SPELLFAMILY_MAGE:
            // Dragon's Breath
            if (spellproto->SpellIconID == 1548)
            {
                return DIMINISHING_DISORIENT;
            }
            break;
        case SPELLFAMILY_ROGUE:
        {
            // Blind
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00001000000))
            {
                return DIMINISHING_FEAR_CHARM_BLIND;
            }
            // Cheap Shot
            else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00000000400))
            {
                return DIMINISHING_CHEAPSHOT_POUNCE;
            }
            // Crippling poison - Limit to 10 seconds in PvP (No SpellFamilyFlags)
            else if (spellproto->SpellIconID == 163)
            {
                return DIMINISHING_LIMITONLY;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Freezing Trap & Freezing Arrow & Wyvern Sting
            if (spellproto->SpellIconID == 180 || spellproto->SpellIconID == 1721)
            {
                return DIMINISHING_DISORIENT;
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Curses/etc
            if (spellproto->IsFitToFamilyMask(UI64LIT(0x00080000000)))
            {
                return DIMINISHING_LIMITONLY;
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Judgement of Justice - Limit to 10 seconds in PvP
            if (spellproto->IsFitToFamilyMask(UI64LIT(0x00000100000)))
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00080000000))
            {
                return DIMINISHING_LIMITONLY;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Cyclone
            if (spellproto->IsFitToFamilyMask(UI64LIT(0x02000000000)))
            {
                return DIMINISHING_CYCLONE;
            }
            // Pounce
            else if (spellproto->IsFitToFamilyMask(UI64LIT(0x00000020000)))
            {
                return DIMINISHING_CHEAPSHOT_POUNCE;
            }
            // Faerie Fire
            else if (spellproto->IsFitToFamilyMask(UI64LIT(0x00000000400)))
            {
                return DIMINISHING_LIMITONLY;
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Hamstring - limit duration to 10s in PvP
            if (spellproto->IsFitToFamilyMask(UI64LIT(0x00000000002)))
            {
                return DIMINISHING_LIMITONLY;
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Shackle Undead
            if (spellproto->SpellIconID == 27)
            {
                return DIMINISHING_DISORIENT;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Hungering Cold (no flags)
            if (spellproto->SpellIconID == 2797)
            {
                return DIMINISHING_DISORIENT;
            }
            break;
        }
        default:
            break;
    }

    // Get by mechanic
    uint32 mechanic = GetAllSpellMechanicMask(spellproto);
    if (!mechanic)
    {
        return DIMINISHING_NONE;
    }

    if (mechanic & ((1 << (MECHANIC_STUN - 1)) | (1 << (MECHANIC_SHACKLE - 1))))
    {
        return triggered ? DIMINISHING_TRIGGER_STUN : DIMINISHING_CONTROL_STUN;
    }
    if (mechanic & ((1 << (MECHANIC_SLEEP - 1)) | (1 << (MECHANIC_FREEZE - 1))))
    {
        return DIMINISHING_FREEZE_SLEEP;
    }
    if (mechanic & ((1 << (MECHANIC_KNOCKOUT - 1)) | (1 << (MECHANIC_POLYMORPH - 1)) | (1 << (MECHANIC_SAPPED - 1))))
    {
        return DIMINISHING_DISORIENT;
    }
    if (mechanic & (1 << (MECHANIC_ROOT - 1)))
    {
        return triggered ? DIMINISHING_TRIGGER_ROOT : DIMINISHING_CONTROL_ROOT;
    }
    if (mechanic & ((1 << (MECHANIC_FEAR - 1)) | (1 << (MECHANIC_CHARM - 1)) | (1 << (MECHANIC_TURN - 1))))
    {
        return DIMINISHING_FEAR_CHARM_BLIND;
    }
    if (mechanic & ((1 << (MECHANIC_SILENCE - 1)) | (1 << (MECHANIC_INTERRUPT - 1))))
    {
        return DIMINISHING_SILENCE;
    }
    if (mechanic & (1 << (MECHANIC_DISARM - 1)))
    {
        return DIMINISHING_DISARM;
    }
    if (mechanic & (1 << (MECHANIC_BANISH - 1)))
    {
        return DIMINISHING_BANISH;
    }
    if (mechanic & (1 << (MECHANIC_HORROR - 1)))
    {
        return DIMINISHING_HORROR;
    }

    return DIMINISHING_NONE;
}

int32 GetDiminishingReturnsLimitDuration(DiminishingGroup group, SpellEntry const* spellproto)
{
    if (!IsDiminishingReturnsGroupDurationLimited(group))
    {
        return 0;
    }

    SpellClassOptionsEntry const* classOptions = spellproto->GetSpellClassOptions();

    // Explicit diminishing duration
    switch(spellproto->GetSpellFamilyName())
    {
        case SPELLFAMILY_HUNTER:
        {
            // Wyvern Sting
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000100000000000))
            {
                return 6000;
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Repentance - limit to 6 seconds in PvP
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00000000004))
            {
                return 6000;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Faerie Fire - limit to 40 seconds in PvP (3.1)
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00000000400))
            {
                return 40000;
            }
            break;
        }
        default:
            break;
    }

    return 8000;
}

/**
 * @brief Checks whether a diminishing returns group has a PvP duration limit.
 *
 * @param group The diminishing returns group.
 * @return true if the group's duration is limited; otherwise false.
 */
bool IsDiminishingReturnsGroupDurationLimited(DiminishingGroup group)
{
    switch (group)
    {
        case DIMINISHING_CONTROL_STUN:
        case DIMINISHING_TRIGGER_STUN:
        case DIMINISHING_CONTROL_ROOT:
        case DIMINISHING_TRIGGER_ROOT:
        case DIMINISHING_FEAR_CHARM_BLIND:
        case DIMINISHING_DISORIENT:
        case DIMINISHING_CHEAPSHOT_POUNCE:
        case DIMINISHING_FREEZE_SLEEP:
        case DIMINISHING_CYCLONE:
        case DIMINISHING_BANISH:
        case DIMINISHING_LIMITONLY:
            return true;
        default:
            return false;
    }
    return false;
}

/**
 * @brief Returns the application scope used for a diminishing returns group.
 *
 * @param group The diminishing returns group.
 * @return The diminishing returns type.
 */
DiminishingReturnsType GetDiminishingReturnsGroupType(DiminishingGroup group)
{
    switch (group)
    {
        case DIMINISHING_CYCLONE:
        case DIMINISHING_TRIGGER_STUN:
        case DIMINISHING_CONTROL_STUN:
            return DRTYPE_ALL;
        case DIMINISHING_CONTROL_ROOT:
        case DIMINISHING_TRIGGER_ROOT:
        case DIMINISHING_FEAR_CHARM_BLIND:
        case DIMINISHING_DISORIENT:
        case DIMINISHING_SILENCE:
        case DIMINISHING_DISARM:
        case DIMINISHING_HORROR:
        case DIMINISHING_FREEZE_SLEEP:
        case DIMINISHING_BANISH:
        case DIMINISHING_CHEAPSHOT_POUNCE:
            return DRTYPE_PLAYER;
        default:
            break;
    }

    return DRTYPE_NONE;
}

/**
 * @brief Checks whether a player satisfies a spell-area requirement record.
 *
 * @param player The player being evaluated.
 * @param newZone The current zone identifier.
 * @param newArea The current area identifier.
 * @return true if all requirements are met; otherwise, false.
 */
bool SpellArea::IsFitToRequirements(Player const* player, uint32 newZone, uint32 newArea) const
{
    if (conditionId)
    {
        if (!player || !sObjectMgr.IsPlayerMeetToCondition(conditionId, player, player->GetMap(), NULL, CONDITION_FROM_SPELL_AREA))
        {
            return false;
        }
    }
    else                                                    // This block will be removed
    {
        if (gender != GENDER_NONE)
        {
            // not in expected gender
            if (!player || gender != player->getGender())
            {
                return false;
            }
        }

        if (raceMask)
        {
            // not in expected race
            if (!player || !(raceMask & player->getRaceMask()))
            {
                return false;
            }
        }

        if (questStart)
        {
            // not in expected required quest state
            if (!player || (!questStartCanActive || !player->IsActiveQuest(questStart)) && !player->GetQuestRewardStatus(questStart))
            {
                return false;
            }
        }

        if (questEnd)
        {
            // not in expected forbidden quest state
            if (!player || player->GetQuestRewardStatus(questEnd))
            {
                return false;
            }
        }
    }

    if (areaId)
    {
        // not in expected zone
        if (newZone != areaId && newArea != areaId)
        {
            return false;
        }
    }

    if (auraSpell)
    {
        // not have expected aura
        if (!player)
        {
            return false;
        }
        if (auraSpell > 0)
            // have expected aura
        {
            return player->HasAura(auraSpell);
        }
        else
            // not have expected aura
        {
            return !player->HasAura(-auraSpell);
        }
    }

    return true;
}

/**
 * @brief Applies or removes an area-based spell according to requirements.
 *
 * @param player The player to update.
 * @param newZone The current zone identifier.
 * @param newArea The current area identifier.
 * @param onlyApply true to skip aura removal when requirements fail.
 */
void SpellArea::ApplyOrRemoveSpellIfCan(Player* player, uint32 newZone, uint32 newArea, bool onlyApply) const
{
    MANGOS_ASSERT(player);

    if (IsFitToRequirements(player, newZone, newArea))
    {
        if (autocast && !player->HasAura(spellId))
        {
            player->CastSpell(player, spellId, true);
        }
    }
    else if (!onlyApply && player->HasAura(spellId))
    {
        player->RemoveAurasDueToSpell(spellId);
    }
}

SpellEntry const* GetSpellEntryByDifficulty(uint32 id, Difficulty difficulty, bool isRaid)
{
    SpellDifficultyEntry const* spellDiff = sSpellDifficultyStore.LookupEntry(id);

    if (!spellDiff)
    {
        return NULL;
    }

    for (Difficulty diff = difficulty; diff >= REGULAR_DIFFICULTY; diff = GetPrevDifficulty(diff, isRaid))
    {
        if (spellDiff->spellId[diff])
        {
            return sSpellStore.LookupEntry(spellDiff->spellId[diff]);
        }
    }

    return NULL;
}

int32 GetMasteryCoefficient(SpellEntry const * spellProto)
{
    if (!spellProto || !spellProto->HasAttribute(SPELL_ATTR_EX8_MASTERY))
    {
        return 0;
    }

    // Find mastery scaling coef
    int32 coef = 0;
    for (uint32 j = 0; j < MAX_EFFECT_INDEX; ++j)
    {
        SpellEffectEntry const * effectEntry = spellProto->GetSpellEffect(SpellEffectIndex(j));
        if (!effectEntry)
        {
            continue;
        }

        // mastery scaling coef is stored in dummy aura, except 77215 (Potent Afflictions, zero effect)
        // and 76808 (Executioner, not stored at all)
        int32 bp = effectEntry->CalculateSimpleValue();
        if (spellProto->Id == 76808)
        {
            bp = 250;
        }

        if (!bp)
        {
            continue;
        }

        coef = bp;
        break;
    }

    return coef;
}

