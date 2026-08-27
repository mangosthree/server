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

#include <openssl/crypto.h>

#include <algorithm>
#include <cstddef>
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
}

namespace warden
{
WardenManager::WardenManager(
    std::shared_ptr<WardenModuleCatalog const> modules,
    std::shared_ptr<WardenCheckCatalog const> checks)
    : m_modules(std::move(modules)), m_checks(std::move(checks))
{
}

bool WardenManager::HasValidCatalogueSnapshot() const
{
    if (!m_modules || !m_checks || m_modules->Profiles().empty() ||
        m_checks->Profiles().empty())
    {
        return false;
    }

    std::vector<WardenCheckProfile> const& profiles = m_checks->Profiles();
    for (WardenCheckProfile const& profile : profiles)
    {
        ModuleProfile const* module = m_modules->FindExact(
            {profile.key.build, profile.key.architecture});
        if (!module || module->operatingMode != ModuleOperatingMode::Full ||
            !HasCompleteVariantSet(profiles, profile.key))
        {
            return false;
        }
    }

    for (ModuleProfile const& module : m_modules->Profiles())
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
    bool const supported = options.build == 15595 &&
        options.clientOs == "Win" && IsExactLocale(options.locale) &&
        bool(send) && HasValidCatalogueSnapshot() &&
        m_modules->FindExact({options.build, WardenArchitecture::X86}) &&
        m_modules->FindExact({options.build, WardenArchitecture::X64}) &&
        CanActivate(options.configuration);

    WardenCryptoContext crypto;
    bool const initialized = supported && crypto.Initialize(options.sessionKey);
    // Creation consumes the sole raw key copy whether admission succeeds or
    // fails; catalogue snapshots never retain session material.
    OPENSSL_cleanse(options.sessionKey.data(), options.sessionKey.size());
    if (!initialized)
        return nullptr;

    return std::unique_ptr<WardenServer>(new WardenServer(options.build,
        std::move(options.clientOs), std::move(options.locale),
        options.configuration, options.initialAggressive, m_modules, m_checks,
        std::move(crypto), std::move(send), std::move(lifecycle),
        std::move(evidence)));
}
}
