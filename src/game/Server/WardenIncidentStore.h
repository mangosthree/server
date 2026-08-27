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

#ifndef MANGOS_WARDEN_INCIDENT_STORE_H
#define MANGOS_WARDEN_INCIDENT_STORE_H

#include "WardenConfiguration.h"
#include "WardenEvidence.h"

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace warden
{
enum class WardenIncidentOutcome : uint8
{
    Mismatch = 1,
    HistoricalUnavailable = 2
};

inline std::optional<WardenIncidentOutcome> ToIncidentOutcome(
    WardenCheckOutcome outcome)
{
    if (outcome == WardenCheckOutcome::Mismatch)
        return WardenIncidentOutcome::Mismatch;
    return std::nullopt;
}

struct WardenIncidentContext
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
    WardenIncidentOutcome outcome = WardenIncidentOutcome::Mismatch;
};

struct WardenIncidentWindowState
{
    uint32 recentCount = 0;
    uint64 aggressiveUntil = 0;
    uint64 databaseNow = 0;
};

enum class WardenIncidentWriteStatus : uint8
{
    Failed,
    Committed,
    CommittedStateUnavailable
};

struct WardenIncidentWriteResult
{
    WardenIncidentWriteStatus status = WardenIncidentWriteStatus::Failed;
    uint32 recentCount = 0;
    bool permanentBanActive = false;
};

using WardenIncidentSummaryObserver =
    std::function<void(WardenIncidentWriteResult const&)>;

struct WardenIncidentApplication
{
    bool mustKick = true;
    bool durable = false;
    bool summaryKnown = false;
    uint32 recentCount = 0;
    bool permanentBanActive = false;
};

namespace detail
{
inline WardenIncidentWindowState ClassifyRecentIncidentWindow(
    std::vector<uint64> recent, uint32 incidentWindowSeconds,
    uint32 aggressiveThreshold)
{
    WardenIncidentWindowState state;
    if (!incidentWindowSeconds || !aggressiveThreshold)
        return state;
    std::sort(recent.begin(), recent.end(), std::greater<uint64>());
    state.recentCount = static_cast<uint32>(std::min<std::size_t>(
        recent.size(), std::numeric_limits<uint32>::max()));
    if (recent.size() < aggressiveThreshold)
        return state;
    uint64 const thresholdNewest = recent[aggressiveThreshold - 1u];
    uint64 const maximum = std::numeric_limits<uint64>::max();
    state.aggressiveUntil = thresholdNewest > maximum - incidentWindowSeconds ?
        maximum : thresholdNewest + incidentWindowSeconds;
    return state;
}
}

inline bool IsValidWardenIncidentContext(
    WardenIncidentContext const& context)
{
    return context.accountId != 0 && context.realmId != 0 &&
        context.checkId != 0 &&
        context.clientBuild == 15595 &&
        (context.architecture == WardenArchitecture::X86 ||
            context.architecture == WardenArchitecture::X64) &&
        IsPublishedCataWardenLocale(context.locale) &&
        (context.variant == ClientVariant::Stock ||
            context.variant == ClientVariant::Grunt) &&
        IsLegalWardenEvidenceClass(context.checkType,
            context.evidenceClass) &&
        IsActionableEvidenceClass(context.evidenceClass) &&
        context.outcome == WardenIncidentOutcome::Mismatch;
}

inline WardenIncidentWindowState ClassifyIncidentWindow(
    std::vector<uint64> const& timestamps, uint64 now,
    uint32 incidentWindowSeconds, uint32 aggressiveThreshold)
{
    if (!incidentWindowSeconds || !aggressiveThreshold)
        return {};
    std::vector<uint64> recent;
    recent.reserve(timestamps.size());
    for (uint64 timestamp : timestamps)
    {
        if (now < incidentWindowSeconds ||
            timestamp > now - incidentWindowSeconds)
        {
            recent.push_back(timestamp);
        }
    }
    return detail::ClassifyRecentIncidentWindow(std::move(recent),
        incidentWindowSeconds, aggressiveThreshold);
}

inline uint64 RebaseIncidentDeadline(uint64 databaseDeadline,
    uint64 databaseNow, uint64 serverNow)
{
    if (!databaseNow || databaseDeadline <= databaseNow)
        return 0;
    uint64 const remaining = databaseDeadline - databaseNow;
    uint64 const maximum = std::numeric_limits<uint64>::max();
    return serverNow > maximum - remaining ? maximum : serverNow + remaining;
}

inline WardenIncidentApplication ClassifyIncidentWriteResult(
    WardenIncidentWriteResult const& result)
{
    WardenIncidentApplication application;
    if (result.status == WardenIncidentWriteStatus::Failed)
        return application;
    application.durable = true;
    if (result.status == WardenIncidentWriteStatus::CommittedStateUnavailable)
        return application;
    application.summaryKnown = true;
    application.recentCount = result.recentCount;
    application.permanentBanActive = result.permanentBanActive;
    return application;
}

class WardenIncidentStore
{
public:
    static WardenIncidentStore& Instance();
    std::optional<WardenIncidentWindowState> Load(uint32 accountId,
        uint32 incidentWindowSeconds, uint32 aggressiveThreshold) const;
    WardenIncidentWriteResult Record(WardenIncidentContext const& context,
        WardenConfiguration const& configuration,
        WardenIncidentSummaryObserver observer = {}) const;
};
}

#endif
