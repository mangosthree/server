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

#include "PacketUtilities.h"
#include "Errors.h"

#include <sstream>
#include <array>

WorldPackets::PacketArrayMaxCapacityException::PacketArrayMaxCapacityException(std::size_t requestedSize, std::size_t sizeLimit)
{
    std::ostringstream builder;
    builder << "Attempted to read more array elements from packet " << requestedSize << " than allowed " << sizeLimit;
    //message().assign(builder.str());
}

void WorldPackets::CheckCompactArrayMaskOverflow(std::size_t index, std::size_t limit)
{
    //MANGOS_ASSERT(index < limit, "Attempted to insert " SZFMTD " values into CompactArray but it can only hold " SZFMTD, index, limit);
}
