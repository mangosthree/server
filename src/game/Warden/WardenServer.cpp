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

#include "WardenServer.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <utility>

namespace
{
constexpr uint32 BootstrapDeadlineMs = 30000;
constexpr std::size_t ModuleTransferChunkSize = 500;
constexpr uint8 MaximumTransferAttempts = 2; // Initial transfer plus one retry.

class PlaintextGuard
{
public:
    explicit PlaintextGuard(warden::Bytes& bytes) : m_bytes(bytes) {}
    ~PlaintextGuard()
    {
        if (!m_bytes.empty())
            OPENSSL_cleanse(m_bytes.data(), m_bytes.size());
    }

private:
    warden::Bytes& m_bytes;
};

void AppendUint16(warden::Bytes& bytes, uint16 value)
{
    bytes.push_back(uint8(value));
    bytes.push_back(uint8(value >> 8));
}

void AppendUint32(warden::Bytes& bytes, uint32 value)
{
    bytes.push_back(uint8(value));
    bytes.push_back(uint8(value >> 8));
    bytes.push_back(uint8(value >> 16));
    bytes.push_back(uint8(value >> 24));
}

bool Digest(EVP_MD const* algorithm, uint8 const* data, std::size_t size,
    uint8* output, std::size_t expectedSize)
{
    unsigned int actualSize = 0;
    return EVP_Digest(data, size, output, &actualSize, algorithm, nullptr) ==
            1 &&
        actualSize == expectedSize;
}

bool SameDigest(warden::Digest20 const& left,
    warden::Digest20 const& right)
{
    return CRYPTO_memcmp(left.data(), right.data(), left.size()) == 0;
}

void CleanseProof(warden::ArchitectureProof& proof)
{
    OPENSSL_cleanse(proof.clientToServer.data(),
        proof.clientToServer.size());
    OPENSSL_cleanse(proof.digest.data(), proof.digest.size());
    OPENSSL_cleanse(proof.serverToClient.data(),
        proof.serverToClient.size());
}

uint32 IntervalMilliseconds(uint32 seconds)
{
    if (seconds > std::numeric_limits<uint32>::max() / 1000)
        return std::numeric_limits<uint32>::max();
    return seconds * 1000;
}

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
WardenServer::WardenServer(uint32 build, std::string clientOs,
    std::string locale, WardenConfiguration configuration,
    bool initialAggressive,
    std::shared_ptr<WardenModuleCatalog const> modules,
    std::shared_ptr<WardenCheckCatalog const> checks,
    WardenCryptoContext&& crypto, SendFrame send,
    LifecycleObserver lifecycle, EvidenceBatchObserver evidence)
    : m_build(build), m_clientOs(std::move(clientOs)),
      m_locale(std::move(locale)), m_configuration(configuration),
      m_modules(std::move(modules)), m_checks(std::move(checks)),
      m_crypto(std::move(crypto)), m_send(std::move(send)),
      m_lifecycle(std::move(lifecycle)), m_evidence(std::move(evidence)),
      m_aggressive(initialAggressive),
      m_aggressiveImmediatePending(initialAggressive)
{
}

WardenServer::~WardenServer()
{
    OPENSSL_cleanse(m_challenge.data(), m_challenge.size());
    CleanseProof(m_x86Proof);
    CleanseProof(m_x64Proof);
}

WardenState WardenServer::GetState() const
{
    return m_state;
}

WardenFailure WardenServer::GetFailure() const
{
    return m_failure;
}

void WardenServer::ResetDeadline()
{
    m_remainingDeadlineMs = BootstrapDeadlineMs;
}

bool WardenServer::HasChargedDeadline() const
{
    switch (m_state)
    {
        case WardenState::ArchitectureChallengeSent:
        case WardenState::ModuleUseSent:
        case WardenState::ModuleTransfer:
        case WardenState::ModuleCached:
        case WardenState::ProfileProbeSent:
        case WardenState::InitialChecksSent:
            return true;
        case WardenState::Recurring:
            return m_pendingPlan.has_value();
        default:
            return false;
    }
}

void WardenServer::NotifyLifecycle()
{
    if (!m_lifecycle || m_notifying)
        return;

    m_notifying = true;
    for (;;)
    {
        WardenState const notifiedState = m_state;
        if (notifiedState == WardenState::Failed)
        {
            if (m_terminalNotified)
                break;
            // Set this before invoking external code so re-entry is harmless.
            m_terminalNotified = true;
        }

        m_lifecycle({m_state, m_failure, m_architecture, m_variant,
            m_transferCount});
        if (m_state != WardenState::Failed || m_terminalNotified ||
            notifiedState == WardenState::Failed)
        {
            break;
        }
    }
    m_notifying = false;
}

void WardenServer::Transition(WardenState state)
{
    if (m_state == WardenState::Failed || m_state == state)
        return;
    m_state = state;
    if (HasChargedDeadline())
        ResetDeadline();
    else
        m_remainingDeadlineMs = 0;
    if (state == WardenState::Healthy)
    {
        m_remainingScheduleMs = IntervalMilliseconds(
            m_aggressive ? m_configuration.aggressiveMinSeconds :
                           m_configuration.normalMinSeconds);
    }
    NotifyLifecycle();
}

void WardenServer::Fail(WardenFailure failure)
{
    if (m_state == WardenState::Failed)
        return;
    m_failure = failure;
    m_state = WardenState::Failed;
    m_remainingDeadlineMs = 0;
    m_pendingPlan.reset();
    m_pendingBootstrapString.reset();
    NotifyLifecycle();
}

bool WardenServer::SendPlain(Bytes plain)
{
    WardenCryptoContext candidate = m_crypto.CloneForTransaction();
    if (!candidate.IsInitialized() ||
        !candidate.TransformServerToClient(plain))
    {
        return false;
    }

    EncodedServerFrame frame;
    if (EncodeServerFrame(ByteView(plain), frame) != EncodeStatus::Ok ||
        !m_send || !m_send(frame))
    {
        return false;
    }

    m_crypto = std::move(candidate);
    return true;
}

bool WardenServer::Start()
{
    if (m_state == WardenState::Failed)
        return false;
    if (m_started)
        return true;
    m_started = true;

    if (!m_crypto.IsInitialized() ||
        RAND_bytes(m_challenge.data(), int(m_challenge.size())) != 1)
    {
        Fail(WardenFailure::CryptoFailure);
        return false;
    }

    std::optional<ArchitectureProof> x86 =
        DeriveArchitectureProof(WardenArchitecture::X86, m_challenge);
    std::optional<ArchitectureProof> x64 =
        DeriveArchitectureProof(WardenArchitecture::X64, m_challenge);
    if (!x86 || !x64)
    {
        Fail(WardenFailure::CryptoFailure);
        return false;
    }
    m_x86Proof = *x86;
    m_x64Proof = *x64;
    CleanseProof(*x86);
    CleanseProof(*x64);

    Bytes request = {5};
    request.insert(request.end(), m_challenge.begin(), m_challenge.end());
    if (!SendPlain(std::move(request)))
    {
        Fail(WardenFailure::SendFailure);
        return false;
    }

    Transition(WardenState::ArchitectureChallengeSent);
    return m_state != WardenState::Failed;
}

bool WardenServer::SendModuleUse()
{
    if (!m_module)
        return false;
    Bytes request;
    request.reserve(1 + m_module->moduleId.size() +
        m_module->moduleKey.size() + sizeof(uint32));
    request.push_back(0);
    request.insert(request.end(), m_module->moduleId.begin(),
        m_module->moduleId.end());
    request.insert(request.end(), m_module->moduleKey.begin(),
        m_module->moduleKey.end());
    AppendUint32(request, m_module->declaredSize);
    return SendPlain(std::move(request));
}

bool WardenServer::SendModuleTransfer()
{
    if (!m_module || m_module->container.empty() ||
        m_module->container.size() != m_module->declaredSize ||
        m_transferCount >= MaximumTransferAttempts)
    {
        return false;
    }

    ++m_transferCount;
    if (m_state != WardenState::ModuleTransfer)
        Transition(WardenState::ModuleTransfer);
    if (m_state == WardenState::Failed)
        return false;

    for (std::size_t offset = 0; offset < m_module->container.size();
         offset += ModuleTransferChunkSize)
    {
        std::size_t const count = std::min(ModuleTransferChunkSize,
            m_module->container.size() - offset);
        Bytes request;
        request.reserve(3 + count);
        request.push_back(1);
        AppendUint16(request, uint16(count));
        request.insert(request.end(), m_module->container.begin() + offset,
            m_module->container.begin() + offset + count);
        if (!SendPlain(std::move(request)))
            return false;
    }
    ResetDeadline();
    return true;
}

void WardenServer::HandleArchitectureReply(Bytes const& plain,
    WardenCryptoContext&& candidate)
{
    if (plain.size() != 1 + Digest20{}.size() || plain[0] != 4)
    {
        Fail(WardenFailure::UnexpectedCommand);
        return;
    }

    Digest20 response{};
    std::copy(plain.begin() + 1, plain.end(), response.begin());
    bool x86Match = SameDigest(response, m_x86Proof.digest);
    bool x64Match = SameDigest(response, m_x64Proof.digest);
#ifdef MANGOS_WARDEN_TEST_ACCESS
    if (m_forceArchitectureMatches)
    {
        x86Match = m_forcedX86Match;
        x64Match = m_forcedX64Match;
        m_forceArchitectureMatches = false;
    }
#endif
    OPENSSL_cleanse(response.data(), response.size());

    if (x86Match == x64Match)
    {
        Fail(x86Match ? WardenFailure::ArchitectureProofAmbiguous :
                        WardenFailure::ArchitectureProofMismatch);
        return;
    }

    m_architecture = x86Match ? WardenArchitecture::X86 :
                                WardenArchitecture::X64;
    ArchitectureProof const& proof = x86Match ? m_x86Proof : m_x64Proof;
    // The command-5 reply consumes the old inbound stream. Both replacement
    // directions are installed on the candidate before one atomic commit.
    if (!candidate.InstallDirectionalKeys(
            proof.clientToServer, proof.serverToClient))
    {
        Fail(WardenFailure::CryptoFailure);
        return;
    }
    m_crypto = std::move(candidate);
    OPENSSL_cleanse(m_challenge.data(), m_challenge.size());
    CleanseProof(m_x86Proof);
    CleanseProof(m_x64Proof);

    m_module = m_modules ?
        m_modules->FindExact({m_build, m_architecture}) : nullptr;
    if (!m_module)
    {
        Fail(WardenFailure::UnsupportedProfile);
        return;
    }

    Transition(WardenState::ArchitectureClassified);
    if (m_state == WardenState::Failed)
        return;
    if (!SendModuleUse())
    {
        Fail(WardenFailure::SendFailure);
        return;
    }
    Transition(WardenState::ModuleUseSent);
}

void WardenServer::HandleBootstrapStatus(Bytes const& plain,
    WardenCryptoContext&& candidate)
{
    if (plain.size() != 1)
    {
        Fail(WardenFailure::MalformedPayload);
        return;
    }

    uint8 const status = plain[0];
    if (status == 5)
    {
        Fail(WardenFailure::ModuleLoadFailed);
        return;
    }
    if (status != 0 && status != 1)
    {
        Fail(status == 4 ? WardenFailure::Replay :
                           WardenFailure::UnexpectedCommand);
        return;
    }

    if (status == 1)
    {
        m_crypto = std::move(candidate);
        Transition(WardenState::ModuleCached);
        return;
    }

    if (m_state == WardenState::ModuleTransfer &&
        m_transferCount >= MaximumTransferAttempts)
    {
        Fail(WardenFailure::ModuleDigestMismatch);
        return;
    }

    m_crypto = std::move(candidate);
    if (!SendModuleTransfer())
        Fail(WardenFailure::SendFailure);
}

void WardenServer::HandleBootstrapStringHash(Bytes const& plain,
    WardenCryptoContext&& candidate)
{
    if (!m_pendingBootstrapString ||
        plain.size() != 1 + Digest20{}.size() + Digest32{}.size() ||
        plain[0] != 2)
    {
        Fail(WardenFailure::MalformedPayload);
        return;
    }

    Digest20 expectedSha1{};
    Digest32 expectedSha256{};
    uint8 const* data = reinterpret_cast<uint8 const*>(
        m_pendingBootstrapString->data());
    bool const digested = Digest(EVP_sha1(), data,
            m_pendingBootstrapString->size(), expectedSha1.data(),
            expectedSha1.size()) &&
        Digest(EVP_sha256(), data, m_pendingBootstrapString->size(),
            expectedSha256.data(), expectedSha256.size());
    bool const matches = digested &&
        CRYPTO_memcmp(plain.data() + 1, expectedSha1.data(),
            expectedSha1.size()) == 0 &&
        CRYPTO_memcmp(plain.data() + 1 + expectedSha1.size(),
            expectedSha256.data(), expectedSha256.size()) == 0;
    OPENSSL_cleanse(expectedSha1.data(), expectedSha1.size());
    OPENSSL_cleanse(expectedSha256.data(), expectedSha256.size());
    if (!matches)
    {
        Fail(WardenFailure::MalformedPayload);
        return;
    }

    m_crypto = std::move(candidate);
    m_pendingBootstrapString.reset();
    ResetDeadline();
}

void WardenServer::HandleClientFrame(ByteView worldPayload)
{
    if (m_state == WardenState::Failed || m_state == WardenState::Dormant)
        return;

    DecodedClientFrame decoded;
    if (DecodeClientFrame(worldPayload, decoded) != FrameDecodeStatus::Ok)
    {
        Fail(WardenFailure::MalformedFrame);
        return;
    }

    Bytes plain(decoded.encryptedBody.data(),
        decoded.encryptedBody.data() + decoded.encryptedBody.size());
    PlaintextGuard plaintextGuard(plain);
    WardenCryptoContext candidate = m_crypto.CloneForTransaction();
    if (!candidate.IsInitialized() ||
        !candidate.TransformClientToServer(plain))
    {
        Fail(WardenFailure::CryptoFailure);
        return;
    }
    switch (m_state)
    {
        case WardenState::ArchitectureChallengeSent:
            HandleArchitectureReply(plain, std::move(candidate));
            return;
        case WardenState::ModuleUseSent:
        case WardenState::ModuleTransfer:
            HandleBootstrapStatus(plain, std::move(candidate));
            return;
        case WardenState::ModuleCached:
            if (m_pendingBootstrapString)
                HandleBootstrapStringHash(plain, std::move(candidate));
            else
                Fail(plain.size() == 21 && plain[0] == 4 ?
                    WardenFailure::Replay : WardenFailure::UnexpectedCommand);
            return;
        case WardenState::ArchitectureClassified:
        case WardenState::ModuleHashVerified:
        case WardenState::ModuleInitialized:
        case WardenState::ReadyForWorld:
        case WardenState::ProfileProbeSent:
        case WardenState::ProfileClassified:
        case WardenState::InitialChecksSent:
        case WardenState::Healthy:
        case WardenState::Recurring:
            Fail(WardenFailure::Replay);
            return;
        case WardenState::Dormant:
        case WardenState::Failed:
            return;
    }
}

bool WardenServer::SendBootstrapStringHash(std::string const& text)
{
    if (m_state != WardenState::ModuleCached ||
        m_pendingBootstrapString || text.empty() ||
        text.size() + 1 > MaxEncryptedServerBody)
    {
        return false;
    }

    Bytes request = {2};
    request.insert(request.end(), text.begin(), text.end());
    if (!SendPlain(std::move(request)))
    {
        Fail(WardenFailure::SendFailure);
        return false;
    }
    m_pendingBootstrapString = text;
    ResetDeadline();
    return true;
}

bool WardenServer::BuildPendingPlan(CheckPlanPurpose purpose,
    uint32 confirmationCheckId)
{
    if (!m_planner || m_pendingPlan)
        return false;
    CheckPlan plan;
    if (m_planner->Build(purpose, m_nextRequestId, plan,
            confirmationCheckId) != CheckPlanValidation::Valid)
    {
        return false;
    }
    ++m_nextRequestId;
    m_pendingPlan = std::move(plan);
    switch (purpose)
    {
        case CheckPlanPurpose::ProfileProbe:
            Transition(WardenState::ProfileProbeSent);
            break;
        case CheckPlanPurpose::Initial:
            Transition(WardenState::InitialChecksSent);
            break;
        default:
            Transition(WardenState::Recurring);
            break;
    }
    // A newly owned request always starts a fresh response deadline. A
    // recurring request often keeps the same public state, so Transition()
    // alone cannot establish this invariant.
    if (m_state != WardenState::Failed && HasChargedDeadline())
        ResetDeadline();
    return m_state != WardenState::Failed;
}

bool WardenServer::BeginProfileProbe()
{
    if (!m_checks || m_architecture == WardenArchitecture::Unclassified ||
        m_locale.size() != 4)
    {
        return false;
    }
    WardenCheckProfile const* profile = m_checks->FindProfileExact(
        {m_build, m_architecture, ExactLocale(m_locale),
            ClientVariant::Unclassified});
    if (!profile)
        return false;
    m_planner.emplace(*profile);
    return BuildPendingPlan(CheckPlanPurpose::ProfileProbe);
}

bool WardenServer::ValidateEvidenceBatch(
    WardenEvidenceBatch const& batch) const
{
    if (!m_pendingPlan || batch.requestId != m_pendingPlan->requestId ||
        batch.purpose != m_pendingPlan->purpose ||
        batch.evidence.size() != m_pendingPlan->checks.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < batch.evidence.size(); ++index)
    {
        WardenEvidence const& evidence = batch.evidence[index];
        WardenCheckDefinition const& check = m_pendingPlan->checks[index];
        if (evidence.requestId != batch.requestId ||
            evidence.checkId != GetWardenCheckId(check) ||
            evidence.checkType != GetWardenCheckType(check) ||
            evidence.evidenceClass != check.evidenceClass)
        {
            return false;
        }
    }
    return true;
}

void WardenServer::Update(bool eligible, uint32 diffMs)
{
    if (m_state == WardenState::Failed || m_state == WardenState::Dormant)
        return;
    if (HasChargedDeadline())
    {
        if (diffMs >= m_remainingDeadlineMs)
        {
            Fail(WardenFailure::DeadlineExpired);
            return;
        }
        m_remainingDeadlineMs -= diffMs;
    }

    if (m_state == WardenState::ReadyForWorld)
    {
        if (eligible && !BeginProfileProbe())
            Fail(WardenFailure::UnsupportedProfile);
        return;
    }
    if (!eligible || (m_state != WardenState::Healthy &&
            m_state != WardenState::Recurring) || m_pendingPlan)
    {
        return;
    }

    if (m_confirmationCheckId)
    {
        uint32 const checkId = m_confirmationCheckId;
        m_confirmationCheckId = 0;
        if (!BuildPendingPlan(CheckPlanPurpose::Confirmation, checkId))
            Fail(WardenFailure::UnexpectedCommand);
        return;
    }
    if (m_aggressiveImmediatePending)
    {
        m_aggressiveImmediatePending = false;
        if (!BuildPendingPlan(CheckPlanPurpose::AggressiveImmediate))
            Fail(WardenFailure::UnexpectedCommand);
        return;
    }

    if (diffMs >= m_remainingScheduleMs)
    {
        CheckPlanPurpose const purpose = m_aggressive ?
            CheckPlanPurpose::AggressiveRecurring :
            CheckPlanPurpose::Recurring;
        if (!BuildPendingPlan(purpose))
            Fail(WardenFailure::UnexpectedCommand);
    }
    else
        m_remainingScheduleMs -= diffMs;
}

bool WardenServer::QueueConfirmation(uint32 checkId)
{
    if (!checkId || !m_planner || m_pendingPlan || m_confirmationCheckId ||
        (m_state != WardenState::Healthy &&
            m_state != WardenState::Recurring))
    {
        return false;
    }
    CheckPlan validation;
    if (m_planner->Build(CheckPlanPurpose::Confirmation, m_nextRequestId,
            validation, checkId) != CheckPlanValidation::Valid)
    {
        return false;
    }
    m_confirmationCheckId = checkId;
    return true;
}

void WardenServer::SetAggressive(bool aggressive)
{
    if (m_state == WardenState::Failed)
        return;
    if (aggressive && !m_aggressive)
        m_aggressiveImmediatePending = true;
    m_aggressive = aggressive;
    m_remainingScheduleMs = IntervalMilliseconds(
        aggressive ? m_configuration.aggressiveMinSeconds :
                     m_configuration.normalMinSeconds);
}

#ifdef MANGOS_WARDEN_TEST_ACCESS
void WardenServerTestAccess::AcceptSyntheticModuleHash(
    WardenServer& server, bool valid)
{
    if (server.m_state != WardenState::ModuleCached)
    {
        server.Fail(WardenFailure::UnexpectedCommand);
        return;
    }
    if (!valid)
    {
        server.Fail(WardenFailure::ModuleDigestMismatch);
        return;
    }
    server.Transition(WardenState::ModuleHashVerified);
}

void WardenServerTestAccess::AcceptSyntheticModuleInitialization(
    WardenServer& server, bool valid)
{
    if (server.m_state != WardenState::ModuleHashVerified)
    {
        server.Fail(WardenFailure::UnexpectedCommand);
        return;
    }
    if (!valid)
    {
        server.Fail(WardenFailure::ModuleLoadFailed);
        return;
    }
    server.Transition(WardenState::ModuleInitialized);
    server.Transition(WardenState::ReadyForWorld);
}

void WardenServerTestAccess::CompleteSyntheticProfileProbe(
    WardenServer& server, std::vector<Bytes>&& results)
{
    if (server.m_state != WardenState::ProfileProbeSent ||
        !server.m_pendingPlan ||
        server.m_pendingPlan->purpose != CheckPlanPurpose::ProfileProbe)
    {
        server.Fail(WardenFailure::UnexpectedCommand);
        return;
    }

    ClientVariant const variant =
        ClassifyProfileProbe(server.m_architecture, results);
    server.m_pendingPlan.reset();
    // Preserve recognized-but-unsupported variants in the terminal lifecycle
    // event so the adapter can persist only their bounded audit token.
    server.m_variant = variant;
    if (variant != ClientVariant::Stock && variant != ClientVariant::Grunt)
    {
        server.Fail(WardenFailure::ProfileUnclassified);
        return;
    }
    WardenCheckProfile const* profile = server.m_checks->FindProfileExact(
        {server.m_build, server.m_architecture,
            ExactLocale(server.m_locale), variant});
    if (!profile)
    {
        server.Fail(WardenFailure::UnsupportedProfile);
        return;
    }
    server.m_planner.emplace(*profile);
    server.Transition(WardenState::ProfileClassified);
    if (server.m_state == WardenState::Failed)
        return;
    if (!server.BuildPendingPlan(CheckPlanPurpose::Initial))
        server.Fail(WardenFailure::UnsupportedProfile);
}

std::optional<CheckPlan> WardenServerTestAccess::PendingCheckPlan(
    WardenServer const& server)
{
    return server.m_pendingPlan;
}

void WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
    WardenServer& server, WardenEvidenceBatch&& batch)
{
    // Probe bodies are raw fingerprints consumed by the dedicated classifier,
    // never generic evidence. Keeping the paths disjoint also prevents a probe
    // request from retaining a charged state after its deadline is cleared.
    if (server.m_pendingPlan &&
        server.m_pendingPlan->purpose == CheckPlanPurpose::ProfileProbe)
    {
        server.Fail(WardenFailure::UnexpectedCommand);
        return;
    }

    if (!server.ValidateEvidenceBatch(batch))
    {
        server.Fail(WardenFailure::InvalidEvidenceBatch);
        return;
    }

    CheckPlanPurpose const purpose = batch.purpose;
    bool const cleanInitial = IsCompleteCleanOperatorBatch(batch);
    server.m_pendingPlan.reset();
    server.m_remainingDeadlineMs = 0;
    if (purpose == CheckPlanPurpose::Initial)
    {
        if (cleanInitial)
        {
            server.Transition(WardenState::Healthy);
        }
        else
        {
            // A valid non-clean first batch is not healthy, but it must release
            // the initial request state before policy queues an isolated
            // confirmation from the evidence callback below.
            server.m_remainingScheduleMs = IntervalMilliseconds(
                server.m_aggressive ?
                    server.m_configuration.aggressiveMinSeconds :
                    server.m_configuration.normalMinSeconds);
            server.Transition(WardenState::Recurring);
        }
    }
    else
    {
        server.m_remainingScheduleMs = IntervalMilliseconds(
            server.m_aggressive ?
                server.m_configuration.aggressiveMinSeconds :
                server.m_configuration.normalMinSeconds);
    }

    if (server.m_state == WardenState::Failed)
        return;

    // State and pending-plan ownership are committed before external policy
    // code observes the evidence and potentially queues a confirmation.
    if (server.m_evidence)
        server.m_evidence(batch);
}

bool WardenServerTestAccess::SendBootstrapStringHash(
    WardenServer& server, std::string const& text)
{
    return server.SendBootstrapStringHash(text);
}

void WardenServerTestAccess::ForceNextArchitectureMatches(
    WardenServer& server, bool x86, bool x64)
{
    server.m_forceArchitectureMatches = true;
    server.m_forcedX86Match = x86;
    server.m_forcedX64Match = x64;
}

bool WardenServerTestAccess::PreviewCommittedClientPlaintext(
    WardenServer const& server, ByteView encryptedBody, Bytes& plain)
{
    plain.clear();
    if (encryptedBody.empty() || !encryptedBody.data())
        return false;
    plain.assign(encryptedBody.data(),
        encryptedBody.data() + encryptedBody.size());
    WardenCryptoContext candidate = server.m_crypto.CloneForTransaction();
    return candidate.IsInitialized() &&
        candidate.TransformClientToServer(plain);
}
#endif
}
