/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include "WardenCryptoContext.h"
#include "WardenX86Transform.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace
{
bool Sha1(uint8 const* data, std::size_t size, warden::Digest20& digest)
{
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context)
        return false;

    unsigned int length = 0;
    bool const success =
        EVP_DigestInit_ex(context, EVP_sha1(), nullptr) == 1 &&
        EVP_DigestUpdate(context, data, size) == 1 &&
        EVP_DigestFinal_ex(context, digest.data(), &length) == 1 &&
        length == digest.size();
    EVP_MD_CTX_free(context);
    return success;
}

uint32 ReadLittleEndian32(uint8 const* bytes)
{
    return uint32(bytes[0]) |
        (uint32(bytes[1]) << 8) |
        (uint32(bytes[2]) << 16) |
        (uint32(bytes[3]) << 24);
}

void WriteLittleEndian32(uint8* bytes, uint32 value)
{
    bytes[0] = uint8(value);
    bytes[1] = uint8(value >> 8);
    bytes[2] = uint8(value >> 16);
    bytes[3] = uint8(value >> 24);
}

warden::Key16 TransformX64Seed(warden::Key16 const& seed)
{
    warden::Key16 transformed{};
    WriteLittleEndian32(transformed.data(),
        ReadLittleEndian32(seed.data()) ^ 0xDEADBEEFu);
    WriteLittleEndian32(transformed.data() + 4,
        ReadLittleEndian32(seed.data() + 4) - 0x35014542u);
    WriteLittleEndian32(transformed.data() + 8,
        ReadLittleEndian32(seed.data() + 8) + 0x05313F22u);
    WriteLittleEndian32(transformed.data() + 12,
        ReadLittleEndian32(seed.data() + 12) * 0x1337F00Du);
    return transformed;
}

bool DeriveInitialKeys(warden::SessionKey const& sessionKey,
    warden::Key16& clientToServer, warden::Key16& serverToClient)
{
    // The bootstrap preserves the legacy fixed-width KDF: hash each 20-byte
    // half, then expand SHA1(left || previous || right) to 32 bytes.
    warden::Digest20 left{};
    warden::Digest20 right{};
    warden::Digest20 current{};
    std::array<uint8, 60> input{};
    std::array<uint8, 32> generated{};

    bool success = Sha1(sessionKey.data(), 20, left) &&
        Sha1(sessionKey.data() + 20, 20, right);
    std::size_t offset = 0;
    while (success && offset < generated.size())
    {
        std::copy(left.begin(), left.end(), input.begin());
        std::copy(current.begin(), current.end(), input.begin() + 20);
        std::copy(right.begin(), right.end(), input.begin() + 40);
        success = Sha1(input.data(), input.size(), current);
        std::size_t const count =
            std::min(current.size(), generated.size() - offset);
        if (success)
        {
            std::copy(current.begin(), current.begin() + count,
                generated.begin() + offset);
            offset += count;
        }
    }

    if (success)
    {
        std::copy(generated.begin(), generated.begin() + clientToServer.size(),
            clientToServer.begin());
        std::copy(generated.begin() + clientToServer.size(), generated.end(),
            serverToClient.begin());
    }

    OPENSSL_cleanse(left.data(), left.size());
    OPENSSL_cleanse(right.data(), right.size());
    OPENSSL_cleanse(current.data(), current.size());
    OPENSSL_cleanse(input.data(), input.size());
    OPENSSL_cleanse(generated.data(), generated.size());
    return success;
}
}

namespace warden
{
std::optional<ArchitectureProof> DeriveArchitectureProof(
    WardenArchitecture architecture, Key16 const& seed)
{
    if (architecture != WardenArchitecture::X86 &&
        architecture != WardenArchitecture::X64)
        return std::nullopt;

    ArchitectureProof proof;
    if (architecture == WardenArchitecture::X86)
    {
        proof.clientToServer = TransformX86ArchitectureSeed(seed);
        proof.serverToClient =
            TransformX86ArchitectureSeed(proof.clientToServer);
    }
    else
    {
        proof.clientToServer = TransformX64Seed(seed);
        proof.serverToClient = TransformX64Seed(proof.clientToServer);
    }
    if (!Sha1(proof.clientToServer.data(), proof.clientToServer.size(),
        proof.digest))
    {
        OPENSSL_cleanse(proof.clientToServer.data(),
            proof.clientToServer.size());
        OPENSSL_cleanse(proof.serverToClient.data(),
            proof.serverToClient.size());
        return std::nullopt;
    }
    return proof;
}

WardenCryptoContext::WardenCryptoContext(WardenCryptoContext&& other) noexcept
    : m_clientToServer(other.m_clientToServer),
      m_serverToClient(other.m_serverToClient)
{
    other.Clear();
}

WardenCryptoContext& WardenCryptoContext::operator=(
    WardenCryptoContext&& other) noexcept
{
    if (this != &other)
    {
        Clear();
        m_clientToServer = other.m_clientToServer;
        m_serverToClient = other.m_serverToClient;
        other.Clear();
    }
    return *this;
}

WardenCryptoContext::~WardenCryptoContext()
{
    Clear();
}

bool WardenCryptoContext::Initialize(SessionKey const& sessionKey)
{
    Key16 clientToServer{};
    Key16 serverToClient{};
    Rc4State clientState;
    Rc4State serverState;

    bool const success =
        DeriveInitialKeys(sessionKey, clientToServer, serverToClient);
    if (success)
    {
        clientState.Initialize(clientToServer);
        serverState.Initialize(serverToClient);
        Clear();
        m_clientToServer = clientState;
        m_serverToClient = serverState;
    }

    clientState.Clear();
    serverState.Clear();
    OPENSSL_cleanse(clientToServer.data(), clientToServer.size());
    OPENSSL_cleanse(serverToClient.data(), serverToClient.size());
    return success;
}

bool WardenCryptoContext::IsInitialized() const
{
    return m_clientToServer.initialized && m_serverToClient.initialized;
}

bool WardenCryptoContext::TransformClientToServer(Bytes& bytes)
{
    return m_clientToServer.Transform(bytes.empty() ? nullptr : bytes.data(),
        bytes.size());
}

bool WardenCryptoContext::TransformServerToClient(Bytes& bytes)
{
    return m_serverToClient.Transform(bytes.empty() ? nullptr : bytes.data(),
        bytes.size());
}

WardenCryptoContext WardenCryptoContext::CloneForTransaction() const
{
    WardenCryptoContext clone;
    clone.m_clientToServer = m_clientToServer;
    clone.m_serverToClient = m_serverToClient;
    return clone;
}

bool WardenCryptoContext::InstallDirectionalKeys(Key16 const& clientToServer,
    Key16 const& serverToClient)
{
    if (!IsInitialized())
        return false;

    Rc4State clientState;
    Rc4State serverState;
    clientState.Initialize(clientToServer);
    serverState.Initialize(serverToClient);

    m_clientToServer.Clear();
    m_serverToClient.Clear();
    m_clientToServer = clientState;
    m_serverToClient = serverState;
    clientState.Clear();
    serverState.Clear();
    return true;
}

void WardenCryptoContext::Rc4State::Initialize(Key16 const& key)
{
    for (std::size_t index = 0; index < permutation.size(); ++index)
        permutation[index] = uint8(index);

    uint32 swapIndex = 0;
    for (std::size_t index = 0; index < permutation.size(); ++index)
    {
        swapIndex = (swapIndex + permutation[index] +
            key[index % key.size()]) & 0xFFu;
        std::swap(permutation[index], permutation[swapIndex]);
    }
    i = 0;
    j = 0;
    initialized = true;
}

bool WardenCryptoContext::Rc4State::Transform(uint8* data, std::size_t size)
{
    if (!initialized || (size != 0 && !data))
        return false;

    for (std::size_t offset = 0; offset < size; ++offset)
    {
        i = uint8(i + 1);
        j = uint8(j + permutation[i]);
        std::swap(permutation[i], permutation[j]);
        data[offset] ^= permutation[uint8(permutation[i] + permutation[j])];
    }
    return true;
}

void WardenCryptoContext::Rc4State::Clear()
{
    OPENSSL_cleanse(permutation.data(), permutation.size());
    i = 0;
    j = 0;
    initialized = false;
}

void WardenCryptoContext::Clear()
{
    m_clientToServer.Clear();
    m_serverToClient.Clear();
}
}
