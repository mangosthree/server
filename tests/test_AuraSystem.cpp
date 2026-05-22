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
#include <map>

// ============================================================================
// Standalone reimplementation of Aura system patterns
// from src/game/Object/SpellAuras.h and SpellAuras.cpp
// ============================================================================

namespace AuraTest
{

enum AuraFlags : uint8_t
{
    AFLAG_NONE              = 0x00,
    AFLAG_EFF_INDEX_0       = 0x01,
    AFLAG_EFF_INDEX_1       = 0x02,
    AFLAG_EFF_INDEX_2       = 0x04,
    AFLAG_IS_CASTER         = 0x08,
    AFLAG_POSITIVE          = 0x10,
    AFLAG_DURATION          = 0x20,
    AFLAG_ANY_EFFECT_AMOUNT = 0x40,
    AFLAG_NEGATIVE          = 0x80,
};

enum AuraRemoveMode
{
    AURA_REMOVE_BY_DEFAULT,
    AURA_REMOVE_BY_STACK,
    AURA_REMOVE_BY_CANCEL,
    AURA_REMOVE_BY_DISPEL,
    AURA_REMOVE_BY_EXPIRE,
    AURA_REMOVE_BY_DELETE
};

struct AuraEntry
{
    uint32_t spellId;
    uint32_t casterId;
    int32_t  baseAmount;
    int32_t  duration;
    int32_t  maxDuration;
    uint8_t  stackCount;
    uint8_t  maxStack;
    uint8_t  charges;
    uint8_t  flags;
    bool     isPassive;
    bool     isPermanent;
};

class AuraManager
{
public:
    bool AddAura(const AuraEntry& entry)
    {
        auto it = FindAura(entry.spellId, entry.casterId);
        if (it != m_auras.end())
        {
            // Refresh or stack
            if (it->stackCount < it->maxStack)
            {
                it->stackCount++;
                it->duration = it->maxDuration;
                return true;
            }
            else
            {
                // At max stacks, just refresh duration
                it->duration = it->maxDuration;
                return true;
            }
        }

        if (m_auras.size() >= MAX_AURAS)
            return false;

        m_auras.push_back(entry);
        if (entry.stackCount == 0)
            m_auras.back().stackCount = 1;
        return true;
    }

    bool RemoveAura(uint32_t spellId, uint32_t casterId, AuraRemoveMode mode = AURA_REMOVE_BY_DEFAULT)
    {
        auto it = FindAura(spellId, casterId);
        if (it == m_auras.end()) return false;
        m_auras.erase(it);
        return true;
    }

    void RemoveAllAuras()
    {
        m_auras.clear();
    }

    void RemoveAurasByDispel(uint32_t count)
    {
        uint32_t removed = 0;
        auto it = m_auras.begin();
        while (it != m_auras.end() && removed < count)
        {
            if (!(it->flags & AFLAG_POSITIVE))
            {
                it = m_auras.erase(it);
                ++removed;
            }
            else
            {
                ++it;
            }
        }
    }

    void UpdateDurations(int32_t diff)
    {
        auto it = m_auras.begin();
        while (it != m_auras.end())
        {
            if (!it->isPermanent)
            {
                it->duration -= diff;
                if (it->duration <= 0)
                {
                    it = m_auras.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    bool ConsumeCharge(uint32_t spellId, uint32_t casterId)
    {
        auto it = FindAura(spellId, casterId);
        if (it == m_auras.end()) return false;
        if (it->charges == 0) return false;
        it->charges--;
        if (it->charges == 0)
        {
            m_auras.erase(it);
            return true;
        }
        return true;
    }

    bool HasAura(uint32_t spellId) const
    {
        for (auto& a : m_auras)
            if (a.spellId == spellId) return true;
        return false;
    }

    bool HasAuraFromCaster(uint32_t spellId, uint32_t casterId) const
    {
        for (auto& a : m_auras)
            if (a.spellId == spellId && a.casterId == casterId) return true;
        return false;
    }

    uint8_t GetStackCount(uint32_t spellId) const
    {
        for (auto& a : m_auras)
            if (a.spellId == spellId) return a.stackCount;
        return 0;
    }

    size_t GetAuraCount() const { return m_auras.size(); }

    const AuraEntry* GetAura(uint32_t spellId) const
    {
        for (auto& a : m_auras)
            if (a.spellId == spellId) return &a;
        return nullptr;
    }

    static const size_t MAX_AURAS = 64;

private:
    std::vector<AuraEntry>::iterator FindAura(uint32_t spellId, uint32_t casterId)
    {
        for (auto it = m_auras.begin(); it != m_auras.end(); ++it)
            if (it->spellId == spellId && it->casterId == casterId) return it;
        return m_auras.end();
    }

    std::vector<AuraEntry> m_auras;
};

} // namespace AuraTest

using namespace AuraTest;

// Fixture for AuraManager tests
struct AuraManagerFixture
{
    void SetUp() { mgr = AuraManager(); }
    void TearDown() {}

    AuraEntry MakeAura(uint32_t spellId, uint32_t casterId, int32_t duration = 30000,
                       uint8_t maxStack = 1, uint8_t charges = 0, uint8_t flags = AFLAG_NONE)
    {
        AuraEntry e = {};
        e.spellId = spellId;
        e.casterId = casterId;
        e.baseAmount = 100;
        e.duration = duration;
        e.maxDuration = duration;
        e.stackCount = 1;
        e.maxStack = maxStack;
        e.charges = charges;
        e.flags = flags;
        e.isPassive = false;
        e.isPermanent = false;
        return e;
    }

    AuraManager mgr;
};

// ============================================================================
// AuraFlags Tests
// ============================================================================

TEST(AuraFlags, NoneIsZero)
{
    EXPECT_EQ(0, AFLAG_NONE);
}

TEST(AuraFlags, AreDistinctBits)
{
    EXPECT_EQ(0x01, AFLAG_EFF_INDEX_0);
    EXPECT_EQ(0x02, AFLAG_EFF_INDEX_1);
    EXPECT_EQ(0x04, AFLAG_EFF_INDEX_2);
    EXPECT_EQ(0x08, AFLAG_IS_CASTER);
    EXPECT_EQ(0x10, AFLAG_POSITIVE);
    EXPECT_EQ(0x20, AFLAG_DURATION);
    EXPECT_EQ(0x80, AFLAG_NEGATIVE);
}

TEST(AuraFlags, CanCombine)
{
    uint8_t flags = AFLAG_POSITIVE | AFLAG_DURATION | AFLAG_EFF_INDEX_0;
    EXPECT_TRUE((flags & AFLAG_POSITIVE) != 0);
    EXPECT_TRUE((flags & AFLAG_DURATION) != 0);
    EXPECT_TRUE((flags & AFLAG_EFF_INDEX_0) != 0);
    EXPECT_FALSE((flags & AFLAG_NEGATIVE) != 0);
}

// ============================================================================
// AuraManager Tests (using TEST_F)
// ============================================================================

TEST_F(AuraManagerFixture, InitiallyEmpty)
{
    EXPECT_EQ(size_t(0), mgr.GetAuraCount());
}

TEST_F(AuraManagerFixture, AddAura)
{
    EXPECT_TRUE(mgr.AddAura(MakeAura(1000, 1)));
    EXPECT_EQ(size_t(1), mgr.GetAuraCount());
    EXPECT_TRUE(mgr.HasAura(1000));
}

TEST_F(AuraManagerFixture, AddMultipleAuras)
{
    mgr.AddAura(MakeAura(1000, 1));
    mgr.AddAura(MakeAura(2000, 1));
    mgr.AddAura(MakeAura(3000, 2));
    EXPECT_EQ(size_t(3), mgr.GetAuraCount());
}

TEST_F(AuraManagerFixture, RemoveAura)
{
    mgr.AddAura(MakeAura(1000, 1));
    EXPECT_TRUE(mgr.RemoveAura(1000, 1));
    EXPECT_EQ(size_t(0), mgr.GetAuraCount());
    EXPECT_FALSE(mgr.HasAura(1000));
}

TEST_F(AuraManagerFixture, RemoveNonexistent)
{
    EXPECT_FALSE(mgr.RemoveAura(9999, 1));
}

TEST_F(AuraManagerFixture, HasAuraFromCaster)
{
    mgr.AddAura(MakeAura(1000, 1));
    EXPECT_TRUE(mgr.HasAuraFromCaster(1000, 1));
    EXPECT_FALSE(mgr.HasAuraFromCaster(1000, 2));
}

TEST_F(AuraManagerFixture, StackingAura)
{
    auto aura = MakeAura(1000, 1, 30000, 5); // max 5 stacks
    mgr.AddAura(aura);
    EXPECT_EQ(uint8_t(1), mgr.GetStackCount(1000));

    mgr.AddAura(aura);
    EXPECT_EQ(uint8_t(2), mgr.GetStackCount(1000));

    mgr.AddAura(aura);
    EXPECT_EQ(uint8_t(3), mgr.GetStackCount(1000));
    EXPECT_EQ(size_t(1), mgr.GetAuraCount()); // still one entry, just stacked
}

TEST_F(AuraManagerFixture, MaxStackCap)
{
    auto aura = MakeAura(1000, 1, 30000, 3); // max 3 stacks
    for (int i = 0; i < 10; ++i)
        mgr.AddAura(aura);
    EXPECT_EQ(uint8_t(3), mgr.GetStackCount(1000));
}

TEST_F(AuraManagerFixture, DurationRefreshOnStack)
{
    auto aura = MakeAura(1000, 1, 30000, 5);
    mgr.AddAura(aura);

    // Simulate time passing
    mgr.UpdateDurations(20000);

    // Re-apply should refresh duration
    mgr.AddAura(aura);
    auto* a = mgr.GetAura(1000);
    EXPECT_TRUE(a != nullptr);
    EXPECT_EQ(int32_t(30000), a->duration);
}

TEST_F(AuraManagerFixture, DurationExpiry)
{
    mgr.AddAura(MakeAura(1000, 1, 5000));
    mgr.UpdateDurations(5001);
    EXPECT_EQ(size_t(0), mgr.GetAuraCount());
}

TEST_F(AuraManagerFixture, DurationPartialExpiry)
{
    mgr.AddAura(MakeAura(1000, 1, 10000));
    mgr.AddAura(MakeAura(2000, 1, 5000));
    mgr.UpdateDurations(6000);
    EXPECT_EQ(size_t(1), mgr.GetAuraCount());
    EXPECT_TRUE(mgr.HasAura(1000));
    EXPECT_FALSE(mgr.HasAura(2000));
}

TEST_F(AuraManagerFixture, PermanentAuraDoesNotExpire)
{
    auto aura = MakeAura(1000, 1, -1);
    aura.isPermanent = true;
    mgr.AddAura(aura);
    mgr.UpdateDurations(999999);
    EXPECT_EQ(size_t(1), mgr.GetAuraCount());
}

TEST_F(AuraManagerFixture, ChargeConsumption)
{
    auto aura = MakeAura(1000, 1, 30000, 1, 3); // 3 charges
    mgr.AddAura(aura);

    EXPECT_TRUE(mgr.ConsumeCharge(1000, 1));
    EXPECT_TRUE(mgr.HasAura(1000)); // 2 charges left

    EXPECT_TRUE(mgr.ConsumeCharge(1000, 1));
    EXPECT_TRUE(mgr.HasAura(1000)); // 1 charge left

    EXPECT_TRUE(mgr.ConsumeCharge(1000, 1));
    EXPECT_FALSE(mgr.HasAura(1000)); // 0 charges, removed
}

TEST_F(AuraManagerFixture, ConsumeChargeNoCharges)
{
    auto aura = MakeAura(1000, 1, 30000, 1, 0); // 0 charges
    mgr.AddAura(aura);
    EXPECT_FALSE(mgr.ConsumeCharge(1000, 1));
    EXPECT_TRUE(mgr.HasAura(1000)); // still present
}

TEST_F(AuraManagerFixture, RemoveAllAuras)
{
    mgr.AddAura(MakeAura(1000, 1));
    mgr.AddAura(MakeAura(2000, 1));
    mgr.AddAura(MakeAura(3000, 2));
    mgr.RemoveAllAuras();
    EXPECT_EQ(size_t(0), mgr.GetAuraCount());
}

TEST_F(AuraManagerFixture, DispelRemovesNegative)
{
    mgr.AddAura(MakeAura(1000, 1, 30000, 1, 0, AFLAG_POSITIVE)); // positive
    mgr.AddAura(MakeAura(2000, 2, 30000, 1, 0, AFLAG_NONE));     // negative (no POSITIVE flag)
    mgr.AddAura(MakeAura(3000, 2, 30000, 1, 0, AFLAG_NONE));     // negative

    mgr.RemoveAurasByDispel(1);
    EXPECT_EQ(size_t(2), mgr.GetAuraCount());
    EXPECT_TRUE(mgr.HasAura(1000)); // positive stays
}

TEST_F(AuraManagerFixture, MaxAurasLimit)
{
    for (uint32_t i = 0; i < AuraManager::MAX_AURAS; ++i)
        EXPECT_TRUE(mgr.AddAura(MakeAura(i + 1, 1)));

    // 65th aura should fail
    EXPECT_FALSE(mgr.AddAura(MakeAura(9999, 1)));
    EXPECT_EQ(AuraManager::MAX_AURAS, mgr.GetAuraCount());
}

TEST_F(AuraManagerFixture, SameSpellDifferentCasters)
{
    mgr.AddAura(MakeAura(1000, 1));
    mgr.AddAura(MakeAura(1000, 2));
    EXPECT_EQ(size_t(2), mgr.GetAuraCount());
    EXPECT_TRUE(mgr.HasAuraFromCaster(1000, 1));
    EXPECT_TRUE(mgr.HasAuraFromCaster(1000, 2));
}

// ============================================================================
// AuraRemoveMode Tests
// ============================================================================

TEST(AuraRemoveMode, EnumValues)
{
    EXPECT_EQ(0, AURA_REMOVE_BY_DEFAULT);
    EXPECT_EQ(1, AURA_REMOVE_BY_STACK);
    EXPECT_EQ(2, AURA_REMOVE_BY_CANCEL);
    EXPECT_EQ(3, AURA_REMOVE_BY_DISPEL);
    EXPECT_EQ(4, AURA_REMOVE_BY_EXPIRE);
    EXPECT_EQ(5, AURA_REMOVE_BY_DELETE);
}
