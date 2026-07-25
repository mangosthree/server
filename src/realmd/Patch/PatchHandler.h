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

/** \file
    \ingroup realmd
  */

#ifndef MANGOS_H_PATCHHANDLER
#define MANGOS_H_PATCHHANDLER

#include <memory>
#include "net/ISession.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>

#include <openssl/md5.h>

/**
 * @brief Caches the MD5 of client patch archives under ./patches.
 *
 * A build that the server does not support can be lifted to a supported one by
 * dropping a "<build><locale>.mpq" archive in ./patches; realmd streams it to
 * the client (the classic background-downloader flow). The XFER_INITIATE packet
 * carries the archive's MD5 so the client can verify the download, and this
 * cache avoids re-hashing the (potentially large) file on every logon attempt.
 * Thread-safe: the SRP6 handlers run on the network thread and the streaming
 * runs on its own thread.
 */
class PatchCache
{
    public:
        static PatchCache* instance();

        /// Look up (computing and caching on first use) the MD5 of the patch at
        /// `fullPath`. Returns false if the file cannot be read.
        bool GetMD5(const std::string& fullPath, uint8_t outMd5[MD5_DIGEST_LENGTH]);

    private:
        PatchCache() = default;

        bool ComputeMD5(const std::string& fullPath, uint8_t outMd5[MD5_DIGEST_LENGTH]);

        std::mutex                                                    m_mutex;
        std::map<std::string, std::array<uint8_t, MD5_DIGEST_LENGTH>> m_cache;
};

/**
 * @brief Stream a patch archive to a client on a detached background thread.
 *
 * Frames the file (from `startOffset`, so XFER_RESUME works) as CMD_XFER_DATA
 * chunks pushed through the session's thread-safe Sender, so the network thread
 * is never blocked on disk or a slow client. `flow` applies real backpressure:
 * before each chunk the thread blocks until the transport's outbound backlog has
 * drained, so queued memory stays bounded (a few chunks) no matter how large the
 * archive or how slow the client. `closer` is invoked only if the file cannot be
 * fully read (to tear the connection down); on success the socket is left open
 * for the client to close once it has the whole archive.
 *
 * @return false if the file cannot be opened at `startOffset` (nothing spawned).
 */
bool StartPatchTransfer(net::Sender sender, net::Closer closer,
                        std::shared_ptr<net::FlowControl> flow,
                        const std::string& path, uint64_t startOffset);

#endif
/// @}
