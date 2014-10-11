/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2014  MaNGOS project <http://getmangos.eu>
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
#ifndef MANGOSSERVER_PROGRESSBAR_H
#define MANGOSSERVER_PROGRESSBAR_H

#include "Platform/Define.h"

class MANGOS_DLL_SPEC BarGoLink
{
    public:                                                 // constructors
        explicit BarGoLink(int row_count);
        explicit BarGoLink(uint32 row_count);               // row_count < ACE_INT32_MAX
        explicit BarGoLink(uint64 row_count);               // row_count < ACE_INT32_MAX
        ~BarGoLink();

    public:                                                 // modifiers
        void step();

        static void SetOutputState(bool on);
    private:
        void init(int row_count);

        static bool m_showOutput;                           // not recommended change with existed active bar
        static char const* const empty;
        static char const* const full;

        int rec_no;
        int rec_pos;
        int num_rec;
        int indic_len;
};
#endif
