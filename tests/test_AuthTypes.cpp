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
#include <array>
#include <cassert>

// ============================================================================
// Auth / BigNumber / SRP6 pattern tests (no OpenSSL dependency)
// These test the logic patterns used in the authentication system.
// ============================================================================

// Simple big-integer-like concept using byte vectors (for testing purposes)
typedef std::vector<uint8_t> ByteArray;

// XOR-based stream cipher pattern (simplified AuthCrypt simulation)
class SimpleCrypt
{
public:
    void Init(const uint8_t* key, size_t keyLen)
    {
        _key.assign(key, key + keyLen);
        _pos = 0;
    }

    void Encrypt(uint8_t* data, size_t len)
    {
        for (size_t i = 0; i < len; ++i)
        {
            data[i] ^= _key[_pos % _key.size()];
            ++_pos;
        }
    }

    void Decrypt(uint8_t* data, size_t len)
    {
        Encrypt(data, len); // XOR is its own inverse
    }

private:
    std::vector<uint8_t> _key;
    size_t _pos = 0;
};

// SRP6 helper patterns (conceptual, no OpenSSL)
struct SRP6Params
{
    uint32_t g = 7;          // generator
    std::string N_hex;       // safe prime (hex string)
};

// MD5-like 16-byte hash (trivial stand-in for pattern testing)
std::array<uint8_t, 16> SimpleHash(const uint8_t* data, size_t len)
{
    std::array<uint8_t, 16> result = {};
    for (size_t i = 0; i < len; ++i)
        result[i % 16] = (result[i % 16] + data[i] * uint8_t(i + 1)) & 0xFF;
    return result;
}

// SHA1-like 20-byte hash (trivial stand-in)
std::array<uint8_t, 20> SimpleHash20(const uint8_t* data, size_t len)
{
    std::array<uint8_t, 20> result = {};
    for (size_t i = 0; i < len; ++i)
        result[i % 20] = (result[i % 20] + data[i] * uint8_t(i + 1)) & 0xFF;
    return result;
}

// Account name normalization (uppercase, as used in auth)
std::string NormalizeAccountName(const std::string& name)
{
    std::string result = name;
    for (char& c : result)
        c = toupper(c);
    return result;
}

// Simple HMAC-like pattern
std::array<uint8_t, 20> SimpleHMAC(const uint8_t* key, size_t keyLen,
                                    const uint8_t* data, size_t dataLen)
{
    std::array<uint8_t, 20> ipad_key = {};
    std::array<uint8_t, 20> opad_key = {};
    for (size_t i = 0; i < 20; ++i)
    {
        uint8_t k = (i < keyLen) ? key[i] : 0;
        ipad_key[i] = k ^ 0x36;
        opad_key[i] = k ^ 0x5C;
    }

    // Inner hash: H(ipad_key || data)
    std::vector<uint8_t> inner(20 + dataLen);
    memcpy(inner.data(), ipad_key.data(), 20);
    memcpy(inner.data() + 20, data, dataLen);
    auto inner_hash = SimpleHash20(inner.data(), inner.size());

    // Outer hash: H(opad_key || inner_hash)
    std::vector<uint8_t> outer(20 + 20);
    memcpy(outer.data(), opad_key.data(), 20);
    memcpy(outer.data() + 20, inner_hash.data(), 20);
    return SimpleHash20(outer.data(), outer.size());
}

// ============================================================================
// SimpleCrypt (AuthCrypt pattern) Tests
// ============================================================================

TEST(AuthCrypt, EncryptAndDecryptRoundtrip)
{
    uint8_t key[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint8_t original[6];
    memcpy(original, data, sizeof(data));

    SimpleCrypt enc, dec;
    enc.Init(key, sizeof(key));
    dec.Init(key, sizeof(key));

    enc.Encrypt(data, sizeof(data));
    // After encrypt, data should differ
    bool differs = (memcmp(data, original, sizeof(data)) != 0);
    EXPECT_TRUE(differs);

    dec.Decrypt(data, sizeof(data));
    EXPECT_EQ(0, memcmp(data, original, sizeof(data)));
}

TEST(AuthCrypt, EncryptChangesData)
{
    uint8_t key[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t original[4];
    memcpy(original, data, sizeof(data));

    SimpleCrypt enc;
    enc.Init(key, sizeof(key));
    enc.Encrypt(data, sizeof(data));

    // Encrypted data should differ from original (unless key is all zeros)
    bool differs = false;
    for (int i = 0; i < 4; ++i)
        if (data[i] != original[i]) { differs = true; break; }
    EXPECT_TRUE(differs);
}

TEST(AuthCrypt, EmptyDataNoChange)
{
    uint8_t key[] = {0x01};
    SimpleCrypt enc;
    enc.Init(key, sizeof(key));
    enc.Encrypt(nullptr, 0); // should not crash
}

TEST(AuthCrypt, DifferentKeysProduceDifferentOutput)
{
    uint8_t key1[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t key2[] = {0xFF, 0xFE, 0xFD, 0xFC};
    uint8_t data1[] = {0xAB, 0xCD, 0xEF, 0x01};
    uint8_t data2[] = {0xAB, 0xCD, 0xEF, 0x01};

    SimpleCrypt enc1, enc2;
    enc1.Init(key1, sizeof(key1));
    enc2.Init(key2, sizeof(key2));
    enc1.Encrypt(data1, sizeof(data1));
    enc2.Encrypt(data2, sizeof(data2));

    EXPECT_NE(0, memcmp(data1, data2, sizeof(data1)));
}

TEST(AuthCrypt, StatefulPosition)
{
    uint8_t key[] = {0x01, 0x02, 0x03};
    uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t original[6];
    memcpy(original, data, sizeof(data));

    SimpleCrypt enc;
    enc.Init(key, sizeof(key));

    // Encrypt first 3 bytes
    enc.Encrypt(data, 3);
    // Encrypt next 3 bytes (key wraps)
    enc.Encrypt(data + 3, 3);

    // Reset and decrypt all 6 bytes at once
    SimpleCrypt dec;
    dec.Init(key, sizeof(key));
    dec.Decrypt(data, 6);

    EXPECT_EQ(0, memcmp(data, original, 6));
}

// ============================================================================
// Account Name Normalization Tests
// ============================================================================

TEST(AccountName, Uppercase)
{
    EXPECT_STR_EQ("ARTHAS", NormalizeAccountName("arthas"));
}

TEST(AccountName, AlreadyUppercase)
{
    EXPECT_STR_EQ("ARTHAS", NormalizeAccountName("ARTHAS"));
}

TEST(AccountName, Mixed)
{
    EXPECT_STR_EQ("ILLIDAN", NormalizeAccountName("iLLiDaN"));
}

TEST(AccountName, Empty)
{
    EXPECT_STR_EQ("", NormalizeAccountName(""));
}

TEST(AccountName, WithNumbers)
{
    EXPECT_STR_EQ("PLAYER123", NormalizeAccountName("player123"));
}

// ============================================================================
// SimpleHash Pattern Tests
// ============================================================================

TEST(SimpleHash, DifferentInputsDifferentHashes)
{
    uint8_t data1[] = {0x01, 0x02, 0x03};
    uint8_t data2[] = {0x01, 0x02, 0x04};
    auto h1 = SimpleHash(data1, sizeof(data1));
    auto h2 = SimpleHash(data2, sizeof(data2));
    EXPECT_TRUE(h1 != h2);
}

TEST(SimpleHash, SameInputSameHash)
{
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    auto h1 = SimpleHash(data, sizeof(data));
    auto h2 = SimpleHash(data, sizeof(data));
    EXPECT_TRUE(h1 == h2);
}

TEST(SimpleHash, OutputIs16Bytes)
{
    uint8_t data[] = {0x01};
    auto h = SimpleHash(data, sizeof(data));
    EXPECT_EQ(size_t(16), h.size());
}

TEST(SimpleHash20, OutputIs20Bytes)
{
    uint8_t data[] = {0x01};
    auto h = SimpleHash20(data, sizeof(data));
    EXPECT_EQ(size_t(20), h.size());
}

TEST(SimpleHash20, SameInputSameOutput)
{
    uint8_t data[] = {0xCA, 0xFE, 0xBA, 0xBE};
    auto h1 = SimpleHash20(data, sizeof(data));
    auto h2 = SimpleHash20(data, sizeof(data));
    EXPECT_TRUE(h1 == h2);
}

// ============================================================================
// HMAC Pattern Tests
// ============================================================================

TEST(HMAC, SameKeyAndDataSameResult)
{
    uint8_t key[] = {0x01, 0x02, 0x03};
    uint8_t data[] = {0xAB, 0xCD, 0xEF};
    auto h1 = SimpleHMAC(key, sizeof(key), data, sizeof(data));
    auto h2 = SimpleHMAC(key, sizeof(key), data, sizeof(data));
    EXPECT_TRUE(h1 == h2);
}

TEST(HMAC, DifferentKeysDifferentResult)
{
    uint8_t key1[] = {0x01, 0x02, 0x03};
    uint8_t key2[] = {0x04, 0x05, 0x06};
    uint8_t data[] = {0xAB, 0xCD, 0xEF};
    auto h1 = SimpleHMAC(key1, sizeof(key1), data, sizeof(data));
    auto h2 = SimpleHMAC(key2, sizeof(key2), data, sizeof(data));
    EXPECT_TRUE(h1 != h2);
}

TEST(HMAC, DifferentDataDifferentResult)
{
    uint8_t key[] = {0x01, 0x02, 0x03};
    uint8_t data1[] = {0xAB, 0xCD, 0xEF};
    uint8_t data2[] = {0xAB, 0xCD, 0x00};
    auto h1 = SimpleHMAC(key, sizeof(key), data1, sizeof(data1));
    auto h2 = SimpleHMAC(key, sizeof(key), data2, sizeof(data2));
    EXPECT_TRUE(h1 != h2);
}

TEST(HMAC, OutputIs20Bytes)
{
    uint8_t key[] = {0x01};
    uint8_t data[] = {0x01};
    auto h = SimpleHMAC(key, sizeof(key), data, sizeof(data));
    EXPECT_EQ(size_t(20), h.size());
}

// ============================================================================
// Realm Handshake Packet Pattern Tests
// (Patterns from realmd/Auth/AuthSocket.cpp)
// ============================================================================

struct AuthLogonChallenge
{
    uint8_t  cmd;
    uint8_t  error;
    uint16_t size;
    uint8_t  gamename[4];
    uint8_t  version1;
    uint8_t  version2;
    uint8_t  version3;
    uint16_t build;
    uint8_t  platform[4];
    uint8_t  os[4];
    uint8_t  country[4];
    uint32_t timezone_bias;
    uint32_t ip;
    uint8_t  I_len;
    // followed by I_len bytes of account name
};

TEST(AuthLogonChallenge, StructSize)
{
    // The struct should be at least 30 bytes (base without variable name)
    EXPECT_GE(sizeof(AuthLogonChallenge), size_t(30));
}

TEST(AuthLogonChallenge, FieldAccess)
{
    AuthLogonChallenge challenge = {};
    challenge.cmd = 0x00;
    challenge.error = 0x00;
    challenge.size = 0x001E;
    challenge.version1 = 4;
    challenge.version2 = 3;
    challenge.version3 = 4;
    challenge.build = 15595;
    challenge.timezone_bias = 0;
    challenge.ip = 0x7F000001; // 127.0.0.1
    challenge.I_len = 5;

    EXPECT_EQ(uint8_t(0x00), challenge.cmd);
    EXPECT_EQ(uint8_t(4), challenge.version1);
    EXPECT_EQ(uint8_t(3), challenge.version2);
    EXPECT_EQ(uint8_t(4), challenge.version3);
    EXPECT_EQ(uint16_t(15595), challenge.build);
    EXPECT_EQ(uint32_t(0x7F000001), challenge.ip);
    EXPECT_EQ(uint8_t(5), challenge.I_len);
}

// ============================================================================
// Session Key Pattern Tests (WoW uses 40-byte session key)
// ============================================================================

TEST(SessionKey, FortyByteKey)
{
    // WoW uses a 40-byte session key derived from SRP6
    uint8_t sessionKey[40] = {};
    for (int i = 0; i < 40; ++i)
        sessionKey[i] = (uint8_t)i;

    EXPECT_EQ(uint8_t(0), sessionKey[0]);
    EXPECT_EQ(uint8_t(39), sessionKey[39]);
}

TEST(SessionKey, XorPatternForHeader)
{
    // WoW client/server encrypts packet headers using session key XOR
    uint8_t sessionKey[40];
    for (int i = 0; i < 40; ++i) sessionKey[i] = (uint8_t)(i * 7 + 3);

    uint8_t header[] = {0x00, 0x04, 0x0D, 0xF0}; // example header
    uint8_t original[4];
    memcpy(original, header, sizeof(header));

    // XOR encrypt
    for (int i = 0; i < 4; ++i)
        header[i] ^= sessionKey[i];

    // XOR decrypt
    for (int i = 0; i < 4; ++i)
        header[i] ^= sessionKey[i];

    EXPECT_EQ(0, memcmp(header, original, 4));
}
