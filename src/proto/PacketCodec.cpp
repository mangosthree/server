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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include <utility>
#include <vector>
#include "PacketCodec.h"

#include <algorithm>
#include <cstring>

namespace proto
{
    PacketCodec::PacketCodec(HeaderDecryptor decryptor)
        : m_decryptor(std::move(decryptor)),
          m_headerFill(0),
          m_haveHeader(false),
          m_opcode(0),
          m_payloadNeeded(0)
    {
        std::memset(m_header, 0, sizeof(m_header));
        std::memset(&m_failure, 0, sizeof(m_failure));
    }

    DecodeStatus PacketCodec::Feed(const uint8* data, size_t len,
                                   std::vector<WorldPacket>& out)
    {
        return Feed(data, len,
            [&out](WorldPacket&& packet) -> bool
            {
                out.push_back(std::move(packet));
                return true;
            });
    }

    DecodeStatus PacketCodec::Feed(const uint8* data, size_t len,
                                   const PacketSink& sink)
    {
        if (data == NULL || len == 0)
        {
            return DecodeStatus::Ok;
        }

        size_t offset = 0;

        while (offset < len)
        {
            // ---- Phase 1: collect and decode the fixed-size header -------------
            if (!m_haveHeader)
            {
                const size_t want = CLIENT_HEADER_SIZE - m_headerFill;
                const size_t take = std::min(want, len - offset);

                std::memcpy(m_header + m_headerFill, data + offset, take);
                m_headerFill += take;
                offset       += take;

                if (m_headerFill < CLIENT_HEADER_SIZE)
                {
                    return DecodeStatus::Ok; // header still incomplete
                }

                // Decrypt exactly once, now that all six bytes are in hand. Doing
                // it per-fragment would corrupt the stream cipher's keystream.
                if (m_decryptor)
                {
                    m_decryptor(m_header, CLIENT_HEADER_SIZE);
                }

                // Read the fields out byte by byte rather than casting the buffer
                // to a packed struct: WorldSocket.cpp's own ClientPktHeader does
                // exactly that and then byte-swaps in place (EndianConvertReverse
                // for size, EndianConvert for cmd) -- the size is big-endian on
                // the wire and the opcode little-endian, so a struct still needs a
                // byte-swap dance, and the cast itself is an aliasing violation on
                // a char buffer.
                const uint32 size = (uint32(m_header[0]) << 8) | uint32(m_header[1]);
                const uint32 cmd  =  uint32(m_header[2])
                                  | (uint32(m_header[3]) << 8)
                                  | (uint32(m_header[4]) << 16)
                                  | (uint32(m_header[5]) << 24);

                // `size` counts the four opcode bytes, so anything below that is
                // impossible and would underflow the payload length below.
                //
                // The opcode is deliberately NOT bounded here, which is the one
                // thing WorldSocket.cpp appeared to forget and in fact relied on.
                // The 15595 client opens the world socket with a greeting that is
                // not a packet at all -- a uint16 size followed by the raw string
                // "WORLD OF WARCRAFT CONNECTION - CLIENT TO SERVER". Read through
                // the 6-byte header that framing puts the ASCII "WORL" in the cmd
                // field, so cmd arrives as 0x4C524F57 and only the uint16
                // truncation below turns it back into MSG_WOW_CONNECTION (0x4F57).
                // Captured live: header 00 30 57 4F 52 4C, size 48, 50 bytes on
                // the wire, payload 48-4 = 44. ANY upper bound on cmd -- 0x2800,
                // 0xFFFF, or the opcode table's own size -- rejects the first
                // thing every client says and no one can log in.
                //
                // Rejecting an opcode that is merely absent from the dispatch
                // table needs NUM_MSG_TYPES, which is game knowledge proto must
                // not link; that check lives in WorldGateway::Deliver(), on the
                // one path proto cannot bypass, safely after the truncation.
                if (size < 4 || size > MAX_CLIENT_PACKET_SIZE)
                {
                    std::memcpy(m_failure.header, m_header, CLIENT_HEADER_SIZE);
                    m_failure.size      = size;
                    m_failure.cmd       = cmd;
                    m_failure.decrypted = bool(m_decryptor);
                    return DecodeStatus::Malformed;
                }

                m_opcode        = uint16(cmd);
                m_payloadNeeded = size - 4;
                m_haveHeader    = true;

                m_payload.clear();
                m_payload.reserve(m_payloadNeeded);
            }

            // ---- Phase 2: collect the payload ---------------------------------
            if (m_payloadNeeded > 0)
            {
                const size_t take = std::min(size_t(m_payloadNeeded), len - offset);
                if (take == 0)
                {
                    return DecodeStatus::Ok; // need more bytes
                }

                m_payload.insert(m_payload.end(), data + offset, data + offset + take);
                offset          += take;
                m_payloadNeeded -= uint32(take);

                if (m_payloadNeeded > 0)
                {
                    return DecodeStatus::Ok; // payload still incomplete
                }
            }

            // ---- Phase 3: emit and reset for the next packet ------------------
            WorldPacket packet(m_opcode, m_payload.size());
            if (!m_payload.empty())
            {
                packet.append(m_payload.data(), m_payload.size());
            }

            // Reset BEFORE handing the packet over. The sink installs the header
            // decryptor from inside this call, so the codec must already be
            // sitting cleanly on the next header boundary when it does.
            m_haveHeader = false;
            m_headerFill = 0;
            m_payload.clear();

            if (!sink(std::move(packet)))
            {
                return DecodeStatus::Ok;
            }
        }

        return DecodeStatus::Ok;
    }

    std::vector<uint8> PacketCodec::Encode(const WorldPacket& packet,
                                           const HeaderEncryptor& encryptor)
    {
        // Mirrors WorldSocket.cpp's ServerPktHeader verbatim: the size field
        // counts the two opcode bytes along with the payload, and packets over
        // 0x7FFF get a three-byte size with the top bit of the first byte set.
        const uint32 size  = uint32(packet.size()) + 2;
        const bool   large = size > 0x7FFF;

        uint8  header[5];
        size_t headerLen = 0;

        if (large)
        {
            header[headerLen++] = uint8(0x80 | ((size >> 16) & 0xFF));
        }
        header[headerLen++] = uint8((size >> 8) & 0xFF);
        header[headerLen++] = uint8(size & 0xFF);

        const uint16 opcode = uint16(packet.GetOpcode());
        header[headerLen++] = uint8(opcode & 0xFF);
        header[headerLen++] = uint8((opcode >> 8) & 0xFF);

        if (encryptor)
        {
            encryptor(header, headerLen);
        }

        std::vector<uint8> wire;
        wire.reserve(headerLen + packet.size());
        wire.insert(wire.end(), header, header + headerLen);

        // contents() is only safe on a non-empty buffer; many packets are pure
        // opcodes with no payload at all.
        if (!packet.empty())
        {
            wire.insert(wire.end(), packet.contents(),
                        packet.contents() + packet.size());
        }

        return wire;
    }
}
