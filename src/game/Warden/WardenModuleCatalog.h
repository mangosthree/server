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

#ifndef MANGOS_WARDEN_MODULE_CATALOG_H
#define MANGOS_WARDEN_MODULE_CATALOG_H

#include "WardenProtocol.h"

#include <vector>

namespace warden
{
/** Architecture is discovered in-band before this exact key is selectable. */
struct ModuleProfileKey
{
    uint32 build = 0;
    WardenArchitecture architecture = WardenArchitecture::Unclassified;
};

/** One encrypted signed container and its separately supplied transfer key. */
struct ModuleProfile
{
    ModuleProfileKey key;
    ModuleId moduleId{};
    Key16 moduleKey{};
    uint32 declaredSize = 0;
    Bytes container;
};

enum class ModuleCatalogValidation : uint8
{
    Valid,
    InvalidBuild,
    InvalidArchitecture,
    EmptyContainer,
    InvalidContainerSize,
    InvalidModuleId,
    InvalidModuleKey,
    DuplicateProfile,
    EmptyCatalog
};

/** Immutable exact-key module catalogue; it never crosses architectures. */
class WardenModuleCatalog
{
public:
    ModuleProfile const* FindExact(ModuleProfileKey const& key) const;
    std::vector<ModuleProfile> const& Profiles() const;

private:
    friend class WardenModuleCatalogBuilder;
    std::vector<ModuleProfile> m_profiles;
};

/** Validates synthetic G1 data or custody-pinned G2 data before publication. */
class WardenModuleCatalogBuilder
{
public:
    ModuleCatalogValidation Add(ModuleProfile const& profile);
    ModuleCatalogValidation Build(WardenModuleCatalog& output) const;

private:
    std::vector<ModuleProfile> m_profiles;
};
}

#endif
