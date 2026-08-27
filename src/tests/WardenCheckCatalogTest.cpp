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

#include "WardenCheckCatalog.h"
#include "WardenCheckCatalogLoader.h"
#include "WardenCheckFixtures.h"

#include <limits>
#include <vector>

namespace
{
warden::CheckCatalogValidation AddOne(
    warden::WardenCheckRowInput const& row)
{
    warden::WardenCheckCatalogBuilder builder;
    warden::WardenCheckDiagnostic diagnostic;
    return builder.Add(row, diagnostic);
}

warden::CheckCatalogValidation BuildRows(
    std::vector<warden::WardenCheckRowInput> const& rows,
    warden::WardenCheckCatalog& catalog)
{
    warden::WardenCheckCatalogBuilder builder;
    warden::WardenCheckDiagnostic diagnostic;
    for (warden::WardenCheckRowInput const& row : rows)
    {
        warden::CheckCatalogValidation const result =
            builder.Add(row, diagnostic);
        if (result != warden::CheckCatalogValidation::Valid)
            return result;
    }
    return builder.Build(catalog, diagnostic);
}

warden::CheckCatalogValidation BuildRows(
    std::vector<warden::WardenCheckRowInput> const& rows)
{
    warden::WardenCheckCatalog catalog;
    return BuildRows(rows, catalog);
}
}

TEST(WardenCheckCatalog_publishes_only_exact_cata_profile_keys)
{
    warden::WardenCheckCatalog const catalog =
        warden::test::BuildX86Catalog();
    CHECK_EQ(catalog.Profiles().size(), size_t(3));

    warden::WardenProfileKey const stock = {15595,
        warden::WardenArchitecture::X86, warden::test::EnUsLocale(),
        warden::ClientVariant::Stock};
    REQUIRE(catalog.FindProfileExact(stock) != nullptr);
    CHECK(catalog.FindProfileExact({15594, warden::WardenArchitecture::X86,
        warden::test::EnUsLocale(), warden::ClientVariant::Stock}) == nullptr);
    CHECK(catalog.FindProfileExact({15595, warden::WardenArchitecture::X64,
        warden::test::EnUsLocale(), warden::ClientVariant::Stock}) == nullptr);
    CHECK(catalog.FindProfileExact({15595, warden::WardenArchitecture::X86,
        {{'e', 'n', 'G', 'B'}}, warden::ClientVariant::Stock}) == nullptr);
    CHECK(catalog.FindProfileExact({15595, warden::WardenArchitecture::X86,
        warden::test::EnUsLocale(), warden::ClientVariant::Grunt}) != nullptr);
}

TEST(WardenCheckCatalog_rejects_unknown_identity_and_wildcards)
{
    warden::WardenCheckRowInput row =
        warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
            warden::ClientVariant::Stock)[1];
    row.build = 15594;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidBuild);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[1];
    row.architectureHex = "756E6B";
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidArchitecture);
    row.architectureHex = "78313238";
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidArchitecture);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[1];
    row.localeHex = "2A2A2A2A";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLocale);
    row.localeHex = "656E55";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLocale);
    row.localeHex = "65005553";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLocale);
    row.localeHex = "7A7A5A5A";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidLocale);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[1];
    row.variantHex = "2A";
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidVariant);
}

TEST(WardenCheckCatalog_never_makes_legacy_grunt_selectable)
{
    warden::WardenCheckRowInput row =
        warden::test::ClassifiedRows(warden::WardenArchitecture::X64,
            warden::ClientVariant::LegacyGrunt)[1];
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidVariant);
}

TEST(WardenCheckCatalog_rejects_unknown_scalar_contracts_before_narrowing)
{
    warden::WardenCheckRowInput row =
        warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
            warden::ClientVariant::Stock)[1];
    row.type = 0xFFFFFFFFu;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidType);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[1];
    row.evidenceClass = 0x100;
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidEvidenceClass);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[1];
    row.phaseMask = 0;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPhase);
    row.phaseMask = 0x10;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPhase);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[1];
    row.addressKind = 0x100;
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidAddressKind);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[1];
    row.sortOrder = 0x10000;
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidSortOrder);
}

TEST(WardenCheckCatalog_rejects_non_textual_module_identifiers)
{
    warden::WardenCheckRowInput row =
        warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
            warden::ClientVariant::Stock)[2];
    row.moduleHex =
        "000102030405060708090A0B0C0D0E0F"
        "101112131415161718191A1B1C1D1E1F";
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidModuleIdentifier);

    row.moduleHex = "576F772E1F657865";
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidModuleIdentifier);
}

TEST(WardenCheckCatalog_profile_probe_is_exclusive_unclassified_and_non_actionable)
{
    warden::WardenCheckRowInput row =
        warden::test::ProfileProbeRows(warden::WardenArchitecture::X86)[0];
    row.variantHex = warden::test::VariantHex(warden::ClientVariant::Stock);
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidProfilePhase);

    row = warden::test::ProfileProbeRows(
        warden::WardenArchitecture::X86)[0];
    row.phaseMask |= warden::PhaseInitial;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidPhase);

    row = warden::test::ProfileProbeRows(
        warden::WardenArchitecture::X86)[0];
    row.evidenceClass = static_cast<uint32>(
        warden::WardenEvidenceClass::IntegrityInvariant);
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::ActionableProfileProbe);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[1];
    row.variantHex =
        warden::test::VariantHex(warden::ClientVariant::Unclassified);
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::InvalidProfilePhase);
}

TEST(WardenCheckCatalog_rejects_duplicate_profile_id_and_order)
{
    std::vector<warden::WardenCheckRowInput> rows =
        warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
            warden::ClientVariant::Stock);
    rows[2].checkId = rows[1].checkId;
    CHECK(BuildRows(rows) == warden::CheckCatalogValidation::DuplicateId);

    rows = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock);
    rows[2].sortOrder = rows[1].sortOrder;
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::DuplicateSortOrder);
}

TEST(WardenCheckCatalog_rejects_empty_catalogue)
{
    CHECK(BuildRows({}) == warden::CheckCatalogValidation::EmptyCatalog);
}

TEST(WardenCheckCatalog_rejects_invalid_address_kind_combinations)
{
    warden::WardenCheckRowInput row =
        warden::test::ProfileProbeRows(warden::WardenArchitecture::X86)[0];
    row.moduleHex.clear();
    CHECK(AddOne(row) ==
        warden::CheckCatalogValidation::MissingModuleIdentifier);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[0];
    row.address = 1;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidAddress);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
        warden::ClientVariant::Stock)[1];
    row.address = uint64(1) << 32;
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidAddress);

    row = warden::test::ClassifiedRows(warden::WardenArchitecture::X64,
        warden::ClientVariant::Stock)[1];
    row.address = uint64(0x0000800000000000);
    CHECK(AddOne(row) == warden::CheckCatalogValidation::InvalidAddress);
}

TEST(WardenCheckCatalog_rejects_catalogues_that_fail_shared_size_preflight)
{
    std::vector<warden::WardenCheckRowInput> rows =
        warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
            warden::ClientVariant::Stock);
    warden::WardenCheckRowInput prototype = rows[1];
    prototype.length = 255;
    prototype.expectedHex.assign(510, 'A');
    for (uint32 index = 0; index < 41; ++index)
    {
        prototype.checkId = 10000 + index;
        prototype.sortOrder = 100 + index;
        prototype.address = 0x00500000 + index * 0x100;
        rows.push_back(prototype);
    }
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::PlanPreflightFailed);
}

TEST(WardenCheckCatalog_rejects_request_preflight_overflow)
{
    std::vector<warden::WardenCheckRowInput> rows =
        warden::test::ClassifiedRows(warden::WardenArchitecture::X86,
            warden::ClientVariant::Stock);
    warden::WardenCheckRowInput prototype = rows[1];
    prototype.length = 1;
    prototype.expectedHex = "90";
    for (uint32 index = 0; index < 2000; ++index)
    {
        prototype.checkId = 20000 + index;
        prototype.sortOrder = 100 + index;
        prototype.address = 0x00600000 + index * 2;
        rows.push_back(prototype);
    }
    CHECK(BuildRows(rows) ==
        warden::CheckCatalogValidation::PlanPreflightFailed);
}

TEST(WardenCheckCatalog_build_is_atomic_on_validation_failure)
{
    warden::WardenCheckCatalog catalog = warden::test::BuildX86Catalog();
    REQUIRE(catalog.Profiles().size() == 3u);

    std::vector<warden::WardenCheckRowInput> rows =
        warden::test::CompleteX86Rows();
    rows[1].sortOrder = rows[0].sortOrder;
    CHECK(BuildRows(rows, catalog) ==
        warden::CheckCatalogValidation::DuplicateSortOrder);
    CHECK_EQ(catalog.Profiles().size(), size_t(3));
    CHECK(catalog.FindProfileExact({15595, warden::WardenArchitecture::X86,
        warden::test::EnUsLocale(), warden::ClientVariant::Stock}) != nullptr);
}

TEST(WardenCheckCatalogLoader_rejects_invalid_snapshot_counts_before_rows)
{
    warden::WardenCheckCatalogLoadTransaction empty;
    CHECK(empty.Begin(0) ==
        warden::WardenCheckCatalogLoadFailure::EmptyCatalogue);

    warden::WardenCheckCatalogLoadTransaction overflow;
    CHECK(overflow.Begin(uint64(std::numeric_limits<uint32>::max()) + 1) ==
        warden::WardenCheckCatalogLoadFailure::SourceCountOverflow);

    warden::WardenCheckCatalogLoadTransaction torn;
    REQUIRE(torn.Begin(3) == warden::WardenCheckCatalogLoadFailure::None);
    CHECK(torn.ObserveSourceCount(2) ==
        warden::WardenCheckCatalogLoadFailure::SourceCountInconsistent);
}

TEST(WardenCheckCatalogLoader_requires_full_module_profile_coverage)
{
    warden::WardenCheckCatalog const checks =
        warden::test::BuildSyntheticCheckCatalog();
    warden::WardenModuleCatalog const modules =
        warden::test::BuildSyntheticModuleCatalog();
    CHECK(warden::ValidateWardenCatalogCoverage(checks, modules) ==
        warden::WardenCheckCatalogLoadFailure::None);

    warden::WardenModuleCatalog const noModules;
    CHECK(warden::ValidateWardenCatalogCoverage(checks, noModules) ==
        warden::WardenCheckCatalogLoadFailure::ProfileWithoutModule);

    warden::WardenCheckCatalog const noChecks;
    CHECK(warden::ValidateWardenCatalogCoverage(noChecks, modules) ==
        warden::WardenCheckCatalogLoadFailure::ModuleWithoutProfile);
}

TEST(WardenCheckCatalogLoader_publishes_only_a_complete_valid_snapshot)
{
    std::vector<warden::WardenCheckRowInput> const rows =
        warden::test::CompleteX86Rows();
    warden::WardenCheckCatalogLoadTransaction transaction;
    REQUIRE(transaction.Begin(rows.size()) ==
        warden::WardenCheckCatalogLoadFailure::None);
    warden::WardenCheckDiagnostic diagnostic;
    for (warden::WardenCheckRowInput const& row : rows)
    {
        REQUIRE(transaction.ObserveSourceCount(rows.size()) ==
            warden::WardenCheckCatalogLoadFailure::None);
        REQUIRE(transaction.Add(row, diagnostic) ==
            warden::WardenCheckCatalogLoadFailure::None);
    }

    bool published = false;
    warden::WardenModuleCatalog const modules =
        warden::test::BuildSyntheticModuleCatalog();
    CHECK(transaction.Finish(modules,
        [&published](std::shared_ptr<warden::WardenCheckCatalog const> const& snapshot)
        {
            published = snapshot && snapshot->Profiles().size() == 3u;
            return published;
        }, diagnostic) == warden::WardenCheckCatalogLoadFailure::None);
    CHECK(published);
}

TEST(WardenCheckCatalogLoader_rejects_short_or_rejected_publication)
{
    std::vector<warden::WardenCheckRowInput> const rows =
        warden::test::CompleteX86Rows();
    warden::WardenCheckCatalogLoadTransaction shortRead;
    REQUIRE(shortRead.Begin(rows.size()) ==
        warden::WardenCheckCatalogLoadFailure::None);
    warden::WardenCheckDiagnostic diagnostic;
    REQUIRE(shortRead.ObserveSourceCount(rows.size()) ==
        warden::WardenCheckCatalogLoadFailure::None);
    REQUIRE(shortRead.Add(rows.front(), diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::None);
    CHECK(shortRead.Finish(warden::test::BuildSyntheticModuleCatalog(), {},
        diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::SourceCountMismatch);

    warden::WardenCheckCatalogLoadTransaction rejected;
    REQUIRE(rejected.Begin(rows.size()) ==
        warden::WardenCheckCatalogLoadFailure::None);
    for (warden::WardenCheckRowInput const& row : rows)
    {
        REQUIRE(rejected.ObserveSourceCount(rows.size()) ==
            warden::WardenCheckCatalogLoadFailure::None);
        REQUIRE(rejected.Add(row, diagnostic) ==
            warden::WardenCheckCatalogLoadFailure::None);
    }
    CHECK(rejected.Finish(warden::test::BuildSyntheticModuleCatalog(),
        [](std::shared_ptr<warden::WardenCheckCatalog const> const&)
        {
            return false;
        }, diagnostic) ==
        warden::WardenCheckCatalogLoadFailure::PublicationFailed);
}
