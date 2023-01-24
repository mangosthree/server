/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2023 MaNGOS <https://getmangos.eu>
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

#ifndef _AUTH_SHA1_H
#define _AUTH_SHA1_H

#include "Common/Common.h"
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <cstring>

class BigNumber;

class Sha1Hash
{
    public:
        Sha1Hash();
        Sha1Hash(Sha1Hash const& other);     // copy
        Sha1Hash(Sha1Hash&& other);          // move
        Sha1Hash& operator=(Sha1Hash other); // assign
        ~Sha1Hash();

        void swap(Sha1Hash& other) throw();
        friend void swap(Sha1Hash& left, Sha1Hash& right) { left.swap(right); }

        void UpdateBigNumbers(BigNumber* bn0, ...);

        void UpdateData(const uint8* dta, int len);
        void UpdateData(const std::string& str);

        void Initialize();
        void Finalize();

        uint8* GetDigest() { return m_digest; };
        static int GetLength() { return SHA_DIGEST_LENGTH; };

    private:
        EVP_MD_CTX* m_ctx;
        uint8 m_digest[SHA_DIGEST_LENGTH];
};

#endif
