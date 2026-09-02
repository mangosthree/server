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

#include "Auth/BigNumber.h"
#include "WardenProtocol.h"
#include "WorldGatewayAuth.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

namespace
{
std::array<uint8, 40> LeadingZeroSessionKey()
{
    std::array<uint8, 40> key{};
    for (std::size_t index = 0; index < key.size() - 1; ++index)
        key[index] = uint8(index + 1);
    return key;
}

bool IsZero(warden::SessionKey const& key)
{
    return std::all_of(key.begin(), key.end(),
        [](uint8 value) { return value == 0; });
}
}

TEST(WardenAdmission_extracts_all_40_bytes_without_consuming_BigNumber)
{
    std::array<uint8, 40> const expected = LeadingZeroSessionKey();
    BigNumber retained;
    retained.SetBinary(expected.data(), int(expected.size()));

    warden::AdmissionData admission = BuildWardenAdmissionData(
        15595, "Win", "enGB", retained);
    REQUIRE(admission.IsAvailable());
    CHECK(admission.status == warden::AdmissionStatus::Available);
    CHECK_EQ(admission.build, uint32(15595));
    CHECK_STR(admission.clientOs.c_str(), "Win");
    CHECK_STR(admission.clientLocale.c_str(), "enGB");
    CHECK(admission.sessionKey == expected);

    std::array<uint8, 40> retainedBytes{};
    uint8 const* serialized = retained.AsByteArray(40);
    REQUIRE(serialized != nullptr);
    std::copy(serialized, serialized + retainedBytes.size(),
        retainedBytes.begin());
    CHECK(retainedBytes == expected);
}

TEST(WardenAdmission_classifies_locale_failures_before_key_extraction)
{
    BigNumber key;
    key.SetDword(7);

    warden::AdmissionData missing = BuildWardenAdmissionData(
        15595, "Win", "", key);
    CHECK(missing.status == warden::AdmissionStatus::MissingExactLocale);
    CHECK(missing.clientLocale.empty());
    CHECK(IsZero(missing.sessionKey));

    warden::AdmissionData malformed = BuildWardenAdmissionData(
        15595, "Win", "enus", key);
    CHECK(malformed.status ==
        warden::AdmissionStatus::UnsupportedExactLocale);
    CHECK(malformed.clientLocale.empty());
    CHECK(IsZero(malformed.sessionKey));

    warden::AdmissionData unknown = BuildWardenAdmissionData(
        15595, "Win", "zzZZ", key);
    CHECK(unknown.status ==
        warden::AdmissionStatus::UnsupportedExactLocale);
    CHECK_STR(unknown.clientLocale.c_str(), "zzZZ");
    CHECK(IsZero(unknown.sessionKey));

    warden::AdmissionData unmeasured = BuildWardenAdmissionData(
        15595, "Win", "itIT", key);
    CHECK(unmeasured.status ==
        warden::AdmissionStatus::UnsupportedExactLocale);
    CHECK_STR(unmeasured.clientLocale.c_str(), "itIT");
    CHECK(IsZero(unmeasured.sessionKey));
}

TEST(WardenAdmission_rejects_zero_or_oversized_session_keys)
{
    BigNumber zero;
    warden::AdmissionData zeroAdmission = BuildWardenAdmissionData(
        15595, "Win", "enUS", zero);
    CHECK(zeroAdmission.status ==
        warden::AdmissionStatus::SessionKeyUnavailable);
    CHECK_STR(zeroAdmission.clientLocale.c_str(), "enUS");
    CHECK(IsZero(zeroAdmission.sessionKey));

    std::array<uint8, 41> oversizedBytes{};
    oversizedBytes.back() = 1;
    BigNumber oversized;
    oversized.SetBinary(oversizedBytes.data(), int(oversizedBytes.size()));
    warden::AdmissionData oversizedAdmission = BuildWardenAdmissionData(
        15595, "Win", "enUS", oversized);
    CHECK(oversizedAdmission.status ==
        warden::AdmissionStatus::SessionKeyUnavailable);
    CHECK(IsZero(oversizedAdmission.sessionKey));
}

TEST(WardenAdmission_unavailable_profiles_follow_explicit_policy_only)
{
    std::array<warden::AdmissionStatus, 3> const failures = {{
        warden::AdmissionStatus::MissingExactLocale,
        warden::AdmissionStatus::UnsupportedExactLocale,
        warden::AdmissionStatus::SessionKeyUnavailable
    }};
    for (warden::AdmissionStatus failure : failures)
    {
        warden::WardenConfiguration configuration;
        configuration.enforcementMode =
            warden::WardenEnforcementMode::Observe;
        CHECK(ClassifyWardenAdmission(failure, configuration) ==
            WardenAdmissionDisposition::AdmitWithoutWarden);

        configuration.enforcementMode = warden::WardenEnforcementMode::Kick;
        configuration.requireExactProfile = true;
        CHECK(ClassifyWardenAdmission(failure, configuration) ==
            WardenAdmissionDisposition::Reject);

        configuration.requireExactProfile = false;
        CHECK(ClassifyWardenAdmission(failure, configuration) ==
            WardenAdmissionDisposition::AdmitWithoutWarden);
    }

    warden::WardenConfiguration configuration;
    CHECK(ClassifyWardenAdmission(warden::AdmissionStatus::Available,
        configuration) == WardenAdmissionDisposition::Start);
}
