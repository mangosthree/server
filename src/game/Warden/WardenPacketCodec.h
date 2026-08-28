/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef MANGOS_WARDEN_PACKET_CODEC_H
#define MANGOS_WARDEN_PACKET_CODEC_H

#include "WardenCheckPlan.h"
#include "WardenCryptoContext.h"
#include "WardenModuleCatalog.h"
#include "WardenProtocol.h"

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace warden
{
constexpr std::size_t MaxClientWardenWireSize = 10240;
constexpr std::size_t NormalClientHeaderSize = 4;
constexpr std::size_t ClientWardenLengthSize = 4;
constexpr std::size_t MaxEncryptedClientBody =
    MaxClientWardenWireSize - NormalClientHeaderSize - ClientWardenLengthSize;
constexpr std::size_t CheckResultEnvelopeSize = 7;
constexpr std::size_t MaxDecryptedCheckResultBody =
    MaxEncryptedClientBody - CheckResultEnvelopeSize;

// A build-15595 live exchange proves the outbound uint32 little-endian length
// counts only encrypted-body bytes. Keep its budget independent from input.
constexpr std::size_t MaxServerWardenWireSize = 10240;
constexpr std::size_t NormalServerHeaderSize = 4;
constexpr std::size_t ServerWardenLengthSize = 4;
constexpr std::size_t MaxEncryptedServerBody =
    MaxServerWardenWireSize - NormalServerHeaderSize -
    ServerWardenLengthSize;
constexpr std::size_t Cata15595X86CheckBufferSize = 512;

struct DecodedClientFrame
{
    ByteView encryptedBody;
};

struct EncodedServerFrame
{
    Bytes payload;
};

enum class FrameDecodeStatus : uint8
{
    Ok,
    Empty,
    TruncatedLength,
    LengthMismatch,
    TrailingData,
    BodyTooLarge
};

enum class EncodeStatus : uint8
{
    Ok,
    Empty,
    BodyTooLarge,
    InvalidAbi,
    InvalidProfile,
    InvalidPlan,
    CryptoFailure
};

/** Transactional command-2 decode status; no partial result is published. */
enum class DecodeStatus : uint8
{
    Ok,
    InvalidAbi,
    Empty,
    WrongSize,
    UnsupportedCommand,
    ChecksumMismatch,
    InvalidValue,
    CryptoFailure
};

enum class ModuleDecodeStatus : uint8
{
    Ok,
    InvalidAbi,
    InvalidCommand,
    InvalidLength,
    InvalidChecksum,
    DigestMismatch,
    ModuleReportedFailure,
    InvalidStatus,
    CryptoFailure
};

struct TimingResult
{
    bool stable = false;
    uint32 clientTick = 0;
};

enum class MpqResultStatus : uint8
{
    Success = 0,
    Unavailable = 1
};

struct MpqResult
{
    MpqResultStatus status = MpqResultStatus::Unavailable;
    Digest20 digest{};
};

enum class LuaResultStatus : uint8
{
    Success = 0,
    Unavailable = 1
};

struct LuaResult
{
    LuaResultStatus status = LuaResultStatus::Unavailable;
    std::string text;
};

enum class MemResultStatus : uint8
{
    Success = 0,
    Unavailable = 1
};

struct MemResult
{
    MemResultStatus status = MemResultStatus::Unavailable;
    Bytes actualBytes;
};

using CheckResult =
    std::variant<TimingResult, MpqResult, LuaResult, MemResult>;

struct CheckBatchResult
{
    std::vector<CheckResult> checks;
};

FrameDecodeStatus DecodeClientFrame(
    ByteView payload, DecodedClientFrame& decoded);
EncodeStatus EncodeServerFrame(
    ByteView encryptedBody, EncodedServerFrame& encoded);
EncodeStatus EncodeModuleInitialization(ModuleAbi abi, Bytes& plaintext);
EncodeStatus EncodeModuleHashRequest(
    ModuleProfile const& profile, Bytes& plaintext);
ModuleDecodeStatus DecodeModuleHashResult(
    ModuleProfile const& profile, ByteView plaintext);
EncodeStatus EncodeCompatibilityTimingProbe(ModuleAbi abi,
    WardenCheckXorKey checkXorKey, Bytes& plaintext);
ModuleDecodeStatus DecodeCompatibilityTimingResult(ModuleAbi abi,
    ByteView plaintext, uint32& clientTick);
CheckPlanValidation InspectCheckPlan(ModuleAbi abi, CheckPlan const& plan,
    WardenCheckPlanBudget& budget);
EncodeStatus EncodeCheckRequest(ModuleProfile const& profile,
    WardenCheckXorKey checkXorKey, CheckPlan const& plan, Bytes& encoded);
DecodeStatus DecodeCheckResult(ModuleAbi abi, ByteView plaintext,
    CheckPlan const& plan, CheckBatchResult& result);
/** Erases raw Lua, archive and memory material after semantic classification. */
void CleanseCheckBatchResult(CheckBatchResult& result);
}

#endif
