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

#ifndef MANGOS_WARDEN_PROTOCOL_H
#define MANGOS_WARDEN_PROTOCOL_H

#include "Platform/Define.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace warden
{
// Build 15595 identifies signed Warden containers by SHA-256. The world
// session key remains exactly 40 bytes, including any leading or trailing zero.
using Bytes = std::vector<uint8>;
using SessionKey = std::array<uint8, 40>;
using Key16 = std::array<uint8, 16>;
using Digest20 = std::array<uint8, 20>;
using Digest32 = std::array<uint8, 32>;
using ModuleId = std::array<uint8, 32>;

/** Non-owning view used by pure Warden parsers without ByteBuffer exceptions. */
class ByteView
{
public:
    constexpr ByteView() = default;
    constexpr ByteView(uint8 const* data, std::size_t size)
        : m_data(data), m_size(size) {}
    ByteView(Bytes const& bytes)
        : m_data(bytes.empty() ? nullptr : bytes.data()), m_size(bytes.size()) {}

    constexpr uint8 const* data() const { return m_data; }
    constexpr std::size_t size() const { return m_size; }
    constexpr bool empty() const { return m_size == 0; }

private:
    uint8 const* m_data = nullptr;
    std::size_t m_size = 0;
};

enum class WardenArchitecture : uint8
{
    Unclassified,
    X86,
    X64
};

enum class ClientVariant : uint8
{
    Unclassified,
    Stock,
    Grunt,
    LegacyGrunt
};

/** Stable bounded tokens used by Realm audit/incident persistence. */
inline char const* ToPersistenceToken(WardenArchitecture architecture)
{
    switch (architecture)
    {
        case WardenArchitecture::X86: return "x86";
        case WardenArchitecture::X64: return "x64";
        case WardenArchitecture::Unclassified: return "unk";
    }
    return nullptr;
}

inline char const* ToPersistenceToken(ClientVariant variant)
{
    switch (variant)
    {
        case ClientVariant::Unclassified: return "unclassified";
        case ClientVariant::Stock: return "stock";
        case ClientVariant::Grunt: return "grunt";
        case ClientVariant::LegacyGrunt: return "legacy-grunt";
    }
    return nullptr;
}

inline bool IsCanonicalLocaleClaim(std::array<char, 4> const& locale)
{
    for (char value : locale)
    {
        if (!((value >= 'A' && value <= 'Z') ||
                (value >= 'a' && value <= 'z')))
        {
            return false;
        }
    }
    return true;
}

inline bool IsPublishedCataWardenLocale(
    std::array<char, 4> const& locale)
{
    static constexpr std::array<std::array<char, 4>, 14> Locales = {{
        {{'d', 'e', 'D', 'E'}}, {{'e', 'n', 'C', 'N'}},
        {{'e', 'n', 'G', 'B'}}, {{'e', 'n', 'T', 'W'}},
        {{'e', 'n', 'U', 'S'}}, {{'e', 's', 'E', 'S'}},
        {{'e', 's', 'M', 'X'}}, {{'f', 'r', 'F', 'R'}},
        {{'k', 'o', 'K', 'R'}}, {{'p', 't', 'B', 'R'}},
        {{'p', 't', 'P', 'T'}}, {{'r', 'u', 'R', 'U'}},
        {{'z', 'h', 'C', 'N'}}, {{'z', 'h', 'T', 'W'}}
    }};
    for (std::array<char, 4> const& candidate : Locales)
    {
        if (candidate == locale)
            return true;
    }
    return false;
}

/** Directional keys and response digest produced by bootstrap command 5. */
struct ArchitectureProof
{
    Key16 clientToServer{};
    Digest20 digest{};
    Key16 serverToClient{};
};

enum class AdmissionStatus : uint8
{
    Available,
    MissingExactLocale,
    UnsupportedExactLocale,
    SessionKeyUnavailable
};

/** Derives the exact architecture-specific command-5 response and rekey. */
std::optional<ArchitectureProof> DeriveArchitectureProof(
    WardenArchitecture architecture, Key16 const& seed);

/**
 * Sole, move-only custody of authenticated Warden admission material.
 * Architecture and executable variant remain unclassified at this boundary.
 */
struct AdmissionData
{
    AdmissionData() = default;
    AdmissionData(AdmissionData const&) = delete;
    AdmissionData& operator=(AdmissionData const&) = delete;
    AdmissionData(AdmissionData&& other) noexcept;
    AdmissionData& operator=(AdmissionData&& other) noexcept;
    ~AdmissionData();

    bool IsAvailable() const;
    void Clear();

    uint32 build = 0;
    std::string clientOs;
    std::string clientLocale;
    SessionKey sessionKey{};
    AdmissionStatus status = AdmissionStatus::SessionKeyUnavailable;
};
}

#endif
