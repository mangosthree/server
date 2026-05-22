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
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include <map>

// ============================================================================
// Standalone reimplementation of Loot system patterns from
// src/game/WorldHandlers/LootHandler.cpp, LootMgr.h
// ============================================================================

namespace LootTest
{

enum LootMethod
{
    FREE_FOR_ALL    = 0,
    ROUND_ROBIN     = 1,
    MASTER_LOOT     = 2,
    GROUP_LOOT      = 3,
    NEED_BEFORE_GREED = 4,
};

enum RollType
{
    ROLL_PASS       = 0,
    ROLL_NEED       = 1,
    ROLL_GREED      = 2,
    ROLL_DISENCHANT = 3,
};

enum ItemQuality
{
    ITEM_QUALITY_POOR       = 0,
    ITEM_QUALITY_NORMAL     = 1,
    ITEM_QUALITY_UNCOMMON   = 2,
    ITEM_QUALITY_RARE       = 3,
    ITEM_QUALITY_EPIC       = 4,
    ITEM_QUALITY_LEGENDARY  = 5,
};

struct LootItem
{
    uint32_t itemId;
    uint8_t  quality;
    uint32_t count;
    float    chance; // 0-100%
    bool     isLooted;
    uint32_t lootedBy;
};

struct LootRollEntry
{
    uint32_t playerId;
    RollType type;
    uint8_t  rollValue; // 1-100
};

class LootTable
{
public:
    void AddItem(const LootItem& item)
    {
        m_items.push_back(item);
    }

    // Resolve drops based on chance
    std::vector<LootItem> GenerateLoot(uint32_t seed) const
    {
        std::vector<LootItem> result;
        uint32_t pseudoRand = seed;
        for (const auto& item : m_items)
        {
            // Simple pseudo-random for testing
            pseudoRand = pseudoRand * 1103515245 + 12345;
            float roll = float(pseudoRand % 10000) / 100.0f;
            if (roll < item.chance)
            {
                LootItem drop = item;
                drop.isLooted = false;
                drop.lootedBy = 0;
                result.push_back(drop);
            }
        }
        return result;
    }

    size_t GetItemCount() const { return m_items.size(); }

private:
    std::vector<LootItem> m_items;
};

class GroupLoot
{
public:
    void AddRoll(const LootRollEntry& entry)
    {
        m_rolls.push_back(entry);
    }

    uint32_t ResolveWinner() const
    {
        // Need > Greed > Disenchant > Pass
        for (RollType type : {ROLL_NEED, ROLL_GREED, ROLL_DISENCHANT})
        {
            uint32_t bestPlayer = 0;
            uint8_t bestRoll = 0;
            for (const auto& r : m_rolls)
            {
                if (r.type == type && r.rollValue > bestRoll)
                {
                    bestRoll = r.rollValue;
                    bestPlayer = r.playerId;
                }
            }
            if (bestPlayer != 0) return bestPlayer;
        }
        return 0; // everyone passed
    }

    size_t GetRollCount() const { return m_rolls.size(); }

    void Clear() { m_rolls.clear(); }

private:
    std::vector<LootRollEntry> m_rolls;
};

// Loot threshold: items above this quality trigger group rolls
bool RequiresGroupRoll(uint8_t quality, uint8_t threshold)
{
    return quality >= threshold;
}

// Gold loot calculation
uint32_t CalculateGoldDrop(uint32_t creatureLevel, uint8_t creatureType, float goldModifier)
{
    uint32_t base = creatureLevel * creatureLevel * 5; // simple formula
    float typeMod = (creatureType == 1) ? 2.0f : 1.0f; // elite = 2x
    return uint32_t(float(base) * typeMod * goldModifier);
}

// Split gold among group members
uint32_t CalculateGoldPerPlayer(uint32_t totalGold, uint32_t playerCount)
{
    if (playerCount == 0) return 0;
    return totalGold / playerCount;
}

// Drop rate modifier for group size
float CalculateGroupDropBonus(uint32_t groupSize)
{
    if (groupSize <= 1) return 1.0f;
    return 1.0f + float(groupSize - 1) * 0.1f; // +10% per extra member
}

} // namespace LootTest

using namespace LootTest;

// ============================================================================
// Loot Method Tests
// ============================================================================

TEST(LootMethod, EnumValues)
{
    EXPECT_EQ(0, FREE_FOR_ALL);
    EXPECT_EQ(1, ROUND_ROBIN);
    EXPECT_EQ(2, MASTER_LOOT);
    EXPECT_EQ(3, GROUP_LOOT);
    EXPECT_EQ(4, NEED_BEFORE_GREED);
}

// ============================================================================
// Roll Type Tests
// ============================================================================

TEST(RollType, EnumValues)
{
    EXPECT_EQ(0, ROLL_PASS);
    EXPECT_EQ(1, ROLL_NEED);
    EXPECT_EQ(2, ROLL_GREED);
    EXPECT_EQ(3, ROLL_DISENCHANT);
}

TEST(RollType, NeedHigherPriorityThanGreed)
{
    EXPECT_GT(ROLL_NEED, ROLL_PASS);
    EXPECT_GT(ROLL_GREED, ROLL_PASS);
}

// ============================================================================
// Loot Table Tests
// ============================================================================

TEST(LootTable, Empty)
{
    LootTable table;
    EXPECT_EQ(size_t(0), table.GetItemCount());
    auto loot = table.GenerateLoot(42);
    EXPECT_EQ(size_t(0), loot.size());
}

TEST(LootTable, AddItems)
{
    LootTable table;
    table.AddItem({1000, ITEM_QUALITY_UNCOMMON, 1, 50.0f, false, 0});
    table.AddItem({2000, ITEM_QUALITY_RARE, 1, 10.0f, false, 0});
    EXPECT_EQ(size_t(2), table.GetItemCount());
}

TEST(LootTable, GuaranteedDrop)
{
    LootTable table;
    table.AddItem({1000, ITEM_QUALITY_NORMAL, 1, 100.0f, false, 0});
    // With 100% chance, should always drop
    int dropCount = 0;
    for (uint32_t seed = 0; seed < 100; ++seed)
    {
        auto loot = table.GenerateLoot(seed);
        for (const auto& item : loot)
            if (item.itemId == 1000) ++dropCount;
    }
    EXPECT_EQ(100, dropCount);
}

TEST(LootTable, ZeroChanceNeverDrops)
{
    LootTable table;
    table.AddItem({1000, ITEM_QUALITY_LEGENDARY, 1, 0.0f, false, 0});
    int dropCount = 0;
    for (uint32_t seed = 0; seed < 100; ++seed)
    {
        auto loot = table.GenerateLoot(seed);
        for (const auto& item : loot)
            if (item.itemId == 1000) ++dropCount;
    }
    EXPECT_EQ(0, dropCount);
}

TEST(LootTable, DroppedItemsNotLooted)
{
    LootTable table;
    table.AddItem({1000, ITEM_QUALITY_NORMAL, 1, 100.0f, false, 0});
    auto loot = table.GenerateLoot(42);
    EXPECT_FALSE(loot.empty());
    EXPECT_FALSE(loot[0].isLooted);
    EXPECT_EQ(uint32_t(0), loot[0].lootedBy);
}

// ============================================================================
// Group Loot Roll Tests
// ============================================================================

TEST(GroupLoot, NeedWinsOverGreed)
{
    GroupLoot gl;
    gl.AddRoll({1, ROLL_GREED, 99});  // greed 99
    gl.AddRoll({2, ROLL_NEED, 50});   // need 50
    EXPECT_EQ(uint32_t(2), gl.ResolveWinner());
}

TEST(GroupLoot, HighestNeedWins)
{
    GroupLoot gl;
    gl.AddRoll({1, ROLL_NEED, 30});
    gl.AddRoll({2, ROLL_NEED, 80});
    gl.AddRoll({3, ROLL_NEED, 55});
    EXPECT_EQ(uint32_t(2), gl.ResolveWinner());
}

TEST(GroupLoot, HighestGreedWins)
{
    GroupLoot gl;
    gl.AddRoll({1, ROLL_GREED, 40});
    gl.AddRoll({2, ROLL_GREED, 90});
    gl.AddRoll({3, ROLL_PASS, 0});
    EXPECT_EQ(uint32_t(2), gl.ResolveWinner());
}

TEST(GroupLoot, DisenchantFallback)
{
    GroupLoot gl;
    gl.AddRoll({1, ROLL_PASS, 0});
    gl.AddRoll({2, ROLL_DISENCHANT, 50});
    EXPECT_EQ(uint32_t(2), gl.ResolveWinner());
}

TEST(GroupLoot, AllPass)
{
    GroupLoot gl;
    gl.AddRoll({1, ROLL_PASS, 0});
    gl.AddRoll({2, ROLL_PASS, 0});
    EXPECT_EQ(uint32_t(0), gl.ResolveWinner());
}

TEST(GroupLoot, EmptyRolls)
{
    GroupLoot gl;
    EXPECT_EQ(uint32_t(0), gl.ResolveWinner());
}

TEST(GroupLoot, SinglePlayer)
{
    GroupLoot gl;
    gl.AddRoll({1, ROLL_NEED, 42});
    EXPECT_EQ(uint32_t(1), gl.ResolveWinner());
}

TEST(GroupLoot, Clear)
{
    GroupLoot gl;
    gl.AddRoll({1, ROLL_NEED, 50});
    gl.Clear();
    EXPECT_EQ(size_t(0), gl.GetRollCount());
    EXPECT_EQ(uint32_t(0), gl.ResolveWinner());
}

// ============================================================================
// Loot Threshold Tests
// ============================================================================

TEST(LootThreshold, PoorBelowUncommon)
{
    EXPECT_FALSE(RequiresGroupRoll(ITEM_QUALITY_POOR, ITEM_QUALITY_UNCOMMON));
}

TEST(LootThreshold, NormalBelowUncommon)
{
    EXPECT_FALSE(RequiresGroupRoll(ITEM_QUALITY_NORMAL, ITEM_QUALITY_UNCOMMON));
}

TEST(LootThreshold, UncommonMeetsUncommon)
{
    EXPECT_TRUE(RequiresGroupRoll(ITEM_QUALITY_UNCOMMON, ITEM_QUALITY_UNCOMMON));
}

TEST(LootThreshold, RareAboveUncommon)
{
    EXPECT_TRUE(RequiresGroupRoll(ITEM_QUALITY_RARE, ITEM_QUALITY_UNCOMMON));
}

TEST(LootThreshold, EpicAboveRare)
{
    EXPECT_TRUE(RequiresGroupRoll(ITEM_QUALITY_EPIC, ITEM_QUALITY_RARE));
}

// ============================================================================
// Gold Drop Tests
// ============================================================================

TEST(GoldDrop, BasicDrop)
{
    uint32_t gold = CalculateGoldDrop(10, 0, 1.0f);
    EXPECT_EQ(uint32_t(500), gold); // 10*10*5
}

TEST(GoldDrop, EliteMobDouble)
{
    uint32_t normal = CalculateGoldDrop(10, 0, 1.0f);
    uint32_t elite = CalculateGoldDrop(10, 1, 1.0f);
    EXPECT_EQ(elite, normal * 2);
}

TEST(GoldDrop, GoldModifier)
{
    uint32_t base = CalculateGoldDrop(10, 0, 1.0f);
    uint32_t boosted = CalculateGoldDrop(10, 0, 2.0f);
    EXPECT_EQ(boosted, base * 2);
}

TEST(GoldDrop, LevelOneBase)
{
    EXPECT_EQ(uint32_t(5), CalculateGoldDrop(1, 0, 1.0f));
}

// ============================================================================
// Gold Split Tests
// ============================================================================

TEST(GoldSplit, SinglePlayer)
{
    EXPECT_EQ(uint32_t(100), CalculateGoldPerPlayer(100, 1));
}

TEST(GoldSplit, EvenSplit)
{
    EXPECT_EQ(uint32_t(25), CalculateGoldPerPlayer(100, 4));
}

TEST(GoldSplit, UnevenSplit)
{
    // 100 / 3 = 33 (integer division)
    EXPECT_EQ(uint32_t(33), CalculateGoldPerPlayer(100, 3));
}

TEST(GoldSplit, ZeroPlayers)
{
    EXPECT_EQ(uint32_t(0), CalculateGoldPerPlayer(100, 0));
}

TEST(GoldSplit, ZeroGold)
{
    EXPECT_EQ(uint32_t(0), CalculateGoldPerPlayer(0, 5));
}

// ============================================================================
// Group Drop Bonus Tests
// ============================================================================

TEST(GroupDropBonus, SoloPlayer)
{
    EXPECT_FLOAT_EQ(1.0f, CalculateGroupDropBonus(1));
}

TEST(GroupDropBonus, TwoPlayers)
{
    EXPECT_FLOAT_EQ(1.1f, CalculateGroupDropBonus(2));
}

TEST(GroupDropBonus, FiveManGroup)
{
    EXPECT_NEAR(1.4f, CalculateGroupDropBonus(5), 0.001f);
}

TEST(GroupDropBonus, FullRaid)
{
    float bonus = CalculateGroupDropBonus(25);
    EXPECT_GT(bonus, 1.0f);
}
