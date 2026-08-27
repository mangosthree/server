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

#include "WardenAuditStore.h"

#include <optional>

namespace
{
warden::WardenAuditContext ValidAudit()
{
    warden::WardenAuditContext context;
    context.accountId = 6;
    context.realmId = 1;
    context.clientBuild = 15595;
    context.architecture = warden::WardenArchitecture::X86;
    context.locale = {{'e', 'n', 'U', 'S'}};
    context.variant = warden::ClientVariant::Stock;
    context.checkId = 2003;
    context.checkType = warden::WardenCheckType::Lua;
    context.evidenceClass = warden::WardenEvidenceClass::Corroboration;
    context.outcome = warden::WardenAuditOutcome::Mismatch;
    return context;
}
}

TEST(WardenAuditOutcome_accepts_only_mismatch_and_unavailable)
{
    CHECK(!warden::ToAuditOutcome(
        warden::WardenCheckOutcome::Match).has_value());
    CHECK(!warden::ToAuditOutcome(
        warden::WardenCheckOutcome::Stable).has_value());
    CHECK(warden::ToAuditOutcome(
        warden::WardenCheckOutcome::Mismatch).has_value());
    CHECK(warden::ToAuditOutcome(
        warden::WardenCheckOutcome::Unavailable).has_value());
}

TEST(WardenAuditContext_accepts_exact_check_and_operational_identities)
{
    warden::WardenAuditContext context = ValidAudit();
    CHECK(warden::IsValidWardenAuditContext(context));

    context.checkId = 0;
    context.architecture = warden::WardenArchitecture::Unclassified;
    context.locale = {{'u', 'n', 'k', 'n'}};
    context.variant = warden::ClientVariant::Unclassified;
    context.checkType = warden::WardenCheckType::Timing;
    context.evidenceClass = warden::WardenEvidenceClass::ProtocolHealth;
    context.outcome = warden::WardenAuditOutcome::Unavailable;
    CHECK(warden::IsValidWardenAuditContext(context));

    context.architecture = warden::WardenArchitecture::X64;
    context.locale = {{'z', 'z', 'Z', 'Z'}};
    context.variant = warden::ClientVariant::LegacyGrunt;
    CHECK(warden::IsValidWardenAuditContext(context));
}

TEST(WardenAuditContext_rejects_unproven_check_identity_and_bad_shapes)
{
    warden::WardenAuditContext context = ValidAudit();
    context.variant = warden::ClientVariant::LegacyGrunt;
    CHECK(!warden::IsValidWardenAuditContext(context));
    context = ValidAudit();
    context.locale = {{'z', 'z', 'Z', 'Z'}};
    CHECK(!warden::IsValidWardenAuditContext(context));
    context = ValidAudit();
    context.architecture = warden::WardenArchitecture::Unclassified;
    CHECK(!warden::IsValidWardenAuditContext(context));
    context = ValidAudit();
    context.accountId = 0;
    CHECK(!warden::IsValidWardenAuditContext(context));
    context = ValidAudit();
    context.realmId = 0;
    CHECK(!warden::IsValidWardenAuditContext(context));
    context = ValidAudit();
    context.checkId = 0;
    CHECK(!warden::IsValidWardenAuditContext(context));
}
