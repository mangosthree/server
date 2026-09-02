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

#include "WorldPacket.h"
#include "WorldSession.h"
#include "WardenServer.h"

void WorldSession::HandleWardenDataOpcode(WorldPacket& recv_data)
{
    std::size_t const readPosition = recv_data.rpos();
    std::size_t const unreadSize = readPosition <= recv_data.size() ?
        recv_data.size() - readPosition : 0;
    uint8 const* unread = unreadSize ?
        recv_data.contents() + readPosition : recv_data.contents();
    warden::ByteView const view(unread, unreadSize);
    if (m_warden)
        m_warden->HandleClientFrame(view);

    recv_data.rfinish();
    FinalizeWardenDisengagement();
}
