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

#include <array>
#include <vector>

namespace warden
{
/** Architecture is discovered in-band before this exact key is selectable. */
struct ModuleProfileKey
{
    uint32 build = 0;
    WardenArchitecture architecture = WardenArchitecture::Unclassified;
};

/** Selects the exact module wire contract after architecture proof. */
enum class ModuleAbi : uint8
{
    Cata15595X86,
    Cata15595X64
};

/** Immutable origin of the signed encrypted module container. */
enum class ModuleProvenance : uint8
{
    RetailCaptured15595,
    BuildMatchedPublic,
    SignedCrossBuild
};

/** Maximum behavior the server may request from this module. */
enum class ModuleOperatingMode : uint8
{
    Full,
    CompatibilityProbeOnly
};

/** Independently reviewed deployment confidence, separate from provenance. */
enum class ModuleAssurance : uint8
{
    StaticVerified,
    ExactClientLabValidated,
    ProductionApproved
};

/** Module-specific encodings for the four currently modelled check families. */
struct ModuleCheckCodes
{
    uint8 timing = 0;
    uint8 lua = 0;
    uint8 mpq = 0;
    uint8 memory = 0;
};

constexpr uint8 CataX64CompatibilityTimingCode = 0xEA;
constexpr uint8 Cata15595X86TimingCode = 0xD0;
constexpr uint8 Cata15595X86LuaCode = 0x41;
constexpr uint8 Cata15595X86MpqCode = 0x82;
constexpr uint8 Cata15595X86MemoryCode = 0x34;
constexpr uint8 Cata15595X86EndCode = 0xD9;

/** One binary-proven module hash challenge and its atomic replacement keys. */
struct ModuleRekeyVector
{
    Key16 seed{};
    Digest20 expectedResponse{};
    Key16 clientToServer{};
    Key16 serverToClient{};
};

/** One encrypted signed container and its separately supplied transfer key. */
struct ModuleProfile
{
    ModuleProfileKey key;
    ModuleAbi abi = static_cast<ModuleAbi>(0xFF);
    ModuleProvenance provenance = static_cast<ModuleProvenance>(0xFF);
    ModuleOperatingMode operatingMode =
        static_cast<ModuleOperatingMode>(0xFF);
    ModuleAssurance assurance = static_cast<ModuleAssurance>(0xFF);
    ModuleCheckCodes checkCodes;
    ModuleRekeyVector rekey;
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
    InvalidAbi,
    InvalidMetadata,
    DuplicateCheckCode,
    InvalidCheckCodeMap,
    InvalidRekeyVector,
    EmptyContainer,
    InvalidContainerSize,
    InvalidModuleId,
    InvalidModuleKey,
    DuplicateProfile,
    EmptyCatalog,
    IncompleteArchitectureSet
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
