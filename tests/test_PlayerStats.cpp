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

// ============================================================================
// Standalone reimplementation of player stat calculation patterns
// from src/game/Object/Player.cpp, Unit.cpp stat systems
// ============================================================================

namespace StatsTest
{

enum Stats
{
    STAT_STRENGTH  = 0,
    STAT_AGILITY   = 1,
    STAT_STAMINA   = 2,
    STAT_INTELLECT = 3,
    STAT_SPIRIT    = 4,
    MAX_STATS      = 5
};

enum PowerType
{
    POWER_MANA   = 0,
    POWER_RAGE   = 1,
    POWER_FOCUS  = 2,
    POWER_ENERGY = 3,
    POWER_RUNIC  = 5,
    MAX_POWERS   = 6
};

enum Classes
{
    CLASS_WARRIOR     = 1,
    CLASS_PALADIN     = 2,
    CLASS_HUNTER      = 3,
    CLASS_ROGUE       = 4,
    CLASS_PRIEST      = 5,
    CLASS_DEATH_KNIGHT = 6,
    CLASS_SHAMAN      = 7,
    CLASS_MAGE        = 8,
    CLASS_WARLOCK     = 9,
    CLASS_DRUID       = 11,
    MAX_CLASSES       = 12
};

// Health from stamina
uint32_t CalculateHealthFromStamina(uint32_t stamina, uint32_t level)
{
    // First 20 stamina = 1hp each, rest = 10hp each (simplified)
    uint32_t baseHP = (stamina <= 20) ? stamina : 20 + (stamina - 20) * 10;
    // Level-based modifier
    float levelMod = 1.0f + float(level) * 0.01f;
    return uint32_t(float(baseHP) * levelMod);
}

// Mana from intellect
uint32_t CalculateManaFromIntellect(uint32_t intellect, uint32_t level)
{
    uint32_t baseMana = (intellect <= 20) ? intellect : 20 + (intellect - 20) * 15;
    float levelMod = 1.0f + float(level) * 0.02f;
    return uint32_t(float(baseMana) * levelMod);
}

// Attack power from strength (melee classes)
uint32_t CalculateMeleeAttackPower(uint32_t strength, uint32_t agility, uint8_t cls)
{
    switch (cls)
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
            return strength * 2 - 20;
        case CLASS_ROGUE:
        case CLASS_HUNTER:
            return strength + agility - 20;
        case CLASS_DRUID:
            return strength * 2 - 20;
        default:
            return strength - 10;
    }
}

// Ranged attack power
uint32_t CalculateRangedAttackPower(uint32_t agility, uint8_t cls)
{
    switch (cls)
    {
        case CLASS_HUNTER:
            return agility * 2 - 20;
        case CLASS_ROGUE:
        case CLASS_WARRIOR:
            return agility - 10;
        default:
            return 0;
    }
}

// Spell power from intellect
float CalculateSpellPower(uint32_t intellect, float spellPowerFromGear)
{
    return spellPowerFromGear + float(intellect) * 0.0f; // base; gear-dependent in later expansions
}

// Mana regeneration per 5 seconds
float CalculateManaRegenPer5(uint32_t spirit, uint32_t intellect, uint8_t cls, uint32_t level)
{
    float baseSpiritRegen = 0.001f + float(spirit) * sqrtf(float(intellect)) * 0.009327f;
    float classCoeff = 1.0f;
    switch (cls)
    {
        case CLASS_MAGE:     classCoeff = 1.2f; break;
        case CLASS_PRIEST:   classCoeff = 1.1f; break;
        case CLASS_WARLOCK:  classCoeff = 1.0f; break;
        case CLASS_PALADIN:  classCoeff = 0.8f; break;
        case CLASS_SHAMAN:   classCoeff = 0.9f; break;
        default:             classCoeff = 0.5f; break;
    }
    return baseSpiritRegen * classCoeff * 5.0f;
}

// Rating conversions at level 80
float RatingToPercent(float rating, float ratingCoeff)
{
    return rating * ratingCoeff;
}

// Mastery from rating
float CalculateMastery(float masteryRating, float ratingCoeff)
{
    return 8.0f + masteryRating * ratingCoeff; // 8 base mastery points
}

// Diminishing returns for avoidance
float ApplyDiminishingReturns(float value, float cap, float coefficient)
{
    // DR formula: 1 / (1/cap + coefficient/value)
    if (value <= 0.0f) return 0.0f;
    return 1.0f / (1.0f / cap + coefficient / value);
}

// GCD (global cooldown) with haste
uint32_t CalculateGCD(float hastePct)
{
    float gcd = 1500.0f / (1.0f + hastePct / 100.0f);
    if (gcd < 1000.0f) gcd = 1000.0f; // 1 second minimum
    return uint32_t(gcd);
}

// Experience needed for level
uint32_t CalculateXPForLevel(uint32_t level)
{
    if (level < 1) return 0;
    // Simplified formula
    return level * level * 100;
}

// Level XP curve
float CalculateXPModifier(uint32_t playerLevel, uint32_t mobLevel)
{
    int32_t diff = int32_t(mobLevel) - int32_t(playerLevel);
    if (diff > 5) return 1.2f;  // can't be more than 20% bonus
    if (diff >= -5) return 1.0f + float(diff) * 0.05f;
    return 0.1f; // grey mob, minimum XP
}

// Rest XP multiplier
float CalculateRestBonus(bool isRested, float restXPLeft, float baseXP)
{
    if (!isRested || restXPLeft <= 0.0f) return 1.0f;
    float bonus = std::min(baseXP, restXPLeft);
    return 1.0f + bonus / baseXP;
}

} // namespace StatsTest

using namespace StatsTest;

// ============================================================================
// Stat Enum Tests
// ============================================================================

TEST(Stats, EnumValues)
{
    EXPECT_EQ(0, STAT_STRENGTH);
    EXPECT_EQ(1, STAT_AGILITY);
    EXPECT_EQ(2, STAT_STAMINA);
    EXPECT_EQ(3, STAT_INTELLECT);
    EXPECT_EQ(4, STAT_SPIRIT);
    EXPECT_EQ(5, MAX_STATS);
}

TEST(PowerType, EnumValues)
{
    EXPECT_EQ(0, POWER_MANA);
    EXPECT_EQ(1, POWER_RAGE);
    EXPECT_EQ(3, POWER_ENERGY);
}

TEST(Classes, EnumValues)
{
    EXPECT_EQ(1, CLASS_WARRIOR);
    EXPECT_EQ(4, CLASS_ROGUE);
    EXPECT_EQ(8, CLASS_MAGE);
    EXPECT_EQ(11, CLASS_DRUID);
}

// ============================================================================
// Health from Stamina Tests
// ============================================================================

TEST(HealthCalc, LowStamina)
{
    // 10 stamina, level 1: 10 * (1 + 0.01) = 10.1 -> 10
    uint32_t hp = CalculateHealthFromStamina(10, 1);
    EXPECT_EQ(uint32_t(10), hp);
}

TEST(HealthCalc, HighStamina)
{
    // 100 stamina, level 1: 20 + 80*10 = 820, * 1.01 = 828
    uint32_t hp = CalculateHealthFromStamina(100, 1);
    EXPECT_EQ(uint32_t(828), hp);
}

TEST(HealthCalc, HighLevel)
{
    // 100 stamina, level 80: 820 * 1.80 = 1476
    uint32_t hp = CalculateHealthFromStamina(100, 80);
    EXPECT_EQ(uint32_t(1476), hp);
}

TEST(HealthCalc, ZeroStamina)
{
    EXPECT_EQ(uint32_t(0), CalculateHealthFromStamina(0, 80));
}

TEST(HealthCalc, ExactlyTwenty)
{
    // 20 stamina, level 0: 20 * 1.0 = 20
    EXPECT_EQ(uint32_t(20), CalculateHealthFromStamina(20, 0));
}

// ============================================================================
// Mana from Intellect Tests
// ============================================================================

TEST(ManaCalc, LowIntellect)
{
    // 15 int, level 1: 15 * 1.02 = 15
    EXPECT_EQ(uint32_t(15), CalculateManaFromIntellect(15, 1));
}

TEST(ManaCalc, HighIntellect)
{
    // 100 int: 20 + 80*15 = 1220, level 1: 1220 * 1.02 = 1244
    EXPECT_EQ(uint32_t(1244), CalculateManaFromIntellect(100, 1));
}

TEST(ManaCalc, HighLevel)
{
    // 100 int, level 80: 1220 * 2.60 = 3172
    EXPECT_EQ(uint32_t(3172), CalculateManaFromIntellect(100, 80));
}

// ============================================================================
// Melee Attack Power Tests
// ============================================================================

TEST(MeleeAP, Warrior)
{
    // 200 str, 100 agi: 200*2 - 20 = 380
    EXPECT_EQ(uint32_t(380), CalculateMeleeAttackPower(200, 100, CLASS_WARRIOR));
}

TEST(MeleeAP, Rogue)
{
    // 100 str, 200 agi: 100 + 200 - 20 = 280
    EXPECT_EQ(uint32_t(280), CalculateMeleeAttackPower(100, 200, CLASS_ROGUE));
}

TEST(MeleeAP, Mage)
{
    // 50 str: 50 - 10 = 40
    EXPECT_EQ(uint32_t(40), CalculateMeleeAttackPower(50, 30, CLASS_MAGE));
}

// ============================================================================
// Ranged Attack Power Tests
// ============================================================================

TEST(RangedAP, Hunter)
{
    // 200 agi: 200*2 - 20 = 380
    EXPECT_EQ(uint32_t(380), CalculateRangedAttackPower(200, CLASS_HUNTER));
}

TEST(RangedAP, Warrior)
{
    // 100 agi: 100 - 10 = 90
    EXPECT_EQ(uint32_t(90), CalculateRangedAttackPower(100, CLASS_WARRIOR));
}

TEST(RangedAP, Mage)
{
    EXPECT_EQ(uint32_t(0), CalculateRangedAttackPower(100, CLASS_MAGE));
}

// ============================================================================
// Mana Regen Tests
// ============================================================================

TEST(ManaRegen, BasicRegen)
{
    float regen = CalculateManaRegenPer5(100, 200, CLASS_MAGE, 80);
    EXPECT_GT(regen, 0.0f);
}

TEST(ManaRegen, MageRegenHigherThanWarrior)
{
    float mageRegen = CalculateManaRegenPer5(100, 200, CLASS_MAGE, 80);
    float warriorRegen = CalculateManaRegenPer5(100, 200, CLASS_WARRIOR, 80);
    EXPECT_GT(mageRegen, warriorRegen);
}

TEST(ManaRegen, MoreSpiritMoreRegen)
{
    float lowSpirit = CalculateManaRegenPer5(50, 200, CLASS_PRIEST, 80);
    float highSpirit = CalculateManaRegenPer5(200, 200, CLASS_PRIEST, 80);
    EXPECT_GT(highSpirit, lowSpirit);
}

// ============================================================================
// Rating Conversion Tests
// ============================================================================

TEST(Rating, BasicConversion)
{
    // 100 rating * 0.03 coeff = 3.0%
    EXPECT_FLOAT_EQ(3.0f, RatingToPercent(100.0f, 0.03f));
}

TEST(Rating, ZeroRating)
{
    EXPECT_FLOAT_EQ(0.0f, RatingToPercent(0.0f, 0.03f));
}

// ============================================================================
// Mastery Tests
// ============================================================================

TEST(Mastery, BaseMastery)
{
    EXPECT_FLOAT_EQ(8.0f, CalculateMastery(0.0f, 0.01f));
}

TEST(Mastery, WithRating)
{
    // 8 base + 200 * 0.01 = 10.0
    EXPECT_FLOAT_EQ(10.0f, CalculateMastery(200.0f, 0.01f));
}

// ============================================================================
// Diminishing Returns Tests
// ============================================================================

TEST(DiminishingReturns, LowValue)
{
    float result = ApplyDiminishingReturns(10.0f, 100.0f, 1.0f);
    EXPECT_GT(result, 0.0f);
    EXPECT_LT(result, 10.0f); // DR reduces it
}

TEST(DiminishingReturns, HighValue)
{
    float result = ApplyDiminishingReturns(1000.0f, 100.0f, 1.0f);
    EXPECT_LT(result, 100.0f); // cap limits it
}

TEST(DiminishingReturns, ZeroValue)
{
    EXPECT_FLOAT_EQ(0.0f, ApplyDiminishingReturns(0.0f, 100.0f, 1.0f));
}

// ============================================================================
// GCD Tests
// ============================================================================

TEST(GCD, NoHaste)
{
    EXPECT_EQ(uint32_t(1500), CalculateGCD(0.0f));
}

TEST(GCD, FiftyPercentHaste)
{
    EXPECT_EQ(uint32_t(1000), CalculateGCD(50.0f));
}

TEST(GCD, HundredPercentHaste)
{
    // 1500/2.0 = 750, clamped to 1000
    EXPECT_EQ(uint32_t(1000), CalculateGCD(100.0f));
}

TEST(GCD, MinimumOneSecond)
{
    EXPECT_GE(CalculateGCD(500.0f), uint32_t(1000));
}

// ============================================================================
// XP Calculation Tests
// ============================================================================

TEST(XPCalc, Level1)
{
    EXPECT_EQ(uint32_t(100), CalculateXPForLevel(1));
}

TEST(XPCalc, Level10)
{
    EXPECT_EQ(uint32_t(10000), CalculateXPForLevel(10));
}

TEST(XPCalc, Level80)
{
    EXPECT_EQ(uint32_t(640000), CalculateXPForLevel(80));
}

TEST(XPCalc, LevelZero)
{
    EXPECT_EQ(uint32_t(0), CalculateXPForLevel(0));
}

// ============================================================================
// XP Modifier Tests
// ============================================================================

TEST(XPMod, SameLevel)
{
    EXPECT_FLOAT_EQ(1.0f, CalculateXPModifier(80, 80));
}

TEST(XPMod, HigherMob)
{
    float mod = CalculateXPModifier(80, 83);
    EXPECT_GT(mod, 1.0f);
    EXPECT_LE(mod, 1.2f);
}

TEST(XPMod, LowerMob)
{
    float mod = CalculateXPModifier(80, 77);
    EXPECT_LT(mod, 1.0f);
    EXPECT_GT(mod, 0.0f);
}

TEST(XPMod, GreyMob)
{
    EXPECT_FLOAT_EQ(0.1f, CalculateXPModifier(80, 70));
}

TEST(XPMod, VeryHighMob)
{
    EXPECT_FLOAT_EQ(1.2f, CalculateXPModifier(80, 90));
}

// ============================================================================
// Rest Bonus Tests
// ============================================================================

TEST(RestBonus, NotRested)
{
    EXPECT_FLOAT_EQ(1.0f, CalculateRestBonus(false, 0.0f, 100.0f));
}

TEST(RestBonus, FullRest)
{
    // Rested with enough rest XP: 1 + 100/100 = 2.0
    EXPECT_FLOAT_EQ(2.0f, CalculateRestBonus(true, 100.0f, 100.0f));
}

TEST(RestBonus, PartialRest)
{
    // Only 50 rest XP available for 100 base: 1 + 50/100 = 1.5
    EXPECT_FLOAT_EQ(1.5f, CalculateRestBonus(true, 50.0f, 100.0f));
}

TEST(RestBonus, ZeroRestXP)
{
    EXPECT_FLOAT_EQ(1.0f, CalculateRestBonus(true, 0.0f, 100.0f));
}

TEST(RestBonus, ExcessRestXP)
{
    // 200 rest XP but only 100 base: capped at 1 + 100/100 = 2.0
    EXPECT_FLOAT_EQ(2.0f, CalculateRestBonus(true, 200.0f, 100.0f));
}
