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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include <cstdint>
#include <utility>
#include <vector>
#include <memory>
#include <mutex>
#include "ClientConnection.h"

#include "Auth/BigNumber.h"
#include "Auth/Sha1.h"
#include "Log/Log.h"
#include "Utilities/ByteBuffer.h"

#include <cstring>

namespace proto
{
    namespace
    {
        /// Number of bytes in the client's SHA-1 login proof.
        const size_t AUTH_DIGEST_SIZE = 20;

        // The handful of transport opcodes this file speaks. Re-derived from
        // (grepped out of, never copied by memory from) this fork's own
        // src/game/Server/Opcodes.h -- proto does not link game, so these are
        // proto-local constants rather than references into that table. Every
        // value carries the exact line it was read from, on 2026-07-24:
        //   MSG_WOW_CONNECTION   Opcodes.h:55   (0x4F57)
        //   SMSG_AUTH_CHALLENGE  Opcodes.h:56   (0x4542)
        //   CMSG_AUTH_SESSION    Opcodes.h:57   (0x0449)
        //   SMSG_AUTH_RESPONSE   Opcodes.h:58   (0x5DB6)
        //   CMSG_PING            Opcodes.h:555  (0x444D)
        //   SMSG_PONG            Opcodes.h:556  (0x4D42)
        //   CMSG_KEEP_ALIVE      Opcodes.h:1119 (0x0015)
        const uint16 MSG_WOW_CONNECTION  = 0x4F57;
        const uint16 SMSG_AUTH_CHALLENGE = 0x4542;
        const uint16 CMSG_AUTH_SESSION   = 0x0449;
        const uint16 SMSG_AUTH_RESPONSE  = 0x5DB6;
        const uint16 CMSG_PING           = 0x444D;
        const uint16 SMSG_PONG           = 0x4D42;
        const uint16 CMSG_KEEP_ALIVE     = 0x0015;

        /**
         * @brief Draw the server authentication nonce from the cryptographic RNG.
         *
         * WorldSocket's constructor drew this from rand32() (the general-purpose
         * PRNG). The value is hashed into the client's proof, so a predictable
         * seed narrows the search space for anyone replaying a captured login --
         * the OpenSSL RNG is the strictly-safer port, per the Stage 2 prompt.
         */
        uint32 MakeAuthSeed()
        {
            BigNumber seed;
            seed.SetRand(32);
            return seed.AsDword();
        }
    }

    ClientConnection::ClientConnection(IWorldGateway& gateway)
        : m_gateway(gateway),
          m_codec(),
          m_seed(MakeAuthSeed()),
          m_session(INVALID_SESSION_ID),
          m_closed(false),
          m_hadPing(false),
          m_fastPingRun(0)
    {
    }

    ClientConnection::~ClientConnection()
    {
    }

    std::vector<uint8_t> ClientConnection::onConnect()
    {
        // Fire-and-continue: WorldSocket::open() sends both packets below back
        // to back and does not wait for the client's own MSG_WOW_CONNECTION in
        // between (WorldSocket.cpp:360-383). This is Cata-only transport
        // scaffolding with no equivalent in any sibling fork; reproduce it
        // byte-for-byte or the 15595 client hangs at "Connecting".
        std::vector<uint8_t> wire;

        // MSG_WOW_CONNECTION (WorldSocket.cpp:360-363). The string looks
        // truncated -- it is the shipped byte sequence. Do not "fix" it.
        WorldPacket connection(MSG_WOW_CONNECTION, 46);
        connection << std::string("RLD OF WARCRAFT CONNECTION - SERVER TO CLIENT");

        const std::vector<uint8> connectionWire =
            PacketCodec::Encode(connection, PacketCodec::HeaderEncryptor());
        wire.insert(wire.end(), connectionWire.begin(), connectionWire.end());

        // SMSG_AUTH_CHALLENGE (37-byte payload, WorldSocket.cpp:371-378): eight
        // zero uint32s, then the server seed, then a trailing uint8(1).
        WorldPacket challenge(SMSG_AUTH_CHALLENGE, 37);
        for (uint32 i = 0; i < 8; ++i)
        {
            challenge << uint32(0);
        }
        challenge << m_seed;
        challenge << uint8(1);

        // Neither packet is encrypted: the crypt is not armed yet, and the
        // client cannot key its own cipher until it has the challenge.
        const std::vector<uint8> challengeWire =
            PacketCodec::Encode(challenge, PacketCodec::HeaderEncryptor());
        wire.insert(wire.end(), challengeWire.begin(), challengeWire.end());

        return wire;
    }

    std::vector<uint8_t> ClientConnection::onData(const uint8_t* data, size_t len)
    {
        std::vector<WorldPacket> packets;

        if (m_codec.Feed(data, len, packets) == DecodeStatus::Malformed)
        {
            sLog.outError("proto: malformed packet framing from %s, dropping",
                          m_address.c_str());
            Close();
            return std::vector<uint8_t>();
        }

        // A short read anywhere below unwinds onto a network worker thread, where
        // nothing else would catch it and the process would abort. Dropping the
        // peer has to be the worst a malformed packet can do.
        try
        {
            for (size_t i = 0; i < packets.size(); ++i)
            {
                if (!HandlePacket(std::move(packets[i])))
                {
                    Close();
                    break;
                }
            }
        }
        catch (ByteBufferException&)
        {
            sLog.outError("proto: short read handling packet from %s, dropping",
                          m_address.c_str());
            Close();
        }

        // Everything this class sends goes through SendPacket() (and therefore the
        // transport's Sender), because a reply may be produced on the world thread
        // long after this call returned. Nothing is ever returned inline.
        return std::vector<uint8_t>();
    }

    void ClientConnection::HandleWowConnection(WorldPacket& packet)
    {
        // WorldSocket.cpp's HandleWowConnection (:398-403): read and ignored,
        // no validation, no state gate. The client's copy of this handshake
        // string arrives whenever it arrives -- onConnect() has already sent
        // the challenge unconditionally by this point.
        std::string clientToServerMsg;
        packet >> clientToServerMsg;
    }

    bool ClientConnection::HandlePacket(WorldPacket&& packet)
    {
        const uint16 opcode = uint16(packet.GetOpcode());

        switch (opcode)
        {
            case MSG_WOW_CONNECTION:
                HandleWowConnection(packet);
                return true;

            case CMSG_AUTH_SESSION:
                if (m_session != INVALID_SESSION_ID)
                {
                    sLog.outError("proto: repeated CMSG_AUTH_SESSION from %s",
                                  m_address.c_str());
                    return false;
                }
                return HandleAuthSession(packet);

            case CMSG_PING:
                return HandlePing(packet);

            case CMSG_KEEP_ALIVE:
                // WorldSocket.cpp:906-915 swallows this with a debug log only;
                // mirrored here. The Eluna OnPacketReceive hook for this opcode
                // is world-side scaffolding and moves to the gateway in the CP3
                // checkpoint that wires proto into game.
                DEBUG_LOG("proto: CMSG_KEEP_ALIVE from %s", m_address.c_str());
                return true;

            default:
                break;
        }

        if (m_session == INVALID_SESSION_ID)
        {
            sLog.outError("proto: opcode 0x%.4X from unauthenticated peer %s",
                          opcode, m_address.c_str());
            return false;
        }

        m_gateway.Deliver(m_session, std::move(packet));
        return true;
    }

    bool ClientConnection::HandleAuthSession(WorldPacket& packet)
    {
        AuthRequest request;
        request.peerAddress = m_address;

        uint16 clientBuild = 0;

        // Cata's CMSG_AUTH_SESSION scrambles the client's SHA-1 digest bytes and
        // interleaves them with the rest of the fields. Moved verbatim from
        // WorldSocket::HandleAuthSession (WorldSocket.cpp:986-1028) -- this
        // exact read order is wire truth, not a style choice.
        try
        {
            packet.read_skip<uint32>();
            packet.read_skip<uint32>();
            packet.read_skip<uint8>();
            packet >> request.digest[10];
            packet >> request.digest[18];
            packet >> request.digest[12];
            packet >> request.digest[5];
            packet.read_skip<uint64>();
            packet >> request.digest[15];
            packet >> request.digest[9];
            packet >> request.digest[19];
            packet >> request.digest[4];
            packet >> request.digest[7];
            packet >> request.digest[16];
            packet >> request.digest[3];
            packet >> clientBuild;
            packet >> request.digest[8];
            packet.read_skip<uint32>();
            packet.read_skip<uint8>();
            packet >> request.digest[17];
            packet >> request.digest[6];
            packet >> request.digest[0];
            packet >> request.digest[1];
            packet >> request.digest[11];
            packet >> request.clientSeed;
            packet >> request.digest[2];
            packet.read_skip<uint32>();
            packet >> request.digest[14];
            packet >> request.digest[13];

            uint32 addonSize = 0;
            packet >> addonSize;                        // addon data size

            request.addonData.resize(addonSize);
            if (addonSize > 0)
            {
                packet.read(request.addonData.data(), addonSize);
            }

            uint8 nameLenHigh = 0;
            uint8 nameLenLow  = 0;
            packet >> nameLenHigh;
            packet >> nameLenLow;

            const uint8 accNameLen = uint8((nameLenHigh << 5) | (nameLenLow >> 3));
            request.account = packet.ReadString(accNameLen);
        }
        catch (ByteBufferException&)
        {
            sLog.outError("proto: truncated CMSG_AUTH_SESSION from %s",
                          m_address.c_str());
            return false;
        }

        request.build = clientBuild;

        // Policy and persistence: account row, bans, IP lock, allowed build,
        // security level, client OS. None of it belongs on this side of the seam.
        const AuthLookup lookup = m_gateway.LookupAccount(request);

        if (lookup.status != AuthStatus::Ok)
        {
            SendAuthStatus(lookup.status);
            sLog.outError("proto: login refused for account '%s' from %s (code 0x%.2X)",
                          request.account.c_str(), m_address.c_str(),
                          uint32(lookup.status));
            return false;
        }

        // Cryptography stays on this side. The client proves it holds the same
        // session key realmd handed it, over both halves of the nonce.
        // WorldSocket.cpp:1194-1206, moved verbatim.
        BigNumber sessionKey = lookup.sessionKey;
        const uint32 zero       = 0;
        const uint32 clientSeed = request.clientSeed;
        const uint32 serverSeed = m_seed;

        Sha1Hash sha;
        sha.UpdateData(request.account);
        sha.UpdateData(reinterpret_cast<const uint8*>(&zero), 4);
        sha.UpdateData(reinterpret_cast<const uint8*>(&clientSeed), 4);
        sha.UpdateData(reinterpret_cast<const uint8*>(&serverSeed), 4);
        sha.UpdateBigNumbers(&sessionKey, NULL);
        sha.Finalize();

        if (std::memcmp(sha.GetDigest(), request.digest, AUTH_DIGEST_SIZE) != 0)
        {
            SendAuthStatus(AuthStatus::Failed);
            sLog.outError("proto: bad login proof for account '%s' from %s",
                          request.account.c_str(), m_address.c_str());
            return false;
        }

        // Arm the cipher BEFORE the world is told, because the world answers with
        // SMSG_AUTH_RESPONSE (or a queue position) the moment it accepts the
        // session (WorldSession::SendAuthWaitQue), and that reply must already be
        // encrypted. WorldSocket.cpp arms at :1235, right after the proof check
        // and before the WorldSession is constructed.
        m_crypt.Init(&sessionKey);
        m_codec.SetHeaderDecryptor(
            [this](uint8* header, size_t len) { m_crypt.DecryptRecv(header, len); });

        // Hand the world a share of our own lifetime. net::ISession is held by
        // shared_ptr from the moment the transport accepts, so this is well
        // formed here, and it is what allows a WorldSession to outlive its socket
        // long enough to unwind the player from the map.
        std::shared_ptr<IClientLink> link =
            std::static_pointer_cast<ClientConnection>(shared_from_this());

        const SessionId session = m_gateway.Attach(request, link, lookup.context);
        if (session == INVALID_SESSION_ID)
        {
            SendAuthStatus(AuthStatus::SystemError);
            return false;
        }

        m_session = session;

        DEBUG_LOG("proto: account '%s' authenticated from %s",
                  request.account.c_str(), m_address.c_str());
        return true;
    }

    bool ClientConnection::HandlePing(WorldPacket& packet)
    {
        uint32 ping    = 0;
        uint32 latency = 0;

        try
        {
            packet >> ping;
            packet >> latency;
        }
        catch (ByteBufferException&)
        {
            sLog.outError("proto: truncated CMSG_PING from %s", m_address.c_str());
            return false;
        }

        // WorldSocket.cpp:1281 -- the client pings roughly every 30 seconds.
        // Anything materially faster is either a broken client or someone
        // probing, so count the run; a single early ping is jitter and must
        // not be treated as either.
        static const std::chrono::seconds MIN_PING_INTERVAL(27);

        const std::chrono::steady_clock::time_point now =
            std::chrono::steady_clock::now();

        if (m_hadPing)
        {
            if (now - m_lastPing < MIN_PING_INTERVAL)
            {
                ++m_fastPingRun;
            }
            else
            {
                m_fastPingRun = 0;
            }
        }
        m_lastPing = now;
        m_hadPing  = true;

        if (!m_gateway.OnPing(m_session, latency, m_fastPingRun))
        {
            return false;
        }

        // SMSG_PONG: 4-byte echo of the ping counter (WorldSocket.cpp:1324-1326).
        WorldPacket pong(SMSG_PONG, 4);
        pong << ping;
        SendPacket(pong);
        return true;
    }

    void ClientConnection::SendAuthStatus(AuthStatus status)
    {
        // Cata's failure SMSG_AUTH_RESPONSE is bit-packed: two false bits (the
        // has-queue-data / has-account-data flags) ahead of the status byte,
        // repeated identically at WorldSocket.cpp:1038-1041, :1074-1077,
        // :1111-1114 and :1208-1211. This is NOT MangosTwo's plain byte stream.
        WorldPacket packet(SMSG_AUTH_RESPONSE, 2);
        packet.WriteBit(false);
        packet.WriteBit(false);
        packet << uint8(status);
        SendPacket(packet);
    }

    void ClientConnection::SendPacket(const WorldPacket& packet)
    {
        if (m_closed.load(std::memory_order_acquire) || !m_sender)
        {
            return;
        }

        std::vector<uint8_t> wire;
        {
            // The cipher is a stream: two threads encrypting headers concurrently
            // would interleave the keystream and desynchronise the client for good.
            std::lock_guard<std::mutex> lock(m_cryptSendLock);
            wire = PacketCodec::Encode(packet,
                [this](uint8* header, size_t len)
                {
                    if (m_crypt.IsInitialized())
                    {
                        m_crypt.EncryptSend(header, len);
                    }
                });
        }

        m_sender(wire.data(), wire.size());
    }

    void ClientConnection::Close()
    {
        m_closed.store(true, std::memory_order_release);
        if (m_closer)
        {
            m_closer();
        }
    }

    void ClientConnection::onClose()
    {
        m_closed.store(true, std::memory_order_release);

        if (m_session != INVALID_SESSION_ID)
        {
            m_gateway.Detach(m_session);
            m_session = INVALID_SESSION_ID;
        }
    }
}
