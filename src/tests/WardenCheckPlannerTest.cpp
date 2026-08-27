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

#include "WardenCheckFixtures.h"
#include "WardenCheckPlanner.h"
#include "WardenPacketCodec.h"

#include <algorithm>
#include <vector>

namespace
{
warden::WardenCheckProfile const* FindProfile(
    warden::WardenCheckCatalog const& catalog, warden::ClientVariant variant)
{
    return catalog.FindProfileExact({15595, warden::WardenArchitecture::X86,
        warden::test::EnUsLocale(), variant});
}

std::vector<uint32> CheckIds(warden::CheckPlan const& plan)
{
    std::vector<uint32> ids;
    for (warden::WardenCheckDefinition const& check : plan.checks)
        ids.push_back(warden::GetWardenCheckId(check));
    return ids;
}

bool Cleansed(std::vector<warden::Bytes> const& values)
{
    return values.empty() || std::all_of(values.begin(), values.end(),
        [](warden::Bytes const& value)
        {
            return std::all_of(value.begin(), value.end(),
                [](uint8 byte) { return byte == 0; });
        });
}
}

TEST(WardenCheckPlanner_builds_only_the_minimal_unclassified_profile_probe)
{
    warden::WardenCheckCatalog const catalog =
        warden::test::BuildX86Catalog();
    warden::WardenCheckProfile const* probe =
        FindProfile(catalog, warden::ClientVariant::Unclassified);
    REQUIRE(probe != nullptr);

    warden::WardenCheckPlanner planner(*probe);
    warden::CheckPlan plan;
    REQUIRE(planner.Build(warden::CheckPlanPurpose::ProfileProbe, 7, plan) ==
        warden::CheckPlanValidation::Valid);
    CHECK_EQ(plan.requestId, uint32(7));
    CHECK(plan.purpose == warden::CheckPlanPurpose::ProfileProbe);
    CHECK(plan.profileKey.variant == warden::ClientVariant::Unclassified);
    CHECK(CheckIds(plan) ==
        std::vector<uint32>({1001, 1002, 1003}));
    for (warden::WardenCheckDefinition const& check : plan.checks)
    {
        CHECK(check.phaseMask == warden::PhaseProfileProbe);
        CHECK(!warden::IsActionableEvidenceClass(check.evidenceClass));
    }
}

TEST(WardenCheckPlanner_rejects_probe_purpose_for_a_classified_profile)
{
    warden::WardenCheckCatalog const catalog =
        warden::test::BuildX86Catalog();
    warden::WardenCheckProfile const* stock =
        FindProfile(catalog, warden::ClientVariant::Stock);
    REQUIRE(stock != nullptr);

    warden::WardenCheckPlanner planner(*stock);
    warden::CheckPlan plan;
    CHECK(planner.Build(warden::CheckPlanPurpose::ProfileProbe, 1, plan) ==
        warden::CheckPlanValidation::InvalidProfilePurpose);
    CHECK(plan.checks.empty());
}

TEST(WardenCheckPlanner_selects_checks_by_exact_phase_and_preserves_order)
{
    warden::WardenCheckCatalog const catalog =
        warden::test::BuildX86Catalog();
    warden::WardenCheckProfile const* stock =
        FindProfile(catalog, warden::ClientVariant::Stock);
    REQUIRE(stock != nullptr);
    warden::WardenCheckPlanner planner(*stock);

    warden::CheckPlan plan;
    REQUIRE(planner.Build(warden::CheckPlanPurpose::Initial, 1, plan) ==
        warden::CheckPlanValidation::Valid);
    CHECK(CheckIds(plan) == std::vector<uint32>({2001, 2002}));
    CHECK(plan.profileKey.variant == warden::ClientVariant::Stock);

    REQUIRE(planner.Build(warden::CheckPlanPurpose::Recurring, 2, plan) ==
        warden::CheckPlanValidation::Valid);
    CHECK(CheckIds(plan) == std::vector<uint32>({2001, 2002, 2003}));

    REQUIRE(planner.Build(warden::CheckPlanPurpose::AggressiveImmediate, 3,
        plan) == warden::CheckPlanValidation::Valid);
    CHECK(CheckIds(plan) == std::vector<uint32>({2002}));
    REQUIRE(planner.Build(warden::CheckPlanPurpose::AggressiveRecurring, 4,
        plan) == warden::CheckPlanValidation::Valid);
    CHECK(CheckIds(plan) == std::vector<uint32>({2002}));
}

TEST(WardenCheckPlanner_confirmation_is_isolated_and_actionable)
{
    warden::WardenCheckCatalog const catalog =
        warden::test::BuildX86Catalog();
    warden::WardenCheckProfile const* stock =
        FindProfile(catalog, warden::ClientVariant::Stock);
    REQUIRE(stock != nullptr);
    warden::WardenCheckPlanner planner(*stock);

    warden::CheckPlan plan;
    REQUIRE(planner.Build(warden::CheckPlanPurpose::Confirmation, 9, plan,
        2002) == warden::CheckPlanValidation::Valid);
    CHECK(CheckIds(plan) == std::vector<uint32>({2002}));
    CHECK(plan.purpose == warden::CheckPlanPurpose::Confirmation);

    CHECK(planner.Build(warden::CheckPlanPurpose::Confirmation, 10, plan,
        2001) == warden::CheckPlanValidation::InvalidConfirmation);
    CHECK(plan.checks.empty());
    CHECK(planner.Build(warden::CheckPlanPurpose::Confirmation, 11, plan,
        9999) == warden::CheckPlanValidation::InvalidConfirmation);
    CHECK(plan.checks.empty());
}

TEST(WardenCheckPlanner_classifies_only_complete_x86_fingerprint_columns)
{
    std::vector<warden::Bytes> results =
        warden::test::X86StockFingerprint();
    CHECK(warden::ClassifyProfileProbe(warden::WardenArchitecture::X86,
        results) == warden::ClientVariant::Stock);
    CHECK(Cleansed(results));

    results = warden::test::X86GruntFingerprint();
    CHECK(warden::ClassifyProfileProbe(warden::WardenArchitecture::X86,
        results) == warden::ClientVariant::Grunt);
    CHECK(Cleansed(results));

    results = warden::test::X86StockFingerprint();
    results[1] = {0xEB};
    CHECK(warden::ClassifyProfileProbe(warden::WardenArchitecture::X86,
        results) == warden::ClientVariant::Unclassified);
    CHECK(Cleansed(results));
}

TEST(WardenCheckPlanner_recognizes_but_never_selects_legacy_grunt)
{
    std::vector<warden::Bytes> results =
        warden::test::X64StockFingerprint();
    CHECK(warden::ClassifyProfileProbe(warden::WardenArchitecture::X64,
        results) == warden::ClientVariant::Stock);
    CHECK(Cleansed(results));

    results = warden::test::X64GruntFingerprint();
    CHECK(warden::ClassifyProfileProbe(warden::WardenArchitecture::X64,
        results) == warden::ClientVariant::Grunt);
    CHECK(Cleansed(results));

    results = warden::test::X64LegacyGruntFingerprint();
    CHECK(warden::ClassifyProfileProbe(warden::WardenArchitecture::X64,
        results) == warden::ClientVariant::LegacyGrunt);
    CHECK(Cleansed(results));

    results = warden::test::X64GruntFingerprint();
    results[2] = {0xEB, 0x10};
    CHECK(warden::ClassifyProfileProbe(warden::WardenArchitecture::X64,
        results) == warden::ClientVariant::Unclassified);
    CHECK(Cleansed(results));
}

TEST(WardenCheckPlanner_rejects_wrong_probe_shape_and_cleanses_it)
{
    std::vector<warden::Bytes> results = {{0x74}};
    CHECK(warden::ClassifyProfileProbe(warden::WardenArchitecture::X64,
        results) == warden::ClientVariant::Unclassified);
    CHECK(Cleansed(results));

    results = warden::test::X86StockFingerprint();
    CHECK(warden::ClassifyProfileProbe(
        warden::WardenArchitecture::Unclassified, results) ==
        warden::ClientVariant::Unclassified);
    CHECK(Cleansed(results));
}

TEST(WardenCheckPlanner_fails_closed_when_result_preflight_exceeds_codec_budget)
{
    warden::WardenCheckCatalog const catalog =
        warden::test::BuildX86Catalog();
    warden::WardenCheckProfile const* exact =
        FindProfile(catalog, warden::ClientVariant::Stock);
    REQUIRE(exact != nullptr);
    warden::WardenCheckProfile oversized = *exact;
    warden::WardenCheckDefinition prototype = oversized.checks[1];
    warden::MemCheckProfile mem =
        std::get<warden::MemCheckProfile>(prototype.payload);
    mem.length = 255;
    mem.expectedBytes.assign(255, 0x90);
    for (uint32 index = 0; index < 41; ++index)
    {
        prototype.sortOrder = 100 + index;
        mem.checkId = 10000 + index;
        mem.addressOrRva = 0x00500000 + index * 0x100;
        prototype.payload = mem;
        oversized.checks.push_back(prototype);
    }

    warden::WardenCheckPlanner planner(oversized);
    warden::CheckPlan plan;
    warden::WardenCheckPlanBudget budget;
    CHECK(planner.Build(warden::CheckPlanPurpose::Initial, 1, plan, 0,
        &budget) ==
        warden::CheckPlanValidation::ResultBodyTooLarge);
    CHECK(plan.checks.empty());
}
