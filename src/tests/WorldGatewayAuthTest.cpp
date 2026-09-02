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

#include "Common/Locales.h"
#include "WorldGatewayAuth.h"

#include <array>
#include <cstddef>
#include <string>
#include <utility>

TEST(WorldGatewayAuth_accepts_only_supported_account_operating_systems)
{
    CHECK(IsSupportedAccountClientOS("Win"));
    CHECK(IsSupportedAccountClientOS("OSX"));
    CHECK(!IsSupportedAccountClientOS(""));
    CHECK(!IsSupportedAccountClientOS("win"));
    CHECK(!IsSupportedAccountClientOS("Windows"));
}

TEST(WorldGatewayAuth_projection_indices_append_exact_locale_last)
{
    using Field = WorldGatewayAccountField;
    std::array<std::pair<Field, std::size_t>, 12> const expected = {{
        {Field::Id, 0}, {Field::Security, 1}, {Field::SessionKey, 2},
        {Field::LastIp, 3}, {Field::Locked, 4}, {Field::SessionSalt, 5},
        {Field::Expansion, 6}, {Field::MuteTime, 7},
        {Field::DbcLocale, 8}, {Field::ClientOS, 9},
        {Field::ClientLocale, 10}, {Field::Count, 11}
    }};
    for (auto const& entry : expected)
    {
        CHECK_EQ(WorldGatewayAccountFieldIndex(entry.first), entry.second);
        CHECK_EQ(static_cast<std::size_t>(entry.first), entry.second);
    }
}

TEST(WorldGatewayAuth_exact_locale_accepts_only_the_dedicated_auth_vocabulary)
{
    std::array<char const*, 15> const locales = {{
        "enUS", "enGB", "enCN", "enTW", "koKR", "frFR", "deDE",
        "zhCN", "zhTW", "esES", "esMX", "ruRU", "ptPT", "ptBR",
        "itIT"
    }};
    for (char const* locale : locales)
    {
        char const* exact = GetExactLocaleName(locale);
        REQUIRE(exact != nullptr);
        CHECK_STR(exact, locale);
    }

    CHECK(GetExactLocaleName("") == nullptr);
    CHECK(GetExactLocaleName("enus") == nullptr);
    CHECK(GetExactLocaleName("ENUS") == nullptr);
    CHECK(GetExactLocaleName("enUS ") == nullptr);
    CHECK(GetExactLocaleName("zzZZ") == nullptr);
    CHECK(GetExactLocaleName(std::string("en\0US", 5)) == nullptr);
}

TEST(WorldGatewayAuth_does_not_mutate_positional_DBC_locale_vocabulary)
{
    std::array<char const*, 13> const expected = {{
        "enUS", "enGB", "koKR", "frFR", "deDE", "zhCN", "zhTW",
        "esES", "esMX", "ruRU", "ptPT", "ptBR", "itIT"
    }};
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        REQUIRE(fullLocaleNameList[index].name != nullptr);
        CHECK_STR(fullLocaleNameList[index].name, expected[index]);
    }
    CHECK(fullLocaleNameList[expected.size()].name == nullptr);
    CHECK_EQ(fullLocaleNameList[expected.size()].locale, LOCALE_enUS);
    CHECK_EQ(std::size(localeNames), std::size_t(MAX_LOCALE));
}
