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
#include <utility>
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
    std::vector<uint64> addresses = architecture == WardenArchitecture::X86 ?
        std::vector<uint64>{0x00007F7A, 0x00088FAE, 0x000895CA,
            0x003BFF88} :
        std::vector<uint64>{0x000AB76F, 0x000AABAB, 0x000AA6D3};
    std::vector<uint32> lengths = architecture == WardenArchitecture::X86 ?
        std::vector<uint32>{5, 1, 7, 24} :
        std::vector<uint32>{5, 2, 2};

    std::vector<WardenCheckRowInput> rows;
    for (uint32 index = 0; index < addresses.size(); ++index)
    {
        WardenCheckRowInput row = MakeRow(architecture,
            ClientVariant::Unclassified, 1001 + index,
            WardenCheckType::Mem, 10 * (index + 1),
            WardenEvidenceClass::Corroboration, PhaseProfileProbe,
            WardenAddressKind::ModuleRelativeRva);
        row.moduleHex = architecture == WardenArchitecture::X86 ?
            "576F772E657865" :       // Wow.exe
            "576F772D36342E657865";  // Wow-64.exe
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

    if (architecture == WardenArchitecture::X86 &&
        variant == ClientVariant::Grunt)
    {
        WardenCheckRowInput mpq = MakeRow(architecture, variant, 2004,
            WardenCheckType::Mpq, 25, WardenEvidenceClass::Corroboration,
            PhaseInitial | PhaseRecurring, WardenAddressKind::None);
        mpq.requestHex =
            "444246696C6573436C69656E745C4974656D2E646232";
        mpq.expectedHex =
            "4706FF83D9B611644A87DE79C244B414612EF4F2";
        rows.push_back(std::move(mpq));
    }

    if (architecture == WardenArchitecture::X64)
    {
        WardenCheckRowInput lua = MakeRow(architecture, variant, 2002,
            WardenCheckType::Lua, 20, WardenEvidenceClass::Corroboration,
            PhaseInitial | PhaseRecurring, WardenAddressKind::None);
        lua.requestHex = "4F4B4159";   // OKAY
        lua.expectedHex = "4F6B6179";  // Okay
        rows.push_back(lua);

        WardenCheckRowInput invariant = MakeRow(architecture, variant, 2003,
            WardenCheckType::Mem, 30,
            WardenEvidenceClass::IntegrityInvariant,
            PhaseInitial | PhaseRecurring | PhaseAggressive,
            WardenAddressKind::ModuleRelativeRva);
        invariant.moduleHex = "576F772D36342E657865"; // Wow-64.exe
        invariant.address = 0x00566C13;
        invariant.length = 16;
        invariant.expectedHex =
            "4883C9FF33C0488BFDBAF0D8FFFFF2AE";
        rows.push_back(invariant);
        return rows;
    }

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
    corroboration.moduleHex = architecture == WardenArchitecture::X86 ?
        "576F772E657865" :       // Wow.exe
        "576F772D36342E657865";  // Wow-64.exe
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
        {ClientVariant::Stock, ClientVariant::LegacyGrunt,
            ClientVariant::Grunt})
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
    profile.abi = architecture == WardenArchitecture::X86 ?
        ModuleAbi::Cata15595X86 : ModuleAbi::Cata15595X64;
    profile.provenance = architecture == WardenArchitecture::X86 ?
        ModuleProvenance::BuildMatchedPublic :
        ModuleProvenance::SignedCrossBuild;
    profile.operatingMode = ModuleOperatingMode::Full;
    profile.assurance = ModuleAssurance::StaticVerified;
    profile.checkCodes = architecture == WardenArchitecture::X86 ?
        ModuleCheckCodes{Cata15595X86TimingCode, Cata15595X86LuaCode,
            Cata15595X86MpqCode, Cata15595X86MemoryCode} :
        ModuleCheckCodes{Cata15595X64TimingCode, Cata15595X64LuaCode,
            Cata15595X64MpqCode, Cata15595X64MemoryCode};
    if (architecture == WardenArchitecture::X86)
    {
        profile.rekey.seed = {{
            0x49, 0xF9, 0x57, 0x76, 0xE6, 0xDD, 0xF9, 0x9D,
            0x9D, 0xE9, 0x1D, 0x75, 0xCC, 0x93, 0xE9, 0x55}};
        profile.rekey.expectedResponse = {{
            0x71, 0xBE, 0x54, 0xFD, 0xF2, 0x30, 0x61, 0x89,
            0x2D, 0x6E, 0xEA, 0x2F, 0xB7, 0x91, 0x19, 0xB9,
            0xF7, 0xE0, 0x50, 0x84}};
        profile.rekey.clientToServer = {{
            0x8A, 0xB0, 0x72, 0x13, 0xFC, 0xFF, 0x7B, 0xAC,
            0xB7, 0x7B, 0x48, 0x04, 0xD2, 0x39, 0x44, 0x5C}};
        profile.rekey.serverToClient = {{
            0x6A, 0xEA, 0x6E, 0x52, 0x47, 0x48, 0xF2, 0x2D,
            0x12, 0x2B, 0x27, 0xD9, 0x66, 0x22, 0xD7, 0x65}};
    }
    else
    {
        profile.rekey.seed = {{
            0x8D, 0xB6, 0xE0, 0xC5, 0x86, 0x5A, 0x1F, 0xDB,
            0x81, 0x0F, 0x26, 0xDB, 0x77, 0x3F, 0x68, 0x1F}};
        profile.rekey.expectedResponse = {{
            0x57, 0x79, 0x0E, 0x89, 0x1C, 0x05, 0xE7, 0xCE,
            0xB3, 0x4E, 0x67, 0x54, 0xDA, 0xF3, 0x9E, 0x81,
            0x97, 0xFF, 0x5C, 0xEC}};
        profile.rekey.clientToServer = {{
            0x55, 0x80, 0x17, 0xAA, 0xED, 0x7F, 0xFF, 0xAB,
            0x27, 0x3C, 0xB0, 0x0A, 0xBF, 0x51, 0x77, 0x95}};
        profile.rekey.serverToClient = {{
            0x1B, 0x12, 0xC1, 0xEA, 0xB4, 0x7A, 0x79, 0xA3,
            0x2B, 0x3F, 0x8F, 0x7B, 0x3C, 0x98, 0x59, 0x12}};
    }
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
    std::vector<WardenCheckRowInput> rows = CompleteX86Rows();
    std::vector<WardenCheckRowInput> x64 =
        ProfileProbeRows(WardenArchitecture::X64);
    for (ClientVariant variant :
        {ClientVariant::Stock, ClientVariant::Grunt})
    {
        std::vector<WardenCheckRowInput> classified =
            ClassifiedRows(WardenArchitecture::X64, variant);
        x64.insert(x64.end(), classified.begin(), classified.end());
    }
    rows.insert(rows.end(), x64.begin(), x64.end());
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
        {0x8B, 0x55, 0x0C, 0x83, 0xFA, 0x02, 0x75},
        Bytes(24, 0xCC)};
}

inline std::vector<Bytes> X86LegacyGruntFingerprint()
{
    return {{0xB8, 0x01, 0x00, 0x00, 0x00}, {0xEB},
        {0xBA, 0x00, 0x00, 0x00, 0x00, 0x90, 0xEB},
        Bytes(24, 0xCC)};
}

inline std::vector<Bytes> X86GruntFingerprint()
{
    return {{0xB8, 0x01, 0x00, 0x00, 0x00}, {0xEB},
        {0xBA, 0x00, 0x00, 0x00, 0x00, 0x90, 0xEB},
        {0x55, 0x8B, 0xEC, 0xFF, 0x75, 0x14, 0xFF, 0x75,
            0x10, 0xFF, 0x75, 0x0C, 0xFF, 0x75, 0x08, 0xE8,
            0xB4, 0x65, 0xFE, 0xFF, 0x5D, 0xC2, 0x14, 0x00}};
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
