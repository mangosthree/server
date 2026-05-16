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

#ifndef MANGOS_H_BYTEBUFFER
#define MANGOS_H_BYTEBUFFER

#include "Common/Common.h"
#include "Log/Log.h"
#include "Utilities/ByteConverter.h"
#include "Utilities/Errors.h"

#define BITS_1 uint8 _1
#define BITS_2 BITS_1, uint8 _2
#define BITS_3 BITS_2, uint8 _3
#define BITS_4 BITS_3, uint8 _4
#define BITS_5 BITS_4, uint8 _5
#define BITS_6 BITS_5, uint8 _6
#define BITS_7 BITS_6, uint8 _7
#define BITS_8 BITS_7, uint8 _8

#define BIT_VALS_1 _1
#define BIT_VALS_2 BIT_VALS_1, _2
#define BIT_VALS_3 BIT_VALS_2, _3
#define BIT_VALS_4 BIT_VALS_3, _4
#define BIT_VALS_5 BIT_VALS_4, _5
#define BIT_VALS_6 BIT_VALS_5, _6
#define BIT_VALS_7 BIT_VALS_6, _7
#define BIT_VALS_8 BIT_VALS_7, _8

class ObjectGuid;

/**
 * @brief Exception thrown when ByteBuffer operations exceed buffer bounds
 *
 * ByteBufferException is raised when attempting to read or write data beyond
 * the current buffer capacity. Provides detailed position and size information
 * for debugging buffer overflow conditions.
 */
class ByteBufferException
{
    public:
    /**
     * @brief Constructs a new ByteBufferException
     *
     * @param _add True if exception occurred during append/write operation, false for read
     * @param _pos Current position in buffer where overflow occurred
     * @param _esize Size of element that was being added/read
     * @param _size Total size of the buffer
     */
        ByteBufferException(bool _add, size_t _pos, size_t _esize, size_t _size)
            : add(_add), pos(_pos), esize(_esize), size(_size)
        {
            PrintPosError();
        }

        /**
         * @brief Prints detailed error information about the buffer overflow
         *
         * Outputs information about the position, operation type, and buffer bounds.
         */
        void PrintPosError() const
        {
            char const* traceStr;

#ifdef HAVE_ACE_STACK_TRACE_H
            ACE_Stack_Trace trace;
            traceStr = trace.c_str();
#else
            traceStr = NULL;
#endif

            sLog.outError(
                "Attempted to %s in ByteBuffer (pos: %zu size: %zu) "
                "value with size: %zu%s%s",
                (add ? "put" : "get"), pos, size, esize,
                traceStr ? "\n" : "", traceStr ? traceStr : "");
        }
    private:
        bool add; /**< True if error occurred during write/append operation */
        size_t pos; /**< Position in buffer where overflow occurred */
        size_t esize; /**< Size of the element being read/written */
        size_t size; /**< Total size of the buffer */
};

class BitStream
{
    public:
        BitStream(): _rpos(0), _wpos(0) {}

        BitStream(uint32 val, size_t len): _rpos(0), _wpos(0)
        {
            WriteBits(val, len);
        }

        BitStream(BitStream const& bs) : _rpos(bs._rpos), _wpos(bs._wpos), _data(bs._data) {}

        void Clear();
        uint8 GetBit(uint32 bit);
        uint8 ReadBit();
        void WriteBit(uint32 bit);
        template <typename T> void WriteBits(T value, size_t bits);
        bool Empty();
        void Reverse();
        void Print();

        size_t GetLength() { return _data.size(); }
        uint32 GetReadPosition() { return _rpos; }
        uint32 GetWritePosition() { return _wpos; }
        void SetReadPos(uint32 pos) { _rpos = pos; }

        uint8 const& operator[](uint32 const pos) const
        {
            return _data[pos];
        }

        uint8& operator[] (uint32 const pos)
        {
            return _data[pos];
        }

    private:
        std::vector<uint8> _data;
        uint32 _rpos, _wpos;
};

template<class T>
/**
 * @brief Template for marking unused template parameters
 *
 * Provides a clean way to suppress compiler warnings about unused template parameters
 * in template specializations and other scenarios where a template parameter must be
 * declared but is intentionally not used.
 */
struct Unused
{
/**
 * @brief Constructs an Unused instance
 */
    Unused() {}
};

/**
 * @brief Binary buffer for network packet serialization and deserialization
 *
 * ByteBuffer provides a container for binary data with methods to read/write
 * various data types in network byte order. It's essential for World of Warcraft
 * protocol handling, allowing proper serialization of client-server packets.
 *
 * Features:
 * - Automatic network byte order conversion (little-endian)
 * - Read/write position tracking
 * - Exception handling for buffer overflows
 * - Support for all basic C++ types and strings
 * - Packed GUID support for efficient network transmission
 *
 * @note This is the primary class used for all WoW protocol communication
 * @note All write operations advance the write position, all reads advance read position
 */
class ByteBuffer
{
    public:
        const static size_t DEFAULT_SIZE = 64;

        // constructor
        ByteBuffer(): _rpos(0), _wpos(0), _bitpos(8), _curbitval(0)
        {
            _storage.reserve(DEFAULT_SIZE);
        }

        // constructor
        ByteBuffer(size_t res): _rpos(0), _wpos(0), _bitpos(8), _curbitval(0)
        {
            _storage.reserve(res);
        }

        /**
         * @brief Copy constructor
         *
         * Creates a new ByteBuffer with the same content and positions as the source buffer.
         * Both read and write positions are copied.
         *
         * @param buf Source ByteBuffer to copy from
         */
        ByteBuffer(const ByteBuffer &buf) : _rpos(buf._rpos), _wpos(buf._wpos),
            _storage(buf._storage), _bitpos(buf._bitpos), _curbitval(buf._curbitval)
        {
        }

        /**
         * @brief Clear the buffer and reset positions
         *
         * Removes all data from the buffer and resets both read and write
         * positions to zero. Equivalent to creating a new empty buffer.
         */
        void clear()
        {
            _storage.clear();
            _rpos = _wpos = 0;
            _curbitval = 0;
            _bitpos = 8;
        }

        template <typename T> ByteBuffer& append(T value)
        {
            FlushBits();
            EndianConvert(value);
            return append((uint8*)&value, sizeof(value));
        }

        void FlushBits()
        {
            if (_bitpos == 8)
            {
                return;
            }

            append((uint8 *)&_curbitval, sizeof(uint8));
            _curbitval = 0;
            _bitpos = 8;
        }

        void ResetBitReader()
        {
            _bitpos = 8;
        }

        template <typename T> bool WriteBit(T bit)
        {
            --_bitpos;
            if (bit)
            {
                _curbitval |= (1 << (_bitpos));
            }

            if (_bitpos == 0)
            {
                _bitpos = 8;
                append((uint8 *)&_curbitval, sizeof(_curbitval));
                _curbitval = 0;
            }

            return (bit != 0);
        }

        bool ReadBit()
        {
            ++_bitpos;
            if (_bitpos > 7)
            {
                _curbitval = read<uint8>();
                _bitpos = 0;
            }

            return ((_curbitval >> (7-_bitpos)) & 1) != 0;
        }

        template <typename T> void WriteBits(T value, size_t bits)
        {
            for (int32 i = bits-1; i >= 0; --i)
            {
                WriteBit((value >> i) & 1);
            }
        }

        uint32 ReadBits(size_t bits)
        {
            uint32 value = 0;
            for (int32 i = bits-1; i >= 0; --i)
                if (ReadBit())
                {
                    value |= (1 << i);
                }

            return value;
        }

        BitStream ReadBitStream(uint32 len)
        {
            BitStream b;
            for (uint32 i = 0; i < len; ++i)
            {
                b.WriteBit(ReadBit());
            }
            return b;
        }

        void WriteGuidMask(uint64 guid, uint8* maskOrder, uint8 maskCount, uint8 maskPos = 0)
        {
            uint8* guidByte = ((uint8*)&guid);

            for (uint8 i = 0; i < maskCount; i++)
            {
                WriteBit(guidByte[maskOrder[i + maskPos]]);
            }
        }

        void WriteGuidBytes(uint64 guid, uint8* byteOrder, uint8 byteCount, uint8 bytePos)
        {
            uint8* guidByte = ((uint8*)&guid);

            for (uint8 i = 0; i < byteCount; i++)
                if (guidByte[byteOrder[i + bytePos]])
                {
                    (*this) << uint8(guidByte[byteOrder[i + bytePos]] ^ 1);
                }
        }

        template<BITS_1>
            void ReadGuidMask(ObjectGuid& guid);
        template<BITS_2>
            void ReadGuidMask(ObjectGuid& guid);
        template<BITS_3>
            void ReadGuidMask(ObjectGuid& guid);
        template<BITS_4>
            void ReadGuidMask(ObjectGuid& guid);
        template<BITS_5>
            void ReadGuidMask(ObjectGuid& guid);
        template<BITS_6>
            void ReadGuidMask(ObjectGuid& guid);
        template<BITS_7>
            void ReadGuidMask(ObjectGuid& guid);
        template<BITS_8>
            void ReadGuidMask(ObjectGuid& guid);

        template<BITS_1>
            void WriteGuidMask(ObjectGuid guid);
        template<BITS_2>
            void WriteGuidMask(ObjectGuid guid);
        template<BITS_3>
            void WriteGuidMask(ObjectGuid guid);
        template<BITS_4>
            void WriteGuidMask(ObjectGuid guid);
        template<BITS_5>
            void WriteGuidMask(ObjectGuid guid);
        template<BITS_6>
            void WriteGuidMask(ObjectGuid guid);
        template<BITS_7>
            void WriteGuidMask(ObjectGuid guid);
        template<BITS_8>
            void WriteGuidMask(ObjectGuid guid);

        template<BITS_1>
            void ReadGuidBytes(ObjectGuid& guid);
        template<BITS_2>
            void ReadGuidBytes(ObjectGuid& guid);
        template<BITS_3>
            void ReadGuidBytes(ObjectGuid& guid);
        template<BITS_4>
            void ReadGuidBytes(ObjectGuid& guid);
        template<BITS_5>
            void ReadGuidBytes(ObjectGuid& guid);
        template<BITS_6>
            void ReadGuidBytes(ObjectGuid& guid);
        template<BITS_7>
            void ReadGuidBytes(ObjectGuid& guid);
        template<BITS_8>
            void ReadGuidBytes(ObjectGuid& guid);

        template<BITS_1>
            void WriteGuidBytes(ObjectGuid guid);
        template<BITS_2>
            void WriteGuidBytes(ObjectGuid guid);
        template<BITS_3>
            void WriteGuidBytes(ObjectGuid guid);
        template<BITS_4>
            void WriteGuidBytes(ObjectGuid guid);
        template<BITS_5>
            void WriteGuidBytes(ObjectGuid guid);
        template<BITS_6>
            void WriteGuidBytes(ObjectGuid guid);
        template<BITS_7>
            void WriteGuidBytes(ObjectGuid guid);
        template<BITS_8>
            void WriteGuidBytes(ObjectGuid guid);

        /**
         * @brief Insert value at specific position in buffer
         *
         * Places a value at the specified position without affecting current
         * read/write positions. Useful for modifying existing data.
         *
         * @param pos Position in buffer where to insert value
         * @param value Value to insert (will be endian-converted)
         */
        template <typename T> void put(size_t pos, T value)
        {
            EndianConvert(value);
            put(pos, (uint8*)&value, sizeof(value));
        }

        /**
         * @brief Append uint8 value to buffer
         *
         * Stream operator for convenient appending of uint8 values.
         * Advances write position by 1 byte.
         *
         * @param value Byte value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(uint8 value)
        {
            append<uint8>(value);
            return *this;
        }

        /**
         * @brief Append uint16 value to buffer
         *
         * Stream operator for convenient appending of uint16 values.
         * Value is automatically endian-converted. Advances write position by 2 bytes.
         *
         * @param value 16-bit value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(uint16 value)
        {
            append<uint16>(value);
            return *this;
        }

        /**
         * @brief Append uint32 value to buffer
         *
         * Stream operator for convenient appending of uint32 values.
         * Value is automatically endian-converted. Advances write position by 4 bytes.
         *
         * @param value 32-bit value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(uint32 value)
        {
            append<uint32>(value);
            return *this;
        }

        /**
         * @brief Append uint64 value to buffer
         *
         * Stream operator for convenient appending of uint64 values.
         * Value is automatically endian-converted. Advances write position by 8 bytes.
         *
         * @param value 64-bit value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(uint64 value)
        {
            append<uint64>(value);
            return *this;
        }

        /**
         * @brief Append signed int8 value to buffer
         *
         * Stream operator for appending signed 8-bit integers (2's complement).
         * Advances write position by 1 byte.
         *
         * @param value Signed byte value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(int8 value)
        {
            append<int8>(value);
            return *this;
        }

        /**
         * @brief Append signed int16 value to buffer
         *
         * Stream operator for appending signed 16-bit integers (2's complement).
         * Value is automatically endian-converted. Advances write position by 2 bytes.
         *
         * @param value Signed 16-bit value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(int16 value)
        {
            append<int16>(value);
            return *this;
        }

        /**
         * @brief Append signed int32 value to buffer
         *
         * Stream operator for appending signed 32-bit integers (2's complement).
         * Value is automatically endian-converted. Advances write position by 4 bytes.
         *
         * @param value Signed 32-bit value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(int32 value)
        {
            append<int32>(value);
            return *this;
        }

        /**
         * @brief Append signed int64 value to buffer
         *
         * Stream operator for appending signed 64-bit integers (2's complement).
         * Value is automatically endian-converted. Advances write position by 8 bytes.
         *
         * @param value Signed 64-bit value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(int64 value)
        {
            append<int64>(value);
            return *this;
        }

        /**
         * @brief Append floating-point value to buffer
         *
         * Stream operator for appending floating-point values.
         * Value is automatically endian-converted. Advances write position by 4 bytes.
         *
         * @param value Float value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(float value)
        {
            append<float>(value);
            return *this;
        }

        /**
         * @brief Append double-precision floating-point value to buffer
         *
         * Stream operator for appending double-precision values.
         * Value is automatically endian-converted. Advances write position by 8 bytes.
         *
         * @param value Double value to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(double value)
        {
            append<double>(value);
            return *this;
        }

        /**
         * @brief Append null-terminated string to buffer
         *
         * Appends the string content followed by a null terminator.
         * Useful for transmitting string data over the network protocol.
         *
         * @param value String to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(const std::string& value)
        {
            append((uint8 const*)value.c_str(), value.length());
            append((uint8)0);
            return *this;
        }

        /**
         * @brief Append C-string to buffer
         *
         * Appends a C-string followed by a null terminator.
         * Safely handles null pointers by writing nothing.
         *
         * @param str C-string to append
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator<<(const char* str)
        {
            append((uint8 const*)str, str ? strlen(str) : 0);
            append((uint8)0);
            return *this;
        }

        /**
         * @brief Extract boolean value from buffer
         *
         * Stream operator for reading and extracting a boolean value from the buffer.
         * Non-zero values are interpreted as true. Advances read position by 1 byte.
         *
         * @param value Boolean reference to store extracted value
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator>>(bool& value)
        {
            value = read<char>() > 0 ? true : false;
            return *this;
        }

        /**
         * @brief Extract uint8 value from buffer
         *
         * Stream operator for reading and extracting an unsigned byte from the buffer.
         * Advances read position by 1 byte.
         *
         * @param value Uint8 reference to store extracted value
         * @return Reference to this ByteBuffer for chaining
         */
        ByteBuffer& operator>>(uint8& value)
        {
            value = read<uint8>();
            return *this;
        }

        /**
         * @brief
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(uint16& value)
        {
            value = read<uint16>();
            return *this;
        }

        /**
         * @brief
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(uint32& value)
        {
            value = read<uint32>();
            return *this;
        }

        /**
         * @brief
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(uint64& value)
        {
            value = read<uint64>();
            return *this;
        }

        /**
         * @brief signed as in 2e complement
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(int8& value)
        {
            value = read<int8>();
            return *this;
        }

        /**
         * @brief
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(int16& value)
        {
            value = read<int16>();
            return *this;
        }

        /**
         * @brief
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(int32& value)
        {
            value = read<int32>();
            return *this;
        }

        /**
         * @brief
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(int64& value)
        {
            value = read<int64>();
            return *this;
        }

        /**
         * @brief
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(float& value)
        {
            value = read<float>();
            return *this;
        }

        /**
         * @brief
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(double& value)
        {
            value = read<double>();
            return *this;
        }

        /**
         * @brief
         *
         * @param value
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(std::string& value)
        {
            value.clear();
            while (rpos() < size())                         // prevent crash at wrong string format in packet
            {
                char c = read<char>();
                if (c == 0)
                {
                    break;
                }
                value += c;
            }
            return *this;
        }

        template<class T>
        /**
         * @brief
         *
         * @param
         * @return ByteBuffer &operator >>
         */
        ByteBuffer& operator>>(Unused<T> const&)
        {
            return read_skip<T>();
        }

        uint8& operator[](size_t const pos)
        {
            if (pos >= size())
            {
                throw ByteBufferException(false, pos, 1, size());
            }
            return _storage[pos];
        }

        uint8 const& operator[](size_t const pos) const
        {
            if (pos >= size())
            {
                throw ByteBufferException(false, pos, 1, size());
            }
            return _storage[pos];
        }

        /**
         * @brief
         *
         * @return size_t
         */
        size_t rpos() const { return _rpos; }

        /**
         * @brief
         *
         * @param rpos_
         * @return size_t
         */
        size_t rpos(size_t rpos_)
        {
            _rpos = rpos_;
            return _rpos;
        }

        void rfinish()
        {
            _rpos = wpos();
        }

        /**
         * @brief
         *
         * @return size_t
         */
        size_t wpos() const { return _wpos; }

        /**
         * @brief
         *
         * @param wpos_
         * @return size_t
         */
        size_t wpos(size_t wpos_)
        {
            _wpos = wpos_;
            return _wpos;
        }

        template<typename T>
        ByteBuffer& read_skip()
        {
            read_skip(sizeof(T));
            return *this;
        }

        ByteBuffer& read_skip(size_t skip)
        {
            ResetBitReader();
            if (_rpos + skip > size())
            {
                throw ByteBufferException(false, _rpos, skip, size());
            }
            _rpos += skip;

            return *this;
        }

        /**
         * @brief
         *
         * @return T
         */
        template <typename T> T read()
        {
            ResetBitReader();
            T r = read<T>(_rpos);
            _rpos += sizeof(T);
            return r;
        }

        /**
         * @brief
         *
         * @param pos
         * @return T
         */
        template <typename T> T read(size_t pos) const
        {
            if (pos + sizeof(T) > size())
            {
                throw ByteBufferException(false, pos, sizeof(T), size());
            }
#if defined(__arm__)
            // ARM has alignment issues, we need to use memcpy to avoid them
            T val;
            memcpy((void*)&val, (void*)&_storage[pos], sizeof(T));
#else
            T val = *((T const*)&_storage[pos]);
#endif

            EndianConvert(val);
            return val;
        }

        /**
         * @brief
         *
         * @param dest
         * @param len
         */
        ByteBuffer& read(uint8* dest, size_t len)
        {
            ResetBitReader();
            if (_rpos  + len > size())
            {
                throw ByteBufferException(false, _rpos, len, size());
            }
            memcpy(dest, &_storage[_rpos], len);
            _rpos += len;

            return *this;
        }

        /**
         * @brief
         *
         * @return uint64
         */
        uint64 readPackGUID()
        {
            uint64 guid = 0;
            uint8 guidmark = 0;
            (*this) >> guidmark;

            for (int i = 0; i < 8; ++i)
            {
                if (guidmark & (uint8(1) << i))
                {
                    uint8 bit;
                    (*this) >> bit;
                    guid |= (uint64(bit) << (i * 8));
                }
            }

            return guid;
        }

        uint8 ReadUInt8()
        {
            uint8 u = 0;
            (*this) >> u;
            return u;
        }

        uint16 ReadUInt16()
        {
            uint16 u = 0;
            (*this) >> u;
            return u;
        }

        uint32 ReadUInt32()
        {
            uint32 u = 0;
            (*this) >> u;
            return u;
        }

        uint64 ReadUInt64()
        {
            uint64 u = 0;
            (*this) >> u;
            return u;
        }

        int8 ReadInt8()
        {
            int8 u = 0;
            (*this) >> u;
            return u;
        }

        int16 ReadInt16()
        {
            int16 u = 0;
            (*this) >> u;
            return u;
        }

        int32 ReadInt32()
        {
            uint32 u = 0;
            (*this) >> u;
            return u;
        }

        int64 ReadInt64()
        {
            int64 u = 0;
            (*this) >> u;
            return u;
        }

        std::string ReadString()
        {
            std::string s = 0;
            (*this) >> s;
            return s;
        }

        std::string ReadString(uint32 count)
        {
            std::string out;
            uint32 start = rpos();
            while (rpos() < size() && rpos() < start + count)       // prevent crash at wrong string format in packet
                out += read<char>();

            return out;
        }

        ByteBuffer& WriteStringData(const std::string& str)
        {
            FlushBits();
            return append((uint8 const*)str.c_str(), str.size());
        }

        bool ReadBoolean()
        {
            uint8 b = 0;
            (*this) >> b;
            return b > 0 ? true : false;
        }

        float ReadSingle()
        {
            float f = 0;
            (*this) >> f;
            return f;
        }

        /**
         * @brief
         *
         * @return const uint8
         */
        const uint8* contents() const { return &_storage[0]; }

        /**
         * @brief
         *
         * @return size_t
         */
        size_t size() const { return _storage.size(); }
        /**
         * @brief
         *
         * @return bool
         */
        bool empty() const { return _storage.empty(); }

        /**
         * @brief
         *
         * @param newsize
         */
        void resize(size_t newsize)
        {
            _storage.resize(newsize);
            _rpos = 0;
            _wpos = size();
        }

        /**
         * @brief
         *
         * @param ressize
         */
        void reserve(size_t ressize)
        {
            if (ressize > size())
            {
                _storage.reserve(ressize);
            }
        }

        /**
         * @brief
         *
         * @param str
         */
        ByteBuffer& append(const std::string& str)
        {
            return append((uint8 const*)str.c_str(), str.size() + 1);
        }

        /**
         * @brief
         *
         * @param src
         * @param cnt
         */
        ByteBuffer& append(const char* src, size_t cnt)
        {
            return append((const uint8*)src, cnt);
        }

        /**
         * @brief
         *
         * @param src
         * @param cnt
         */
        template<class T> ByteBuffer& append(const T* src, size_t cnt)
        {
            return append((const uint8*)src, cnt * sizeof(T));
        }

        /**
         * @brief
         *
         * @param src
         * @param cnt
         */
        ByteBuffer& append(const uint8* src, size_t cnt)
        {
            if (!cnt)
            {
                return *this;
            }

            MANGOS_ASSERT(size() < 10000000);

            if (_storage.size() < _wpos + cnt)
            {
                _storage.resize(_wpos + cnt);
            }
            memcpy(&_storage[_wpos], src, cnt);
            _wpos += cnt;

            return *this;
        }

        /**
         * @brief
         *
         * @param buffer
         */
        ByteBuffer& append(const ByteBuffer& buffer)
        {
            if (buffer.wpos())
            {
                return append(buffer.contents(), buffer.wpos());
            }
            return *this;
        }

        /**
         * @brief can be used in SMSG_MONSTER_MOVE opcode
         *
         * @param x
         * @param y
         * @param z
         */
        ByteBuffer& appendPackXYZ(float x, float y, float z)
        {
            uint32 packed = 0;
            packed |= ((int)(x / 0.25f) & 0x7FF);
            packed |= ((int)(y / 0.25f) & 0x7FF) << 11;
            packed |= ((int)(z / 0.25f) & 0x3FF) << 22;
            *this << packed;
            return *this;
        }

        /**
         * @brief
         *
         * @param guid
         */
        ByteBuffer& appendPackGUID(uint64 guid)
        {
            uint8 packGUID[8 + 1];
            packGUID[0] = 0;
            size_t size = 1;
            for (uint8 i = 0; guid != 0; ++i)
            {
                if (guid & 0xFF)
                {
                    packGUID[0] |= uint8(1 << i);
                    packGUID[size] =  uint8(guid & 0xFF);
                    ++size;
                }

                guid >>= 8;
            }

            return append(packGUID, size);
        }

        /**
         * @brief
         *
         * @param pos
         * @param src
         * @param cnt
         */
        void put(size_t pos, const uint8* src, size_t cnt)
        {
            if (pos + cnt > size())
            {
                throw ByteBufferException(true, pos, cnt, size());
            }
            memcpy(&_storage[pos], src, cnt);
        }

        /**
         * @brief
         *
         */
        void print_storage() const
        {
            sLog.outDebug("STORAGE_SIZE: %lu", (unsigned long)size() );
            for (uint32 i = 0; i < size(); ++i)
            {
                sLog.outDebug("%u - ", read<uint8>(i) );
            }
            sLog.outDebug(" ");
        }

        void textlike() const
        {
            sLog.outDebug("STORAGE_SIZE: %lu", (unsigned long)size() );
            for (uint32 i = 0; i < size(); ++i)
            {
                sLog.outDebug("%c", read<uint8>(i) );
            }
            sLog.outDebug(" ");
        }

        void hexlike() const
        {
            uint32 j = 1, k = 1;
            sLog.outDebug("STORAGE_SIZE: %lu", (unsigned long)size() );

            for (uint32 i = 0; i < size(); ++i)
            {
                if ((i == (j * 8)) && ((i != (k * 16))))
                {
                    if (read<uint8>(i) < 0x10)
                    {
                        sLog.outDebug("| 0%X ", read<uint8>(i) );
                    }
                    else
                    {
                        sLog.outDebug("| %X ", read<uint8>(i) );
                    }
                    ++j;
                }
                else if (i == (k * 16))
                {
                    if (read<uint8>(i) < 0x10)
                    {
                        sLog.outDebug("\n");

                        sLog.outDebug("0%X ", read<uint8>(i) );
                    }
                    else
                    {
                        sLog.outDebug("\n");

                        sLog.outDebug("%X ", read<uint8>(i) );
                    }

                    ++k;
                    ++j;
                }
                else
                {
                    if (read<uint8>(i) < 0x10)
                    {
                        sLog.outDebug("0%X ", read<uint8>(i) );
                    }
                    else
                    {
                        sLog.outDebug("%X ", read<uint8>(i) );
                    }
                }
            }
            sLog.outDebug("\n");
        }

    protected:
        size_t _rpos, _wpos, _bitpos;
        uint8 _curbitval;
        std::vector<uint8> _storage; /**< TODO */
};

template <typename T>
/**
 * @brief
 *
 * @param b
 * @param v
 * @return ByteBuffer &operator
 */
inline ByteBuffer& operator<<(ByteBuffer& b, std::vector<T> const& v)
{
    b << (uint32)v.size();
    for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
/**
 * @brief
 *
 * @param b
 * @param v
 * @return ByteBuffer &operator >>
 */
inline ByteBuffer& operator>>(ByteBuffer& b, std::vector<T>& v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename T>
/**
 * @brief
 *
 * @param b
 * @param v
 * @return ByteBuffer &operator
 */
inline ByteBuffer& operator<<(ByteBuffer& b, std::list<T> const& v)
{
    b << (uint32)v.size();
    for (typename std::list<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
/**
 * @brief
 *
 * @param b
 * @param v
 * @return ByteBuffer &operator >>
 */
inline ByteBuffer& operator>>(ByteBuffer& b, std::list<T>& v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename K, typename V>
/**
 * @brief
 *
 * @param b
 * @param std::map<K
 * @param m
 * @return ByteBuffer &operator
 */
inline ByteBuffer& operator<<(ByteBuffer& b, std::map<K, V>& m)
{
    b << (uint32)m.size();
    for (typename std::map<K, V>::iterator i = m.begin(); i != m.end(); ++i)
    {
        b << i->first << i->second;
    }
    return b;
}

template <typename K, typename V>
/**
 * @brief
 *
 * @param b
 * @param std::map<K
 * @param m
 * @return ByteBuffer &operator >>
 */
inline ByteBuffer& operator>>(ByteBuffer& b, std::map<K, V>& m)
{
    uint32 msize;
    b >> msize;
    m.clear();
    while (msize--)
    {
        K k;
        V v;
        b >> k >> v;
        m.insert(make_pair(k, v));
    }
    return b;
}

// TODO: Make a ByteBuffer.cpp and move all this inlining to it.
template<> inline std::string ByteBuffer::read<std::string>()
{
    std::string tmp;
    *this >> tmp;
    return tmp;
}

template<>
inline ByteBuffer& ByteBuffer::read_skip<char*>()
{
    std::string temp;
    *this >> temp;

    return *this;
}

template<>
inline ByteBuffer& ByteBuffer::read_skip<char const*>()
{
    return read_skip<char*>();
}

template<>
inline ByteBuffer& ByteBuffer::read_skip<std::string>()
{
    return read_skip<char*>();
}

class BitConverter
{
    public:
        static uint8 ToUInt8(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<uint8>(start);
        }

        static uint16 ToUInt16(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<uint16>(start);
        }

        static uint32 ToUInt32(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<uint32>(start);
        }

        static uint64 ToUInt64(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<uint64>(start);
        }

        static int16 ToInt16(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<int16>(start);
        }

        static int32 ToInt32(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<int32>(start);
        }

        static int64 ToInt64(ByteBuffer const& buff, size_t start = 0)
        {
            return buff.read<int64>(start);
        }
};

#endif
