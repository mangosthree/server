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

#ifndef SQLSTORAGE_RELOAD_H
#define SQLSTORAGE_RELOAD_H

#include "DataStores/DBCFileLoader.h"
#include <memory>

struct ReloadBuffer
{
    char*           data;
    uint32          recordCount;
    uint32          maxEntry;
    uint32          recordSize;
    uint32          dstFieldCount;
    const char*     dst_format_ref;
    char**          indexArray;

    ReloadBuffer()
        : data(nullptr), recordCount(0), maxEntry(0), recordSize(0),
          dstFieldCount(0), dst_format_ref(nullptr), indexArray(nullptr) {}

    ~ReloadBuffer()
    {
        if (!data)
            return;

        uint32 offset = 0;
        for (uint32 x = 0; x < dstFieldCount; ++x)
        {
            switch (dst_format_ref[x])
            {
                case DBC_FF_STRING:
                    for (uint32 i = 0; i < recordCount; ++i)
                        delete[] *(char**)((data + (i * recordSize)) + offset);
                    offset += sizeof(char*);
                    break;
                case DBC_FF_LOGIC:   offset += sizeof(bool);    break;
                case DBC_FF_INT:     offset += sizeof(uint32);  break;
                case DBC_FF_FLOAT:   offset += sizeof(float);   break;
                case DBC_FF_BYTE:    offset += sizeof(char);    break;
                case DBC_FF_NA:      offset += sizeof(uint32);  break;
                case DBC_FF_NA_BYTE: offset += sizeof(char);    break;
                case DBC_FF_NA_FLOAT: offset += sizeof(float);  break;
                case DBC_FF_NA_POINTER: offset += sizeof(char*); break;
                default: break;
            }
        }

        delete[] data;
        delete[] indexArray;
    }

    ReloadBuffer(const ReloadBuffer&) = delete;
    ReloadBuffer& operator=(const ReloadBuffer&) = delete;
    ReloadBuffer(ReloadBuffer&&) = delete;
    ReloadBuffer& operator=(ReloadBuffer&&) = delete;
};

using ReloadBufferPtr = std::shared_ptr<ReloadBuffer>;

#endif
