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

#ifndef MANGOS_WARDEN_AUDIT_STORE_H
#define MANGOS_WARDEN_AUDIT_STORE_H

#include "WardenEvidence.h"

#include <array>
#include <optional>

namespace warden
{
/** Durable non-punitive outcome stored in the Realm audit table. */
enum class WardenAuditOutcome : uint8
{
    Mismatch = 1,
    Unavailable = 2
};

/** Secret-free exact identity for check evidence or an operational failure. */
struct WardenAuditContext
{
    uint32 accountId = 0;
    uint32 realmId = 0;
    uint32 clientBuild = 0;
    WardenArchitecture architecture = WardenArchitecture::Unclassified;
    std::array<char, 4> locale{};
    ClientVariant variant = ClientVariant::Unclassified;
    uint32 checkId = 0;
    WardenCheckType checkType = WardenCheckType::Timing;
    WardenEvidenceClass evidenceClass = WardenEvidenceClass::ProtocolHealth;
    WardenAuditOutcome outcome = WardenAuditOutcome::Mismatch;
};

inline std::optional<WardenAuditOutcome> ToAuditOutcome(
    WardenCheckOutcome outcome)
{
    if (outcome == WardenCheckOutcome::Mismatch)
        return WardenAuditOutcome::Mismatch;
    if (outcome == WardenCheckOutcome::Unavailable)
        return WardenAuditOutcome::Unavailable;
    return std::nullopt;
}

inline bool IsValidWardenAuditContext(WardenAuditContext const& context)
{
    bool const knownArchitecture =
        ToPersistenceToken(context.architecture) != nullptr;
    bool const knownVariant = ToPersistenceToken(context.variant) != nullptr;
    std::array<char, 4> const unknownLocale = {{'u', 'n', 'k', 'n'}};
    bool const operational = context.checkId == 0 && knownArchitecture &&
        knownVariant && (context.locale == unknownLocale ||
            IsCanonicalLocaleClaim(context.locale)) &&
        context.checkType == WardenCheckType::Timing &&
        context.evidenceClass == WardenEvidenceClass::ProtocolHealth &&
        context.outcome == WardenAuditOutcome::Unavailable;
    bool const checkEvidence = context.checkId != 0 &&
        (context.architecture == WardenArchitecture::X86 ||
            context.architecture == WardenArchitecture::X64) &&
        IsPublishedCataWardenLocale(context.locale) &&
        (context.variant == ClientVariant::Stock ||
            context.variant == ClientVariant::Grunt ||
            (context.architecture == WardenArchitecture::X86 &&
                context.variant == ClientVariant::LegacyGrunt)) &&
        context.checkType != WardenCheckType::Timing &&
        IsLegalWardenEvidenceClass(context.checkType,
            context.evidenceClass) &&
        (context.outcome == WardenAuditOutcome::Mismatch ||
            context.outcome == WardenAuditOutcome::Unavailable);
    return context.accountId != 0 && context.realmId != 0 &&
        context.clientBuild == 15595 &&
        (operational || checkEvidence);
}

/** Best-effort append-only store; it never makes an enforcement decision. */
class WardenAuditStore
{
public:
    static WardenAuditStore& Instance();
    bool Record(WardenAuditContext const& context) const;
};
}

#endif
