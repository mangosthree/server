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

#include "WardenPacketCodec.h"

#include <algorithm>

namespace
{
uint32 ReadLittleEndian32(uint8 const* bytes)
{
    return uint32(bytes[0]) |
        (uint32(bytes[1]) << 8) |
        (uint32(bytes[2]) << 16) |
        (uint32(bytes[3]) << 24);
}

void AppendLittleEndian32(warden::Bytes& bytes, uint32 value)
{
    bytes.push_back(uint8(value));
    bytes.push_back(uint8(value >> 8));
    bytes.push_back(uint8(value >> 16));
    bytes.push_back(uint8(value >> 24));
}
}

namespace warden
{
FrameDecodeStatus DecodeClientFrame(
    ByteView payload, DecodedClientFrame& decoded)
{
    decoded.encryptedBody = ByteView();
    if (payload.empty())
        return FrameDecodeStatus::Empty;
    if (payload.size() < ClientWardenLengthSize)
        return FrameDecodeStatus::TruncatedLength;

    uint32 const declaredLength = ReadLittleEndian32(payload.data());
    if (declaredLength == 0)
        return FrameDecodeStatus::Empty;
    if (declaredLength > MaxEncryptedClientBody)
        return FrameDecodeStatus::BodyTooLarge;

    std::size_t const actualLength = payload.size() - ClientWardenLengthSize;
    if (actualLength < declaredLength)
        return FrameDecodeStatus::LengthMismatch;
    if (actualLength > declaredLength)
        return FrameDecodeStatus::TrailingData;

    decoded.encryptedBody = ByteView(
        payload.data() + ClientWardenLengthSize, actualLength);
    return FrameDecodeStatus::Ok;
}

EncodeStatus EncodeServerFrame(
    ByteView encryptedBody, EncodedServerFrame& encoded)
{
    encoded.payload.clear();
    if (encryptedBody.empty())
        return EncodeStatus::Empty;
    if (encryptedBody.size() > MaxEncryptedServerBody)
        return EncodeStatus::BodyTooLarge;

    encoded.payload.reserve(
        ProvisionalServerWardenLengthSize + encryptedBody.size());
    AppendLittleEndian32(encoded.payload, uint32(encryptedBody.size()));
    encoded.payload.insert(encoded.payload.end(), encryptedBody.data(),
        encryptedBody.data() + encryptedBody.size());
    return EncodeStatus::Ok;
}
}
