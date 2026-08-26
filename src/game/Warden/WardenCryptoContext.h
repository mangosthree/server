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

#ifndef MANGOS_WARDEN_CRYPTO_CONTEXT_H
#define MANGOS_WARDEN_CRYPTO_CONTEXT_H

#include "WardenProtocol.h"

#include <array>
#include <cstddef>

namespace warden
{
/** Owns independent streaming RC4 state for one Warden session. */
class WardenCryptoContext
{
public:
    WardenCryptoContext() = default;
    WardenCryptoContext(WardenCryptoContext const&) = delete;
    WardenCryptoContext& operator=(WardenCryptoContext const&) = delete;
    WardenCryptoContext(WardenCryptoContext&& other) noexcept;
    WardenCryptoContext& operator=(WardenCryptoContext&& other) noexcept;
    ~WardenCryptoContext();

    bool Initialize(SessionKey const& sessionKey);
    bool IsInitialized() const;

    bool TransformClientToServer(Bytes& bytes);
    bool TransformServerToClient(Bytes& bytes);

    // Parsing advances only a temporary clone. Moving an accepted clone back
    // commits the stream; destroying a rejected clone rolls it back.
    WardenCryptoContext CloneForTransaction() const;

    // Both replacement streams are built before either live stream changes.
    bool InstallDirectionalKeys(Key16 const& clientToServer,
        Key16 const& serverToClient);

private:
    struct Rc4State
    {
        void Initialize(Key16 const& key);
        bool Transform(uint8* data, std::size_t size);
        void Clear();

        std::array<uint8, 256> permutation{};
        uint8 i = 0;
        uint8 j = 0;
        bool initialized = false;
    };

    void Clear();

    Rc4State m_clientToServer;
    Rc4State m_serverToClient;
};
}

#endif
