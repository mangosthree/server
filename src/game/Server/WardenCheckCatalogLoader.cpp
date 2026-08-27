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

#include "WardenCheckCatalogLoader.h"

#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "WardenManager.h"

#include <memory>
#include <string>

namespace
{
std::string SqlHex(Field const& field)
{
    char const* value = field.GetString();
    return value ? std::string(value) : std::string();
}

void LogLoadFailure(warden::WardenCheckCatalogLoadFailure failure)
{
    sLog.outError("Warden catalogue load failed: %s.",
        warden::ToString(failure));
}

void LogLoadFailure(warden::WardenCheckCatalogLoadFailure failure,
    warden::WardenCheckDiagnostic const& diagnostic)
{
    sLog.outError("Warden catalogue load failed: %s "
        "(build %u; check %u; validation %u).",
        warden::ToString(failure), diagnostic.profile.build,
        diagnostic.checkId, uint32(diagnostic.validation));
}
}

namespace warden
{
char const* ToString(WardenCheckCatalogLoadFailure failure)
{
    switch (failure)
    {
        case WardenCheckCatalogLoadFailure::None: return "None";
        case WardenCheckCatalogLoadFailure::CatalogueQueryFailed:
            return "CatalogueQueryFailed";
        case WardenCheckCatalogLoadFailure::EmptyCatalogue:
            return "EmptyCatalogue";
        case WardenCheckCatalogLoadFailure::SourceCountOverflow:
            return "SourceCountOverflow";
        case WardenCheckCatalogLoadFailure::SourceCountInconsistent:
            return "SourceCountInconsistent";
        case WardenCheckCatalogLoadFailure::SourceCountMismatch:
            return "SourceCountMismatch";
        case WardenCheckCatalogLoadFailure::InvalidRow: return "InvalidRow";
        case WardenCheckCatalogLoadFailure::ProfileWithoutModule:
            return "ProfileWithoutModule";
        case WardenCheckCatalogLoadFailure::ModuleWithoutProfile:
            return "ModuleWithoutProfile";
        case WardenCheckCatalogLoadFailure::PublicationFailed:
            return "PublicationFailed";
    }
    return "Unknown";
}

bool WardenCheckCatalogLoader::LoadAndStage() const
{
    // Count and rows are projected by one statement. Filtering here makes a
    // disabled operator row inert while a disabled required profile still
    // fails the complete-coverage transaction below.
    std::unique_ptr<QueryResult> result(WorldDatabase.Query(
        "SELECT `snapshot`.`snapshot_count`, `checks`.`build`, "
        "HEX(`checks`.`architecture`), HEX(`checks`.`locale`), "
        "HEX(`checks`.`variant`), `checks`.`check_id`, `checks`.`type`, "
        "`checks`.`enabled`, `checks`.`sort_order`, "
        "`checks`.`evidence_class`, `checks`.`phase_mask`, "
        "`checks`.`address_kind`, HEX(`checks`.`module`), "
        "`checks`.`address`, `checks`.`length`, "
        "HEX(`checks`.`request`), HEX(`checks`.`expected`) "
        "FROM (SELECT COUNT(*) AS `snapshot_count` FROM `warden_checks` "
        "WHERE `enabled` = 1) AS `snapshot` "
        "LEFT JOIN (SELECT * FROM `warden_checks` WHERE `enabled` = 1) "
        "AS `checks` ON TRUE "
        "ORDER BY `checks`.`build`, `checks`.`architecture`, "
        "`checks`.`locale`, `checks`.`variant`, `checks`.`sort_order`, "
        "`checks`.`check_id`"));
    if (!result)
    {
        LogLoadFailure(WardenCheckCatalogLoadFailure::CatalogueQueryFailed);
        return false;
    }

    Field const* fields = result->Fetch();
    if (!fields)
    {
        LogLoadFailure(WardenCheckCatalogLoadFailure::CatalogueQueryFailed);
        return false;
    }

    WardenCheckCatalogLoadTransaction transaction;
    WardenCheckCatalogLoadFailure failure =
        transaction.Begin(fields[0].GetUInt64());
    if (failure != WardenCheckCatalogLoadFailure::None)
    {
        // The empty-table sentinel owns only snapshot_count. Never inspect its
        // nullable catalogue columns after Begin rejects it.
        LogLoadFailure(failure);
        return false;
    }

    uint32 rows = 0;
    WardenCheckDiagnostic diagnostic;
    do
    {
        fields = result->Fetch();
        failure = transaction.ObserveSourceCount(fields[0].GetUInt64());
        if (failure != WardenCheckCatalogLoadFailure::None)
        {
            LogLoadFailure(failure);
            return false;
        }

        // HEX preserves exact binary lengths and embedded NULs. Scalar fields
        // remain at schema width until WardenCheckCatalogBuilder validates
        // every family-specific narrowing and address rule.
        WardenCheckRowInput input;
        input.build = fields[1].GetUInt32();
        input.architectureHex = SqlHex(fields[2]);
        input.localeHex = SqlHex(fields[3]);
        input.variantHex = SqlHex(fields[4]);
        input.checkId = fields[5].GetUInt32();
        input.type = fields[6].GetUInt32();
        input.enabled = fields[7].GetUInt32();
        input.sortOrder = fields[8].GetUInt32();
        input.evidenceClass = fields[9].GetUInt32();
        input.phaseMask = fields[10].GetUInt32();
        input.addressKind = fields[11].GetUInt32();
        input.moduleHex = SqlHex(fields[12]);
        input.address = fields[13].GetUInt64();
        input.length = fields[14].GetUInt32();
        input.requestHex = SqlHex(fields[15]);
        input.expectedHex = SqlHex(fields[16]);
        failure = transaction.Add(input, diagnostic);
        if (failure != WardenCheckCatalogLoadFailure::None)
        {
            if (failure == WardenCheckCatalogLoadFailure::InvalidRow)
                LogLoadFailure(failure, diagnostic);
            else
                LogLoadFailure(failure);
            return false;
        }
        ++rows;
    }
    while (result->NextRow());

    WardenManager& manager = WardenManager::Instance();
    WardenModuleCatalog const* modules =
        manager.GetModuleCatalogForStartup();
    if (!modules)
    {
        LogLoadFailure(WardenCheckCatalogLoadFailure::ProfileWithoutModule);
        return false;
    }

    std::shared_ptr<WardenCheckCatalog const> published;
    failure = transaction.Finish(*modules,
        [&manager, &published](
            std::shared_ptr<WardenCheckCatalog const> const& snapshot)
        {
            if (!manager.StageCatalogues(snapshot))
                return false;
            published = snapshot;
            return true;
        }, diagnostic);
    if (failure != WardenCheckCatalogLoadFailure::None)
    {
        if (failure == WardenCheckCatalogLoadFailure::InvalidRow)
            LogLoadFailure(failure, diagnostic);
        else
            LogLoadFailure(failure);
        return false;
    }

    sLog.outString("Warden catalogue staged: %u enabled rows in %u "
        "exact profiles; runtime remains inactive pending configuration.",
        rows, uint32(published->Profiles().size()));
    return true;
}
}
