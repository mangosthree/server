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
#include <array>
#include <utility>

namespace
{
std::array<char, 4> ExactLocale(std::string const& locale)
{
    std::array<char, 4> result{};
    if (locale.size() == result.size())
        std::copy(locale.begin(), locale.end(), result.begin());
    return result;
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

bool WardenManager::HasCompleteExactProfiles(uint32 build,
    std::string const& locale) const
{
    if (!m_modules || !m_checks || locale.size() != 4)
        return false;

    std::array<char, 4> const exactLocale = ExactLocale(locale);
    for (WardenArchitecture architecture :
        {WardenArchitecture::X86, WardenArchitecture::X64})
    {
        if (!m_modules->FindExact({build, architecture}))
            return false;

        for (ClientVariant variant :
            {ClientVariant::Unclassified, ClientVariant::Stock,
                ClientVariant::Grunt})
        {
            if (!m_checks->FindProfileExact(
                    {build, architecture, exactLocale, variant}))
            {
                return false;
            }
        }
    }
    return true;
}

std::unique_ptr<WardenServer> WardenManager::Create(
    WardenCreationOptions&& options, SendFrame send,
    LifecycleObserver lifecycle, EvidenceBatchObserver evidence) const
{
    bool const supported = options.build == 15595 &&
        options.clientOs == "Win" && bool(send) &&
        HasCompleteExactProfiles(options.build, options.locale);

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
