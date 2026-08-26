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

#include "TestHarness.h"

#include "WardenPacketCodec.h"

#include <algorithm>
#include <cstddef>

namespace
{
warden::Bytes ClientFrame(uint32 declaredLength, std::size_t actualLength)
{
    warden::Bytes payload(4 + actualLength, uint8(0xA5));
    payload[0] = uint8(declaredLength);
    payload[1] = uint8(declaredLength >> 8);
    payload[2] = uint8(declaredLength >> 16);
    payload[3] = uint8(declaredLength >> 24);
    return payload;
}

void CheckRejectedClientFrame(warden::Bytes const& payload,
    warden::FrameDecodeStatus expected)
{
    warden::DecodedClientFrame decoded;
    decoded.encryptedBody = warden::ByteView(
        reinterpret_cast<uint8 const*>("sentinel"), 8);
    CHECK_EQ(int(warden::DecodeClientFrame(warden::ByteView(payload), decoded)),
        int(expected));
    CHECK(decoded.encryptedBody.empty());
}
}

static_assert(warden::MaxClientWardenWireSize == 10240);
static_assert(warden::NormalClientHeaderSize == 4);
static_assert(warden::ClientWardenLengthSize == 4);
static_assert(warden::MaxEncryptedClientBody == 10232);
static_assert(warden::MaxDecryptedCheckResultBody == 10225);
static_assert(warden::MaxServerWardenWireSize == 10240);
static_assert(warden::NormalServerHeaderSize == 4);
static_assert(warden::ProvisionalServerWardenLengthSize == 4);
static_assert(warden::MaxEncryptedServerBody == 10232);
static_assert(warden::CheckResultEnvelopeSize == 7);

TEST(WardenPacketCodec_client_frame_decodes_exact_little_endian_length)
{
    warden::Bytes const payload = {3, 0, 0, 0, 0x11, 0x22, 0x33};
    warden::DecodedClientFrame decoded;
    CHECK_EQ(int(warden::DecodeClientFrame(warden::ByteView(payload), decoded)),
        int(warden::FrameDecodeStatus::Ok));
    CHECK_EQ(decoded.encryptedBody.size(), std::size_t(3));
    CHECK_HEX(decoded.encryptedBody.data(), decoded.encryptedBody.size(),
        "112233");
}

TEST(WardenPacketCodec_client_frame_accepts_exact_maximum_body)
{
    warden::Bytes const payload = ClientFrame(
        uint32(warden::MaxEncryptedClientBody),
        warden::MaxEncryptedClientBody);
    warden::DecodedClientFrame decoded;
    CHECK_EQ(int(warden::DecodeClientFrame(warden::ByteView(payload), decoded)),
        int(warden::FrameDecodeStatus::Ok));
    CHECK_EQ(decoded.encryptedBody.size(), warden::MaxEncryptedClientBody);
}

TEST(WardenPacketCodec_client_frame_rejects_empty_and_truncated_length)
{
    CheckRejectedClientFrame({}, warden::FrameDecodeStatus::Empty);
    CheckRejectedClientFrame({1}, warden::FrameDecodeStatus::TruncatedLength);
    CheckRejectedClientFrame({1, 0, 0},
        warden::FrameDecodeStatus::TruncatedLength);
    CheckRejectedClientFrame({0, 0, 0, 0},
        warden::FrameDecodeStatus::Empty);
}

TEST(WardenPacketCodec_client_frame_rejects_short_body_and_trailing_data)
{
    CheckRejectedClientFrame(ClientFrame(4, 3),
        warden::FrameDecodeStatus::LengthMismatch);
    CheckRejectedClientFrame(ClientFrame(3, 4),
        warden::FrameDecodeStatus::TrailingData);
}

TEST(WardenPacketCodec_client_frame_rejects_oversized_body_before_slicing)
{
    CheckRejectedClientFrame(ClientFrame(
        uint32(warden::MaxEncryptedClientBody + 1), 0),
        warden::FrameDecodeStatus::BodyTooLarge);
    CheckRejectedClientFrame(ClientFrame(0xFFFFFFFFu, 0),
        warden::FrameDecodeStatus::BodyTooLarge);
}

TEST(WardenPacketCodec_server_frame_encodes_provisional_little_endian_length)
{
    warden::Bytes const body = {0x11, 0x22, 0x33};
    warden::EncodedServerFrame encoded;
    CHECK_EQ(int(warden::EncodeServerFrame(warden::ByteView(body), encoded)),
        int(warden::EncodeStatus::Ok));
    CHECK_HEX(encoded.payload.data(), encoded.payload.size(),
        "03000000112233");
}

TEST(WardenPacketCodec_server_frame_accepts_exact_maximum_body)
{
    warden::Bytes const body(warden::MaxEncryptedServerBody, uint8(0x5A));
    warden::EncodedServerFrame encoded;
    CHECK_EQ(int(warden::EncodeServerFrame(warden::ByteView(body), encoded)),
        int(warden::EncodeStatus::Ok));
    CHECK_EQ(encoded.payload.size(),
        warden::ProvisionalServerWardenLengthSize + body.size());
    CHECK_HEX(encoded.payload.data(), 4, "f8270000");
    CHECK(std::equal(body.begin(), body.end(), encoded.payload.begin() + 4));
}

TEST(WardenPacketCodec_server_frame_rejects_empty_and_oversized_bodies)
{
    warden::EncodedServerFrame encoded;
    encoded.payload = {1, 2, 3};
    CHECK_EQ(int(warden::EncodeServerFrame(warden::ByteView(), encoded)),
        int(warden::EncodeStatus::Empty));
    CHECK(encoded.payload.empty());

    warden::Bytes const tooLarge(
        warden::MaxEncryptedServerBody + 1, uint8(0));
    encoded.payload = {1, 2, 3};
    CHECK_EQ(int(warden::EncodeServerFrame(warden::ByteView(tooLarge), encoded)),
        int(warden::EncodeStatus::BodyTooLarge));
    CHECK(encoded.payload.empty());
}
