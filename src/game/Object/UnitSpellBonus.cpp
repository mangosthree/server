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
#include "MovementGenerator.h"
#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"
#include "CreatureLinkingMgr.h"
#include "GameTime.h"
// NOTE: movement/MovementStructures.h intentionally NOT included here - it defines
// the MovementStatusElements sequence arrays at file scope (one-TU-only), owned by Unit.cpp.
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#include "ElunaConfig.h"
#include "ElunaEventMgr.h"
#endif /* ENABLE_ELUNA */

#include <math.h>
#include <stdarg.h>

/** Calculate spell coefficents and level penalties for spell/melee damage or heal
 *
 * this is the caster of the spell/ melee attacker
 * @param spellProto SpellEntry of the used spell
 * @param total current value onto which the Bonus and level penalty will be calculated
 * @param benefit additional benefit from ie spellpower-auras
 * @param ap_benefit additional melee attackpower benefit from auras
 * @param damagetype what kind of damage
 * @param donePart calculate for done or taken
 * @param defCoeffMod default coefficient for additional scaling (i.e. normal player healing SCALE_SPELLPOWER_HEALING)
 */
int32 Unit::SpellBonusWithCoeffs(SpellEntry const* spellProto, int32 total, int32 benefit, int32 ap_benefit,  DamageEffectType damagetype, bool donePart, float defCoeffMod)
{
    // Distribute Damage over multiple effects, reduce by AoE
    float coeff = 1.0f;

    // Not apply this to creature casted spells
    if (GetTypeId() == TYPEID_UNIT && !((Creature*)this)->IsPet())
    {
        coeff = 1.0f;
    }
    // Check for table values
    else if (SpellBonusEntry const* bonus = sSpellMgr.GetSpellBonusData(spellProto->Id))
    {
        coeff = damagetype == DOT ? bonus->dot_damage : bonus->direct_damage;

        // apply ap bonus at done part calculation only (it flat total mod so common with taken)
        if (donePart && (bonus->ap_bonus || bonus->ap_dot_bonus))
        {
            float ap_bonus = damagetype == DOT ? bonus->ap_dot_bonus : bonus->ap_bonus;

            // Impurity
            if (GetTypeId() == TYPEID_PLAYER && spellProto->GetSpellFamilyName() == SPELLFAMILY_DEATHKNIGHT)
            {
                if (SpellEntry const* spell = ((Player*)this)->GetKnownTalentRankById(2005))
                {
                    ap_bonus += ((spell->CalculateSimpleValue(EFFECT_INDEX_0) * ap_bonus) / 100.0f);
                }
            }

            total += int32(ap_bonus * (GetTotalAttackPowerValue(IsSpellRequiresRangedAP(spellProto) ? RANGED_ATTACK : BASE_ATTACK) + ap_benefit));
        }
    }
    // Default calculation
    else if (benefit)
    {
        coeff = CalculateDefaultCoefficient(spellProto, damagetype) * defCoeffMod;
    }

    if (benefit)
    {
        float LvlPenalty = CalculateLevelPenalty(spellProto);

        // Spellmod SpellDamage
        if (Player* modOwner = GetSpellModOwner())
        {
            coeff *= 100.0f;
            modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_SPELL_BONUS_DAMAGE, coeff);
            coeff /= 100.0f;
        }

        total += int32(benefit * coeff * LvlPenalty);
    }

    return total;
};

/**
 * Calculates caster part of spell damage bonuses,
 * also includes different bonuses dependent from target auras
 */
uint32 Unit::SpellDamageBonusDone(Unit* pVictim, SpellEntry const* spellProto, uint32 pdamage, DamageEffectType damagetype, uint32 stack)
{
    if (!spellProto || !pVictim || damagetype == DIRECT_DAMAGE || spellProto->HasAttribute(SPELL_ATTR_EX6_NO_DMG_MODS))
    {
        return pdamage;
    }

    // For totems get damage bonus from owner (statue isn't totem in fact)
    if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsTotem() && ((Totem*)this)->GetTotemType() != TOTEM_STATUE)
    {
        if (Unit* owner = GetOwner())
        {
            return owner->SpellDamageBonusDone(pVictim, spellProto, pdamage, damagetype);
        }
    }

    uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();
    float DoneTotalMod = 1.0f;
    int32 DoneTotal = 0;

    // Creature damage
    if (GetTypeId() == TYPEID_UNIT && !((Creature*)this)->IsPet())
    {
        DoneTotalMod *= ((Creature*)this)->_GetSpellDamageMod(((Creature*)this)->GetCreatureInfo()->Rank);
    }

    AuraList const& mModDamagePercentDone = GetAurasByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
    for (AuraList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
    {
        SpellEquippedItemsEntry const* spellEquip = (*i)->GetSpellProto()->GetSpellEquippedItems();
        if (((*i)->GetModifier()->m_miscvalue & GetSpellSchoolMask(spellProto)) &&
            (!spellEquip || spellEquip->EquippedItemClass == -1 &&
            // -1 == any item class (not wand then)
            spellEquip->EquippedItemInventoryTypeMask == 0))
            // 0 == any inventory type (not wand then)
        {
            DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
        }
    }

    // Add flat bonus from spell damage versus
    DoneTotal += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS, creatureTypeMask);

    // Add pct bonus from spell damage versus
    DoneTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS, creatureTypeMask);

    // Add flat bonus from spell damage creature
    DoneTotal += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_DAMAGE_DONE_CREATURE, creatureTypeMask);

    if (GetPowerType() == POWER_MANA)
    {
        Unit::AuraList const& doneFromManaPctAuras = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE_FROM_PCT_POWER);
        if (!doneFromManaPctAuras.empty())
        {
            uint32 maxPower = GetMaxPower(POWER_MANA);
            float powerPct = maxPower ? std::min(float(GetPower(POWER_MANA)) / maxPower, 1.0f) : 0.0f;
            for (Unit::AuraList::const_iterator itr = doneFromManaPctAuras.begin(); itr != doneFromManaPctAuras.end(); ++itr)
            {
                if (GetSpellSchoolMask(spellProto) & (*itr)->GetModifier()->m_miscvalue)
                {
                    DoneTotalMod *= (100.0f + (*itr)->GetModifier()->m_amount * powerPct) / 100.0f;
                }
            }
        }
    }

    // Mastery: Unshackled Fury (Fury Warrior 76856) — applies to non-
    // melee-class warrior damage spells (rare; most warrior abilities
    // go through MeleeDamageBonusDone instead). Same direct-aura-read
    // pattern as the melee-path version. See MeleeDamageBonusDone for
    // the canonical implementation comment.
    if (GetPowerType() == POWER_RAGE)
    {
        if (SpellAuraHolder* ufHolder = GetSpellAuraHolder(76856))
        {
            if (Aura* uf = ufHolder->GetAuraByEffectIndex(EFFECT_INDEX_0))
            {
                if (uf->GetModifier()->m_amount > 0)
                {
                    uint32 maxPower = GetMaxPower(POWER_RAGE);
                    float powerPct = maxPower ? std::min(float(GetPower(POWER_RAGE)) / maxPower, 1.0f) : 0.0f;
                    DoneTotalMod *= (100.0f + uf->GetModifier()->m_amount * powerPct) / 100.0f;
                }
            }
        }
    }

    // Mastery: Frostburn (Frost Mage 76613) — increases damage dealt to
    // frozen targets by mastery%. Cata mechanic: the bonus only applies
    // when the victim is in AURA_STATE_FROZEN (Frostbite / Freeze /
    // Frost Nova / Deep Freeze / Ice Lance vs. frozen / etc.). The
    // aura's m_miscvalue + family flags filter the affected spells
    // (Frostbolt, Ice Lance, Frostfire Bolt — Mage family). Mirrors
    // TC-Preservation at tc-preservation/src/server/game/Entities/Unit/
    // Unit.cpp:6569-6573. Guard against the DBC placeholder (negative)
    // bleeding through when SPELL_AURA_MASTERY isn't applied yet (e.g.
    // mid-spec-swap, talent reset). UpdateMasteryAuras only overrides
    // m_amount to the positive mastery-scaled value once the caster has
    // the SPELL_AURA_MASTERY aura active; before that, the negative
    // placeholder would apply as a damage REDUCTION on frozen targets.
    if (pVictim)
    {
        if (SpellAuraHolder* frostburnHolder = GetSpellAuraHolder(76613))
        {
            if (Aura* frostburn = frostburnHolder->GetAuraByEffectIndex(EFFECT_INDEX_0))
            {
                if (frostburn->GetModifier()->m_amount > 0 &&
                    pVictim->HasAuraState(AURA_STATE_FROZEN) &&
                    frostburn->isAffectedOnSpell(spellProto))
                {
                    DoneTotalMod *= (100.0f + frostburn->GetModifier()->m_amount) / 100.0f;
                }
            }
        }
    }

    // done scripted mod (take it from owner)
    Unit* owner = GetOwner();
    if (!owner)
    {
        owner = this;
    }

    AuraList const& mOverrideClassScript = owner->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->isAffectedOnSpell(spellProto))
        {
            continue;
        }

        switch ((*i)->GetModifier()->m_miscvalue)
        {
                // Molten Fury
            case 4920:
            case 4919:
            case 6917: // Death's Embrace
            case 6926:
            case 6928:
            {
                if (pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                {
                    DoneTotalMod *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
                }
                break;
            }
            // Soul Siphon
            case 4992:
            case 4993:
            {
                // effect 1 m_amount
                int32 maxPercent = (*i)->GetModifier()->m_amount;
                // effect 0 m_amount
                int32 stepPercent = CalculateSpellDamage(this, (*i)->GetSpellProto(), EFFECT_INDEX_0);
                // count affliction effects and calc additional damage in percentage
                int32 modPercent = 0;
                SpellAuraHolderMap const& victimAuras = pVictim->GetSpellAuraHolderMap();
                for (SpellAuraHolderMap::const_iterator itr = victimAuras.begin(); itr != victimAuras.end(); ++itr)
                {
                    SpellEntry const* m_spell = itr->second->GetSpellProto();
                    SpellClassOptionsEntry const* itrClassOptions = m_spell->GetSpellClassOptions();
                    if (itrClassOptions && (itrClassOptions->SpellFamilyName != SPELLFAMILY_WARLOCK || !(itrClassOptions->SpellFamilyFlags & UI64LIT(0x0004071B8044C402))))
                    {
                        continue;
                    }
                    modPercent += stepPercent * itr->second->GetStackAmount();
                    if (modPercent >= maxPercent)
                    {
                        modPercent = maxPercent;
                        break;
                    }
                }
                DoneTotalMod *= (modPercent + 100.0f) / 100.0f;
                break;
            }
            case 6916: // Death's Embrace
            case 6925:
            case 6927:
                if (HasAuraState(AURA_STATE_HEALTHLESS_20_PERCENT))
                {
                    DoneTotalMod *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
                }
                break;
            case 5481: // Starfire Bonus
            {
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, UI64LIT(0x0000000000200002)))
                {
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            }
            case 4418: // Increased Shock Damage
            case 4554: // Increased Lightning Damage
            case 4555: // Improved Moonfire
            case 5142: // Increased Lightning Damage
            case 5147: // Improved Consecration / Libram of Resurgence
            case 5148: // Idol of the Shooting Star
            case 6008: // Increased Lightning Damage
            case 8627: // Totem of Hex
            {
                DoneTotal += (*i)->GetModifier()->m_amount;
                break;
            }
            // Tundra Stalker
            // Merciless Combat
            case 7277:
            {
                // Merciless Combat
                if ((*i)->GetSpellProto()->SpellIconID == 2656)
                {
                    if (pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                    {
                        DoneTotalMod *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
                    }
                }
                else // Tundra Stalker
                {
                    // Frost Fever (target debuff)
                    if (pVictim->GetAura(SPELL_AURA_MOD_MELEE_HASTE, SPELLFAMILY_DEATHKNIGHT, UI64LIT(0x0000000000000000), 0x00000002))
                    {
                        DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                    }
                    break;
                }
                break;
            }
            case 7293: // Rage of Rivendare
            {
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DEATHKNIGHT, UI64LIT(0x0200000000000000)))
                {
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            }
            // Twisted Faith
            case 7377:
            {
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, UI64LIT(0x0000000000008000), 0, GetObjectGuid()))
                {
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            }
            // Marked for Death
            case 7598:
            case 7599:
            case 7600:
            case 7601:
            case 7602:
            {
                if (pVictim->GetAura(SPELL_AURA_MOD_STALKED, SPELLFAMILY_HUNTER, UI64LIT(0x0000000000000400)))
                {
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            }
        }
    }

    SpellClassOptionsEntry const* classOptions = spellProto->GetSpellClassOptions();

    // Custom scripted damage
    switch(spellProto->GetSpellFamilyName())
    {
        case SPELLFAMILY_MAGE:
        {
            // Ice Lance
            if (spellProto->SpellIconID == 186)
            {
                if (pVictim->IsFrozen() || IsIgnoreUnitState(spellProto, IGNORE_UNIT_TARGET_NON_FROZEN))
                {
                    float multiplier = 3.0f;

                    // if target have higher level
                    if (pVictim->getLevel() > getLevel())
                        // Glyph of Ice Lance
                        if (Aura* glyph = GetDummyAura(56377))
                        {
                            multiplier = glyph->GetModifier()->m_amount;
                        }

                    DoneTotalMod *= multiplier;
                }
            }
            // Torment the weak affected (Arcane Barrage, Arcane Blast, Frostfire Bolt, Arcane Missiles, Fireball)
            if (classOptions && (classOptions->SpellFamilyFlags & UI64LIT(0x0000900020200021)) &&
                (pVictim->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED) || pVictim->HasAuraType(SPELL_AURA_HASTE_ALL)))
            {
                // Search for Torment the weak dummy aura
                Unit::AuraList const& ttw = GetAurasByType(SPELL_AURA_DUMMY);
                for (Unit::AuraList::const_iterator i = ttw.begin(); i != ttw.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 3263)
                    {
                        DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                        break;
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_WARLOCK:
        {
            // Drain Soul
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000004000))
            {
                if (pVictim->GetMaxHealth() && pVictim->GetHealth() * 100 / pVictim->GetMaxHealth() <= 25)
                {
                    DoneTotalMod *= 4;
                }
            }
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            // Smite
            if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000000080)))
            {
                // Holy Fire
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, UI64LIT(0x00100000)))
                    // Glyph of Smite
                    if (Aura* aur = GetAura(55692, EFFECT_INDEX_0))
                    {
                        DoneTotalMod *= (aur->GetModifier()->m_amount + 100.0f) / 100.0f;
                    }
            }
            // Shadow word: Death
            else if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000200000000)))
            {
                // Glyph of Shadow word: Death
                if (SpellAuraHolder const* glyph = GetSpellAuraHolder(55682))
                {
                    Aura const* hpPct = glyph->GetAuraByEffectIndex(EFFECT_INDEX_0);
                    Aura const* dmPct = glyph->GetAuraByEffectIndex(EFFECT_INDEX_1);
                    if (hpPct && dmPct && pVictim->GetHealth() * 100 <= pVictim->GetMaxHealth() * hpPct->GetModifier()->m_amount)
                    {
                        DoneTotalMod *= (dmPct->GetModifier()->m_amount + 100.0f) / 100.0f;
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Improved Insect Swarm (Wrath part)
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000000001))
            {
                // if Insect Swarm on target
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, UI64LIT(0x000000000200000), 0, GetObjectGuid()))
                {
                    Unit::AuraList const& improvedSwarm = GetAurasByType(SPELL_AURA_DUMMY);
                    for (Unit::AuraList::const_iterator iter = improvedSwarm.begin(); iter != improvedSwarm.end(); ++iter)
                    {
                        if ((*iter)->GetSpellProto()->SpellIconID == 1771)
                        {
                            DoneTotalMod *= ((*iter)->GetModifier()->m_amount + 100.0f) / 100.0f;
                            break;
                        }
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Icy Touch and Howling Blast
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000200000002))
            {
                // search disease
                bool found = false;
                Unit::SpellAuraHolderMap const& auras = pVictim->GetSpellAuraHolderMap();
                for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                {
                    if (itr->second->GetSpellProto()->GetDispel() == DISPEL_DISEASE)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    break;
                }

                // search for Glacier Rot dummy aura
                Unit::AuraList const& dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
                for (Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->GetEffectMiscValue((*i)->GetEffIndex()) == 7244)
                    {
                        DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                        break;
                    }
                }
            }
            // Death Coil (bonus from Item - Death Knight T8 DPS Relic)
            else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00002000))
            {
                if (Aura* sigil = GetDummyAura(64962))
                {
                    DoneTotal += sigil->GetModifier()->m_amount;
                }
            }
            break;
        }
        default:
            break;
    }

    // Done fixed damage bonus auras
    int32 DoneAdvertisedBenefit = SpellBaseDamageBonusDone(GetSpellSchoolMask(spellProto));

    // Pets just add their bonus damage to their spell damage
    // note that their spell damage is just gain of their own auras
    if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet())
    {
        DoneAdvertisedBenefit += ((Pet*)this)->GetBonusDamage();
    }

    // apply ap bonus and benefit affected by spell power implicit coeffs and spell level penalties
    DoneTotal = SpellBonusWithCoeffs(spellProto, DoneTotal, DoneAdvertisedBenefit, 0, damagetype, true);

    float tmpDamage = (int32(pdamage) + DoneTotal * int32(stack)) * DoneTotalMod;
    // apply spellmod to Done damage (flat and pct)
    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, tmpDamage);
    }

    return tmpDamage > 0 ? uint32(tmpDamage) : 0;
}

/**
 * Calculates target part of spell damage bonuses,
 * will be called on each tick for periodic damage over time auras
 */
uint32 Unit::SpellDamageBonusTaken(Unit* pCaster, SpellEntry const* spellProto, uint32 pdamage, DamageEffectType damagetype, uint32 stack)
{
    if (!spellProto || !pCaster || damagetype == DIRECT_DAMAGE)
    {
        return pdamage;
    }

    uint32 schoolMask = spellProto->SchoolMask;

    // Taken total percent damage auras
    float TakenTotalMod = 1.0f;
    int32 TakenTotal = 0;

    // ..taken
    TakenTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, schoolMask);

    // .. taken pct: dummy auras
    AuraList const& mDummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    for (AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
    {
        switch ((*i)->GetId())
        {
            case 45182:                                     // Cheating Death
                if ((*i)->GetModifier()->m_miscvalue & SPELL_SCHOOL_MASK_NORMAL)
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                    {
                        continue;
                    }

                    TakenTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            case 20911:                                     // Blessing of Sanctuary
            case 25899:                                     // Greater Blessing of Sanctuary
                TakenTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                break;
            case 47580:                                     // Pain and Suffering (Rank 1)      TODO: can be pct modifier aura
            case 47581:                                     // Pain and Suffering (Rank 2)
            case 47582:                                     // Pain and Suffering (Rank 3)
                // Shadow Word: Death
                if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000200000000)))
                {
                    TakenTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
        }
    }

    // From caster spells
    AuraList const& mOwnerTaken = GetAurasByType(SPELL_AURA_MOD_DAMAGE_FROM_CASTER);
    for (AuraList::const_iterator i = mOwnerTaken.begin(); i != mOwnerTaken.end(); ++i)
    {
        if ((*i)->GetCasterGuid() == pCaster->GetObjectGuid() && (*i)->isAffectedOnSpell(spellProto))
        {
            TakenTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
        }
    }

    // Mod damage from spell mechanic
    TakenTotalMod *= GetTotalAuraMultiplierByMiscValueForMask(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT, GetAllSpellMechanicMask(spellProto));

    // Mod damage taken from AoE spells
    if (IsAreaOfEffectSpell(spellProto))
    {
        TakenTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE, schoolMask);
        if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet())
        {
            TakenTotalMod *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_PET_AOE_DAMAGE_AVOIDANCE, schoolMask);
        }
    }

    // Taken fixed damage bonus auras
    int32 TakenAdvertisedBenefit = SpellBaseDamageBonusTaken(GetSpellSchoolMask(spellProto));

    // apply benefit affected by spell power implicit coeffs and spell level penalties
    TakenTotal = pCaster->SpellBonusWithCoeffs(spellProto, TakenTotal, TakenAdvertisedBenefit, 0, damagetype, false);

    float tmpDamage = (int32(pdamage) + TakenTotal * int32(stack)) * TakenTotalMod;

    return tmpDamage > 0 ? uint32(tmpDamage) : 0;
}

/**
 * @brief Computes the unit's advertised spell damage bonus for a school mask.
 *
 * @param schoolMask The spell school mask.
 * @return The advertised damage bonus.
 */
int32 Unit::SpellBaseDamageBonusDone(SpellSchoolMask schoolMask)
{
    int32 DoneAdvertisedBenefit = 0;

    Unit::AuraList const& mOverrideSpellPowerAuras = GetAurasByType(SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT);
    if (!mOverrideSpellPowerAuras.empty())
    {
        for (Unit::AuraList::const_iterator itr = mOverrideSpellPowerAuras.begin(); itr != mOverrideSpellPowerAuras.end(); ++itr)
            if (schoolMask & (*itr)->GetModifier()->m_miscvalue)
            {
                DoneAdvertisedBenefit += (*itr)->GetModifier()->m_amount;
            }

        return int32(GetTotalAttackPowerValue(BASE_ATTACK) * (100.0f + DoneAdvertisedBenefit) / 100.0f);
    }

    // ..done
    AuraList const& mDamageDone = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE);
    for (AuraList::const_iterator i = mDamageDone.begin(); i != mDamageDone.end(); ++i)
    {
        SpellEquippedItemsEntry const* spellEquip = (*i)->GetSpellProto()->GetSpellEquippedItems();
        if (((*i)->GetModifier()->m_miscvalue & schoolMask) != 0 &&
            (!spellEquip || spellEquip->EquippedItemClass == -1 &&      // -1 == any item class (not wand then)
            spellEquip->EquippedItemInventoryTypeMask == 0))            //  0 == any inventory type (not wand then)
                DoneAdvertisedBenefit += (*i)->GetModifier()->m_amount;
    }

    if (GetTypeId() == TYPEID_PLAYER)
    {
        // Base value
        DoneAdvertisedBenefit += ((Player*)this)->GetBaseSpellPowerBonus();

        if (GetPowerIndex(POWER_MANA) != INVALID_POWER_INDEX)
        {
            DoneAdvertisedBenefit += std::max(0, int32(GetStat(STAT_INTELLECT)) - 10);  // spellpower from intellect
        }

        // Damage bonus from stats
        AuraList const& mDamageDoneOfStatPercent = GetAurasByType(SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT);
        for (AuraList::const_iterator i = mDamageDoneOfStatPercent.begin(); i != mDamageDoneOfStatPercent.end(); ++i)
        {
            if ((*i)->GetModifier()->m_miscvalue & schoolMask)
            {
                // stat used stored in miscValueB for this aura
                Stats usedStat = Stats((*i)->GetMiscBValue());
                DoneAdvertisedBenefit += int32(GetStat(usedStat) * (*i)->GetModifier()->m_amount / 100.0f);
            }
        }
        // ... and attack power
        AuraList const& mDamageDonebyAP = GetAurasByType(SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER);
        for (AuraList::const_iterator i = mDamageDonebyAP.begin(); i != mDamageDonebyAP.end(); ++i)
        {
            if ((*i)->GetModifier()->m_miscvalue & schoolMask)
            {
                DoneAdvertisedBenefit += int32(GetTotalAttackPowerValue(BASE_ATTACK) * (*i)->GetModifier()->m_amount / 100.0f);
            }
        }
    }

    // pct spell power modifier
    Unit::AuraList const& mSpellPowerPctAuras = GetAurasByType(SPELL_AURA_MOD_INCREASE_SPELL_POWER_PCT);
    for (Unit::AuraList::const_iterator itr = mSpellPowerPctAuras.begin(); itr != mSpellPowerPctAuras.end(); ++itr)
    {
        if (!(*itr)->GetModifier()->m_miscvalue || (*itr)->GetModifier()->m_miscvalue & schoolMask)
        {
            DoneAdvertisedBenefit = int32(DoneAdvertisedBenefit * (100.0f + (*itr)->GetModifier()->m_amount) / 100.0f);
        }
    }

    return DoneAdvertisedBenefit;
}

/**
 * @brief Computes the target's advertised spell damage taken bonus for a school mask.
 *
 * @param schoolMask The spell school mask.
 * @return The advertised taken-damage bonus.
 */
int32 Unit::SpellBaseDamageBonusTaken(SpellSchoolMask schoolMask)
{
    int32 TakenAdvertisedBenefit = 0;

    // ..taken
    AuraList const& mDamageTaken = GetAurasByType(SPELL_AURA_MOD_DAMAGE_TAKEN);
    for (AuraList::const_iterator i = mDamageTaken.begin(); i != mDamageTaken.end(); ++i)
    {
        if (((*i)->GetModifier()->m_miscvalue & schoolMask) != 0)
        {
            TakenAdvertisedBenefit += (*i)->GetModifier()->m_amount;
        }
    }

    return TakenAdvertisedBenefit;
}

/**
 * @brief Checks whether a spell cast critically hits a victim.
 *
 * @param pVictim The spell victim.
 * @param spellProto The spell entry.
 * @param schoolMask The spell school mask.
 * @param attackType The associated attack type.
 * @return True if the spell crits; otherwise, false.
 */
bool Unit::IsSpellCrit(Unit* pVictim, SpellEntry const* spellProto, SpellSchoolMask schoolMask, WeaponAttackType attackType)
{
    // not critting spell
    if (spellProto->HasAttribute(SPELL_ATTR_EX2_CANT_CRIT))
    {
        return false;
    }

    // Creatures do not crit with their spells or abilities, unless it is owned by a player (pet, totem, etc)
    if (GetTypeId() != TYPEID_PLAYER)
    {
        Unit* owner = GetOwner();
        if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
        {
            return false;
        }
    }

    float crit_chance = 0.0f;
    switch(spellProto->GetDmgClass())
    {
        case SPELL_DAMAGE_CLASS_NONE:
            return false;
        case SPELL_DAMAGE_CLASS_MAGIC:
        {
            if (schoolMask & SPELL_SCHOOL_MASK_NORMAL)
            {
                crit_chance = 0.0f;
            }
            // For other schools
            else if (GetTypeId() == TYPEID_PLAYER)
            {
                crit_chance = GetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + GetFirstSchoolInMask(schoolMask));
            }
            else
            {
                crit_chance = float(m_baseSpellCritChance);
                crit_chance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, schoolMask);
            }
            // taken
            if (pVictim)
            {
                if (!IsPositiveSpell(spellProto->Id))
                {
                    // Modify critical chance by victim SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE
                    crit_chance += pVictim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE, schoolMask);
                    // Modify critical chance by victim SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE
                    crit_chance += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE);
                }

                // scripted (increase crit chance ... against ... target by x%)
                // scripted (Increases the critical effect chance of your .... by x% on targets ...)
                AuraList const& mOverrideClassScript = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for (AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
                {
                    if (!((*i)->isAffectedOnSpell(spellProto)))
                    {
                        continue;
                    }
                    switch ((*i)->GetModifier()->m_miscvalue)
                    {
                        case  849:                          // Shatter Rank 1
                            if (pVictim->IsFrozen() || IsIgnoreUnitState(spellProto, IGNORE_UNIT_TARGET_NON_FROZEN))
                            {
                                crit_chance += 17.0f;
                            }
                            break;
                        case  910:                          // Shatter Rank 2
                            if (pVictim->IsFrozen() || IsIgnoreUnitState(spellProto, IGNORE_UNIT_TARGET_NON_FROZEN))
                            {
                                crit_chance += 34.0f;
                            }
                            break;
                        case  911:                          // Shatter Rank 3
                            if (pVictim->IsFrozen() || IsIgnoreUnitState(spellProto, IGNORE_UNIT_TARGET_NON_FROZEN))
                            {
                                crit_chance += 50.0f;
                            }
                            break;
                        case 7917:                          // Glyph of Shadowburn
                            if (pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                            {
                                crit_chance += (*i)->GetModifier()->m_amount;
                            }
                            break;
                        case 7997:                          // Renewed Hope
                        case 7998:
                            if (pVictim->HasAura(6788))
                            {
                                crit_chance += (*i)->GetModifier()->m_amount;
                            }
                            break;
                        default:
                            break;
                    }
                }

                SpellClassOptionsEntry const* classOptions = spellProto->GetSpellClassOptions();
                // Custom crit by class
                switch(spellProto->GetSpellFamilyName())
                {
                    case SPELLFAMILY_MAGE:
                    {
                        // Fire Blast
                        if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000000002)) && spellProto->SpellIconID == 12)
                        {
                            // Glyph of Fire Blast
                            if (pVictim->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED) || pVictim->IsInRoots())
                                if (Aura* aura = GetAura(56369, EFFECT_INDEX_0))
                                {
                                    crit_chance += aura->GetModifier()->m_amount;
                                }
                        }
                        break;
                    }
                    case SPELLFAMILY_PRIEST:
                        // Flash Heal
                        if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000000800)))
                        {
                            if (pVictim->GetHealth() > pVictim->GetMaxHealth() / 2)
                            {
                                break;
                            }
                            AuraList const& mDummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
                            for (AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
                            {
                                // Improved Flash Heal
                                if ((*i)->GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_PRIEST &&
                                    (*i)->GetSpellProto()->SpellIconID == 2542)
                                {
                                    crit_chance += (*i)->GetModifier()->m_amount;
                                    break;
                                }
                            }
                        }
                        break;
                    case SPELLFAMILY_DRUID:
                        // Improved Insect Swarm (Starfire part)
                        if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000000000000004)))
                        {
                            // search for Moonfire on target
                            if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, UI64LIT(0x000000000000002), 0, GetObjectGuid()))
                            {
                                Unit::AuraList const& improvedSwarm = GetAurasByType(SPELL_AURA_DUMMY);
                                for (Unit::AuraList::const_iterator iter = improvedSwarm.begin(); iter != improvedSwarm.end(); ++iter)
                                {
                                    if ((*iter)->GetSpellProto()->SpellIconID == 1771)
                                    {
                                        crit_chance += (*iter)->GetModifier()->m_amount;
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    case SPELLFAMILY_PALADIN:
                        // Sacred Shield
                        if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000040000000))
                        {
                            Aura* aura = pVictim->GetDummyAura(58597);
                            if (aura && aura->GetCasterGuid() == GetObjectGuid())
                            {
                                crit_chance += aura->GetModifier()->m_amount;
                            }
                        }
                        // Exorcism
                        else if (spellProto->GetCategory() == 19)
                        {
                            if (pVictim->GetCreatureTypeMask() & CREATURE_TYPEMASK_DEMON_OR_UNDEAD)
                            {
                                return true;
                            }
                        }
                        break;
                    case SPELLFAMILY_SHAMAN:
                        // Lava Burst
                        if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000100000000000)))
                        {
                            // Flame Shock
                            if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_SHAMAN, UI64LIT(0x0000000010000000), 0, GetObjectGuid()))
                            {
                                return true;
                            }
                        }
                        break;
                }
            }
            break;
        }
        case SPELL_DAMAGE_CLASS_MELEE:
        case SPELL_DAMAGE_CLASS_RANGED:
        {
            if (pVictim)
            {
                crit_chance = GetUnitCriticalChance(attackType, pVictim);
            }

            crit_chance += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, schoolMask);
            break;
        }
        default:
            return false;
    }
    // percent done
    // only players use intelligence for critical chance computations
    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CRITICAL_CHANCE, crit_chance);
    }

    crit_chance = crit_chance > 0.0f ? crit_chance : 0.0f;
    if (roll_chance_f(crit_chance))
    {
        return true;
    }
    return false;
}

/**
 * @brief Applies critical damage bonuses for a spell hit.
 *
 * @param spellProto The spell entry.
 * @param damage The base damage.
 * @param pVictim The victim, if any.
 * @return The damage after critical bonuses.
 */
uint32 Unit::SpellCriticalDamageBonus(SpellEntry const* spellProto, uint32 damage, Unit* pVictim)
{
    // Calculate critical bonus
    int32 crit_bonus;
    switch(spellProto->GetDmgClass())
    {
        case SPELL_DAMAGE_CLASS_MELEE:                      // for melee based spells is 100%
        case SPELL_DAMAGE_CLASS_RANGED:
            crit_bonus = damage;
            break;
        default:
            crit_bonus = damage / 2;                        // for spells is 50%
            break;
    }

    // Apply SPELL_AURA_MOD_CRIT_DAMAGE_BONUS modifier first
    const int32 pctBonus = GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_CRIT_DAMAGE_BONUS, GetSpellSchoolMask(spellProto));
    crit_bonus += int32((damage + crit_bonus) * float(pctBonus / 100.0f));

    // adds additional damage to crit_bonus (from talents)
    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id, SPELLMOD_CRIT_DAMAGE_BONUS, crit_bonus);
    }

    if (!pVictim)
    {
        return damage += crit_bonus;
    }

    int32 critPctDamageMod = 0;
    if (spellProto->GetDmgClass() >= SPELL_DAMAGE_CLASS_MELEE)
    {
        if (GetWeaponAttackType(spellProto) == RANGED_ATTACK)
        {
            critPctDamageMod += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE);
        }
        else
        {
            critPctDamageMod += pVictim->GetTotalAuraModifier(SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE);
        }
    }
    else
    {
        critPctDamageMod += pVictim->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_DAMAGE, GetSpellSchoolMask(spellProto));
    }

    if (critPctDamageMod != 0)
    {
        crit_bonus = int32(crit_bonus * float((100.0f + critPctDamageMod) / 100.0f));
    }

    if (crit_bonus > 0)
    {
        damage += crit_bonus;
    }

    return damage;
}

/**
 * @brief Applies critical healing bonuses for a spell heal.
 *
 * @param spellProto The spell entry.
 * @param damage The base healing amount.
 * @param pVictim The healed victim, if any.
 * @return The healing after critical bonuses.
 */
uint32 Unit::SpellCriticalHealingBonus(SpellEntry const* spellProto, uint32 damage, Unit* pVictim)
{
    // Calculate critical bonus
    int32 crit_bonus = damage;

    if (crit_bonus > 0)
    {
        damage += crit_bonus;
    }

    damage = int32(damage * GetTotalAuraMultiplier(SPELL_AURA_MOD_CRITICAL_HEALING_AMOUNT));

    return damage;
}

/**
 * Calculates caster part of healing spell bonuses,
 * also includes different bonuses dependent from target auras
 */
uint32 Unit::SpellHealingBonusDone(Unit* pVictim, SpellEntry const* spellProto, int32 healamount, DamageEffectType damagetype, uint32 stack)
{
    // For totems get healing bonus from owner (statue isn't totem in fact)
    if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsTotem() && ((Totem*)this)->GetTotemType() != TOTEM_STATUE)
        if (Unit* owner = GetOwner())
        {
            return owner->SpellHealingBonusDone(pVictim, spellProto, healamount, damagetype, stack);
        }

    // No heal amount for this class spells
    if (spellProto->GetDmgClass() == SPELL_DAMAGE_CLASS_NONE)
    {
        return healamount < 0 ? 0 : healamount;
    }

    // Healing Done
    // Done total percent damage auras
    float  DoneTotalMod = 1.0f;
    int32  DoneTotal = 0;

    // Healing done percent
    AuraList const& mHealingDonePct = GetAurasByType(SPELL_AURA_MOD_HEALING_DONE_PERCENT);
    for (AuraList::const_iterator i = mHealingDonePct.begin(); i != mHealingDonePct.end(); ++i)
    {
        DoneTotalMod *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
    }

    AuraList const& mHealingFromHealthPct = GetAurasByType(SPELL_AURA_MOD_HEALING_DONE_FROM_PCT_HEALTH);
    if (!mHealingFromHealthPct.empty())
    {
        uint32 maxHealth = pVictim->GetMaxHealth();
        float healthPct = maxHealth ? std::max(0.0f, 1.0f - float(pVictim->GetHealth()) / maxHealth) : 0.0f;
        for (AuraList::const_iterator i = mHealingFromHealthPct.begin();i != mHealingFromHealthPct.end(); ++i)
            if ((*i)->isAffectedOnSpell(spellProto))
            {
                DoneTotalMod *= (100.0f + (*i)->GetModifier()->m_amount * healthPct) / 100.0f;
            }
    }

    // done scripted mod (take it from owner)
    Unit* owner = GetOwner();
    if (!owner)
    {
        owner = this;
    }
    AuraList const& mOverrideClassScript = owner->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
    {
        if (!(*i)->isAffectedOnSpell(spellProto))
        {
            continue;
        }
        switch ((*i)->GetModifier()->m_miscvalue)
        {
            case 4415: // Increased Rejuvenation Healing
            case 4953:
            case 3736: // Hateful Totem of the Third Wind / Increased Lesser Healing Wave / LK Arena (4/5/6) Totem of the Third Wind / Savage Totem of the Third Wind
                DoneTotal += (*i)->GetModifier()->m_amount;
                break;
            case 7997: // Renewed Hope
            case 7998:
                if (pVictim->HasAura(6788))
                {
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            case   21: // Test of Faith
            case 6935:
            case 6918:
                if (pVictim->GetHealth() < pVictim->GetMaxHealth() / 2)
                {
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            case 7798: // Glyph of Regrowth
            {
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_DRUID, UI64LIT(0x0000000000000040)))
                {
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            }
            case 8477: // Nourish Heal Boost
            {
                int32 stepPercent = (*i)->GetModifier()->m_amount;

                int ownHotCount = 0;                        // counted HoT types amount, not stacks

                Unit::AuraList const& RejorRegr = pVictim->GetAurasByType(SPELL_AURA_PERIODIC_HEAL);
                for (Unit::AuraList::const_iterator itr = RejorRegr.begin(); itr != RejorRegr.end(); ++itr)
                    if ((*itr)->GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_DRUID &&
                            (*itr)->GetCasterGuid() == GetObjectGuid())
                        ++ownHotCount;

                if (ownHotCount)
                {
                    DoneTotalMod *= (stepPercent * ownHotCount + 100.0f) / 100.0f;
                }
                break;
            }
            case 7871: // Glyph of Lesser Healing Wave
            {
                if (pVictim->GetAura(SPELL_AURA_DUMMY, SPELLFAMILY_SHAMAN, UI64LIT(0x0000040000000000), 0, GetObjectGuid()))
                {
                    DoneTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            }
            default:
                break;
        }
    }

    // Nourish 20% of heal increase if target is affected by Druids HOTs
    SpellClassOptionsEntry const* classOptions = spellProto->GetSpellClassOptions();
    if (classOptions && classOptions->SpellFamilyName == SPELLFAMILY_DRUID && (classOptions->SpellFamilyFlags & UI64LIT(0x0200000000000000)))
    {
        int ownHotCount = 0;                        // counted HoT types amount, not stacks
        Unit::AuraList const& RejorRegr = pVictim->GetAurasByType(SPELL_AURA_PERIODIC_HEAL);
        for(Unit::AuraList::const_iterator i = RejorRegr.begin(); i != RejorRegr.end(); ++i)
            if ((*i)->GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_DRUID &&
                (*i)->GetCasterGuid() == GetObjectGuid())
                ++ownHotCount;

        if (ownHotCount)
        {
            DoneTotalMod *= 1.2f;                       // base bonus at HoTs

            if (Aura* glyph = GetAura(62971, EFFECT_INDEX_0))// Glyph of Nourish
            {
                DoneTotalMod *= (glyph->GetModifier()->m_amount * ownHotCount + 100.0f) / 100.0f;
            }
        }
        // Lifebloom
        else if (spellProto->IsFitToFamilyMask(UI64LIT(0x0000001000000000)))
        {
            AuraList const& dummyList = owner->GetAurasByType(SPELL_AURA_DUMMY);
            for (AuraList::const_iterator i = dummyList.begin(); i != dummyList.end(); ++i)
            {
                switch ((*i)->GetId())
                {
                    case 34246:                             // Idol of the Emerald Queen        TODO: can be flat modifier aura
                    case 60779:                             // Idol of Lush Moss
                        DoneTotal += (*i)->GetModifier()->m_amount / 7;
                        break;
                }
            }
        }
    }

    // Done fixed damage bonus auras
    int32 DoneAdvertisedBenefit  = SpellBaseHealingBonusDone(GetSpellSchoolMask(spellProto));

    // apply ap bonus and benefit affected by spell power implicit coeffs and spell level penalties
    DoneTotal = SpellBonusWithCoeffs(spellProto, DoneTotal, DoneAdvertisedBenefit, 0, damagetype, true, SCALE_SPELLPOWER_HEALING);

    // use float as more appropriate for negative values and percent applying
    float heal = (healamount + DoneTotal * int32(stack)) * DoneTotalMod;
    // apply spellmod to Done amount
    if (Player* modOwner = GetSpellModOwner())
    {
        modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, heal);
    }

    return heal < 0 ? 0 : uint32(heal);
}

/**
 * Calculates target part of healing spell bonuses,
 * will be called on each tick for periodic damage over time auras
 */
uint32 Unit::SpellHealingBonusTaken(Unit* pCaster, SpellEntry const* spellProto, int32 healamount, DamageEffectType damagetype, uint32 stack)
{
    float  TakenTotalMod = 1.0f;

    // Healing taken percent
    float minval = float(GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
    if (minval)
    {
        TakenTotalMod *= (100.0f + minval) / 100.0f;
    }

    float maxval = float(GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HEALING_PCT));
    // no SPELL_AURA_MOD_PERIODIC_HEAL positive cases
    if (maxval)
    {
        TakenTotalMod *= (100.0f + maxval) / 100.0f;
    }

    // No heal amount for this class spells
    if (spellProto->GetDmgClass() == SPELL_DAMAGE_CLASS_NONE)
    {
        healamount = int32(healamount * TakenTotalMod);
        return healamount < 0 ? 0 : healamount;
    }

    // Healing Done
    // Done total percent damage auras
    int32  TakenTotal = 0;

    // Taken fixed damage bonus auras
    int32 TakenAdvertisedBenefit = SpellBaseHealingBonusTaken(GetSpellSchoolMask(spellProto));

    // apply benefit affected by spell power implicit coeffs and spell level penalties
    TakenTotal = pCaster->SpellBonusWithCoeffs(spellProto, TakenTotal, TakenAdvertisedBenefit, 0, damagetype, false, SCALE_SPELLPOWER_HEALING);

    AuraList const& mHealingGet = GetAurasByType(SPELL_AURA_MOD_HEALING_RECEIVED);
    for (AuraList::const_iterator i = mHealingGet.begin(); i != mHealingGet.end(); ++i)
        if ((*i)->isAffectedOnSpell(spellProto))
        {
            TakenTotalMod *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
        }

    // use float as more appropriate for negative values and percent applying
    float heal = (healamount + TakenTotal * int32(stack)) * TakenTotalMod;

    return heal < 0 ? 0 : uint32(heal);
}

/**
 * @brief Computes the unit's advertised healing bonus for a school mask.
 *
 * @param schoolMask The spell school mask.
 * @return The advertised healing bonus.
 */
int32 Unit::SpellBaseHealingBonusDone(SpellSchoolMask schoolMask)
{
    int32 AdvertisedBenefit = 0;

    Unit::AuraList const& mOverrideSpellPowerAuras = GetAurasByType(SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT);
    if (!mOverrideSpellPowerAuras.empty())
    {
        for (Unit::AuraList::const_iterator itr = mOverrideSpellPowerAuras.begin(); itr != mOverrideSpellPowerAuras.end(); ++itr)
            if (schoolMask & (*itr)->GetModifier()->m_miscvalue)
            {
                AdvertisedBenefit += (*itr)->GetModifier()->m_amount;
            }

        return int32(GetTotalAttackPowerValue(BASE_ATTACK) * (100.0f + AdvertisedBenefit) / 100.0f);
    }

    AuraList const& mHealingDone = GetAurasByType(SPELL_AURA_MOD_HEALING_DONE);
    for (AuraList::const_iterator i = mHealingDone.begin(); i != mHealingDone.end(); ++i)
        if (!(*i)->GetModifier()->m_miscvalue || ((*i)->GetModifier()->m_miscvalue & schoolMask) != 0)
        {
            AdvertisedBenefit += (*i)->GetModifier()->m_amount;
        }

    // Healing bonus of spirit, intellect and strength
    if (GetTypeId() == TYPEID_PLAYER)
    {
        // Base value
        AdvertisedBenefit += ((Player*)this)->GetBaseSpellPowerBonus();

        if (GetPowerIndex(POWER_MANA) != INVALID_POWER_INDEX)
        {
            AdvertisedBenefit += std::max(0, int32(GetStat(STAT_INTELLECT)) - 10);  // spellpower from intellect
        }

        // Healing bonus from stats
        AuraList const& mHealingDoneOfStatPercent = GetAurasByType(SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT);
        for (AuraList::const_iterator i = mHealingDoneOfStatPercent.begin(); i != mHealingDoneOfStatPercent.end(); ++i)
        {
            // stat used dependent from misc value (stat index)
            Stats usedStat = Stats((*i)->GetSpellProto()->GetEffectMiscValue((*i)->GetEffIndex()));
            AdvertisedBenefit += int32(GetStat(usedStat) * (*i)->GetModifier()->m_amount / 100.0f);
        }

        // ... and attack power
        AuraList const& mHealingDonebyAP = GetAurasByType(SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER);
        for (AuraList::const_iterator i = mHealingDonebyAP.begin(); i != mHealingDonebyAP.end(); ++i)
            if ((*i)->GetModifier()->m_miscvalue & schoolMask)
            {
                AdvertisedBenefit += int32(GetTotalAttackPowerValue(BASE_ATTACK) * (*i)->GetModifier()->m_amount / 100.0f);
            }
    }

    // pct spell power modifier
    Unit::AuraList const& mSpellPowerPctAuras = GetAurasByType(SPELL_AURA_MOD_INCREASE_SPELL_POWER_PCT);
    for (Unit::AuraList::const_iterator itr = mSpellPowerPctAuras.begin(); itr != mSpellPowerPctAuras.end(); ++itr)
    {
        if (!(*itr)->GetModifier()->m_miscvalue || (*itr)->GetModifier()->m_miscvalue & schoolMask)
        {
            AdvertisedBenefit = int32(AdvertisedBenefit * (100.0f + (*itr)->GetModifier()->m_amount) / 100.0f);
        }
    }

    return AdvertisedBenefit;
}

/**
 * @brief Computes the target's advertised healing taken bonus for a school mask.
 *
 * @param schoolMask The spell school mask.
 * @return The advertised healing taken bonus.
 */
int32 Unit::SpellBaseHealingBonusTaken(SpellSchoolMask schoolMask)
{
    int32 AdvertisedBenefit = 0;
    AuraList const& mDamageTaken = GetAurasByType(SPELL_AURA_MOD_HEALING);
    for (AuraList::const_iterator i = mDamageTaken.begin(); i != mDamageTaken.end(); ++i)
        if ((*i)->GetModifier()->m_miscvalue & schoolMask)
        {
            AdvertisedBenefit += (*i)->GetModifier()->m_amount;
        }

    return AdvertisedBenefit;
}

/**
 * @brief Checks whether the unit is immune to a damage school mask.
 *
 * @param shoolMask The damage school mask.
 * @return True if the unit is immune; otherwise, false.
 */
bool Unit::IsImmunedToDamage(SpellSchoolMask shoolMask)
{
    // If m_immuneToSchool type contain this school type, IMMUNE damage.
    SpellImmuneList const& schoolList = m_spellImmune[IMMUNITY_SCHOOL];
    for (SpellImmuneList::const_iterator itr = schoolList.begin(); itr != schoolList.end(); ++itr)
        if (itr->type & shoolMask)
        {
            return true;
        }

    // If m_immuneToDamage type contain magic, IMMUNE damage.
    SpellImmuneList const& damageList = m_spellImmune[IMMUNITY_DAMAGE];
    for (SpellImmuneList::const_iterator itr = damageList.begin(); itr != damageList.end(); ++itr)
        if (itr->type & shoolMask)
        {
            return true;
        }

    return false;
}

/**
 * @brief Checks whether the unit is immune to an entire spell.
 *
 * @param spellInfo The spell entry to test.
 * @param castOnSelf Unused self-cast flag placeholder.
 * @return True if the spell is immune; otherwise, false.
 */
bool Unit::IsImmuneToSpell(SpellEntry const* spellInfo, bool castOnSelf)
{
    if (!spellInfo)
    {
        return false;
    }

    // TODO add spellEffect immunity checks!, player with flag in bg is immune to immunity buffs from other friendly players!
    // SpellImmuneList const& dispelList = m_spellImmune[IMMUNITY_EFFECT];

    SpellImmuneList const& dispelList = m_spellImmune[IMMUNITY_DISPEL];
    for (SpellImmuneList::const_iterator itr = dispelList.begin(); itr != dispelList.end(); ++itr)
        if (itr->type == spellInfo->GetDispel())
        {
            return true;
        }

    if (!spellInfo->HasAttribute(SPELL_ATTR_EX_UNAFFECTED_BY_SCHOOL_IMMUNE) &&          // unaffected by school immunity
        !spellInfo->HasAttribute(SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY))           // can remove immune (by dispell or immune it)
    {
        SpellImmuneList const& schoolList = m_spellImmune[IMMUNITY_SCHOOL];
        for (SpellImmuneList::const_iterator itr = schoolList.begin(); itr != schoolList.end(); ++itr)
            if (!(IsPositiveSpell(itr->spellId) && IsPositiveSpell(spellInfo->Id)) &&
                (itr->type & GetSpellSchoolMask(spellInfo)))
            {
                return true;
            }
    }

    if (uint32 mechanic = spellInfo->GetMechanic())
    {
        SpellImmuneList const& mechanicList = m_spellImmune[IMMUNITY_MECHANIC];
        for (SpellImmuneList::const_iterator itr = mechanicList.begin(); itr != mechanicList.end(); ++itr)
            if (itr->type == mechanic)
            {
                return true;
            }

        AuraList const& immuneAuraApply = GetAurasByType(SPELL_AURA_MECHANIC_IMMUNITY_MASK);
        for (AuraList::const_iterator iter = immuneAuraApply.begin(); iter != immuneAuraApply.end(); ++iter)
            if ((*iter)->GetModifier()->m_miscvalue & (1 << (mechanic - 1)))
            {
                return true;
            }
    }

    return false;
}

/**
 * @brief Checks whether the unit is immune to a specific spell effect.
 *
 * @param spellInfo The spell entry to test.
 * @param index The effect index.
 * @param castOnSelf Unused self-cast flag placeholder.
 * @return True if the effect is immune; otherwise, false.
 */
bool Unit::IsImmuneToSpellEffect(SpellEntry const* spellInfo, SpellEffectIndex index, bool castOnSelf) const
{
    // If m_immuneToEffect type contain this effect type, IMMUNE effect.
    SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(index);
    if (!spellEffect)
    {
        return false;
    }

    uint32 effect = spellEffect->Effect;
    SpellImmuneList const& effectList = m_spellImmune[IMMUNITY_EFFECT];
    for (SpellImmuneList::const_iterator itr = effectList.begin(); itr != effectList.end(); ++itr)
        if (itr->type == effect)
        {
            return true;
        }

    if (uint32 mechanic = spellEffect->EffectMechanic)
    {
        SpellImmuneList const& mechanicList = m_spellImmune[IMMUNITY_MECHANIC];
        for (SpellImmuneList::const_iterator itr = mechanicList.begin(); itr != mechanicList.end(); ++itr)
            if (itr->type == mechanic)
            {
                return true;
            }

        AuraList const& immuneAuraApply = GetAurasByType(SPELL_AURA_MECHANIC_IMMUNITY_MASK);
        for (AuraList::const_iterator iter = immuneAuraApply.begin(); iter != immuneAuraApply.end(); ++iter)
            if ((*iter)->GetModifier()->m_miscvalue & (1 << (mechanic - 1)))
            {
                return true;
            }
    }

    if (uint32 aura = spellEffect->EffectApplyAuraName)
    {
        SpellImmuneList const& list = m_spellImmune[IMMUNITY_STATE];
        for (SpellImmuneList::const_iterator itr = list.begin(); itr != list.end(); ++itr)
            if (itr->type == aura)
            {
                return true;
            }

        // Check for immune to application of harmful magical effects
        AuraList const& immuneAuraApply = GetAurasByType(SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL);
        if (!immuneAuraApply.empty() &&
            spellInfo->GetDispel() == DISPEL_MAGIC &&       // Magic debuff)
            !IsPositiveEffect(spellInfo, index))            // Harmful
        {
            // Check school
            SpellSchoolMask schoolMask = GetSpellSchoolMask(spellInfo);
            for (AuraList::const_iterator iter = immuneAuraApply.begin(); iter != immuneAuraApply.end(); ++iter)
                if ((*iter)->GetModifier()->m_miscvalue & schoolMask)
                {
                    return true;
                }
        }
    }
    return false;
}

/**
 * Calculates caster part of melee damage bonuses,
 * also includes different bonuses dependent from target auras
 */
uint32 Unit::MeleeDamageBonusDone(Unit* pVictim, uint32 pdamage, WeaponAttackType attType, SpellEntry const* spellProto, DamageEffectType damagetype, uint32 stack)
{
    if (!pVictim || pdamage == 0 || (spellProto && spellProto->HasAttribute(SPELL_ATTR_EX6_NO_DMG_MODS)))
    {
        return pdamage;
    }

    // differentiate for weapon damage based spells
    bool isWeaponDamageBasedSpell = !(spellProto && (damagetype == DOT || IsSpellHaveEffect(spellProto, SPELL_EFFECT_SCHOOL_DAMAGE)));
    Item*  pWeapon          = GetTypeId() == TYPEID_PLAYER ? ((Player*)this)->GetWeaponForAttack(attType, true, false) : NULL;
    uint32 creatureTypeMask = pVictim->GetCreatureTypeMask();
    uint32 schoolMask       = spellProto ? spellProto->SchoolMask : uint32(GetMeleeDamageSchoolMask());

    // FLAT damage bonus auras
    // =======================
    int32 DoneFlat  = 0;
    int32 APbonus   = 0;

    // ..done flat, already included in weapon damage based spells
    if (!isWeaponDamageBasedSpell)
    {
        AuraList const& mModDamageDone = GetAurasByType(SPELL_AURA_MOD_DAMAGE_DONE);
        for (AuraList::const_iterator i = mModDamageDone.begin(); i != mModDamageDone.end(); ++i)
        {
            if ((*i)->GetModifier()->m_miscvalue & schoolMask &&                         // schoolmask has to fit with the intrinsic spell school
                (*i)->GetModifier()->m_miscvalue & GetMeleeDamageSchoolMask() &&         // AND schoolmask has to fit with weapon damage school (essential for non-physical spells)
                (((*i)->GetSpellProto()->GetEquippedItemClass() == -1) ||                 // general, weapon independent
                (pWeapon && pWeapon->IsFitToSpellRequirements((*i)->GetSpellProto()))))  // OR used weapon fits aura requirements
            {
                DoneFlat += (*i)->GetModifier()->m_amount;
            }
        }

        // Pets just add their bonus damage to their melee damage
        if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet())
        {
            DoneFlat += ((Pet*)this)->GetBonusDamage();
        }
    }

    // ..done flat (by creature type mask)
    DoneFlat += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_DAMAGE_DONE_CREATURE, creatureTypeMask);

    // ..done flat (base at attack power for marked target and base at attack power for creature type)
    if (attType == RANGED_ATTACK)
    {
        APbonus += pVictim->GetTotalAuraModifier(SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS);
        APbonus += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS, creatureTypeMask);
    }
    else
    {
        APbonus += pVictim->GetTotalAuraModifier(SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS);
        APbonus += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS, creatureTypeMask);
    }

    // PERCENT damage auras
    // ====================
    float DonePercent   = 1.0f;

    // ..done pct, already included in weapon damage based spells
    if (!isWeaponDamageBasedSpell)
    {
        AuraList const& mModDamagePercentDone = GetAurasByType(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);
        for (AuraList::const_iterator i = mModDamagePercentDone.begin(); i != mModDamagePercentDone.end(); ++i)
        {
            if ((*i)->GetModifier()->m_miscvalue & schoolMask &&                         // schoolmask has to fit with the intrinsic spell school
                (*i)->GetModifier()->m_miscvalue & GetMeleeDamageSchoolMask() &&         // AND schoolmask has to fit with weapon damage school (essential for non-physical spells)
                (((*i)->GetSpellProto()->GetEquippedItemClass()) == -1 ||                 // general, weapon independent
                (pWeapon && pWeapon->IsFitToSpellRequirements((*i)->GetSpellProto()))))  // OR used weapon fits aura requirements
            {
                DonePercent *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
            }
        }

        if (attType == OFF_ATTACK)
        {
            DonePercent *= GetModifierValue(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT);                     // no school check required
        }
    }

    if (!spellProto)
    {
        // apply SPELL_AURA_MOD_AUTOATTACK_DAMAGE for white damage
        AuraList const& mModAutoAttackDamageAuras = GetAurasByType(SPELL_AURA_MOD_AUTOATTACK_DAMAGE);
        for (AuraList::const_iterator i = mModAutoAttackDamageAuras.begin(); i != mModAutoAttackDamageAuras.end(); ++i)
        {
            if ((*i)->GetSpellProto()->GetEquippedItemClass() == -1 ||                  // general, weapon independent
                pWeapon && pWeapon->IsFitToSpellRequirements((*i)->GetSpellProto()))    // OR used weapon fits aura requirements
            {
                DonePercent *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
            }
        }
    }

    // ..done pct (by creature type mask)
    DonePercent *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_DONE_VERSUS, creatureTypeMask);

    // Mastery: Unshackled Fury (Fury Warrior 76856) — applies to all
    // damage dealt scaled by current rage%. Warrior abilities like
    // Bloodthirst / Raging Blow are SPELL_DAMAGE_CLASS_MELEE and route
    // through MeleeDamageBonusDone (not SpellDamageBonusDone). Read the
    // 76856 aura holder directly instead of by aura type because
    // mangosthree's DBC routes Unshackled Fury through a different aura
    // type than Mana Adept's 356 (SPELL_AURA_MOD_DAMAGE_DONE_FROM_PCT_POWER);
    // observed in-game with a temporary diagnostic on 2026-05-17.
    // m_amount > 0 guard avoids the DBC placeholder bleeding through
    // when SPELL_AURA_MASTERY isn't yet active on the caster.
    if (GetPowerType() == POWER_RAGE)
    {
        if (SpellAuraHolder* ufHolder = GetSpellAuraHolder(76856))
        {
            if (Aura* uf = ufHolder->GetAuraByEffectIndex(EFFECT_INDEX_0))
            {
                if (uf->GetModifier()->m_amount > 0)
                {
                    uint32 maxPower = GetMaxPower(POWER_RAGE);
                    float powerPct = maxPower ? std::min(float(GetPower(POWER_RAGE)) / maxPower, 1.0f) : 0.0f;
                    DonePercent *= (100.0f + uf->GetModifier()->m_amount * powerPct) / 100.0f;
                }
            }
        }
    }

    // special dummys/class scripts and other effects
    // =============================================
    Unit* owner = GetOwner();
    if (!owner)
    {
        owner = this;
    }

    // ..done (class scripts)
    if (spellProto)
    {
        AuraList const& mOverrideClassScript = owner->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
        for (AuraList::const_iterator i = mOverrideClassScript.begin(); i != mOverrideClassScript.end(); ++i)
        {
            if (!(*i)->isAffectedOnSpell(spellProto))
            {
                continue;
            }

            switch ((*i)->GetModifier()->m_miscvalue)
            {
                    // Tundra Stalker
                    // Merciless Combat
                case 7277:
                {
                    // Merciless Combat
                    if ((*i)->GetSpellProto()->SpellIconID == 2656)
                    {
                        if (pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                        {
                            DonePercent *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
                        }
                    }
                    else // Tundra Stalker
                    {
                        // Frost Fever (target debuff)
                        if (pVictim->GetAura(SPELL_AURA_MOD_MELEE_HASTE, SPELLFAMILY_DEATHKNIGHT, UI64LIT(0x0000000000000000), 0x00000002))
                        {
                            DonePercent *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                        }
                        break;
                    }
                    break;
                }
                case 7293: // Rage of Rivendare
                {
                    if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DEATHKNIGHT, UI64LIT(0x0200000000000000)))
                    {
                        DonePercent *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                    }
                    break;
                }
                // Marked for Death
                case 7598:
                case 7599:
                case 7600:
                case 7601:
                case 7602:
                {
                    if (pVictim->GetAura(SPELL_AURA_MOD_STALKED, SPELLFAMILY_HUNTER, UI64LIT(0x0000000000000400)))
                    {
                        DonePercent *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                    }
                    break;
                }
            }
        }
    }

    // .. done (class scripts)
    AuraList const& mclassScritAuras = GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
    for (AuraList::const_iterator i = mclassScritAuras.begin(); i != mclassScritAuras.end(); ++i)
    {
        switch ((*i)->GetMiscValue())
        {
                // Dirty Deeds
            case 6427:
            case 6428:
                if (pVictim->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                {
                    Aura* eff0 = GetAura((*i)->GetId(), EFFECT_INDEX_0);
                    if (!eff0 || (*i)->GetEffIndex() != EFFECT_INDEX_1)
                    {
                        sLog.outError("Spell structure of DD (%u) changed.", (*i)->GetId());
                        continue;
                    }

                    // effect 0 have expected value but in negative state
                    DonePercent *= (-eff0->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
        }
    }

    if (spellProto)
    {
        SpellClassOptionsEntry const* classOptions = spellProto->GetSpellClassOptions();

        // Frost Strike
        if (classOptions && classOptions->IsFitToFamily(SPELLFAMILY_DEATHKNIGHT, UI64LIT(0x0000000400000000)))
        {
            // search disease
            bool found = false;
            Unit::SpellAuraHolderMap const& auras = pVictim->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                if (itr->second->GetSpellProto()->GetDispel() == DISPEL_DISEASE)
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                // search for Glacier Rot dummy aura
                Unit::AuraList const& dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
                for (Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->GetEffectMiscValue((*i)->GetEffIndex()) == 7244)
                    {
                        DonePercent *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                        break;
                    }
                }
            }
        }
        // Glyph of Steady Shot (Steady Shot check)
        else if (classOptions && classOptions->IsFitToFamily(SPELLFAMILY_HUNTER, UI64LIT(0x0000000100000000)))
        {
            // search for glyph dummy aura
            if (Aura* aur = GetDummyAura(56826))
                // check for Serpent Sting at target
                if (pVictim->GetAura(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_HUNTER, UI64LIT(0x0000000000004000)))
                {
                    DonePercent *= (aur->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
        }
    }

    // final calculation
    // =================

    float DoneTotal = 0.0f;

    // scaling of non weapon based spells
    if (!isWeaponDamageBasedSpell)
    {
        // apply ap bonus and benefit affected by spell power implicit coeffs and spell level penalties
        DoneTotal = SpellBonusWithCoeffs(spellProto, DoneTotal, DoneFlat, APbonus, damagetype, true);
    }
    // weapon damage based spells
    else if (APbonus || DoneFlat)
    {
        bool normalized = spellProto ? IsSpellHaveEffect(spellProto, SPELL_EFFECT_NORMALIZED_WEAPON_DMG) : false;
        DoneTotal += int32(APbonus / 14.0f * GetAPMultiplier(attType, normalized));

        // for weapon damage based spells we still have to apply damage done percent mods
        // (that are already included into pdamage) to not-yet included DoneFlat
        // e.g. from doneVersusCreature, apBonusVs...
        UnitMods unitMod;
        switch (attType)
        {
            default:
            case BASE_ATTACK:   unitMod = UNIT_MOD_DAMAGE_MAINHAND; break;
            case OFF_ATTACK:    unitMod = UNIT_MOD_DAMAGE_OFFHAND;  break;
            case RANGED_ATTACK: unitMod = UNIT_MOD_DAMAGE_RANGED;   break;
        }

        DoneTotal += DoneFlat;

        DoneTotal *= GetModifierValue(unitMod, TOTAL_PCT);
    }

    float tmpDamage = float(int32(pdamage) + DoneTotal * int32(stack)) * DonePercent;

    // apply spellmod to Done damage
    if (spellProto)
    {
        if (Player* modOwner = GetSpellModOwner())
        {
            modOwner->ApplySpellMod(spellProto->Id, damagetype == DOT ? SPELLMOD_DOT : SPELLMOD_DAMAGE, tmpDamage);
        }
    }

    // bonus result can be negative
    return tmpDamage > 0 ? uint32(tmpDamage) : 0;
}

/**
 * Calculates target part of melee damage bonuses,
 * will be called on each tick for periodic damage over time auras
 */
uint32 Unit::MeleeDamageBonusTaken(Unit* pCaster, uint32 pdamage, WeaponAttackType attType, SpellEntry const* spellProto, DamageEffectType damagetype, uint32 stack)
{
    if (!pCaster)
    {
        return pdamage;
    }

    if (pdamage == 0)
    {
        return pdamage;
    }

    // differentiate for weapon damage based spells
    bool isWeaponDamageBasedSpell = !(spellProto && (damagetype == DOT || IsSpellHaveEffect(spellProto, SPELL_EFFECT_SCHOOL_DAMAGE)));
    uint32 schoolMask       = spellProto ? spellProto->SchoolMask : uint32(GetMeleeDamageSchoolMask());
    uint32 mechanicMask     = spellProto ? GetAllSpellMechanicMask(spellProto) : 0;

    // Shred also have bonus as MECHANIC_BLEED damages
    SpellClassOptionsEntry const* classOptions = spellProto ? spellProto->GetSpellClassOptions() : NULL;
    if (classOptions && classOptions->SpellFamilyName==SPELLFAMILY_DRUID && classOptions->SpellFamilyFlags & UI64LIT(0x00008000))
    {
        mechanicMask |= (1 << (MECHANIC_BLEED - 1));
    }

    // FLAT damage bonus auras
    // =======================
    int32 TakenFlat = 0;

    // ..taken flat (base at attack power for marked target and base at attack power for creature type)
    if (attType == RANGED_ATTACK)
    {
        TakenFlat += GetTotalAuraModifier(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN);
    }
    else
    {
        TakenFlat += GetTotalAuraModifier(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN);
    }

    // ..taken flat (by school mask)
    TakenFlat += GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_DAMAGE_TAKEN, schoolMask);

    // PERCENT damage auras
    // ====================
    float TakenPercent  = 1.0f;

    // ..taken pct (by school mask)
    TakenPercent *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, schoolMask);

    // ..taken pct (by mechanic mask)
    TakenPercent *= GetTotalAuraMultiplierByMiscValueForMask(SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT, mechanicMask);

    // ..taken pct (melee/ranged)
    if (attType == RANGED_ATTACK)
    {
        TakenPercent *= GetTotalAuraMultiplier(SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT);
    }
    else
    {
        TakenPercent *= GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT);
    }

    // ..taken pct (aoe avoidance)
    if (spellProto && IsAreaOfEffectSpell(spellProto))
    {
        TakenPercent *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE, schoolMask);
        if (GetTypeId() == TYPEID_UNIT && ((Creature*)this)->IsPet())
        {
            TakenPercent *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_PET_AOE_DAMAGE_AVOIDANCE, schoolMask);
        }
    }

    // special dummys/class scripts and other effects
    // =============================================

    // .. taken (dummy auras)
    AuraList const& mDummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    for (AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
    {
        switch ((*i)->GetId())
        {
            case 45182:                                     // Cheating Death
                if ((*i)->GetModifier()->m_miscvalue & SPELL_SCHOOL_MASK_NORMAL)
                {
                    if (GetTypeId() != TYPEID_PLAYER)
                    {
                        continue;
                    }

                    TakenPercent *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                }
                break;
            case 20911:                                     // Blessing of Sanctuary
            case 25899:                                     // Greater Blessing of Sanctuary
                TakenPercent *= ((*i)->GetModifier()->m_amount + 100.0f) / 100.0f;
                break;
        }
    }

    // final calculation
    // =================

    // scaling of non weapon based spells
    if (!isWeaponDamageBasedSpell)
    {
        // apply benefit affected by spell power implicit coeffs and spell level penalties
        TakenFlat = pCaster->SpellBonusWithCoeffs(spellProto, 0, TakenFlat, 0, damagetype, false);
    }

    float tmpDamage = float(int32(pdamage) + TakenFlat * int32(stack)) * TakenPercent;

    // bonus result can be negative
    return tmpDamage > 0 ? uint32(tmpDamage) : 0;
}
