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

#include "WardenCheckCatalog.h"
#include "WardenCheckPlanner.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace
{
bool SameKey(warden::WardenProfileKey const& left,
    warden::WardenProfileKey const& right)
{
    return left.build == right.build &&
        left.architecture == right.architecture &&
        left.locale == right.locale && left.variant == right.variant;
}

bool DecodeHex(std::string const& input, warden::Bytes& output)
{
    output.clear();
    if ((input.size() & 1u) != 0)
        return false;
    output.reserve(input.size() / 2);
    auto nibble = [](char value) -> int
    {
        if (value >= '0' && value <= '9')
            return value - '0';
        if (value >= 'A' && value <= 'F')
            return value - 'A' + 10;
        if (value >= 'a' && value <= 'f')
            return value - 'a' + 10;
        return -1;
    };
    for (std::size_t offset = 0; offset < input.size(); offset += 2)
    {
        int const high = nibble(input[offset]);
        int const low = nibble(input[offset + 1]);
        if (high < 0 || low < 0)
        {
            output.clear();
            return false;
        }
        output.push_back(uint8((high << 4) | low));
    }
    return true;
}

bool DecodeText(std::string const& input, std::string& output)
{
    warden::Bytes bytes;
    if (!DecodeHex(input, bytes) ||
        std::any_of(bytes.begin(), bytes.end(),
            [](uint8 value) { return value == 0; }))
        return false;
    output.assign(bytes.begin(), bytes.end());
    return true;
}

bool DecodeArchitecture(std::string const& input,
    warden::WardenArchitecture& output)
{
    std::string value;
    if (!DecodeText(input, value))
        return false;
    if (value == "x86")
        output = warden::WardenArchitecture::X86;
    else if (value == "x64")
        output = warden::WardenArchitecture::X64;
    else
        return false;
    return true;
}

bool DecodeVariant(std::string const& input, warden::ClientVariant& output)
{
    std::string value;
    if (!DecodeText(input, value))
        return false;
    if (value == "unclassified")
        output = warden::ClientVariant::Unclassified;
    else if (value == "stock")
        output = warden::ClientVariant::Stock;
    else if (value == "grunt")
        output = warden::ClientVariant::Grunt;
    else
        return false;
    return true;
}

bool DecodeLocale(std::string const& input, std::array<char, 4>& output)
{
    warden::Bytes bytes;
    if (!DecodeHex(input, bytes) || bytes.size() != output.size() ||
        std::any_of(bytes.begin(), bytes.end(),
            [](uint8 value)
            {
                return !std::isalpha(static_cast<unsigned char>(value));
            }))
        return false;

    std::string const locale(bytes.begin(), bytes.end());
    std::array<char const*, 14> const supported = {{
        "deDE", "enCN", "enGB", "enTW", "enUS", "esES", "esMX",
        "frFR", "koKR", "ptBR", "ptPT", "ruRU", "zhCN", "zhTW"}};
    if (std::find_if(supported.begin(), supported.end(),
        [&locale](char const* candidate) { return locale == candidate; }) ==
        supported.end())
        return false;

    std::copy(bytes.begin(), bytes.end(), output.begin());
    return true;
}

bool IsKnownType(uint32 value)
{
    return value <= static_cast<uint32>(warden::WardenCheckType::Mem);
}

bool IsKnownEvidenceClass(uint32 value)
{
    return value <=
        static_cast<uint32>(warden::WardenEvidenceClass::Corroboration);
}

bool IsKnownAddressKind(uint32 value)
{
    return value <=
        static_cast<uint32>(warden::WardenAddressKind::AbsoluteVa);
}

bool IsAddressValid(warden::WardenArchitecture architecture, uint64 address,
    uint32 length)
{
    if (!address || !length || length > 255)
        return false;
    uint64 const maximum = architecture == warden::WardenArchitecture::X86 ?
        uint64(std::numeric_limits<uint32>::max()) :
        uint64(0x00007FFFFFFFFFFF);
    return address <= maximum && uint64(length - 1) <= maximum - address;
}

void SetDiagnostic(warden::WardenCheckDiagnostic& diagnostic,
    warden::CheckCatalogValidation validation,
    warden::WardenProfileKey const& key, uint32 checkId)
{
    diagnostic.validation = validation;
    diagnostic.profile = key;
    diagnostic.checkId = checkId;
}
}

namespace warden
{
bool IsLegalWardenEvidenceClass(WardenCheckType type,
    WardenEvidenceClass evidenceClass)
{
    switch (type)
    {
        case WardenCheckType::Timing:
            return evidenceClass == WardenEvidenceClass::ProtocolHealth;
        case WardenCheckType::Lua:
            return evidenceClass == WardenEvidenceClass::Corroboration;
        case WardenCheckType::Mpq:
            return evidenceClass == WardenEvidenceClass::IntegrityInvariant ||
                evidenceClass == WardenEvidenceClass::Corroboration;
        case WardenCheckType::Mem:
            return evidenceClass == WardenEvidenceClass::IntegrityInvariant ||
                evidenceClass == WardenEvidenceClass::ThreatSignature ||
                evidenceClass == WardenEvidenceClass::Corroboration;
    }
    return false;
}

bool IsActionableEvidenceClass(WardenEvidenceClass evidenceClass)
{
    return evidenceClass == WardenEvidenceClass::IntegrityInvariant ||
        evidenceClass == WardenEvidenceClass::ThreatSignature;
}

uint32 GetWardenCheckId(WardenCheckDefinition const& definition)
{
    return std::visit([](auto const& payload) { return payload.checkId; },
        definition.payload);
}

WardenCheckType GetWardenCheckType(WardenCheckDefinition const& definition)
{
    if (std::holds_alternative<TimingCheckProfile>(definition.payload))
        return WardenCheckType::Timing;
    if (std::holds_alternative<LuaCheckProfile>(definition.payload))
        return WardenCheckType::Lua;
    if (std::holds_alternative<MpqCheckProfile>(definition.payload))
        return WardenCheckType::Mpq;
    return WardenCheckType::Mem;
}

bool IsConfirmationEligible(WardenCheckDefinition const& definition)
{
    return GetWardenCheckType(definition) != WardenCheckType::Timing &&
        IsActionableEvidenceClass(definition.evidenceClass);
}

WardenCheckProfile const* WardenCheckCatalog::FindProfileExact(
    WardenProfileKey const& key) const
{
    auto const profile = std::find_if(m_profiles.begin(), m_profiles.end(),
        [&key](WardenCheckProfile const& candidate)
        {
            return SameKey(candidate.key, key);
        });
    return profile == m_profiles.end() ? nullptr : &*profile;
}

std::vector<WardenCheckProfile> const& WardenCheckCatalog::Profiles() const
{
    return m_profiles;
}

CheckCatalogValidation WardenCheckCatalogBuilder::Add(
    WardenCheckRowInput const& input, WardenCheckDiagnostic& diagnostic)
{
    WardenProfileKey key;
    key.build = input.build;
    if (input.build != 15595)
    {
        SetDiagnostic(diagnostic, CheckCatalogValidation::InvalidBuild, key,
            input.checkId);
        return diagnostic.validation;
    }
    if (!DecodeArchitecture(input.architectureHex, key.architecture))
    {
        SetDiagnostic(diagnostic, CheckCatalogValidation::InvalidArchitecture,
            key, input.checkId);
        return diagnostic.validation;
    }
    if (!DecodeLocale(input.localeHex, key.locale))
    {
        SetDiagnostic(diagnostic, CheckCatalogValidation::InvalidLocale, key,
            input.checkId);
        return diagnostic.validation;
    }
    if (!DecodeVariant(input.variantHex, key.variant))
    {
        SetDiagnostic(diagnostic, CheckCatalogValidation::InvalidVariant, key,
            input.checkId);
        return diagnostic.validation;
    }

    auto fail = [&diagnostic, &key, &input](CheckCatalogValidation validation)
    {
        SetDiagnostic(diagnostic, validation, key, input.checkId);
        return validation;
    };
    if (!input.checkId)
        return fail(CheckCatalogValidation::InvalidId);
    if (!IsKnownType(input.type))
        return fail(CheckCatalogValidation::InvalidType);
    if (input.enabled > 1)
        return fail(CheckCatalogValidation::InvalidEnabled);
    if (!input.sortOrder ||
        input.sortOrder > std::numeric_limits<uint16>::max())
        return fail(CheckCatalogValidation::InvalidSortOrder);
    if (!IsKnownEvidenceClass(input.evidenceClass))
        return fail(CheckCatalogValidation::InvalidEvidenceClass);
    constexpr uint32 knownPhases = PhaseProfileProbe | PhaseInitial |
        PhaseRecurring | PhaseAggressive;
    if (!input.phaseMask || (input.phaseMask & ~knownPhases) != 0 ||
        ((input.phaseMask & PhaseProfileProbe) != 0 &&
            input.phaseMask != PhaseProfileProbe))
        return fail(CheckCatalogValidation::InvalidPhase);
    if (!IsKnownAddressKind(input.addressKind))
        return fail(CheckCatalogValidation::InvalidAddressKind);

    bool const probe = input.phaseMask == PhaseProfileProbe;
    if (probe != (key.variant == ClientVariant::Unclassified))
        return fail(CheckCatalogValidation::InvalidProfilePhase);

    WardenCheckType const type = static_cast<WardenCheckType>(input.type);
    WardenEvidenceClass const evidenceClass =
        static_cast<WardenEvidenceClass>(input.evidenceClass);
    if (probe && IsActionableEvidenceClass(evidenceClass))
        return fail(CheckCatalogValidation::ActionableProfileProbe);
    if (!IsLegalWardenEvidenceClass(type, evidenceClass))
        return fail(CheckCatalogValidation::IllegalTypeEvidenceClass);
    if ((input.phaseMask & PhaseAggressive) != 0 &&
        !IsActionableEvidenceClass(evidenceClass))
        return fail(CheckCatalogValidation::IllegalTypeEvidenceClass);

    WardenAddressKind const addressKind =
        static_cast<WardenAddressKind>(input.addressKind);
    WardenCheckDefinition definition;
    definition.sortOrder = input.sortOrder;
    definition.evidenceClass = evidenceClass;
    definition.phaseMask = static_cast<uint8>(input.phaseMask);
    definition.addressKind = addressKind;

    Bytes expected;
    if (!DecodeHex(input.expectedHex, expected))
        return fail(CheckCatalogValidation::InvalidHex);

    switch (type)
    {
        case WardenCheckType::Timing:
            if (addressKind != WardenAddressKind::None || input.address ||
                input.length || !input.moduleHex.empty() ||
                !input.requestHex.empty() || !expected.empty())
                return fail(CheckCatalogValidation::InvalidAddress);
            definition.payload = TimingCheckProfile{input.checkId};
            break;
        case WardenCheckType::Mem:
        {
            if (addressKind == WardenAddressKind::None)
                return fail(CheckCatalogValidation::InvalidAddressKind);
            if (!IsAddressValid(key.architecture, input.address, input.length))
                return fail(CheckCatalogValidation::InvalidAddress);
            if (!input.requestHex.empty())
                return fail(CheckCatalogValidation::InvalidUnusedField);
            Bytes moduleIdentifier;
            if (addressKind == WardenAddressKind::ModuleRelativeRva)
            {
                if (input.moduleHex.empty())
                    return fail(
                        CheckCatalogValidation::MissingModuleIdentifier);
                // PE RVAs stay 32-bit even when the selected image is x64;
                // the uint64 database field exists for absolute x64 VAs.
                if (input.address > std::numeric_limits<uint32>::max())
                    return fail(CheckCatalogValidation::InvalidAddress);
                if (!DecodeHex(input.moduleHex, moduleIdentifier) ||
                    moduleIdentifier.empty() ||
                    moduleIdentifier.size() > ModuleId{}.size())
                    return fail(
                        CheckCatalogValidation::InvalidModuleIdentifier);
            }
            else if (!input.moduleHex.empty())
            {
                return fail(CheckCatalogValidation::InvalidUnusedField);
            }
            if (probe)
            {
                if (!expected.empty())
                    return fail(CheckCatalogValidation::InvalidExpectedBytes);
            }
            else if (expected.size() != input.length)
            {
                return fail(CheckCatalogValidation::InvalidExpectedBytes);
            }
            definition.payload = MemCheckProfile{input.checkId,
                std::move(moduleIdentifier), input.address, input.length,
                std::move(expected)};
            break;
        }
        case WardenCheckType::Mpq:
        {
            if (addressKind != WardenAddressKind::None || input.address ||
                input.length || !input.moduleHex.empty())
                return fail(CheckCatalogValidation::InvalidUnusedField);
            std::string path;
            if (!DecodeText(input.requestHex, path) || path.empty() ||
                expected.size() != Digest20{}.size())
                return fail(CheckCatalogValidation::InvalidRequest);
            MpqCheckProfile profile;
            profile.checkId = input.checkId;
            profile.path = std::move(path);
            std::copy(expected.begin(), expected.end(),
                profile.expectedSha1.begin());
            definition.payload = std::move(profile);
            break;
        }
        case WardenCheckType::Lua:
        {
            if (addressKind != WardenAddressKind::None || input.address ||
                input.length || !input.moduleHex.empty())
                return fail(CheckCatalogValidation::InvalidUnusedField);
            std::string query;
            std::string expectedText;
            if (!DecodeText(input.requestHex, query) || query.empty() ||
                !DecodeText(input.expectedHex, expectedText) ||
                expectedText.empty())
                return fail(CheckCatalogValidation::InvalidRequest);
            definition.payload = LuaCheckProfile{input.checkId,
                std::move(query), std::move(expectedText)};
            break;
        }
    }

    for (PendingRow const& pending : m_rows)
    {
        if (!SameKey(pending.key, key))
            continue;
        if (GetWardenCheckId(pending.definition) == input.checkId)
            return fail(CheckCatalogValidation::DuplicateId);
        if (pending.definition.sortOrder == input.sortOrder)
            return fail(CheckCatalogValidation::DuplicateSortOrder);
    }

    m_rows.push_back({key, input.enabled != 0, std::move(definition)});
    diagnostic = {};
    return CheckCatalogValidation::Valid;
}

CheckCatalogValidation WardenCheckCatalogBuilder::Build(
    WardenCheckCatalog& output, WardenCheckDiagnostic& diagnostic) const
{
    if (m_rows.empty())
    {
        diagnostic.validation = CheckCatalogValidation::EmptyCatalog;
        return diagnostic.validation;
    }

    WardenCheckCatalog staged;
    for (PendingRow const& row : m_rows)
    {
        auto profile = std::find_if(staged.m_profiles.begin(),
            staged.m_profiles.end(), [&row](WardenCheckProfile const& candidate)
            {
                return SameKey(candidate.key, row.key);
            });
        if (profile == staged.m_profiles.end())
        {
            staged.m_profiles.push_back({row.key, 0, {}});
            profile = staged.m_profiles.end() - 1;
        }
        ++profile->totalRows;
        if (row.enabled)
            profile->checks.push_back(row.definition);
    }

    for (WardenCheckProfile& profile : staged.m_profiles)
    {
        if (profile.checks.empty())
        {
            SetDiagnostic(diagnostic, CheckCatalogValidation::EmptyProfile,
                profile.key, 0);
            return diagnostic.validation;
        }
        std::sort(profile.checks.begin(), profile.checks.end(),
            [](WardenCheckDefinition const& left,
                WardenCheckDefinition const& right)
            {
                return left.sortOrder < right.sortOrder;
            });

        WardenCheckPlanner planner(profile);
        CheckPlan plan;
        CheckPlanPurpose const purposes[] = {
            profile.key.variant == ClientVariant::Unclassified ?
                CheckPlanPurpose::ProfileProbe : CheckPlanPurpose::Initial,
            CheckPlanPurpose::Recurring,
            CheckPlanPurpose::AggressiveImmediate,
            CheckPlanPurpose::AggressiveRecurring};
        std::size_t const purposeCount =
            profile.key.variant == ClientVariant::Unclassified ? 1u : 4u;
        for (std::size_t index = 0; index < purposeCount; ++index)
        {
            CheckPlanValidation const validation =
                planner.Build(purposes[index], 1, plan);
            if (validation == CheckPlanValidation::EmptyPlan && index > 0)
                continue;
            if (validation != CheckPlanValidation::Valid)
            {
                SetDiagnostic(diagnostic,
                    CheckCatalogValidation::PlanPreflightFailed,
                    profile.key, 0);
                return diagnostic.validation;
            }
        }
    }

    output = std::move(staged);
    diagnostic = {};
    return CheckCatalogValidation::Valid;
}
}
