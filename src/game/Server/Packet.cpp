/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2022 MaNGOS <https://getmangos.eu>
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

#include "Packet.h"
#include "Errors.h"

WorldPackets::Packet::Packet(WorldPacket&& worldPacket) : _worldPacket(std::move(worldPacket)) { }

WorldPackets::ServerPacket::ServerPacket(OpcodeServer opcode, size_t initialSize) :
    Packet(WorldPacket(opcode, initialSize)) { }

void WorldPackets::ServerPacket::Read()
{
    MANGOS_ASSERT(!"Read not implemented for server packets.");
}

WorldPackets::ClientPacket::ClientPacket(OpcodeClient expectedOpcode, WorldPacket&& packet) :
    Packet(std::move(packet))
{
    MANGOS_ASSERT(GetOpcode() == expectedOpcode);
}

WorldPackets::ClientPacket::ClientPacket(WorldPacket&& packet) : Packet(std::move(packet)) { }

WorldPacket const* WorldPackets::ClientPacket::Write()
{
    MANGOS_ASSERT(!"Write not allowed for client packets.");

    return nullptr;
}
