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

#include "WardenModuleCatalog.h"
#include "WardenModuleWin15595X64Data.h"
#include "WardenModuleWin15595X86Data.h"

#include <array>
#include <type_traits>
#include <vector>

namespace
{
warden::ModuleProfile SyntheticProfile(warden::WardenArchitecture architecture)
{
    warden::ModuleProfile profile;
    profile.key = {15595, architecture};
    profile.abi = architecture == warden::WardenArchitecture::X86 ?
        warden::ModuleAbi::Cata15595X86 :
        warden::ModuleAbi::Cata15595X64;
    profile.provenance = architecture == warden::WardenArchitecture::X86 ?
        warden::ModuleProvenance::BuildMatchedPublic :
        warden::ModuleProvenance::SignedCrossBuild;
    profile.operatingMode = architecture == warden::WardenArchitecture::X86 ?
        warden::ModuleOperatingMode::Full :
        warden::ModuleOperatingMode::CompatibilityProbeOnly;
    profile.assurance = warden::ModuleAssurance::StaticVerified;
    if (architecture == warden::WardenArchitecture::X86)
        profile.checkCodes = {
            warden::Cata15595X86TimingCode,
            warden::Cata15595X86LuaCode,
            0,
            warden::Cata15595X86MemoryCode};
    else
        profile.checkCodes = {0xEA, 0x00, 0x00, 0x00};
    profile.rekey.seed[0] = 0x11;
    profile.rekey.expectedResponse[0] = 0x22;
    profile.rekey.clientToServer[0] = 0x33;
    profile.rekey.serverToClient[0] = 0x44;
    profile.container = {0x00, 0x01, 0x02, 0x03};
    profile.declaredSize = 4;
    // SHA-256 of the encrypted synthetic container above. Production module
    // bytes and keys never belong in unit-test fixtures.
    profile.moduleId = {{
        0x05, 0x4E, 0xDE, 0xC1, 0xD0, 0x21, 0x1F, 0x62,
        0x4F, 0xED, 0x0C, 0xBC, 0xA9, 0xD4, 0xF9, 0x40,
        0x0B, 0x0E, 0x49, 0x1C, 0x43, 0x74, 0x2A, 0xF2,
        0xC5, 0xB0, 0xAB, 0xEB, 0xF0, 0xC9, 0x90, 0xD8}};
    profile.moduleKey = {{
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}};
    return profile;
}

warden::ModuleCatalogValidation BuildProfiles(
    std::vector<warden::ModuleProfile> const& profiles,
    warden::WardenModuleCatalog& catalog)
{
    warden::WardenModuleCatalogBuilder builder;
    for (warden::ModuleProfile const& profile : profiles)
    {
        warden::ModuleCatalogValidation const result = builder.Add(profile);
        if (result != warden::ModuleCatalogValidation::Valid)
            return result;
    }
    return builder.Build(catalog);
}
}

static_assert(std::tuple_size<warden::ModuleId>::value == 32,
    "Cata ModuleUse identities are exactly 32 bytes");

TEST(WardenModuleCatalog_accepts_exact_15595_architecture_profiles)
{
    warden::WardenModuleCatalog catalog;
    REQUIRE(BuildProfiles({
        SyntheticProfile(warden::WardenArchitecture::X86),
        SyntheticProfile(warden::WardenArchitecture::X64)}, catalog) ==
        warden::ModuleCatalogValidation::Valid);

    CHECK_EQ(catalog.Profiles().size(), size_t(2));
    CHECK(catalog.FindExact({15595, warden::WardenArchitecture::X86}) !=
        nullptr);
    CHECK(catalog.FindExact({15595, warden::WardenArchitecture::X64}) !=
        nullptr);
}

TEST(WardenModuleCatalog_never_falls_back_across_build_or_architecture)
{
    warden::WardenModuleCatalog catalog;
    REQUIRE(BuildProfiles({
        SyntheticProfile(warden::WardenArchitecture::X86),
        SyntheticProfile(warden::WardenArchitecture::X64)}, catalog) ==
        warden::ModuleCatalogValidation::Valid);

    CHECK(catalog.FindExact({15594, warden::WardenArchitecture::X86}) ==
        nullptr);
    CHECK(catalog.FindExact(
        {15595, warden::WardenArchitecture::Unclassified}) == nullptr);
}

TEST(WardenModuleCatalog_requires_both_architecture_profiles)
{
    warden::WardenModuleCatalog catalog;
    CHECK(BuildProfiles({SyntheticProfile(warden::WardenArchitecture::X86)},
        catalog) != warden::ModuleCatalogValidation::Valid);
    CHECK(catalog.Profiles().empty());
}

TEST(WardenModuleCatalog_allows_signed_cross_build_full_below_production_assurance)
{
    warden::ModuleProfile profile =
        SyntheticProfile(warden::WardenArchitecture::X64);
    profile.operatingMode = warden::ModuleOperatingMode::Full;
    profile.checkCodes = {0xEA, 0x51, 0x00, 0x36};

    auto validateOne = [](warden::ModuleProfile const& candidate)
    {
        warden::WardenModuleCatalogBuilder builder;
        return builder.Add(candidate);
    };

    profile.assurance = warden::ModuleAssurance::StaticVerified;
    CHECK(validateOne(profile) == warden::ModuleCatalogValidation::Valid);

    profile.assurance = warden::ModuleAssurance::ExactClientLabValidated;
    CHECK(validateOne(profile) == warden::ModuleCatalogValidation::Valid);

    profile.assurance = warden::ModuleAssurance::ProductionApproved;
    CHECK(validateOne(profile) ==
        warden::ModuleCatalogValidation::InvalidMetadata);

    profile.operatingMode =
        warden::ModuleOperatingMode::CompatibilityProbeOnly;
    profile.checkCodes = {0xEA, 0x00, 0x00, 0x00};
    CHECK(validateOne(profile) ==
        warden::ModuleCatalogValidation::InvalidMetadata);
}

TEST(WardenModuleCatalog_requires_the_supported_x64_full_check_code_map)
{
    auto validateOne = [](warden::ModuleCheckCodes const& codes)
    {
        warden::ModuleProfile profile =
            SyntheticProfile(warden::WardenArchitecture::X64);
        profile.operatingMode = warden::ModuleOperatingMode::Full;
        profile.checkCodes = codes;
        warden::WardenModuleCatalogBuilder builder;
        return builder.Add(profile);
    };

    warden::ModuleCheckCodes const valid = {0xEA, 0x51, 0x00, 0x36};
    CHECK(validateOne(valid) == warden::ModuleCatalogValidation::Valid);

    std::array<warden::ModuleCheckCodes, 4> const substitutions = {{
        {0xEB, 0x51, 0x00, 0x36},
        {0xEA, 0x50, 0x00, 0x36},
        {0xEA, 0x51, 0x01, 0x36},
        {0xEA, 0x51, 0x00, 0x37}}};
    for (warden::ModuleCheckCodes const& codes : substitutions)
    {
        CHECK(validateOne(codes) ==
            warden::ModuleCatalogValidation::InvalidCheckCodeMap);
    }

    std::array<warden::ModuleCheckCodes, 6> const duplicates = {{
        {0xEA, 0xEA, 0x00, 0x36},
        {0xEA, 0x51, 0xEA, 0x36},
        {0xEA, 0x51, 0x00, 0xEA},
        {0xEA, 0x51, 0x51, 0x36},
        {0xEA, 0x51, 0x00, 0x51},
        {0xEA, 0x51, 0x36, 0x36}}};
    for (warden::ModuleCheckCodes const& codes : duplicates)
    {
        CHECK(validateOne(codes) ==
            warden::ModuleCatalogValidation::DuplicateCheckCode);
    }
}

TEST(WardenModuleCatalog_rejects_duplicate_nonzero_check_codes)
{
    warden::ModuleProfile profile =
        SyntheticProfile(warden::WardenArchitecture::X86);
    profile.checkCodes.lua = profile.checkCodes.timing;

    warden::WardenModuleCatalogBuilder builder;
    CHECK(builder.Add(profile) != warden::ModuleCatalogValidation::Valid);
}

TEST(WardenModuleCatalog_requires_the_supported_x86_check_code_map)
{
    warden::ModuleProfile profile =
        SyntheticProfile(warden::WardenArchitecture::X86);
    profile.checkCodes.memory = 0;

    warden::WardenModuleCatalogBuilder missingCode;
    CHECK(missingCode.Add(profile) ==
        warden::ModuleCatalogValidation::InvalidCheckCodeMap);

    profile = SyntheticProfile(warden::WardenArchitecture::X86);
    profile.checkCodes.lua ^= 0x01;
    warden::WardenModuleCatalogBuilder wrongCode;
    CHECK(wrongCode.Add(profile) ==
        warden::ModuleCatalogValidation::InvalidCheckCodeMap);

    profile = SyntheticProfile(warden::WardenArchitecture::X86);
    profile.checkCodes.mpq = warden::Cata15595X86MpqCode;
    warden::WardenModuleCatalogBuilder unsupportedMpq;
    CHECK(unsupportedMpq.Add(profile) ==
        warden::ModuleCatalogValidation::InvalidCheckCodeMap);
}

TEST(WardenModuleCatalog_restricts_probe_to_corrected_timing_code)
{
    warden::ModuleProfile profile =
        SyntheticProfile(warden::WardenArchitecture::X64);
    profile.checkCodes.timing = 0xAE;
    warden::WardenModuleCatalogBuilder wrongTiming;
    CHECK(wrongTiming.Add(profile) !=
        warden::ModuleCatalogValidation::Valid);

    profile = SyntheticProfile(warden::WardenArchitecture::X64);
    profile.checkCodes.memory = 0x36;
    warden::WardenModuleCatalogBuilder unreviewedCheck;
    CHECK(unreviewedCheck.Add(profile) !=
        warden::ModuleCatalogValidation::Valid);
}

TEST(WardenModuleCatalog_rejects_missing_module_rekey_vector)
{
    warden::ModuleProfile profile =
        SyntheticProfile(warden::WardenArchitecture::X64);
    profile.rekey = {};

    warden::WardenModuleCatalogBuilder builder;
    CHECK(builder.Add(profile) != warden::ModuleCatalogValidation::Valid);
}

TEST(WardenModuleCatalog_rejects_unknown_or_non_15595_identity)
{
    warden::WardenModuleCatalogBuilder builder;
    warden::ModuleProfile profile =
        SyntheticProfile(warden::WardenArchitecture::X86);
    profile.key.build = 15594;
    CHECK(builder.Add(profile) ==
        warden::ModuleCatalogValidation::InvalidBuild);

    profile = SyntheticProfile(warden::WardenArchitecture::Unclassified);
    CHECK(builder.Add(profile) ==
        warden::ModuleCatalogValidation::InvalidArchitecture);
}

TEST(WardenModuleCatalog_rejects_duplicate_architecture_keys)
{
    warden::WardenModuleCatalog catalog;
    warden::ModuleProfile const profile =
        SyntheticProfile(warden::WardenArchitecture::X86);
    CHECK(BuildProfiles({profile, profile}, catalog) ==
        warden::ModuleCatalogValidation::DuplicateProfile);
    CHECK(catalog.Profiles().empty());
}

TEST(WardenModuleCatalog_rejects_empty_catalogue)
{
    warden::WardenModuleCatalogBuilder builder;
    warden::WardenModuleCatalog catalog;
    CHECK(builder.Build(catalog) ==
        warden::ModuleCatalogValidation::EmptyCatalog);
    CHECK(catalog.Profiles().empty());
}

TEST(WardenModuleCatalog_rejects_wrong_sha256_identity)
{
    warden::ModuleProfile profile =
        SyntheticProfile(warden::WardenArchitecture::X86);
    profile.moduleId[31] ^= 0x80;
    warden::WardenModuleCatalogBuilder builder;
    CHECK(builder.Add(profile) ==
        warden::ModuleCatalogValidation::InvalidModuleId);
}

TEST(WardenModuleCatalog_rejects_empty_container_and_zero_key)
{
    warden::ModuleProfile profile =
        SyntheticProfile(warden::WardenArchitecture::X86);
    profile.container.clear();
    warden::WardenModuleCatalogBuilder builder;
    CHECK(builder.Add(profile) ==
        warden::ModuleCatalogValidation::EmptyContainer);

    profile = SyntheticProfile(warden::WardenArchitecture::X86);
    profile.moduleKey.fill(0);
    CHECK(builder.Add(profile) ==
        warden::ModuleCatalogValidation::InvalidModuleKey);
}

TEST(WardenModuleCatalog_rejects_declared_container_size_mismatch)
{
    warden::ModuleProfile profile =
        SyntheticProfile(warden::WardenArchitecture::X86);
    profile.declaredSize = 3;
    warden::WardenModuleCatalogBuilder builder;
    CHECK(builder.Add(profile) ==
        warden::ModuleCatalogValidation::InvalidContainerSize);

    profile.declaredSize = 5;
    CHECK(builder.Add(profile) ==
        warden::ModuleCatalogValidation::InvalidContainerSize);
}

TEST(WardenModuleCatalog_compiled_profiles_match_custody_manifests)
{
    warden::ModuleProfile const& x86 =
        warden::GetWardenModuleWin15595X86Profile();
    CHECK_EQ(x86.key.build, uint32(15595));
    CHECK(x86.key.architecture == warden::WardenArchitecture::X86);
    CHECK(x86.abi == warden::ModuleAbi::Cata15595X86);
    CHECK(x86.provenance == warden::ModuleProvenance::BuildMatchedPublic);
    CHECK(x86.operatingMode == warden::ModuleOperatingMode::Full);
    CHECK(x86.assurance == warden::ModuleAssurance::StaticVerified);
    CHECK_EQ(x86.declaredSize, uint32(18439));
    CHECK_EQ(x86.container.size(), size_t(18439));
    CHECK_HEX(x86.moduleId.data(), x86.moduleId.size(),
        "7ad7870d064c5a2bc8e55b00c23239b6e964c622c298beb99187fc6f163df4cd");
    CHECK_HEX(x86.moduleKey.data(), x86.moduleKey.size(),
        "14cf93daf112b3faa823cc0914e54627");
    CHECK_HEX(x86.rekey.seed.data(), x86.rekey.seed.size(),
        "49f95776e6ddf99d9de91d75cc93e955");
    CHECK_HEX(x86.rekey.expectedResponse.data(),
        x86.rekey.expectedResponse.size(),
        "71be54fdf23061892d6eea2fb79119b9f7e05084");
    CHECK_HEX(x86.rekey.clientToServer.data(),
        x86.rekey.clientToServer.size(),
        "8ab07213fcff7bacb77b4804d239445c");
    CHECK_HEX(x86.rekey.serverToClient.data(),
        x86.rekey.serverToClient.size(),
        "6aea6e524748f22d122b27d96622d765");
    CHECK_EQ(x86.checkCodes.timing, warden::Cata15595X86TimingCode);
    CHECK_EQ(x86.checkCodes.lua, warden::Cata15595X86LuaCode);
    CHECK_EQ(x86.checkCodes.mpq, uint8(0));
    CHECK_EQ(x86.checkCodes.memory, warden::Cata15595X86MemoryCode);

    warden::ModuleProfile const& x64 =
        warden::GetWardenModuleWin15595X64Profile();
    CHECK_EQ(x64.key.build, uint32(15595));
    CHECK(x64.key.architecture == warden::WardenArchitecture::X64);
    CHECK(x64.abi == warden::ModuleAbi::Cata15595X64);
    CHECK(x64.provenance == warden::ModuleProvenance::SignedCrossBuild);
    CHECK(x64.operatingMode == warden::ModuleOperatingMode::Full);
    CHECK(x64.assurance == warden::ModuleAssurance::StaticVerified);
    CHECK_EQ(x64.declaredSize, uint32(24405));
    CHECK_EQ(x64.container.size(), size_t(24405));
    CHECK_HEX(x64.moduleId.data(), x64.moduleId.size(),
        "3ead4470f0f4b6d4e5f620153f138993ce76821ad55c08866b600f54a8462248");
    CHECK_HEX(x64.moduleKey.data(), x64.moduleKey.size(),
        "2804d38f80eb03a6419f35371747d1f3");
    CHECK_HEX(x64.rekey.seed.data(), x64.rekey.seed.size(),
        "8db6e0c5865a1fdb810f26db773f681f");
    CHECK_HEX(x64.rekey.expectedResponse.data(),
        x64.rekey.expectedResponse.size(),
        "57790e891c05e7ceb34e6754daf39e8197ff5cec");
    CHECK_HEX(x64.rekey.clientToServer.data(),
        x64.rekey.clientToServer.size(),
        "558017aaed7fffab273cb00abf517795");
    CHECK_HEX(x64.rekey.serverToClient.data(),
        x64.rekey.serverToClient.size(),
        "1b12c1eab47a79a32b3f8f7b3c985912");
    CHECK_EQ(x64.checkCodes.timing, uint8(0xEA));
    CHECK_EQ(x64.checkCodes.lua, uint8(0x51));
    CHECK_EQ(x64.checkCodes.mpq, uint8(0));
    CHECK_EQ(x64.checkCodes.memory, uint8(0x36));

    warden::WardenModuleCatalog catalog;
    CHECK(BuildProfiles({x86, x64}, catalog) ==
        warden::ModuleCatalogValidation::Valid);
}
