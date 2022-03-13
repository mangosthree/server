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

#include "ByteBuffer.h"

#include "Errors.h"
#include "Log.h"
#include "Utilities/MessageBuffer.h"
#include "Util.h"

#include <sstream>
#include <ctime>

ByteBuffer::ByteBuffer(MessageBuffer&& buffer) :
    _rpos(0), _wpos(0), _bitpos(InitialBitPos), _curbitval(0), _storage(buffer.Move()) { }

ByteBufferPositionException::ByteBufferPositionException(bool add, size_t pos, size_t size, size_t valueSize)
{
    std::ostringstream ss;

    ss << "Attempted to " << (add ? "put" : "get") << " value with size: "
       << valueSize << " in ByteBuffer (pos: " << pos << " size: " << size
       << ")";

    message().assign(ss.str());
}

ByteBufferSourceException::ByteBufferSourceException(size_t pos, size_t size, size_t valueSize)
{
    std::ostringstream ss;

    ss << "Attempted to put a "
       << (valueSize > 0 ? "nullptr-pointer" : "zero-sized value")
       << " in ByteBuffer (pos: " << pos << " size: " << size << ")";

    message().assign(ss.str());
}

ByteBuffer& ByteBuffer::operator>>(double& value)
{
    value = read<double>();

    if (!std::isfinite(value))
    {
        throw ByteBufferException();
    }

    return *this;
}

ByteBuffer& ByteBuffer::operator>>(double& value)
{
    value = read<double>();

    if (!std::isfinite(value))
    {
        throw ByteBufferException();
    }

    return *this;
}

uint32 ByteBuffer::ReadPackedTime()
{
    uint32 packedDate = read<uint32>();
    tm lt = tm();

    lt.tm_min = packedDate & 0x3F;
    lt.tm_hour = (packedDate >> 6) & 0x1F;
    //lt.tm_wday = (packedDate >> 11) & 7;
    lt.tm_mday = ((packedDate >> 14) & 0x3F) + 1;
    lt.tm_mon = (packedDate >> 20) & 0xF;
    lt.tm_year = ((packedDate >> 24) & 0x1F) + 100;

    return uint32(mktime(&lt));
}

void ByteBuffer::append(uint8 const* src, size_t cnt)
{
    /**
     * To Do: adjust Mangos Assert to work with 4 args
     */

    //MANGOS_ASSERT(src, "", _wpos, size());
    //MANGOS_ASSERT(cnt, "", _wpos, size());
    MANGOS_ASSERT(size() < 10000000);

    FlushBits();

    size_t const newSize = _wpos + cnt;
    if (_storage.capacity() < newSize) /* cust memory allocation rules */
    {
        if (newSize < 100)
        {
            _storage.reserve(300);
        }
        else if (newSize < 750)
        {
            _storage.reserve(2500);
        }
        else if (newSize < 6000)
        {
            _storage.reserve(10000);
        }
        else
        {
            _storage.reserve(400000);
        }
    }

    if (_storage.size() < newSize)
    {
        _storage.resize(newSize);
    }

    std::memcpy(&_storage[_wpos], src, cnt);
    _wpos = newSize;
}

void ByteBuffer::AppendPackedTime(time_t time)
{
    tm lt;
    localtime_r(&time, &lt);
    append<uint32>((lt.tm_year - 100) << 24 | lt.tm_mon << 20 |
        (lt.tm_mday - 1) << 14 | lt.tm_wday << 11 | lt.tm_hour << 6 | lt.tm_min);

}

void ByteBuffer::put(size_t pos, uint8 const* src, size_t cnt)
{
    /**
     * To Do: adjust Mangos Assert to work with 4 args
     */

    //MANGOS_ASSERT(pos + cnt <= size(), "", cnt, pos, size());
    //MANGOS_ASSERT(src, "", pos, size());
    //MANGOS_ASSERT(cnt, "", pos, size());

    std::memcpy(&_storage[pos], src, cnt);
}

void ByteBuffer::print_storage() const
{
    std::ostringstream o;
    o << "STORAGE_SIZE: " << size();
    for (uint32 i = 0; i < size(); ++i)
    {
        o << read<uint8>(i) << " - ";
    }

    o << " ";
}

void ByteBuffer::hexlike() const
{
    uint32 j = 1, k = 1;

    std::ostringstream o;
    o << "STORAGE_SIZE: " << size();

    for (uint32 i = 0; i < size(); ++i)
    {
        char buf[3];
        snprintf(buf, 3, "%2X", read<uint8>(i));
        if ((i == (j * 8)) && ((i != (k * 16))))
        {
            o << "| ";
            ++j;
        }
        else if (i == (k * 16))
        {
            o << "\n";
            ++k;
            ++j;
        }

        o << buf;
    }

    o << " ";
}
