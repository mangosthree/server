/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
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

/// \addtogroup realmd
/// @{
/// \file

#ifndef MANGOS_H_AUTHSOCKET
#define MANGOS_H_AUTHSOCKET

#include <utility>
#include "Common.h"
#include "Auth/BigNumber.h"
#include "Auth/Sha1.h"
#include "ByteBuffer.h"
#include "Utilities/Util.h"

#include "net/ISession.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

struct Realm;
struct RealmAddress;

/**
 * @brief Handle login commands.
 *
 * A request/response session: the SRP6 login handshake and realm-list handler
 * run inline on the network thread, on the shared net::ISession transport,
 * which owns the socket and hands the session a thread-safe Sender/Closer.
 */
class AuthSocket: public net::ISession
{
    public:
        const static int s_BYTE_SIZE = 32; /**< TODO */

        /**
         * @brief
         *
         */
        AuthSocket();

        /**
         * @brief
         *
         */
        ~AuthSocket();

        // --- net::ISession -------------------------------------------------
        void setPeerAddress(const std::string& addr) override { remote_address_ = addr; }
        void setSender(net::Sender sender) override { m_sender = std::move(sender); }
        void setCloser(net::Closer closer) override { m_closer = std::move(closer); }
        void setFlowControl(std::shared_ptr<net::FlowControl> fc) override { m_flowControl = std::move(fc); }
        std::vector<uint8_t> onConnect() override;
        std::vector<uint8_t> onData(const uint8_t* data, size_t len) override;
        void onClose() override { m_closed.store(true); }
        bool closed() const override { return m_closed.load(); }

        /**
         * @brief
         *
         * @param sha
         */
        void SendProof(Sha1Hash sha);

        /**
         * @brief
         *
         * @param pkt
         * @param acctid
         */
        void LoadRealmlist(ByteBuffer& pkt, uint32 acctid);

        static RealmAddress GetAddressForClient(Realm const& realm, uint32 clientIp);

        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleLogonChallenge();

        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleLogonProof();

        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleReconnectChallenge();

        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleReconnectProof();

        /**
         * @brief
         *
         * @return bool
         */
        bool _HandleRealmList();

        /**
         * @brief Client accepted the offered patch transfer (from the start).
         * @return bool
         */
        bool _HandleXferAccept();

        /**
         * @brief Client wants to resume the patch transfer from an offset.
         * @return bool
         */
        bool _HandleXferResume();

        /**
         * @brief Client cancelled the patch transfer.
         * @return bool
         */
        bool _HandleXferCancel();

        /**
         * @brief
         *
         * @param rI
         */
        void _SetVSFields(const std::string& rI);

#ifdef _WIN32
        /// Number of currently open auth TCP socket connections (observability).
        static uint32 GetConnectionCount() { return s_connections.load(std::memory_order_relaxed); }

        /// Number of clients currently authenticated and waiting for realm list (observability).
        static uint32 GetAuthWaitingCount() { return s_authed.load(std::memory_order_relaxed); }
#endif

    private:
        enum eStatus
        {
            STATUS_CHALLENGE,
            STATUS_LOGON_PROOF,
            STATUS_RECON_PROOF,
            STATUS_PATCH,       ///< awaiting the client's XFER accept/resume/cancel
            STATUS_AUTHED,
            STATUS_CLOSED
        };

        /// Kick off the background patch stream for the offered archive, honouring
        /// an XFER_RESUME start offset. Closes the connection on failure.
        bool BeginPatchStream(uint64 startOffset);

        // --- Buffered-stream emulation (formerly provided by BufferedSocket) ----
        // net::ISession delivers raw bytes via onData(); these mirror the old
        // recv_soft/recv/recv_skip/recv_len/send API so the SRP6 handlers below
        // are unchanged. Bytes consumed during one onData() pass are dropped from
        // the pending buffer afterwards, exactly like the old crunch() behaviour.
        size_t recv_len() const { return m_readBuf.size() - m_readPos; }
        bool recv_soft(char* buf, size_t len);
        bool recv(char* buf, size_t len);
        void recv_skip(size_t len);
        bool send(const char* buf, size_t len);
        const std::string& get_remote_address() const { return remote_address_; }
        void close_connection();

        std::vector<uint8_t> m_readBuf;      ///< pending unconsumed inbound bytes
        size_t               m_readPos = 0;  ///< consume cursor within m_readBuf

        net::Sender          m_sender;       ///< thread-safe outbound channel
        net::Closer          m_closer;       ///< request-teardown channel
        std::shared_ptr<net::FlowControl> m_flowControl; ///< outbound backpressure (patch stream)
        std::atomic<bool>    m_closed{false};

        std::string remote_address_ = "<unknown>";

        BigNumber N, s, g, v; /**< TODO */
        BigNumber b, B; /**< TODO */
        BigNumber K; /**< TODO */
        BigNumber _reconnectProof; /**< TODO */

        eStatus _status; /**< TODO */

        std::string _login; /**< TODO */
        std::string _safelogin; /**< TODO */

        std::string _localizationName; /**< Since GetLocaleByName() is _NOT_ bijective, we have to store the locale as a string. Otherwise we can't differ between enUS and enGB, which is important for the patch system */
        std::string _os;
        std::string _patchPath; /**< resolved ./patches archive offered to an unsupported build, empty if none */
        uint16 _build; /**< TODO */
        AccountTypes _accountSecurityLevel; /**< TODO */

#ifdef _WIN32
        /// Live count of constructed AuthSocket objects (open connections). Observability only.
        static std::atomic<uint32> s_connections;

        /// Live count of sockets currently in STATUS_AUTHED. Observability only.
        static std::atomic<uint32> s_authed;
#endif
};
#endif
/// @}
