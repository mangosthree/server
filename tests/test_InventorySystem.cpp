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

// ============================================================================
// Standalone reimplementation of Inventory/Bag slot patterns
// from src/game/Object/Player.h, Item.h
// ============================================================================

namespace InventoryTest
{

enum InventorySlots
{
    EQUIPMENT_SLOT_START    = 0,
    EQUIPMENT_SLOT_HEAD     = 0,
    EQUIPMENT_SLOT_NECK     = 1,
    EQUIPMENT_SLOT_SHOULDERS = 2,
    EQUIPMENT_SLOT_BODY     = 3, // shirt
    EQUIPMENT_SLOT_CHEST    = 4,
    EQUIPMENT_SLOT_WAIST    = 5,
    EQUIPMENT_SLOT_LEGS     = 6,
    EQUIPMENT_SLOT_FEET     = 7,
    EQUIPMENT_SLOT_WRISTS   = 8,
    EQUIPMENT_SLOT_HANDS    = 9,
    EQUIPMENT_SLOT_FINGER1  = 10,
    EQUIPMENT_SLOT_FINGER2  = 11,
    EQUIPMENT_SLOT_TRINKET1 = 12,
    EQUIPMENT_SLOT_TRINKET2 = 13,
    EQUIPMENT_SLOT_BACK     = 14,
    EQUIPMENT_SLOT_MAINHAND = 15,
    EQUIPMENT_SLOT_OFFHAND  = 16,
    EQUIPMENT_SLOT_RANGED   = 17,
    EQUIPMENT_SLOT_TABARD   = 18,
    EQUIPMENT_SLOT_END      = 19,

    INVENTORY_SLOT_BAG_START = 19,
    INVENTORY_SLOT_BAG_END   = 23,

    INVENTORY_SLOT_ITEM_START = 23,
    INVENTORY_SLOT_ITEM_END   = 39,

    BANK_SLOT_ITEM_START     = 39,
    BANK_SLOT_ITEM_END       = 67,

    BANK_SLOT_BAG_START      = 67,
    BANK_SLOT_BAG_END        = 74,
};

enum InventoryResult
{
    EQUIP_ERR_OK                    = 0,
    EQUIP_ERR_CANT_EQUIP_LEVEL_I    = 1,
    EQUIP_ERR_CANT_EQUIP_SKILL      = 2,
    EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT = 3,
    EQUIP_ERR_BAG_FULL              = 4,
    EQUIP_ERR_ITEM_NOT_FOUND        = 5,
    EQUIP_ERR_CANT_STACK            = 6,
    EQUIP_ERR_ITEM_MAX_COUNT        = 7,
};

enum ItemQuality
{
    ITEM_QUALITY_POOR       = 0, // grey
    ITEM_QUALITY_NORMAL     = 1, // white
    ITEM_QUALITY_UNCOMMON   = 2, // green
    ITEM_QUALITY_RARE       = 3, // blue
    ITEM_QUALITY_EPIC       = 4, // purple
    ITEM_QUALITY_LEGENDARY  = 5, // orange
    ITEM_QUALITY_ARTIFACT   = 6, // red
    ITEM_QUALITY_HEIRLOOM   = 7,
    MAX_ITEM_QUALITY        = 8,
};

struct ItemTemplate
{
    uint32_t itemId;
    uint32_t maxStack;
    uint32_t requiredLevel;
    uint8_t  quality;
    uint32_t slot;
    uint32_t itemLevel;
    uint32_t buyPrice;
    uint32_t sellPrice;
};

struct InventoryItem
{
    uint32_t itemId;
    uint32_t count;
    uint8_t  slot;
};

class Inventory
{
public:
    static const uint8_t BACKPACK_SIZE = 16;

    Inventory() : m_slots(BACKPACK_SIZE, {0, 0, 0})
    {
        for (uint8_t i = 0; i < BACKPACK_SIZE; ++i)
            m_slots[i].slot = i;
    }

    InventoryResult AddItem(uint32_t itemId, uint32_t count, uint32_t maxStack)
    {
        // First try to stack with existing
        for (auto& slot : m_slots)
        {
            if (slot.itemId == itemId && slot.count < maxStack)
            {
                uint32_t canAdd = maxStack - slot.count;
                uint32_t toAdd = std::min(count, canAdd);
                slot.count += toAdd;
                count -= toAdd;
                if (count == 0) return EQUIP_ERR_OK;
            }
        }

        // Put remaining in empty slots
        while (count > 0)
        {
            int8_t freeSlot = FindFreeSlot();
            if (freeSlot < 0) return EQUIP_ERR_BAG_FULL;

            uint32_t toAdd = std::min(count, maxStack);
            m_slots[freeSlot].itemId = itemId;
            m_slots[freeSlot].count = toAdd;
            count -= toAdd;
        }

        return EQUIP_ERR_OK;
    }

    InventoryResult RemoveItem(uint32_t itemId, uint32_t count)
    {
        uint32_t totalFound = GetItemCount(itemId);
        if (totalFound < count) return EQUIP_ERR_ITEM_NOT_FOUND;

        for (auto& slot : m_slots)
        {
            if (slot.itemId == itemId && count > 0)
            {
                uint32_t toRemove = std::min(count, slot.count);
                slot.count -= toRemove;
                count -= toRemove;
                if (slot.count == 0)
                    slot.itemId = 0;
            }
        }
        return EQUIP_ERR_OK;
    }

    uint32_t GetItemCount(uint32_t itemId) const
    {
        uint32_t total = 0;
        for (auto& slot : m_slots)
            if (slot.itemId == itemId) total += slot.count;
        return total;
    }

    bool HasItem(uint32_t itemId) const
    {
        return GetItemCount(itemId) > 0;
    }

    uint8_t GetFreeSlotCount() const
    {
        uint8_t count = 0;
        for (auto& slot : m_slots)
            if (slot.itemId == 0) ++count;
        return count;
    }

    bool IsFull() const
    {
        return GetFreeSlotCount() == 0;
    }

    void Clear()
    {
        for (auto& slot : m_slots)
        {
            slot.itemId = 0;
            slot.count = 0;
        }
    }

    bool SwapSlots(uint8_t slot1, uint8_t slot2)
    {
        if (slot1 >= BACKPACK_SIZE || slot2 >= BACKPACK_SIZE) return false;
        std::swap(m_slots[slot1].itemId, m_slots[slot2].itemId);
        std::swap(m_slots[slot1].count, m_slots[slot2].count);
        return true;
    }

    const InventoryItem& GetSlot(uint8_t slot) const { return m_slots[slot]; }

private:
    int8_t FindFreeSlot() const
    {
        for (uint8_t i = 0; i < BACKPACK_SIZE; ++i)
            if (m_slots[i].itemId == 0) return i;
        return -1;
    }

    std::vector<InventoryItem> m_slots;
};

// Vendor sell price calculation
uint32_t CalculateSellPrice(uint32_t buyPrice, uint8_t quality)
{
    float ratio;
    switch (quality)
    {
        case ITEM_QUALITY_POOR:     ratio = 0.10f; break;
        case ITEM_QUALITY_NORMAL:   ratio = 0.20f; break;
        case ITEM_QUALITY_UNCOMMON: ratio = 0.25f; break;
        case ITEM_QUALITY_RARE:     ratio = 0.25f; break;
        case ITEM_QUALITY_EPIC:     ratio = 0.25f; break;
        default:                    ratio = 0.25f; break;
    }
    return uint32_t(float(buyPrice) * ratio);
}

// Repair cost (based on item level and quality)
uint32_t CalculateRepairCost(uint32_t itemLevel, uint8_t quality, float durabilityLostPct)
{
    float baseCost = float(itemLevel) * float(itemLevel) * 0.5f;
    float qualityMult = 1.0f + float(quality) * 0.5f;
    return uint32_t(baseCost * qualityMult * durabilityLostPct);
}

} // namespace InventoryTest

using namespace InventoryTest;

struct InventoryFixture
{
    void SetUp() { inv = Inventory(); }
    void TearDown() {}
    Inventory inv;
};

// ============================================================================
// Equipment Slot Tests
// ============================================================================

TEST(EquipmentSlots, Range)
{
    EXPECT_EQ(0, EQUIPMENT_SLOT_START);
    EXPECT_EQ(19, EQUIPMENT_SLOT_END);
    EXPECT_EQ(19, EQUIPMENT_SLOT_END - EQUIPMENT_SLOT_START);
}

TEST(EquipmentSlots, BagSlots)
{
    EXPECT_EQ(19, INVENTORY_SLOT_BAG_START);
    EXPECT_EQ(23, INVENTORY_SLOT_BAG_END);
    EXPECT_EQ(4, INVENTORY_SLOT_BAG_END - INVENTORY_SLOT_BAG_START);
}

TEST(EquipmentSlots, BackpackSlots)
{
    EXPECT_EQ(23, INVENTORY_SLOT_ITEM_START);
    EXPECT_EQ(39, INVENTORY_SLOT_ITEM_END);
    EXPECT_EQ(16, INVENTORY_SLOT_ITEM_END - INVENTORY_SLOT_ITEM_START);
}

TEST(EquipmentSlots, BankSlots)
{
    EXPECT_EQ(39, BANK_SLOT_ITEM_START);
    EXPECT_EQ(67, BANK_SLOT_ITEM_END);
    EXPECT_EQ(28, BANK_SLOT_ITEM_END - BANK_SLOT_ITEM_START);
}

TEST(EquipmentSlots, BankBagSlots)
{
    EXPECT_EQ(67, BANK_SLOT_BAG_START);
    EXPECT_EQ(74, BANK_SLOT_BAG_END);
    EXPECT_EQ(7, BANK_SLOT_BAG_END - BANK_SLOT_BAG_START);
}

// ============================================================================
// Item Quality Tests
// ============================================================================

TEST(ItemQuality, Ordering)
{
    EXPECT_LT(ITEM_QUALITY_POOR, ITEM_QUALITY_NORMAL);
    EXPECT_LT(ITEM_QUALITY_NORMAL, ITEM_QUALITY_UNCOMMON);
    EXPECT_LT(ITEM_QUALITY_UNCOMMON, ITEM_QUALITY_RARE);
    EXPECT_LT(ITEM_QUALITY_RARE, ITEM_QUALITY_EPIC);
    EXPECT_LT(ITEM_QUALITY_EPIC, ITEM_QUALITY_LEGENDARY);
}

TEST(ItemQuality, MaxQuality)
{
    EXPECT_EQ(8, MAX_ITEM_QUALITY);
}

// ============================================================================
// Inventory Tests (using TEST_F)
// ============================================================================

TEST_F(InventoryFixture, InitiallyEmpty)
{
    EXPECT_EQ(uint8_t(16), inv.GetFreeSlotCount());
    EXPECT_FALSE(inv.IsFull());
}

TEST_F(InventoryFixture, AddSingleItem)
{
    EXPECT_EQ(EQUIP_ERR_OK, inv.AddItem(1000, 1, 20));
    EXPECT_TRUE(inv.HasItem(1000));
    EXPECT_EQ(uint32_t(1), inv.GetItemCount(1000));
    EXPECT_EQ(uint8_t(15), inv.GetFreeSlotCount());
}

TEST_F(InventoryFixture, AddStackedItem)
{
    EXPECT_EQ(EQUIP_ERR_OK, inv.AddItem(1000, 10, 20));
    EXPECT_EQ(uint32_t(10), inv.GetItemCount(1000));
    EXPECT_EQ(uint8_t(15), inv.GetFreeSlotCount()); // one slot used
}

TEST_F(InventoryFixture, AddItemExceedsStack)
{
    EXPECT_EQ(EQUIP_ERR_OK, inv.AddItem(1000, 25, 20));
    EXPECT_EQ(uint32_t(25), inv.GetItemCount(1000));
    EXPECT_EQ(uint8_t(14), inv.GetFreeSlotCount()); // two slots used
}

TEST_F(InventoryFixture, AddItemStacksWithExisting)
{
    inv.AddItem(1000, 10, 20);
    inv.AddItem(1000, 5, 20);
    EXPECT_EQ(uint32_t(15), inv.GetItemCount(1000));
    EXPECT_EQ(uint8_t(15), inv.GetFreeSlotCount()); // still one slot
}

TEST_F(InventoryFixture, AddItemBagFull)
{
    // Fill all 16 slots
    for (uint32_t i = 0; i < 16; ++i)
        inv.AddItem(i + 1, 1, 1);

    EXPECT_TRUE(inv.IsFull());
    EXPECT_EQ(EQUIP_ERR_BAG_FULL, inv.AddItem(9999, 1, 1));
}

TEST_F(InventoryFixture, RemoveItem)
{
    inv.AddItem(1000, 10, 20);
    EXPECT_EQ(EQUIP_ERR_OK, inv.RemoveItem(1000, 5));
    EXPECT_EQ(uint32_t(5), inv.GetItemCount(1000));
}

TEST_F(InventoryFixture, RemoveAllOfItem)
{
    inv.AddItem(1000, 10, 20);
    EXPECT_EQ(EQUIP_ERR_OK, inv.RemoveItem(1000, 10));
    EXPECT_FALSE(inv.HasItem(1000));
    EXPECT_EQ(uint8_t(16), inv.GetFreeSlotCount());
}

TEST_F(InventoryFixture, RemoveMoreThanExists)
{
    inv.AddItem(1000, 5, 20);
    EXPECT_EQ(EQUIP_ERR_ITEM_NOT_FOUND, inv.RemoveItem(1000, 10));
    EXPECT_EQ(uint32_t(5), inv.GetItemCount(1000)); // unchanged
}

TEST_F(InventoryFixture, RemoveNonExistent)
{
    EXPECT_EQ(EQUIP_ERR_ITEM_NOT_FOUND, inv.RemoveItem(9999, 1));
}

TEST_F(InventoryFixture, ClearInventory)
{
    inv.AddItem(1000, 10, 20);
    inv.AddItem(2000, 5, 20);
    inv.Clear();
    EXPECT_EQ(uint8_t(16), inv.GetFreeSlotCount());
    EXPECT_FALSE(inv.HasItem(1000));
    EXPECT_FALSE(inv.HasItem(2000));
}

TEST_F(InventoryFixture, SwapSlots)
{
    inv.AddItem(1000, 5, 20);
    inv.AddItem(2000, 3, 20);
    inv.SwapSlots(0, 1);
    EXPECT_EQ(uint32_t(2000), inv.GetSlot(0).itemId);
    EXPECT_EQ(uint32_t(1000), inv.GetSlot(1).itemId);
}

TEST_F(InventoryFixture, SwapInvalidSlot)
{
    EXPECT_FALSE(inv.SwapSlots(0, 99));
}

TEST_F(InventoryFixture, MultipleItemTypes)
{
    inv.AddItem(1000, 5, 20);
    inv.AddItem(2000, 10, 20);
    inv.AddItem(3000, 1, 1);
    EXPECT_EQ(uint32_t(5), inv.GetItemCount(1000));
    EXPECT_EQ(uint32_t(10), inv.GetItemCount(2000));
    EXPECT_EQ(uint32_t(1), inv.GetItemCount(3000));
    EXPECT_EQ(uint8_t(13), inv.GetFreeSlotCount());
}

// ============================================================================
// Sell Price Tests
// ============================================================================

TEST(SellPrice, PoorItem)
{
    // 1000 buy price, poor quality: 10%
    EXPECT_EQ(uint32_t(100), CalculateSellPrice(1000, ITEM_QUALITY_POOR));
}

TEST(SellPrice, NormalItem)
{
    EXPECT_EQ(uint32_t(200), CalculateSellPrice(1000, ITEM_QUALITY_NORMAL));
}

TEST(SellPrice, RareItem)
{
    EXPECT_EQ(uint32_t(250), CalculateSellPrice(1000, ITEM_QUALITY_RARE));
}

TEST(SellPrice, ZeroBuyPrice)
{
    EXPECT_EQ(uint32_t(0), CalculateSellPrice(0, ITEM_QUALITY_EPIC));
}

// ============================================================================
// Repair Cost Tests
// ============================================================================

TEST(RepairCost, FullDurabilityLoss)
{
    uint32_t cost = CalculateRepairCost(200, ITEM_QUALITY_EPIC, 1.0f);
    EXPECT_GT(cost, uint32_t(0));
}

TEST(RepairCost, NoDurabilityLoss)
{
    EXPECT_EQ(uint32_t(0), CalculateRepairCost(200, ITEM_QUALITY_EPIC, 0.0f));
}

TEST(RepairCost, HigherQualityCostsMore)
{
    uint32_t costRare = CalculateRepairCost(200, ITEM_QUALITY_RARE, 0.5f);
    uint32_t costEpic = CalculateRepairCost(200, ITEM_QUALITY_EPIC, 0.5f);
    EXPECT_GT(costEpic, costRare);
}

TEST(RepairCost, HigherIlevelCostsMore)
{
    uint32_t costLow = CalculateRepairCost(100, ITEM_QUALITY_RARE, 0.5f);
    uint32_t costHigh = CalculateRepairCost(200, ITEM_QUALITY_RARE, 0.5f);
    EXPECT_GT(costHigh, costLow);
}
