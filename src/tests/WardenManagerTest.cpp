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
#include "WardenManager.h"

#include <memory>
#include <utility>

TEST(WardenManager_stages_once_without_exposing_a_runtime)
{
    warden::WardenManager manager;
    CHECK(!manager.HasStagedCatalogues());
    CHECK(!manager.HasActiveRuntimeSnapshot());

    auto checks = std::make_shared<warden::WardenCheckCatalog const>(
        warden::test::BuildSyntheticCheckCatalog());
    REQUIRE(manager.StageCatalogues(checks));
    CHECK(manager.HasStagedCatalogues());
    CHECK(!manager.HasActiveRuntimeSnapshot());
    CHECK(!manager.StageCatalogues(std::move(checks)));

    warden::WardenCreationOptions options;
    options.build = 15595;
    options.clientOs = "Win";
    options.locale = "enUS";
    options.sessionKey = warden::test::SyntheticSessionKey();
    options.configuration.enforcementMode =
        warden::WardenEnforcementMode::Observe;
    CHECK(manager.Create(std::move(options),
        [](warden::EncodedServerFrame const&) { return true; }) == nullptr);
}

TEST(WardenManager_rejects_empty_staging_without_consuming_the_slot)
{
    warden::WardenManager manager;
    auto empty = std::make_shared<warden::WardenCheckCatalog const>();
    CHECK(!manager.StageCatalogues(std::move(empty)));
    CHECK(!manager.HasStagedCatalogues());

    auto complete = std::make_shared<warden::WardenCheckCatalog const>(
        warden::test::BuildSyntheticCheckCatalog());
    CHECK(manager.StageCatalogues(std::move(complete)));
}

TEST(WardenManager_requires_staging_and_observe_for_nonproduction_modules)
{
    warden::WardenManager manager;
    warden::WardenConfiguration observe;
    observe.enforcementMode = warden::WardenEnforcementMode::Observe;
    CHECK(manager.ValidateRuntimeConfiguration(observe) ==
        warden::RuntimeValidation::CataloguesUnavailable);
    CHECK(!manager.ActivateRuntimeConfiguration(observe));

    auto checks = std::make_shared<warden::WardenCheckCatalog const>(
        warden::test::BuildSyntheticCheckCatalog());
    REQUIRE(manager.StageCatalogues(std::move(checks)));
    CHECK(manager.ValidateRuntimeConfiguration(observe) ==
        warden::RuntimeValidation::Valid);

    warden::WardenConfiguration malformed = observe;
    malformed.normalMinSeconds = 61;
    malformed.normalMaxSeconds = 60;
    CHECK(manager.ValidateRuntimeConfiguration(malformed) ==
        warden::RuntimeValidation::InvalidConfiguration);

    warden::WardenConfiguration kick = observe;
    kick.enforcementMode = warden::WardenEnforcementMode::Kick;
    CHECK(manager.ValidateRuntimeConfiguration(kick) ==
        warden::RuntimeValidation::ObserveRequired);
    CHECK(!manager.ActivateRuntimeConfiguration(kick));
    kick.enforcementMode = warden::WardenEnforcementMode::KickAndBan;
    CHECK(manager.ValidateRuntimeConfiguration(kick) ==
        warden::RuntimeValidation::ObserveRequired);
}

TEST(WardenManager_rejected_reload_preserves_the_exact_active_snapshot)
{
    warden::WardenManager manager;
    auto checks = std::make_shared<warden::WardenCheckCatalog const>(
        warden::test::BuildSyntheticCheckCatalog());
    REQUIRE(manager.StageCatalogues(std::move(checks)));

    warden::WardenConfiguration observe;
    observe.enforcementMode = warden::WardenEnforcementMode::Observe;
    REQUIRE(manager.ActivateRuntimeConfiguration(observe));
    std::shared_ptr<warden::WardenRuntimeSnapshot const> first =
        manager.GetRuntimeSnapshot();
    REQUIRE(first != nullptr);
    CHECK(first->configuration.normalMinSeconds == uint32(30));

    warden::WardenConfiguration replacement = observe;
    replacement.normalMinSeconds = 40;
    replacement.normalMaxSeconds = 70;
    REQUIRE(manager.TryReplaceRuntimeConfiguration(replacement));
    std::shared_ptr<warden::WardenRuntimeSnapshot const> second =
        manager.GetRuntimeSnapshot();
    REQUIRE(second != nullptr);
    CHECK(second != first);
    CHECK(first->configuration.normalMinSeconds == uint32(30));
    CHECK(second->configuration.normalMinSeconds == uint32(40));

    warden::WardenConfiguration rejected = replacement;
    rejected.enforcementMode = warden::WardenEnforcementMode::Kick;
    CHECK(!manager.TryReplaceRuntimeConfiguration(rejected));
    CHECK(manager.GetRuntimeSnapshot() == second);
    CHECK(!manager.ActivateRuntimeConfiguration(observe));
}

TEST(WardenManager_creates_only_from_a_manager_published_generation)
{
    warden::WardenManager manager;
    auto checks = std::make_shared<warden::WardenCheckCatalog const>(
        warden::test::BuildSyntheticCheckCatalog());
    REQUIRE(manager.StageCatalogues(std::move(checks)));
    warden::WardenConfiguration observe;
    observe.enforcementMode = warden::WardenEnforcementMode::Observe;
    REQUIRE(manager.ActivateRuntimeConfiguration(observe));
    std::shared_ptr<warden::WardenRuntimeSnapshot const> captured =
        manager.GetRuntimeSnapshot();
    REQUIRE(captured != nullptr);

    warden::WardenConfiguration replacement = observe;
    replacement.normalMinSeconds = 40;
    replacement.normalMaxSeconds = 70;
    REQUIRE(manager.TryReplaceRuntimeConfiguration(replacement));

    warden::WardenCreationOptions options;
    options.build = 15595;
    options.clientOs = "Win";
    options.locale = "enUS";
    options.sessionKey = warden::test::SyntheticSessionKey();
    options.runtimeSnapshot = captured;
    CHECK(manager.Create(std::move(options),
        [](warden::EncodedServerFrame const&) { return true; }) != nullptr);

    warden::WardenCreationOptions missingSnapshot;
    missingSnapshot.build = 15595;
    missingSnapshot.clientOs = "Win";
    missingSnapshot.locale = "enUS";
    missingSnapshot.sessionKey = warden::test::SyntheticSessionKey();
    CHECK(manager.Create(std::move(missingSnapshot),
        [](warden::EncodedServerFrame const&) { return true; }) == nullptr);
}
