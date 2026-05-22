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
#include <set>
#include <vector>
#include <list>

// ============================================================================
// Standalone reimplementation of ObjectGuid and related enums for testing.
// Mirrors src/game/Object/ObjectGuid.h
// ============================================================================

enum TypeID
{
    TYPEID_OBJECT        = 0,
    TYPEID_ITEM          = 1,
    TYPEID_CONTAINER     = 2,
    TYPEID_UNIT          = 3,
    TYPEID_PLAYER        = 4,
    TYPEID_GAMEOBJECT    = 5,
    TYPEID_DYNAMICOBJECT = 6,
    TYPEID_CORPSE        = 7
};

enum TypeMask
{
    TYPEMASK_OBJECT         = 0x0001,
    TYPEMASK_ITEM           = 0x0002,
    TYPEMASK_CONTAINER      = 0x0004,
    TYPEMASK_UNIT           = 0x0008,
    TYPEMASK_PLAYER         = 0x0010,
    TYPEMASK_GAMEOBJECT     = 0x0020,
    TYPEMASK_DYNAMICOBJECT  = 0x0040,
    TYPEMASK_CORPSE         = 0x0080,

    TYPEMASK_CREATURE_OR_GAMEOBJECT = TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT,
    TYPEMASK_CREATURE_GAMEOBJECT_OR_ITEM = TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM,
    TYPEMASK_CREATURE_GAMEOBJECT_PLAYER_OR_ITEM = TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM | TYPEMASK_PLAYER,
    TYPEMASK_WORLDOBJECT = TYPEMASK_UNIT | TYPEMASK_PLAYER | TYPEMASK_GAMEOBJECT | TYPEMASK_DYNAMICOBJECT | TYPEMASK_CORPSE,
};

enum HighGuid
{
    HIGHGUID_ITEM           = 0x470,
    HIGHGUID_CONTAINER      = 0x470,
    HIGHGUID_PLAYER         = 0x000,
    HIGHGUID_GAMEOBJECT     = 0xF11,
    HIGHGUID_TRANSPORT      = 0xF12,
    HIGHGUID_UNIT           = 0xF13,
    HIGHGUID_PET            = 0xF14,
    HIGHGUID_VEHICLE        = 0xF15,
    HIGHGUID_DYNAMICOBJECT  = 0xF10,
    HIGHGUID_CORPSE         = 0xF50,
    HIGHGUID_BATTLEGROUND   = 0x1F1,
    HIGHGUID_MO_TRANSPORT   = 0x1FC,
    HIGHGUID_INSTANCE       = 0x1F4,
    HIGHGUID_GROUP          = 0x1F5,
    HIGHGUID_GUILD          = 0x1FF7,
};

class ObjectGuid
{
public:
    ObjectGuid() : m_guid(0) {}
    explicit ObjectGuid(uint64_t guid) : m_guid(guid) {}

    ObjectGuid(HighGuid hi, uint32_t entry, uint32_t counter)
        : m_guid(counter ? uint64_t(counter) | (uint64_t(entry) << 32) | (uint64_t(hi) << (IsLargeHigh(hi) ? 48 : 52)) : 0) {}

    ObjectGuid(HighGuid hi, uint32_t counter)
        : m_guid(counter ? uint64_t(counter) | (uint64_t(hi) << (IsLargeHigh(hi) ? 48 : 52)) : 0) {}

    operator uint64_t() const { return m_guid; }

    void Set(uint64_t guid) { m_guid = guid; }
    void Clear() { m_guid = 0; }

    uint64_t GetRawValue() const { return m_guid; }

    HighGuid GetHigh() const
    {
        HighGuid high = HighGuid((m_guid >> 48) & 0xFFFF);
        return HighGuid(IsLargeHigh(high) ? high : (m_guid >> 52) & 0xFFF);
    }

    uint32_t GetEntry() const { return HasEntry() ? uint32_t((m_guid >> 32) & uint64_t(0xFFFF)) : 0; }

    uint32_t GetCounter() const
    {
        return uint32_t(m_guid & uint64_t(0x00000000FFFFFFFF));
    }

    static uint32_t GetMaxCounter(HighGuid /*high*/)
    {
        return uint32_t(0xFFFFFFFF);
    }

    uint32_t GetMaxCounter() const { return GetMaxCounter(GetHigh()); }

    bool IsEmpty()             const { return m_guid == 0; }
    bool IsCreature()          const { return GetHigh() == HIGHGUID_UNIT; }
    bool IsPet()               const { return GetHigh() == HIGHGUID_PET; }
    bool IsVehicle()           const { return GetHigh() == HIGHGUID_VEHICLE; }
    bool IsCreatureOrPet()     const { return IsCreature() || IsPet(); }
    bool IsCreatureOrVehicle() const { return IsCreature() || IsVehicle(); }
    bool IsAnyTypeCreature()   const { return IsCreature() || IsPet() || IsVehicle(); }
    bool IsPlayer()            const { return !IsEmpty() && GetHigh() == HIGHGUID_PLAYER; }
    bool IsUnit()              const { return IsAnyTypeCreature() || IsPlayer(); }
    bool IsItem()              const { return GetHigh() == HIGHGUID_ITEM; }
    bool IsGameObject()        const { return GetHigh() == HIGHGUID_GAMEOBJECT; }
    bool IsDynamicObject()     const { return GetHigh() == HIGHGUID_DYNAMICOBJECT; }
    bool IsCorpse()            const { return GetHigh() == HIGHGUID_CORPSE; }
    bool IsTransport()         const { return GetHigh() == HIGHGUID_TRANSPORT; }
    bool IsMOTransport()       const { return GetHigh() == HIGHGUID_MO_TRANSPORT; }
    bool IsInstance()          const { return GetHigh() == HIGHGUID_INSTANCE; }
    bool IsGroup()             const { return GetHigh() == HIGHGUID_GROUP; }
    bool IsBattleGround()      const { return GetHigh() == HIGHGUID_BATTLEGROUND; }
    bool IsGuild()             const { return GetHigh() == HIGHGUID_GUILD; }

    static TypeID GetTypeId(HighGuid high)
    {
        switch (high)
        {
            case HIGHGUID_ITEM:         return TYPEID_ITEM;
            case HIGHGUID_UNIT:         return TYPEID_UNIT;
            case HIGHGUID_PET:          return TYPEID_UNIT;
            case HIGHGUID_PLAYER:       return TYPEID_PLAYER;
            case HIGHGUID_GAMEOBJECT:   return TYPEID_GAMEOBJECT;
            case HIGHGUID_DYNAMICOBJECT: return TYPEID_DYNAMICOBJECT;
            case HIGHGUID_CORPSE:       return TYPEID_CORPSE;
            case HIGHGUID_MO_TRANSPORT: return TYPEID_GAMEOBJECT;
            case HIGHGUID_VEHICLE:      return TYPEID_UNIT;
            case HIGHGUID_INSTANCE:
            case HIGHGUID_GROUP:
            default:                    return TYPEID_OBJECT;
        }
    }

    TypeID GetTypeId() const { return GetTypeId(GetHigh()); }

    bool operator!() const { return IsEmpty(); }
    bool operator==(ObjectGuid const& guid) const { return GetRawValue() == guid.GetRawValue(); }
    bool operator!=(ObjectGuid const& guid) const { return GetRawValue() != guid.GetRawValue(); }
    bool operator<(ObjectGuid const& guid) const { return GetRawValue() < guid.GetRawValue(); }

private:
    static bool HasEntry(HighGuid high)
    {
        switch (high)
        {
            case HIGHGUID_ITEM:
            case HIGHGUID_PLAYER:
            case HIGHGUID_DYNAMICOBJECT:
            case HIGHGUID_CORPSE:
            case HIGHGUID_MO_TRANSPORT:
            case HIGHGUID_INSTANCE:
            case HIGHGUID_GROUP:
                return false;
            case HIGHGUID_GAMEOBJECT:
            case HIGHGUID_TRANSPORT:
            case HIGHGUID_UNIT:
            case HIGHGUID_PET:
            case HIGHGUID_VEHICLE:
            case HIGHGUID_BATTLEGROUND:
            default:
                return true;
        }
    }

    bool HasEntry() const { return HasEntry(GetHigh()); }

    static bool IsLargeHigh(HighGuid high)
    {
        switch (high)
        {
            case HIGHGUID_GUILD:
                return true;
            default:
                return false;
        }
    }

    bool IsLargeHigh() const { return IsLargeHigh(GetHigh()); }

    uint64_t m_guid;
};

template<HighGuid high>
class ObjectGuidGenerator
{
public:
    explicit ObjectGuidGenerator(uint32_t start = 1) : m_nextGuid(start) {}
    void Set(uint32_t val) { m_nextGuid = val; }
    uint32_t Generate() { return m_nextGuid++; }
    uint32_t GetNextAfterMaxUsed() const { return m_nextGuid; }

private:
    uint32_t m_nextGuid;
};

typedef std::set<ObjectGuid> GuidSet;
typedef std::list<ObjectGuid> GuidList;
typedef std::vector<ObjectGuid> GuidVector;

// ============================================================================
// Tests
// ============================================================================

// --- Construction ---

TEST(ObjectGuid, DefaultConstructor)
{
    ObjectGuid guid;
    EXPECT_TRUE(guid.IsEmpty());
    EXPECT_EQ(uint64_t(0), guid.GetRawValue());
}

TEST(ObjectGuid, ExplicitUint64Constructor)
{
    ObjectGuid guid(uint64_t(12345));
    EXPECT_FALSE(guid.IsEmpty());
    EXPECT_EQ(uint64_t(12345), guid.GetRawValue());
}

TEST(ObjectGuid, HighGuidEntryCounterConstructor)
{
    ObjectGuid guid(HIGHGUID_UNIT, uint32_t(100), uint32_t(1));
    EXPECT_FALSE(guid.IsEmpty());
    EXPECT_TRUE(guid.IsCreature());
    EXPECT_EQ(uint32_t(100), guid.GetEntry());
    EXPECT_EQ(uint32_t(1), guid.GetCounter());
}

TEST(ObjectGuid, HighGuidCounterConstructor)
{
    ObjectGuid guid(HIGHGUID_PLAYER, uint32_t(42));
    EXPECT_FALSE(guid.IsEmpty());
    EXPECT_TRUE(guid.IsPlayer());
    EXPECT_EQ(uint32_t(42), guid.GetCounter());
}

TEST(ObjectGuid, ZeroCounterConstructsEmpty)
{
    ObjectGuid guid(HIGHGUID_UNIT, uint32_t(100), uint32_t(0));
    EXPECT_TRUE(guid.IsEmpty());
}

TEST(ObjectGuid, ZeroCounterNoEntryConstructsEmpty)
{
    ObjectGuid guid(HIGHGUID_PLAYER, uint32_t(0));
    EXPECT_TRUE(guid.IsEmpty());
}

// --- Type Identification ---

TEST(ObjectGuid, IsCreature)
{
    ObjectGuid guid(HIGHGUID_UNIT, uint32_t(1234), uint32_t(1));
    EXPECT_TRUE(guid.IsCreature());
    EXPECT_FALSE(guid.IsPlayer());
    EXPECT_FALSE(guid.IsPet());
    EXPECT_FALSE(guid.IsGameObject());
    EXPECT_TRUE(guid.IsAnyTypeCreature());
    EXPECT_TRUE(guid.IsUnit());
}

TEST(ObjectGuid, IsPet)
{
    ObjectGuid guid(HIGHGUID_PET, uint32_t(5678), uint32_t(1));
    EXPECT_TRUE(guid.IsPet());
    EXPECT_FALSE(guid.IsCreature());
    EXPECT_TRUE(guid.IsCreatureOrPet());
    EXPECT_TRUE(guid.IsAnyTypeCreature());
    EXPECT_TRUE(guid.IsUnit());
}

TEST(ObjectGuid, IsVehicle)
{
    ObjectGuid guid(HIGHGUID_VEHICLE, uint32_t(90), uint32_t(1));
    EXPECT_TRUE(guid.IsVehicle());
    EXPECT_FALSE(guid.IsCreature());
    EXPECT_TRUE(guid.IsCreatureOrVehicle());
    EXPECT_TRUE(guid.IsAnyTypeCreature());
    EXPECT_TRUE(guid.IsUnit());
}

TEST(ObjectGuid, IsPlayer)
{
    ObjectGuid guid(HIGHGUID_PLAYER, uint32_t(1));
    EXPECT_TRUE(guid.IsPlayer());
    EXPECT_TRUE(guid.IsUnit());
    EXPECT_FALSE(guid.IsCreature());
    EXPECT_FALSE(guid.IsAnyTypeCreature());
}

TEST(ObjectGuid, IsItem)
{
    ObjectGuid guid(HIGHGUID_ITEM, uint32_t(1));
    EXPECT_TRUE(guid.IsItem());
    EXPECT_FALSE(guid.IsUnit());
    EXPECT_FALSE(guid.IsGameObject());
}

TEST(ObjectGuid, IsGameObject)
{
    ObjectGuid guid(HIGHGUID_GAMEOBJECT, uint32_t(100), uint32_t(1));
    EXPECT_TRUE(guid.IsGameObject());
    EXPECT_FALSE(guid.IsCreature());
    EXPECT_FALSE(guid.IsUnit());
}

TEST(ObjectGuid, IsDynamicObject)
{
    ObjectGuid guid(HIGHGUID_DYNAMICOBJECT, uint32_t(1));
    EXPECT_TRUE(guid.IsDynamicObject());
}

TEST(ObjectGuid, IsCorpse)
{
    ObjectGuid guid(HIGHGUID_CORPSE, uint32_t(1));
    EXPECT_TRUE(guid.IsCorpse());
}

TEST(ObjectGuid, IsTransport)
{
    ObjectGuid guid(HIGHGUID_TRANSPORT, uint32_t(1), uint32_t(1));
    EXPECT_TRUE(guid.IsTransport());
}

TEST(ObjectGuid, IsMOTransport)
{
    ObjectGuid guid(HIGHGUID_MO_TRANSPORT, uint32_t(1));
    EXPECT_TRUE(guid.IsMOTransport());
}

TEST(ObjectGuid, IsInstance)
{
    ObjectGuid guid(HIGHGUID_INSTANCE, uint32_t(1));
    EXPECT_TRUE(guid.IsInstance());
}

TEST(ObjectGuid, IsGroup)
{
    ObjectGuid guid(HIGHGUID_GROUP, uint32_t(1));
    EXPECT_TRUE(guid.IsGroup());
}

TEST(ObjectGuid, IsBattleGround)
{
    ObjectGuid guid(HIGHGUID_BATTLEGROUND, uint32_t(1), uint32_t(1));
    EXPECT_TRUE(guid.IsBattleGround());
}

TEST(ObjectGuid, IsGuild)
{
    ObjectGuid guid(HIGHGUID_GUILD, uint32_t(1));
    EXPECT_TRUE(guid.IsGuild());
}

// --- TypeID Mapping ---

TEST(ObjectGuid, GetTypeIdUnit)
{
    EXPECT_EQ(TYPEID_UNIT, ObjectGuid::GetTypeId(HIGHGUID_UNIT));
    EXPECT_EQ(TYPEID_UNIT, ObjectGuid::GetTypeId(HIGHGUID_PET));
    EXPECT_EQ(TYPEID_UNIT, ObjectGuid::GetTypeId(HIGHGUID_VEHICLE));
}

TEST(ObjectGuid, GetTypeIdPlayer)
{
    EXPECT_EQ(TYPEID_PLAYER, ObjectGuid::GetTypeId(HIGHGUID_PLAYER));
}

TEST(ObjectGuid, GetTypeIdItem)
{
    EXPECT_EQ(TYPEID_ITEM, ObjectGuid::GetTypeId(HIGHGUID_ITEM));
}

TEST(ObjectGuid, GetTypeIdGameObject)
{
    EXPECT_EQ(TYPEID_GAMEOBJECT, ObjectGuid::GetTypeId(HIGHGUID_GAMEOBJECT));
    EXPECT_EQ(TYPEID_GAMEOBJECT, ObjectGuid::GetTypeId(HIGHGUID_MO_TRANSPORT));
}

TEST(ObjectGuid, GetTypeIdDynamicObject)
{
    EXPECT_EQ(TYPEID_DYNAMICOBJECT, ObjectGuid::GetTypeId(HIGHGUID_DYNAMICOBJECT));
}

TEST(ObjectGuid, GetTypeIdCorpse)
{
    EXPECT_EQ(TYPEID_CORPSE, ObjectGuid::GetTypeId(HIGHGUID_CORPSE));
}

TEST(ObjectGuid, GetTypeIdInstance)
{
    EXPECT_EQ(TYPEID_OBJECT, ObjectGuid::GetTypeId(HIGHGUID_INSTANCE));
}

TEST(ObjectGuid, GetTypeIdGroup)
{
    EXPECT_EQ(TYPEID_OBJECT, ObjectGuid::GetTypeId(HIGHGUID_GROUP));
}

// --- Comparison Operators ---

TEST(ObjectGuid, Equality)
{
    ObjectGuid a(uint64_t(100));
    ObjectGuid b(uint64_t(100));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(ObjectGuid, Inequality)
{
    ObjectGuid a(uint64_t(100));
    ObjectGuid b(uint64_t(200));
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a == b);
}

TEST(ObjectGuid, LessThan)
{
    ObjectGuid a(uint64_t(100));
    ObjectGuid b(uint64_t(200));
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(ObjectGuid, NotOperator)
{
    ObjectGuid empty;
    EXPECT_TRUE(!empty);
    ObjectGuid nonEmpty(uint64_t(1));
    EXPECT_FALSE(!nonEmpty);
}

// --- Modification ---

TEST(ObjectGuid, Set)
{
    ObjectGuid guid;
    EXPECT_TRUE(guid.IsEmpty());
    guid.Set(42);
    EXPECT_FALSE(guid.IsEmpty());
    EXPECT_EQ(uint64_t(42), guid.GetRawValue());
}

TEST(ObjectGuid, ClearGuid)
{
    ObjectGuid guid(uint64_t(42));
    EXPECT_FALSE(guid.IsEmpty());
    guid.Clear();
    EXPECT_TRUE(guid.IsEmpty());
    EXPECT_EQ(uint64_t(0), guid.GetRawValue());
}

// --- Entry/Counter Access ---

TEST(ObjectGuid, EntryForCreature)
{
    uint32_t entry = 12345;
    uint32_t counter = 67890;
    ObjectGuid guid(HIGHGUID_UNIT, entry, counter);
    EXPECT_EQ(entry, guid.GetEntry());
    EXPECT_EQ(counter, guid.GetCounter());
}

TEST(ObjectGuid, NoEntryForPlayer)
{
    ObjectGuid guid(HIGHGUID_PLAYER, uint32_t(42));
    EXPECT_EQ(uint32_t(0), guid.GetEntry());
    EXPECT_EQ(uint32_t(42), guid.GetCounter());
}

TEST(ObjectGuid, NoEntryForItem)
{
    ObjectGuid guid(HIGHGUID_ITEM, uint32_t(100));
    EXPECT_EQ(uint32_t(0), guid.GetEntry());
    EXPECT_EQ(uint32_t(100), guid.GetCounter());
}

TEST(ObjectGuid, EntryForGameObject)
{
    uint32_t entry = 999;
    uint32_t counter = 555;
    ObjectGuid guid(HIGHGUID_GAMEOBJECT, entry, counter);
    EXPECT_EQ(entry, guid.GetEntry());
    EXPECT_EQ(counter, guid.GetCounter());
}

TEST(ObjectGuid, EntryForPet)
{
    uint32_t entry = 777;
    uint32_t counter = 333;
    ObjectGuid guid(HIGHGUID_PET, entry, counter);
    EXPECT_EQ(entry, guid.GetEntry());
    EXPECT_EQ(counter, guid.GetCounter());
}

// --- Container Types ---

TEST(ObjectGuid, GuidSet)
{
    GuidSet s;
    s.insert(ObjectGuid(uint64_t(1)));
    s.insert(ObjectGuid(uint64_t(2)));
    s.insert(ObjectGuid(uint64_t(1)));
    EXPECT_EQ(size_t(2), s.size());
}

TEST(ObjectGuid, GuidVector)
{
    GuidVector v;
    v.push_back(ObjectGuid(uint64_t(1)));
    v.push_back(ObjectGuid(uint64_t(2)));
    v.push_back(ObjectGuid(uint64_t(3)));
    EXPECT_EQ(size_t(3), v.size());
    EXPECT_EQ(uint64_t(2), v[1].GetRawValue());
}

TEST(ObjectGuid, GuidList)
{
    GuidList l;
    l.push_back(ObjectGuid(uint64_t(10)));
    l.push_back(ObjectGuid(uint64_t(20)));
    EXPECT_EQ(size_t(2), l.size());
    EXPECT_EQ(uint64_t(10), l.front().GetRawValue());
    EXPECT_EQ(uint64_t(20), l.back().GetRawValue());
}

// --- Guid Generator ---

TEST(ObjectGuid, GeneratorSequential)
{
    ObjectGuidGenerator<HIGHGUID_UNIT> gen;
    EXPECT_EQ(uint32_t(1), gen.Generate());
    EXPECT_EQ(uint32_t(2), gen.Generate());
    EXPECT_EQ(uint32_t(3), gen.Generate());
    EXPECT_EQ(uint32_t(4), gen.GetNextAfterMaxUsed());
}

TEST(ObjectGuid, GeneratorCustomStart)
{
    ObjectGuidGenerator<HIGHGUID_PLAYER> gen(100);
    EXPECT_EQ(uint32_t(100), gen.Generate());
    EXPECT_EQ(uint32_t(101), gen.Generate());
}

TEST(ObjectGuid, GeneratorSet)
{
    ObjectGuidGenerator<HIGHGUID_ITEM> gen;
    gen.Set(500);
    EXPECT_EQ(uint32_t(500), gen.Generate());
}

// --- MaxCounter ---

TEST(ObjectGuid, MaxCounterForUnit)
{
    EXPECT_EQ(uint32_t(0xFFFFFFFF), ObjectGuid::GetMaxCounter(HIGHGUID_UNIT));
}

TEST(ObjectGuid, MaxCounterForPlayer)
{
    EXPECT_EQ(uint32_t(0xFFFFFFFF), ObjectGuid::GetMaxCounter(HIGHGUID_PLAYER));
}

// --- TypeMask Combinations ---

TEST(ObjectGuid, TypeMaskCreatureOrGameObject)
{
    EXPECT_EQ(TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT, TYPEMASK_CREATURE_OR_GAMEOBJECT);
}

TEST(ObjectGuid, TypeMaskCreatureGameObjectOrItem)
{
    EXPECT_EQ(TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM,
              TYPEMASK_CREATURE_GAMEOBJECT_OR_ITEM);
}

TEST(ObjectGuid, TypeMaskWorldObject)
{
    EXPECT_EQ(TYPEMASK_UNIT | TYPEMASK_PLAYER | TYPEMASK_GAMEOBJECT |
              TYPEMASK_DYNAMICOBJECT | TYPEMASK_CORPSE, TYPEMASK_WORLDOBJECT);
}

// --- Uint64 Implicit Conversion ---

TEST(ObjectGuid, ImplicitUint64Conversion)
{
    ObjectGuid guid(uint64_t(999));
    uint64_t val = guid;
    EXPECT_EQ(uint64_t(999), val);
}

// --- Guild (Large High GUID) ---

TEST(ObjectGuid, GuildGuid)
{
    ObjectGuid guid(HIGHGUID_GUILD, uint32_t(42));
    EXPECT_TRUE(guid.IsGuild());
    EXPECT_EQ(uint32_t(42), guid.GetCounter());
    EXPECT_EQ(TYPEID_OBJECT, guid.GetTypeId());
}
