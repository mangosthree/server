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

#include <array>
#include <type_traits>
#include <vector>

namespace
{
warden::ModuleProfile SyntheticProfile(warden::WardenArchitecture architecture)
{
    warden::ModuleProfile profile;
    profile.key = {15595, architecture};
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
    REQUIRE(BuildProfiles({SyntheticProfile(warden::WardenArchitecture::X86)},
        catalog) == warden::ModuleCatalogValidation::Valid);

    CHECK(catalog.FindExact({15594, warden::WardenArchitecture::X86}) ==
        nullptr);
    CHECK(catalog.FindExact({15595, warden::WardenArchitecture::X64}) ==
        nullptr);
    CHECK(catalog.FindExact(
        {15595, warden::WardenArchitecture::Unclassified}) == nullptr);
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
