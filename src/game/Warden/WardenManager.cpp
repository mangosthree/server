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

#include "WardenManager.h"

#include "WardenModuleWin15595X64Data.h"
#include "WardenModuleWin15595X86Data.h"

#include <openssl/crypto.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace
{
bool IsAsciiAlpha(char value)
{
    return (value >= 'A' && value <= 'Z') ||
        (value >= 'a' && value <= 'z');
}

bool IsExactLocale(std::string const& locale)
{
    return locale.size() == 4 &&
        std::all_of(locale.begin(), locale.end(), IsAsciiAlpha);
}

bool SameProfileGroup(warden::WardenProfileKey const& left,
    warden::WardenProfileKey const& right)
{
    return left.build == right.build &&
        left.architecture == right.architecture &&
        left.locale == right.locale;
}

bool HasCompleteVariantSet(
    std::vector<warden::WardenCheckProfile> const& profiles,
    warden::WardenProfileKey const& key)
{
    bool unclassified = false;
    bool stock = false;
    bool grunt = false;
    std::size_t count = 0;
    for (warden::WardenCheckProfile const& profile : profiles)
    {
        if (!SameProfileGroup(profile.key, key))
            continue;
        ++count;
        switch (profile.key.variant)
        {
            case warden::ClientVariant::Unclassified:
                unclassified = true;
                break;
            case warden::ClientVariant::Stock:
                stock = true;
                break;
            case warden::ClientVariant::Grunt:
                grunt = true;
                break;
            default:
                return false;
        }
    }
    return count == 3 && unclassified && stock && grunt;
}

bool IsValidInterval(uint32 minimum, uint32 maximum)
{
    uint32 constexpr MaximumSeconds =
        std::numeric_limits<uint32>::max() / 1000u;
    return minimum > 0 && minimum <= maximum && maximum <= MaximumSeconds;
}

bool IsValidRuntimeConfiguration(
    warden::WardenConfiguration const& configuration)
{
    uint32 const mode = uint32(configuration.enforcementMode);
    return mode <= uint32(warden::WardenEnforcementMode::KickAndBan) &&
        IsValidInterval(configuration.normalMinSeconds,
            configuration.normalMaxSeconds) &&
        IsValidInterval(configuration.aggressiveMinSeconds,
            configuration.aggressiveMaxSeconds) &&
        configuration.aggressiveThreshold > 0 &&
        configuration.banThreshold > configuration.aggressiveThreshold &&
        warden::IsValidWardenIncidentWindow(
            configuration.incidentWindowSeconds);
}
}

namespace warden
{
char const* ToString(RuntimeValidation validation)
{
    switch (validation)
    {
        case RuntimeValidation::Valid: return "Valid";
        case RuntimeValidation::CataloguesUnavailable:
            return "CataloguesUnavailable";
        case RuntimeValidation::InvalidConfiguration:
            return "InvalidConfiguration";
        case RuntimeValidation::ObserveRequired: return "ObserveRequired";
    }
    return "Unknown";
}

WardenManager::WardenManager()
{
    WardenModuleCatalogBuilder builder;
    if (builder.Add(GetWardenModuleWin15595X86Profile()) !=
            ModuleCatalogValidation::Valid ||
        builder.Add(GetWardenModuleWin15595X64Profile()) !=
            ModuleCatalogValidation::Valid)
    {
        return;
    }

    auto modules = std::make_shared<WardenModuleCatalog>();
    if (builder.Build(*modules) == ModuleCatalogValidation::Valid)
        m_modules = std::move(modules);
}

WardenManager::WardenManager(
    std::shared_ptr<WardenModuleCatalog const> modules,
    std::shared_ptr<WardenCheckCatalog const> checks)
    : m_modules(std::move(modules)), m_checks(std::move(checks)),
      m_injectedCatalogues(true)
{
}

WardenManager& WardenManager::Instance()
{
    static WardenManager instance;
    return instance;
}

bool WardenManager::HasValidCatalogueSnapshot(
    WardenModuleCatalog const& modules, WardenCheckCatalog const& checks)
{
    if (modules.Profiles().empty() || checks.Profiles().empty())
    {
        return false;
    }

    std::vector<WardenCheckProfile> const& profiles = checks.Profiles();
    for (WardenCheckProfile const& profile : profiles)
    {
        ModuleProfile const* module = modules.FindExact(
            {profile.key.build, profile.key.architecture});
        if (!module || module->operatingMode != ModuleOperatingMode::Full ||
            !HasCompleteVariantSet(profiles, profile.key))
        {
            return false;
        }
    }

    for (ModuleProfile const& module : modules.Profiles())
    {
        bool const hasProfiles = std::any_of(profiles.begin(), profiles.end(),
            [&module](WardenCheckProfile const& profile)
            {
                return profile.key.build == module.key.build &&
                    profile.key.architecture == module.key.architecture;
            });
        if ((module.operatingMode == ModuleOperatingMode::Full) != hasProfiles)
            return false;
    }
    return true;
}

bool WardenManager::StageCatalogues(
    std::shared_ptr<WardenCheckCatalog const> checks)
{
    if (m_checks || !m_modules || !checks ||
        !HasValidCatalogueSnapshot(*m_modules, *checks))
    {
        return false;
    }
    m_checks = std::move(checks);
    return true;
}

bool WardenManager::HasStagedCatalogues() const
{
    return m_modules && m_checks &&
        HasValidCatalogueSnapshot(*m_modules, *m_checks);
}

bool WardenManager::HasActiveRuntimeSnapshot() const
{
    return GetRuntimeSnapshot() != nullptr;
}

RuntimeValidation WardenManager::ValidateRuntimeConfiguration(
    WardenConfiguration const& configuration) const
{
    if (!HasStagedCatalogues())
        return RuntimeValidation::CataloguesUnavailable;
    if (!IsValidRuntimeConfiguration(configuration))
        return RuntimeValidation::InvalidConfiguration;
    if (!CanActivate(configuration))
        return RuntimeValidation::ObserveRequired;
    return RuntimeValidation::Valid;
}

bool WardenManager::ActivateRuntimeConfiguration(
    WardenConfiguration configuration)
{
    if (ValidateRuntimeConfiguration(configuration) !=
        RuntimeValidation::Valid)
    {
        return false;
    }

    auto mutableSnapshot = std::make_shared<WardenRuntimeSnapshot>();
    mutableSnapshot->modules = m_modules;
    mutableSnapshot->checks = m_checks;
    mutableSnapshot->configuration = configuration;
    std::shared_ptr<WardenRuntimeSnapshot const> candidate = mutableSnapshot;
    std::shared_ptr<WardenRuntimeSnapshot const> expected;
    return std::atomic_compare_exchange_strong_explicit(&m_runtimeSnapshot,
        &expected, std::move(candidate), std::memory_order_release,
        std::memory_order_acquire);
}

bool WardenManager::TryReplaceRuntimeConfiguration(
    WardenConfiguration configuration)
{
    if (!GetRuntimeSnapshot() ||
        ValidateRuntimeConfiguration(configuration) !=
            RuntimeValidation::Valid)
    {
        return false;
    }

    auto mutableSnapshot = std::make_shared<WardenRuntimeSnapshot>();
    mutableSnapshot->modules = m_modules;
    mutableSnapshot->checks = m_checks;
    mutableSnapshot->configuration = configuration;
    std::shared_ptr<WardenRuntimeSnapshot const> candidate = mutableSnapshot;
    std::atomic_store_explicit(&m_runtimeSnapshot, std::move(candidate),
        std::memory_order_release);
    return true;
}

std::shared_ptr<WardenRuntimeSnapshot const>
WardenManager::GetRuntimeSnapshot() const
{
    return std::atomic_load_explicit(&m_runtimeSnapshot,
        std::memory_order_acquire);
}

WardenModuleCatalog const* WardenManager::GetModuleCatalogForStartup() const
{
    return m_modules.get();
}

bool WardenManager::CanActivate(
    WardenConfiguration const& configuration) const
{
    if (configuration.enforcementMode == WardenEnforcementMode::Observe)
        return true;

    return std::all_of(m_modules->Profiles().begin(),
        m_modules->Profiles().end(), [](ModuleProfile const& profile)
        {
            return profile.operatingMode == ModuleOperatingMode::Full &&
                profile.assurance == ModuleAssurance::ProductionApproved;
        });
}

std::unique_ptr<WardenServer> WardenManager::Create(
    WardenCreationOptions&& options, SendFrame send,
    LifecycleObserver lifecycle, EvidenceBatchObserver evidence) const
{
    std::shared_ptr<WardenRuntimeSnapshot const> runtime =
        options.runtimeSnapshot;
    std::shared_ptr<WardenModuleCatalog const> modules;
    std::shared_ptr<WardenCheckCatalog const> checks;
    WardenConfiguration configuration;
    if (runtime)
    {
        modules = runtime->modules;
        checks = runtime->checks;
        configuration = runtime->configuration;
        if (modules != m_modules || checks != m_checks)
            runtime.reset();
    }
    else if (m_injectedCatalogues)
    {
        modules = m_modules;
        checks = m_checks;
        configuration = options.configuration;
    }

    bool const supported = options.build == 15595 &&
        options.clientOs == "Win" && IsExactLocale(options.locale) &&
        bool(send) && (runtime || m_injectedCatalogues) && modules && checks &&
        HasValidCatalogueSnapshot(*modules, *checks) &&
        modules->FindExact({options.build, WardenArchitecture::X86}) &&
        modules->FindExact({options.build, WardenArchitecture::X64}) &&
        CanActivate(configuration);

    WardenCryptoContext crypto;
    bool const initialized = supported && crypto.Initialize(options.sessionKey);
    // Creation consumes the sole raw key copy whether admission succeeds or
    // fails; catalogue snapshots never retain session material.
    OPENSSL_cleanse(options.sessionKey.data(), options.sessionKey.size());
    if (!initialized)
        return nullptr;

    return std::unique_ptr<WardenServer>(new WardenServer(options.build,
        std::move(options.clientOs), std::move(options.locale),
        configuration, options.initialAggressive, std::move(modules),
        std::move(checks),
        std::move(crypto), std::move(send), std::move(lifecycle),
        std::move(evidence)));
}
}
