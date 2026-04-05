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

#include "SQLStorage.h"
#include "SQLStorageReload.h"
#include "Utilities/ProgressBar.h"
#include "Log/Log.h"

void SQLStorage::ReloadAtomic()
{
    sLog.outString("SQLStorage::ReloadAtomic: Reloading %s (atomic) ...", GetTableName());

    QueryResult* result = WorldDatabase.PQuery("SELECT MAX(`%s`) FROM `%s`",
                                               EntryFieldName(), GetTableName());
    if (!result)
    {
        sLog.outString("SQLStorage::ReloadAtomic: Table %s does not exist, skipping.\n", GetTableName());
        return;
    }
    uint32 maxRecordId = (*result)[0].GetUInt32() + 1;
    delete result;

    result = WorldDatabase.PQuery("SELECT COUNT(*) FROM `%s`", GetTableName());
    uint32 recordCount = 0;
    if (result)
    {
        recordCount = result->Fetch()[0].GetUInt32();
        delete result;
    }

    result = WorldDatabase.PQuery("SELECT * FROM `%s`", GetTableName());
    if (!result)
    {
        sLog.outString("%s table is empty!\n", GetTableName());
        return;
    }

    if (GetSrcFieldCount() != result->GetFieldCount())
    {
        sLog.outError("SQLStorage::ReloadAtomic: %s has wrong column count (expected %u, got %u)\n",
                      GetTableName(), GetSrcFieldCount(), result->GetFieldCount());
        delete result;
        return;
    }

    uint32 recordSize = 0;
    for (uint32 x = 0; x < GetDstFieldCount(); ++x)
    {
        switch (GetDstFormat(x))
        {
            case DBC_FF_LOGIC:   recordSize += sizeof(bool);   break;
            case DBC_FF_BYTE:    recordSize += sizeof(char);   break;
            case DBC_FF_INT:     recordSize += sizeof(uint32); break;
            case DBC_FF_FLOAT:   recordSize += sizeof(float);  break;
            case DBC_FF_STRING:  recordSize += sizeof(char*);  break;
            case DBC_FF_NA:      recordSize += sizeof(uint32); break;
            case DBC_FF_NA_BYTE: recordSize += sizeof(char);   break;
            case DBC_FF_NA_FLOAT:recordSize += sizeof(float);  break;
            case DBC_FF_NA_POINTER: recordSize += sizeof(char*); break;
            default: break;
        }
    }

    auto newBuf = std::make_unique<ReloadBuffer>();
    newBuf->data = new char[recordCount * recordSize]();
    newBuf->indexArray = new char*[maxRecordId]();
    newBuf->recordCount = 0;
    newBuf->maxEntry = maxRecordId;
    newBuf->recordSize = recordSize;
    newBuf->dstFieldCount = GetDstFieldCount();
    newBuf->dst_format_ref = GetDstFormat();

    uint32 dstFields = GetDstFieldCount();
    uint32 srcFields = GetSrcFieldCount();
    const char* dstFmt = GetDstFormat();
    const char* srcFmt = GetSrcFormat();
    BarGoLink bar(recordCount);
    uint32 rowIdx = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 entryId = fields[0].GetUInt32();
        bar.step();

        char* record = &newBuf->data[rowIdx * recordSize];
        newBuf->indexArray[entryId] = record;
        ++newBuf->recordCount;
        ++rowIdx;

        uint32 offset = 0;
        for (uint32 x = 0, y = 0; x < dstFields;)
        {
            switch (dstFmt[x])
            {
                case DBC_FF_NA:
                { uint32 val = 0; memcpy(&record[offset], &val, sizeof(uint32)); offset += sizeof(uint32); ++x; continue; }
                case DBC_FF_NA_BYTE:
                    record[offset] = 0; offset += sizeof(char); ++x; continue;
                case DBC_FF_NA_FLOAT:
                { float val = 0.0f; memcpy(&record[offset], &val, sizeof(float)); offset += sizeof(float); ++x; continue; }
                case DBC_FF_NA_POINTER:
                { char const* val = nullptr; memcpy(&record[offset], &val, sizeof(char const*)); offset += sizeof(char const*); ++x; continue; }
                default: break;
            }
            if (y >= srcFields) break;
            switch (srcFmt[y])
            {
                case DBC_FF_LOGIC:
                { bool val = fields[y].GetUInt32() > 0; memcpy(&record[offset], &val, sizeof(bool)); offset += sizeof(bool); ++x; break; }
                case DBC_FF_BYTE:
                    record[offset] = fields[y].GetUInt8(); offset += sizeof(char); ++x; break;
                case DBC_FF_INT:
                { uint32 val = fields[y].GetUInt32(); memcpy(&record[offset], &val, sizeof(uint32)); offset += sizeof(uint32); ++x; break; }
                case DBC_FF_FLOAT:
                { float val = fields[y].GetFloat(); memcpy(&record[offset], &val, sizeof(float)); offset += sizeof(float); ++x; break; }
                case DBC_FF_STRING:
                {
                    const char* str = fields[y].GetString();
                    char* dst = nullptr;
                    if (!str) { dst = new char[1]; *dst = 0; }
                    else { uint32 l = strlen(str) + 1; dst = new char[l]; memcpy(dst, str, l); }
                    memcpy(&record[offset], &dst, sizeof(char*));
                    offset += sizeof(char*); ++x; break;
                }
                case DBC_FF_NA:
                case DBC_FF_NA_BYTE:
                case DBC_FF_NA_FLOAT:
                    break;
                default: break;
            }
            ++y;
        }
    }
    while (result->NextRow());
    delete result;

    char*     svData   = m_data;
    char**    svIndex  = m_Index;
    uint32_t  svRC     = m_recordCount;
    uint32_t  svRSize  = m_recordSize;
    const char* svFmt  = GetDstFormat();

    m_data         = newBuf->data;
    m_Index        = newBuf->indexArray;
    m_maxEntry     = newBuf->maxEntry;
    m_recordCount  = newBuf->recordCount;
    m_recordSize   = newBuf->recordSize;

    if (svData)
    {
        uint32_t offset = 0;
        for (uint32 x = 0; x < GetDstFieldCount(); ++x)
        {
            if (svFmt[x] == DBC_FF_STRING)
            {
                for (uint32 i = 0; i < svRC; ++i)
                    delete[] *(char**)((svData + (i * svRSize)) + offset);
                offset += sizeof(char*);
            }
            else if (svFmt[x] == DBC_FF_LOGIC)    offset += sizeof(bool);
            else if (svFmt[x] == DBC_FF_INT)      offset += sizeof(uint32);
            else if (svFmt[x] == DBC_FF_FLOAT)    offset += sizeof(float);
            else if (svFmt[x] == DBC_FF_BYTE)     offset += sizeof(char);
            else if (svFmt[x] == DBC_FF_NA)       offset += sizeof(uint32);
            else if (svFmt[x] == DBC_FF_NA_BYTE)  offset += sizeof(char);
            else if (svFmt[x] == DBC_FF_NA_FLOAT) offset += sizeof(float);
            else if (svFmt[x] == DBC_FF_NA_POINTER) offset += sizeof(char*);
        }
        delete[] svData;
        delete[] svIndex;
    }

    m_atomicBuffer.reset(newBuf.release());

    sLog.outString("SQLStorage::ReloadAtomic: %s reloaded (%u entries).",
                   GetTableName(), m_recordCount);
}

void SQLStorage::Reload()
{
    ReloadAtomic();
}

void SQLHashStorage::ReloadAtomic()
{
    sLog.outString("SQLHashStorage::ReloadAtomic: Reloading %s (atomic) ...", GetTableName());

    QueryResult* result = WorldDatabase.PQuery("SELECT MAX(`%s`) FROM `%s`",
                                               EntryFieldName(), GetTableName());
    if (!result)
    {
        sLog.outString("SQLHashStorage::ReloadAtomic: Table %s does not exist, skipping.\n", GetTableName());
        return;
    }
    uint32 maxRecordId = (*result)[0].GetUInt32() + 1;
    delete result;

    result = WorldDatabase.PQuery("SELECT COUNT(*) FROM `%s`", GetTableName());
    uint32 recordCount = 0;
    if (result)
    {
        recordCount = result->Fetch()[0].GetUInt32();
        delete result;
    }

    result = WorldDatabase.PQuery("SELECT * FROM `%s`", GetTableName());
    if (!result)
    {
        sLog.outString("%s table is empty!\n", GetTableName());
        return;
    }

    uint32 recordSize = 0;
    for (uint32 x = 0; x < GetDstFieldCount(); ++x)
    {
        switch (GetDstFormat(x))
        {
            case DBC_FF_LOGIC:   recordSize += sizeof(bool);   break;
            case DBC_FF_BYTE:    recordSize += sizeof(char);   break;
            case DBC_FF_INT:     recordSize += sizeof(uint32); break;
            case DBC_FF_FLOAT:   recordSize += sizeof(float);  break;
            case DBC_FF_STRING:  recordSize += sizeof(char*);  break;
            case DBC_FF_NA:      recordSize += sizeof(uint32); break;
            case DBC_FF_NA_BYTE: recordSize += sizeof(char);   break;
            case DBC_FF_NA_FLOAT:recordSize += sizeof(float);  break;
            case DBC_FF_NA_POINTER: recordSize += sizeof(char*); break;
            default: break;
        }
    }

    auto newBuf = std::make_unique<ReloadBuffer>();
    newBuf->data = new char[recordCount * recordSize]();
    newBuf->indexArray = nullptr;
    newBuf->recordCount = 0;
    newBuf->maxEntry = maxRecordId;
    newBuf->recordSize = recordSize;
    newBuf->dstFieldCount = GetDstFieldCount();
    newBuf->dst_format_ref = GetDstFormat();

    RecordMap newMap;

    uint32 dstFields = GetDstFieldCount();
    uint32 srcFields = GetSrcFieldCount();
    const char* dstFmt = GetDstFormat();
    const char* srcFmt = GetSrcFormat();
    BarGoLink bar(recordCount);
    uint32 rowIdx = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 entryId = fields[0].GetUInt32();
        bar.step();

        char* record = &newBuf->data[rowIdx * recordSize];
        newMap.insert(RecordMap::value_type(entryId, record));
        ++newBuf->recordCount;
        ++rowIdx;

        uint32 offset = 0;
        for (uint32 x = 0, y = 0; x < dstFields;)
        {
            switch (dstFmt[x])
            {
                case DBC_FF_NA:
                { uint32 v = 0; memcpy(&record[offset], &v, sizeof(uint32)); offset += sizeof(uint32); ++x; continue; }
                case DBC_FF_NA_BYTE:
                    record[offset] = 0; offset += sizeof(char); ++x; continue;
                case DBC_FF_NA_FLOAT:
                { float v = 0.0f; memcpy(&record[offset], &v, sizeof(float)); offset += sizeof(float); ++x; continue; }
                case DBC_FF_NA_POINTER:
                { char const* v = nullptr; memcpy(&record[offset], &v, sizeof(char const*)); offset += sizeof(char const*); ++x; continue; }
                default: break;
            }
            if (y >= srcFields) break;
            switch (srcFmt[y])
            {
                case DBC_FF_LOGIC:
                { bool v = fields[y].GetUInt32() > 0; memcpy(&record[offset], &v, sizeof(bool)); offset += sizeof(bool); ++x; break; }
                case DBC_FF_BYTE:
                    record[offset] = fields[y].GetUInt8(); offset += sizeof(char); ++x; break;
                case DBC_FF_INT:
                { uint32 v = fields[y].GetUInt32(); memcpy(&record[offset], &v, sizeof(uint32)); offset += sizeof(uint32); ++x; break; }
                case DBC_FF_FLOAT:
                { float v = fields[y].GetFloat(); memcpy(&record[offset], &v, sizeof(float)); offset += sizeof(float); ++x; break; }
                case DBC_FF_STRING:
                {
                    const char* str = fields[y].GetString();
                    char* dst = nullptr;
                    if (!str) { dst = new char[1]; *dst = 0; }
                    else { uint32 l = strlen(str) + 1; dst = new char[l]; memcpy(dst, str, l); }
                    memcpy(&record[offset], &dst, sizeof(char*));
                    offset += sizeof(char*); ++x; break;
                }
                case DBC_FF_NA:
                case DBC_FF_NA_BYTE:
                case DBC_FF_NA_FLOAT:
                    break;
                default: break;
            }
            ++y;
        }
    }
    while (result->NextRow());
    delete result;

    auto oldAutoBuf = std::move(m_atomicBuffer);
    m_data = newBuf->data;
    m_indexMap = std::move(newMap);
    m_maxEntry = newBuf->maxEntry;
    m_recordCount = newBuf->recordCount;
    m_recordSize = newBuf->recordSize;

    if (oldAutoBuf)
    {
        oldAutoBuf->data = nullptr;
        oldAutoBuf->indexArray = nullptr;
    }
    m_atomicBuffer.reset(newBuf.release());

    sLog.outString("SQLHashStorage::ReloadAtomic: %s reloaded (%u entries).",
                   GetTableName(), m_recordCount);
}

void SQLHashStorage::Reload()
{
    ReloadAtomic();
}

void SQLMultiStorage::ReloadAtomic()
{
    sLog.outString("SQLMultiStorage::ReloadAtomic: Reloading %s (atomic) ...", GetTableName());

    QueryResult* result = WorldDatabase.PQuery("SELECT MAX(`%s`) FROM `%s`",
                                               EntryFieldName(), GetTableName());
    if (!result)
    {
        sLog.outString("SQLMultiStorage::ReloadAtomic: Table %s does not exist, skipping.\n", GetTableName());
        return;
    }
    uint32 maxRecordId = (*result)[0].GetUInt32() + 1;
    delete result;

    result = WorldDatabase.PQuery("SELECT COUNT(*) FROM `%s`", GetTableName());
    uint32 recordCount = 0;
    if (result)
    {
        recordCount = result->Fetch()[0].GetUInt32();
        delete result;
    }

    result = WorldDatabase.PQuery("SELECT * FROM `%s`", GetTableName());
    if (!result)
    {
        sLog.outString("%s table is empty!\n", GetTableName());
        return;
    }

    uint32 recordSize = 0;
    for (uint32 x = 0; x < GetDstFieldCount(); ++x)
    {
        switch (GetDstFormat(x))
        {
            case DBC_FF_LOGIC:   recordSize += sizeof(bool);   break;
            case DBC_FF_BYTE:    recordSize += sizeof(char);   break;
            case DBC_FF_INT:     recordSize += sizeof(uint32); break;
            case DBC_FF_FLOAT:   recordSize += sizeof(float);  break;
            case DBC_FF_STRING:  recordSize += sizeof(char*);  break;
            case DBC_FF_NA:      recordSize += sizeof(uint32); break;
            case DBC_FF_NA_BYTE: recordSize += sizeof(char);   break;
            case DBC_FF_NA_FLOAT:recordSize += sizeof(float);  break;
            case DBC_FF_NA_POINTER: recordSize += sizeof(char*); break;
            default: break;
        }
    }

    auto newBuf = std::make_unique<ReloadBuffer>();
    newBuf->data = new char[recordCount * recordSize]();
    newBuf->indexArray = nullptr;
    newBuf->recordCount = 0;
    newBuf->maxEntry = maxRecordId;
    newBuf->recordSize = recordSize;
    newBuf->dstFieldCount = GetDstFieldCount();
    newBuf->dst_format_ref = GetDstFormat();

    RecordMultiMap newMap;

    uint32 dstFields = GetDstFieldCount();
    uint32 srcFields = GetSrcFieldCount();
    const char* dstFmt = GetDstFormat();
    const char* srcFmt = GetSrcFormat();
    BarGoLink bar(recordCount);
    uint32 rowIdx = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 entryId = fields[0].GetUInt32();
        bar.step();

        char* record = &newBuf->data[rowIdx * recordSize];
        newMap.insert(RecordMultiMap::value_type(entryId, record));
        ++newBuf->recordCount;
        ++rowIdx;

        uint32 offset = 0;
        for (uint32 x = 0, y = 0; x < dstFields;)
        {
            switch (dstFmt[x])
            {
                case DBC_FF_NA:
                { uint32 v = 0; memcpy(&record[offset], &v, sizeof(uint32)); offset += sizeof(uint32); ++x; continue; }
                case DBC_FF_NA_BYTE:
                    record[offset] = 0; offset += sizeof(char); ++x; continue;
                case DBC_FF_NA_FLOAT:
                { float v = 0.0f; memcpy(&record[offset], &v, sizeof(float)); offset += sizeof(float); ++x; continue; }
                case DBC_FF_NA_POINTER:
                { char const* v = nullptr; memcpy(&record[offset], &v, sizeof(char const*)); offset += sizeof(char const*); ++x; continue; }
                default: break;
            }
            if (y >= srcFields) break;
            switch (srcFmt[y])
            {
                case DBC_FF_LOGIC:
                { bool v = fields[y].GetUInt32() > 0; memcpy(&record[offset], &v, sizeof(bool)); offset += sizeof(bool); ++x; break; }
                case DBC_FF_BYTE:
                    record[offset] = fields[y].GetUInt8(); offset += sizeof(char); ++x; break;
                case DBC_FF_INT:
                { uint32 v = fields[y].GetUInt32(); memcpy(&record[offset], &v, sizeof(uint32)); offset += sizeof(uint32); ++x; break; }
                case DBC_FF_FLOAT:
                { float v = fields[y].GetFloat(); memcpy(&record[offset], &v, sizeof(float)); offset += sizeof(float); ++x; break; }
                case DBC_FF_STRING:
                {
                    const char* str = fields[y].GetString();
                    char* dst = nullptr;
                    if (!str) { dst = new char[1]; *dst = 0; }
                    else { uint32 l = strlen(str) + 1; dst = new char[l]; memcpy(dst, str, l); }
                    memcpy(&record[offset], &dst, sizeof(char*));
                    offset += sizeof(char*); ++x; break;
                }
                case DBC_FF_NA:
                case DBC_FF_NA_BYTE:
                case DBC_FF_NA_FLOAT:
                    break;
                default: break;
            }
            ++y;
        }
    }
    while (result->NextRow());
    delete result;

    auto oldAutoBuf = std::move(m_atomicBuffer);
    m_data = newBuf->data;
    m_indexMultiMap = std::move(newMap);
    m_maxEntry = newBuf->maxEntry;
    m_recordCount = newBuf->recordCount;
    m_recordSize = newBuf->recordSize;

    if (oldAutoBuf)
    {
        oldAutoBuf->data = nullptr;
        oldAutoBuf->indexArray = nullptr;
    }
    m_atomicBuffer.reset(newBuf.release());

    sLog.outString("SQLMultiStorage::ReloadAtomic: %s reloaded (%u entries).",
                   GetTableName(), m_recordCount);
}

void SQLMultiStorage::Reload()
{
    ReloadAtomic();
}
