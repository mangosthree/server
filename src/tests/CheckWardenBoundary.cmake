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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#
# World of Warcraft, and all World of Warcraft or Warcraft art, images,
# and lore are copyrighted by Blizzard Entertainment, Inc.

if(NOT DEFINED SOURCE_ROOT)
    get_filename_component(SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(OPCODES "${SOURCE_ROOT}/src/proto/Opcodes.h")
set(OPCODE_TABLE "${SOURCE_ROOT}/src/game/Server/OpcodeTable.cpp")
set(GATEWAY "${SOURCE_ROOT}/src/game/Server/WorldGateway.cpp")
set(GATEWAY_AUTH "${SOURCE_ROOT}/src/game/Server/WorldGatewayAuth.cpp")
set(SESSION_HEADER "${SOURCE_ROOT}/src/game/Server/WorldSession.h")
set(SESSION_SOURCE "${SOURCE_ROOT}/src/game/Server/WorldSession.cpp")
set(HANDLER "${SOURCE_ROOT}/src/game/WorldHandlers/WardenHandler.cpp")
set(CHARACTER "${SOURCE_ROOT}/src/game/WorldHandlers/CharacterHandler.cpp")
set(WORLD "${SOURCE_ROOT}/src/game/WorldHandlers/World.cpp")
set(SESSION_MGR "${SOURCE_ROOT}/src/game/WorldHandlers/WorldSessionMgr.cpp")
set(WORLD_CONFIG "${SOURCE_ROOT}/src/game/WorldHandlers/WorldConfig.cpp")
set(MASTER "${SOURCE_ROOT}/src/mangosd/Master.cpp")
set(CONF "${SOURCE_ROOT}/src/mangosd/mangosd.conf.dist.in")
set(PARAMS "${SOURCE_ROOT}/cmake/MangosParams.cmake")
set(REVISIONS "${SOURCE_ROOT}/src/shared/revision_data.h.in")

foreach(PATH IN ITEMS OPCODES OPCODE_TABLE GATEWAY GATEWAY_AUTH SESSION_HEADER
        SESSION_SOURCE HANDLER CHARACTER WORLD SESSION_MGR)
    if(NOT EXISTS "${${PATH}}")
        message(FATAL_ERROR "Active Warden boundary artifact missing: ${${PATH}}")
    endif()
    file(READ "${${PATH}}" ${PATH}_TEXT)
    string(REPLACE "\r\n" "\n" ${PATH}_TEXT "${${PATH}_TEXT}")
endforeach()

foreach(PATH IN ITEMS WORLD_CONFIG MASTER CONF PARAMS REVISIONS)
    if(NOT EXISTS "${${PATH}}")
        message(FATAL_ERROR "Warden publication artifact missing: ${${PATH}}")
    endif()
    file(READ "${${PATH}}" ${PATH}_TEXT)
    string(REPLACE "\r\n" "\n" ${PATH}_TEXT "${${PATH}_TEXT}")
endforeach()

function(require_count VARIABLE PATTERN EXPECTED DESCRIPTION)
    string(REGEX MATCHALL "${PATTERN}" MATCHES "${${VARIABLE}}")
    list(LENGTH MATCHES COUNT)
    if(NOT COUNT EQUAL EXPECTED)
        message(FATAL_ERROR "${DESCRIPTION}: expected ${EXPECTED}, found ${COUNT}")
    endif()
endfunction()

function(require_text VARIABLE NEEDLE DESCRIPTION)
    string(FIND "${${VARIABLE}}" "${NEEDLE}" POSITION)
    if(POSITION EQUAL -1)
        message(FATAL_ERROR "${DESCRIPTION}")
    endif()
endfunction()

function(require_order VARIABLE FIRST SECOND DESCRIPTION)
    string(FIND "${${VARIABLE}}" "${FIRST}" FIRST_AT)
    string(FIND "${${VARIABLE}}" "${SECOND}" SECOND_AT)
    if(FIRST_AT EQUAL -1 OR SECOND_AT EQUAL -1 OR NOT FIRST_AT LESS SECOND_AT)
        message(FATAL_ERROR "${DESCRIPTION}")
    endif()
endfunction()

# Exact 15595 wire ownership.
require_count(OPCODES_TEXT "CMSG_WARDEN_DATA[ \t]*=[ \t]*0x25A2" 1
    "CMSG_WARDEN_DATA must be exact")
require_count(OPCODES_TEXT "SMSG_WARDEN_DATA[ \t]*=[ \t]*0x31A0" 1
    "SMSG_WARDEN_DATA must be exact")
require_count(OPCODE_TABLE_TEXT "void[ \t]+RegisterWardenOpcodes[ \t]*[(]" 1
    "one named Warden registration helper must own both rows")
require_count(OPCODE_TABLE_TEXT "assertUnclaimed[ \t]*[(][ \t]*CMSG_WARDEN_DATA" 1
    "CMSG must be unclaimed before registration")
require_count(OPCODE_TABLE_TEXT "assertUnclaimed[ \t]*[(][ \t]*SMSG_WARDEN_DATA" 1
    "SMSG must be unclaimed before registration")
require_count(OPCODE_TABLE_TEXT
    "OPCODE[ \t\n]*[(][ \t\n]*CMSG_WARDEN_DATA[ \t\n]*,[ \t\n]*STATUS_AUTHED[ \t\n]*,[ \t\n]*PROCESS_THREADUNSAFE[ \t\n]*,[ \t\n]*&WorldSession::HandleWardenDataOpcode[ \t\n]*[)]" 1
    "CMSG must have one grouped authenticated registration")
require_count(OPCODE_TABLE_TEXT
    "OPCODE[ \t\n]*[(][ \t\n]*SMSG_WARDEN_DATA[ \t\n]*,[ \t\n]*STATUS_NEVER[ \t\n]*,[ \t\n]*PROCESS_INPLACE[ \t\n]*,[ \t\n]*&WorldSession::Handle_ServerSide[ \t\n]*[)]" 1
    "SMSG must have one complete outbound registration")
require_text(OPCODE_TABLE_TEXT "    RegisterWardenOpcodes();"
    "Warden helper must be invoked once")
require_count(OPCODE_TABLE_TEXT "void[ \t]+AssertWardenOpcodes[ \t]*[(]" 1
    "post-registration Warden ownership must be asserted")
require_text(OPCODE_TABLE_TEXT "    AssertWardenOpcodes();"
    "post-registration Warden assertion must run once")

# The grouped handler is only a synchronous immutable-view adapter.
require_count(HANDLER_TEXT "recv_data[.]rfinish[ \t]*[(]" 1
    "Warden handler must finish the packet exactly once")
require_text(HANDLER_TEXT "recv_data.contents() + readPosition"
    "Warden handler must capture the unread start")
require_text(HANDLER_TEXT "recv_data.size() - readPosition"
    "Warden handler must capture the unread length")
require_text(HANDLER_TEXT "m_warden->HandleClientFrame(view)"
    "Warden handler must call the server synchronously")
require_order(HANDLER_TEXT "m_warden->HandleClientFrame(view)"
    "recv_data.rfinish()" "Warden handler must finish after the server returns")
require_order(HANDLER_TEXT "recv_data.rfinish()"
    "FinalizeWardenDisengagement()"
    "Warden handler must finalize only after the server is off-stack")
foreach(FORBIDDEN IN ITEMS "switch[ \t]*\\(" "LoginDatabase" "WorldDatabase"
        "PQuery[ \t]*\\(" "PExecute[ \t]*\\(" "Decrypt" "Encode"
        "Decode" "QueueConfirmation" "EvaluateBatch" "sLog" "DEBUG_LOG")
    if(HANDLER_TEXT MATCHES "${FORBIDDEN}")
        message(FATAL_ERROR "Grouped Warden handler owns forbidden behavior: ${FORBIDDEN}")
    endif()
endforeach()

# Exactly two extension hooks use one shared transport predicate.
set(SESSION_ALL "${SESSION_HEADER_TEXT}\n${SESSION_SOURCE_TEXT}")
require_count(SESSION_ALL "IsWardenTransportOpcode[ \t]*[(]" 3
    "one predicate definition and two extension guards are required")
require_text(SESSION_SOURCE_TEXT
    "!IsWardenTransportOpcode(packet->GetOpcode())"
    "PlayerBots send hook must exclude only Warden transport")
require_text(SESSION_SOURCE_TEXT
    "!IsWardenTransportOpcode(packet->GetOpcode())"
    "Eluna receive hook must exclude only Warden transport")
if(GATEWAY_TEXT MATCHES "IsWardenTransportOpcode")
    message(FATAL_ERROR "packet tracing must not exempt Warden")
endif()
require_text(SESSION_SOURCE_TEXT "CMSG_WARDEN_DATA"
    "recent-logout activity exception must name CMSG Warden")

# Admission keeps the auth key and redirect salt in distinct custody.
require_text(GATEWAY_TEXT "`client_locale`"
    "WorldGateway account projection must append exact locale")
require_text(GATEWAY_TEXT "BuildWardenAdmissionData(request.build"
    "WorldGateway must build authenticated Warden custody")
require_text(GATEWAY_TEXT "row->sessionKey"
    "WorldGateway must source Warden from the authenticated session key")
require_text(GATEWAY_TEXT "row->sessionSalt"
    "WorldGateway must preserve the redirect-HMAC salt")
require_text(GATEWAY_TEXT "std::move(admission)"
    "WorldGateway must move the sole Warden custody owner")
require_count(SESSION_MGR_TEXT "OnAuthenticatedAdmission[ \t]*[(]" 2
    "immediate and queued publication must both consume admission")
require_order(GATEWAY_AUTH_TEXT "GetExactLocaleName(clientLocale)"
    "sessionKey.AsByteArray"
    "exact locale must validate before session-key serialization")
require_text(GATEWAY_TEXT "GetRuntimeSnapshot()"
    "WorldGateway must capture one active runtime generation")
require_text(GATEWAY_TEXT "WardenIncidentStore::Instance().Load"
    "WorldGateway must load history under the captured policy")
require_text(SESSION_SOURCE_TEXT "options.runtimeSnapshot = m_wardenRuntimeSnapshot"
    "queued admission must create from its attach-time generation")

# Bootstrap begins after character enumeration, with login as a safety net.
require_count(CHARACTER_TEXT "StartWardenBootstrap[ \t]*[(]" 2
    "character enumeration and player login must each call the idempotent start")
require_order(CHARACTER_TEXT "SendPacket(&data)" "StartWardenBootstrap()"
    "character enumeration must be published before Warden starts")

# Deadlines charge before ordinary update/reaping and all off-stack calls finish.
require_text(WORLD_TEXT "pSession->UpdateWarden(diff)"
    "world sessions must charge Warden deadlines")
require_order(WORLD_TEXT "pSession->UpdateWarden(diff)"
    "pSession->Update(updater)"
    "Warden must update before ordinary session reaping")
string(FIND "${SESSION_SOURCE_TEXT}"
    "void WorldSession::StartWardenBootstrap()" START_FUNCTION_AT)
if(START_FUNCTION_AT EQUAL -1)
    message(FATAL_ERROR "Warden bootstrap function is missing")
endif()
string(SUBSTRING "${SESSION_SOURCE_TEXT}" ${START_FUNCTION_AT} -1 START_TAIL)
require_order(START_TAIL "m_warden->Start()" "FinalizeWardenDisengagement()"
    "Start must finalize only after returning")

string(FIND "${SESSION_SOURCE_TEXT}"
    "void WorldSession::UpdateWarden(uint32 diffMs)" UPDATE_FUNCTION_AT)
if(UPDATE_FUNCTION_AT EQUAL -1)
    message(FATAL_ERROR "Warden update function is missing")
endif()
string(SUBSTRING "${SESSION_SOURCE_TEXT}" ${UPDATE_FUNCTION_AT} -1 UPDATE_TAIL)
require_text(UPDATE_TAIL "bool const scanEligible ="
    "Warden update must distinguish real-scan eligibility")
require_order(UPDATE_TAIL "m_warden->Update(scanEligible, diffMs)"
    "FinalizeWardenDisengagement()"
    "Update must finalize only after returning")

# Configuration is mandatory, versioned, and atomically published before any
# ordinary cached World setting can change.
require_text(CONF_TEXT "Warden.EnforcementMode       = 0"
    "provisional Warden must ship in observe mode")
require_text(WORLD_CONFIG_TEXT "\"Warden.EnforcementMode\", 0"
    "missing-key Warden fallback must remain observe-only")
require_text(CONF_TEXT "Warden.RequireExactProfile   = 1"
    "exact-profile configuration is missing")
require_text(CONF_TEXT "Warden.RequireCurrentX86Patch = 1"
    "current x86 client patch must be required by default")
require_text(WORLD_CONFIG_TEXT "\"Warden.RequireCurrentX86Patch\", true"
    "missing-key current x86 patch fallback must reject legacy clients")
if(CONF_TEXT MATCHES "Warden[.]Enabled")
    message(FATAL_ERROR "Warden must not have a master disable switch")
endif()
require_text(PARAMS_TEXT "set(MANGOS_WORLD_VER 2026090100)"
    "mangosd configuration version was not advanced")
require_text(REVISIONS_TEXT "#define REALMD_DB_STRUCTURE_NR      \"5\""
    "Realm structure requirement must be 22/05/001")
require_text(REVISIONS_TEXT
    "#define REALMD_DB_UPDATE_DESCRIPT   \"Cata Warden identity\""
    "Realm description requirement is stale")
require_text(REVISIONS_TEXT "#define WORLD_DB_STRUCTURE_NR       \"10\""
    "World structure requirement must be 22/10/003")
require_text(REVISIONS_TEXT "#define WORLD_DB_CONTENT_NR         \"3\""
    "World content requirement must be 22/10/003")
require_text(REVISIONS_TEXT
    "#define WORLD_DB_UPDATE_DESCRIPT    \"Cata_Warden_MPQ_Checks\""
    "World description requirement is stale")
require_text(WORLD_CONFIG_TEXT "bool World::LoadConfigSettings(bool reload)"
    "configuration reload must report rejection")
require_order(WORLD_CONFIG_TEXT "ValidateRuntimeConfiguration"
    "SetPlayerLimit"
    "Warden must validate before ordinary cached settings change")
require_text(WORLD_CONFIG_TEXT "TryReplaceRuntimeConfiguration"
    "reload must atomically replace the runtime generation")
require_text(WORLD_CONFIG_TEXT "ActivateRuntimeConfiguration"
    "startup must atomically activate the runtime generation")
require_text(MASTER_TEXT "if (!sWorld.SetInitialWorldSettings())"
    "Master must abort before listeners when Warden publication fails")

message(STATUS "Active Cata Warden transport/session boundary is intact")
