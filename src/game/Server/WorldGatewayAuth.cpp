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

#include "WorldGatewayAuth.h"

#include <openssl/crypto.h>

#include <algorithm>
#include <array>
#include <utility>

bool IsSupportedAccountClientOS(std::string const& os)
{
    return os == "Win" || os == "OSX";
}

WardenAdmissionDisposition ClassifyWardenAdmission(
    warden::AdmissionStatus status,
    warden::WardenConfiguration const& configuration)
{
    if (status == warden::AdmissionStatus::Available)
        return WardenAdmissionDisposition::Start;
    if (configuration.enforcementMode ==
            warden::WardenEnforcementMode::Observe ||
        !configuration.requireExactProfile)
    {
        return WardenAdmissionDisposition::AdmitWithoutWarden;
    }
    return WardenAdmissionDisposition::Reject;
}

char const* GetExactLocaleName(std::string const& locale)
{
    static constexpr std::array<char const*, 15> Locales = {{
        "enUS", "enGB", "enCN", "enTW", "koKR", "frFR", "deDE",
        "zhCN", "zhTW", "esES", "esMX", "ruRU", "ptPT", "ptBR",
        "itIT"
    }};
    auto const match = std::find_if(Locales.begin(), Locales.end(),
        [&locale](char const* candidate) { return locale == candidate; });
    return match == Locales.end() ? nullptr : *match;
}

warden::AdmissionData BuildWardenAdmissionData(uint32 build,
    std::string clientOs, std::string clientLocale, BigNumber& sessionKey)
{
    warden::AdmissionData admission;
    admission.build = build;
    admission.clientOs = std::move(clientOs);

    if (clientLocale.empty())
    {
        admission.status = warden::AdmissionStatus::MissingExactLocale;
        return admission;
    }

    std::array<char, 4> rawLocale{};
    if (clientLocale.size() == rawLocale.size())
    {
        std::copy(clientLocale.begin(), clientLocale.end(), rawLocale.begin());
        if (warden::IsCanonicalLocaleClaim(rawLocale))
            admission.clientLocale = clientLocale;
    }

    char const* exactLocale = GetExactLocaleName(clientLocale);
    if (!exactLocale)
    {
        admission.status = warden::AdmissionStatus::UnsupportedExactLocale;
        return admission;
    }

    std::copy_n(exactLocale, rawLocale.size(), rawLocale.begin());
    admission.clientLocale.assign(exactLocale, rawLocale.size());
    if (!warden::IsPublishedCataWardenLocale(rawLocale))
    {
        // itIT is a real Cata authentication locale but has no measured Warden
        // profile yet. It must remain distinguishable from malformed input.
        admission.status = warden::AdmissionStatus::UnsupportedExactLocale;
        return admission;
    }

    int const keyBytes = sessionKey.GetNumBytes();
    if (sessionKey.isZero() || keyBytes <= 0 ||
        keyBytes > int(admission.sessionKey.size()))
    {
        admission.status = warden::AdmissionStatus::SessionKeyUnavailable;
        return admission;
    }

    // BigNumber owns this buffer and replaces it on the next serialization.
    // Copy all 40 bytes, cleanse in place, and neither retain nor free it.
    uint8* const serialized = sessionKey.AsByteArray(
        static_cast<int>(admission.sessionKey.size()));
    if (!serialized)
    {
        admission.status = warden::AdmissionStatus::SessionKeyUnavailable;
        return admission;
    }
    std::copy(serialized, serialized + admission.sessionKey.size(),
        admission.sessionKey.begin());
    OPENSSL_cleanse(serialized, admission.sessionKey.size());
    admission.status = warden::AdmissionStatus::Available;
    return admission;
}
