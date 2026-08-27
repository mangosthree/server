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

#include "WardenIncidentStore.h"

#include <limits>
#include <vector>

namespace
{
warden::WardenIncidentContext ValidIncident()
{
    warden::WardenIncidentContext context;
    context.accountId = 6;
    context.realmId = 1;
    context.clientBuild = 15595;
    context.architecture = warden::WardenArchitecture::X86;
    context.locale = {{'e', 'n', 'U', 'S'}};
    context.variant = warden::ClientVariant::Stock;
    context.checkId = 2004;
    context.checkType = warden::WardenCheckType::Mem;
    context.evidenceClass = warden::WardenEvidenceClass::IntegrityInvariant;
    context.outcome = warden::WardenIncidentOutcome::Mismatch;
    return context;
}
}

TEST(WardenIncidentContext_accepts_only_proven_actionable_profiles)
{
    CHECK(warden::ToIncidentOutcome(
        warden::WardenCheckOutcome::Mismatch).has_value());
    CHECK(!warden::ToIncidentOutcome(
        warden::WardenCheckOutcome::Unavailable).has_value());

    warden::WardenIncidentContext context = ValidIncident();
    CHECK(warden::IsValidWardenIncidentContext(context));
    context.architecture = warden::WardenArchitecture::X64;
    context.variant = warden::ClientVariant::Grunt;
    CHECK(warden::IsValidWardenIncidentContext(context));

    context = ValidIncident();
    context.architecture = warden::WardenArchitecture::Unclassified;
    CHECK(!warden::IsValidWardenIncidentContext(context));
    context = ValidIncident();
    context.variant = warden::ClientVariant::LegacyGrunt;
    CHECK(!warden::IsValidWardenIncidentContext(context));
    context = ValidIncident();
    context.locale = {{'z', 'z', 'Z', 'Z'}};
    CHECK(!warden::IsValidWardenIncidentContext(context));
    context = ValidIncident();
    context.realmId = 0;
    CHECK(!warden::IsValidWardenIncidentContext(context));
    context = ValidIncident();
    context.evidenceClass = warden::WardenEvidenceClass::Corroboration;
    CHECK(!warden::IsValidWardenIncidentContext(context));
}

TEST(WardenIncidentWindow_uses_exclusive_boundary_and_threshold_timestamp)
{
    warden::WardenIncidentWindowState const state =
        warden::ClassifyIncidentWindow({100, 101, 102, 110}, 110, 10, 3);
    CHECK_EQ(state.recentCount, uint32(3));
    CHECK_EQ(state.aggressiveUntil, uint64(111));

    CHECK_EQ(warden::RebaseIncidentDeadline(130, 100, 1000), uint64(1030));
    CHECK_EQ(warden::RebaseIncidentDeadline(100, 100, 1000), uint64(0));
    uint64 const maximum = std::numeric_limits<uint64>::max();
    CHECK_EQ(warden::RebaseIncidentDeadline(120, 100, maximum - 5), maximum);
}

TEST(WardenIncidentWriteResult_never_manufactures_durable_summary)
{
    warden::WardenIncidentWriteResult failed;
    warden::WardenIncidentApplication application =
        warden::ClassifyIncidentWriteResult(failed);
    CHECK(application.mustKick);
    CHECK(!application.durable);

    warden::WardenIncidentWriteResult committed;
    committed.status = warden::WardenIncidentWriteStatus::Committed;
    committed.recentCount = 10;
    committed.permanentBanActive = true;
    application = warden::ClassifyIncidentWriteResult(committed);
    CHECK(application.durable);
    CHECK(application.summaryKnown);
    CHECK_EQ(application.recentCount, uint32(10));
    CHECK(application.permanentBanActive);
}
