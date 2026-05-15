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
// Canonical references:
//   https://warcraft.wiki.gg/wiki/Crushing_blow  (2025-11-27)
//   https://wowpedia.fandom.com/wiki/Crushing_blow
//   https://www.wowhead.com/guide=4.0.1
// ============================================================================

namespace CataCombatTest
{

// ----------------------------------------------------------------------------
// Crushing blow (post-Patch 3.0.2, retained through Cata 4.3.4)
//
// Canonical Blizzard formula:
//   chance% = max(skill_diff, 20) * 2% - 15%
//   where  skill_diff = (attackerLevel - defenderLevel) * 5
//
// Rules:
// - Attacker must be 4+ levels above defender to crush
//   (skill_diff >= 20 in the formula's floor).
// - Raid bosses in 4.0+ are standardized to +3 levels above max-level
//   players; they fail the floor and cannot crush. This is the entire
//   reason tanks are "effectively uncrushable" in raids — it's emergent
//   from the level rule, not a separate stance/aura mechanism.
// ----------------------------------------------------------------------------

// Returns crushing-blow chance as a percentage [0, 100].
float CalculateCrushingChanceCata(int32_t attackerLevel, int32_t defenderLevel)
{
    const int32_t skillDiff = (attackerLevel - defenderLevel) * 5;
    if (skillDiff < 20)
    {
        return 0.0f;
    }
    return float(skillDiff) * 2.0f - 15.0f;
}

// ----------------------------------------------------------------------------
// Armor damage reduction (Cata 4.3.4)
//
// Canonical wiki formula:
//   DR% = armor / (armor + K), capped at 75%
//
// where K depends on the attacker's level L:
//   K(L) = 400 + 85*L              for L < 60
//   K(L) = 467.5*L - 22167.5       for L >= 60
//
// Equivalent expanded form used in production code:
//   denom         = 8.5 * levelModifier + 40
//   levelModifier = L                       for L < 60
//   levelModifier = L + 4.5*(L - 59)        for L >= 60
//   K             = 10 * denom
//
// Specific K values:
//   60: 5882.5     70: 10557.5    80: 15232.5
//   83 (raid boss vs L80): 16635  85 (max): 17570
//
// NOTE: TC-Preservation includes an additional `+20*(L-80)` term in the
// level modifier for L > 80 (giving K=26070 at L=85), citing the client's
// Paperdollframe.lua. That file is the tooltip-display calculation and may
// not reflect the server-side damage math. The Wowpedia / Warcraft Wiki /
// WoWWiki sources consistently document the no-kicker formula, and
// mangosthree's `Unit::CalcArmorReducedDamage` already implements it
// correctly. These tests lock that in as a regression guard.
// ----------------------------------------------------------------------------

float CalculateArmorDRCata(float armor, int32_t attackerLevel)
{
    if (armor < 0.0f)
    {
        return 0.0f;
    }

    float levelModifier = float(attackerLevel);
    if (attackerLevel > 59)
    {
        levelModifier += 4.5f * (levelModifier - 59.0f);
    }

    const float denom = 8.5f * levelModifier + 40.0f;
    const float temp = 0.1f * armor / denom;
    float dr = temp / (1.0f + temp);

    if (dr > 0.75f)
    {
        dr = 0.75f;
    }
    return dr * 100.0f;
}

} // namespace CataCombatTest

// ============================================================================
// Below the 4-level floor: no crushing
// ============================================================================

TEST(CataCrushingBlow, EqualLevel_NoCrush)
{
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateCrushingChanceCata(85, 85));
}

TEST(CataCrushingBlow, AttackerOneAbove_NoCrush)
{
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateCrushingChanceCata(86, 85));
}

TEST(CataCrushingBlow, AttackerThreeAbove_BossDiff_NoCrush)
{
    // Raid bosses are standardized to +3 levels above max-level players.
    // skill_diff = 15, below the 20 floor.
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateCrushingChanceCata(88, 85));
}

// ============================================================================
// At and above the 4-level floor: verified Blizzard percentages
// ============================================================================

TEST(CataCrushingBlow, AttackerFourAbove_25Percent)
{
    // skill_diff = 20, at the floor. 20*2% - 15% = 25%.
    EXPECT_FLOAT_EQ(25.0f, CataCombatTest::CalculateCrushingChanceCata(85, 81));
}

TEST(CataCrushingBlow, AttackerFiveAbove_35Percent)
{
    // skill_diff = 25. 25*2% - 15% = 35%.
    EXPECT_FLOAT_EQ(35.0f, CataCombatTest::CalculateCrushingChanceCata(85, 80));
}

TEST(CataCrushingBlow, AttackerSixAbove_45Percent)
{
    // skill_diff = 30. 30*2% - 15% = 45%.
    EXPECT_FLOAT_EQ(45.0f, CataCombatTest::CalculateCrushingChanceCata(85, 79));
}

TEST(CataCrushingBlow, AttackerTenAbove_85Percent)
{
    // skill_diff = 50. 50*2% - 15% = 85%.
    EXPECT_FLOAT_EQ(85.0f, CataCombatTest::CalculateCrushingChanceCata(85, 75));
}

// ============================================================================
// Armor DR: zero / negative / cap behaviour
// ============================================================================

TEST(CataArmorDR, ZeroArmor_NoReduction)
{
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateArmorDRCata(0.0f, 85));
}

TEST(CataArmorDR, NegativeArmor_ClampedToZero)
{
    EXPECT_FLOAT_EQ(0.0f, CataCombatTest::CalculateArmorDRCata(-100.0f, 85));
}

TEST(CataArmorDR, MassiveArmor_CappedAt75Percent)
{
    EXPECT_FLOAT_EQ(75.0f, CataCombatTest::CalculateArmorDRCata(1000000.0f, 85));
}

// ============================================================================
// 50% reduction is reached at armor == K(level)
// ============================================================================

TEST(CataArmorDR, Level60_AtK_50Percent)
{
    // K = 467.5*60 - 22167.5 = 5882.5
    EXPECT_FLOAT_EQ(50.0f, CataCombatTest::CalculateArmorDRCata(5882.5f, 60));
}

TEST(CataArmorDR, Level70_AtK_50Percent)
{
    // K = 467.5*70 - 22167.5 = 10557.5
    EXPECT_FLOAT_EQ(50.0f, CataCombatTest::CalculateArmorDRCata(10557.5f, 70));
}

TEST(CataArmorDR, Level80_AtK_50Percent)
{
    // K = 467.5*80 - 22167.5 = 15232.5
    EXPECT_FLOAT_EQ(50.0f, CataCombatTest::CalculateArmorDRCata(15232.5f, 80));
}

TEST(CataArmorDR, Level83_RaidBoss_AtK_50Percent)
{
    // K = 467.5*83 - 22167.5 = 16635.0. Raid bosses are +3 levels above
    // max-level players; this is the value seen by an L80 tank in WotLK
    // raids and an L82 tank vs Cata heroic-mode +1 trash.
    EXPECT_FLOAT_EQ(50.0f, CataCombatTest::CalculateArmorDRCata(16635.0f, 83));
}

TEST(CataArmorDR, Level85_MaxLevel_AtK_50Percent)
{
    // K = 467.5*85 - 22167.5 = 17570
    EXPECT_FLOAT_EQ(50.0f, CataCombatTest::CalculateArmorDRCata(17570.0f, 85));
}

// ============================================================================
// 75% cap is reached exactly at armor == 3 * K
// ============================================================================

TEST(CataArmorDR, Level85_ThreeXK_HitsCapExactly)
{
    // armor/K = 3 -> DR = 3/(3+1) = 0.75 = cap.
    EXPECT_FLOAT_EQ(75.0f, CataCombatTest::CalculateArmorDRCata(52710.0f, 85));
}
