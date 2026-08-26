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

#ifndef MANGOS_WARDEN_PACKET_CODEC_H
#define MANGOS_WARDEN_PACKET_CODEC_H

#include "WardenProtocol.h"

#include <cstddef>

namespace warden
{
constexpr std::size_t MaxClientWardenWireSize = 10240;
constexpr std::size_t NormalClientHeaderSize = 4;
constexpr std::size_t ClientWardenLengthSize = 4;
constexpr std::size_t MaxEncryptedClientBody =
    MaxClientWardenWireSize - NormalClientHeaderSize - ClientWardenLengthSize;
constexpr std::size_t CheckResultEnvelopeSize = 7;
constexpr std::size_t MaxDecryptedCheckResultBody =
    MaxEncryptedClientBody - CheckResultEnvelopeSize;

// The outbound envelope is intentionally independent from client-input limits.
// Its uint32 framing remains provisional until the G3 live capture freezes it.
constexpr std::size_t MaxServerWardenWireSize = 10240;
constexpr std::size_t NormalServerHeaderSize = 4;
constexpr std::size_t ProvisionalServerWardenLengthSize = 4;
constexpr std::size_t MaxEncryptedServerBody =
    MaxServerWardenWireSize - NormalServerHeaderSize -
    ProvisionalServerWardenLengthSize;

struct DecodedClientFrame
{
    ByteView encryptedBody;
};

struct EncodedServerFrame
{
    Bytes payload;
};

enum class FrameDecodeStatus : uint8
{
    Ok,
    Empty,
    TruncatedLength,
    LengthMismatch,
    TrailingData,
    BodyTooLarge
};

enum class EncodeStatus : uint8
{
    Ok,
    Empty,
    BodyTooLarge
};

FrameDecodeStatus DecodeClientFrame(
    ByteView payload, DecodedClientFrame& decoded);
EncodeStatus EncodeServerFrame(
    ByteView encryptedBody, EncodedServerFrame& encoded);
}

#endif
