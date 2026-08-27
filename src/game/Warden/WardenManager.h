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

#ifndef MANGOS_WARDEN_MANAGER_H
#define MANGOS_WARDEN_MANAGER_H

#include "WardenServer.h"

#include <memory>
#include <string>

namespace warden
{
/** Immutable authenticated inputs; architecture is deliberately absent. */
struct WardenCreationOptions
{
    uint32 build = 0;
    std::string clientOs;
    std::string locale;
    SessionKey sessionKey{};
    WardenConfiguration configuration;
    bool initialAggressive = false;
};

/**
 * Holds complete immutable catalogue snapshots and creates inert sessions.
 * Create validates catalogue coherence but never chooses an architecture.
 */
class WardenManager
{
public:
    WardenManager(std::shared_ptr<WardenModuleCatalog const> modules,
        std::shared_ptr<WardenCheckCatalog const> checks);

    std::unique_ptr<WardenServer> Create(WardenCreationOptions&& options,
        SendFrame send, LifecycleObserver lifecycle = {},
        EvidenceBatchObserver evidence = {}) const;

private:
    bool HasValidCatalogueSnapshot() const;
    bool CanActivate(WardenConfiguration const& configuration) const;

    std::shared_ptr<WardenModuleCatalog const> m_modules;
    std::shared_ptr<WardenCheckCatalog const> m_checks;
};
}

#endif
