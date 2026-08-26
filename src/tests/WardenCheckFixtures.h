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

#ifndef MANGOS_TEST_WARDEN_CHECK_FIXTURES_H
#define MANGOS_TEST_WARDEN_CHECK_FIXTURES_H

#include "WardenCheckCatalog.h"
#include "WardenModuleCatalog.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace warden
{
namespace test
{
inline std::array<char, 4> EnUsLocale()
{
    return {{'e', 'n', 'U', 'S'}};
}

inline std::string ArchitectureHex(WardenArchitecture architecture)
{
    switch (architecture)
    {
        case WardenArchitecture::X86: return "783836";
        case WardenArchitecture::X64: return "783634";
        case WardenArchitecture::Unclassified: return "756E6B";
    }
    return {};
}

inline std::string VariantHex(ClientVariant variant)
{
    switch (variant)
    {
        case ClientVariant::Unclassified:
            return "756E636C6173736966696564";
        case ClientVariant::Stock: return "73746F636B";
        case ClientVariant::Grunt: return "6772756E74";
        case ClientVariant::LegacyGrunt:
            return "6C65676163792D6772756E74";
    }
    return {};
}

inline WardenCheckRowInput MakeRow(WardenArchitecture architecture,
    ClientVariant variant, uint32 checkId, WardenCheckType type,
    uint32 sortOrder, WardenEvidenceClass evidenceClass, uint32 phaseMask,
    WardenAddressKind addressKind)
{
    WardenCheckRowInput row;
    row.build = 15595;
    row.architectureHex = ArchitectureHex(architecture);
    row.variantHex = VariantHex(variant);
    row.localeHex = "656E5553";
    row.checkId = checkId;
    row.type = static_cast<uint32>(type);
    row.enabled = 1;
    row.sortOrder = sortOrder;
    row.evidenceClass = static_cast<uint32>(evidenceClass);
    row.phaseMask = phaseMask;
    row.addressKind = static_cast<uint32>(addressKind);
    return row;
}

/** Test-only database rows for the architecture-specific profile probe. */
inline std::vector<WardenCheckRowInput> ProfileProbeRows(
    WardenArchitecture architecture)
{
    std::array<uint64, 3> addresses = architecture == WardenArchitecture::X86 ?
        std::array<uint64, 3>{{0x00007F7A, 0x00088FAE, 0x000895CA}} :
        std::array<uint64, 3>{{0x000AB76F, 0x000AABAB, 0x000AA6D3}};
    std::array<uint32, 3> lengths = architecture == WardenArchitecture::X86 ?
        std::array<uint32, 3>{{5, 1, 7}} :
        std::array<uint32, 3>{{5, 2, 2}};

    std::vector<WardenCheckRowInput> rows;
    for (uint32 index = 0; index < 3; ++index)
    {
        WardenCheckRowInput row = MakeRow(architecture,
            ClientVariant::Unclassified, 1001 + index,
            WardenCheckType::Mem, 10 * (index + 1),
            WardenEvidenceClass::Corroboration, PhaseProfileProbe,
            WardenAddressKind::ModuleRelativeRva);
        row.moduleHex = "576F772E657865"; // Wow.exe
        row.address = addresses[index];
        row.length = lengths[index];
        // Probe bytes are classified against the complete columns below.
        // They are never treated as a row-level actionable expectation.
        row.expectedHex.clear();
        rows.push_back(row);
    }
    return rows;
}

/** Test-only exact profile used to exercise phase selection and preflight. */
inline std::vector<WardenCheckRowInput> ClassifiedRows(
    WardenArchitecture architecture, ClientVariant variant)
{
    std::vector<WardenCheckRowInput> rows;

    WardenCheckRowInput timing = MakeRow(architecture, variant, 2001,
        WardenCheckType::Timing, 10,
        WardenEvidenceClass::ProtocolHealth,
        PhaseInitial | PhaseRecurring, WardenAddressKind::None);
    rows.push_back(timing);

    WardenCheckRowInput invariant = MakeRow(architecture, variant, 2002,
        WardenCheckType::Mem, 20,
        WardenEvidenceClass::IntegrityInvariant,
        PhaseInitial | PhaseRecurring | PhaseAggressive,
        WardenAddressKind::AbsoluteVa);
    invariant.address = architecture == WardenArchitecture::X86 ?
        uint64(0x00401000) : uint64(0x0000000140001000);
    invariant.length = 4;
    invariant.expectedHex = "90909090";
    rows.push_back(invariant);

    WardenCheckRowInput corroboration = MakeRow(architecture, variant, 2003,
        WardenCheckType::Mem, 30, WardenEvidenceClass::Corroboration,
        PhaseRecurring, WardenAddressKind::ModuleRelativeRva);
    corroboration.moduleHex = "576F772E657865";
    corroboration.address = 0x1000;
    corroboration.length = 2;
    corroboration.expectedHex = "4D5A";
    rows.push_back(corroboration);

    return rows;
}

inline std::vector<WardenCheckRowInput> CompleteX86Rows()
{
    std::vector<WardenCheckRowInput> rows =
        ProfileProbeRows(WardenArchitecture::X86);
    for (ClientVariant variant :
        {ClientVariant::Stock, ClientVariant::Grunt})
    {
        std::vector<WardenCheckRowInput> classified =
            ClassifiedRows(WardenArchitecture::X86, variant);
        rows.insert(rows.end(), classified.begin(), classified.end());
    }
    return rows;
}

inline WardenCheckCatalog BuildX86Catalog()
{
    WardenCheckCatalogBuilder builder;
    WardenCheckDiagnostic diagnostic;
    for (WardenCheckRowInput const& row : CompleteX86Rows())
    {
        if (builder.Add(row, diagnostic) != CheckCatalogValidation::Valid)
            return {};
    }

    WardenCheckCatalog catalog;
    if (builder.Build(catalog, diagnostic) != CheckCatalogValidation::Valid)
        return {};
    return catalog;
}

/** Exact 40-byte key used only by synthetic state-machine peers. */
inline SessionKey SyntheticSessionKey()
{
    SessionKey key{};
    for (std::size_t index = 0; index < key.size(); ++index)
        key[index] = uint8(index + 1);
    return key;
}

/**
 * Synthetic encrypted-container stand-in. The bytes, keys and SHA-256 IDs are
 * deliberately unrelated to any shipped client module and never leave tests.
 */
inline ModuleProfile SyntheticModuleProfile(WardenArchitecture architecture)
{
    ModuleProfile profile;
    profile.key = {15595, architecture};
    std::size_t const size = architecture == WardenArchitecture::X86 ?
        std::size_t(1201) : std::size_t(1001);
    uint8 const multiplier = architecture == WardenArchitecture::X86 ?
        uint8(17) : uint8(29);
    uint8 const addend = architecture == WardenArchitecture::X86 ?
        uint8(0x31) : uint8(0x52);
    profile.container.resize(size);
    for (std::size_t index = 0; index < size; ++index)
        profile.container[index] = uint8(index * multiplier + addend);
    profile.declaredSize = uint32(profile.container.size());

    profile.moduleId = architecture == WardenArchitecture::X86 ?
        ModuleId{{
            0x77, 0x5E, 0xE2, 0x67, 0x66, 0xB2, 0x6E, 0x51,
            0xED, 0x28, 0x72, 0x3A, 0x95, 0xD4, 0x7C, 0xFF,
            0x81, 0x90, 0xC5, 0x9D, 0x07, 0x70, 0x75, 0xD0,
            0xD5, 0x0F, 0x65, 0x80, 0x3A, 0x4E, 0x42, 0xE2}} :
        ModuleId{{
            0xDB, 0x0A, 0xA9, 0xDC, 0xBF, 0x5F, 0xCE, 0xF6,
            0x77, 0xC7, 0xA9, 0x4B, 0x3F, 0x66, 0xAF, 0x40,
            0xB1, 0xEA, 0xFA, 0x1A, 0x9A, 0x2F, 0x23, 0xC4,
            0xBE, 0xF6, 0xD6, 0x3A, 0xC8, 0xE3, 0x02, 0xFD}};
    for (std::size_t index = 0; index < profile.moduleKey.size(); ++index)
    {
        profile.moduleKey[index] = uint8(index +
            (architecture == WardenArchitecture::X86 ? 1 : 0x41));
    }
    return profile;
}

inline WardenModuleCatalog BuildSyntheticModuleCatalog()
{
    WardenModuleCatalogBuilder builder;
    if (builder.Add(SyntheticModuleProfile(WardenArchitecture::X86)) !=
            ModuleCatalogValidation::Valid ||
        builder.Add(SyntheticModuleProfile(WardenArchitecture::X64)) !=
            ModuleCatalogValidation::Valid)
    {
        return {};
    }

    WardenModuleCatalog catalog;
    if (builder.Build(catalog) != ModuleCatalogValidation::Valid)
        return {};
    return catalog;
}

inline std::vector<WardenCheckRowInput> CompleteSyntheticRows()
{
    std::vector<WardenCheckRowInput> rows;
    for (WardenArchitecture architecture :
        {WardenArchitecture::X86, WardenArchitecture::X64})
    {
        std::vector<WardenCheckRowInput> probe =
            ProfileProbeRows(architecture);
        rows.insert(rows.end(), probe.begin(), probe.end());
        for (ClientVariant variant :
            {ClientVariant::Stock, ClientVariant::Grunt})
        {
            std::vector<WardenCheckRowInput> classified =
                ClassifiedRows(architecture, variant);
            rows.insert(rows.end(), classified.begin(), classified.end());
        }
    }
    return rows;
}

inline WardenCheckCatalog BuildSyntheticCheckCatalog()
{
    WardenCheckCatalogBuilder builder;
    WardenCheckDiagnostic diagnostic;
    for (WardenCheckRowInput const& row : CompleteSyntheticRows())
    {
        if (builder.Add(row, diagnostic) != CheckCatalogValidation::Valid)
            return {};
    }

    WardenCheckCatalog catalog;
    if (builder.Build(catalog, diagnostic) != CheckCatalogValidation::Valid)
        return {};
    return catalog;
}

inline std::vector<Bytes> X86StockFingerprint()
{
    return {{0xE8, 0xB1, 0xED, 0xFF, 0xFF}, {0x74},
        {0x8B, 0x55, 0x0C, 0x83, 0xFA, 0x02, 0x75}};
}

inline std::vector<Bytes> X86GruntFingerprint()
{
    return {{0xB8, 0x01, 0x00, 0x00, 0x00}, {0xEB},
        {0xBA, 0x00, 0x00, 0x00, 0x00, 0x90, 0xEB}};
}

inline std::vector<Bytes> X64StockFingerprint()
{
    return {{0x74, 0x08, 0x41, 0x8B, 0xD5}, {0x74, 0x1A},
        {0x74, 0x10}};
}

inline std::vector<Bytes> X64GruntFingerprint()
{
    return {{0x90, 0x90, 0x31, 0xD2, 0x90}, {0xEB, 0x1A},
        {0x74, 0x10}};
}

inline std::vector<Bytes> X64LegacyGruntFingerprint()
{
    return {{0x90, 0x90, 0x31, 0xD2, 0x90}, {0x74, 0x1A},
        {0xEB, 0x10}};
}
}
}

#endif
