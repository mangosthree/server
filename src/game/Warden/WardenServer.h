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

#ifndef MANGOS_WARDEN_SERVER_H
#define MANGOS_WARDEN_SERVER_H

#include "WardenCheckPlanner.h"
#include "WardenCryptoContext.h"
#include "WardenEvidence.h"
#include "WardenModuleCatalog.h"
#include "WardenPacketCodec.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace warden
{
/**
 * Observable architecture-first state; Failed is absorbing. Healthy is the
 * one-shot initial-health edge and Recurring is the steady active state.
 */
enum class WardenState : uint8
{
    Dormant,
    ArchitectureChallengeSent,
    ArchitectureClassified,
    ModuleUseSent,
    ModuleTransfer,
    ModuleCached,
    ModuleHashVerified,
    ModuleInitialized,
    ReadyForWorld,
    ProvisionalTimingProbeSent,
    ProvisionalValidated,
    ProfileProbeSent,
    ProfileClassified,
    InitialChecksSent,
    Healthy,
    Recurring,
    Failed
};

/** Operational terminal reasons. None of these is actionable evidence. */
enum class WardenFailure : uint8
{
    None,
    UnsupportedProfile,
    MalformedFrame,
    MalformedPayload,
    UnexpectedCommand,
    Replay,
    ArchitectureProofMismatch,
    ArchitectureProofAmbiguous,
    ModuleDigestMismatch,
    ModuleLoadFailed,
    CompatibilityProbeFailed,
    DeadlineExpired,
    CryptoFailure,
    SendFailure,
    ProfileUnclassified,
    InvalidEvidenceBatch
};

/** Secret-free transition fact for the session lifecycle adapter. */
struct WardenLifecycleEvent
{
    WardenState state = WardenState::Dormant;
    WardenFailure failure = WardenFailure::None;
    WardenArchitecture architecture = WardenArchitecture::Unclassified;
    ClientVariant variant = ClientVariant::Unclassified;
    uint8 transferCount = 0;
};

// All callbacks run synchronously inside WardenServer methods. They may
// re-enter the server, but must defer destroying or resetting its owner until
// the initiating Start, Update or HandleClientFrame call has returned.
using SendFrame = std::function<bool(EncodedServerFrame const&)>;
using LifecycleObserver = std::function<void(WardenLifecycleEvent const&)>;
using EvidenceBatchObserver =
    std::function<void(WardenEvidenceBatch const&)>;

class WardenManager;
#ifdef MANGOS_WARDEN_TEST_ACCESS
class WardenServerTestAccess;
#endif

/**
 * Owns one session's complete Warden parse, crypto and scheduling transaction.
 *
 * The public surface accepts complete world payloads only. Module selection is
 * impossible until command 5 authenticates exactly one architecture. The
 * selected module ABI then owns every request and result layout.
 */
class WardenServer
{
public:
    ~WardenServer();
    bool Start();
    void HandleClientFrame(ByteView worldPayload);
    void Update(bool eligible, uint32 diffMs);
    WardenState GetState() const;
    WardenFailure GetFailure() const;
    bool QueueConfirmation(uint32 checkId);
    void SetAggressive(bool aggressive);

private:
    friend class WardenManager;
#ifdef MANGOS_WARDEN_TEST_ACCESS
    friend class WardenServerTestAccess;
#endif

    WardenServer(uint32 build, std::string clientOs, std::string locale,
        WardenConfiguration configuration, bool initialAggressive,
        std::shared_ptr<WardenModuleCatalog const> modules,
        std::shared_ptr<WardenCheckCatalog const> checks,
        WardenCryptoContext&& crypto, SendFrame send,
        LifecycleObserver lifecycle, EvidenceBatchObserver evidence);

    bool SendPlain(Bytes plain);
    bool SendModuleUse();
    bool SendModuleTransfer();
    bool SendModuleHashRequest();
    bool SendModuleInitialization();
    bool SendCompatibilityTimingProbe();
    bool HasCompleteSelectedProfiles() const;
    bool BeginProfileProbe();
    bool BuildPendingPlan(CheckPlanPurpose purpose,
        uint32 confirmationCheckId = 0);
    void HandleCheckResult(Bytes const& plain,
        WardenCryptoContext&& candidate);
    void CompleteProfileProbe(std::vector<Bytes>&& results);
    void CompleteEvidenceBatch(WardenEvidenceBatch&& batch);
    bool ValidateEvidenceBatch(WardenEvidenceBatch const& batch) const;
    bool SelectScheduleMilliseconds(uint32& milliseconds) const;
    bool HasChargedDeadline() const;
    void ResetDeadline();
    void Transition(WardenState state);
    void Fail(WardenFailure failure);
    void NotifyLifecycle();
    void HandleArchitectureReply(Bytes const& plain,
        WardenCryptoContext&& candidate);
    void HandleBootstrapStatus(Bytes const& plain,
        WardenCryptoContext&& candidate);
    void HandleModuleHashResult(Bytes const& plain,
        WardenCryptoContext&& candidate);
    void HandleCompatibilityTimingResult(Bytes const& plain,
        WardenCryptoContext&& candidate);

    uint32 m_build = 0;
    std::string m_clientOs;
    std::string m_locale;
    WardenConfiguration m_configuration;
    std::shared_ptr<WardenModuleCatalog const> m_modules;
    std::shared_ptr<WardenCheckCatalog const> m_checks;
    ModuleProfile const* m_module = nullptr;
    WardenCryptoContext m_crypto;
    SendFrame m_send;
    LifecycleObserver m_lifecycle;
    EvidenceBatchObserver m_evidence;
    WardenState m_state = WardenState::Dormant;
    WardenFailure m_failure = WardenFailure::None;
    WardenArchitecture m_architecture = WardenArchitecture::Unclassified;
    ClientVariant m_variant = ClientVariant::Unclassified;
    Key16 m_challenge{};
    ArchitectureProof m_x86Proof{};
    ArchitectureProof m_x64Proof{};
    std::optional<WardenCheckXorKey> m_checkXorKey;
    std::optional<WardenCheckPlanner> m_planner;
    std::optional<CheckPlan> m_pendingPlan;
    uint32 m_nextRequestId = 1;
    uint32 m_remainingDeadlineMs = 0;
    uint32 m_remainingScheduleMs = 0;
    uint32 m_confirmationCheckId = 0;
    uint8 m_transferCount = 0;
    bool m_started = false;
    bool m_aggressive = false;
    bool m_aggressiveImmediatePending = false;
    bool m_terminalNotified = false;
    bool m_notifying = false;
    bool m_inSendCallback = false;
#ifdef MANGOS_WARDEN_TEST_ACCESS
    bool m_forceArchitectureMatches = false;
    bool m_forcedX86Match = false;
    bool m_forcedX64Match = false;
    std::function<uint32(uint32, uint32)> m_scheduleSecondsSelector;
#endif
};

#ifdef MANGOS_WARDEN_TEST_ACCESS
/**
 * Friend-only G1 seam. It injects already validated facts and never defines a
 * production module command, hash, initialization or check-result ABI.
 */
class WardenServerTestAccess
{
public:
    static void CompleteSyntheticProfileProbe(
        WardenServer& server, std::vector<Bytes>&& results);
    static std::optional<CheckPlan> PendingCheckPlan(
        WardenServer const& server);
    static void CompleteSyntheticEvidenceBatch(
        WardenServer& server, WardenEvidenceBatch&& batch);
    static void ForceNextArchitectureMatches(
        WardenServer& server, bool x86, bool x64);
    static bool PreviewCommittedClientPlaintext(WardenServer const& server,
        ByteView encryptedBody, Bytes& plain);
    static void SetScheduleSecondsSelector(WardenServer& server,
        std::function<uint32(uint32, uint32)> selector);
    static uint32 RemainingScheduleMs(WardenServer const& server);
};
#endif
}

#endif
