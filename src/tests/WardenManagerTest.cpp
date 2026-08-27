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
