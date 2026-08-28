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

#include "WardenPacketCodec.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace
{
uint32 ReadLittleEndian32(uint8 const* bytes)
{
    return uint32(bytes[0]) |
        (uint32(bytes[1]) << 8) |
        (uint32(bytes[2]) << 16) |
        (uint32(bytes[3]) << 24);
}

uint16 ReadLittleEndian16(uint8 const* bytes)
{
    return uint16(bytes[0]) | (uint16(bytes[1]) << 8);
}

bool Sha1XorChecksum(warden::ByteView input, uint32& checksum)
{
    warden::Digest20 digest{};
    unsigned int digestSize = 0;
    bool const success = EVP_Digest(input.data(), input.size(), digest.data(),
        &digestSize, EVP_sha1(), nullptr) == 1 &&
        digestSize == digest.size();
    if (!success)
    {
        OPENSSL_cleanse(digest.data(), digest.size());
        checksum = 0;
        return false;
    }

    checksum = 0;
    for (std::size_t offset = 0; offset < digest.size(); offset += 4)
        checksum ^= ReadLittleEndian32(digest.data() + offset);
    OPENSSL_cleanse(digest.data(), digest.size());
    return true;
}

void AppendLittleEndian32(warden::Bytes& bytes, uint32 value)
{
    bytes.push_back(uint8(value));
    bytes.push_back(uint8(value >> 8));
    bytes.push_back(uint8(value >> 16));
    bytes.push_back(uint8(value >> 24));
}

void CleanseDecodedBatch(warden::CheckBatchResult& result)
{
    for (warden::CheckResult& check : result.checks)
    {
        if (warden::MpqResult* mpq =
                std::get_if<warden::MpqResult>(&check))
        {
            OPENSSL_cleanse(mpq->digest.data(), mpq->digest.size());
        }
        else if (warden::LuaResult* lua =
                     std::get_if<warden::LuaResult>(&check))
        {
            if (!lua->text.empty())
                OPENSSL_cleanse(lua->text.data(), lua->text.size());
        }
        else if (warden::MemResult* memory =
                     std::get_if<warden::MemResult>(&check))
        {
            if (!memory->actualBytes.empty())
            {
                OPENSSL_cleanse(memory->actualBytes.data(),
                    memory->actualBytes.size());
            }
        }
    }
    result.checks.clear();
}

class DecodedBatchGuard
{
public:
    explicit DecodedBatchGuard(warden::CheckBatchResult& result)
        : m_result(result)
    {
    }

    ~DecodedBatchGuard()
    {
        if (m_active)
            CleanseDecodedBatch(m_result);
    }

    void Release() { m_active = false; }

private:
    warden::CheckBatchResult& m_result;
    bool m_active = true;
};

struct CheckPlanAnalysis
{
    warden::WardenCheckPlanBudget budget;
    std::vector<std::string> strings;
};

bool IsWireString(std::string const& value)
{
    return !value.empty() &&
        value.size() <= std::numeric_limits<uint8>::max() &&
        value.find('\0') == std::string::npos;
}

bool ModuleIdentifierToString(warden::Bytes const& identifier,
    std::string& value)
{
    value.clear();
    if (identifier.empty() ||
        identifier.size() > std::numeric_limits<uint8>::max() ||
        std::any_of(identifier.begin(), identifier.end(), [](uint8 byte)
        {
            return byte < 0x20 || byte > 0x7E;
        }))
    {
        return false;
    }
    value.assign(identifier.begin(), identifier.end());
    return true;
}

std::size_t PackedValueSize(uint64 value)
{
    std::array<uint8, 8> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = uint8(value >> (index * 8));

    std::size_t zeroCount = 0;
    std::size_t fullCount = 0;
    for (std::size_t index = 1; index < bytes.size(); ++index)
    {
        zeroCount += bytes[index] == 0;
        fullCount += bytes[index] == 0xFF;
    }
    uint8 const defaultValue = zeroCount < fullCount ? 0xFF : 0x00;

    std::size_t size = 2; // Presence mask plus the mandatory low byte.
    for (std::size_t index = 1; index < bytes.size(); ++index)
        size += bytes[index] != defaultValue;
    return size;
}

void AppendPackedValue(warden::Bytes& output, uint64 value)
{
    std::array<uint8, 8> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = uint8(value >> (index * 8));

    std::size_t zeroCount = 0;
    std::size_t fullCount = 0;
    for (std::size_t index = 1; index < bytes.size(); ++index)
    {
        zeroCount += bytes[index] == 0;
        fullCount += bytes[index] == 0xFF;
    }
    uint8 const defaultValue = zeroCount < fullCount ? 0xFF : 0x00;
    uint8 mask = defaultValue == 0xFF ? 1 : 0;
    for (std::size_t index = 1; index < bytes.size(); ++index)
    {
        if (bytes[index] != defaultValue)
            mask |= uint8(1u << index);
    }

    output.push_back(mask);
    output.push_back(bytes[0]);
    for (std::size_t index = 1; index < bytes.size(); ++index)
    {
        if ((mask & uint8(1u << index)) != 0)
            output.push_back(bytes[index]);
    }
}

warden::CheckPlanValidation AnalyzeCheckPlan(warden::ModuleAbi abi,
    warden::CheckPlan const& plan, CheckPlanAnalysis& output)
{
    if (abi != warden::ModuleAbi::Cata15595X86 ||
        plan.profileKey.architecture != warden::WardenArchitecture::X86)
    {
        return warden::CheckPlanValidation::InvalidAbi;
    }
    if (plan.profileKey.build != 15595)
        return warden::CheckPlanValidation::InvalidDefinition;
    if (!plan.requestId)
        return warden::CheckPlanValidation::InvalidRequestId;
    if (plan.checks.empty())
        return warden::CheckPlanValidation::EmptyPlan;
    if (plan.purpose == warden::CheckPlanPurpose::Confirmation &&
        (plan.checks.size() != 1 ||
            !warden::IsConfirmationEligible(plan.checks.front())))
    {
        return warden::CheckPlanValidation::InvalidConfirmation;
    }

    CheckPlanAnalysis candidate;
    candidate.budget.stringTableBytes = 1; // Zero-length table terminator.
    // Command, string-table terminator and encoded module end sentinel.
    candidate.budget.requestBody = 3;
    std::vector<uint32> checkIds;
    std::size_t timingCount = 0;

    auto addRequestBytes = [&candidate](std::size_t bytes)
    {
        if (bytes > warden::Cata15595X86CheckBufferSize -
                candidate.budget.requestBody)
        {
            return false;
        }
        candidate.budget.requestBody += bytes;
        return true;
    };
    auto addResultBytes = [&candidate](std::size_t bytes)
    {
        if (bytes > warden::Cata15595X86CheckBufferSize -
                candidate.budget.maximumResultBody)
        {
            return false;
        }
        candidate.budget.maximumResultBody += bytes;
        return true;
    };
    auto addString = [&candidate, &addRequestBytes](
                         std::string const& value)
    {
        auto const found = std::find(candidate.strings.begin(),
            candidate.strings.end(), value);
        if (found != candidate.strings.end())
            return warden::CheckPlanValidation::Valid;
        if (candidate.strings.size() >=
            std::numeric_limits<uint8>::max())
        {
            return warden::CheckPlanValidation::TooManyStrings;
        }
        std::size_t const bytes = 1 + value.size();
        if (bytes > warden::Cata15595X86CheckBufferSize -
                candidate.budget.stringTableBytes)
        {
            return warden::CheckPlanValidation::StringTableTooLarge;
        }
        if (!addRequestBytes(bytes))
            return warden::CheckPlanValidation::RequestBodyTooLarge;
        candidate.budget.stringTableBytes += bytes;
        candidate.strings.push_back(value);
        candidate.budget.stringCount = candidate.strings.size();
        return warden::CheckPlanValidation::Valid;
    };

    for (warden::WardenCheckDefinition const& definition : plan.checks)
    {
        uint32 const checkId = warden::GetWardenCheckId(definition);
        if (!checkId || !warden::IsLegalWardenEvidenceClass(
                warden::GetWardenCheckType(definition),
                definition.evidenceClass))
        {
            return warden::CheckPlanValidation::InvalidDefinition;
        }
        if (std::find(checkIds.begin(), checkIds.end(), checkId) !=
            checkIds.end())
        {
            return warden::CheckPlanValidation::DuplicateCheckId;
        }
        checkIds.push_back(checkId);

        if (warden::TimingCheckProfile const* timing =
                std::get_if<warden::TimingCheckProfile>(&definition.payload))
        {
            if (!timing->checkId ||
                definition.addressKind != warden::WardenAddressKind::None)
            {
                return warden::CheckPlanValidation::InvalidDefinition;
            }
            if (++timingCount > 1)
                return warden::CheckPlanValidation::DuplicateTiming;
            if (!addRequestBytes(1))
                return warden::CheckPlanValidation::RequestBodyTooLarge;
            if (!addResultBytes(5))
                return warden::CheckPlanValidation::ResultBodyTooLarge;
            continue;
        }

        if (warden::LuaCheckProfile const* lua =
                std::get_if<warden::LuaCheckProfile>(&definition.payload))
        {
            if (definition.addressKind != warden::WardenAddressKind::None ||
                !IsWireString(lua->query) || !IsWireString(lua->expectedText))
            {
                return warden::CheckPlanValidation::InvalidDefinition;
            }
            warden::CheckPlanValidation const stringStatus =
                addString(lua->query);
            if (stringStatus != warden::CheckPlanValidation::Valid)
                return stringStatus;
            if (!addRequestBytes(2))
                return warden::CheckPlanValidation::RequestBodyTooLarge;
            // Successful simple-Lua results carry status, uint8 length and up
            // to 255 bytes, regardless of the expected catalogue text.
            if (!addResultBytes(2 +
                    std::numeric_limits<uint8>::max()))
            {
                return warden::CheckPlanValidation::ResultBodyTooLarge;
            }
            continue;
        }

        if (warden::MpqCheckProfile const* mpq =
                std::get_if<warden::MpqCheckProfile>(&definition.payload))
        {
            if (definition.addressKind != warden::WardenAddressKind::None ||
                !IsWireString(mpq->path))
            {
                return warden::CheckPlanValidation::InvalidDefinition;
            }
            warden::CheckPlanValidation const stringStatus =
                addString(mpq->path);
            if (stringStatus != warden::CheckPlanValidation::Valid)
                return stringStatus;
            if (!addRequestBytes(2))
                return warden::CheckPlanValidation::RequestBodyTooLarge;
            if (!addResultBytes(1 + mpq->expectedSha1.size()))
                return warden::CheckPlanValidation::ResultBodyTooLarge;
            continue;
        }

        warden::MemCheckProfile const* memory =
            std::get_if<warden::MemCheckProfile>(&definition.payload);
        if (!memory || !memory->checkId || !memory->addressOrRva ||
            memory->addressOrRva > std::numeric_limits<uint32>::max() ||
            !memory->length ||
            memory->length > std::numeric_limits<uint8>::max())
        {
            return warden::CheckPlanValidation::InvalidDefinition;
        }
        bool const profileProbe =
            plan.purpose == warden::CheckPlanPurpose::ProfileProbe;
        if ((!profileProbe && memory->expectedBytes.size() != memory->length) ||
            (profileProbe && !memory->expectedBytes.empty()))
        {
            return warden::CheckPlanValidation::InvalidDefinition;
        }

        std::string moduleName;
        if (definition.addressKind ==
            warden::WardenAddressKind::ModuleRelativeRva)
        {
            if (!ModuleIdentifierToString(memory->moduleIdentifier,
                    moduleName))
            {
                return warden::CheckPlanValidation::InvalidDefinition;
            }
            warden::CheckPlanValidation const stringStatus =
                addString(moduleName);
            if (stringStatus != warden::CheckPlanValidation::Valid)
                return stringStatus;
        }
        else if (definition.addressKind !=
                     warden::WardenAddressKind::AbsoluteVa ||
            !memory->moduleIdentifier.empty())
        {
            return warden::CheckPlanValidation::InvalidDefinition;
        }

        std::size_t const requestBytes =
            1 + 1 + PackedValueSize(memory->addressOrRva) + 1;
        if (!addRequestBytes(requestBytes))
            return warden::CheckPlanValidation::RequestBodyTooLarge;
        if (!addResultBytes(1 + memory->length))
            return warden::CheckPlanValidation::ResultBodyTooLarge;
    }

    if (candidate.budget.maximumResultBody >
        warden::MaxDecryptedCheckResultBody)
    {
        return warden::CheckPlanValidation::TransportResultBodyTooLarge;
    }
    output = std::move(candidate);
    return warden::CheckPlanValidation::Valid;
}
}

namespace warden
{
FrameDecodeStatus DecodeClientFrame(
    ByteView payload, DecodedClientFrame& decoded)
{
    decoded.encryptedBody = ByteView();
    if (payload.empty())
        return FrameDecodeStatus::Empty;
    if (payload.size() < ClientWardenLengthSize)
        return FrameDecodeStatus::TruncatedLength;

    uint32 const declaredLength = ReadLittleEndian32(payload.data());
    if (declaredLength == 0)
        return FrameDecodeStatus::Empty;
    if (declaredLength > MaxEncryptedClientBody)
        return FrameDecodeStatus::BodyTooLarge;

    std::size_t const actualLength = payload.size() - ClientWardenLengthSize;
    if (actualLength < declaredLength)
        return FrameDecodeStatus::LengthMismatch;
    if (actualLength > declaredLength)
        return FrameDecodeStatus::TrailingData;

    decoded.encryptedBody = ByteView(
        payload.data() + ClientWardenLengthSize, actualLength);
    return FrameDecodeStatus::Ok;
}

EncodeStatus EncodeServerFrame(
    ByteView encryptedBody, EncodedServerFrame& encoded)
{
    encoded.payload.clear();
    if (encryptedBody.empty())
        return EncodeStatus::Empty;
    if (encryptedBody.size() > MaxEncryptedServerBody)
        return EncodeStatus::BodyTooLarge;

    encoded.payload.reserve(
        ProvisionalServerWardenLengthSize + encryptedBody.size());
    AppendLittleEndian32(encoded.payload, uint32(encryptedBody.size()));
    encoded.payload.insert(encoded.payload.end(), encryptedBody.data(),
        encryptedBody.data() + encryptedBody.size());
    return EncodeStatus::Ok;
}

EncodeStatus EncodeModuleInitialization(ModuleAbi abi, Bytes& plaintext)
{
    plaintext.clear();
    switch (abi)
    {
        case ModuleAbi::Cata15595X86:
        {
            // The exact x86 client safely satisfies the FrameXML and timing
            // callback ABIs. Its filesystem readers do not satisfy either
            // stdcall shape exposed by this signed module, so no archive
            // callback is registered.
            static constexpr std::array<uint8, 34> Initialization = {{
                0x03, 0x0C, 0x00, 0xE9, 0xAB, 0x2B, 0xD1,
                0x04, 0x00, 0x00, 0x10, 0xD3, 0x43, 0x00,
                0x30, 0xC2, 0x43, 0x00, 0x01,
                0x03, 0x08, 0x00, 0x4C, 0xA0, 0x9E, 0x6C,
                0x01, 0x01, 0x00, 0x40, 0x97, 0x47, 0x00, 0x01
            }};
            plaintext.assign(Initialization.begin(), Initialization.end());
            return EncodeStatus::Ok;
        }
        case ModuleAbi::Cata15595X64:
        {
            // The signed-cross-build candidate is constrained to one timing
            // record and never receives Lua or filesystem callbacks.
            static constexpr std::array<uint8, 15> TimingOnlyInitialization =
            {{
                0x03, 0x08, 0x00, 0x99, 0x06, 0x76, 0x7A, 0x01,
                0x01, 0x00, 0x00, 0x25, 0x5B, 0x00, 0x01
            }};
            plaintext.assign(TimingOnlyInitialization.begin(),
                TimingOnlyInitialization.end());
            return EncodeStatus::Ok;
        }
        default:
            return EncodeStatus::InvalidAbi;
    }
}

EncodeStatus EncodeModuleHashRequest(
    ModuleProfile const& profile, Bytes& plaintext)
{
    plaintext.clear();
    plaintext.reserve(1 + profile.rekey.seed.size());
    plaintext.push_back(0x05);
    plaintext.insert(plaintext.end(), profile.rekey.seed.begin(),
        profile.rekey.seed.end());
    return EncodeStatus::Ok;
}

ModuleDecodeStatus DecodeModuleHashResult(
    ModuleProfile const& profile, ByteView plaintext)
{
    if (plaintext.size() != 1 + profile.rekey.expectedResponse.size() ||
        !plaintext.data())
    {
        return ModuleDecodeStatus::InvalidLength;
    }
    if (plaintext.data()[0] != 0x04)
        return ModuleDecodeStatus::InvalidCommand;
    if (CRYPTO_memcmp(plaintext.data() + 1,
            profile.rekey.expectedResponse.data(),
            profile.rekey.expectedResponse.size()) != 0)
    {
        return ModuleDecodeStatus::DigestMismatch;
    }
    return ModuleDecodeStatus::Ok;
}

EncodeStatus EncodeCompatibilityTimingProbe(ModuleAbi abi,
    WardenCheckXorKey checkXorKey, Bytes& plaintext)
{
    plaintext.clear();
    if (abi != ModuleAbi::Cata15595X64)
        return EncodeStatus::InvalidAbi;

    uint8 const key = checkXorKey.Value();
    plaintext = {0x02, 0x00,
        uint8(CataX64CompatibilityTimingCode ^ key), key};
    return EncodeStatus::Ok;
}

ModuleDecodeStatus DecodeCompatibilityTimingResult(ModuleAbi abi,
    ByteView plaintext, uint32& clientTick)
{
    clientTick = 0;
    if (abi != ModuleAbi::Cata15595X64)
        return ModuleDecodeStatus::InvalidAbi;
    if (plaintext.size() != 12 || !plaintext.data())
        return ModuleDecodeStatus::InvalidLength;
    if (plaintext.data()[0] != 0x02)
        return ModuleDecodeStatus::InvalidCommand;
    if (ReadLittleEndian16(plaintext.data() + 1) != 5)
        return ModuleDecodeStatus::InvalidLength;

    uint32 actualChecksum = 0;
    if (!Sha1XorChecksum(ByteView(plaintext.data() + 7, 5), actualChecksum))
        return ModuleDecodeStatus::CryptoFailure;
    if (actualChecksum != ReadLittleEndian32(plaintext.data() + 3))
        return ModuleDecodeStatus::InvalidChecksum;

    uint8 const status = plaintext.data()[7];
    if (status == 0)
        return ModuleDecodeStatus::ModuleReportedFailure;
    if (status != 1)
        return ModuleDecodeStatus::InvalidStatus;

    clientTick = ReadLittleEndian32(plaintext.data() + 8);
    return ModuleDecodeStatus::Ok;
}

CheckPlanValidation InspectCheckPlan(ModuleAbi abi, CheckPlan const& plan,
    WardenCheckPlanBudget& budget)
{
    budget = {};
    CheckPlanAnalysis analysis;
    CheckPlanValidation const status = AnalyzeCheckPlan(abi, plan, analysis);
    if (status == CheckPlanValidation::Valid)
        budget = analysis.budget;
    return status;
}

EncodeStatus EncodeCheckRequest(ModuleProfile const& profile,
    WardenCheckXorKey checkXorKey, CheckPlan const& plan, Bytes& encoded)
{
    if (profile.abi != ModuleAbi::Cata15595X86)
        return EncodeStatus::InvalidAbi;
    if (profile.checkCodes.timing != Cata15595X86TimingCode ||
        profile.checkCodes.lua != Cata15595X86LuaCode ||
        profile.checkCodes.mpq != 0 ||
        profile.checkCodes.memory != Cata15595X86MemoryCode)
    {
        return EncodeStatus::InvalidProfile;
    }

    // This signed module recognizes an MPQ opcode, but build 15595 x86 has no
    // host reader with the required calling convention and return contract.
    // Reject the plan before it can reach the client even if a database row is
    // added accidentally.
    if (std::any_of(plan.checks.begin(), plan.checks.end(),
            [](WardenCheckDefinition const& definition)
            {
                return std::holds_alternative<MpqCheckProfile>(
                    definition.payload);
            }))
    {
        return EncodeStatus::InvalidPlan;
    }

    CheckPlanAnalysis analysis;
    if (AnalyzeCheckPlan(profile.abi, plan, analysis) !=
        CheckPlanValidation::Valid)
    {
        return EncodeStatus::InvalidPlan;
    }

    Bytes candidate;
    candidate.reserve(analysis.budget.requestBody);
    candidate.push_back(0x02);
    for (std::string const& value : analysis.strings)
    {
        candidate.push_back(uint8(value.size()));
        candidate.insert(candidate.end(), value.begin(), value.end());
    }
    candidate.push_back(0);

    auto stringIndex = [&analysis](std::string const& value) -> uint8
    {
        auto const found = std::find(analysis.strings.begin(),
            analysis.strings.end(), value);
        if (found == analysis.strings.end())
            return 0;
        return uint8((found - analysis.strings.begin()) + 1);
    };
    uint8 const xorKey = checkXorKey.Value();
    for (WardenCheckDefinition const& definition : plan.checks)
    {
        if (std::holds_alternative<TimingCheckProfile>(definition.payload))
        {
            candidate.push_back(uint8(profile.checkCodes.timing ^ xorKey));
            continue;
        }
        if (LuaCheckProfile const* lua =
                std::get_if<LuaCheckProfile>(&definition.payload))
        {
            uint8 const index = stringIndex(lua->query);
            if (!index)
                return EncodeStatus::InvalidPlan;
            candidate.push_back(uint8(profile.checkCodes.lua ^ xorKey));
            candidate.push_back(index);
            continue;
        }
        MemCheckProfile const* memory =
            std::get_if<MemCheckProfile>(&definition.payload);
        if (!memory)
            return EncodeStatus::InvalidPlan;
        uint8 moduleIndex = 0;
        if (!memory->moduleIdentifier.empty())
        {
            std::string moduleName;
            if (!ModuleIdentifierToString(memory->moduleIdentifier,
                    moduleName))
            {
                return EncodeStatus::InvalidPlan;
            }
            moduleIndex = stringIndex(moduleName);
            if (!moduleIndex)
                return EncodeStatus::InvalidPlan;
        }
        candidate.push_back(uint8(profile.checkCodes.memory ^ xorKey));
        candidate.push_back(moduleIndex);
        AppendPackedValue(candidate, memory->addressOrRva);
        candidate.push_back(uint8(memory->length));
    }
    // Unlike the stale public enum, this module terminates its decoded check
    // stream with 0xD9. The wire byte remains per-session XOR encoded.
    candidate.push_back(uint8(Cata15595X86EndCode ^ xorKey));
    if (candidate.size() != analysis.budget.requestBody)
        return EncodeStatus::InvalidPlan;

    encoded = std::move(candidate);
    return EncodeStatus::Ok;
}

DecodeStatus DecodeCheckResult(ModuleAbi abi, ByteView plaintext,
    CheckPlan const& plan, CheckBatchResult& result)
{
    if (abi != ModuleAbi::Cata15595X86)
        return DecodeStatus::InvalidAbi;
    if (plaintext.empty())
        return DecodeStatus::Empty;
    if (!plaintext.data() || plaintext.size() < CheckResultEnvelopeSize)
        return DecodeStatus::WrongSize;
    if (plaintext.data()[0] != 0x02)
        return DecodeStatus::UnsupportedCommand;

    uint16 const resultLength = ReadLittleEndian16(plaintext.data() + 1);
    if (resultLength > Cata15595X86CheckBufferSize ||
        plaintext.size() != CheckResultEnvelopeSize + resultLength)
    {
        return DecodeStatus::WrongSize;
    }
    ByteView const resultBody(
        plaintext.data() + CheckResultEnvelopeSize, resultLength);
    uint32 checksum = 0;
    if (!Sha1XorChecksum(resultBody, checksum))
        return DecodeStatus::CryptoFailure;
    if (checksum != ReadLittleEndian32(plaintext.data() + 3))
        return DecodeStatus::ChecksumMismatch;

    CheckPlanAnalysis analysis;
    if (AnalyzeCheckPlan(abi, plan, analysis) != CheckPlanValidation::Valid)
        return DecodeStatus::InvalidValue;

    CheckBatchResult decoded;
    DecodedBatchGuard guard(decoded);
    decoded.checks.reserve(plan.checks.size());
    std::size_t offset = 0;
    auto has = [resultLength, &offset](std::size_t bytes)
    {
        return offset <= resultLength && bytes <= resultLength - offset;
    };

    for (WardenCheckDefinition const& definition : plan.checks)
    {
        if (std::holds_alternative<TimingCheckProfile>(definition.payload))
        {
            if (!has(5))
                return DecodeStatus::WrongSize;
            uint8 const status = resultBody.data()[offset];
            if (status > 1)
                return DecodeStatus::InvalidValue;
            decoded.checks.emplace_back(TimingResult{
                status == 1,
                ReadLittleEndian32(resultBody.data() + offset + 1)});
            offset += 5;
            continue;
        }

        if (std::holds_alternative<LuaCheckProfile>(definition.payload))
        {
            if (!has(1))
                return DecodeStatus::WrongSize;
            uint8 const status = resultBody.data()[offset++];
            if (status > uint8(LuaResultStatus::Unavailable))
                return DecodeStatus::InvalidValue;
            LuaResult lua;
            lua.status = LuaResultStatus(status);
            if (lua.status == LuaResultStatus::Success)
            {
                if (!has(1))
                    return DecodeStatus::WrongSize;
                uint8 const length = resultBody.data()[offset++];
                if (!has(length))
                    return DecodeStatus::WrongSize;
                lua.text.assign(reinterpret_cast<char const*>(
                    resultBody.data() + offset), length);
                offset += length;
            }
            decoded.checks.emplace_back(std::move(lua));
            continue;
        }

        if (std::holds_alternative<MpqCheckProfile>(definition.payload))
        {
            if (!has(1))
                return DecodeStatus::WrongSize;
            uint8 const status = resultBody.data()[offset++];
            if (status > uint8(MpqResultStatus::Unavailable))
                return DecodeStatus::InvalidValue;
            MpqResult mpq;
            mpq.status = MpqResultStatus(status);
            if (mpq.status == MpqResultStatus::Success)
            {
                if (!has(mpq.digest.size()))
                    return DecodeStatus::WrongSize;
                std::copy(resultBody.data() + offset,
                    resultBody.data() + offset + mpq.digest.size(),
                    mpq.digest.begin());
                offset += mpq.digest.size();
            }
            decoded.checks.emplace_back(mpq);
            continue;
        }

        MemCheckProfile const* memory =
            std::get_if<MemCheckProfile>(&definition.payload);
        if (!memory || !has(1))
            return DecodeStatus::WrongSize;
        uint8 const status = resultBody.data()[offset++];
        if (status > uint8(MemResultStatus::Unavailable))
            return DecodeStatus::InvalidValue;
        MemResult memoryResult;
        memoryResult.status = MemResultStatus(status);
        if (memoryResult.status == MemResultStatus::Success)
        {
            if (!has(memory->length))
                return DecodeStatus::WrongSize;
            memoryResult.actualBytes.assign(resultBody.data() + offset,
                resultBody.data() + offset + memory->length);
            offset += memory->length;
        }
        decoded.checks.emplace_back(std::move(memoryResult));
    }

    if (offset != resultLength)
        return DecodeStatus::WrongSize;
    CleanseDecodedBatch(result);
    result = std::move(decoded);
    guard.Release();
    return DecodeStatus::Ok;
}

void CleanseCheckBatchResult(CheckBatchResult& result)
{
    CleanseDecodedBatch(result);
}
}
