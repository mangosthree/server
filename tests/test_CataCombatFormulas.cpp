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

// ============================================================================
// Cataclysm 4.0.1+ combat-formula regressions
//
// This file complements test_CombatFormulas.cpp, which covers pre-Cata
// (WotLK-era) combat math. The mechanics below were reworked in Patch 4.0.1
// (the Cataclysm pre-expansion patch) and must behave per the new rules in
// any 4.3.4-build-15595 server.
//
// Canonical reference: https://www.wowhead.com/guide=4.0.1
//
// Tests in this file verify STRUCTURAL Cata-correctness only (i.e. "when can
// outcome X happen"). Exact magnitudes (chance percentages, damage multipliers)
// are intentionally not asserted yet — they need to be looked up on Wowhead
// and cross-checked against TC-Preservation 4.3.4 before being pinned to a
// specific value, per the project rule on Wowhead ID verification.
// ============================================================================

namespace CataCombatTest
{

// ----------------------------------------------------------------------------
// Crushing blow (post-Patch 4.0.1)
//
// Rules retained from 3.0.2+: the attacker must be 4+ levels above the
// defender to score a crushing blow. (Raid bosses were standardized to 3
// levels above max-level players in 4.0.1, so bosses can no longer crush.)
//
// New in 4.0.1: properly-spec'd tanks in their tanking stance / presence /
// form gain effective uncrushable status alongside uncrittable, regardless
// of attacker level. This replaces the WotLK defense-cap mechanic.
// ----------------------------------------------------------------------------

// Returns crushing-blow chance as a percentage [0, 100].
// `defenderIsProperTank` should be true when the defender is in their
// tanking stance/presence/form (Defensive Stance, Bear Form, Blood Presence,
// Righteous Fury active, etc.).
//
// Note: the non-zero return for the "can crush" branch is intentionally a
// placeholder constant. The Cata-correct chance formula needs to be
// confirmed against Wowhead 4.0.1 and TC-Preservation before being pinned.
// Tests below only assert which branch is taken, not the magnitude.
float CalculateCrushingChanceCata(int32_t attackerLevel,
                                  int32_t defenderLevel,
                                  bool defenderIsProperTank)
{
    if (defenderIsProperTank)
    {
        return 0.0f;
    }

    const int32_t diff = attackerLevel - defenderLevel;
    if (diff < 4)
    {
        return 0.0f;
    }

    // Placeholder positive chance — actual Cata value to be verified.
    return 15.0f;
}

} // namespace CataCombatTest

// ============================================================================
// Level-difference rule (post-3.0.2, retained in 4.0.1)
// ============================================================================

TEST(CataCrushingBlow, EqualLevel_NoCrush)
{
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateCrushingChanceCata(85, 85, false));
}

TEST(CataCrushingBlow, AttackerOneAbove_NoCrush)
{
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateCrushingChanceCata(86, 85, false));
}

TEST(CataCrushingBlow, AttackerThreeAbove_BossDiff_NoCrush)
{
    // Raid bosses are 3 levels above max-level players in 4.0.1.
    // They must not be able to crush.
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateCrushingChanceCata(88, 85, false));
}

TEST(CataCrushingBlow, AttackerFourAbove_CanCrush)
{
    // Trash mobs 4+ levels above retain the ability to crush in Cata.
    EXPECT_GT(CataCombatTest::CalculateCrushingChanceCata(85, 81, false), 0.0f);
}

TEST(CataCrushingBlow, AttackerTenAbove_CanCrush)
{
    EXPECT_GT(CataCombatTest::CalculateCrushingChanceCata(85, 75, false), 0.0f);
}

// ============================================================================
// Tank-stance uncrushable (new in 4.0.1)
// ============================================================================

TEST(CataCrushingBlow, TankInStance_FourAbove_Uncrushable)
{
    // A proper tank in stance must be uncrushable even when the attacker
    // meets the level-diff threshold.
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateCrushingChanceCata(85, 81, true));
}

TEST(CataCrushingBlow, TankInStance_TenAbove_Uncrushable)
{
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateCrushingChanceCata(95, 85, true));
}

TEST(CataCrushingBlow, TankInStance_BossDiff_Uncrushable)
{
    // Both rules apply: 3-level boss diff already blocks crushing, and
    // tank-stance must also block. Belt and braces.
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateCrushingChanceCata(88, 85, true));
}

// ============================================================================
// Non-tank-player sanity (must still take crushes from 4+ above)
// ============================================================================

TEST(CataCrushingBlow, NonTankPlayer_HighDiff_CanCrush)
{
    // A DPS/healer (no tanking stance) at level 85 attacked by a level-95
    // elite must still be crushable.
    EXPECT_GT(CataCombatTest::CalculateCrushingChanceCata(95, 85, false), 0.0f);
}
