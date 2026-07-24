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

#include <cstdint>
#include <string>
#include <memory>
#include <mutex>
#include "PatchHandler.h"
#include "Auth/AuthCodes.h"
#include "Log.h"

#include <openssl/evp.h>

#include <chrono>
#include <cstring>
#include <fstream>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    /// Bytes carried per CMD_XFER_DATA packet. 4 KiB matches the historical page
    /// size the ACE-era PatchHandler used (Chunk::data[4096]).
    constexpr size_t kChunkSize = 4096;

    /// Backpressure ceiling: the streaming thread will not get further than this many
    /// bytes ahead of what the transport has actually written to the socket, so queued
    /// memory stays bounded regardless of archive size or client speed. Stated in
    /// bytes because the transport coalesces queued packets into one contiguous
    /// stream, which makes a count of outstanding buffers meaningless.
    constexpr uint64_t kMaxOutstandingBytes = 256 * 1024;
}

PatchCache* PatchCache::instance()
{
    static PatchCache s_instance;
    return &s_instance;
}

bool PatchCache::ComputeMD5(const std::string& fullPath, uint8_t outMd5[MD5_DIGEST_LENGTH])
{
    std::ifstream in(fullPath, std::ios::binary);
    if (!in)
    {
        return false;
    }

    // Streamed, so a large patch is never held in memory: the one-shot EVP_Digest()
    // cannot express that, hence the explicit context.
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        return false;
    }

    if (EVP_DigestInit_ex(ctx, EVP_md5(), NULL) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return false;
    }

    char buf[kChunkSize];
    while (in)
    {
        in.read(buf, sizeof(buf));
        std::streamsize got = in.gcount();
        if (got > 0 && EVP_DigestUpdate(ctx, buf, static_cast<size_t>(got)) != 1)
        {
            EVP_MD_CTX_free(ctx);
            return false;
        }
    }

    // A read error (as opposed to a clean EOF) leaves a partial hash -- reject it.
    if (in.bad())
    {
        EVP_MD_CTX_free(ctx);
        return false;
    }

    unsigned int len = 0;
    const bool ok = EVP_DigestFinal_ex(ctx, outMd5, &len) == 1;
    EVP_MD_CTX_free(ctx);
    return ok;
}

bool PatchCache::GetMD5(const std::string& fullPath, uint8_t outMd5[MD5_DIGEST_LENGTH])
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(fullPath);
    if (it != m_cache.end())
    {
        memcpy(outMd5, it->second.data(), MD5_DIGEST_LENGTH);
        return true;
    }

    std::array<uint8_t, MD5_DIGEST_LENGTH> digest{};
    if (!ComputeMD5(fullPath, digest.data()))
    {
        return false;
    }

    m_cache[fullPath] = digest;
    memcpy(outMd5, digest.data(), MD5_DIGEST_LENGTH);
    return true;
}

bool StartPatchTransfer(net::Sender sender, net::Closer closer,
                        std::shared_ptr<net::FlowControl> flow,
                        const std::string& path, uint64_t startOffset)
{
    // Open (and position) up front so a failure is reported synchronously to the
    // caller instead of on the background thread.
    auto in = std::make_shared<std::ifstream>(path, std::ios::binary);
    if (!in->is_open())
    {
        return false;
    }
    if (startOffset != 0)
    {
        in->seekg(static_cast<std::streamoff>(startOffset), std::ios::beg);
        if (!in->good())
        {
            return false;
        }
    }

    std::thread([in, sender = std::move(sender), closer = std::move(closer),
                 flow = std::move(flow)]() mutable
    {
        // Do 1 second sleep, similar to the one in game/WorldSocket.cpp.
        // Seems client have problems with too fast sends.
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // One framing buffer for the whole transfer: the header is rewritten in place
        // each pass and the payload read straight in behind it, so streaming a 100 MB
        // archive allocates exactly once rather than once per 4 KiB chunk.
        std::vector<uint8_t> pkt(3 + kChunkSize);

        for (;;)
        {
            // Block until the transport has drained the outbound backlog below the
            // ceiling (real backpressure), or bail out if the connection is gone.
            if (flow && !flow->awaitWritable(kMaxOutstandingBytes))
            {
                return;
            }

            in->read(reinterpret_cast<char*>(pkt.data() + 3), kChunkSize);
            std::streamsize got = in->gcount();
            if (got <= 0)
            {
                break;
            }

            // Frame: CMD_XFER_DATA, uint16 size (little-endian), payload.
            pkt[0] = uint8_t(CMD_XFER_DATA);
            pkt[1] = uint8_t(got & 0xFF);
            pkt[2] = uint8_t((got >> 8) & 0xFF);
            sender(pkt.data(), 3 + static_cast<size_t>(got));
        }

        if (in->bad())
        {
            sLog.outError("PatchTransfer: read error streaming patch, closing connection");
            closer();
        }
        // On success the socket is left open; the client tears it down once it has
        // the whole archive and reconnects with the patched build.
    }).detach();

    return true;
}
