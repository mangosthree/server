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
#include <vector>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <numeric>
#include <algorithm>

// ============================================================================
// ByteBuffer Stress Tests - adversarial inputs designed to break serialization
// Mirrors src/shared/Utilities/ByteBuffer.h
// ============================================================================

class ByteBufferException
{
public:
    ByteBufferException(bool _add, size_t _pos, size_t _esize, size_t _size)
        : add(_add), pos(_pos), esize(_esize), size(_size) {}
    bool add; size_t pos, esize, size;
};

class ByteBuffer
{
public:
    static const size_t DEFAULT_SIZE = 64;
    ByteBuffer() : _rpos(0), _wpos(0), _bitpos(8), _curbitval(0) { _storage.reserve(DEFAULT_SIZE); }
    explicit ByteBuffer(size_t res) : _rpos(0), _wpos(0), _bitpos(8), _curbitval(0) { _storage.reserve(res); }
    ByteBuffer(const ByteBuffer& b) : _rpos(b._rpos), _wpos(b._wpos), _storage(b._storage), _bitpos(b._bitpos), _curbitval(b._curbitval) {}

    void clear() { _storage.clear(); _rpos = _wpos = 0; _curbitval = 0; _bitpos = 8; }

    template<typename T> ByteBuffer& append(T value)
    { FlushBits(); return append((uint8_t*)&value, sizeof(value)); }

    ByteBuffer& append(const uint8_t* src, size_t cnt)
    {
        if (!cnt) return *this;
        if (_storage.size() < _wpos + cnt) _storage.resize(_wpos + cnt);
        memcpy(&_storage[_wpos], src, cnt);
        _wpos += cnt;
        return *this;
    }

    ByteBuffer& append(const char* src, size_t cnt) { return append((const uint8_t*)src, cnt); }

    void FlushBits()
    { if (_bitpos == 8) return; append((uint8_t*)&_curbitval, 1); _curbitval = 0; _bitpos = 8; }

    bool WriteBit(bool bit)
    {
        --_bitpos;
        if (bit) _curbitval |= (1 << (_bitpos));
        if (_bitpos == 0) { _bitpos = 8; append((uint8_t*)&_curbitval, sizeof(_curbitval)); _curbitval = 0; }
        return bit;
    }

    bool ReadBit()
    {
        ++_bitpos;
        if (_bitpos > 7) { _curbitval = read<uint8_t>(); _bitpos = 0; }
        return ((_curbitval >> (7 - _bitpos)) & 1) != 0;
    }

    void WriteBits(uint32_t value, size_t bits)
    { for (int32_t i = bits - 1; i >= 0; --i) WriteBit((value >> i) & 1); }

    uint32_t ReadBits(size_t bits)
    {
        uint32_t value = 0;
        for (int32_t i = bits - 1; i >= 0; --i) if (ReadBit()) value |= (1 << i);
        return value;
    }

    ByteBuffer& operator<<(uint8_t v) { append<uint8_t>(v); return *this; }
    ByteBuffer& operator<<(uint16_t v) { append<uint16_t>(v); return *this; }
    ByteBuffer& operator<<(uint32_t v) { append<uint32_t>(v); return *this; }
    ByteBuffer& operator<<(uint64_t v) { append<uint64_t>(v); return *this; }
    ByteBuffer& operator<<(int8_t v) { append<int8_t>(v); return *this; }
    ByteBuffer& operator<<(int16_t v) { append<int16_t>(v); return *this; }
    ByteBuffer& operator<<(int32_t v) { append<int32_t>(v); return *this; }
    ByteBuffer& operator<<(int64_t v) { append<int64_t>(v); return *this; }
    ByteBuffer& operator<<(float v) { append<float>(v); return *this; }
    ByteBuffer& operator<<(double v) { append<double>(v); return *this; }
    ByteBuffer& operator<<(const std::string& v)
    { append((uint8_t*)v.c_str(), v.size()); append<uint8_t>(0); return *this; }

    ByteBuffer& operator>>(uint8_t& v) { v = read<uint8_t>(); return *this; }
    ByteBuffer& operator>>(uint16_t& v) { v = read<uint16_t>(); return *this; }
    ByteBuffer& operator>>(uint32_t& v) { v = read<uint32_t>(); return *this; }
    ByteBuffer& operator>>(uint64_t& v) { v = read<uint64_t>(); return *this; }
    ByteBuffer& operator>>(int8_t& v) { v = read<int8_t>(); return *this; }
    ByteBuffer& operator>>(int16_t& v) { v = read<int16_t>(); return *this; }
    ByteBuffer& operator>>(int32_t& v) { v = read<int32_t>(); return *this; }
    ByteBuffer& operator>>(int64_t& v) { v = read<int64_t>(); return *this; }
    ByteBuffer& operator>>(float& v) { v = read<float>(); return *this; }
    ByteBuffer& operator>>(double& v) { v = read<double>(); return *this; }
    ByteBuffer& operator>>(std::string& v)
    {
        v.clear();
        while (_rpos < size()) { char c = read<char>(); if (!c) break; v += c; }
        return *this;
    }

    template<typename T> T read()
    {
        if (_rpos + sizeof(T) > size()) throw ByteBufferException(false, _rpos, sizeof(T), size());
        T r; memcpy(&r, &_storage[_rpos], sizeof(T)); _rpos += sizeof(T); return r;
    }

    template<typename T> T read(size_t pos) const
    {
        if (pos + sizeof(T) > size()) throw ByteBufferException(false, pos, sizeof(T), size());
        T r; memcpy(&r, &_storage[pos], sizeof(T)); return r;
    }

    void read(uint8_t* dest, size_t len)
    {
        if (_rpos + len > size()) throw ByteBufferException(false, _rpos, len, size());
        memcpy(dest, &_storage[_rpos], len);
        _rpos += len;
    }

    template<typename T> void put(size_t pos, T value)
    { put(pos, (uint8_t*)&value, sizeof(value)); }

    void put(size_t pos, const uint8_t* src, size_t cnt)
    {
        if (pos + cnt > size()) throw ByteBufferException(true, pos, cnt, size());
        memcpy(&_storage[pos], src, cnt);
    }

    size_t rpos() const { return _rpos; }
    size_t rpos(size_t p) { _rpos = p; return _rpos; }
    size_t wpos() const { return _wpos; }
    size_t wpos(size_t p) { _wpos = p; return _wpos; }
    size_t size() const { return _storage.size(); }
    bool empty() const { return _storage.empty(); }
    void resize(size_t s) { _storage.resize(s); _rpos = 0; _wpos = size(); }
    void reserve(size_t s) { _storage.reserve(s); }
    const uint8_t* contents() const { return _storage.data(); }
    uint8_t& operator[](size_t p) { return _storage[p]; }

    void appendPackGUID(uint64_t guid)
    {
        uint8_t packGUID[8 + 1];
        packGUID[0] = 0;
        size_t byteCount = 1;
        for (uint8_t i = 0; guid != 0; ++i)
        {
            if (guid & 0xFF) { packGUID[0] |= uint8_t(1 << i); packGUID[byteCount] = uint8_t(guid & 0xFF); ++byteCount; }
            guid >>= 8;
        }
        append(packGUID, byteCount);
    }

protected:
    size_t _rpos, _wpos;
    std::vector<uint8_t> _storage;
    uint8_t _bitpos, _curbitval;
};

// ============================================================================
// STRESS: ByteBuffer overflow & underflow attacks
// ============================================================================

TEST(BufferStress, ReadFromEmptyBuffer)
{
    ByteBuffer buf;
    bool caught = false;
    try { buf.read<uint8_t>(); } catch (const ByteBufferException&) { caught = true; }
    EXPECT_TRUE(caught);
}

TEST(BufferStress, ReadEveryTypeFromEmpty)
{
    ByteBuffer buf;
    int catches = 0;
    try { buf.read<uint8_t>(); } catch (const ByteBufferException&) { ++catches; }
    try { buf.read<uint16_t>(); } catch (const ByteBufferException&) { ++catches; }
    try { buf.read<uint32_t>(); } catch (const ByteBufferException&) { ++catches; }
    try { buf.read<uint64_t>(); } catch (const ByteBufferException&) { ++catches; }
    try { buf.read<float>(); } catch (const ByteBufferException&) { ++catches; }
    try { buf.read<double>(); } catch (const ByteBufferException&) { ++catches; }
    EXPECT_EQ(6, catches);
}

TEST(BufferStress, ReadPartialUint32)
{
    ByteBuffer buf;
    buf << uint8_t(0xFF) << uint8_t(0xFF); // 2 bytes
    bool caught = false;
    try { buf.read<uint32_t>(); } catch (const ByteBufferException&) { caught = true; }
    EXPECT_TRUE(caught);
}

TEST(BufferStress, MassiveWrite100kEntries)
{
    ByteBuffer buf;
    for (uint32_t i = 0; i < 100000; ++i)
        buf << i;
    EXPECT_EQ(size_t(400000), buf.size());
    for (uint32_t i = 0; i < 100000; ++i)
    {
        uint32_t v;
        buf >> v;
        EXPECT_EQ(i, v);
    }
}

TEST(BufferStress, AlternatingTypeSizes)
{
    ByteBuffer buf;
    for (int i = 0; i < 1000; ++i)
    {
        buf << uint8_t(i & 0xFF);
        buf << uint16_t(i);
        buf << uint32_t(i);
        buf << uint64_t(i);
    }
    for (int i = 0; i < 1000; ++i)
    {
        uint8_t a; uint16_t b; uint32_t c; uint64_t d;
        buf >> a >> b >> c >> d;
        EXPECT_EQ(uint8_t(i & 0xFF), a);
        EXPECT_EQ(uint16_t(i), b);
        EXPECT_EQ(uint32_t(i), c);
        EXPECT_EQ(uint64_t(i), d);
    }
}

TEST(BufferStress, ZeroLengthAppend)
{
    ByteBuffer buf;
    buf.append((const uint8_t*)nullptr, 0);
    EXPECT_EQ(size_t(0), buf.size());
}

TEST(BufferStress, StringWithEmbeddedNulls)
{
    ByteBuffer buf;
    buf << std::string("hello");
    // Reading should stop at first null
    std::string out;
    buf >> out;
    EXPECT_STR_EQ("hello", out);
}

TEST(BufferStress, VeryLongString)
{
    ByteBuffer buf;
    std::string longStr(10000, 'X');
    buf << longStr;
    std::string out;
    buf >> out;
    EXPECT_EQ(size_t(10000), out.size());
    EXPECT_STR_EQ(longStr, out);
}

TEST(BufferStress, PutBeyondBounds)
{
    ByteBuffer buf;
    buf << uint32_t(0);
    bool caught = false;
    try { buf.put<uint32_t>(100, 42); } catch (const ByteBufferException&) { caught = true; }
    EXPECT_TRUE(caught);
}

TEST(BufferStress, ReadAtBeyondBounds)
{
    ByteBuffer buf;
    buf << uint32_t(0);
    bool caught = false;
    try { buf.read<uint32_t>(100); } catch (const ByteBufferException&) { caught = true; }
    EXPECT_TRUE(caught);
}

TEST(BufferStress, ReadRawBeyondBounds)
{
    ByteBuffer buf;
    buf << uint32_t(0);
    uint8_t dest[100];
    bool caught = false;
    try { buf.read(dest, 100); } catch (const ByteBufferException&) { caught = true; }
    EXPECT_TRUE(caught);
}

TEST(BufferStress, DoubleReadFails)
{
    ByteBuffer buf;
    buf << uint32_t(42);
    uint32_t v1, v2;
    buf >> v1;
    bool caught = false;
    try { buf >> v2; } catch (const ByteBufferException&) { caught = true; }
    EXPECT_EQ(uint32_t(42), v1);
    EXPECT_TRUE(caught);
}

TEST(BufferStress, WriteThenClearThenRead)
{
    ByteBuffer buf;
    buf << uint32_t(42);
    buf.clear();
    bool caught = false;
    try { buf.read<uint32_t>(); } catch (const ByteBufferException&) { caught = true; }
    EXPECT_TRUE(caught);
    EXPECT_TRUE(buf.empty());
}

TEST(BufferStress, FloatNaN)
{
    ByteBuffer buf;
    float nan = std::numeric_limits<float>::quiet_NaN();
    buf << nan;
    float out;
    buf >> out;
    EXPECT_TRUE(std::isnan(out));
}

TEST(BufferStress, FloatInfinity)
{
    ByteBuffer buf;
    buf << std::numeric_limits<float>::infinity();
    buf << -std::numeric_limits<float>::infinity();
    float posInf, negInf;
    buf >> posInf >> negInf;
    EXPECT_TRUE(std::isinf(posInf) && posInf > 0);
    EXPECT_TRUE(std::isinf(negInf) && negInf < 0);
}

TEST(BufferStress, FloatDenormalized)
{
    ByteBuffer buf;
    float denorm = std::numeric_limits<float>::denorm_min();
    buf << denorm;
    float out;
    buf >> out;
    EXPECT_TRUE(out > 0.0f);
    EXPECT_TRUE(out < std::numeric_limits<float>::min());
}

TEST(BufferStress, PackGUIDEveryBitPattern)
{
    ByteBuffer buf;
    // Test all single-byte GUIDs
    for (uint64_t g = 0; g < 256; ++g)
    {
        ByteBuffer local;
        local.appendPackGUID(g);
        EXPECT_LE(local.size(), size_t(9));
    }
    // Sparse GUIDs (only one byte set in each position)
    for (int shift = 0; shift < 64; shift += 8)
    {
        uint64_t guid = uint64_t(0xAB) << shift;
        buf.clear(); buf.appendPackGUID(guid);
        EXPECT_LE(buf.size(), size_t(9));
    }
}

TEST(BufferStress, BitOperationsMaxBits)
{
    ByteBuffer buf;
    buf.WriteBits(0xFFFFFFFF, 32);
    buf.FlushBits();
    uint32_t val = buf.ReadBits(32);
    EXPECT_EQ(uint32_t(0xFFFFFFFF), val);
}

TEST(BufferStress, BitOperationsZeroBits)
{
    ByteBuffer buf;
    buf.WriteBits(0, 32);
    buf.FlushBits();
    uint32_t val = buf.ReadBits(32);
    EXPECT_EQ(uint32_t(0), val);
}

TEST(BufferStress, BitsMixedWithBytes)
{
    ByteBuffer buf;
    buf.WriteBit(true);
    buf.WriteBit(false);
    buf.WriteBit(true);
    buf.FlushBits();
    buf << uint32_t(0xDEADBEEF);
    buf.WriteBit(false);
    buf.WriteBit(true);
    buf.FlushBits();

    bool b1 = buf.ReadBit();
    bool b2 = buf.ReadBit();
    bool b3 = buf.ReadBit();
    // Need to realign to byte boundary to read uint32
    buf.rpos(1); // skip the flushed bit byte
    uint32_t val;
    buf >> val;
    EXPECT_TRUE(b1);
    EXPECT_FALSE(b2);
    EXPECT_TRUE(b3);
    EXPECT_EQ(uint32_t(0xDEADBEEF), val);
}

TEST(BufferStress, RapidClearAndReuse)
{
    ByteBuffer buf;
    for (int cycle = 0; cycle < 1000; ++cycle)
    {
        buf.clear();
        buf << uint32_t(cycle);
        uint32_t val;
        buf >> val;
        EXPECT_EQ(uint32_t(cycle), val);
    }
}

TEST(BufferStress, CopiedBufferIndependence)
{
    ByteBuffer original;
    original << uint32_t(42);
    ByteBuffer copy(original);
    copy << uint32_t(99);
    EXPECT_EQ(size_t(4), original.size());
    EXPECT_EQ(size_t(8), copy.size());
}

TEST(BufferStress, ReadPositionManipulation)
{
    ByteBuffer buf;
    buf << uint32_t(10) << uint32_t(20) << uint32_t(30);
    // Read backward by resetting position
    buf.rpos(8);
    uint32_t v;
    buf >> v;
    EXPECT_EQ(uint32_t(30), v);
    buf.rpos(0);
    buf >> v;
    EXPECT_EQ(uint32_t(10), v);
    buf.rpos(4);
    buf >> v;
    EXPECT_EQ(uint32_t(20), v);
}
