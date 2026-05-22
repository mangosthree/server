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
#include <map>
#include <cstring>

// ============================================================================
// Game logic helpers - standalone reimplementation of patterns used throughout
// the MaNGOS codebase (combat math, position, orientation, etc.)
// ============================================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_PI_F float(M_PI)

// ---- Position / Movement helpers ----
struct Position
{
    float x, y, z, o;
    Position(float x=0, float y=0, float z=0, float o=0) : x(x), y(y), z(z), o(o) {}

    float GetExactDist2d(const Position& p) const
    {
        float dx = x - p.x, dy = y - p.y;
        return sqrtf(dx*dx + dy*dy);
    }
    float GetExactDist(const Position& p) const
    {
        float dx = x - p.x, dy = y - p.y, dz = z - p.z;
        return sqrtf(dx*dx + dy*dy + dz*dz);
    }
    float GetAngle(const Position& p) const
    {
        float dx = p.x - x, dy = p.y - y;
        float ang = atan2f(dy, dx);
        if (ang < 0) ang += 2.0f * M_PI_F;
        return ang;
    }
    bool IsInDist2d(const Position& p, float dist) const
    {
        return GetExactDist2d(p) < dist;
    }
    bool IsInDist(const Position& p, float dist) const
    {
        return GetExactDist(p) < dist;
    }
};

// ---- Combat math helpers (patterns from Unit.cpp) ----
inline uint32_t CalculateDamage(uint32_t base, float pct)
{
    return uint32_t(float(base) * pct / 100.0f);
}

inline float CalculateMeleeCrit(float agility, float critCoeff)
{
    return agility * critCoeff;
}

inline uint32_t CalculateHealAmount(uint32_t base, float spellPower, float coefficient)
{
    return base + uint32_t(spellPower * coefficient);
}

inline int32_t CalculateAbsorbAmount(int32_t damage, int32_t absorb)
{
    if (absorb <= 0) return damage;
    int32_t remaining = damage - absorb;
    return remaining < 0 ? 0 : remaining;
}

inline float CalculateArmorReduction(float armor, float attackerLevel)
{
    // Simplified formula used in Cata: DR = Armor / (Armor + K)
    // K varies by level; approximate for level 85: K ~ 4037.5
    float K = 4037.5f;
    float dr = armor / (armor + K);
    if (dr > 0.75f) dr = 0.75f; // 75% cap
    if (dr < 0.0f)  dr = 0.0f;
    return dr;
}

inline uint32_t ApplyArmorReduction(uint32_t damage, float armorReduction)
{
    return uint32_t(float(damage) * (1.0f - armorReduction));
}

// ---- Orientation helpers ----
inline float NormalizeOrientation(float o)
{
    if (o < 0) { float mod = fmodf(-o, 2.0f * M_PI_F); return -mod + 2.0f * M_PI_F; }
    return fmodf(o, 2.0f * M_PI_F);
}

inline bool IsInFront(const Position& from, float ori, const Position& target, float arc)
{
    float angle = from.GetAngle(target);
    float diff = NormalizeOrientation(angle - ori);
    return diff <= arc / 2.0f || diff >= 2.0f * M_PI_F - arc / 2.0f;
}

inline bool IsInBack(const Position& from, float ori, const Position& target, float arc)
{
    float angle = from.GetAngle(target);
    float diff = NormalizeOrientation(angle - ori);
    float back_angle = NormalizeOrientation(ori + M_PI_F);
    float diff2 = NormalizeOrientation(angle - back_angle);
    return diff2 <= arc / 2.0f || diff2 >= 2.0f * M_PI_F - arc / 2.0f;
}

// ---- Percentage / stat helpers ----
inline float AddPct(float& val, float pct) { val *= (1.0f + pct / 100.0f); return val; }
inline int32_t AddPct(int32_t& val, int32_t pct) { val += val * pct / 100; return val; }
inline float ApplyPct(float val, float pct) { return val * pct / 100.0f; }
inline uint32_t RoundToInterval(uint32_t val, uint32_t lo, uint32_t hi) { return std::max(lo, std::min(hi, val)); }

// ---- Loot roll simulation (probability) ----
enum RollType { ROLL_NEED = 0, ROLL_GREED = 1, ROLL_DISENCHANT = 2, ROLL_PASS = 3 };
struct LootRoll
{
    uint32_t playerId;
    RollType type;
    uint8_t value; // 1-100
};

inline uint32_t DetermineWinner(const std::vector<LootRoll>& rolls, RollType highestType)
{
    uint32_t winner = 0;
    uint8_t bestRoll = 0;
    for (const auto& r : rolls)
    {
        if (r.type == highestType && r.value > bestRoll)
        {
            bestRoll = r.value;
            winner = r.playerId;
        }
    }
    return winner;
}

// ---- Quest XP helper (pattern from Player.cpp) ----
inline uint32_t CalculateQuestXP(uint32_t questLevel, uint32_t playerLevel, uint32_t questXP)
{
    if (playerLevel > questLevel + 5) return 0; // grey quest
    if (playerLevel <= questLevel - 5) return questXP; // green+ quest full XP
    // Within 5 levels: scale linearly
    int32_t diff = int32_t(playerLevel) - int32_t(questLevel);
    float scale = 1.0f - float(diff) / 5.0f;
    if (scale < 0.1f) scale = 0.1f;
    return uint32_t(questXP * scale);
}

// ---- Money handling ----
struct Money
{
    uint32_t gold;
    uint32_t silver;
    uint32_t copper;

    explicit Money(uint64_t totalCopper)
    {
        gold   = totalCopper / 10000;
        silver = (totalCopper % 10000) / 100;
        copper = totalCopper % 100;
    }

    uint64_t ToCopper() const { return uint64_t(gold) * 10000 + silver * 100 + copper; }
};

// ============================================================================
// Position Tests
// ============================================================================

TEST(Position, DefaultConstruction)
{
    Position p;
    EXPECT_FLOAT_EQ(0.0f, p.x);
    EXPECT_FLOAT_EQ(0.0f, p.y);
    EXPECT_FLOAT_EQ(0.0f, p.z);
    EXPECT_FLOAT_EQ(0.0f, p.o);
}

TEST(Position, ParameterizedConstruction)
{
    Position p(1.0f, 2.0f, 3.0f, 1.5f);
    EXPECT_FLOAT_EQ(1.0f, p.x);
    EXPECT_FLOAT_EQ(2.0f, p.y);
    EXPECT_FLOAT_EQ(3.0f, p.z);
    EXPECT_FLOAT_EQ(1.5f, p.o);
}

TEST(Position, Dist2dSamePoint)
{
    Position p(1.0f, 2.0f);
    EXPECT_FLOAT_EQ(0.0f, p.GetExactDist2d(p));
}

TEST(Position, Dist2dSimple)
{
    Position a(0.0f, 0.0f);
    Position b(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(5.0f, a.GetExactDist2d(b));
}

TEST(Position, Dist2dSymmetric)
{
    Position a(1.0f, 2.0f);
    Position b(4.0f, 6.0f);
    EXPECT_FLOAT_EQ(a.GetExactDist2d(b), b.GetExactDist2d(a));
}

TEST(Position, Dist3d)
{
    Position a(0.0f, 0.0f, 0.0f);
    Position b(1.0f, 2.0f, 2.0f);
    EXPECT_FLOAT_EQ(3.0f, a.GetExactDist(b));
}

TEST(Position, IsInDist2d)
{
    Position a(0.0f, 0.0f);
    Position b(3.0f, 4.0f); // dist = 5.0
    EXPECT_TRUE(a.IsInDist2d(b, 6.0f));
    EXPECT_FALSE(a.IsInDist2d(b, 4.0f));
}

TEST(Position, IsInDist3d)
{
    Position a(0.0f, 0.0f, 0.0f);
    Position b(1.0f, 2.0f, 2.0f); // dist = 3.0
    EXPECT_TRUE(a.IsInDist(b, 3.5f));
    EXPECT_FALSE(a.IsInDist(b, 2.5f));
}

TEST(Position, GetAngleEast)
{
    Position from(0.0f, 0.0f);
    Position to(1.0f, 0.0f);
    EXPECT_NEAR(0.0f, from.GetAngle(to), 0.0001f);
}

TEST(Position, GetAngleNorth)
{
    Position from(0.0f, 0.0f);
    Position to(0.0f, 1.0f);
    EXPECT_NEAR(M_PI_F / 2.0f, from.GetAngle(to), 0.0001f);
}

TEST(Position, GetAngleWest)
{
    Position from(0.0f, 0.0f);
    Position to(-1.0f, 0.0f);
    EXPECT_NEAR(M_PI_F, from.GetAngle(to), 0.0001f);
}

// ============================================================================
// Combat Math Tests
// ============================================================================

TEST(CombatMath, CalculateDamageBasic)
{
    EXPECT_EQ(uint32_t(100), CalculateDamage(100, 100.0f));
}

TEST(CombatMath, CalculateDamageHalf)
{
    EXPECT_EQ(uint32_t(50), CalculateDamage(100, 50.0f));
}

TEST(CombatMath, CalculateDamageZero)
{
    EXPECT_EQ(uint32_t(0), CalculateDamage(100, 0.0f));
}

TEST(CombatMath, CalculateDamageDouble)
{
    EXPECT_EQ(uint32_t(200), CalculateDamage(100, 200.0f));
}

TEST(CombatMath, CalculateHealBasic)
{
    // base 500 + 1000 spellpower * 0.5 coefficient = 1000
    EXPECT_EQ(uint32_t(1000), CalculateHealAmount(500, 1000.0f, 0.5f));
}

TEST(CombatMath, CalculateHealNoSpellpower)
{
    EXPECT_EQ(uint32_t(300), CalculateHealAmount(300, 0.0f, 0.5f));
}

TEST(CombatMath, AbsorbFull)
{
    // Absorb >= damage, result should be 0
    EXPECT_EQ(int32_t(0), CalculateAbsorbAmount(500, 1000));
}

TEST(CombatMath, AbsorbPartial)
{
    EXPECT_EQ(int32_t(300), CalculateAbsorbAmount(500, 200));
}

TEST(CombatMath, AbsorbNone)
{
    EXPECT_EQ(int32_t(500), CalculateAbsorbAmount(500, 0));
}

TEST(CombatMath, AbsorbNegative)
{
    EXPECT_EQ(int32_t(500), CalculateAbsorbAmount(500, -100));
}

// ============================================================================
// Armor Reduction Tests
// ============================================================================

TEST(ArmorReduction, ZeroArmor)
{
    EXPECT_FLOAT_EQ(0.0f, CalculateArmorReduction(0.0f, 85.0f));
}

TEST(ArmorReduction, ModerateArmor)
{
    float dr = CalculateArmorReduction(4037.5f, 85.0f);
    EXPECT_NEAR(0.5f, dr, 0.001f); // exactly 50% at K value
}

TEST(ArmorReduction, CapAt75Pct)
{
    float dr = CalculateArmorReduction(100000.0f, 85.0f);
    EXPECT_FLOAT_EQ(0.75f, dr);
}

TEST(ArmorReduction, ApplyToUInt32)
{
    // 1000 damage, 50% reduction = 500
    uint32_t dmg = ApplyArmorReduction(1000, 0.5f);
    EXPECT_EQ(uint32_t(500), dmg);
}

TEST(ArmorReduction, ZeroReductionPassthrough)
{
    EXPECT_EQ(uint32_t(1000), ApplyArmorReduction(1000, 0.0f));
}

TEST(ArmorReduction, FullReductionCapped)
{
    // 75% cap means 250 damage remains
    EXPECT_EQ(uint32_t(250), ApplyArmorReduction(1000, 0.75f));
}

// ============================================================================
// Orientation Tests
// ============================================================================

TEST(Orientation, NormalizeZero)
{
    EXPECT_FLOAT_EQ(0.0f, NormalizeOrientation(0.0f));
}

TEST(Orientation, NormalizeHalfCircle)
{
    EXPECT_NEAR(M_PI_F, NormalizeOrientation(M_PI_F), 0.0001f);
}

TEST(Orientation, NormalizeFullCircle)
{
    EXPECT_NEAR(0.0f, NormalizeOrientation(2.0f * M_PI_F), 0.0001f);
}

TEST(Orientation, NormalizeNegative)
{
    EXPECT_NEAR(M_PI_F, NormalizeOrientation(-M_PI_F), 0.0001f);
}

TEST(Orientation, NormalizeOver2Pi)
{
    EXPECT_NEAR(M_PI_F, NormalizeOrientation(3.0f * M_PI_F), 0.001f);
}

TEST(Orientation, InFront_DirectlyAhead)
{
    Position from(0.0f, 0.0f);
    Position target(1.0f, 0.0f);
    float ori = 0.0f; // facing east
    EXPECT_TRUE(IsInFront(from, ori, target, M_PI_F)); // 180 arc
}

TEST(Orientation, InFront_Behind_Not)
{
    Position from(0.0f, 0.0f);
    Position target(-1.0f, 0.0f); // behind
    float ori = 0.0f;
    EXPECT_FALSE(IsInFront(from, ori, target, M_PI_F / 2.0f)); // 90 arc
}

// ============================================================================
// Stat Modifier Tests
// ============================================================================

TEST(StatModifiers, AddPctFloat)
{
    float val = 100.0f;
    AddPct(val, 50.0f);
    EXPECT_FLOAT_EQ(150.0f, val);
}

TEST(StatModifiers, AddPctFloatNegative)
{
    float val = 100.0f;
    AddPct(val, -25.0f);
    EXPECT_FLOAT_EQ(75.0f, val);
}

TEST(StatModifiers, AddPctInt)
{
    int32_t val = 200;
    AddPct(val, 50);
    EXPECT_EQ(int32_t(300), val);
}

TEST(StatModifiers, ApplyPct)
{
    EXPECT_FLOAT_EQ(50.0f, ApplyPct(100.0f, 50.0f));
    EXPECT_FLOAT_EQ(25.0f, ApplyPct(100.0f, 25.0f));
}

TEST(StatModifiers, RoundToInterval_InRange)
{
    EXPECT_EQ(uint32_t(50), RoundToInterval(50, 0, 100));
}

TEST(StatModifiers, RoundToInterval_BelowMin)
{
    EXPECT_EQ(uint32_t(10), RoundToInterval(5, 10, 100));
}

TEST(StatModifiers, RoundToInterval_AboveMax)
{
    EXPECT_EQ(uint32_t(100), RoundToInterval(150, 10, 100));
}

// ============================================================================
// Quest XP Tests
// ============================================================================

TEST(QuestXP, SameLevelFullXP)
{
    EXPECT_EQ(uint32_t(1000), CalculateQuestXP(30, 30, 1000));
}

TEST(QuestXP, PlayerHigherByLessThan5)
{
    uint32_t xp = CalculateQuestXP(30, 33, 1000);
    EXPECT_GT(xp, uint32_t(0));
    EXPECT_LT(xp, uint32_t(1000));
}

TEST(QuestXP, PlayerHigherByMoreThan5_GreyQuest)
{
    EXPECT_EQ(uint32_t(0), CalculateQuestXP(30, 36, 1000));
}

TEST(QuestXP, PlayerLowerByMoreThan5_FullXP)
{
    EXPECT_EQ(uint32_t(1000), CalculateQuestXP(30, 24, 1000));
}

TEST(QuestXP, PlayerLowerBy5)
{
    // diff = -5, scale = 1 - (-5/5) = 2.0 clamped to... wait,
    // diff = playerLevel - questLevel = 25 - 30 = -5, scale = 1 - (-5)/5 = 2.0
    // but the condition is: playerLevel <= questLevel - 5 -> 25 <= 25 -> true -> full xp
    EXPECT_EQ(uint32_t(1000), CalculateQuestXP(30, 25, 1000));
}

TEST(QuestXP, ZeroBaseXP)
{
    EXPECT_EQ(uint32_t(0), CalculateQuestXP(30, 30, 0));
}

// ============================================================================
// Loot Roll Tests
// ============================================================================

TEST(LootRoll, SingleNeedWinner)
{
    std::vector<LootRoll> rolls = {{1, ROLL_NEED, 75}, {2, ROLL_GREED, 99}};
    uint32_t winner = DetermineWinner(rolls, ROLL_NEED);
    EXPECT_EQ(uint32_t(1), winner);
}

TEST(LootRoll, HighestNeedWins)
{
    std::vector<LootRoll> rolls = {
        {1, ROLL_NEED, 42},
        {2, ROLL_NEED, 87},
        {3, ROLL_NEED, 23}
    };
    EXPECT_EQ(uint32_t(2), DetermineWinner(rolls, ROLL_NEED));
}

TEST(LootRoll, GreedWinner)
{
    std::vector<LootRoll> rolls = {
        {1, ROLL_GREED, 55},
        {2, ROLL_PASS, 0},
        {3, ROLL_GREED, 82}
    };
    EXPECT_EQ(uint32_t(3), DetermineWinner(rolls, ROLL_GREED));
}

TEST(LootRoll, NoMatchingType)
{
    std::vector<LootRoll> rolls = {{1, ROLL_PASS, 0}, {2, ROLL_PASS, 0}};
    EXPECT_EQ(uint32_t(0), DetermineWinner(rolls, ROLL_NEED));
}

TEST(LootRoll, EmptyRolls)
{
    std::vector<LootRoll> rolls;
    EXPECT_EQ(uint32_t(0), DetermineWinner(rolls, ROLL_NEED));
}

// ============================================================================
// Money Tests
// ============================================================================

TEST(Money, Zero)
{
    Money m(0);
    EXPECT_EQ(uint32_t(0), m.gold);
    EXPECT_EQ(uint32_t(0), m.silver);
    EXPECT_EQ(uint32_t(0), m.copper);
}

TEST(Money, CopperOnly)
{
    Money m(50);
    EXPECT_EQ(uint32_t(0), m.gold);
    EXPECT_EQ(uint32_t(0), m.silver);
    EXPECT_EQ(uint32_t(50), m.copper);
}

TEST(Money, SilverOnly)
{
    Money m(100);
    EXPECT_EQ(uint32_t(0), m.gold);
    EXPECT_EQ(uint32_t(1), m.silver);
    EXPECT_EQ(uint32_t(0), m.copper);
}

TEST(Money, GoldOnly)
{
    Money m(10000);
    EXPECT_EQ(uint32_t(1), m.gold);
    EXPECT_EQ(uint32_t(0), m.silver);
    EXPECT_EQ(uint32_t(0), m.copper);
}

TEST(Money, Mixed)
{
    Money m(12345);
    EXPECT_EQ(uint32_t(1), m.gold);
    EXPECT_EQ(uint32_t(23), m.silver);
    EXPECT_EQ(uint32_t(45), m.copper);
}

TEST(Money, ToCopper_Roundtrip)
{
    uint64_t orig = 98765;
    Money m(orig);
    EXPECT_EQ(orig, m.ToCopper());
}

TEST(Money, LargeAmount)
{
    Money m(uint64_t(1000) * 10000); // 1000g
    EXPECT_EQ(uint32_t(1000), m.gold);
    EXPECT_EQ(uint32_t(0), m.silver);
    EXPECT_EQ(uint32_t(0), m.copper);
}

// ============================================================================
// Melee Crit Rate Tests
// ============================================================================

TEST(MeleeCrit, BasicCalculation)
{
    // e.g. 1000 agility, 0.01 crit per agility point = 10% crit
    EXPECT_FLOAT_EQ(10.0f, CalculateMeleeCrit(1000.0f, 0.01f));
}

TEST(MeleeCrit, ZeroAgility)
{
    EXPECT_FLOAT_EQ(0.0f, CalculateMeleeCrit(0.0f, 0.01f));
}

TEST(MeleeCrit, ZeroCoefficient)
{
    EXPECT_FLOAT_EQ(0.0f, CalculateMeleeCrit(1000.0f, 0.0f));
}
