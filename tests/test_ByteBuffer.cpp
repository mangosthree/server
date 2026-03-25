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

// ============================================================================
// Minimal reimplementation of ByteBuffer for standalone testing.
// This mirrors the interface in src/shared/Utilities/ByteBuffer.h without
// pulling in ACE or Log dependencies.
// ============================================================================

class ByteBufferException
{
public:
    ByteBufferException(bool _add, size_t _pos, size_t _esize, size_t _size)
        : add(_add), pos(_pos), esize(_esize), size(_size) {}

    bool add;
    size_t pos;
    size_t esize;
    size_t size;
};

class ByteBuffer
{
public:
    static const size_t DEFAULT_SIZE = 64;

    ByteBuffer() : _rpos(0), _wpos(0), _bitpos(8), _curbitval(0)
    {
        _storage.reserve(DEFAULT_SIZE);
    }

    explicit ByteBuffer(size_t res) : _rpos(0), _wpos(0), _bitpos(8), _curbitval(0)
    {
        _storage.reserve(res);
    }

    ByteBuffer(const ByteBuffer& buf)
        : _rpos(buf._rpos), _wpos(buf._wpos), _storage(buf._storage),
          _bitpos(buf._bitpos), _curbitval(buf._curbitval) {}

    void clear()
    {
        _storage.clear();
        _rpos = _wpos = 0;
        _curbitval = 0;
        _bitpos = 8;
    }

    template <typename T> ByteBuffer& append(T value)
    {
        FlushBits();
        return append((uint8_t*)&value, sizeof(value));
    }

    ByteBuffer& append(const uint8_t* src, size_t cnt)
    {
        if (cnt == 0) return *this;
        if (_storage.size() < _wpos + cnt)
            _storage.resize(_wpos + cnt);
        memcpy(&_storage[_wpos], src, cnt);
        _wpos += cnt;
        return *this;
    }

    ByteBuffer& append(const char* src, size_t cnt)
    {
        return append((const uint8_t*)src, cnt);
    }

    void FlushBits()
    {
        if (_bitpos == 8) return;
        append((uint8_t*)&_curbitval, sizeof(uint8_t));
        _curbitval = 0;
        _bitpos = 8;
    }

    bool WriteBit(bool bit)
    {
        --_bitpos;
        if (bit)
            _curbitval |= (1 << (_bitpos));
        if (_bitpos == 0)
        {
            _bitpos = 8;
            append((uint8_t*)&_curbitval, sizeof(_curbitval));
            _curbitval = 0;
        }
        return bit;
    }

    bool ReadBit()
    {
        ++_bitpos;
        if (_bitpos > 7)
        {
            _curbitval = read<uint8_t>();
            _bitpos = 0;
        }
        return ((_curbitval >> (7 - _bitpos)) & 1) != 0;
    }

    void WriteBits(uint32_t value, size_t bits)
    {
        for (int32_t i = bits - 1; i >= 0; --i)
            WriteBit((value >> i) & 1);
    }

    uint32_t ReadBits(size_t bits)
    {
        uint32_t value = 0;
        for (int32_t i = bits - 1; i >= 0; --i)
            if (ReadBit())
                value |= (1 << i);
        return value;
    }

    void ResetBitReader()
    {
        _bitpos = 8;
    }

    template <typename T> void put(size_t pos, T value)
    {
        put(pos, (uint8_t*)&value, sizeof(value));
    }

    void put(size_t pos, const uint8_t* src, size_t cnt)
    {
        if (pos + cnt > size())
            throw ByteBufferException(true, pos, cnt, size());
        memcpy(&_storage[pos], src, cnt);
    }

    // Stream operators for writing
    ByteBuffer& operator<<(uint8_t value) { append<uint8_t>(value); return *this; }
    ByteBuffer& operator<<(uint16_t value) { append<uint16_t>(value); return *this; }
    ByteBuffer& operator<<(uint32_t value) { append<uint32_t>(value); return *this; }
    ByteBuffer& operator<<(uint64_t value) { append<uint64_t>(value); return *this; }
    ByteBuffer& operator<<(int8_t value) { append<int8_t>(value); return *this; }
    ByteBuffer& operator<<(int16_t value) { append<int16_t>(value); return *this; }
    ByteBuffer& operator<<(int32_t value) { append<int32_t>(value); return *this; }
    ByteBuffer& operator<<(int64_t value) { append<int64_t>(value); return *this; }
    ByteBuffer& operator<<(float value) { append<float>(value); return *this; }
    ByteBuffer& operator<<(double value) { append<double>(value); return *this; }

    ByteBuffer& operator<<(const std::string& value)
    {
        append((uint8_t const*)value.c_str(), value.size());
        append<uint8_t>(0);
        return *this;
    }

    ByteBuffer& operator<<(const char* str)
    {
        append((uint8_t const*)str, str ? strlen(str) : 0);
        append<uint8_t>(0);
        return *this;
    }

    // Stream operators for reading
    ByteBuffer& operator>>(uint8_t& value) { value = read<uint8_t>(); return *this; }
    ByteBuffer& operator>>(uint16_t& value) { value = read<uint16_t>(); return *this; }
    ByteBuffer& operator>>(uint32_t& value) { value = read<uint32_t>(); return *this; }
    ByteBuffer& operator>>(uint64_t& value) { value = read<uint64_t>(); return *this; }
    ByteBuffer& operator>>(int8_t& value) { value = read<int8_t>(); return *this; }
    ByteBuffer& operator>>(int16_t& value) { value = read<int16_t>(); return *this; }
    ByteBuffer& operator>>(int32_t& value) { value = read<int32_t>(); return *this; }
    ByteBuffer& operator>>(int64_t& value) { value = read<int64_t>(); return *this; }
    ByteBuffer& operator>>(float& value) { value = read<float>(); return *this; }
    ByteBuffer& operator>>(double& value) { value = read<double>(); return *this; }

    ByteBuffer& operator>>(std::string& value)
    {
        value.clear();
        while (rpos() < size())
        {
            char c = read<char>();
            if (c == 0) break;
            value += c;
        }
        return *this;
    }

    template <typename T> T read()
    {
        T r;
        read((uint8_t*)&r, sizeof(T));
        return r;
    }

    void read(uint8_t* dest, size_t len)
    {
        if (_rpos + len > size())
            throw ByteBufferException(false, _rpos, len, size());
        memcpy(dest, &_storage[_rpos], len);
        _rpos += len;
    }

    template <typename T> T read(size_t pos) const
    {
        if (pos + sizeof(T) > size())
            throw ByteBufferException(false, pos, sizeof(T), size());
        T val;
        memcpy(&val, &_storage[pos], sizeof(T));
        return val;
    }

    size_t rpos() const { return _rpos; }
    size_t rpos(size_t rpos_)
    {
        _rpos = rpos_;
        return _rpos;
    }

    size_t wpos() const { return _wpos; }
    size_t wpos(size_t wpos_)
    {
        _wpos = wpos_;
        return _wpos;
    }

    size_t size() const { return _storage.size(); }
    bool empty() const { return _storage.empty(); }

    void resize(size_t newsize)
    {
        _storage.resize(newsize);
        _rpos = 0;
        _wpos = size();
    }

    void reserve(size_t ressize) { _storage.reserve(ressize); }

    const uint8_t* contents() const { return &_storage[0]; }

    uint8_t& operator[](size_t pos) { return _storage[pos]; }
    const uint8_t& operator[](size_t pos) const { return _storage[pos]; }

    void appendPackGUID(uint64_t guid)
    {
        uint8_t packGUID[8 + 1];
        packGUID[0] = 0;
        size_t byteCount = 1;
        for (uint8_t i = 0; guid != 0; ++i)
        {
            if (guid & 0xFF)
            {
                packGUID[0] |= uint8_t(1 << i);
                packGUID[byteCount] = uint8_t(guid & 0xFF);
                ++byteCount;
            }
            guid >>= 8;
        }
        append(packGUID, byteCount);
    }

protected:
    size_t _rpos, _wpos;
    std::vector<uint8_t> _storage;
    uint8_t _bitpos;
    uint8_t _curbitval;
};

// ============================================================================
// ByteBuffer Tests
// ============================================================================

// --- Construction & Initialization ---

TEST(ByteBuffer, DefaultConstructor)
{
    ByteBuffer buf;
    EXPECT_EQ(size_t(0), buf.size());
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(size_t(0), buf.rpos());
    EXPECT_EQ(size_t(0), buf.wpos());
}

TEST(ByteBuffer, SizedConstructor)
{
    ByteBuffer buf(128);
    EXPECT_EQ(size_t(0), buf.size());
    EXPECT_TRUE(buf.empty());
}

TEST(ByteBuffer, CopyConstructor)
{
    ByteBuffer original;
    original << uint32_t(42) << uint16_t(7);
    ByteBuffer copy(original);
    EXPECT_EQ(original.size(), copy.size());
    EXPECT_EQ(original.rpos(), copy.rpos());
    EXPECT_EQ(original.wpos(), copy.wpos());
    for (size_t i = 0; i < original.size(); ++i)
        EXPECT_EQ(original[i], copy[i]);
}

TEST(ByteBuffer, Clear)
{
    ByteBuffer buf;
    buf << uint32_t(123);
    EXPECT_FALSE(buf.empty());
    buf.clear();
    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(size_t(0), buf.rpos());
    EXPECT_EQ(size_t(0), buf.wpos());
}

// --- Integer Read/Write ---

TEST(ByteBuffer, WriteReadUint8)
{
    ByteBuffer buf;
    buf << uint8_t(0xFF);
    EXPECT_EQ(size_t(1), buf.size());
    uint8_t val;
    buf >> val;
    EXPECT_EQ(uint8_t(0xFF), val);
}

TEST(ByteBuffer, WriteReadUint16)
{
    ByteBuffer buf;
    buf << uint16_t(0xABCD);
    EXPECT_EQ(size_t(2), buf.size());
    uint16_t val;
    buf >> val;
    EXPECT_EQ(uint16_t(0xABCD), val);
}

TEST(ByteBuffer, WriteReadUint32)
{
    ByteBuffer buf;
    buf << uint32_t(0xDEADBEEF);
    EXPECT_EQ(size_t(4), buf.size());
    uint32_t val;
    buf >> val;
    EXPECT_EQ(uint32_t(0xDEADBEEF), val);
}

TEST(ByteBuffer, WriteReadUint64)
{
    ByteBuffer buf;
    buf << uint64_t(0x123456789ABCDEF0ULL);
    EXPECT_EQ(size_t(8), buf.size());
    uint64_t val;
    buf >> val;
    EXPECT_EQ(uint64_t(0x123456789ABCDEF0ULL), val);
}

TEST(ByteBuffer, WriteReadInt8)
{
    ByteBuffer buf;
    buf << int8_t(-42);
    int8_t val;
    buf >> val;
    EXPECT_EQ(int8_t(-42), val);
}

TEST(ByteBuffer, WriteReadInt16)
{
    ByteBuffer buf;
    buf << int16_t(-1234);
    int16_t val;
    buf >> val;
    EXPECT_EQ(int16_t(-1234), val);
}

TEST(ByteBuffer, WriteReadInt32)
{
    ByteBuffer buf;
    buf << int32_t(-999999);
    int32_t val;
    buf >> val;
    EXPECT_EQ(int32_t(-999999), val);
}

TEST(ByteBuffer, WriteReadInt64)
{
    ByteBuffer buf;
    buf << int64_t(-9876543210LL);
    int64_t val;
    buf >> val;
    EXPECT_EQ(int64_t(-9876543210LL), val);
}

// --- Floating Point Read/Write ---

TEST(ByteBuffer, WriteReadFloat)
{
    ByteBuffer buf;
    buf << float(3.14159f);
    EXPECT_EQ(size_t(4), buf.size());
    float val;
    buf >> val;
    EXPECT_NEAR(3.14159f, val, 0.0001f);
}

TEST(ByteBuffer, WriteReadDouble)
{
    ByteBuffer buf;
    buf << double(2.718281828);
    EXPECT_EQ(size_t(8), buf.size());
    double val;
    buf >> val;
    EXPECT_NEAR(2.718281828, val, 0.000001);
}

TEST(ByteBuffer, WriteReadFloatZero)
{
    ByteBuffer buf;
    buf << float(0.0f);
    float val;
    buf >> val;
    EXPECT_FLOAT_EQ(0.0f, val);
}

TEST(ByteBuffer, WriteReadFloatNegative)
{
    ByteBuffer buf;
    buf << float(-1.5f);
    float val;
    buf >> val;
    EXPECT_FLOAT_EQ(-1.5f, val);
}

// --- String Read/Write ---

TEST(ByteBuffer, WriteReadString)
{
    ByteBuffer buf;
    std::string input = "Hello, MaNGOS!";
    buf << input;
    EXPECT_EQ(input.size() + 1, buf.size()); // +1 for null terminator
    std::string output;
    buf >> output;
    EXPECT_STR_EQ(input, output);
}

TEST(ByteBuffer, WriteReadEmptyString)
{
    ByteBuffer buf;
    std::string input = "";
    buf << input;
    EXPECT_EQ(size_t(1), buf.size()); // just null terminator
    std::string output;
    buf >> output;
    EXPECT_STR_EQ(input, output);
}

TEST(ByteBuffer, WriteReadCString)
{
    ByteBuffer buf;
    const char* input = "World of Warcraft";
    buf << input;
    std::string output;
    buf >> output;
    EXPECT_STR_EQ(std::string(input), output);
}

// --- Multiple Values Sequential ---

TEST(ByteBuffer, MultipleValuesSequential)
{
    ByteBuffer buf;
    buf << uint8_t(1) << uint16_t(2) << uint32_t(3) << uint64_t(4);

    EXPECT_EQ(size_t(1 + 2 + 4 + 8), buf.size());

    uint8_t a; uint16_t b; uint32_t c; uint64_t d;
    buf >> a >> b >> c >> d;

    EXPECT_EQ(uint8_t(1), a);
    EXPECT_EQ(uint16_t(2), b);
    EXPECT_EQ(uint32_t(3), c);
    EXPECT_EQ(uint64_t(4), d);
}

TEST(ByteBuffer, MixedTypesSequential)
{
    ByteBuffer buf;
    buf << uint32_t(42) << float(1.5f) << std::string("test") << int16_t(-1);

    uint32_t a; float b; std::string c; int16_t d;
    buf >> a >> b >> c >> d;

    EXPECT_EQ(uint32_t(42), a);
    EXPECT_FLOAT_EQ(1.5f, b);
    EXPECT_STR_EQ("test", c);
    EXPECT_EQ(int16_t(-1), d);
}

// --- Position Management ---

TEST(ByteBuffer, ReadPositionAdvances)
{
    ByteBuffer buf;
    buf << uint32_t(1) << uint32_t(2);
    EXPECT_EQ(size_t(0), buf.rpos());
    uint32_t val;
    buf >> val;
    EXPECT_EQ(size_t(4), buf.rpos());
    buf >> val;
    EXPECT_EQ(size_t(8), buf.rpos());
}

TEST(ByteBuffer, WritePositionAdvances)
{
    ByteBuffer buf;
    EXPECT_EQ(size_t(0), buf.wpos());
    buf << uint32_t(1);
    EXPECT_EQ(size_t(4), buf.wpos());
    buf << uint16_t(2);
    EXPECT_EQ(size_t(6), buf.wpos());
}

TEST(ByteBuffer, SetReadPosition)
{
    ByteBuffer buf;
    buf << uint32_t(10) << uint32_t(20) << uint32_t(30);
    buf.rpos(4); // skip first uint32
    uint32_t val;
    buf >> val;
    EXPECT_EQ(uint32_t(20), val);
}

TEST(ByteBuffer, SetWritePosition)
{
    ByteBuffer buf;
    buf << uint32_t(0) << uint32_t(0);
    buf.wpos(0);
    buf << uint32_t(42);
    buf.rpos(0);
    uint32_t val;
    buf >> val;
    EXPECT_EQ(uint32_t(42), val);
}

// --- Put Operation ---

TEST(ByteBuffer, PutAtPosition)
{
    ByteBuffer buf;
    buf << uint32_t(0) << uint32_t(0) << uint32_t(0);
    buf.put<uint32_t>(4, 0xBEEF);
    uint32_t val = buf.read<uint32_t>(4);
    EXPECT_EQ(uint32_t(0xBEEF), val);
}

TEST(ByteBuffer, PutDoesNotChangePositions)
{
    ByteBuffer buf;
    buf << uint32_t(0) << uint32_t(0);
    size_t rposBefore = buf.rpos();
    size_t wposBefore = buf.wpos();
    buf.put<uint32_t>(0, 42);
    EXPECT_EQ(rposBefore, buf.rpos());
    EXPECT_EQ(wposBefore, buf.wpos());
}

// --- Read at Position ---

TEST(ByteBuffer, ReadAtPosition)
{
    ByteBuffer buf;
    buf << uint32_t(100) << uint32_t(200) << uint32_t(300);
    EXPECT_EQ(uint32_t(100), buf.read<uint32_t>(0));
    EXPECT_EQ(uint32_t(200), buf.read<uint32_t>(4));
    EXPECT_EQ(uint32_t(300), buf.read<uint32_t>(8));
    // rpos should not have changed
    EXPECT_EQ(size_t(0), buf.rpos());
}

// --- Boundary Conditions ---

TEST(ByteBuffer, ReadBeyondSizeThrows)
{
    ByteBuffer buf;
    buf << uint8_t(1);
    bool caught = false;
    try
    {
        buf.read<uint32_t>(); // only 1 byte available, reading 4
    }
    catch (const ByteBufferException&)
    {
        caught = true;
    }
    EXPECT_TRUE(caught);
}

TEST(ByteBuffer, PutBeyondSizeThrows)
{
    ByteBuffer buf;
    buf << uint8_t(1);
    bool caught = false;
    try
    {
        buf.put<uint32_t>(0, 42); // buffer is only 1 byte
    }
    catch (const ByteBufferException&)
    {
        caught = true;
    }
    EXPECT_TRUE(caught);
}

TEST(ByteBuffer, ReadAtBeyondSizeThrows)
{
    ByteBuffer buf;
    buf << uint32_t(1);
    bool caught = false;
    try
    {
        buf.read<uint32_t>(8); // beyond size
    }
    catch (const ByteBufferException&)
    {
        caught = true;
    }
    EXPECT_TRUE(caught);
}

// --- Resize & Reserve ---

TEST(ByteBuffer, Resize)
{
    ByteBuffer buf;
    buf.resize(16);
    EXPECT_EQ(size_t(16), buf.size());
    EXPECT_EQ(size_t(0), buf.rpos());
}

TEST(ByteBuffer, ReserveDoesNotChangeSize)
{
    ByteBuffer buf;
    buf.reserve(1024);
    EXPECT_EQ(size_t(0), buf.size());
}

// --- Bit Operations ---

TEST(ByteBuffer, WriteBitAndFlush)
{
    ByteBuffer buf;
    buf.WriteBit(true);
    buf.WriteBit(false);
    buf.WriteBit(true);
    buf.WriteBit(false);
    buf.WriteBit(true);
    buf.WriteBit(false);
    buf.WriteBit(true);
    buf.WriteBit(false);
    // 8 bits written, should auto-flush
    EXPECT_EQ(size_t(1), buf.size());
    // Pattern: 10101010 = 0xAA
    EXPECT_EQ(uint8_t(0xAA), buf[0]);
}

TEST(ByteBuffer, WriteBitsPartialFlush)
{
    ByteBuffer buf;
    buf.WriteBit(true);
    buf.WriteBit(true);
    buf.FlushBits();
    EXPECT_EQ(size_t(1), buf.size());
    // 11000000 = 0xC0
    EXPECT_EQ(uint8_t(0xC0), buf[0]);
}

TEST(ByteBuffer, WriteBitsValue)
{
    ByteBuffer buf;
    buf.WriteBits(0b1010, 4);
    buf.WriteBits(0b0101, 4);
    // Pattern: 10100101 = 0xA5
    EXPECT_EQ(size_t(1), buf.size());
    EXPECT_EQ(uint8_t(0xA5), buf[0]);
}

TEST(ByteBuffer, ReadBits)
{
    ByteBuffer buf;
    buf.WriteBits(0b11010, 5);
    buf.WriteBits(0b110, 3);
    buf.FlushBits();

    uint32_t val1 = buf.ReadBits(5);
    uint32_t val2 = buf.ReadBits(3);
    EXPECT_EQ(uint32_t(0b11010), val1);
    EXPECT_EQ(uint32_t(0b110), val2);
}

TEST(ByteBuffer, ReadWriteBitRoundtrip)
{
    ByteBuffer buf;
    for (int i = 0; i < 16; ++i)
        buf.WriteBit(i % 3 == 0);
    buf.FlushBits();

    for (int i = 0; i < 16; ++i)
    {
        bool expected = (i % 3 == 0);
        bool actual = buf.ReadBit();
        EXPECT_EQ(expected, actual);
    }
}

// --- Packed GUID ---

TEST(ByteBuffer, PackGUIDZero)
{
    ByteBuffer buf;
    buf.appendPackGUID(0);
    EXPECT_EQ(size_t(1), buf.size());
    EXPECT_EQ(uint8_t(0), buf[0]); // mask byte only, no data bytes
}

TEST(ByteBuffer, PackGUIDSmall)
{
    ByteBuffer buf;
    buf.appendPackGUID(1);
    EXPECT_EQ(size_t(2), buf.size());
    EXPECT_EQ(uint8_t(0x01), buf[0]); // mask: byte 0 present
    EXPECT_EQ(uint8_t(0x01), buf[1]); // value
}

TEST(ByteBuffer, PackGUIDLarge)
{
    ByteBuffer buf;
    uint64_t guid = 0x0100000000000001ULL;
    buf.appendPackGUID(guid);
    // Byte 0 = 0x01, byte 7 = 0x01 -> mask should have bits 0 and 7 set
    EXPECT_EQ(uint8_t(0x81), buf[0]); // 10000001
    EXPECT_EQ(uint8_t(0x01), buf[1]); // low byte
    EXPECT_EQ(uint8_t(0x01), buf[2]); // high byte
}

TEST(ByteBuffer, PackGUIDAllBytes)
{
    ByteBuffer buf;
    buf.appendPackGUID(0xFFFFFFFFFFFFFFFFULL);
    EXPECT_EQ(size_t(9), buf.size()); // 1 mask + 8 data bytes
    EXPECT_EQ(uint8_t(0xFF), buf[0]); // all bytes present
}

// --- Array/Index operator ---

TEST(ByteBuffer, IndexOperator)
{
    ByteBuffer buf;
    buf << uint8_t(0xAA) << uint8_t(0xBB) << uint8_t(0xCC);
    EXPECT_EQ(uint8_t(0xAA), buf[0]);
    EXPECT_EQ(uint8_t(0xBB), buf[1]);
    EXPECT_EQ(uint8_t(0xCC), buf[2]);
}

TEST(ByteBuffer, IndexOperatorWrite)
{
    ByteBuffer buf;
    buf << uint8_t(0) << uint8_t(0) << uint8_t(0);
    buf[1] = 0xFF;
    EXPECT_EQ(uint8_t(0xFF), buf[1]);
}

// --- Contents ---

TEST(ByteBuffer, Contents)
{
    ByteBuffer buf;
    buf << uint8_t(1) << uint8_t(2) << uint8_t(3);
    const uint8_t* data = buf.contents();
    EXPECT_EQ(uint8_t(1), data[0]);
    EXPECT_EQ(uint8_t(2), data[1]);
    EXPECT_EQ(uint8_t(3), data[2]);
}

// --- Large Data ---

TEST(ByteBuffer, LargeDataWriteRead)
{
    ByteBuffer buf;
    const size_t count = 10000;
    for (uint32_t i = 0; i < count; ++i)
        buf << i;

    EXPECT_EQ(count * sizeof(uint32_t), buf.size());

    for (uint32_t i = 0; i < count; ++i)
    {
        uint32_t val;
        buf >> val;
        EXPECT_EQ(i, val);
    }
}

// --- Edge Cases ---

TEST(ByteBuffer, WriteMaxValues)
{
    ByteBuffer buf;
    buf << uint8_t(0xFF) << uint16_t(0xFFFF)
        << uint32_t(0xFFFFFFFF) << uint64_t(0xFFFFFFFFFFFFFFFFULL);

    uint8_t a; uint16_t b; uint32_t c; uint64_t d;
    buf >> a >> b >> c >> d;
    EXPECT_EQ(uint8_t(0xFF), a);
    EXPECT_EQ(uint16_t(0xFFFF), b);
    EXPECT_EQ(uint32_t(0xFFFFFFFF), c);
    EXPECT_EQ(uint64_t(0xFFFFFFFFFFFFFFFFULL), d);
}

TEST(ByteBuffer, WriteMinSignedValues)
{
    ByteBuffer buf;
    buf << int8_t(-128) << int16_t(-32768) << int32_t(-2147483648LL);

    int8_t a; int16_t b; int32_t c;
    buf >> a >> b >> c;
    EXPECT_EQ(int8_t(-128), a);
    EXPECT_EQ(int16_t(-32768), b);
    EXPECT_EQ(int32_t(-2147483648LL), c);
}

TEST(ByteBuffer, MultipleStrings)
{
    ByteBuffer buf;
    buf << std::string("first") << std::string("second") << std::string("third");

    std::string a, b, c;
    buf >> a >> b >> c;
    EXPECT_STR_EQ("first", a);
    EXPECT_STR_EQ("second", b);
    EXPECT_STR_EQ("third", c);
}

TEST(ByteBuffer, InterleavedTypesAndStrings)
{
    ByteBuffer buf;
    buf << uint32_t(100) << std::string("hello") << float(2.5f) << std::string("world");

    uint32_t a; std::string b; float c; std::string d;
    buf >> a >> b >> c >> d;
    EXPECT_EQ(uint32_t(100), a);
    EXPECT_STR_EQ("hello", b);
    EXPECT_FLOAT_EQ(2.5f, c);
    EXPECT_STR_EQ("world", d);
}

TEST(ByteBuffer, RawDataAppend)
{
    ByteBuffer buf;
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    buf.append(data, sizeof(data));
    EXPECT_EQ(size_t(5), buf.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(data[i], buf[i]);
}

TEST(ByteBuffer, RawDataRead)
{
    ByteBuffer buf;
    uint8_t input[] = {0xAA, 0xBB, 0xCC, 0xDD};
    buf.append(input, sizeof(input));

    uint8_t output[4] = {0};
    buf.read(output, sizeof(output));
    EXPECT_EQ(uint8_t(0xAA), output[0]);
    EXPECT_EQ(uint8_t(0xBB), output[1]);
    EXPECT_EQ(uint8_t(0xCC), output[2]);
    EXPECT_EQ(uint8_t(0xDD), output[3]);
}
