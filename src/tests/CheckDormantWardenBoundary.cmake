# SPDX-License-Identifier: GPL-3.0-or-later
#
# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
#
# Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

if(NOT DEFINED SOURCE_ROOT)
    get_filename_component(SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(OPCODE_TABLE "${SOURCE_ROOT}/src/game/Server/OpcodeTable.cpp")
set(SESSION_HEADER "${SOURCE_ROOT}/src/game/Server/WorldSession.h")
set(HANDLER_SOURCE "${SOURCE_ROOT}/src/game/WorldHandlers/WardenHandler.cpp")

foreach(REQUIRED_PATH OPCODE_TABLE SESSION_HEADER HANDLER_SOURCE)
    if(NOT EXISTS "${${REQUIRED_PATH}}")
        message(FATAL_ERROR
            "Dormant Warden boundary artifact is missing: ${${REQUIRED_PATH}}")
    endif()
endforeach()

file(STRINGS "${OPCODE_TABLE}" OPCODE_LINES)
set(CMSG_COUNT 0)
set(SMSG_COUNT 0)
foreach(LINE IN LISTS OPCODE_LINES)
    if(LINE MATCHES "^[ \t]*//")
        continue()
    endif()

    string(REGEX REPLACE "[ \t]" "" COMPACT_LINE "${LINE}")
    if(COMPACT_LINE STREQUAL
        "OPCODE(CMSG_WARDEN_DATA,STATUS_AUTHED,PROCESS_THREADUNSAFE,&WorldSession::HandleWardenDataOpcode);")
        math(EXPR CMSG_COUNT "${CMSG_COUNT} + 1")
    elseif(COMPACT_LINE STREQUAL
        "OPCODE(SMSG_WARDEN_DATA,STATUS_NEVER,PROCESS_INPLACE,&WorldSession::Handle_ServerSide);")
        math(EXPR SMSG_COUNT "${SMSG_COUNT} + 1")
    endif()
endforeach()

if(NOT CMSG_COUNT EQUAL 1)
    message(FATAL_ERROR
        "CMSG_WARDEN_DATA must have one authenticated, thread-unsafe drain registration")
endif()
if(NOT SMSG_COUNT EQUAL 1)
    message(FATAL_ERROR
        "SMSG_WARDEN_DATA must have one dormant server-side registration")
endif()

file(READ "${SESSION_HEADER}" SESSION_TEXT)
string(FIND "${SESSION_TEXT}"
    "void HandleWardenDataOpcode(WorldPacket& recv_data);"
    SESSION_DECLARATION_POSITION)
if(SESSION_DECLARATION_POSITION EQUAL -1)
    message(FATAL_ERROR
        "WorldSession must declare the Warden drain handler")
endif()

file(READ "${HANDLER_SOURCE}" HANDLER_TEXT)
string(REPLACE "\r\n" "\n" HANDLER_TEXT "${HANDLER_TEXT}")
set(EXPECTED_HANDLER
    "void WorldSession::HandleWardenDataOpcode(WorldPacket& recv_data)\n{\n    recv_data.rfinish();\n}")
string(FIND "${HANDLER_TEXT}" "${EXPECTED_HANDLER}" HANDLER_POSITION)
if(HANDLER_POSITION EQUAL -1)
    message(FATAL_ERROR
        "Warden handler must drain the complete packet without interpreting it")
endif()

foreach(FORBIDDEN_PATTERN
    "DEBUG_LOG\\(" "ERROR_LOG\\(" "sLog" "operator>>" ">>"
    "\\.read" "Decrypt" "_warden")
    if(HANDLER_TEXT MATCHES "${FORBIDDEN_PATTERN}")
        message(FATAL_ERROR
            "Dormant Warden handler contains parsing, logging, or runtime state: ${FORBIDDEN_PATTERN}")
    endif()
endforeach()

message(STATUS
    "Dormant Warden opcodes retain an authenticated drain-only client boundary")
