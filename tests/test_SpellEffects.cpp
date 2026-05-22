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
 */

#include "TestFramework.h"

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>

// ============================================================================
// Standalone reimplementation of spell effect calculation patterns
// from src/game/WorldHandlers/Spell*.cpp and SpellAuras.cpp
// ============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_PI_F float(M_PI)

namespace SpellTest
{

enum SpellSchool
{
    SPELL_SCHOOL_NORMAL  = 0, // Physical
    SPELL_SCHOOL_HOLY    = 1,
    SPELL_SCHOOL_FIRE    = 2,
    SPELL_SCHOOL_NATURE  = 3,
    SPELL_SCHOOL_FROST   = 4,
    SPELL_SCHOOL_SHADOW  = 5,
    SPELL_SCHOOL_ARCANE  = 6,
    MAX_SPELL_SCHOOL     = 7
};

enum SpellSchoolMask
{
    SPELL_SCHOOL_MASK_NONE   = 0x00,
    SPELL_SCHOOL_MASK_NORMAL = (1 << SPELL_SCHOOL_NORMAL),
    SPELL_SCHOOL_MASK_HOLY   = (1 << SPELL_SCHOOL_HOLY),
    SPELL_SCHOOL_MASK_FIRE   = (1 << SPELL_SCHOOL_FIRE),
    SPELL_SCHOOL_MASK_NATURE = (1 << SPELL_SCHOOL_NATURE),
    SPELL_SCHOOL_MASK_FROST  = (1 << SPELL_SCHOOL_FROST),
    SPELL_SCHOOL_MASK_SHADOW = (1 << SPELL_SCHOOL_SHADOW),
    SPELL_SCHOOL_MASK_ARCANE = (1 << SPELL_SCHOOL_ARCANE),
    SPELL_SCHOOL_MASK_SPELL  = (SPELL_SCHOOL_MASK_FIRE | SPELL_SCHOOL_MASK_NATURE |
                                SPELL_SCHOOL_MASK_FROST | SPELL_SCHOOL_MASK_SHADOW |
                                SPELL_SCHOOL_MASK_ARCANE),
    SPELL_SCHOOL_MASK_MAGIC  = (SPELL_SCHOOL_MASK_HOLY | SPELL_SCHOOL_MASK_SPELL),
    SPELL_SCHOOL_MASK_ALL    = (SPELL_SCHOOL_MASK_NORMAL | SPELL_SCHOOL_MASK_MAGIC)
};

enum SpellDmgClass
{
    SPELL_DAMAGE_CLASS_NONE   = 0,
    SPELL_DAMAGE_CLASS_MAGIC  = 1,
    SPELL_DAMAGE_CLASS_MELEE  = 2,
    SPELL_DAMAGE_CLASS_RANGED = 3
};

enum SpellMechanic
{
    MECHANIC_NONE           = 0,
    MECHANIC_CHARM          = 1,
    MECHANIC_DISORIENTED    = 2,
    MECHANIC_DISARM         = 3,
    MECHANIC_DISTRACT       = 4,
    MECHANIC_FEAR           = 5,
    MECHANIC_GRIP           = 6,
    MECHANIC_ROOT           = 7,
    MECHANIC_SLOW_ATTACK    = 8,
    MECHANIC_SILENCE        = 9,
    MECHANIC_SLEEP          = 10,
    MECHANIC_SNARE          = 11,
    MECHANIC_STUN           = 12,
    MECHANIC_FREEZE         = 13,
    MECHANIC_KNOCKOUT       = 14,
    MECHANIC_BLEED          = 15,
    MECHANIC_BANDAGE        = 16,
    MECHANIC_POLYMORPH      = 17,
    MECHANIC_BANISH         = 18,
    MECHANIC_SHIELD         = 19,
    MECHANIC_SHACKLE        = 20,
    MECHANIC_MOUNT          = 21,
    MECHANIC_INFECTED       = 22,
    MECHANIC_TURN           = 23,
    MECHANIC_HORROR         = 24,
    MECHANIC_INVULNERABILITY = 25,
    MECHANIC_INTERRUPT       = 26,
    MECHANIC_DAZE            = 27,
    MECHANIC_DISCOVERY       = 28,
    MECHANIC_IMMUNE_SHIELD   = 29,
    MECHANIC_SAPPED          = 30
};

struct SpellEntry
{
    uint32_t Id;
    uint32_t School;
    uint32_t DmgClass;
    uint32_t Mechanic;
    int32_t  BasePoints[3];
    float    DmgMultiplier[3];
    uint32_t DurationIndex;
    float    RangeMin;
    float    RangeMax;
    float    Speed;
    uint32_t ManaCost;
    float    ManaCostPct;
    uint32_t CastTime;
    uint32_t Cooldown;
};

// Spell damage calculation (simplified from SpellEffects.cpp)
int32_t CalculateSpellDamage(const SpellEntry& spell, uint8_t effIndex,
                              float spellPower, float coefficient)
{
    int32_t base = spell.BasePoints[effIndex];
    float damage = float(base) + spellPower * coefficient;
    damage *= spell.DmgMultiplier[effIndex];
    return int32_t(damage);
}

// Spell resist calculation (binary resist for spells)
float CalculatePartialResist(float resistPct)
{
    if (resistPct < 0.0f) resistPct = 0.0f;
    if (resistPct > 75.0f) resistPct = 75.0f; // 75% cap
    return resistPct / 100.0f;
}

// DoT tick damage
int32_t CalculateDoTTickDamage(int32_t totalDamage, uint32_t totalTicks)
{
    if (totalTicks == 0) return 0;
    return totalDamage / int32_t(totalTicks);
}

// Spell duration with haste
uint32_t CalculateHastedDuration(uint32_t baseDuration, float hastePct)
{
    if (hastePct <= 0.0f) return baseDuration;
    return uint32_t(float(baseDuration) / (1.0f + hastePct / 100.0f));
}

// Spell power coefficient for HoTs/DoTs
float CalculateOverTimeCoefficient(float baseCastTime, uint32_t duration, bool isHoT)
{
    float coeff = baseCastTime / 3500.0f;
    float durationFactor = float(duration) / 15000.0f;
    if (isHoT)
        return coeff * durationFactor;
    return coeff;
}

// Mana cost with modifiers
uint32_t CalculateManaCost(const SpellEntry& spell, uint32_t baseMana, float costReduction)
{
    uint32_t cost = spell.ManaCost;
    if (spell.ManaCostPct > 0.0f)
        cost += uint32_t(baseMana * spell.ManaCostPct / 100.0f);
    float reduction = 1.0f - costReduction / 100.0f;
    if (reduction < 0.0f) reduction = 0.0f;
    return uint32_t(float(cost) * reduction);
}

// Spell range check
bool IsInSpellRange(float distance, float rangeMin, float rangeMax)
{
    return distance >= rangeMin && distance <= rangeMax;
}

// Critical strike damage multiplier
float CalculateCritDamage(float baseDamage, float critBonus)
{
    return baseDamage * (2.0f + critBonus / 100.0f);
}

// Diminishing returns duration
uint32_t CalculateDRDuration(uint32_t baseDuration, uint32_t drLevel)
{
    switch (drLevel)
    {
        case 0: return baseDuration;                       // full
        case 1: return baseDuration / 2;                   // 50%
        case 2: return baseDuration / 4;                   // 25%
        default: return 0;                                 // immune
    }
}

// Spell travel time
uint32_t CalculateSpellTravelTime(float distance, float speed)
{
    if (speed <= 0.0f) return 0;
    return uint32_t(distance / speed * 1000.0f);
}

} // namespace SpellTest

using namespace SpellTest;

// ============================================================================
// Spell School Tests
// ============================================================================

TEST(SpellSchool, EnumValues)
{
    EXPECT_EQ(0, SPELL_SCHOOL_NORMAL);
    EXPECT_EQ(1, SPELL_SCHOOL_HOLY);
    EXPECT_EQ(2, SPELL_SCHOOL_FIRE);
    EXPECT_EQ(6, SPELL_SCHOOL_ARCANE);
    EXPECT_EQ(7, MAX_SPELL_SCHOOL);
}

TEST(SpellSchool, MaskValues)
{
    EXPECT_EQ(0x01, SPELL_SCHOOL_MASK_NORMAL);
    EXPECT_EQ(0x02, SPELL_SCHOOL_MASK_HOLY);
    EXPECT_EQ(0x04, SPELL_SCHOOL_MASK_FIRE);
}

TEST(SpellSchool, MaskSpellCombination)
{
    int expected = SPELL_SCHOOL_MASK_FIRE | SPELL_SCHOOL_MASK_NATURE |
                   SPELL_SCHOOL_MASK_FROST | SPELL_SCHOOL_MASK_SHADOW |
                   SPELL_SCHOOL_MASK_ARCANE;
    EXPECT_EQ(expected, SPELL_SCHOOL_MASK_SPELL);
}

TEST(SpellSchool, MaskMagicIncludesHoly)
{
    EXPECT_TRUE((SPELL_SCHOOL_MASK_MAGIC & SPELL_SCHOOL_MASK_HOLY) != 0);
    EXPECT_TRUE((SPELL_SCHOOL_MASK_MAGIC & SPELL_SCHOOL_MASK_FIRE) != 0);
}

TEST(SpellSchool, MaskAllIncludesPhysical)
{
    EXPECT_TRUE((SPELL_SCHOOL_MASK_ALL & SPELL_SCHOOL_MASK_NORMAL) != 0);
    EXPECT_TRUE((SPELL_SCHOOL_MASK_ALL & SPELL_SCHOOL_MASK_MAGIC) != 0);
}

TEST(SpellSchool, MaskBitsAreDistinct)
{
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        for (int j = i + 1; j < MAX_SPELL_SCHOOL; ++j)
            EXPECT_EQ(0, (1 << i) & (1 << j));
}

// ============================================================================
// Spell Damage Calculation Tests
// ============================================================================

TEST(SpellDamage, BasicDamage)
{
    SpellEntry spell = {};
    spell.BasePoints[0] = 100;
    spell.DmgMultiplier[0] = 1.0f;
    EXPECT_EQ(int32_t(100), CalculateSpellDamage(spell, 0, 0.0f, 0.0f));
}

TEST(SpellDamage, WithSpellPower)
{
    SpellEntry spell = {};
    spell.BasePoints[0] = 100;
    spell.DmgMultiplier[0] = 1.0f;
    // 100 base + 500 * 0.5 coefficient = 350
    EXPECT_EQ(int32_t(350), CalculateSpellDamage(spell, 0, 500.0f, 0.5f));
}

TEST(SpellDamage, WithMultiplier)
{
    SpellEntry spell = {};
    spell.BasePoints[0] = 200;
    spell.DmgMultiplier[0] = 1.5f;
    // 200 * 1.5 = 300
    EXPECT_EQ(int32_t(300), CalculateSpellDamage(spell, 0, 0.0f, 0.0f));
}

TEST(SpellDamage, WithSpellPowerAndMultiplier)
{
    SpellEntry spell = {};
    spell.BasePoints[0] = 100;
    spell.DmgMultiplier[0] = 2.0f;
    // (100 + 1000 * 0.8) * 2.0 = 900 * 2.0 = 1800
    EXPECT_EQ(int32_t(1800), CalculateSpellDamage(spell, 0, 1000.0f, 0.8f));
}

TEST(SpellDamage, ZeroBase)
{
    SpellEntry spell = {};
    spell.BasePoints[0] = 0;
    spell.DmgMultiplier[0] = 1.0f;
    EXPECT_EQ(int32_t(500), CalculateSpellDamage(spell, 0, 1000.0f, 0.5f));
}

TEST(SpellDamage, NegativeBase_Healing)
{
    SpellEntry spell = {};
    spell.BasePoints[0] = -50;
    spell.DmgMultiplier[0] = 1.0f;
    // -50 + 200*0.5 = 50
    EXPECT_EQ(int32_t(50), CalculateSpellDamage(spell, 0, 200.0f, 0.5f));
}

TEST(SpellDamage, DifferentEffectIndices)
{
    SpellEntry spell = {};
    spell.BasePoints[0] = 100;
    spell.BasePoints[1] = 200;
    spell.BasePoints[2] = 300;
    spell.DmgMultiplier[0] = 1.0f;
    spell.DmgMultiplier[1] = 1.0f;
    spell.DmgMultiplier[2] = 1.0f;
    EXPECT_EQ(int32_t(100), CalculateSpellDamage(spell, 0, 0.0f, 0.0f));
    EXPECT_EQ(int32_t(200), CalculateSpellDamage(spell, 1, 0.0f, 0.0f));
    EXPECT_EQ(int32_t(300), CalculateSpellDamage(spell, 2, 0.0f, 0.0f));
}

// ============================================================================
// Partial Resist Tests
// ============================================================================

TEST(SpellResist, ZeroResist)
{
    EXPECT_FLOAT_EQ(0.0f, CalculatePartialResist(0.0f));
}

TEST(SpellResist, FullResist)
{
    EXPECT_FLOAT_EQ(0.75f, CalculatePartialResist(100.0f)); // capped at 75%
}

TEST(SpellResist, FiftyPercent)
{
    EXPECT_FLOAT_EQ(0.50f, CalculatePartialResist(50.0f));
}

TEST(SpellResist, NegativeClamped)
{
    EXPECT_FLOAT_EQ(0.0f, CalculatePartialResist(-20.0f));
}

TEST(SpellResist, AtCap)
{
    EXPECT_FLOAT_EQ(0.75f, CalculatePartialResist(75.0f));
}

// ============================================================================
// DoT Tick Damage Tests
// ============================================================================

TEST(SpellDoT, BasicTickDamage)
{
    // 3000 total over 10 ticks = 300 per tick
    EXPECT_EQ(int32_t(300), CalculateDoTTickDamage(3000, 10));
}

TEST(SpellDoT, ZeroTicks)
{
    EXPECT_EQ(int32_t(0), CalculateDoTTickDamage(1000, 0));
}

TEST(SpellDoT, SingleTick)
{
    EXPECT_EQ(int32_t(500), CalculateDoTTickDamage(500, 1));
}

TEST(SpellDoT, UnevenDivision)
{
    // 1000 / 3 = 333 (integer division)
    EXPECT_EQ(int32_t(333), CalculateDoTTickDamage(1000, 3));
}

TEST(SpellDoT, CorruptionDamage)
{
    // Corruption: 1080 over 6 ticks = 180 per tick
    EXPECT_EQ(int32_t(180), CalculateDoTTickDamage(1080, 6));
}

// ============================================================================
// Hasted Duration Tests
// ============================================================================

TEST(SpellHaste, NoHaste)
{
    EXPECT_EQ(uint32_t(30000), CalculateHastedDuration(30000, 0.0f));
}

TEST(SpellHaste, FiftyPercentHaste)
{
    // 30000 / 1.5 = 20000
    EXPECT_EQ(uint32_t(20000), CalculateHastedDuration(30000, 50.0f));
}

TEST(SpellHaste, HundredPercentHaste)
{
    // 30000 / 2.0 = 15000
    EXPECT_EQ(uint32_t(15000), CalculateHastedDuration(30000, 100.0f));
}

TEST(SpellHaste, NegativeHasteNoEffect)
{
    EXPECT_EQ(uint32_t(30000), CalculateHastedDuration(30000, -10.0f));
}

// ============================================================================
// Mana Cost Tests
// ============================================================================

TEST(SpellManaCost, FlatCost)
{
    SpellEntry spell = {};
    spell.ManaCost = 500;
    spell.ManaCostPct = 0.0f;
    EXPECT_EQ(uint32_t(500), CalculateManaCost(spell, 10000, 0.0f));
}

TEST(SpellManaCost, PercentCost)
{
    SpellEntry spell = {};
    spell.ManaCost = 0;
    spell.ManaCostPct = 10.0f; // 10% of base mana
    EXPECT_EQ(uint32_t(1000), CalculateManaCost(spell, 10000, 0.0f));
}

TEST(SpellManaCost, CostReduction)
{
    SpellEntry spell = {};
    spell.ManaCost = 1000;
    spell.ManaCostPct = 0.0f;
    // 50% cost reduction: 1000 * 0.5 = 500
    EXPECT_EQ(uint32_t(500), CalculateManaCost(spell, 10000, 50.0f));
}

TEST(SpellManaCost, FullReduction)
{
    SpellEntry spell = {};
    spell.ManaCost = 1000;
    spell.ManaCostPct = 0.0f;
    EXPECT_EQ(uint32_t(0), CalculateManaCost(spell, 10000, 100.0f));
}

TEST(SpellManaCost, OverReductionClampedToZero)
{
    SpellEntry spell = {};
    spell.ManaCost = 1000;
    spell.ManaCostPct = 0.0f;
    EXPECT_EQ(uint32_t(0), CalculateManaCost(spell, 10000, 150.0f));
}

// ============================================================================
// Spell Range Tests
// ============================================================================

TEST(SpellRange, InRange)
{
    EXPECT_TRUE(IsInSpellRange(20.0f, 0.0f, 40.0f));
}

TEST(SpellRange, AtMinRange)
{
    EXPECT_TRUE(IsInSpellRange(8.0f, 8.0f, 40.0f));
}

TEST(SpellRange, AtMaxRange)
{
    EXPECT_TRUE(IsInSpellRange(40.0f, 0.0f, 40.0f));
}

TEST(SpellRange, BelowMin)
{
    EXPECT_FALSE(IsInSpellRange(5.0f, 8.0f, 40.0f));
}

TEST(SpellRange, AboveMax)
{
    EXPECT_FALSE(IsInSpellRange(45.0f, 0.0f, 40.0f));
}

TEST(SpellRange, MeleeRange)
{
    EXPECT_TRUE(IsInSpellRange(3.0f, 0.0f, 5.0f));
}

// ============================================================================
// Critical Damage Tests
// ============================================================================

TEST(SpellCrit, BasicCrit)
{
    // 1000 * 2.0 = 2000
    EXPECT_FLOAT_EQ(2000.0f, CalculateCritDamage(1000.0f, 0.0f));
}

TEST(SpellCrit, WithCritBonus)
{
    // 1000 * (2.0 + 0.5) = 2500 (50% crit bonus = chaotic skyflare)
    EXPECT_FLOAT_EQ(2500.0f, CalculateCritDamage(1000.0f, 50.0f));
}

TEST(SpellCrit, ZeroDamage)
{
    EXPECT_FLOAT_EQ(0.0f, CalculateCritDamage(0.0f, 0.0f));
}

// ============================================================================
// Diminishing Returns Duration Tests
// ============================================================================

TEST(SpellDR, FirstApplication)
{
    EXPECT_EQ(uint32_t(6000), CalculateDRDuration(6000, 0));
}

TEST(SpellDR, SecondApplication)
{
    EXPECT_EQ(uint32_t(3000), CalculateDRDuration(6000, 1));
}

TEST(SpellDR, ThirdApplication)
{
    EXPECT_EQ(uint32_t(1500), CalculateDRDuration(6000, 2));
}

TEST(SpellDR, Immune)
{
    EXPECT_EQ(uint32_t(0), CalculateDRDuration(6000, 3));
    EXPECT_EQ(uint32_t(0), CalculateDRDuration(6000, 10));
}

// ============================================================================
// Spell Travel Time Tests
// ============================================================================

TEST(SpellTravel, BasicTravel)
{
    // 40 yards at 20 yards/sec = 2000ms
    EXPECT_EQ(uint32_t(2000), CalculateSpellTravelTime(40.0f, 20.0f));
}

TEST(SpellTravel, InstantSpeed)
{
    EXPECT_EQ(uint32_t(0), CalculateSpellTravelTime(40.0f, 0.0f));
}

TEST(SpellTravel, ZeroDistance)
{
    EXPECT_EQ(uint32_t(0), CalculateSpellTravelTime(0.0f, 20.0f));
}

TEST(SpellTravel, LongRange)
{
    // 100 yards at 25 yards/sec = 4000ms
    EXPECT_EQ(uint32_t(4000), CalculateSpellTravelTime(100.0f, 25.0f));
}

// ============================================================================
// Spell Mechanic Tests
// ============================================================================

TEST(SpellMechanic, EnumOrdering)
{
    EXPECT_EQ(0, MECHANIC_NONE);
    EXPECT_LT(MECHANIC_STUN, MECHANIC_POLYMORPH);
    EXPECT_LT(MECHANIC_ROOT, MECHANIC_STUN);
}

TEST(SpellMechanic, AllDistinct)
{
    int mechanics[] = {
        MECHANIC_CHARM, MECHANIC_FEAR, MECHANIC_ROOT, MECHANIC_STUN,
        MECHANIC_POLYMORPH, MECHANIC_BANISH, MECHANIC_SILENCE, MECHANIC_SNARE
    };
    for (int i = 0; i < 8; ++i)
        for (int j = i + 1; j < 8; ++j)
            EXPECT_NE(mechanics[i], mechanics[j]);
}

// ============================================================================
// Spell Power Coefficient Tests
// ============================================================================

TEST(SpellCoeff, BasicCoefficient)
{
    // 3.5s cast time: coeff = 3500/3500 = 1.0
    EXPECT_FLOAT_EQ(1.0f, CalculateOverTimeCoefficient(3500.0f, 15000, false));
}

TEST(SpellCoeff, FastCast)
{
    // 1.5s cast time: coeff = 1500/3500 = 0.4286
    EXPECT_NEAR(0.4286f, CalculateOverTimeCoefficient(1500.0f, 15000, false), 0.001f);
}

TEST(SpellCoeff, HoTCoefficient)
{
    // 3.5s cast, 15s duration HoT: 1.0 * 1.0 = 1.0
    EXPECT_FLOAT_EQ(1.0f, CalculateOverTimeCoefficient(3500.0f, 15000, true));
}

TEST(SpellCoeff, ShortHoT)
{
    // 1.5s cast, 9s duration HoT: (1500/3500) * (9000/15000) = 0.4286 * 0.6 = 0.2571
    EXPECT_NEAR(0.2571f, CalculateOverTimeCoefficient(1500.0f, 9000, true), 0.001f);
}
