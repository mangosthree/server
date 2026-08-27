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

#ifndef MANGOS_WARDEN_CONFIGURATION_H
#define MANGOS_WARDEN_CONFIGURATION_H

#include "Platform/Define.h"

#include <memory>

namespace warden
{
struct WardenRuntimeSnapshot;
/** Controls which confirmed failures may affect a connected account. */
enum class WardenEnforcementMode : uint8
{
    Observe = 0,
    Kick = 1,
    KickAndBan = 2
};

/** Largest rolling incident window accepted from configuration (one year). */
uint32 constexpr MaxWardenIncidentWindowSeconds = 31536000;

/** Prevents unsafe or operationally unbounded database window arithmetic. */
inline bool IsValidWardenIncidentWindow(uint32 seconds)
{
    return seconds > 0 && seconds <= MaxWardenIncidentWindowSeconds;
}

/** Untrusted values read directly from mangosd.conf. */
struct WardenRawConfiguration
{
    // Both compiled Cata modules remain StaticVerified, so the only safe
    // missing-key/default policy is observe-only until G4 promotes them.
    uint32 enforcementMode = 0;
    // When false, missing/unsupported profiles and protocol lifecycle failures
    // disengage Warden for the session instead of enforcing admission.
    bool requireExactProfile = true;
    uint32 normalMinSeconds = 30;
    uint32 normalMaxSeconds = 60;
    uint32 aggressiveMinSeconds = 10;
    uint32 aggressiveMaxSeconds = 20;
    uint32 aggressiveThreshold = 5;
    uint32 banThreshold = 10;
    uint32 incidentWindowSeconds = 900;
};

/** Values safe for planners and enforcement policy to consume. */
struct WardenConfiguration
{
    WardenEnforcementMode enforcementMode =
        WardenEnforcementMode::Observe;
    // This is an operational admission gate only; confirmed cheat evidence
    // remains actionable regardless of its value.
    bool requireExactProfile = true;
    uint32 normalMinSeconds = 30;
    uint32 normalMaxSeconds = 60;
    uint32 aggressiveMinSeconds = 10;
    uint32 aggressiveMaxSeconds = 20;
    uint32 aggressiveThreshold = 5;
    uint32 banThreshold = 10;
    uint32 incidentWindowSeconds = 900;
};

/** Secret-free incident history carried across an authenticated login queue. */
struct WardenAdmissionHistory
{
    uint32 recentIncidentCount = 0;
    // Absolute server deadline so queue residence consumes eligibility.
    uint64 aggressiveUntilServer = 0;
    bool incidentHistoryLoaded = false;
};

/** One coherent Attach-time policy and the history classified under it. */
struct WardenAdmissionContext
{
    /** Exact manager-published generation captured by WorldGateway::Attach. */
    std::shared_ptr<WardenRuntimeSnapshot const> runtimeSnapshot;
    WardenConfiguration configuration;
    WardenAdmissionHistory history;
};

/** Applies the current admission policy to previously loaded history. */
bool IsWardenAdmissionAggressive(WardenAdmissionHistory const& history,
    WardenConfiguration const& configuration, uint64 nowServer);

/** Observe and missing/expired history always select normal admission cadence. */
bool ShouldUseAggressiveWardenAdmission(
    WardenAdmissionHistory const& history,
    WardenConfiguration const& configuration, uint64 nowServer);

/** Invalid groups are reported separately so the loader can explain repairs. */
enum class WardenConfigurationCorrection : uint32
{
    None = 0,
    EnforcementMode = 1u << 0,
    NormalInterval = 1u << 1,
    AggressiveInterval = 1u << 2,
    Thresholds = 1u << 3,
    IncidentWindow = 1u << 4
};

/** Safe snapshot plus an explicit mask of repaired operator inputs. */
struct WardenConfigurationNormalization
{
    WardenConfiguration value;
    WardenConfigurationCorrection corrections =
        WardenConfigurationCorrection::None;
};

/**
 * Replaces each invalid logical group with its approved safe defaults while
 * preserving every independent valid group.
 */
WardenConfigurationNormalization NormalizeWardenConfiguration(
    WardenRawConfiguration const& raw);

bool HasWardenConfigurationCorrection(
    WardenConfigurationCorrection corrections,
    WardenConfigurationCorrection correction);
}

#endif
