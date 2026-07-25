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

#ifndef MANGOS_H_AUTHSERVER
#define MANGOS_H_AUTHSERVER

#include <cstdint>
#include <memory>
#include <string>

/**
 * @brief Listening acceptor for realmd auth connections.
 *
 * Thin owner of the shared networking engine (net::Server). Each accepted
 * connection is handed a fresh AuthSocket (a net::ISession). The engine
 * (IOCP / epoll / kqueue depending on platform) owns the sockets and worker
 * threads. Held behind a PIMPL so the backend's platform headers (winsock2 on
 * Windows) never leak into realmd's Main.cpp.
 */
class AuthServer
{
    public:
        AuthServer();
        ~AuthServer();

        /// Start accepting auth connections on the given port. `bindIp` is the
        /// configured BindIP: empty (or "0.0.0.0") listens on every local
        /// interface, otherwise the listener binds that single IPv4/hostname.
        /// @return true on success, false if the port could not be bound.
        bool Start(uint16_t port, const std::string& bindIp = std::string());

        /// Stop the network engine and join its worker threads.
        void Stop();

    private:
        AuthServer(const AuthServer&) = delete;
        AuthServer& operator=(const AuthServer&) = delete;

        struct Impl;
        std::unique_ptr<Impl> m_impl;
};

#endif
/// @}
