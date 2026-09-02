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

#include "WardenAuditStore.h"

#include "Database/DatabaseEnv.h"

#include <string>

namespace warden
{
WardenAuditStore& WardenAuditStore::Instance()
{
    static WardenAuditStore instance;
    return instance;
}

bool WardenAuditStore::Record(WardenAuditContext const& context) const
{
    if (!IsValidWardenAuditContext(context))
        return false;

    char const* architecture = ToPersistenceToken(context.architecture);
    char const* variant = ToPersistenceToken(context.variant);
    if (!architecture || !variant)
        return false;

    // Copy every value before the asynchronous statement is queued. Enum
    // tokens are fixed vocabulary and the locale has already passed the
    // exact four-byte identity validator; escaping remains defense in depth.
    std::string safePlatform(context.clientPlatform);
    std::string safeArchitecture(architecture);
    std::string safeLocale(context.locale.begin(), context.locale.end());
    std::string safeVariant(variant);
    LoginDatabase.escape_string(safePlatform);
    LoginDatabase.escape_string(safeArchitecture);
    LoginDatabase.escape_string(safeLocale);
    LoginDatabase.escape_string(safeVariant);
    return LoginDatabase.PExecute(
        "INSERT INTO `warden_audit` "
        "(`account_id`,`occurred_at`,`realm_id`,`client_build`,"
        "`client_platform`,`client_architecture`,`client_locale`,"
        "`client_variant`,`check_id`,"
        "`check_type`,`evidence_class`,`outcome`) "
        "VALUES (%u,UNIX_TIMESTAMP(),%u,%u,'%s','%s','%s','%s',%u,%u,%u,%u)",
        context.accountId, context.realmId, context.clientBuild,
        safePlatform.c_str(), safeArchitecture.c_str(), safeLocale.c_str(),
        safeVariant.c_str(),
        context.checkId, uint32(context.checkType),
        uint32(context.evidenceClass), uint32(context.outcome));
}
}
