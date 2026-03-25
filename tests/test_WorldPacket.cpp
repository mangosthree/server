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
// Minimal OpcodesList for standalone testing
// ============================================================================

enum OpcodesList : uint16_t
{
    MSG_NULL_ACTION              = 0x000,
    SMSG_CHAR_CREATE             = 0x03A,
    SMSG_CHAR_ENUM               = 0x03B,
    SMSG_CHAR_DELETE             = 0x03C,
    CMSG_PLAYER_LOGIN            = 0x03D,
    SMSG_LOGIN_SETTIMESPEED      = 0x042,
    SMSG_TUTORIAL_FLAGS          = 0x0FD,
    CMSG_PING                    = 0x1DC,
    SMSG_PONG                    = 0x1DD,
    SMSG_MOVE_SET_RUN_SPEED      = 0x0DF,
    CMSG_MOVE_SET_RUN_SPEED_CHEAT = 0x0E0,
    SMSG_UPDATE_OBJECT           = 0x0A9,
    SMSG_COMPRESSED_UPDATE_OBJECT = 0x1F6,
    CMSG_LOGOUT_REQUEST          = 0x04B,
    SMSG_LOGOUT_RESPONSE         = 0x04C,
    SMSG_LOGOUT_COMPLETE         = 0x04D,
    CMSG_NAME_QUERY              = 0x050,
    SMSG_NAME_QUERY_RESPONSE     = 0x051,
    MSG_MOVE_WORLDPORT_ACK       = 0x0CA,
    SMSG_NEW_WORLD               = 0x03E,
};

inline const char* LookupOpcodeName(OpcodesList opcode)
{
    switch (opcode)
    {
        case MSG_NULL_ACTION:              return "MSG_NULL_ACTION";
        case SMSG_CHAR_CREATE:             return "SMSG_CHAR_CREATE";
        case SMSG_CHAR_ENUM:               return "SMSG_CHAR_ENUM";
        case CMSG_PLAYER_LOGIN:            return "CMSG_PLAYER_LOGIN";
        case SMSG_LOGIN_SETTIMESPEED:      return "SMSG_LOGIN_SETTIMESPEED";
        case CMSG_PING:                    return "CMSG_PING";
        case SMSG_PONG:                    return "SMSG_PONG";
        case SMSG_UPDATE_OBJECT:           return "SMSG_UPDATE_OBJECT";
        case CMSG_LOGOUT_REQUEST:          return "CMSG_LOGOUT_REQUEST";
        case SMSG_LOGOUT_RESPONSE:         return "SMSG_LOGOUT_RESPONSE";
        case SMSG_LOGOUT_COMPLETE:         return "SMSG_LOGOUT_COMPLETE";
        case CMSG_NAME_QUERY:              return "CMSG_NAME_QUERY";
        case SMSG_NAME_QUERY_RESPONSE:     return "SMSG_NAME_QUERY_RESPONSE";
        default:                           return "UNKNOWN";
    }
}

// Minimal ByteBuffer (same as test_ByteBuffer.cpp but self-contained)
class ByteBuffer
{
public:
    static const size_t DEFAULT_SIZE = 64;

    ByteBuffer() : _rpos(0), _wpos(0), _bitpos(8), _curbitval(0) { _storage.reserve(DEFAULT_SIZE); }
    explicit ByteBuffer(size_t res) : _rpos(0), _wpos(0), _bitpos(8), _curbitval(0) { _storage.reserve(res); }
    ByteBuffer(const ByteBuffer& b) : _rpos(b._rpos), _wpos(b._wpos), _storage(b._storage), _bitpos(b._bitpos), _curbitval(b._curbitval) {}

    void clear() { _storage.clear(); _rpos = _wpos = 0; _curbitval = 0; _bitpos = 8; }

    ByteBuffer& append(const uint8_t* src, size_t cnt)
    {
        if (!cnt) return *this;
        if (_storage.size() < _wpos + cnt) _storage.resize(_wpos + cnt);
        memcpy(&_storage[_wpos], src, cnt);
        _wpos += cnt;
        return *this;
    }

    template<typename T> ByteBuffer& append(T value) { FlushBits(); return append((uint8_t*)&value, sizeof(value)); }
    void FlushBits() { if (_bitpos == 8) return; append((uint8_t*)&_curbitval, 1); _curbitval = 0; _bitpos = 8; }

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
    ByteBuffer& operator<<(const std::string& v) { append((uint8_t*)v.c_str(), v.size()); append<uint8_t>(0); return *this; }

    ByteBuffer& operator>>(uint8_t& v) { v = read<uint8_t>(); return *this; }
    ByteBuffer& operator>>(uint16_t& v) { v = read<uint16_t>(); return *this; }
    ByteBuffer& operator>>(uint32_t& v) { v = read<uint32_t>(); return *this; }
    ByteBuffer& operator>>(uint64_t& v) { v = read<uint64_t>(); return *this; }
    ByteBuffer& operator>>(int32_t& v) { v = read<int32_t>(); return *this; }
    ByteBuffer& operator>>(float& v) { v = read<float>(); return *this; }
    ByteBuffer& operator>>(std::string& v)
    {
        v.clear();
        while (_rpos < size()) { char c = read<char>(); if (!c) break; v += c; }
        return *this;
    }

    template<typename T> T read()
    {
        if (_rpos + sizeof(T) > size()) throw std::runtime_error("ByteBuffer read overflow");
        T r; memcpy(&r, &_storage[_rpos], sizeof(T)); _rpos += sizeof(T); return r;
    }

    template<typename T> T read(size_t pos) const
    {
        if (pos + sizeof(T) > size()) throw std::runtime_error("ByteBuffer read overflow");
        T r; memcpy(&r, &_storage[pos], sizeof(T)); return r;
    }

    size_t rpos() const { return _rpos; }
    size_t rpos(size_t p) { _rpos = p; return _rpos; }
    size_t wpos() const { return _wpos; }
    size_t wpos(size_t p) { _wpos = p; return _wpos; }
    size_t size() const { return _storage.size(); }
    bool empty() const { return _storage.empty(); }
    void reserve(size_t s) { _storage.reserve(s); }
    void resize(size_t s) { _storage.resize(s); _rpos = 0; _wpos = size(); }
    const uint8_t* contents() const { return _storage.data(); }
    uint8_t& operator[](size_t p) { return _storage[p]; }

protected:
    size_t _rpos, _wpos;
    std::vector<uint8_t> _storage;
    uint8_t _bitpos, _curbitval;
};

// WorldPacket mirrors src/shared/Utilities/WorldPacket.h
class WorldPacket : public ByteBuffer
{
public:
    WorldPacket() : ByteBuffer(0), m_opcode(MSG_NULL_ACTION) {}
    explicit WorldPacket(OpcodesList opcode, size_t res = 200) : ByteBuffer(res), m_opcode(opcode) {}
    WorldPacket(const WorldPacket& p) : ByteBuffer(p), m_opcode(p.m_opcode) {}

    void Initialize(OpcodesList opcode, size_t newres = 200)
    {
        clear();
        _storage.reserve(newres);
        m_opcode = opcode;
    }

    OpcodesList GetOpcode() const { return m_opcode; }
    void SetOpcode(OpcodesList opcode) { m_opcode = opcode; }
    const char* GetOpcodeName() const { return LookupOpcodeName(m_opcode); }

protected:
    OpcodesList m_opcode;
};

// ============================================================================
// WorldPacket Tests
// ============================================================================

// --- Construction ---

TEST(WorldPacket, DefaultConstructor)
{
    WorldPacket pkt;
    EXPECT_EQ(MSG_NULL_ACTION, pkt.GetOpcode());
    EXPECT_TRUE(pkt.empty());
    EXPECT_EQ(size_t(0), pkt.size());
}

TEST(WorldPacket, OpcodeConstructor)
{
    WorldPacket pkt(SMSG_CHAR_ENUM);
    EXPECT_EQ(SMSG_CHAR_ENUM, pkt.GetOpcode());
    EXPECT_TRUE(pkt.empty());
}

TEST(WorldPacket, OpcodeAndSizeConstructor)
{
    WorldPacket pkt(SMSG_UPDATE_OBJECT, 512);
    EXPECT_EQ(SMSG_UPDATE_OBJECT, pkt.GetOpcode());
    EXPECT_TRUE(pkt.empty());
}

TEST(WorldPacket, CopyConstructor)
{
    WorldPacket original(CMSG_PING);
    original << uint32_t(12345) << uint32_t(9999);

    WorldPacket copy(original);
    EXPECT_EQ(original.GetOpcode(), copy.GetOpcode());
    EXPECT_EQ(original.size(), copy.size());

    uint32_t seq, latency;
    copy >> seq >> latency;
    EXPECT_EQ(uint32_t(12345), seq);
    EXPECT_EQ(uint32_t(9999), latency);
}

// --- Opcode Management ---

TEST(WorldPacket, GetOpcode)
{
    WorldPacket pkt(SMSG_PONG);
    EXPECT_EQ(SMSG_PONG, pkt.GetOpcode());
}

TEST(WorldPacket, SetOpcode)
{
    WorldPacket pkt(MSG_NULL_ACTION);
    EXPECT_EQ(MSG_NULL_ACTION, pkt.GetOpcode());
    pkt.SetOpcode(SMSG_LOGOUT_COMPLETE);
    EXPECT_EQ(SMSG_LOGOUT_COMPLETE, pkt.GetOpcode());
}

TEST(WorldPacket, GetOpcodeName_NullAction)
{
    WorldPacket pkt(MSG_NULL_ACTION);
    EXPECT_STR_EQ("MSG_NULL_ACTION", pkt.GetOpcodeName());
}

TEST(WorldPacket, GetOpcodeName_Ping)
{
    WorldPacket pkt(CMSG_PING);
    EXPECT_STR_EQ("CMSG_PING", pkt.GetOpcodeName());
}

TEST(WorldPacket, GetOpcodeName_Pong)
{
    WorldPacket pkt(SMSG_PONG);
    EXPECT_STR_EQ("SMSG_PONG", pkt.GetOpcodeName());
}

TEST(WorldPacket, GetOpcodeName_UpdateObject)
{
    WorldPacket pkt(SMSG_UPDATE_OBJECT);
    EXPECT_STR_EQ("SMSG_UPDATE_OBJECT", pkt.GetOpcodeName());
}

TEST(WorldPacket, GetOpcodeName_Unknown)
{
    WorldPacket pkt;
    pkt.SetOpcode(static_cast<OpcodesList>(0x9999));
    EXPECT_STR_EQ("UNKNOWN", pkt.GetOpcodeName());
}

// --- Initialize ---

TEST(WorldPacket, Initialize_ClearsData)
{
    WorldPacket pkt(CMSG_PING);
    pkt << uint32_t(1) << uint32_t(2);
    EXPECT_FALSE(pkt.empty());

    pkt.Initialize(SMSG_PONG);
    EXPECT_EQ(SMSG_PONG, pkt.GetOpcode());
    EXPECT_TRUE(pkt.empty());
}

TEST(WorldPacket, Initialize_SetsNewOpcode)
{
    WorldPacket pkt(MSG_NULL_ACTION);
    pkt.Initialize(SMSG_LOGOUT_RESPONSE, 64);
    EXPECT_EQ(SMSG_LOGOUT_RESPONSE, pkt.GetOpcode());
}

// --- Data Writing/Reading (inherited from ByteBuffer) ---

TEST(WorldPacket, WriteAndReadPingPacket)
{
    // Simulates: CMSG_PING contains sequence + latency
    WorldPacket pkt(CMSG_PING);
    uint32_t sequence = 42;
    uint32_t latency = 55;
    pkt << sequence << latency;

    EXPECT_EQ(CMSG_PING, pkt.GetOpcode());
    EXPECT_EQ(size_t(8), pkt.size());

    uint32_t seq_out, lat_out;
    pkt >> seq_out >> lat_out;
    EXPECT_EQ(sequence, seq_out);
    EXPECT_EQ(latency, lat_out);
}

TEST(WorldPacket, WriteAndReadNameQuery)
{
    // CMSG_NAME_QUERY: [uint64 guid]
    WorldPacket pkt(CMSG_NAME_QUERY);
    uint64_t guid = 0x0F13000000001234ULL;
    pkt << guid;

    EXPECT_EQ(size_t(8), pkt.size());
    uint64_t guid_out;
    pkt >> guid_out;
    EXPECT_EQ(guid, guid_out);
}

TEST(WorldPacket, WriteAndReadNameQueryResponse)
{
    // SMSG_NAME_QUERY_RESPONSE: [uint64 guid, string name, uint8 realm_str, uint32 race, uint32 gender, uint32 class]
    WorldPacket pkt(SMSG_NAME_QUERY_RESPONSE);
    uint64_t guid = 0x0F13000000001234ULL;
    std::string name = "Arthas";
    uint8_t realm_str = 0;
    uint32_t race = 1;   // human
    uint32_t gender = 0; // male
    uint32_t cls = 2;    // paladin

    pkt << guid << name << realm_str << race << gender << cls;

    uint64_t g; std::string n; uint8_t r_str; uint32_t r, ge, c;
    pkt >> g >> n >> r_str >> r >> ge >> c;

    EXPECT_EQ(guid, g);
    EXPECT_STR_EQ("Arthas", n);
    EXPECT_EQ(uint8_t(0), r_str);
    EXPECT_EQ(uint32_t(1), r);
    EXPECT_EQ(uint32_t(0), ge);
    EXPECT_EQ(uint32_t(2), c);
}

TEST(WorldPacket, WriteAndReadLoginSetTimeSpeed)
{
    // SMSG_LOGIN_SETTIMESPEED: [uint32 gametime, float speed]
    WorldPacket pkt(SMSG_LOGIN_SETTIMESPEED);
    uint32_t gametime = 0x5B3A1234;
    float speed = 0.01666667f; // 1/60

    pkt << gametime << speed;

    uint32_t gt_out; float speed_out;
    pkt >> gt_out >> speed_out;
    EXPECT_EQ(gametime, gt_out);
    EXPECT_NEAR(speed, speed_out, 0.000001f);
}

TEST(WorldPacket, WriteAndReadLogoutResponse)
{
    // SMSG_LOGOUT_RESPONSE: [uint32 reason, uint8 instant]
    WorldPacket pkt(SMSG_LOGOUT_RESPONSE);
    pkt << uint32_t(0) << uint8_t(0); // approved, not instant

    uint32_t reason; uint8_t instant;
    pkt >> reason >> instant;
    EXPECT_EQ(uint32_t(0), reason);
    EXPECT_EQ(uint8_t(0), instant);
}

TEST(WorldPacket, WriteAndReadTutorialFlags)
{
    // SMSG_TUTORIAL_FLAGS: [uint32[8] flags]
    WorldPacket pkt(SMSG_TUTORIAL_FLAGS);
    uint32_t flags[8] = {0xFF, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x11, 0x22};
    for (int i = 0; i < 8; ++i)
        pkt << flags[i];

    EXPECT_EQ(size_t(32), pkt.size());

    uint32_t read_flags[8];
    for (int i = 0; i < 8; ++i)
        pkt >> read_flags[i];

    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(flags[i], read_flags[i]);
}

// --- Opcode Numeric Values ---

TEST(WorldPacket, OpcodeValues)
{
    EXPECT_EQ(uint16_t(0x000), uint16_t(MSG_NULL_ACTION));
    EXPECT_EQ(uint16_t(0x03A), uint16_t(SMSG_CHAR_CREATE));
    EXPECT_EQ(uint16_t(0x03B), uint16_t(SMSG_CHAR_ENUM));
    EXPECT_EQ(uint16_t(0x1DC), uint16_t(CMSG_PING));
    EXPECT_EQ(uint16_t(0x1DD), uint16_t(SMSG_PONG));
    EXPECT_EQ(uint16_t(0x0A9), uint16_t(SMSG_UPDATE_OBJECT));
}

// --- Multiple Packets ---

TEST(WorldPacket, MultiplePacketsIndependent)
{
    WorldPacket pkt1(CMSG_PING);
    pkt1 << uint32_t(1) << uint32_t(100);

    WorldPacket pkt2(SMSG_PONG);
    pkt2 << uint32_t(1);

    // Packets should be independent
    EXPECT_EQ(CMSG_PING, pkt1.GetOpcode());
    EXPECT_EQ(SMSG_PONG, pkt2.GetOpcode());
    EXPECT_EQ(size_t(8), pkt1.size());
    EXPECT_EQ(size_t(4), pkt2.size());
}

TEST(WorldPacket, PacketReinitialized)
{
    WorldPacket pkt(CMSG_PING);
    pkt << uint32_t(100) << uint32_t(50);
    EXPECT_EQ(size_t(8), pkt.size());

    pkt.Initialize(SMSG_PONG, 100);
    EXPECT_EQ(SMSG_PONG, pkt.GetOpcode());
    EXPECT_TRUE(pkt.empty());

    pkt << uint32_t(100);
    EXPECT_EQ(size_t(4), pkt.size());
}

// --- WorldPacket Payload Types ---

TEST(WorldPacket, AppendString)
{
    WorldPacket pkt(SMSG_CHAR_CREATE);
    pkt << std::string("Thrall");
    EXPECT_EQ(size_t(7), pkt.size()); // 6 chars + null

    std::string name;
    pkt >> name;
    EXPECT_STR_EQ("Thrall", name);
}

TEST(WorldPacket, LargePayload)
{
    WorldPacket pkt(SMSG_UPDATE_OBJECT);
    const int N = 1000;
    for (int i = 0; i < N; ++i)
        pkt << uint32_t(i);

    EXPECT_EQ(size_t(N * 4), pkt.size());

    for (int i = 0; i < N; ++i)
    {
        uint32_t val;
        pkt >> val;
        EXPECT_EQ(uint32_t(i), val);
    }
}

TEST(WorldPacket, ContentsPointer)
{
    WorldPacket pkt(CMSG_PING);
    pkt << uint8_t(0xAA) << uint8_t(0xBB);
    const uint8_t* data = pkt.contents();
    EXPECT_EQ(uint8_t(0xAA), data[0]);
    EXPECT_EQ(uint8_t(0xBB), data[1]);
}
