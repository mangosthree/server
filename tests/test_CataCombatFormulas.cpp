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
