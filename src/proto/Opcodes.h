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

#ifndef MANGOS_PROTO_OPCODES_H
#define MANGOS_PROTO_OPCODES_H

// MangosTwo moved its entire opcode enum (1,379 lines) into proto/Opcodes.h.
// MangosThree deliberately does NOT: the 15595 table stays put, unmodified, in
// src/game/Server/Opcodes.h. This file defines only the handful of transport
// opcodes the protocol layer itself needs to speak -- values re-derived from
// (grepped out of, never copied by memory from) that same file so they can
// never drift from the live table.
//
// Every value below carries the exact line it was read from, on 2026-07-24:
//   MSG_WOW_CONNECTION   Opcodes.h:55  (0x4F57)
//   SMSG_AUTH_CHALLENGE  Opcodes.h:56  (0x4542)
//   CMSG_AUTH_SESSION    Opcodes.h:57  (0x0449)
//   SMSG_AUTH_RESPONSE   Opcodes.h:58  (0x5DB6)
//   MSG_NULL_ACTION      Opcodes.h:59  (0x1001)
//   CMSG_PING            Opcodes.h:555 (0x444D)
//   SMSG_PONG            Opcodes.h:556 (0x4D42)
//   CMSG_KEEP_ALIVE      Opcodes.h:1119 (0x0015)

#include "Platform/Define.h"

/**
 * @brief The transport-level subset of the 4.3.4 opcode space.
 *
 * `Utilities/WorldPacket.h` (in `shared`) stores its opcode as an `OpcodesList`
 * and quote-includes "Opcodes.h" to find that type -- a pre-existing shared/
 * game coupling this stage does not touch (WorldPacket's shape is wire-format
 * and off limits here). Within `game`, that include resolves to the real
 * 1,382-entry table; within `proto`, it resolves to this file instead, so this
 * enum must share the name and the handful of values proto actually uses.
 *
 * This is a deliberate, narrow duplication, not a second source of truth: as
 * long as `proto` is not linked into the same binary as `game` (true for this
 * checkpoint -- nothing calls into proto yet), the two enums never have to
 * agree bit-for-bit. Wiring proto into game (a later checkpoint) should first
 * do what MangosTwo did instead: change WorldPacket's opcode storage to a
 * plain uint16 (MangosTwo's WorldPacket.h takes `uint16 opcode`, not
 * `OpcodesList`) so shared stops needing an opcode enum at all. See the CP2
 * report for the discovery writeup.
 */
enum OpcodesList
{
    MSG_WOW_CONNECTION                                    = 0x4F57, // 4.3.4 15595
    SMSG_AUTH_CHALLENGE                                   = 0x4542, // 4.3.4 15595
    CMSG_AUTH_SESSION                                     = 0x0449, // 4.3.4 15595
    SMSG_AUTH_RESPONSE                                    = 0x5DB6, // 4.3.4 15595
    MSG_NULL_ACTION                                       = 0x1001, // 4.3.4 15595
    CMSG_PING                                             = 0x444D, // 4.3.4 15595
    SMSG_PONG                                             = 0x4D42, // 4.3.4 15595
    CMSG_KEEP_ALIVE                                       = 0x0015  // 4.3.4 15595
};

/**
 * @brief Stand-in for `game`'s real opcode-name table.
 *
 * `WorldPacket::GetOpcodeName()` (shared/Utilities/WorldPacket.h) calls this by
 * name, unconditionally -- it is compiled as part of the class body the
 * moment the header is parsed, whether or not anything actually calls it, so
 * something named `LookupOpcodeName` must exist wherever `proto` compiles
 * WorldPacket.h. The real, game-owned table (`src/game/Server/Opcodes.h`,
 * `opcodeTable`) is out of reach from here by design; nothing in `proto`
 * calls `GetOpcodeName()`, so this only needs to type-check.
 */
inline const char* LookupOpcodeName(uint16 /*id*/)
{
    return "UNKNOWN OPCODE (proto)";
}

#endif
