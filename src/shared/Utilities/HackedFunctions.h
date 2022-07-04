/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2022 MaNGOS <https://getmangos.eu>
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
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

// Hack, function taken from c++ standard files

#ifndef HACKED_FUNCTIONS_H
#define HACKED_FUNCTIONS_H

namespace Mangos
{
    /* original binary_function code */
    template<typename _Arg1, typename _Arg2, typename _Result>
    struct binary_function
    {
        typedef _Arg1   first_argument_type;
        typedef _Arg2   second_argument_type;
        typedef _Result result_type;
    };

    /* original unary_function code */
    template<typename _Arg, typename _Result>
    struct unary_function
    {
        typedef _Arg    argument_type;
        typedef _Result result_type;
    };
};

#endif
