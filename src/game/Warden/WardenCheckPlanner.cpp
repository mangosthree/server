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

#include "WardenCheckPlanner.h"
#include "WardenPacketCodec.h"

#include <openssl/crypto.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace
{
uint8 PurposePhase(warden::CheckPlanPurpose purpose)
{
    switch (purpose)
    {
        case warden::CheckPlanPurpose::ProfileProbe:
            return warden::PhaseProfileProbe;
        case warden::CheckPlanPurpose::Initial:
            return warden::PhaseInitial;
        case warden::CheckPlanPurpose::Recurring:
            return warden::PhaseRecurring;
        case warden::CheckPlanPurpose::AggressiveImmediate:
        case warden::CheckPlanPurpose::AggressiveRecurring:
            return warden::PhaseAggressive;
        case warden::CheckPlanPurpose::Confirmation:
            return 0;
    }
    return 0;
}

bool AddChecked(std::size_t value, std::size_t& total)
{
    if (value > std::numeric_limits<std::size_t>::max() - total)
    {
        total = std::numeric_limits<std::size_t>::max();
        return false;
    }
    total += value;
    return true;
}

void Cleanse(std::vector<warden::Bytes>& results)
{
    for (warden::Bytes& result : results)
    {
        if (!result.empty())
            OPENSSL_cleanse(result.data(), result.size());
        result.clear();
    }
    results.clear();
}

bool Matches(std::vector<warden::Bytes> const& actual,
    std::vector<warden::Bytes> const& expected)
{
    return actual == expected;
}

bool IsExactProbeProfile(warden::WardenCheckProfile const& profile)
{
    if (profile.checks.size() != 3u)
        return false;

    std::vector<uint64> addresses;
    std::vector<uint32> lengths;
    for (warden::WardenCheckDefinition const& check : profile.checks)
    {
        if (check.phaseMask != warden::PhaseProfileProbe ||
            check.addressKind !=
                warden::WardenAddressKind::ModuleRelativeRva ||
            warden::GetWardenCheckType(check) !=
                warden::WardenCheckType::Mem ||
            warden::IsActionableEvidenceClass(check.evidenceClass))
            return false;
        warden::MemCheckProfile const& mem =
            std::get<warden::MemCheckProfile>(check.payload);
        warden::Bytes const syntheticIdentifier =
            {0x57, 0x6F, 0x77, 0x2E, 0x65, 0x78, 0x65}; // Wow.exe
        if (mem.moduleIdentifier != syntheticIdentifier ||
            !mem.expectedBytes.empty())
            return false;
        addresses.push_back(mem.addressOrRva);
        lengths.push_back(mem.length);
    }

    if (profile.key.architecture == warden::WardenArchitecture::X86)
        return addresses == std::vector<uint64>(
            {0x00007F7A, 0x00088FAE, 0x000895CA}) &&
            lengths == std::vector<uint32>({5, 1, 7});
    if (profile.key.architecture == warden::WardenArchitecture::X64)
        return addresses == std::vector<uint64>(
            {0x000AB76F, 0x000AABAB, 0x000AA6D3}) &&
            lengths == std::vector<uint32>({5, 2, 2});
    return false;
}
}

namespace warden
{
WardenCheckPlanner::WardenCheckPlanner(WardenCheckProfile const& profile)
    : m_profile(profile)
{
}

CheckPlanValidation WardenCheckPlanner::Build(CheckPlanPurpose purpose,
    uint32 requestId, CheckPlan& output, uint32 confirmationCheckId,
    WardenCheckPlanBudget* budget) const
{
    output = {};
    if (budget)
        *budget = {};

    bool const probeProfile =
        m_profile.key.variant == ClientVariant::Unclassified;
    if ((purpose == CheckPlanPurpose::ProfileProbe) != probeProfile)
        return CheckPlanValidation::InvalidProfilePurpose;
    if (probeProfile && !IsExactProbeProfile(m_profile))
        return CheckPlanValidation::InvalidProfilePurpose;
    if (!probeProfile && m_profile.key.variant != ClientVariant::Stock &&
        m_profile.key.variant != ClientVariant::Grunt)
        return CheckPlanValidation::InvalidProfilePurpose;

    CheckPlan staged;
    staged.requestId = requestId;
    staged.purpose = purpose;
    staged.profileKey = m_profile.key;

    if (purpose == CheckPlanPurpose::Confirmation)
    {
        auto const check = std::find_if(m_profile.checks.begin(),
            m_profile.checks.end(), [confirmationCheckId](
                WardenCheckDefinition const& candidate)
            {
                return GetWardenCheckId(candidate) == confirmationCheckId;
            });
        if (check == m_profile.checks.end() ||
            !IsConfirmationEligible(*check))
            return CheckPlanValidation::InvalidConfirmation;
        staged.checks.push_back(*check);
    }
    else
    {
        uint8 const phase = PurposePhase(purpose);
        for (WardenCheckDefinition const& check : m_profile.checks)
        {
            if ((check.phaseMask & phase) == 0)
                continue;
            if ((purpose == CheckPlanPurpose::AggressiveImmediate ||
                    purpose == CheckPlanPurpose::AggressiveRecurring) &&
                !IsActionableEvidenceClass(check.evidenceClass))
                continue;
            staged.checks.push_back(check);
        }
    }

    if (staged.checks.empty())
        return CheckPlanValidation::EmptyPlan;

    WardenCheckPlanBudget inspected;
    CheckPlanValidation const validation = InspectCheckPlan(staged, inspected);
    if (budget)
        *budget = inspected;
    if (validation != CheckPlanValidation::Valid)
        return validation;

    output = std::move(staged);
    return CheckPlanValidation::Valid;
}

CheckPlanValidation InspectCheckPlan(CheckPlan const& plan,
    WardenCheckPlanBudget& budget)
{
    budget = {};
    if (plan.checks.empty())
        return CheckPlanValidation::EmptyPlan;

    bool valid = true;
    for (WardenCheckDefinition const& check : plan.checks)
    {
        switch (GetWardenCheckType(check))
        {
            case WardenCheckType::Timing:
                valid = AddChecked(1, budget.requestBody) && valid;
                valid = AddChecked(4, budget.maximumResultBody) && valid;
                break;
            case WardenCheckType::Mpq:
            {
                MpqCheckProfile const& profile =
                    std::get<MpqCheckProfile>(check.payload);
                valid = AddChecked(2 + profile.path.size(),
                    budget.requestBody) && valid;
                valid = AddChecked(1 + profile.expectedSha1.size(),
                    budget.maximumResultBody) && valid;
                break;
            }
            case WardenCheckType::Lua:
            {
                LuaCheckProfile const& profile =
                    std::get<LuaCheckProfile>(check.payload);
                valid = AddChecked(2 + profile.query.size(),
                    budget.requestBody) && valid;
                valid = AddChecked(2 + profile.expectedText.size(),
                    budget.maximumResultBody) && valid;
                break;
            }
            case WardenCheckType::Mem:
            {
                MemCheckProfile const& profile =
                    std::get<MemCheckProfile>(check.payload);
                std::size_t const addressBytes =
                    plan.profileKey.architecture == WardenArchitecture::X64 ?
                    8u : 4u;
                valid = AddChecked(2 + addressBytes +
                    profile.moduleIdentifier.size(), budget.requestBody) &&
                    valid;
                valid = AddChecked(1 + std::max<std::size_t>(profile.length,
                    profile.expectedBytes.size()),
                    budget.maximumResultBody) && valid;
                break;
            }
        }
    }

    if (!valid || budget.requestBody > MaxEncryptedServerBody)
        return CheckPlanValidation::RequestBodyTooLarge;
    if (budget.maximumResultBody > MaxPlannedCheckResultBody)
        return CheckPlanValidation::TransportResultBodyTooLarge;
    return CheckPlanValidation::Valid;
}

ClientVariant ClassifyProfileProbe(WardenArchitecture architecture,
    std::vector<Bytes>& results)
{
    ClientVariant classification = ClientVariant::Unclassified;
    uint32 matches = 0;
    auto consider = [&results, &classification, &matches](
        ClientVariant variant, std::vector<Bytes> const& expected)
    {
        if (Matches(results, expected))
        {
            classification = variant;
            ++matches;
        }
    };

    if (architecture == WardenArchitecture::X86)
    {
        consider(ClientVariant::Stock,
            {{0xE8, 0xB1, 0xED, 0xFF, 0xFF}, {0x74},
                {0x8B, 0x55, 0x0C, 0x83, 0xFA, 0x02, 0x75}});
        consider(ClientVariant::Grunt,
            {{0xB8, 0x01, 0x00, 0x00, 0x00}, {0xEB},
                {0xBA, 0x00, 0x00, 0x00, 0x00, 0x90, 0xEB}});
    }
    else if (architecture == WardenArchitecture::X64)
    {
        consider(ClientVariant::Stock,
            {{0x74, 0x08, 0x41, 0x8B, 0xD5}, {0x74, 0x1A},
                {0x74, 0x10}});
        consider(ClientVariant::Grunt,
            {{0x90, 0x90, 0x31, 0xD2, 0x90}, {0xEB, 0x1A},
                {0x74, 0x10}});
        consider(ClientVariant::LegacyGrunt,
            {{0x90, 0x90, 0x31, 0xD2, 0x90}, {0x74, 0x1A},
                {0xEB, 0x10}});
    }

    Cleanse(results);
    return matches == 1 ? classification : ClientVariant::Unclassified;
}
}
