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
#include <cmath>
#include <map>

// ============================================================================
// Standalone reimplementation of ThreatManager patterns
// from src/game/Object/ThreatManager.h/.cpp
// ============================================================================

namespace ThreatTest
{

struct ThreatEntry
{
    uint64_t guid;
    float    threat;
    bool     isOnline;
    bool     isTauntedBy;
};

class ThreatManager
{
public:
    void AddThreat(uint64_t guid, float threat)
    {
        if (threat <= 0.0f) return;

        auto it = FindEntry(guid);
        if (it != m_entries.end())
        {
            it->threat += threat;
        }
        else
        {
            m_entries.push_back({guid, threat, true, false});
        }
        m_dirty = true;
    }

    void ModifyThreatPct(uint64_t guid, float pct)
    {
        auto it = FindEntry(guid);
        if (it == m_entries.end()) return;
        it->threat *= (1.0f + pct / 100.0f);
        if (it->threat < 0.0f) it->threat = 0.0f;
        m_dirty = true;
    }

    void SetThreat(uint64_t guid, float threat)
    {
        auto it = FindEntry(guid);
        if (it != m_entries.end())
            it->threat = threat;
        m_dirty = true;
    }

    void RemoveThreat(uint64_t guid)
    {
        auto it = FindEntry(guid);
        if (it != m_entries.end())
            m_entries.erase(it);
        m_dirty = true;
    }

    void ClearAllThreat()
    {
        m_entries.clear();
        m_dirty = true;
    }

    void Taunt(uint64_t guid)
    {
        for (auto& e : m_entries)
            e.isTauntedBy = (e.guid == guid);
        m_dirty = true;
    }

    void ClearTaunt()
    {
        for (auto& e : m_entries)
            e.isTauntedBy = false;
        m_dirty = true;
    }

    void SetOffline(uint64_t guid, bool offline)
    {
        auto it = FindEntry(guid);
        if (it != m_entries.end())
            it->isOnline = !offline;
        m_dirty = true;
    }

    uint64_t GetTopThreatGuid() const
    {
        // Taunt overrides threat list
        for (const auto& e : m_entries)
            if (e.isTauntedBy && e.isOnline) return e.guid;

        float best = -1.0f;
        uint64_t bestGuid = 0;
        for (const auto& e : m_entries)
        {
            if (e.isOnline && e.threat > best)
            {
                best = e.threat;
                bestGuid = e.guid;
            }
        }
        return bestGuid;
    }

    float GetThreat(uint64_t guid) const
    {
        for (const auto& e : m_entries)
            if (e.guid == guid) return e.threat;
        return 0.0f;
    }

    bool HasThreat(uint64_t guid) const
    {
        for (const auto& e : m_entries)
            if (e.guid == guid) return true;
        return false;
    }

    size_t GetThreatListSize() const { return m_entries.size(); }

    // Requires 110% threat to pull aggro in melee, 130% at range
    bool CanOvertake(uint64_t challenger, bool isMelee) const
    {
        uint64_t current = GetTopThreatGuid();
        if (current == 0 || current == challenger) return true;

        float currentThreat = GetThreat(current);
        float challengerThreat = GetThreat(challenger);
        float threshold = isMelee ? 1.10f : 1.30f;

        return challengerThreat >= currentThreat * threshold;
    }

private:
    std::vector<ThreatEntry>::iterator FindEntry(uint64_t guid)
    {
        for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
            if (it->guid == guid) return it;
        return m_entries.end();
    }

    std::vector<ThreatEntry> m_entries;
    bool m_dirty = false;
};

} // namespace ThreatTest

using namespace ThreatTest;

struct ThreatManagerFixture
{
    void SetUp() { tm = ThreatManager(); }
    void TearDown() {}
    ThreatManager tm;
};

// ============================================================================
// ThreatManager Tests
// ============================================================================

TEST_F(ThreatManagerFixture, InitiallyEmpty)
{
    EXPECT_EQ(size_t(0), tm.GetThreatListSize());
    EXPECT_EQ(uint64_t(0), tm.GetTopThreatGuid());
}

TEST_F(ThreatManagerFixture, AddThreat)
{
    tm.AddThreat(1, 100.0f);
    EXPECT_TRUE(tm.HasThreat(1));
    EXPECT_FLOAT_EQ(100.0f, tm.GetThreat(1));
    EXPECT_EQ(size_t(1), tm.GetThreatListSize());
}

TEST_F(ThreatManagerFixture, AddThreatCumulative)
{
    tm.AddThreat(1, 100.0f);
    tm.AddThreat(1, 50.0f);
    EXPECT_FLOAT_EQ(150.0f, tm.GetThreat(1));
    EXPECT_EQ(size_t(1), tm.GetThreatListSize());
}

TEST_F(ThreatManagerFixture, ZeroThreatIgnored)
{
    tm.AddThreat(1, 0.0f);
    EXPECT_FALSE(tm.HasThreat(1));
}

TEST_F(ThreatManagerFixture, NegativeThreatIgnored)
{
    tm.AddThreat(1, -50.0f);
    EXPECT_FALSE(tm.HasThreat(1));
}

TEST_F(ThreatManagerFixture, TopThreat)
{
    tm.AddThreat(1, 100.0f);
    tm.AddThreat(2, 200.0f);
    tm.AddThreat(3, 150.0f);
    EXPECT_EQ(uint64_t(2), tm.GetTopThreatGuid());
}

TEST_F(ThreatManagerFixture, ModifyThreatPct)
{
    tm.AddThreat(1, 100.0f);
    tm.ModifyThreatPct(1, 50.0f); // +50%
    EXPECT_FLOAT_EQ(150.0f, tm.GetThreat(1));
}

TEST_F(ThreatManagerFixture, ModifyThreatPctNegative)
{
    tm.AddThreat(1, 100.0f);
    tm.ModifyThreatPct(1, -50.0f); // -50%
    EXPECT_FLOAT_EQ(50.0f, tm.GetThreat(1));
}

TEST_F(ThreatManagerFixture, ModifyThreatPctClampToZero)
{
    tm.AddThreat(1, 100.0f);
    tm.ModifyThreatPct(1, -150.0f); // -150% -> clamped to 0
    EXPECT_FLOAT_EQ(0.0f, tm.GetThreat(1));
}

TEST_F(ThreatManagerFixture, SetThreat)
{
    tm.AddThreat(1, 100.0f);
    tm.SetThreat(1, 500.0f);
    EXPECT_FLOAT_EQ(500.0f, tm.GetThreat(1));
}

TEST_F(ThreatManagerFixture, RemoveThreat)
{
    tm.AddThreat(1, 100.0f);
    tm.AddThreat(2, 200.0f);
    tm.RemoveThreat(1);
    EXPECT_FALSE(tm.HasThreat(1));
    EXPECT_TRUE(tm.HasThreat(2));
    EXPECT_EQ(size_t(1), tm.GetThreatListSize());
}

TEST_F(ThreatManagerFixture, ClearAllThreat)
{
    tm.AddThreat(1, 100.0f);
    tm.AddThreat(2, 200.0f);
    tm.ClearAllThreat();
    EXPECT_EQ(size_t(0), tm.GetThreatListSize());
}

TEST_F(ThreatManagerFixture, TauntOverridesThreat)
{
    tm.AddThreat(1, 100.0f);
    tm.AddThreat(2, 200.0f); // higher threat
    EXPECT_EQ(uint64_t(2), tm.GetTopThreatGuid());

    tm.Taunt(1);
    EXPECT_EQ(uint64_t(1), tm.GetTopThreatGuid());
}

TEST_F(ThreatManagerFixture, ClearTaunt)
{
    tm.AddThreat(1, 100.0f);
    tm.AddThreat(2, 200.0f);
    tm.Taunt(1);
    tm.ClearTaunt();
    EXPECT_EQ(uint64_t(2), tm.GetTopThreatGuid());
}

TEST_F(ThreatManagerFixture, OfflineExcludedFromTop)
{
    tm.AddThreat(1, 200.0f);
    tm.AddThreat(2, 100.0f);
    tm.SetOffline(1, true);
    EXPECT_EQ(uint64_t(2), tm.GetTopThreatGuid());
}

TEST_F(ThreatManagerFixture, BackOnline)
{
    tm.AddThreat(1, 200.0f);
    tm.AddThreat(2, 100.0f);
    tm.SetOffline(1, true);
    tm.SetOffline(1, false);
    EXPECT_EQ(uint64_t(1), tm.GetTopThreatGuid());
}

TEST_F(ThreatManagerFixture, MeleeOvertakeAt110Pct)
{
    tm.AddThreat(1, 200.0f);  // current top
    tm.AddThreat(2, 219.0f);  // challenger: 219 < 200*1.1=220
    // Player 2 has higher raw threat, but CanOvertake checks from current top (player 2 IS top)
    // So test from player 3's perspective:
    tm.AddThreat(3, 199.0f);
    EXPECT_FALSE(tm.CanOvertake(3, true)); // 199 < 219*1.1=240.9

    tm.AddThreat(3, 42.0f); // now 241
    EXPECT_TRUE(tm.CanOvertake(3, true));
}

TEST_F(ThreatManagerFixture, RangedOvertakeAt130Pct)
{
    tm.AddThreat(1, 100.0f);  // current top
    tm.AddThreat(2, 50.0f);   // challenger at range
    EXPECT_FALSE(tm.CanOvertake(2, false)); // 50 < 100*1.3=130

    tm.AddThreat(2, 81.0f); // now 131
    EXPECT_TRUE(tm.CanOvertake(2, false));
}

TEST_F(ThreatManagerFixture, EmptyListOvertake)
{
    EXPECT_TRUE(tm.CanOvertake(1, true)); // no current target
}

TEST_F(ThreatManagerFixture, CurrentTopAlwaysCanOvertake)
{
    tm.AddThreat(1, 100.0f);
    EXPECT_TRUE(tm.CanOvertake(1, true)); // already on top
}

TEST_F(ThreatManagerFixture, MultiplePlayers)
{
    tm.AddThreat(1, 1000.0f);  // tank
    tm.AddThreat(2, 500.0f);   // DPS 1
    tm.AddThreat(3, 800.0f);   // DPS 2
    tm.AddThreat(4, 300.0f);   // healer
    tm.AddThreat(5, 700.0f);   // DPS 3

    EXPECT_EQ(uint64_t(1), tm.GetTopThreatGuid());
    EXPECT_EQ(size_t(5), tm.GetThreatListSize());
}

TEST_F(ThreatManagerFixture, ThreatNonexistentGuidIsZero)
{
    EXPECT_FLOAT_EQ(0.0f, tm.GetThreat(999));
}
