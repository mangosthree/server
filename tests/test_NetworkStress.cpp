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
#include <cstring>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <array>

// ============================================================================
// Network/Packet stress tests: malformed packets, overflow payloads,
// truncated reads, and adversarial protocol inputs
// ============================================================================

enum OpcodesList : uint16_t
{
    MSG_NULL_ACTION        = 0x000,
    CMSG_PING              = 0x1DC,
    SMSG_PONG              = 0x1DD,
    CMSG_PLAYER_LOGIN      = 0x03D,
    CMSG_NAME_QUERY        = 0x050,
    SMSG_NAME_QUERY_RESPONSE = 0x051,
    CMSG_LOGOUT_REQUEST    = 0x04B,
    SMSG_UPDATE_OBJECT     = 0x0A9,
    CMSG_MESSAGECHAT       = 0x095,
    CMSG_MOVE_HEARTBEAT    = 0x0EE,
};

class ByteBuffer
{
public:
    ByteBuffer() : _rpos(0), _wpos(0) { _storage.reserve(64); }
    explicit ByteBuffer(size_t res) : _rpos(0), _wpos(0) { _storage.reserve(res); }
    ByteBuffer(const ByteBuffer& b) : _rpos(b._rpos), _wpos(b._wpos), _storage(b._storage) {}

    void clear() { _storage.clear(); _rpos = _wpos = 0; }

    template<typename T> ByteBuffer& append(T value)
    { return append((uint8_t*)&value, sizeof(value)); }

    ByteBuffer& append(const uint8_t* src, size_t cnt)
    {
        if (!cnt) return *this;
        if (_storage.size() < _wpos + cnt) _storage.resize(_wpos + cnt);
        memcpy(&_storage[_wpos], src, cnt);
        _wpos += cnt;
        return *this;
    }

    ByteBuffer& operator<<(uint8_t v) { append<uint8_t>(v); return *this; }
    ByteBuffer& operator<<(uint16_t v) { append<uint16_t>(v); return *this; }
    ByteBuffer& operator<<(uint32_t v) { append<uint32_t>(v); return *this; }
    ByteBuffer& operator<<(uint64_t v) { append<uint64_t>(v); return *this; }
    ByteBuffer& operator<<(float v) { append<float>(v); return *this; }
    ByteBuffer& operator<<(const std::string& v)
    { append((uint8_t*)v.c_str(), v.size()); append<uint8_t>(0); return *this; }

    ByteBuffer& operator>>(uint8_t& v) { v = read<uint8_t>(); return *this; }
    ByteBuffer& operator>>(uint16_t& v) { v = read<uint16_t>(); return *this; }
    ByteBuffer& operator>>(uint32_t& v) { v = read<uint32_t>(); return *this; }
    ByteBuffer& operator>>(uint64_t& v) { v = read<uint64_t>(); return *this; }
    ByteBuffer& operator>>(float& v) { v = read<float>(); return *this; }
    ByteBuffer& operator>>(std::string& v)
    {
        v.clear();
        while (_rpos < size()) { char c = read<char>(); if (!c) break; v += c; }
        return *this;
    }

    template<typename T> T read()
    {
        if (_rpos + sizeof(T) > size()) throw std::runtime_error("read overflow");
        T r; memcpy(&r, &_storage[_rpos], sizeof(T)); _rpos += sizeof(T); return r;
    }

    size_t rpos() const { return _rpos; }
    size_t rpos(size_t p) { _rpos = p; return _rpos; }
    size_t wpos() const { return _wpos; }
    size_t size() const { return _storage.size(); }
    bool empty() const { return _storage.empty(); }
    const uint8_t* contents() const { return _storage.data(); }

protected:
    size_t _rpos, _wpos;
    std::vector<uint8_t> _storage;
};

class WorldPacket : public ByteBuffer
{
public:
    WorldPacket() : ByteBuffer(0), m_opcode(MSG_NULL_ACTION) {}
    explicit WorldPacket(OpcodesList opcode, size_t res = 200) : ByteBuffer(res), m_opcode(opcode) {}

    void Initialize(OpcodesList opcode, size_t newres = 200)
    { clear(); _storage.reserve(newres); m_opcode = opcode; }

    OpcodesList GetOpcode() const { return m_opcode; }
    void SetOpcode(OpcodesList opcode) { m_opcode = opcode; }

protected:
    OpcodesList m_opcode;
};

// Packet header parsing (simulates the server's packet size validation)
struct PacketHeader
{
    uint16_t size;
    uint16_t opcode;
};

bool ValidatePacketHeader(const PacketHeader& header)
{
    if (header.size > 10240) return false; // max 10KB packet
    if (header.size < 4) return false; // minimum: opcode + something
    return true;
}

// Chat message sanitization
bool SanitizeChatMessage(std::string& msg)
{
    if (msg.empty()) return false;
    if (msg.size() > 255) { msg.resize(255); }
    // Strip control characters
    msg.erase(std::remove_if(msg.begin(), msg.end(),
        [](char c) { return c < 32 && c != '\0'; }), msg.end());
    return !msg.empty();
}

// Movement validation
struct MovementInfo
{
    float x, y, z, o;
    float speed;
    uint32_t flags;
    uint32_t time;
};

bool ValidateMovement(const MovementInfo& info, float maxSpeed)
{
    if (!std::isfinite(info.x) || !std::isfinite(info.y) || !std::isfinite(info.z))
        return false;
    if (!std::isfinite(info.o))
        return false;
    if (!std::isfinite(info.speed) || info.speed < 0.0f)
        return false;
    if (info.speed > maxSpeed * 1.1f) // 10% tolerance
        return false;
    // Map bounds check
    if (std::fabs(info.x) > 17066.0f || std::fabs(info.y) > 17066.0f)
        return false;
    return true;
}

// Session key validation
bool ValidateSessionKey(const uint8_t* key, size_t len)
{
    if (len != 40) return false;
    // Check for all zeros (invalid key)
    bool allZero = true;
    for (size_t i = 0; i < len; ++i)
        if (key[i] != 0) { allZero = false; break; }
    return !allZero;
}

// ============================================================================
// Packet Header Validation Tests
// ============================================================================

TEST(PacketStress, ValidHeader)
{
    PacketHeader h = {100, CMSG_PING};
    EXPECT_TRUE(ValidatePacketHeader(h));
}

TEST(PacketStress, OversizedPacket)
{
    PacketHeader h = {20000, CMSG_PING};
    EXPECT_FALSE(ValidatePacketHeader(h));
}

TEST(PacketStress, MaxSizePacket)
{
    PacketHeader h = {10240, CMSG_PING};
    EXPECT_TRUE(ValidatePacketHeader(h));
}

TEST(PacketStress, ZeroSizePacket)
{
    PacketHeader h = {0, CMSG_PING};
    EXPECT_FALSE(ValidatePacketHeader(h));
}

TEST(PacketStress, MinimalPacket)
{
    PacketHeader h = {4, CMSG_PING};
    EXPECT_TRUE(ValidatePacketHeader(h));
}

TEST(PacketStress, TooSmallPacket)
{
    PacketHeader h = {3, CMSG_PING};
    EXPECT_FALSE(ValidatePacketHeader(h));
}

TEST(PacketStress, MaxUint16Size)
{
    PacketHeader h = {0xFFFF, CMSG_PING};
    EXPECT_FALSE(ValidatePacketHeader(h));
}

// ============================================================================
// Malformed Packet Payload Tests
// ============================================================================

TEST(PacketStress, TruncatedPingPacket)
{
    WorldPacket pkt(CMSG_PING);
    pkt << uint32_t(42); // sequence only, missing latency
    uint32_t seq;
    pkt >> seq;
    EXPECT_EQ(uint32_t(42), seq);
    // Reading latency should fail
    bool caught = false;
    try { uint32_t lat; pkt >> lat; } catch (...) { caught = true; }
    EXPECT_TRUE(caught);
}

TEST(PacketStress, EmptyPacket)
{
    WorldPacket pkt(CMSG_PING);
    // Try reading from empty packet
    bool caught = false;
    try { uint32_t v; pkt >> v; } catch (...) { caught = true; }
    EXPECT_TRUE(caught);
}

TEST(PacketStress, OversizedStringInPacket)
{
    WorldPacket pkt(CMSG_MESSAGECHAT);
    std::string longMsg(10000, 'A');
    pkt << longMsg;
    std::string out;
    pkt >> out;
    EXPECT_EQ(size_t(10000), out.size());
}

TEST(PacketStress, StringWithNoTerminator)
{
    WorldPacket pkt(CMSG_NAME_QUERY);
    // Write raw bytes without null terminator
    uint8_t rawData[] = {'H', 'e', 'l', 'l', 'o'};
    pkt.append(rawData, sizeof(rawData));
    std::string out;
    pkt >> out;
    // Should read until end of buffer
    EXPECT_EQ(size_t(5), out.size());
    EXPECT_EQ('H', out[0]);
}

TEST(PacketStress, ReinitializedPacketReuse)
{
    WorldPacket pkt(CMSG_PING);
    pkt << uint32_t(1) << uint32_t(2);

    for (int i = 0; i < 100; ++i)
    {
        pkt.Initialize(CMSG_PING);
        EXPECT_TRUE(pkt.empty());
        pkt << uint32_t(i) << uint32_t(i * 2);
        uint32_t a, b;
        pkt >> a >> b;
        EXPECT_EQ(uint32_t(i), a);
        EXPECT_EQ(uint32_t(i * 2), b);
    }
}

TEST(PacketStress, PacketWithGarbageOpcode)
{
    WorldPacket pkt;
    pkt.SetOpcode(static_cast<OpcodesList>(0xFFFF));
    EXPECT_EQ(uint16_t(0xFFFF), uint16_t(pkt.GetOpcode()));
}

// ============================================================================
// Chat Sanitization Stress Tests
// ============================================================================

TEST(ChatStress, EmptyMessage)
{
    std::string msg = "";
    EXPECT_FALSE(SanitizeChatMessage(msg));
}

TEST(ChatStress, OnlyControlChars)
{
    std::string msg = "\x01\x02\x03\x04\x05";
    EXPECT_FALSE(SanitizeChatMessage(msg));
}

TEST(ChatStress, TruncateLongMessage)
{
    std::string msg(1000, 'A');
    SanitizeChatMessage(msg);
    EXPECT_EQ(size_t(255), msg.size());
}

TEST(ChatStress, MixedControlAndText)
{
    std::string msg = "Hello\x01World\x02!";
    SanitizeChatMessage(msg);
    EXPECT_STR_EQ("HelloWorld!", msg);
}

TEST(ChatStress, NormalMessage)
{
    std::string msg = "Hello World!";
    EXPECT_TRUE(SanitizeChatMessage(msg));
    EXPECT_STR_EQ("Hello World!", msg);
}

TEST(ChatStress, MaxLengthExact)
{
    std::string msg(255, 'X');
    EXPECT_TRUE(SanitizeChatMessage(msg));
    EXPECT_EQ(size_t(255), msg.size());
}

// ============================================================================
// Movement Validation Stress Tests
// ============================================================================

TEST(MovementStress, ValidMovement)
{
    MovementInfo info = {100.0f, 200.0f, 50.0f, 1.5f, 7.0f, 0, 12345};
    EXPECT_TRUE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, NaNPosition)
{
    MovementInfo info = {std::numeric_limits<float>::quiet_NaN(), 0, 0, 0, 7.0f, 0, 0};
    EXPECT_FALSE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, InfinityPosition)
{
    MovementInfo info = {std::numeric_limits<float>::infinity(), 0, 0, 0, 7.0f, 0, 0};
    EXPECT_FALSE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, NaNOrientation)
{
    MovementInfo info = {0, 0, 0, std::numeric_limits<float>::quiet_NaN(), 7.0f, 0, 0};
    EXPECT_FALSE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, NegativeSpeed)
{
    MovementInfo info = {0, 0, 0, 0, -1.0f, 0, 0};
    EXPECT_FALSE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, SpeedHackDetection)
{
    // Speed exceeds max by more than 10%
    MovementInfo info = {0, 0, 0, 0, 15.0f, 0, 0};
    EXPECT_FALSE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, SpeedWithinTolerance)
{
    MovementInfo info = {0, 0, 0, 0, 7.5f, 0, 0};
    EXPECT_TRUE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, OutOfMapBounds)
{
    MovementInfo info = {20000.0f, 0, 0, 0, 7.0f, 0, 0};
    EXPECT_FALSE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, MapBoundaryExact)
{
    MovementInfo info = {17066.0f, 0, 0, 0, 7.0f, 0, 0};
    EXPECT_TRUE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, NaNSpeed)
{
    MovementInfo info = {0, 0, 0, 0, std::numeric_limits<float>::quiet_NaN(), 0, 0};
    EXPECT_FALSE(ValidateMovement(info, 7.0f));
}

TEST(MovementStress, ZeroSpeed)
{
    MovementInfo info = {0, 0, 0, 0, 0.0f, 0, 0};
    EXPECT_TRUE(ValidateMovement(info, 7.0f));
}

// ============================================================================
// Session Key Validation
// ============================================================================

TEST(SessionKeyStress, ValidKey)
{
    uint8_t key[40];
    for (int i = 0; i < 40; ++i) key[i] = uint8_t(i + 1);
    EXPECT_TRUE(ValidateSessionKey(key, 40));
}

TEST(SessionKeyStress, AllZeroKey)
{
    uint8_t key[40] = {};
    EXPECT_FALSE(ValidateSessionKey(key, 40));
}

TEST(SessionKeyStress, WrongLength)
{
    uint8_t key[20];
    for (int i = 0; i < 20; ++i) key[i] = uint8_t(i + 1);
    EXPECT_FALSE(ValidateSessionKey(key, 20));
}

TEST(SessionKeyStress, SingleNonZeroByte)
{
    uint8_t key[40] = {};
    key[39] = 0x01;
    EXPECT_TRUE(ValidateSessionKey(key, 40));
}

// ============================================================================
// XOR Cipher Stress Tests
// ============================================================================

class SimpleCipher
{
public:
    void Init(const uint8_t* key, size_t keyLen) { _key.assign(key, key+keyLen); _pos = 0; }
    void Process(uint8_t* data, size_t len)
    { for (size_t i = 0; i < len; ++i) { data[i] ^= _key[_pos % _key.size()]; ++_pos; } }
private:
    std::vector<uint8_t> _key;
    size_t _pos = 0;
};

TEST(CipherStress, LargeDataEncryptDecrypt)
{
    uint8_t key[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};
    std::vector<uint8_t> data(100000);
    for (size_t i = 0; i < data.size(); ++i) data[i] = uint8_t(i * 7 + 3);
    std::vector<uint8_t> original = data;

    SimpleCipher enc, dec;
    enc.Init(key, sizeof(key));
    dec.Init(key, sizeof(key));

    enc.Process(data.data(), data.size());
    EXPECT_NE(0, memcmp(data.data(), original.data(), data.size()));

    dec.Process(data.data(), data.size());
    EXPECT_EQ(0, memcmp(data.data(), original.data(), data.size()));
}

TEST(CipherStress, SingleByteKey)
{
    uint8_t key[] = {0xFF};
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    uint8_t original[3];
    memcpy(original, data, 3);

    SimpleCipher enc, dec;
    enc.Init(key, 1);
    dec.Init(key, 1);

    enc.Process(data, 3);
    dec.Process(data, 3);
    EXPECT_EQ(0, memcmp(data, original, 3));
}

TEST(CipherStress, ChunkedProcessing)
{
    uint8_t key[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<uint8_t> data(1000);
    for (size_t i = 0; i < data.size(); ++i) data[i] = uint8_t(i);
    std::vector<uint8_t> original = data;

    SimpleCipher enc;
    enc.Init(key, sizeof(key));
    // Encrypt in small chunks
    for (size_t offset = 0; offset < data.size(); offset += 7)
    {
        size_t chunk = std::min(size_t(7), data.size() - offset);
        enc.Process(data.data() + offset, chunk);
    }

    SimpleCipher dec;
    dec.Init(key, sizeof(key));
    dec.Process(data.data(), data.size());

    EXPECT_EQ(0, memcmp(data.data(), original.data(), data.size()));
}

// ============================================================================
// Packet flood simulation
// ============================================================================

TEST(FloodStress, CreateDestroyThousandPackets)
{
    for (int i = 0; i < 10000; ++i)
    {
        WorldPacket pkt(CMSG_PING);
        pkt << uint32_t(i) << uint32_t(100);
        EXPECT_EQ(size_t(8), pkt.size());
    }
}

TEST(FloodStress, RapidReinitialize)
{
    WorldPacket pkt;
    for (int i = 0; i < 10000; ++i)
    {
        pkt.Initialize(static_cast<OpcodesList>(i % 256));
        pkt << uint32_t(i);
    }
    // Should not leak memory or crash
    EXPECT_TRUE(true);
}

TEST(FloodStress, MassivePayloadAccumulation)
{
    WorldPacket pkt(SMSG_UPDATE_OBJECT);
    for (int i = 0; i < 50000; ++i)
        pkt << uint8_t(i & 0xFF);
    EXPECT_EQ(size_t(50000), pkt.size());
    // Verify data integrity
    pkt.rpos(0);
    for (int i = 0; i < 50000; ++i)
    {
        uint8_t v;
        pkt >> v;
        EXPECT_EQ(uint8_t(i & 0xFF), v);
    }
}
