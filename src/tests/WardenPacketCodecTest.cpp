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

#include "TestHarness.h"

#include "WardenCheckPlan.h"
#include "WardenCryptoContext.h"
#include "WardenModuleCatalog.h"
#include "WardenPacketCodec.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <variant>

namespace
{
warden::Bytes ClientFrame(uint32 declaredLength, std::size_t actualLength)
{
    warden::Bytes payload(4 + actualLength, uint8(0xA5));
    payload[0] = uint8(declaredLength);
    payload[1] = uint8(declaredLength >> 8);
    payload[2] = uint8(declaredLength >> 16);
    payload[3] = uint8(declaredLength >> 24);
    return payload;
}

void CheckRejectedClientFrame(warden::Bytes const& payload,
    warden::FrameDecodeStatus expected)
{
    warden::DecodedClientFrame decoded;
    decoded.encryptedBody = warden::ByteView(
        reinterpret_cast<uint8 const*>("sentinel"), 8);
    CHECK_EQ(int(warden::DecodeClientFrame(warden::ByteView(payload), decoded)),
        int(expected));
    CHECK(decoded.encryptedBody.empty());
}

warden::CheckPlan FourFamilyX86Plan()
{
    warden::CheckPlan plan;
    plan.requestId = 7;
    plan.purpose = warden::CheckPlanPurpose::Initial;
    plan.profileKey = {15595, warden::WardenArchitecture::X86,
        {{'e', 'n', 'U', 'S'}}, warden::ClientVariant::Stock};

    warden::WardenCheckDefinition timing;
    timing.sortOrder = 10;
    timing.evidenceClass = warden::WardenEvidenceClass::ProtocolHealth;
    timing.phaseMask = warden::PhaseInitial;
    timing.addressKind = warden::WardenAddressKind::None;
    timing.payload = warden::TimingCheckProfile{1};
    plan.checks.push_back(timing);

    warden::WardenCheckDefinition lua;
    lua.sortOrder = 20;
    lua.evidenceClass = warden::WardenEvidenceClass::Corroboration;
    lua.phaseMask = warden::PhaseInitial;
    lua.addressKind = warden::WardenAddressKind::None;
    lua.payload = warden::LuaCheckProfile{2, "warden_test", "ok"};
    plan.checks.push_back(lua);

    warden::MpqCheckProfile mpqProfile;
    mpqProfile.checkId = 3;
    mpqProfile.path = "DBFilesClient\\Item.db2";
    for (std::size_t index = 0; index < mpqProfile.expectedSha1.size(); ++index)
        mpqProfile.expectedSha1[index] = uint8(index);
    warden::WardenCheckDefinition mpq;
    mpq.sortOrder = 30;
    mpq.evidenceClass = warden::WardenEvidenceClass::IntegrityInvariant;
    mpq.phaseMask = warden::PhaseInitial;
    mpq.addressKind = warden::WardenAddressKind::None;
    mpq.payload = mpqProfile;
    plan.checks.push_back(mpq);

    warden::WardenCheckDefinition memory;
    memory.sortOrder = 40;
    memory.evidenceClass = warden::WardenEvidenceClass::Corroboration;
    memory.phaseMask = warden::PhaseInitial;
    memory.addressKind = warden::WardenAddressKind::ModuleRelativeRva;
    memory.payload = warden::MemCheckProfile{4,
        {'W', 'o', 'w', '.', 'e', 'x', 'e'}, 0x00007F7A, 5,
        {0xE8, 0xB1, 0xED, 0xFF, 0xFF}};
    plan.checks.push_back(memory);
    return plan;
}

warden::CheckPlan ThreeFamilyX64Plan()
{
    warden::CheckPlan plan;
    plan.requestId = 8;
    plan.purpose = warden::CheckPlanPurpose::Initial;
    plan.profileKey = {15595, warden::WardenArchitecture::X64,
        {{'e', 'n', 'U', 'S'}}, warden::ClientVariant::Stock};

    warden::WardenCheckDefinition timing;
    timing.sortOrder = 10;
    timing.evidenceClass = warden::WardenEvidenceClass::ProtocolHealth;
    timing.phaseMask = warden::PhaseInitial;
    timing.addressKind = warden::WardenAddressKind::None;
    timing.payload = warden::TimingCheckProfile{1};
    plan.checks.push_back(timing);

    warden::WardenCheckDefinition lua;
    lua.sortOrder = 20;
    lua.evidenceClass = warden::WardenEvidenceClass::Corroboration;
    lua.phaseMask = warden::PhaseInitial;
    lua.addressKind = warden::WardenAddressKind::None;
    lua.payload = warden::LuaCheckProfile{2, "OKAY", "Okay"};
    plan.checks.push_back(lua);

    warden::WardenCheckDefinition memory;
    memory.sortOrder = 30;
    memory.evidenceClass = warden::WardenEvidenceClass::IntegrityInvariant;
    memory.phaseMask = warden::PhaseInitial;
    memory.addressKind = warden::WardenAddressKind::ModuleRelativeRva;
    memory.payload = warden::MemCheckProfile{3,
        {'W', 'o', 'w', '-', '6', '4', '.', 'e', 'x', 'e'},
        0x00566C13, 16,
        {0x48, 0x83, 0xC9, 0xFF, 0x33, 0xC0, 0x48, 0x8B,
            0xFD, 0xBA, 0xF0, 0xD8, 0xFF, 0xFF, 0xF2, 0xAE}};
    plan.checks.push_back(memory);
    return plan;
}

std::optional<warden::WardenCheckXorKey> X86CheckXorKey()
{
    warden::WardenCryptoContext crypto;
    if (!crypto.Initialize(warden::SessionKey{}))
        return std::nullopt;
    warden::Key16 const clientToServer = {{
        0x8A, 0xB0, 0x72, 0x13, 0xFC, 0xFF, 0x7B, 0xAC,
        0xB7, 0x7B, 0x48, 0x04, 0xD2, 0x39, 0x44, 0x5C}};
    warden::Key16 const serverToClient = {{
        0x6A, 0xEA, 0x6E, 0x52, 0x47, 0x48, 0xF2, 0x2D,
        0x12, 0x2B, 0x27, 0xD9, 0x66, 0x22, 0xD7, 0x65}};
    return crypto.InstallModuleDirectionalKeys(
        clientToServer, serverToClient);
}

std::optional<warden::WardenCheckXorKey> X64CheckXorKey()
{
    warden::WardenCryptoContext crypto;
    if (!crypto.Initialize(warden::SessionKey{}))
        return std::nullopt;
    warden::Key16 const clientToServer = {{
        0x55, 0x80, 0x17, 0xAA, 0xED, 0x7F, 0xFF, 0xAB,
        0x27, 0x3C, 0xB0, 0x0A, 0xBF, 0x51, 0x77, 0x95}};
    warden::Key16 const serverToClient = {{
        0x1B, 0x12, 0xC1, 0xEA, 0xB4, 0x7A, 0x79, 0xA3,
        0x2B, 0x3F, 0x8F, 0x7B, 0x3C, 0x98, 0x59, 0x12}};
    return crypto.InstallModuleDirectionalKeys(
        clientToServer, serverToClient);
}

warden::ModuleProfile X64FullProfile()
{
    warden::ModuleProfile profile;
    profile.abi = warden::ModuleAbi::Cata15595X64;
    profile.checkCodes = {warden::Cata15595X64TimingCode,
        warden::Cata15595X64LuaCode, 0,
        warden::Cata15595X64MemoryCode};
    return profile;
}

warden::Bytes ValidFourFamilyResult()
{
    return {
        0x02, 0x24, 0x00, 0xD7, 0x64, 0xB4, 0xF7,
        0x01, 0x78, 0x56, 0x34, 0x12,
        0x00, 0x02, 0x6F, 0x6B,
        0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
        0x0F, 0x10, 0x11, 0x12, 0x13,
        0x00, 0xE8, 0xB1, 0xED, 0xFF, 0xFF};
}

warden::Bytes ValidThreeFamilyX64Result()
{
    return {
        0x02, 0x1C, 0x00, 0x00, 0xE6, 0x16, 0x3B,
        0x01, 0x78, 0x56, 0x34, 0x12,
        0x00, 0x04, 0x4F, 0x6B, 0x61, 0x79,
        0x00, 0x48, 0x83, 0xC9, 0xFF, 0x33, 0xC0, 0x48,
        0x8B, 0xFD, 0xBA, 0xF0, 0xD8, 0xFF, 0xFF, 0xF2, 0xAE};
}

bool RewriteCheckResultEnvelope(warden::Bytes& plaintext)
{
    if (plaintext.size() < warden::CheckResultEnvelopeSize)
        return false;
    std::size_t const bodySize =
        plaintext.size() - warden::CheckResultEnvelopeSize;
    if (bodySize > 0xFFFF)
        return false;
    plaintext[1] = uint8(bodySize);
    plaintext[2] = uint8(bodySize >> 8);

    warden::Digest20 digest{};
    unsigned int digestSize = 0;
    if (EVP_Digest(plaintext.data() + warden::CheckResultEnvelopeSize,
            bodySize, digest.data(), &digestSize, EVP_sha1(), nullptr) != 1 ||
        digestSize != digest.size())
    {
        return false;
    }

    uint32 checksum = 0;
    for (std::size_t offset = 0; offset < digest.size(); offset += 4)
    {
        checksum ^= uint32(digest[offset]) |
            (uint32(digest[offset + 1]) << 8) |
            (uint32(digest[offset + 2]) << 16) |
            (uint32(digest[offset + 3]) << 24);
    }
    plaintext[3] = uint8(checksum);
    plaintext[4] = uint8(checksum >> 8);
    plaintext[5] = uint8(checksum >> 16);
    plaintext[6] = uint8(checksum >> 24);
    return true;
}

void CheckRejectedCheckResult(warden::Bytes const& plaintext,
    warden::DecodeStatus expected,
    warden::ModuleAbi abi = warden::ModuleAbi::Cata15595X86)
{
    warden::CheckBatchResult result;
    result.checks.emplace_back(warden::TimingResult{true, 0xDEADBEEF});
    CHECK(warden::DecodeCheckResult(abi, warden::ByteView(plaintext),
        FourFamilyX86Plan(), result) == expected);
    REQUIRE(result.checks.size() == 1u);
    warden::TimingResult const* sentinel =
        std::get_if<warden::TimingResult>(&result.checks.front());
    REQUIRE(sentinel != nullptr);
    CHECK(sentinel->stable);
    CHECK_EQ(sentinel->clientTick, uint32(0xDEADBEEF));
}
}

static_assert(warden::MaxClientWardenWireSize == 10240);
static_assert(warden::NormalClientHeaderSize == 4);
static_assert(warden::ClientWardenLengthSize == 4);
static_assert(warden::MaxEncryptedClientBody == 10232);
static_assert(warden::MaxDecryptedCheckResultBody == 10225);
static_assert(warden::MaxServerWardenWireSize == 10240);
static_assert(warden::NormalServerHeaderSize == 4);
static_assert(warden::ServerWardenLengthSize == 4);
static_assert(warden::MaxEncryptedServerBody == 10232);
static_assert(warden::CheckResultEnvelopeSize == 7);

TEST(WardenPacketCodec_client_frame_decodes_exact_little_endian_length)
{
    warden::Bytes const payload = {3, 0, 0, 0, 0x11, 0x22, 0x33};
    warden::DecodedClientFrame decoded;
    CHECK_EQ(int(warden::DecodeClientFrame(warden::ByteView(payload), decoded)),
        int(warden::FrameDecodeStatus::Ok));
    CHECK_EQ(decoded.encryptedBody.size(), std::size_t(3));
    CHECK_HEX(decoded.encryptedBody.data(), decoded.encryptedBody.size(),
        "112233");
}

TEST(WardenPacketCodec_client_frame_accepts_exact_maximum_body)
{
    warden::Bytes const payload = ClientFrame(
        uint32(warden::MaxEncryptedClientBody),
        warden::MaxEncryptedClientBody);
    warden::DecodedClientFrame decoded;
    CHECK_EQ(int(warden::DecodeClientFrame(warden::ByteView(payload), decoded)),
        int(warden::FrameDecodeStatus::Ok));
    CHECK_EQ(decoded.encryptedBody.size(), warden::MaxEncryptedClientBody);
}

TEST(WardenPacketCodec_client_frame_rejects_empty_and_truncated_length)
{
    CheckRejectedClientFrame({}, warden::FrameDecodeStatus::Empty);
    CheckRejectedClientFrame({1}, warden::FrameDecodeStatus::TruncatedLength);
    CheckRejectedClientFrame({1, 0, 0},
        warden::FrameDecodeStatus::TruncatedLength);
    CheckRejectedClientFrame({0, 0, 0, 0},
        warden::FrameDecodeStatus::Empty);
}

TEST(WardenPacketCodec_client_frame_rejects_short_body_and_trailing_data)
{
    CheckRejectedClientFrame(ClientFrame(4, 3),
        warden::FrameDecodeStatus::LengthMismatch);
    CheckRejectedClientFrame(ClientFrame(3, 4),
        warden::FrameDecodeStatus::TrailingData);
}

TEST(WardenPacketCodec_client_frame_rejects_oversized_body_before_slicing)
{
    CheckRejectedClientFrame(ClientFrame(
        uint32(warden::MaxEncryptedClientBody + 1), 0),
        warden::FrameDecodeStatus::BodyTooLarge);
    CheckRejectedClientFrame(ClientFrame(0xFFFFFFFFu, 0),
        warden::FrameDecodeStatus::BodyTooLarge);
}

TEST(WardenPacketCodec_server_frame_encodes_live_proven_little_endian_length)
{
    warden::Bytes const body = {0x11, 0x22, 0x33};
    warden::EncodedServerFrame encoded;
    CHECK_EQ(int(warden::EncodeServerFrame(warden::ByteView(body), encoded)),
        int(warden::EncodeStatus::Ok));
    CHECK_HEX(encoded.payload.data(), encoded.payload.size(),
        "03000000112233");
}

TEST(WardenPacketCodec_server_frame_accepts_exact_maximum_body)
{
    warden::Bytes const body(warden::MaxEncryptedServerBody, uint8(0x5A));
    warden::EncodedServerFrame encoded;
    CHECK_EQ(int(warden::EncodeServerFrame(warden::ByteView(body), encoded)),
        int(warden::EncodeStatus::Ok));
    CHECK_EQ(encoded.payload.size(),
        warden::ServerWardenLengthSize + body.size());
    CHECK_HEX(encoded.payload.data(), 4, "f8270000");
    CHECK(std::equal(body.begin(), body.end(), encoded.payload.begin() + 4));
}

TEST(WardenPacketCodec_server_frame_rejects_empty_and_oversized_bodies)
{
    warden::EncodedServerFrame encoded;
    encoded.payload = {1, 2, 3};
    CHECK_EQ(int(warden::EncodeServerFrame(warden::ByteView(), encoded)),
        int(warden::EncodeStatus::Empty));
    CHECK(encoded.payload.empty());

    warden::Bytes const tooLarge(
        warden::MaxEncryptedServerBody + 1, uint8(0));
    encoded.payload = {1, 2, 3};
    CHECK_EQ(int(warden::EncodeServerFrame(warden::ByteView(tooLarge), encoded)),
        int(warden::EncodeStatus::BodyTooLarge));
    CHECK(encoded.payload.empty());
}

TEST(WardenPacketCodec_encodes_exact_build_15595_x64_initialization)
{
    warden::Bytes plaintext = {0xA5};
    CHECK_EQ(int(warden::EncodeModuleInitialization(
                 warden::ModuleAbi::Cata15595X64, plaintext)),
        int(warden::EncodeStatus::Ok));
    CHECK_HEX(plaintext.data(), plaintext.size(),
        "030c00f717b38304000010815600c06b560001"
        "0308009906767a01010000255b0001");
}

TEST(WardenPacketCodec_encodes_exact_build_15595_x86_initialization)
{
    warden::Bytes plaintext = {0xA5};
    CHECK_EQ(int(warden::EncodeModuleInitialization(
                 warden::ModuleAbi::Cata15595X86, plaintext)),
        int(warden::EncodeStatus::Ok));
    CHECK_HEX(plaintext.data(), plaintext.size(),
        "030c00e9ab2bd104000010d3430030c2430001"
        "0308004ca09e6c0101004097470001");
}

TEST(WardenPacketCodec_encodes_pinned_x64_timing_request_from_typed_key)
{
    warden::WardenCryptoContext crypto;
    REQUIRE(crypto.Initialize(warden::SessionKey{}));
    warden::Key16 const clientToServer = {{
        0x55, 0x80, 0x17, 0xAA, 0xED, 0x7F, 0xFF, 0xAB,
        0x27, 0x3C, 0xB0, 0x0A, 0xBF, 0x51, 0x77, 0x95}};
    warden::Key16 const serverToClient = {{
        0x1B, 0x12, 0xC1, 0xEA, 0xB4, 0x7A, 0x79, 0xA3,
        0x2B, 0x3F, 0x8F, 0x7B, 0x3C, 0x98, 0x59, 0x12}};
    std::optional<warden::WardenCheckXorKey> const checkXorKey =
        crypto.InstallModuleDirectionalKeys(clientToServer, serverToClient);
    REQUIRE(checkXorKey.has_value());

    warden::Bytes plaintext = {0xA5};
    CHECK_EQ(int(warden::EncodeCompatibilityTimingProbe(
                 warden::ModuleAbi::Cata15595X64, *checkXorKey, plaintext)),
        int(warden::EncodeStatus::Ok));
    CHECK_HEX(plaintext.data(), plaintext.size(), "0200bf55");
}

TEST(WardenPacketCodec_decodes_binary_proven_x64_timing_result)
{
    warden::Bytes const plaintext = {
        0x02, 0x05, 0x00, 0xA7, 0xD4, 0x3E,
        0x25, 0x01, 0x78, 0x56, 0x34, 0x12};
    uint32 clientTick = 0;
    CHECK_EQ(int(warden::DecodeCompatibilityTimingResult(
                 warden::ModuleAbi::Cata15595X64,
                 warden::ByteView(plaintext), clientTick)),
        int(warden::ModuleDecodeStatus::Ok));
    CHECK_EQ(clientTick, uint32(0x12345678));
}

TEST(WardenPacketCodec_accepts_unstable_timing_and_rejects_bad_status)
{
    warden::Bytes const unstable = {
        0x02, 0x05, 0x00, 0xA4, 0x90, 0xE0,
        0x96, 0x00, 0x78, 0x56, 0x34, 0x12};
    uint32 clientTick = 0xFFFFFFFF;
    CHECK_EQ(int(warden::DecodeCompatibilityTimingResult(
                 warden::ModuleAbi::Cata15595X64,
                 warden::ByteView(unstable), clientTick)),
        int(warden::ModuleDecodeStatus::Ok));
    CHECK_EQ(clientTick, uint32(0x12345678));

    warden::Bytes const invalidStatus = {
        0x02, 0x05, 0x00, 0x24, 0x36, 0x22,
        0x04, 0x02, 0x78, 0x56, 0x34, 0x12};
    clientTick = 0xFFFFFFFF;
    CHECK_EQ(int(warden::DecodeCompatibilityTimingResult(
                 warden::ModuleAbi::Cata15595X64,
                 warden::ByteView(invalidStatus), clientTick)),
        int(warden::ModuleDecodeStatus::InvalidStatus));
    CHECK_EQ(clientTick, uint32(0));
}

TEST(WardenPacketCodec_rejects_mutated_timing_result_envelope_and_checksum)
{
    warden::Bytes mutated = {
        0x02, 0x05, 0x00, 0xA7, 0xD4, 0x3E,
        0x25, 0x01, 0x78, 0x56, 0x34, 0x12};
    uint32 clientTick = 0xFFFFFFFF;

    mutated[0] = 0x03;
    CHECK_EQ(int(warden::DecodeCompatibilityTimingResult(
                 warden::ModuleAbi::Cata15595X64,
                 warden::ByteView(mutated), clientTick)),
        int(warden::ModuleDecodeStatus::InvalidCommand));
    CHECK_EQ(clientTick, uint32(0));

    mutated[0] = 0x02;
    mutated[1] = 0x04;
    clientTick = 0xFFFFFFFF;
    CHECK_EQ(int(warden::DecodeCompatibilityTimingResult(
                 warden::ModuleAbi::Cata15595X64,
                 warden::ByteView(mutated), clientTick)),
        int(warden::ModuleDecodeStatus::InvalidLength));
    CHECK_EQ(clientTick, uint32(0));

    mutated[1] = 0x05;
    mutated[3] ^= 0x01;
    clientTick = 0xFFFFFFFF;
    CHECK_EQ(int(warden::DecodeCompatibilityTimingResult(
                 warden::ModuleAbi::Cata15595X64,
                 warden::ByteView(mutated), clientTick)),
        int(warden::ModuleDecodeStatus::InvalidChecksum));
    CHECK_EQ(clientTick, uint32(0));
}

TEST(WardenPacketCodec_rejects_timing_result_for_non_candidate_abi)
{
    warden::Bytes const plaintext = {
        0x02, 0x05, 0x00, 0xA7, 0xD4, 0x3E,
        0x25, 0x01, 0x78, 0x56, 0x34, 0x12};
    uint32 clientTick = 0xFFFFFFFF;
    CHECK_EQ(int(warden::DecodeCompatibilityTimingResult(
                 warden::ModuleAbi::Cata15595X86,
                 warden::ByteView(plaintext), clientTick)),
        int(warden::ModuleDecodeStatus::InvalidAbi));
    CHECK_EQ(clientTick, uint32(0));
}

TEST(WardenPacketCodec_encodes_pinned_x64_module_hash_request)
{
    warden::ModuleProfile profile;
    profile.rekey.seed = {{
        0x8D, 0xB6, 0xE0, 0xC5, 0x86, 0x5A, 0x1F, 0xDB,
        0x81, 0x0F, 0x26, 0xDB, 0x77, 0x3F, 0x68, 0x1F}};

    warden::Bytes plaintext = {0xA5};
    CHECK_EQ(int(warden::EncodeModuleHashRequest(profile, plaintext)),
        int(warden::EncodeStatus::Ok));
    CHECK_HEX(plaintext.data(), plaintext.size(),
        "058db6e0c5865a1fdb810f26db773f681f");
}

TEST(WardenPacketCodec_accepts_only_pinned_x64_module_hash_response)
{
    warden::ModuleProfile profile;
    profile.rekey.expectedResponse = {{
        0x57, 0x79, 0x0E, 0x89, 0x1C, 0x05, 0xE7, 0xCE,
        0xB3, 0x4E, 0x67, 0x54, 0xDA, 0xF3, 0x9E, 0x81,
        0x97, 0xFF, 0x5C, 0xEC}};
    warden::Bytes response = {0x04};
    response.insert(response.end(), profile.rekey.expectedResponse.begin(),
        profile.rekey.expectedResponse.end());

    CHECK_EQ(int(warden::DecodeModuleHashResult(
                 profile, warden::ByteView(response))),
        int(warden::ModuleDecodeStatus::Ok));

    response.back() ^= 0x01;
    CHECK_EQ(int(warden::DecodeModuleHashResult(
                 profile, warden::ByteView(response))),
        int(warden::ModuleDecodeStatus::DigestMismatch));
}

TEST(WardenPacketCodec_x86_rejects_mpq_without_a_compatible_client_callback)
{
    warden::CheckPlan const plan = FourFamilyX86Plan();
    warden::WardenCheckPlanBudget budget;
    CHECK(warden::InspectCheckPlan(warden::ModuleAbi::Cata15595X86,
        plan, budget) == warden::CheckPlanValidation::Valid);
    CHECK_EQ(budget.stringCount, std::size_t(3));
    CHECK_EQ(budget.stringTableBytes, std::size_t(44));
    CHECK_EQ(budget.requestBody, std::size_t(57));
    CHECK_EQ(budget.maximumResultBody, std::size_t(289));

    std::optional<warden::WardenCheckXorKey> const key = X86CheckXorKey();
    REQUIRE(key.has_value());
    warden::ModuleProfile profile;
    profile.abi = warden::ModuleAbi::Cata15595X86;
    profile.checkCodes = {warden::Cata15595X86TimingCode,
        warden::Cata15595X86LuaCode, 0,
        warden::Cata15595X86MemoryCode};
    warden::Bytes encoded = {0xA5};
    CHECK(warden::EncodeCheckRequest(profile, *key, plan, encoded) ==
        warden::EncodeStatus::InvalidPlan);
    CHECK_HEX(encoded.data(), encoded.size(), "a5");
}

TEST(WardenPacketCodec_x86_encodes_supported_timing_lua_and_memory_plan)
{
    warden::CheckPlan plan = FourFamilyX86Plan();
    plan.checks.erase(plan.checks.begin() + 2);

    std::optional<warden::WardenCheckXorKey> const key = X86CheckXorKey();
    REQUIRE(key.has_value());
    warden::ModuleProfile profile;
    profile.abi = warden::ModuleAbi::Cata15595X86;
    profile.checkCodes = {warden::Cata15595X86TimingCode,
        warden::Cata15595X86LuaCode, 0,
        warden::Cata15595X86MemoryCode};
    warden::Bytes encoded;
    CHECK(warden::EncodeCheckRequest(profile, *key, plan, encoded) ==
        warden::EncodeStatus::Ok);
    CHECK_HEX(encoded.data(), encoded.size(),
        "020b77617264656e5f7465737407576f772e65786500"
        "5acb01be02027a7f0553");
}

TEST(WardenPacketCodec_x64_encodes_pinned_timing_lua_and_memory_plan)
{
    std::optional<warden::WardenCheckXorKey> const key =
        X64CheckXorKey();
    REQUIRE(key.has_value());
    CHECK_EQ(key->Value(), uint8(0x55));

    warden::Bytes encoded;
    CHECK(warden::EncodeCheckRequest(X64FullProfile(), *key,
        ThreeFamilyX64Plan(), encoded) == warden::EncodeStatus::Ok);
    CHECK_HEX(encoded.data(), encoded.size(),
        "02044f4b41590a576f772d36342e65786500"
        "bf0401630206136c561055");
}

TEST(WardenPacketCodec_x64_decodes_pinned_timing_lua_and_memory_result)
{
    warden::Bytes const plaintext = ValidThreeFamilyX64Result();
    REQUIRE(plaintext.size() == std::size_t(35));

    warden::CheckBatchResult decoded;
    CHECK(warden::DecodeCheckResult(warden::ModuleAbi::Cata15595X64,
        warden::ByteView(plaintext), ThreeFamilyX64Plan(), decoded) ==
        warden::DecodeStatus::Ok);
    REQUIRE(decoded.checks.size() == 3u);

    warden::TimingResult const* timing =
        std::get_if<warden::TimingResult>(&decoded.checks[0]);
    REQUIRE(timing != nullptr);
    CHECK(timing->stable);
    CHECK_EQ(timing->clientTick, uint32(0x12345678));

    warden::LuaResult const* lua =
        std::get_if<warden::LuaResult>(&decoded.checks[1]);
    REQUIRE(lua != nullptr);
    CHECK(lua->status == warden::LuaResultStatus::Success);
    CHECK(lua->text == "Okay");

    warden::MemResult const* memory =
        std::get_if<warden::MemResult>(&decoded.checks[2]);
    REQUIRE(memory != nullptr);
    CHECK(memory->status == warden::MemResultStatus::Success);
    CHECK(memory->actualBytes == warden::Bytes({
        0x48, 0x83, 0xC9, 0xFF, 0x33, 0xC0, 0x48, 0x8B,
        0xFD, 0xBA, 0xF0, 0xD8, 0xFF, 0xFF, 0xF2, 0xAE}));
}

TEST(WardenPacketCodec_x64_treats_timing_status_zero_as_unstable_health)
{
    warden::Bytes plaintext = ValidThreeFamilyX64Result();
    plaintext[warden::CheckResultEnvelopeSize] = 0;
    REQUIRE(RewriteCheckResultEnvelope(plaintext));

    warden::CheckBatchResult decoded;
    CHECK(warden::DecodeCheckResult(warden::ModuleAbi::Cata15595X64,
        warden::ByteView(plaintext), ThreeFamilyX64Plan(), decoded) ==
        warden::DecodeStatus::Ok);
    REQUIRE(decoded.checks.size() == 3u);
    warden::TimingResult const* timing =
        std::get_if<warden::TimingResult>(&decoded.checks[0]);
    REQUIRE(timing != nullptr);
    CHECK(!timing->stable);
    CHECK_EQ(timing->clientTick, uint32(0x12345678));
}

TEST(WardenPacketCodec_x64_preserves_packed_canonical_absolute_addresses)
{
    warden::CheckPlan plan = ThreeFamilyX64Plan();
    plan.checks.erase(plan.checks.begin(), plan.checks.begin() + 2);
    warden::WardenCheckDefinition& definition = plan.checks.front();
    definition.addressKind = warden::WardenAddressKind::AbsoluteVa;
    warden::MemCheckProfile& memory =
        std::get<warden::MemCheckProfile>(definition.payload);
    memory.moduleIdentifier.clear();
    memory.addressOrRva = uint64(0x00007FFFFFFF0000);
    memory.length = 4;
    memory.expectedBytes = {0x48, 0x83, 0xC9, 0xFF};

    warden::WardenCheckPlanBudget budget;
    CHECK(warden::InspectCheckPlan(warden::ModuleAbi::Cata15595X64,
        plan, budget) == warden::CheckPlanValidation::Valid);
    CHECK_EQ(budget.requestBody, std::size_t(12));

    std::optional<warden::WardenCheckXorKey> const key =
        X64CheckXorKey();
    REQUIRE(key.has_value());
    warden::Bytes encoded;
    CHECK(warden::EncodeCheckRequest(X64FullProfile(), *key, plan, encoded) ==
        warden::EncodeStatus::Ok);
    CHECK_HEX(encoded.data(), encoded.size(),
        "020063003c00ffffff7f0455");

    warden::Bytes response = {
        0x02, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x48, 0x83, 0xC9, 0xFF};
    REQUIRE(RewriteCheckResultEnvelope(response));
    warden::CheckBatchResult decoded;
    CHECK(warden::DecodeCheckResult(warden::ModuleAbi::Cata15595X64,
        warden::ByteView(response), plan, decoded) ==
        warden::DecodeStatus::Ok);
    REQUIRE(decoded.checks.size() == 1u);
    warden::MemResult const* result =
        std::get_if<warden::MemResult>(&decoded.checks.front());
    REQUIRE(result != nullptr);
    CHECK(result->status == warden::MemResultStatus::Success);
    CHECK(result->actualBytes ==
        warden::Bytes({0x48, 0x83, 0xC9, 0xFF}));
}

TEST(WardenPacketCodec_keeps_rvas_and_x86_absolute_addresses_32_bit)
{
    warden::CheckPlan x64 = ThreeFamilyX64Plan();
    x64.checks.erase(x64.checks.begin(), x64.checks.begin() + 2);
    std::get<warden::MemCheckProfile>(x64.checks.front().payload).
        addressOrRva = uint64(std::numeric_limits<uint32>::max()) + 1;
    warden::WardenCheckPlanBudget budget;
    CHECK(warden::InspectCheckPlan(warden::ModuleAbi::Cata15595X64,
        x64, budget) == warden::CheckPlanValidation::InvalidDefinition);

    warden::CheckPlan x86 = FourFamilyX86Plan();
    x86.checks.erase(x86.checks.begin(), x86.checks.begin() + 3);
    x86.checks.front().addressKind = warden::WardenAddressKind::AbsoluteVa;
    warden::MemCheckProfile& x86Memory =
        std::get<warden::MemCheckProfile>(x86.checks.front().payload);
    x86Memory.moduleIdentifier.clear();
    x86Memory.addressOrRva =
        uint64(std::numeric_limits<uint32>::max()) + 1;
    CHECK(warden::InspectCheckPlan(warden::ModuleAbi::Cata15595X86,
        x86, budget) == warden::CheckPlanValidation::InvalidDefinition);
}

TEST(WardenPacketCodec_x64_rejects_mpq_and_unpublished_check_code_maps)
{
    std::optional<warden::WardenCheckXorKey> const key =
        X64CheckXorKey();
    REQUIRE(key.has_value());

    warden::CheckPlan mpqPlan = ThreeFamilyX64Plan();
    warden::MpqCheckProfile mpqProfile;
    mpqProfile.checkId = 4;
    mpqProfile.path = "DBFilesClient\\Item.db2";
    warden::WardenCheckDefinition mpq;
    mpq.sortOrder = 25;
    mpq.evidenceClass = warden::WardenEvidenceClass::IntegrityInvariant;
    mpq.phaseMask = warden::PhaseInitial;
    mpq.addressKind = warden::WardenAddressKind::None;
    mpq.payload = mpqProfile;
    mpqPlan.checks.insert(mpqPlan.checks.begin() + 2, mpq);

    warden::Bytes encoded = {0xA5};
    CHECK(warden::EncodeCheckRequest(X64FullProfile(), *key, mpqPlan,
        encoded) == warden::EncodeStatus::InvalidPlan);
    CHECK_HEX(encoded.data(), encoded.size(), "a5");

    std::array<warden::ModuleCheckCodes, 4> const invalidCodes = {{
        {0xEB, 0x51, 0x00, 0x36},
        {0xEA, 0x50, 0x00, 0x36},
        {0xEA, 0x51, 0x01, 0x36},
        {0xEA, 0x51, 0x00, 0x37}}};
    for (warden::ModuleCheckCodes const& codes : invalidCodes)
    {
        warden::ModuleProfile profile = X64FullProfile();
        profile.checkCodes = codes;
        encoded = {0xA5};
        CHECK(warden::EncodeCheckRequest(profile, *key,
            ThreeFamilyX64Plan(), encoded) ==
            warden::EncodeStatus::InvalidProfile);
        CHECK_HEX(encoded.data(), encoded.size(), "a5");
    }
}

TEST(WardenPacketCodec_x86_decodes_exact_four_family_result_transactionally)
{
    warden::Bytes const plaintext = ValidFourFamilyResult();
    warden::CheckBatchResult decoded;
    CHECK(warden::DecodeCheckResult(warden::ModuleAbi::Cata15595X86,
        warden::ByteView(plaintext), FourFamilyX86Plan(), decoded) ==
        warden::DecodeStatus::Ok);
    REQUIRE(decoded.checks.size() == 4u);

    warden::TimingResult const* timing =
        std::get_if<warden::TimingResult>(&decoded.checks[0]);
    REQUIRE(timing != nullptr);
    CHECK(timing->stable);
    CHECK_EQ(timing->clientTick, uint32(0x12345678));

    warden::LuaResult const* lua =
        std::get_if<warden::LuaResult>(&decoded.checks[1]);
    REQUIRE(lua != nullptr);
    CHECK(lua->status == warden::LuaResultStatus::Success);
    CHECK(lua->text == "ok");

    warden::MpqResult const* mpq =
        std::get_if<warden::MpqResult>(&decoded.checks[2]);
    REQUIRE(mpq != nullptr);
    CHECK(mpq->status == warden::MpqResultStatus::Success);
    for (std::size_t index = 0; index < mpq->digest.size(); ++index)
        CHECK_EQ(mpq->digest[index], uint8(index));

    warden::MemResult const* memory =
        std::get_if<warden::MemResult>(&decoded.checks[3]);
    REQUIRE(memory != nullptr);
    CHECK(memory->status == warden::MemResultStatus::Success);
    CHECK(memory->actualBytes ==
        warden::Bytes({0xE8, 0xB1, 0xED, 0xFF, 0xFF}));
}

TEST(WardenPacketCodec_x86_rejects_mutated_result_envelopes_transactionally)
{
    warden::Bytes mutated = ValidFourFamilyResult();
    mutated[0] ^= 0x01;
    CheckRejectedCheckResult(
        mutated, warden::DecodeStatus::UnsupportedCommand);

    mutated = ValidFourFamilyResult();
    --mutated[1];
    CheckRejectedCheckResult(mutated, warden::DecodeStatus::WrongSize);

    mutated = ValidFourFamilyResult();
    ++mutated[1];
    CheckRejectedCheckResult(mutated, warden::DecodeStatus::WrongSize);

    mutated = ValidFourFamilyResult();
    mutated[3] ^= 0x01;
    CheckRejectedCheckResult(mutated, warden::DecodeStatus::ChecksumMismatch);

    mutated = ValidFourFamilyResult();
    CheckRejectedCheckResult(mutated, warden::DecodeStatus::InvalidAbi,
        warden::ModuleAbi::Cata15595X64);
}

TEST(WardenPacketCodec_x86_rejects_each_malformed_result_member)
{
    // Body offsets: timing 0..4, Lua 5..8, MPQ 9..29, memory 30..35.
    for (std::size_t bodyOffset : {std::size_t(0), std::size_t(5),
             std::size_t(9), std::size_t(30)})
    {
        warden::Bytes mutated = ValidFourFamilyResult();
        mutated[warden::CheckResultEnvelopeSize + bodyOffset] = 2;
        REQUIRE(RewriteCheckResultEnvelope(mutated));
        CheckRejectedCheckResult(mutated, warden::DecodeStatus::InvalidValue);
    }

    warden::Bytes malformedLua = ValidFourFamilyResult();
    malformedLua[warden::CheckResultEnvelopeSize + 6] = 0xFF;
    REQUIRE(RewriteCheckResultEnvelope(malformedLua));
    CheckRejectedCheckResult(
        malformedLua, warden::DecodeStatus::WrongSize);

    warden::Bytes truncatedMemory = ValidFourFamilyResult();
    truncatedMemory.pop_back();
    REQUIRE(RewriteCheckResultEnvelope(truncatedMemory));
    CheckRejectedCheckResult(
        truncatedMemory, warden::DecodeStatus::WrongSize);

    warden::Bytes trailing = ValidFourFamilyResult();
    trailing.push_back(0xA5);
    REQUIRE(RewriteCheckResultEnvelope(trailing));
    CheckRejectedCheckResult(trailing, warden::DecodeStatus::WrongSize);
}
