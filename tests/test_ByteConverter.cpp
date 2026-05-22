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
#include <algorithm>
#include <cstring>

// ============================================================================
// Reimplementation of ByteConverter for standalone testing.
// Mirrors src/shared/Utilities/ByteConverter.h
// ============================================================================

namespace ByteConverter
{
    template<size_t T>
    inline void convert(char* val)
    {
        std::swap(*val, *(val + T - 1));
        convert<T - 2>(val + 1);
    }

    template<> inline void convert<0>(char*) {}
    template<> inline void convert<1>(char*) {}

    template<typename T>
    inline void apply(T* val)
    {
        convert<sizeof(T)>((char*)(val));
    }
}

// Utility to swap bytes of a value
template<typename T>
T ByteSwap(T val)
{
    ByteConverter::apply<T>(&val);
    return val;
}

// ============================================================================
// ByteConverter Tests
// ============================================================================

TEST(ByteConverter, Uint8NoChange)
{
    uint8_t val = 0xAB;
    ByteConverter::apply<uint8_t>(&val);
    EXPECT_EQ(uint8_t(0xAB), val); // No change for 1 byte
}

TEST(ByteConverter, Uint16Swap)
{
    uint16_t val = 0x1234;
    ByteConverter::apply<uint16_t>(&val);
    EXPECT_EQ(uint16_t(0x3412), val);
}

TEST(ByteConverter, Uint32Swap)
{
    uint32_t val = 0x12345678;
    ByteConverter::apply<uint32_t>(&val);
    EXPECT_EQ(uint32_t(0x78563412), val);
}

TEST(ByteConverter, Uint64Swap)
{
    uint64_t val = 0x0102030405060708ULL;
    ByteConverter::apply<uint64_t>(&val);
    EXPECT_EQ(uint64_t(0x0807060504030201ULL), val);
}

TEST(ByteConverter, DoubleSwapRestores)
{
    uint32_t original = 0xDEADBEEF;
    uint32_t val = original;
    ByteConverter::apply<uint32_t>(&val);
    ByteConverter::apply<uint32_t>(&val);
    EXPECT_EQ(original, val);
}

TEST(ByteConverter, Uint16DoubleSwap)
{
    uint16_t original = 0xABCD;
    uint16_t val = original;
    ByteConverter::apply<uint16_t>(&val);
    ByteConverter::apply<uint16_t>(&val);
    EXPECT_EQ(original, val);
}

TEST(ByteConverter, Uint64DoubleSwap)
{
    uint64_t original = 0x123456789ABCDEF0ULL;
    uint64_t val = original;
    ByteConverter::apply<uint64_t>(&val);
    ByteConverter::apply<uint64_t>(&val);
    EXPECT_EQ(original, val);
}

TEST(ByteConverter, Uint16ZeroValue)
{
    uint16_t val = 0x0000;
    ByteConverter::apply<uint16_t>(&val);
    EXPECT_EQ(uint16_t(0x0000), val);
}

TEST(ByteConverter, Uint16MaxValue)
{
    uint16_t val = 0xFFFF;
    ByteConverter::apply<uint16_t>(&val);
    EXPECT_EQ(uint16_t(0xFFFF), val);
}

TEST(ByteConverter, Uint32MaxValue)
{
    uint32_t val = 0xFFFFFFFF;
    ByteConverter::apply<uint32_t>(&val);
    EXPECT_EQ(uint32_t(0xFFFFFFFF), val);
}

TEST(ByteConverter, Int16Swap)
{
    int16_t val = 0x1234;
    ByteConverter::apply<int16_t>(&val);
    EXPECT_EQ(int16_t(0x3412), val);
}

TEST(ByteConverter, Int32Swap)
{
    int32_t val = 0x12345678;
    ByteConverter::apply<int32_t>(&val);
    EXPECT_EQ(int32_t(0x78563412), val);
}

TEST(ByteConverter, FloatSwap)
{
    // Float 1.0f is 0x3F800000 in IEEE 754
    // After swap: 0x0000803F
    float original = 1.0f;
    uint32_t swapped_bits = 0x0000803F;
    float val = original;
    ByteConverter::apply<float>(&val);
    uint32_t val_bits;
    memcpy(&val_bits, &val, sizeof(float));
    EXPECT_EQ(swapped_bits, val_bits);
}

TEST(ByteConverter, ByteSwapTemplate)
{
    uint16_t val = 0xABCD;
    uint16_t swapped = ByteSwap(val);
    EXPECT_EQ(uint16_t(0xCDAB), swapped);
    EXPECT_EQ(val, uint16_t(0xABCD)); // original unchanged
}

TEST(ByteConverter, ByteSwapUint32)
{
    uint32_t val = 0xDEADBEEF;
    uint32_t swapped = ByteSwap(val);
    EXPECT_EQ(uint32_t(0xEFBEADDE), swapped);
}

// ---- Network byte order conversion patterns ----
// Simulates the htobe16/be16toh patterns used in packet handling

TEST(ByteConverter, NetworkByteOrder_Uint16)
{
    // Value 256 in little-endian is: 0x00 0x01
    // Value 256 in big-endian (network order) is: 0x01 0x00
    uint16_t host = 0x0100; // 256
    uint16_t net = ByteSwap(host);
    uint16_t back = ByteSwap(net);
    EXPECT_EQ(host, back);
}

TEST(ByteConverter, NetworkByteOrder_Uint32)
{
    uint32_t host = 0x12345678;
    uint32_t net = ByteSwap(host);
    EXPECT_EQ(uint32_t(0x78563412), net);
    EXPECT_EQ(host, ByteSwap(net));
}

// ---- Character-level byte manipulation ----
TEST(ByteConverter, RawByteAccess_Uint32)
{
    uint32_t val = 0x12345678;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&val);
    // On little-endian: bytes[0]=0x78, bytes[1]=0x56, bytes[2]=0x34, bytes[3]=0x12
    EXPECT_EQ(uint8_t(0x78), bytes[0]);
    EXPECT_EQ(uint8_t(0x56), bytes[1]);
    EXPECT_EQ(uint8_t(0x34), bytes[2]);
    EXPECT_EQ(uint8_t(0x12), bytes[3]);
}

TEST(ByteConverter, RawByteAccess_Uint16)
{
    uint16_t val = 0x1234;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&val);
    EXPECT_EQ(uint8_t(0x34), bytes[0]);
    EXPECT_EQ(uint8_t(0x12), bytes[1]);
}
