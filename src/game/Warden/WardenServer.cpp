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
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <utility>
#include <variant>
#include <vector>

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

bool SameBytes(warden::Bytes const& left, warden::Bytes const& right)
{
    return left.size() == right.size() &&
        (left.empty() || CRYPTO_memcmp(
            left.data(), right.data(), left.size()) == 0);
}

bool BuildEvidenceBatch(warden::CheckPlan const& plan,
    warden::CheckBatchResult const& decoded,
    warden::WardenEvidenceBatch& output)
{
    if (plan.purpose == warden::CheckPlanPurpose::ProfileProbe ||
        decoded.checks.size() != plan.checks.size())
    {
        return false;
    }

    warden::WardenEvidenceBatch batch;
    batch.requestId = plan.requestId;
    batch.purpose = plan.purpose;
    batch.evidence.reserve(plan.checks.size());
    for (std::size_t index = 0; index < plan.checks.size(); ++index)
    {
        warden::WardenCheckDefinition const& definition = plan.checks[index];
        warden::WardenEvidence evidence;
        evidence.requestId = plan.requestId;
        evidence.checkId = warden::GetWardenCheckId(definition);
        evidence.checkType = warden::GetWardenCheckType(definition);
        evidence.evidenceClass = definition.evidenceClass;

        if (std::holds_alternative<warden::TimingCheckProfile>(
                definition.payload))
        {
            warden::TimingResult const* result =
                std::get_if<warden::TimingResult>(&decoded.checks[index]);
            if (!result)
                return false;
            evidence.outcome = result->stable ?
                warden::WardenCheckOutcome::Stable :
                warden::WardenCheckOutcome::Unstable;
            evidence.clientTick = result->clientTick;
        }
        else if (warden::LuaCheckProfile const* luaExpected =
                     std::get_if<warden::LuaCheckProfile>(
                         &definition.payload))
        {
            warden::LuaResult const* result =
                std::get_if<warden::LuaResult>(&decoded.checks[index]);
            if (!result)
                return false;
            evidence.outcome = result->status ==
                    warden::LuaResultStatus::Unavailable ?
                warden::WardenCheckOutcome::Unavailable :
                (result->text == luaExpected->expectedText ?
                        warden::WardenCheckOutcome::Match :
                        warden::WardenCheckOutcome::Mismatch);
        }
        else if (warden::MpqCheckProfile const* mpqExpected =
                     std::get_if<warden::MpqCheckProfile>(
                         &definition.payload))
        {
            warden::MpqResult const* result =
                std::get_if<warden::MpqResult>(&decoded.checks[index]);
            if (!result)
                return false;
            bool const matches = CRYPTO_memcmp(result->digest.data(),
                mpqExpected->expectedSha1.data(), result->digest.size()) == 0;
            evidence.outcome = result->status ==
                    warden::MpqResultStatus::Unavailable ?
                warden::WardenCheckOutcome::Unavailable :
                (matches ? warden::WardenCheckOutcome::Match :
                           warden::WardenCheckOutcome::Mismatch);
        }
        else
        {
            warden::MemCheckProfile const* memExpected =
                std::get_if<warden::MemCheckProfile>(&definition.payload);
            warden::MemResult const* result =
                std::get_if<warden::MemResult>(&decoded.checks[index]);
            if (!memExpected || !result)
                return false;
            evidence.outcome = result->status ==
                    warden::MemResultStatus::Unavailable ?
                warden::WardenCheckOutcome::Unavailable :
                (SameBytes(result->actualBytes, memExpected->expectedBytes) ?
                        warden::WardenCheckOutcome::Match :
                        warden::WardenCheckOutcome::Mismatch);
        }
        batch.evidence.push_back(evidence);
    }

    output = std::move(batch);
    return true;
}

bool ExtractProfileProbeResults(warden::CheckPlan const& plan,
    warden::CheckBatchResult& decoded, std::vector<warden::Bytes>& output)
{
    if (plan.purpose != warden::CheckPlanPurpose::ProfileProbe ||
        decoded.checks.size() != plan.checks.size())
    {
        return false;
    }

    std::vector<warden::Bytes> results;
    results.reserve(decoded.checks.size());
    for (std::size_t index = 0; index < decoded.checks.size(); ++index)
    {
        if (!std::holds_alternative<warden::MemCheckProfile>(
                plan.checks[index].payload))
        {
            return false;
        }
        warden::MemResult* result =
            std::get_if<warden::MemResult>(&decoded.checks[index]);
        if (!result)
            return false;
        if (result->status == warden::MemResultStatus::Success)
            results.push_back(std::move(result->actualBytes));
        else
            results.emplace_back();
    }
    output = std::move(results);
    return true;
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

bool WardenServer::SelectScheduleMilliseconds(uint32& milliseconds) const
{
    uint32 const minimum = m_aggressive ?
        m_configuration.aggressiveMinSeconds :
        m_configuration.normalMinSeconds;
    uint32 const maximum = m_aggressive ?
        m_configuration.aggressiveMaxSeconds :
        m_configuration.normalMaxSeconds;
    if (!minimum || minimum > maximum)
        return false;

    uint32 seconds = minimum;
#ifdef MANGOS_WARDEN_TEST_ACCESS
    if (m_scheduleSecondsSelector)
    {
        seconds = m_scheduleSecondsSelector(minimum, maximum);
        if (seconds < minimum || seconds > maximum)
            return false;
    }
    else
#endif
    if (minimum != maximum)
    {
        // Scan cadence is externally observable. Draw it from OpenSSL instead
        // of the shared gameplay PRNG, whose state can leak through rolls.
        uint64 const span = uint64(maximum) - minimum + 1;
        uint64 const sampleSpace = uint64(std::numeric_limits<uint32>::max()) + 1;
        uint64 const unbiasedLimit = sampleSpace - (sampleSpace % span);
        uint32 sample = 0;
        do
        {
            if (RAND_bytes(reinterpret_cast<unsigned char*>(&sample),
                    int(sizeof(sample))) != 1)
            {
                return false;
            }
        }
        while (uint64(sample) >= unbiasedLimit);
        seconds = minimum + uint32(uint64(sample) % span);
    }

    milliseconds = IntervalMilliseconds(seconds);
    return true;
}

bool WardenServer::HasChargedDeadline() const
{
    switch (m_state)
    {
        case WardenState::ArchitectureChallengeSent:
        case WardenState::ModuleUseSent:
        case WardenState::ModuleTransfer:
        case WardenState::ModuleCached:
        case WardenState::ProvisionalTimingProbeSent:
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
        if (!SelectScheduleMilliseconds(m_remainingScheduleMs))
        {
            Fail(WardenFailure::CryptoFailure);
            return;
        }
    }
    NotifyLifecycle();
}

void WardenServer::Fail(WardenFailure failure)
{
    if (m_state == WardenState::Failed)
        return;
    if (m_module && m_module->operatingMode ==
            ModuleOperatingMode::CompatibilityProbeOnly &&
        failure != WardenFailure::None)
    {
        failure = WardenFailure::CompatibilityProbeFailed;
    }
    m_failure = failure;
    m_state = WardenState::Failed;
    m_remainingDeadlineMs = 0;
    m_pendingPlan.reset();
    m_checkXorKey.reset();
    NotifyLifecycle();
}

bool WardenServer::SendPlain(Bytes plain)
{
    if (m_inSendCallback)
        return false;
    WardenCryptoContext candidate = m_crypto.CloneForTransaction();
    if (!candidate.IsInitialized() ||
        !candidate.TransformServerToClient(plain))
    {
        return false;
    }

    EncodedServerFrame frame;
    if (EncodeServerFrame(ByteView(plain), frame) != EncodeStatus::Ok ||
        !m_send)
    {
        return false;
    }

    WardenState const stateBeforeSend = m_state;
    m_inSendCallback = true;
    bool const sent = m_send(frame);
    m_inSendCallback = false;
    if (!sent || m_state != stateBeforeSend)
        return false;

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

bool WardenServer::SendModuleHashRequest()
{
    if (!m_module)
        return false;
    Bytes request;
    if (EncodeModuleHashRequest(*m_module, request) != EncodeStatus::Ok ||
        !SendPlain(std::move(request)))
    {
        return false;
    }
    ResetDeadline();
    return true;
}

bool WardenServer::SendModuleInitialization()
{
    if (!m_module)
        return false;
    Bytes initialization;
    return EncodeModuleInitialization(*m_module, initialization) ==
            EncodeStatus::Ok &&
        SendPlain(std::move(initialization));
}

WardenFailure WardenServer::SendDeferredFilesystemInitialization()
{
    if (!m_module)
        return WardenFailure::UnsupportedProfile;
    Bytes initialization;
    if (EncodeDeferredFilesystemInitialization(
            *m_module, m_variant, initialization) != EncodeStatus::Ok)
    {
        return WardenFailure::UnsupportedProfile;
    }
    return SendPlain(std::move(initialization)) ? WardenFailure::None :
        WardenFailure::SendFailure;
}

bool WardenServer::SendCompatibilityTimingProbe()
{
    if (!m_module || !m_checkXorKey)
        return false;
    Bytes request;
    if (EncodeCompatibilityTimingProbe(
            m_module->abi, *m_checkXorKey, request) != EncodeStatus::Ok)
    {
        return false;
    }
    m_checkXorKey.reset();
    return SendPlain(std::move(request));
}

bool WardenServer::HasCompleteSelectedProfiles() const
{
    if (!m_module || !m_checks)
        return false;
    if (m_module->operatingMode == ModuleOperatingMode::CompatibilityProbeOnly)
        return true;
    if (m_module->operatingMode != ModuleOperatingMode::Full ||
        m_locale.size() != 4)
    {
        return false;
    }

    std::array<char, 4> locale{};
    std::copy(m_locale.begin(), m_locale.end(), locale.begin());
    std::vector<ClientVariant> variants = {
        ClientVariant::Unclassified, ClientVariant::Stock,
        ClientVariant::Grunt};
    if (m_architecture == WardenArchitecture::X86)
        variants.push_back(ClientVariant::LegacyGrunt);
    for (ClientVariant variant : variants)
    {
        if (!m_checks->FindProfileExact(
                {m_build, m_architecture, locale, variant}))
        {
            return false;
        }
    }
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
    // Exact locale profiles are meaningful only after command 5 proves the
    // architecture. Probe-only modules deliberately publish no check rows.
    if (!HasCompleteSelectedProfiles())
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
        if (m_state != WardenState::Failed && !SendModuleHashRequest())
            Fail(WardenFailure::SendFailure);
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

void WardenServer::HandleModuleHashResult(Bytes const& plain,
    WardenCryptoContext&& candidate)
{
    if (!m_module ||
        DecodeModuleHashResult(*m_module, ByteView(plain)) !=
            ModuleDecodeStatus::Ok)
    {
        Fail(WardenFailure::ModuleDigestMismatch);
        return;
    }

    std::optional<WardenCheckXorKey> checkXorKey =
        candidate.InstallModuleDirectionalKeys(
            m_module->rekey.clientToServer, m_module->rekey.serverToClient);
    if (!checkXorKey)
    {
        Fail(WardenFailure::CryptoFailure);
        return;
    }

    m_crypto = std::move(candidate);
    m_checkXorKey = *checkXorKey;
    Transition(WardenState::ModuleHashVerified);
    if (m_state == WardenState::Failed || !SendModuleInitialization())
    {
        Fail(WardenFailure::SendFailure);
        return;
    }

    Transition(WardenState::ModuleInitialized);
    if (m_state == WardenState::Failed)
        return;
    if (m_module->operatingMode == ModuleOperatingMode::Full)
    {
        Transition(WardenState::ReadyForWorld);
        return;
    }
    if (!SendCompatibilityTimingProbe())
    {
        Fail(WardenFailure::SendFailure);
        return;
    }
    Transition(WardenState::ProvisionalTimingProbeSent);
}

void WardenServer::HandleCompatibilityTimingResult(Bytes const& plain,
    WardenCryptoContext&& candidate)
{
    if (!m_module)
    {
        Fail(WardenFailure::CompatibilityProbeFailed);
        return;
    }

    uint32 clientTick = 0;
    ModuleDecodeStatus const result = DecodeCompatibilityTimingResult(
        m_module->abi, ByteView(plain), clientTick);
    OPENSSL_cleanse(&clientTick, sizeof(clientTick));
    if (result != ModuleDecodeStatus::Ok)
    {
        Fail(WardenFailure::CompatibilityProbeFailed);
        return;
    }

    m_crypto = std::move(candidate);
    Transition(WardenState::ProvisionalValidated);
}

void WardenServer::HandleClientFrame(ByteView worldPayload)
{
    if (m_inSendCallback)
    {
        Fail(WardenFailure::UnexpectedCommand);
        return;
    }
    if (m_state == WardenState::Failed || m_state == WardenState::Dormant)
        return;
    if (m_state == WardenState::ProvisionalValidated)
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
            HandleModuleHashResult(plain, std::move(candidate));
            return;
        case WardenState::ProvisionalTimingProbeSent:
            HandleCompatibilityTimingResult(plain, std::move(candidate));
            return;
        case WardenState::ProfileProbeSent:
        case WardenState::InitialChecksSent:
            HandleCheckResult(plain, std::move(candidate));
            return;
        case WardenState::Recurring:
            if (m_pendingPlan)
                HandleCheckResult(plain, std::move(candidate));
            else
                Fail(WardenFailure::Replay);
            return;
        case WardenState::ArchitectureClassified:
        case WardenState::ModuleHashVerified:
        case WardenState::ModuleInitialized:
        case WardenState::ReadyForWorld:
        case WardenState::ProfileClassified:
        case WardenState::Healthy:
            Fail(WardenFailure::Replay);
            return;
        case WardenState::ProvisionalValidated:
        case WardenState::Dormant:
        case WardenState::Failed:
            return;
    }
}

bool WardenServer::BuildPendingPlan(CheckPlanPurpose purpose,
    uint32 confirmationCheckId)
{
    if (!m_module || !m_checkXorKey || !m_planner || m_pendingPlan)
        return false;
    CheckPlan plan;
    if (m_planner->Build(purpose, m_nextRequestId, plan,
            confirmationCheckId) != CheckPlanValidation::Valid)
    {
        return false;
    }

    Bytes request;
    if (EncodeCheckRequest(*m_module, *m_checkXorKey, plan, request) !=
            EncodeStatus::Ok)
    {
        return false;
    }
    if (!SendPlain(std::move(request)))
    {
        Fail(WardenFailure::SendFailure);
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

void WardenServer::HandleCheckResult(Bytes const& plain,
    WardenCryptoContext&& candidate)
{
    if (!m_module || m_module->operatingMode != ModuleOperatingMode::Full ||
        !m_pendingPlan)
    {
        Fail(WardenFailure::UnexpectedCommand);
        return;
    }

    CheckBatchResult decoded;
    if (DecodeCheckResult(m_module->abi, ByteView(plain), *m_pendingPlan,
            decoded) != DecodeStatus::Ok)
    {
        CleanseCheckBatchResult(decoded);
        Fail(WardenFailure::MalformedPayload);
        return;
    }

    bool const profileProbe =
        m_pendingPlan->purpose == CheckPlanPurpose::ProfileProbe;
    std::vector<Bytes> probeResults;
    WardenEvidenceBatch evidence;
    bool const prepared = profileProbe ?
        ExtractProfileProbeResults(*m_pendingPlan, decoded, probeResults) :
        BuildEvidenceBatch(*m_pendingPlan, decoded, evidence);
    CleanseCheckBatchResult(decoded);
    if (!prepared)
    {
        Fail(WardenFailure::InvalidEvidenceBatch);
        return;
    }

    // Commit the inbound RC4 stream only after the complete module response
    // and its typed semantic projection have both validated.
    m_crypto = std::move(candidate);
    if (profileProbe)
        CompleteProfileProbe(std::move(probeResults));
    else
        CompleteEvidenceBatch(std::move(evidence));
}

void WardenServer::CompleteProfileProbe(std::vector<Bytes>&& results)
{
    if (m_state != WardenState::ProfileProbeSent || !m_pendingPlan ||
        m_pendingPlan->purpose != CheckPlanPurpose::ProfileProbe)
    {
        for (Bytes& result : results)
        {
            if (!result.empty())
                OPENSSL_cleanse(result.data(), result.size());
        }
        Fail(WardenFailure::UnexpectedCommand);
        return;
    }

    ClientVariant const variant =
        ClassifyProfileProbe(m_architecture, results);
    m_pendingPlan.reset();
    m_remainingDeadlineMs = 0;
    // Preserve recognized-but-unsupported variants in the terminal lifecycle
    // event so the adapter can persist only their bounded audit token.
    m_variant = variant;
    bool const selectable = variant == ClientVariant::Stock ||
        variant == ClientVariant::Grunt ||
        (m_architecture == WardenArchitecture::X86 &&
            variant == ClientVariant::LegacyGrunt);
    if (!selectable)
    {
        Fail(WardenFailure::ProfileUnclassified);
        return;
    }
    WardenCheckProfile const* profile = m_checks->FindProfileExact(
        {m_build, m_architecture, ExactLocale(m_locale), variant});
    if (!profile)
    {
        Fail(WardenFailure::UnsupportedProfile);
        return;
    }
    m_planner.emplace(*profile);
    Transition(WardenState::ProfileClassified);
    if (m_state == WardenState::Failed)
        return;
    // The initial x86 command 3 deliberately withholds filesystem callbacks.
    // Install them only after the fourth probe proves the exact current-Grunt
    // adapter; stock and legacy clients never receive this record.
    if (m_architecture == WardenArchitecture::X86 &&
        variant == ClientVariant::Grunt)
    {
        WardenFailure const failure =
            SendDeferredFilesystemInitialization();
        if (failure != WardenFailure::None)
        {
            Fail(failure);
            return;
        }
    }
    if (!BuildPendingPlan(CheckPlanPurpose::Initial))
        Fail(WardenFailure::UnsupportedProfile);
}

void WardenServer::CompleteEvidenceBatch(WardenEvidenceBatch&& batch)
{
    if (m_pendingPlan &&
        m_pendingPlan->purpose == CheckPlanPurpose::ProfileProbe)
    {
        Fail(WardenFailure::UnexpectedCommand);
        return;
    }
    if (!ValidateEvidenceBatch(batch))
    {
        Fail(WardenFailure::InvalidEvidenceBatch);
        return;
    }

    CheckPlanPurpose const purpose = batch.purpose;
    bool const cleanInitial = IsCompleteCleanOperatorBatch(batch);
    m_pendingPlan.reset();
    m_remainingDeadlineMs = 0;
    if (purpose == CheckPlanPurpose::Initial)
    {
        if (cleanInitial)
        {
            Transition(WardenState::Healthy);
        }
        else
        {
            // Release a valid non-clean first request before policy can queue
            // an isolated confirmation from the evidence callback below.
            if (!SelectScheduleMilliseconds(m_remainingScheduleMs))
            {
                Fail(WardenFailure::CryptoFailure);
                return;
            }
            Transition(WardenState::Recurring);
        }
    }
    else
    {
        if (!SelectScheduleMilliseconds(m_remainingScheduleMs))
        {
            Fail(WardenFailure::CryptoFailure);
            return;
        }
    }

    if (m_state == WardenState::Failed)
        return;
    // Ownership and state commit before external policy can queue follow-up.
    if (m_evidence)
        m_evidence(batch);
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
    if (m_inSendCallback)
        return;
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
    if (m_inSendCallback)
        return false;
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
    if (m_inSendCallback)
        return;
    if (m_state == WardenState::Failed)
        return;
    if (aggressive && !m_aggressive)
        m_aggressiveImmediatePending = true;
    m_aggressive = aggressive;
    if (!SelectScheduleMilliseconds(m_remainingScheduleMs))
        Fail(WardenFailure::CryptoFailure);
}

#ifdef MANGOS_WARDEN_TEST_ACCESS
void WardenServerTestAccess::CompleteSyntheticProfileProbe(
    WardenServer& server, std::vector<Bytes>&& results)
{
    server.CompleteProfileProbe(std::move(results));
}

std::optional<CheckPlan> WardenServerTestAccess::PendingCheckPlan(
    WardenServer const& server)
{
    return server.m_pendingPlan;
}

void WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
    WardenServer& server, WardenEvidenceBatch&& batch)
{
    server.CompleteEvidenceBatch(std::move(batch));
}

void WardenServerTestAccess::ForceNextArchitectureMatches(
    WardenServer& server, bool x86, bool x64)
{
    server.m_forceArchitectureMatches = true;
    server.m_forcedX86Match = x86;
    server.m_forcedX64Match = x64;
}

WardenFailure WardenServerTestAccess::TryDeferredFilesystemInitialization(
    WardenServer& server)
{
    return server.SendDeferredFilesystemInitialization();
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

void WardenServerTestAccess::SetScheduleSecondsSelector(
    WardenServer& server,
    std::function<uint32(uint32, uint32)> selector)
{
    server.m_scheduleSecondsSelector = std::move(selector);
}

uint32 WardenServerTestAccess::RemainingScheduleMs(
    WardenServer const& server)
{
    return server.m_remainingScheduleMs;
}
#endif
}
