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

#include "WardenModuleCatalog.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace
{
bool SameKey(warden::ModuleProfileKey const& left,
    warden::ModuleProfileKey const& right)
{
    return left.build == right.build &&
        left.architecture == right.architecture;
}

template <std::size_t Size>
bool IsZero(std::array<uint8, Size> const& value)
{
    return std::all_of(value.begin(), value.end(),
        [](uint8 value) { return value == 0; });
}

bool HasValidMetadata(warden::ModuleProfile const& profile)
{
    bool const abiMatches =
        (profile.key.architecture == warden::WardenArchitecture::X86 &&
            profile.abi == warden::ModuleAbi::Cata15595X86) ||
        (profile.key.architecture == warden::WardenArchitecture::X64 &&
            profile.abi == warden::ModuleAbi::Cata15595X64);
    if (!abiMatches)
        return false;

    switch (profile.assurance)
    {
        case warden::ModuleAssurance::StaticVerified:
        case warden::ModuleAssurance::ExactClientLabValidated:
        case warden::ModuleAssurance::ProductionApproved:
            break;
        default:
            return false;
    }

    if (profile.operatingMode == warden::ModuleOperatingMode::Full)
    {
        return profile.provenance ==
                warden::ModuleProvenance::RetailCaptured15595 ||
            profile.provenance ==
                warden::ModuleProvenance::BuildMatchedPublic;
    }

    return profile.operatingMode ==
            warden::ModuleOperatingMode::CompatibilityProbeOnly &&
        profile.provenance == warden::ModuleProvenance::SignedCrossBuild &&
        profile.assurance != warden::ModuleAssurance::ProductionApproved;
}

bool HasDuplicateNonzeroCheckCode(warden::ModuleCheckCodes const& codes)
{
    std::array<uint8, 4> const values =
        {codes.timing, codes.lua, codes.mpq, codes.memory};
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (values[index] == 0)
            continue;
        if (std::find(values.begin(), values.begin() + index,
                values[index]) != values.begin() + index)
        {
            return true;
        }
    }
    return false;
}

bool Sha256(warden::Bytes const& input, warden::ModuleId& digest)
{
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context)
        return false;

    unsigned int length = 0;
    bool const success =
        EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
        EVP_DigestUpdate(context, input.data(), input.size()) == 1 &&
        EVP_DigestFinal_ex(context, digest.data(), &length) == 1 &&
        length == digest.size();
    EVP_MD_CTX_free(context);
    return success;
}
}

namespace warden
{
ModuleProfile const* WardenModuleCatalog::FindExact(
    ModuleProfileKey const& key) const
{
    auto const profile = std::find_if(m_profiles.begin(), m_profiles.end(),
        [&key](ModuleProfile const& candidate)
        {
            return SameKey(candidate.key, key);
        });
    return profile == m_profiles.end() ? nullptr : &*profile;
}

std::vector<ModuleProfile> const& WardenModuleCatalog::Profiles() const
{
    return m_profiles;
}

ModuleCatalogValidation WardenModuleCatalogBuilder::Add(
    ModuleProfile const& profile)
{
    if (profile.key.build != 15595)
        return ModuleCatalogValidation::InvalidBuild;
    if (profile.key.architecture != WardenArchitecture::X86 &&
        profile.key.architecture != WardenArchitecture::X64)
        return ModuleCatalogValidation::InvalidArchitecture;
    bool const abiMatches =
        (profile.key.architecture == WardenArchitecture::X86 &&
            profile.abi == ModuleAbi::Cata15595X86) ||
        (profile.key.architecture == WardenArchitecture::X64 &&
            profile.abi == ModuleAbi::Cata15595X64);
    if (!abiMatches)
        return ModuleCatalogValidation::InvalidAbi;
    if (!HasValidMetadata(profile))
        return ModuleCatalogValidation::InvalidMetadata;
    if (HasDuplicateNonzeroCheckCode(profile.checkCodes))
        return ModuleCatalogValidation::DuplicateCheckCode;
    if (profile.operatingMode == ModuleOperatingMode::Full &&
        (profile.abi != ModuleAbi::Cata15595X86 ||
            profile.checkCodes.timing != Cata15595X86TimingCode ||
            profile.checkCodes.lua != Cata15595X86LuaCode ||
            profile.checkCodes.mpq != Cata15595X86MpqCode ||
            profile.checkCodes.memory != Cata15595X86MemoryCode))
    {
        // The x64 full-scan ABI remains deliberately unpublished until an
        // exact-client module and its complete command grammar satisfy G2.
        return ModuleCatalogValidation::InvalidCheckCodeMap;
    }
    if (profile.operatingMode == ModuleOperatingMode::CompatibilityProbeOnly &&
        (profile.checkCodes.timing != CataX64CompatibilityTimingCode ||
            profile.checkCodes.lua != 0 || profile.checkCodes.mpq != 0 ||
            profile.checkCodes.memory != 0))
    {
        return ModuleCatalogValidation::InvalidCheckCodeMap;
    }
    if (IsZero(profile.rekey.seed) ||
        IsZero(profile.rekey.expectedResponse) ||
        IsZero(profile.rekey.clientToServer) ||
        IsZero(profile.rekey.serverToClient))
    {
        return ModuleCatalogValidation::InvalidRekeyVector;
    }
    if (profile.container.empty())
        return ModuleCatalogValidation::EmptyContainer;
    if (std::size_t(profile.declaredSize) != profile.container.size())
        return ModuleCatalogValidation::InvalidContainerSize;
    if (IsZero(profile.moduleKey))
        return ModuleCatalogValidation::InvalidModuleKey;

    ModuleId actual{};
    bool const digestValid = Sha256(profile.container, actual) &&
        CRYPTO_memcmp(actual.data(), profile.moduleId.data(), actual.size()) ==
            0;
    OPENSSL_cleanse(actual.data(), actual.size());
    if (!digestValid)
        return ModuleCatalogValidation::InvalidModuleId;

    if (std::any_of(m_profiles.begin(), m_profiles.end(),
        [&profile](ModuleProfile const& candidate)
        {
            return SameKey(candidate.key, profile.key);
        }))
        return ModuleCatalogValidation::DuplicateProfile;

    m_profiles.push_back(profile);
    return ModuleCatalogValidation::Valid;
}

ModuleCatalogValidation WardenModuleCatalogBuilder::Build(
    WardenModuleCatalog& output) const
{
    if (m_profiles.empty())
        return ModuleCatalogValidation::EmptyCatalog;

    bool const hasX86 = std::any_of(m_profiles.begin(), m_profiles.end(),
        [](ModuleProfile const& profile)
        {
            return profile.key.architecture == WardenArchitecture::X86;
        });
    bool const hasX64 = std::any_of(m_profiles.begin(), m_profiles.end(),
        [](ModuleProfile const& profile)
        {
            return profile.key.architecture == WardenArchitecture::X64;
        });
    if (!hasX86 || !hasX64)
        return ModuleCatalogValidation::IncompleteArchitectureSet;

    WardenModuleCatalog staged;
    staged.m_profiles = m_profiles;
    output = std::move(staged);
    return ModuleCatalogValidation::Valid;
}
}
