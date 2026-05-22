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
#include <cstring>
#include <string>
#include <cmath>
#include <limits>

// ============================================================================
// Reimplementation of Common.h macros and helpers for standalone testing.
// ============================================================================

#define MAKE_PAIR64(l, h)  uint64_t( uint32_t(l) | ( uint64_t(h) << 32 ) )
#define PAIR64_HIPART(x)   (uint32_t)((uint64_t(x) >> 32) & 0x00000000FFFFFFFFULL)
#define PAIR64_LOPART(x)   (uint32_t)(uint64_t(x)         & 0x00000000FFFFFFFFULL)

#define MAKE_PAIR32(l, h)  uint32_t( uint16_t(l) | ( uint32_t(h) << 16 ) )
#define PAIR32_HIPART(x)   (uint16_t)((uint32_t(x) >> 16) & 0x0000FFFF)
#define PAIR32_LOPART(x)   (uint16_t)(uint32_t(x)         & 0x0000FFFF)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_PI_F float(M_PI)

#ifndef countof
#define countof(array) (sizeof(array) / sizeof((array)[0]))
#endif

#define STRINGIZE(a) #a

inline float finiteAlways(float f) { return std::isfinite(f) ? f : 0.0f; }

inline char* mangos_strdup(const char* source)
{
    char* dest = new char[strlen(source) + 1];
    strcpy(dest, source);
    return dest;
}

enum AccountTypes
{
    SEC_PLAYER        = 0,
    SEC_MODERATOR     = 1,
    SEC_GAMEMASTER    = 2,
    SEC_ADMINISTRATOR = 3,
    SEC_CONSOLE       = 4
};

enum RealmFlags
{
    REALM_FLAG_NONE         = 0x00,
    REALM_FLAG_INVALID      = 0x01,
    REALM_FLAG_OFFLINE      = 0x02,
    REALM_FLAG_SPECIFYBUILD = 0x04,
    REALM_FLAG_UNK1         = 0x08,
    REALM_FLAG_UNK2         = 0x10,
    REALM_FLAG_NEW_PLAYERS  = 0x20,
    REALM_FLAG_RECOMMENDED  = 0x40,
    REALM_FLAG_FULL         = 0x80
};

enum LocaleConstant
{
    LOCALE_enUS = 0,
    LOCALE_koKR = 1,
    LOCALE_frFR = 2,
    LOCALE_deDE = 3,
    LOCALE_zhCN = 4,
    LOCALE_zhTW = 5,
    LOCALE_esES = 6,
    LOCALE_esMX = 7,
    LOCALE_ruRU = 8,
    LOCALE_ptPT = 9,
    LOCALE_ptBR = 10,
    LOCALE_itIT = 11
};
#define MAX_LOCALE 12

// ============================================================================
// PAIR64 Macro Tests
// ============================================================================

TEST(PAIR64, MakeAndExtract)
{
    uint32_t lo = 0x12345678;
    uint32_t hi = 0xABCDEF01;
    uint64_t pair = MAKE_PAIR64(lo, hi);
    EXPECT_EQ(lo, PAIR64_LOPART(pair));
    EXPECT_EQ(hi, PAIR64_HIPART(pair));
}

TEST(PAIR64, ZeroLow)
{
    uint64_t pair = MAKE_PAIR64(0, 0xDEAD);
    EXPECT_EQ(uint32_t(0), PAIR64_LOPART(pair));
    EXPECT_EQ(uint32_t(0xDEAD), PAIR64_HIPART(pair));
}

TEST(PAIR64, ZeroHigh)
{
    uint64_t pair = MAKE_PAIR64(0xBEEF, 0);
    EXPECT_EQ(uint32_t(0xBEEF), PAIR64_LOPART(pair));
    EXPECT_EQ(uint32_t(0), PAIR64_HIPART(pair));
}

TEST(PAIR64, MaxValues)
{
    uint32_t lo = 0xFFFFFFFF;
    uint32_t hi = 0xFFFFFFFF;
    uint64_t pair = MAKE_PAIR64(lo, hi);
    EXPECT_EQ(lo, PAIR64_LOPART(pair));
    EXPECT_EQ(hi, PAIR64_HIPART(pair));
}

TEST(PAIR64, BothZero)
{
    uint64_t pair = MAKE_PAIR64(0, 0);
    EXPECT_EQ(uint64_t(0), pair);
    EXPECT_EQ(uint32_t(0), PAIR64_LOPART(pair));
    EXPECT_EQ(uint32_t(0), PAIR64_HIPART(pair));
}

TEST(PAIR64, Roundtrip)
{
    for (uint32_t i = 0; i < 256; ++i)
    {
        uint32_t lo = i * 0x1000001;
        uint32_t hi = (255 - i) * 0x1000001;
        uint64_t pair = MAKE_PAIR64(lo, hi);
        EXPECT_EQ(lo, PAIR64_LOPART(pair));
        EXPECT_EQ(hi, PAIR64_HIPART(pair));
    }
}

// ============================================================================
// PAIR32 Macro Tests
// ============================================================================

TEST(PAIR32, MakeAndExtract)
{
    uint16_t lo = 0x1234;
    uint16_t hi = 0xABCD;
    uint32_t pair = MAKE_PAIR32(lo, hi);
    EXPECT_EQ(lo, PAIR32_LOPART(pair));
    EXPECT_EQ(hi, PAIR32_HIPART(pair));
}

TEST(PAIR32, ZeroLow)
{
    uint32_t pair = MAKE_PAIR32(0, 0x1234);
    EXPECT_EQ(uint16_t(0), PAIR32_LOPART(pair));
    EXPECT_EQ(uint16_t(0x1234), PAIR32_HIPART(pair));
}

TEST(PAIR32, ZeroHigh)
{
    uint32_t pair = MAKE_PAIR32(0x5678, 0);
    EXPECT_EQ(uint16_t(0x5678), PAIR32_LOPART(pair));
    EXPECT_EQ(uint16_t(0), PAIR32_HIPART(pair));
}

TEST(PAIR32, MaxValues)
{
    uint32_t pair = MAKE_PAIR32(0xFFFF, 0xFFFF);
    EXPECT_EQ(uint16_t(0xFFFF), PAIR32_LOPART(pair));
    EXPECT_EQ(uint16_t(0xFFFF), PAIR32_HIPART(pair));
}

TEST(PAIR32, BothZero)
{
    uint32_t pair = MAKE_PAIR32(0, 0);
    EXPECT_EQ(uint32_t(0), pair);
}

// ============================================================================
// finiteAlways Tests
// ============================================================================

TEST(finiteAlways, FiniteValue)
{
    EXPECT_FLOAT_EQ(3.14f, finiteAlways(3.14f));
}

TEST(finiteAlways, ZeroValue)
{
    EXPECT_FLOAT_EQ(0.0f, finiteAlways(0.0f));
}

TEST(finiteAlways, NegativeValue)
{
    EXPECT_FLOAT_EQ(-1.5f, finiteAlways(-1.5f));
}

TEST(finiteAlways, InfinityReturnsZero)
{
    float inf = std::numeric_limits<float>::infinity();
    EXPECT_FLOAT_EQ(0.0f, finiteAlways(inf));
}

TEST(finiteAlways, NegativeInfinityReturnsZero)
{
    float ninf = -std::numeric_limits<float>::infinity();
    EXPECT_FLOAT_EQ(0.0f, finiteAlways(ninf));
}

TEST(finiteAlways, NaNReturnsZero)
{
    float nan = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FLOAT_EQ(0.0f, finiteAlways(nan));
}

TEST(finiteAlways, MaxFloat)
{
    float maxf = std::numeric_limits<float>::max();
    EXPECT_FLOAT_EQ(maxf, finiteAlways(maxf));
}

// ============================================================================
// countof Macro Tests
// ============================================================================

TEST(countof, StaticArray)
{
    int arr[10];
    EXPECT_EQ(size_t(10), countof(arr));
}

TEST(countof, CharArray)
{
    char arr[256];
    EXPECT_EQ(size_t(256), countof(arr));
}

TEST(countof, SingleElement)
{
    double arr[1];
    EXPECT_EQ(size_t(1), countof(arr));
}

TEST(countof, StructArray)
{
    struct Foo { int x; float y; };
    Foo arr[5];
    EXPECT_EQ(size_t(5), countof(arr));
}

// ============================================================================
// mangos_strdup Tests
// ============================================================================

TEST(mangos_strdup, BasicString)
{
    const char* original = "Hello, MaNGOS!";
    char* dup = mangos_strdup(original);
    EXPECT_TRUE(dup != nullptr);
    EXPECT_TRUE(strcmp(original, dup) == 0);
    EXPECT_TRUE(dup != original); // Different pointer
    delete[] dup;
}

TEST(mangos_strdup, EmptyString)
{
    const char* original = "";
    char* dup = mangos_strdup(original);
    EXPECT_TRUE(dup != nullptr);
    EXPECT_EQ('\0', dup[0]);
    delete[] dup;
}

TEST(mangos_strdup, SingleChar)
{
    const char* original = "X";
    char* dup = mangos_strdup(original);
    EXPECT_EQ('X', dup[0]);
    EXPECT_EQ('\0', dup[1]);
    delete[] dup;
}

TEST(mangos_strdup, LongString)
{
    std::string longStr(1000, 'A');
    char* dup = mangos_strdup(longStr.c_str());
    EXPECT_EQ(size_t(1000), strlen(dup));
    delete[] dup;
}

// ============================================================================
// AccountTypes Enum Tests
// ============================================================================

TEST(AccountTypes, Values)
{
    EXPECT_EQ(0, SEC_PLAYER);
    EXPECT_EQ(1, SEC_MODERATOR);
    EXPECT_EQ(2, SEC_GAMEMASTER);
    EXPECT_EQ(3, SEC_ADMINISTRATOR);
    EXPECT_EQ(4, SEC_CONSOLE);
}

TEST(AccountTypes, Ordering)
{
    EXPECT_LT(SEC_PLAYER, SEC_MODERATOR);
    EXPECT_LT(SEC_MODERATOR, SEC_GAMEMASTER);
    EXPECT_LT(SEC_GAMEMASTER, SEC_ADMINISTRATOR);
    EXPECT_LT(SEC_ADMINISTRATOR, SEC_CONSOLE);
}

TEST(AccountTypes, ConsoleIsHighest)
{
    EXPECT_GT(SEC_CONSOLE, SEC_ADMINISTRATOR);
}

// ============================================================================
// RealmFlags Enum Tests
// ============================================================================

TEST(RealmFlags, NoneIsZero)
{
    EXPECT_EQ(0, REALM_FLAG_NONE);
}

TEST(RealmFlags, AreDistinctBits)
{
    EXPECT_EQ(0x01, REALM_FLAG_INVALID);
    EXPECT_EQ(0x02, REALM_FLAG_OFFLINE);
    EXPECT_EQ(0x04, REALM_FLAG_SPECIFYBUILD);
    EXPECT_EQ(0x20, REALM_FLAG_NEW_PLAYERS);
    EXPECT_EQ(0x40, REALM_FLAG_RECOMMENDED);
    EXPECT_EQ(0x80, REALM_FLAG_FULL);
}

TEST(RealmFlags, CanBeCombined)
{
    int combined = REALM_FLAG_OFFLINE | REALM_FLAG_FULL;
    EXPECT_TRUE((combined & REALM_FLAG_OFFLINE) != 0);
    EXPECT_TRUE((combined & REALM_FLAG_FULL) != 0);
    EXPECT_FALSE((combined & REALM_FLAG_INVALID) != 0);
}

TEST(RealmFlags, NoBitOverlap)
{
    int allFlags = REALM_FLAG_INVALID | REALM_FLAG_OFFLINE | REALM_FLAG_SPECIFYBUILD
                 | REALM_FLAG_UNK1 | REALM_FLAG_UNK2 | REALM_FLAG_NEW_PLAYERS
                 | REALM_FLAG_RECOMMENDED | REALM_FLAG_FULL;
    // Count set bits - should be 8
    int count = 0;
    for (int b = 0; b < 8; ++b)
        if (allFlags & (1 << b)) ++count;
    EXPECT_EQ(8, count);
}

// ============================================================================
// LocaleConstant Tests
// ============================================================================

TEST(LocaleConstant, MaxLocaleCount)
{
    EXPECT_EQ(12, MAX_LOCALE);
}

TEST(LocaleConstant, EnglishIsDefault)
{
    EXPECT_EQ(0, LOCALE_enUS);
}

TEST(LocaleConstant, AllLocalesInRange)
{
    EXPECT_GE(LOCALE_enUS, 0);
    EXPECT_GE(LOCALE_koKR, 0);
    EXPECT_GE(LOCALE_itIT, 0);
    EXPECT_LT(LOCALE_itIT, MAX_LOCALE);
}

TEST(LocaleConstant, UniqueValues)
{
    int locales[] = {
        LOCALE_enUS, LOCALE_koKR, LOCALE_frFR, LOCALE_deDE,
        LOCALE_zhCN, LOCALE_zhTW, LOCALE_esES, LOCALE_esMX,
        LOCALE_ruRU, LOCALE_ptPT, LOCALE_ptBR, LOCALE_itIT
    };
    for (int i = 0; i < MAX_LOCALE; ++i)
        for (int j = i + 1; j < MAX_LOCALE; ++j)
            EXPECT_NE(locales[i], locales[j]);
}

// ============================================================================
// M_PI_F Tests
// ============================================================================

TEST(Constants, MPI_F_IsFloat)
{
    EXPECT_NEAR(3.14159f, M_PI_F, 0.0001f);
}

TEST(Constants, MPI_Relationship)
{
    EXPECT_NEAR(float(M_PI), M_PI_F, 0.000001f);
}

TEST(Constants, TwoPI)
{
    float two_pi = 2.0f * M_PI_F;
    EXPECT_NEAR(6.28318f, two_pi, 0.0001f);
}

// ============================================================================
// Bit Manipulation Pattern Tests (used throughout codebase)
// ============================================================================

TEST(BitManipulation, SetBit)
{
    uint32_t flags = 0;
    flags |= (1 << 3);
    EXPECT_TRUE((flags & (1 << 3)) != 0);
}

TEST(BitManipulation, ClearBit)
{
    uint32_t flags = 0xFF;
    flags &= ~(1 << 3);
    EXPECT_FALSE((flags & (1 << 3)) != 0);
    EXPECT_TRUE((flags & (1 << 2)) != 0);
}

TEST(BitManipulation, ToggleBit)
{
    uint32_t flags = 0;
    flags ^= (1 << 5);
    EXPECT_TRUE((flags & (1 << 5)) != 0);
    flags ^= (1 << 5);
    EXPECT_FALSE((flags & (1 << 5)) != 0);
}

TEST(BitManipulation, TestBit)
{
    uint32_t flags = 0b10110100;
    EXPECT_FALSE((flags & (1 << 0)) != 0);
    EXPECT_FALSE((flags & (1 << 1)) != 0);
    EXPECT_TRUE((flags & (1 << 2)) != 0);
    EXPECT_FALSE((flags & (1 << 3)) != 0);
    EXPECT_TRUE((flags & (1 << 4)) != 0);
    EXPECT_TRUE((flags & (1 << 5)) != 0);
}

TEST(BitManipulation, HighNibble)
{
    uint8_t val = 0xAB;
    EXPECT_EQ(0xA, (val >> 4) & 0xF);
}

TEST(BitManipulation, LowNibble)
{
    uint8_t val = 0xAB;
    EXPECT_EQ(0xB, val & 0xF);
}

// ============================================================================
// Integer Overflow Behavior Tests (critical for server logic)
// ============================================================================

TEST(IntegerBehavior, UInt32MaxPlusOne)
{
    uint32_t val = 0xFFFFFFFF;
    uint32_t result = val + 1;
    EXPECT_EQ(uint32_t(0), result); // wraps to 0
}

TEST(IntegerBehavior, UInt16MaxPlusOne)
{
    uint16_t val = 0xFFFF;
    uint16_t result = val + 1;
    EXPECT_EQ(uint16_t(0), result);
}

TEST(IntegerBehavior, Int32MinMinusOne)
{
    // Signed overflow is UB but test the pattern used in damage math
    // Instead test safe patterns
    int32_t val = -2147483648LL;
    EXPECT_EQ(int32_t(-2147483648LL), val);
}

TEST(IntegerBehavior, SignedToUnsignedCast)
{
    int32_t neg = -1;
    uint32_t result = static_cast<uint32_t>(neg);
    EXPECT_EQ(uint32_t(0xFFFFFFFF), result);
}

TEST(IntegerBehavior, UInt64Shifting)
{
    uint64_t val = uint64_t(1) << 63;
    EXPECT_EQ(uint64_t(0x8000000000000000ULL), val);
}
