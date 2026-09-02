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

#ifndef MANGOS_WORLD_GATEWAY_AUTH_H
#define MANGOS_WORLD_GATEWAY_AUTH_H

#include "Auth/BigNumber.h"
#include "WardenConfiguration.h"
#include "WardenProtocol.h"

#include <cstddef>
#include <string>

/** Append-only account projection used by WorldGateway::LookupAccount. */
enum class WorldGatewayAccountField : std::size_t
{
    Id = 0,
    Security = 1,
    SessionKey = 2,
    LastIp = 3,
    Locked = 4,
    SessionSalt = 5,
    Expansion = 6,
    MuteTime = 7,
    DbcLocale = 8,
    ClientOS = 9,
    ClientLocale = 10,
    Count = 11
};

constexpr std::size_t WorldGatewayAccountFieldIndex(
    WorldGatewayAccountField field)
{
    return static_cast<std::size_t>(field);
}

bool IsSupportedAccountClientOS(std::string const& os);

enum class WardenAdmissionDisposition : uint8
{
    Start,
    AdmitWithoutWarden,
    Reject
};

/** Applies exact-profile policy to a typed admission preparation result. */
WardenAdmissionDisposition ClassifyWardenAdmission(
    warden::AdmissionStatus status,
    warden::WardenConfiguration const& configuration);

/** Exact auth vocabulary; independent of positional DBC locale aliases. */
char const* GetExactLocaleName(std::string const& locale);

/** Copies one fixed-width Warden key while retaining BigNumber ownership. */
warden::AdmissionData BuildWardenAdmissionData(uint32 build,
    std::string clientOs, std::string clientLocale, BigNumber& sessionKey);

#endif
