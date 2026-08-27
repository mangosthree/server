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

#ifndef MANGOS_WARDEN_CHECK_CATALOG_H
#define MANGOS_WARDEN_CHECK_CATALOG_H

#include "WardenProtocol.h"

#include <array>
#include <string>
#include <variant>
#include <vector>

namespace warden
{
#ifdef MANGOS_WARDEN_TEST_ACCESS
class WardenCheckCatalogTestAccess;
#endif
/**
 * Semantic check families only. G2 maps these to architecture-specific
 * module commands after the authentic module ABI has been recovered.
 */
enum class WardenCheckType : uint8
{
    Timing = 0,
    Lua = 1,
    Mpq = 2,
    Mem = 3
};

enum class WardenEvidenceClass : uint8
{
    ProtocolHealth = 0,
    IntegrityInvariant = 1,
    ThreatSignature = 2,
    Corroboration = 3
};

enum class WardenCheckOutcome : uint8
{
    Match = 0,
    Mismatch = 1,
    Unavailable = 2,
    Stable = 3,
    Unstable = 4
};

enum class WardenAddressKind : uint8
{
    None = 0,
    ModuleRelativeRva = 1,
    AbsoluteVa = 2
};

constexpr uint8 PhaseProfileProbe = 0x01;
constexpr uint8 PhaseInitial = 0x02;
constexpr uint8 PhaseRecurring = 0x04;
constexpr uint8 PhaseAggressive = 0x08;

bool IsLegalWardenEvidenceClass(WardenCheckType type,
    WardenEvidenceClass evidenceClass);
bool IsActionableEvidenceClass(WardenEvidenceClass evidenceClass);

struct TimingCheckProfile
{
    uint32 checkId = 0;
};

struct MpqCheckProfile
{
    uint32 checkId = 0;
    std::string path;
    Digest20 expectedSha1{};
};

struct LuaCheckProfile
{
    uint32 checkId = 0;
    std::string query;
    std::string expectedText;
};

struct MemCheckProfile
{
    uint32 checkId = 0;
    // The module ABI pools a printable process-module name such as Wow.exe;
    // this is not the 32-byte SHA-256 identity used by ModuleUse.
    Bytes moduleIdentifier;
    uint64 addressOrRva = 0;
    uint32 length = 0;
    Bytes expectedBytes;
};

using WardenCheckPayload = std::variant<TimingCheckProfile,
    MpqCheckProfile, LuaCheckProfile, MemCheckProfile>;

struct WardenCheckDefinition
{
    uint32 sortOrder = 0;
    WardenEvidenceClass evidenceClass =
        WardenEvidenceClass::ProtocolHealth;
    uint8 phaseMask = 0;
    WardenAddressKind addressKind = WardenAddressKind::None;
    WardenCheckPayload payload;
};

uint32 GetWardenCheckId(WardenCheckDefinition const& definition);
WardenCheckType GetWardenCheckType(WardenCheckDefinition const& definition);
bool IsConfirmationEligible(WardenCheckDefinition const& definition);

struct WardenProfileKey
{
    uint32 build = 0;
    WardenArchitecture architecture = WardenArchitecture::Unclassified;
    std::array<char, 4> locale{};
    ClientVariant variant = ClientVariant::Unclassified;
};

struct WardenCheckProfile
{
    WardenProfileKey key;
    uint32 totalRows = 0;
    std::vector<WardenCheckDefinition> checks;
};

/** Full-width untrusted SQL projection, retained until validation succeeds. */
struct WardenCheckRowInput
{
    uint32 build = 0;
    std::string architectureHex;
    std::string variantHex;
    std::string localeHex;
    uint32 checkId = 0;
    uint32 type = 0;
    uint32 enabled = 0;
    uint32 sortOrder = 0;
    uint32 evidenceClass = 0;
    uint32 phaseMask = 0;
    uint32 addressKind = 0;
    std::string moduleHex;
    uint64 address = 0;
    uint32 length = 0;
    std::string requestHex;
    std::string expectedHex;
};

enum class CheckCatalogValidation : uint8
{
    Valid,
    InvalidHex,
    InvalidBuild,
    InvalidArchitecture,
    InvalidLocale,
    InvalidVariant,
    InvalidId,
    InvalidType,
    InvalidEnabled,
    InvalidSortOrder,
    InvalidEvidenceClass,
    InvalidPhase,
    InvalidAddressKind,
    InvalidProfilePhase,
    ActionableProfileProbe,
    IllegalTypeEvidenceClass,
    InvalidUnusedField,
    InvalidRequest,
    InvalidExpectedBytes,
    InvalidLength,
    MissingModuleIdentifier,
    InvalidModuleIdentifier,
    InvalidAddress,
    DuplicateId,
    DuplicateSortOrder,
    EmptyCatalog,
    EmptyProfile,
    PlanPreflightFailed
};

struct WardenCheckDiagnostic
{
    CheckCatalogValidation validation = CheckCatalogValidation::Valid;
    WardenProfileKey profile;
    uint32 checkId = 0;
};

class WardenCheckCatalog
{
public:
    WardenCheckProfile const* FindProfileExact(
        WardenProfileKey const& key) const;
    std::vector<WardenCheckProfile> const& Profiles() const;

private:
    friend class WardenCheckCatalogBuilder;
#ifdef MANGOS_WARDEN_TEST_ACCESS
    friend class WardenCheckCatalogTestAccess;
#endif
    std::vector<WardenCheckProfile> m_profiles;
};

class WardenCheckCatalogBuilder
{
public:
    CheckCatalogValidation Add(WardenCheckRowInput const& input,
        WardenCheckDiagnostic& diagnostic);
    CheckCatalogValidation Build(WardenCheckCatalog& output,
        WardenCheckDiagnostic& diagnostic) const;

private:
    struct PendingRow
    {
        WardenProfileKey key;
        bool enabled = false;
        WardenCheckDefinition definition;
    };
    std::vector<PendingRow> m_rows;
};
}

#endif
