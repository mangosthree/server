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

#include "WardenCheckFixtures.h"
#include "WardenManager.h"
#include "WardenServer.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
/*
 * WardenServerTestAccess is a friend-only seam, not a module ABI. G1 tests feed
 * it already-validated post-bootstrap facts so state ownership, planner
 * forwarding and callback lifetime can be fixed before G2 reveals the real
 * module hash, initialization and result wire layouts. The production frame
 * path must replace these hooks once those custody-pinned codecs exist.
 */
bool Digest(EVP_MD const* algorithm, warden::ByteView input, uint8* output,
    std::size_t expectedSize)
{
    unsigned int actualSize = 0;
    return EVP_Digest(input.data(), input.size(), output, &actualSize,
               algorithm, nullptr) == 1 &&
        actualSize == expectedSize;
}

void AppendUint32(warden::Bytes& bytes, uint32 value)
{
    bytes.push_back(uint8(value));
    bytes.push_back(uint8(value >> 8));
    bytes.push_back(uint8(value >> 16));
    bytes.push_back(uint8(value >> 24));
}

uint32 ReadUint32(uint8 const* bytes)
{
    return uint32(bytes[0]) | (uint32(bytes[1]) << 8) |
        (uint32(bytes[2]) << 16) | (uint32(bytes[3]) << 24);
}

/**
 * Minimal client-side transport peer. It knows only the proven bootstrap
 * envelope and directional RC4; no production module ABI is reproduced here.
 */
class BootstrapPeer
{
public:
    explicit BootstrapPeer(warden::SessionKey const& sessionKey)
    {
        m_ready = m_crypto.Initialize(sessionKey);
    }

    bool IsReady() const { return m_ready; }

    warden::Bytes DecryptServer(warden::EncodedServerFrame const& frame)
    {
        if (frame.payload.size() < 5 ||
            ReadUint32(frame.payload.data()) != frame.payload.size() - 4)
        {
            return {};
        }

        warden::Bytes plain(frame.payload.begin() + 4, frame.payload.end());
        if (!m_crypto.TransformServerToClient(plain))
            return {};
        return plain;
    }

    warden::Bytes EncryptClient(warden::Bytes plain)
    {
        if (!m_crypto.TransformClientToServer(plain))
            return {};

        warden::Bytes frame;
        AppendUint32(frame, uint32(plain.size()));
        frame.insert(frame.end(), plain.begin(), plain.end());
        return frame;
    }

    warden::Bytes EncryptClientBody(warden::Bytes plain)
    {
        if (!m_crypto.TransformClientToServer(plain))
            return {};
        return plain;
    }

    bool InstallArchitecture(warden::ArchitectureProof const& proof)
    {
        return m_crypto.InstallDirectionalKeys(
            proof.clientToServer, proof.serverToClient);
    }

private:
    warden::WardenCryptoContext m_crypto;
    bool m_ready = false;
};

struct Harness
{
    explicit Harness(bool allowSend = true)
        : modules(std::make_shared<warden::WardenModuleCatalog const>(
              warden::test::BuildSyntheticModuleCatalog())),
          checks(std::make_shared<warden::WardenCheckCatalog const>(
              warden::test::BuildSyntheticCheckCatalog())),
          manager(modules, checks), peer(warden::test::SyntheticSessionKey()),
          sendSucceeds(allowSend)
    {
        warden::WardenCreationOptions options;
        options.build = 15595;
        options.clientOs = "Win";
        options.locale = "enUS";
        options.sessionKey = warden::test::SyntheticSessionKey();
        options.configuration.enforcementMode =
            warden::WardenEnforcementMode::Observe;
        options.configuration.normalMinSeconds = 1;
        options.configuration.normalMaxSeconds = 1;
        options.configuration.aggressiveMinSeconds = 1;
        options.configuration.aggressiveMaxSeconds = 1;

        server = manager.Create(std::move(options),
            [this](warden::EncodedServerFrame const& frame)
            {
                ++sendCalls;
                if (!sendSucceeds)
                    return false;
                sent.push_back(frame);
                return true;
            },
            [this](warden::WardenLifecycleEvent const& event)
            {
                events.push_back(event);
                if (reenterState && event.state == *reenterState && server)
                    server->HandleClientFrame(warden::ByteView());
                if (reenterTerminal && server &&
                    event.state == warden::WardenState::Failed)
                {
                    server->Update(false, 1);
                    server->HandleClientFrame(warden::ByteView());
                }
            },
            [this](warden::WardenEvidenceBatch const& batch)
            {
                evidence.push_back(batch);
            });
    }

    warden::Bytes ReadServer()
    {
        if (nextSent >= sent.size())
            return {};
        return peer.DecryptServer(sent[nextSent++]);
    }

    void SendClient(warden::Bytes plain)
    {
        warden::Bytes frame = peer.EncryptClient(std::move(plain));
        server->HandleClientFrame(warden::ByteView(frame));
    }

    bool StartArchitecture(warden::Key16& challenge)
    {
        if (!server || !peer.IsReady() || !server->Start())
            return false;
        warden::Bytes const request = ReadServer();
        if (request.size() != 17 || request[0] != 5)
            return false;
        std::copy(request.begin() + 1, request.end(), challenge.begin());
        return true;
    }

    bool Classify(warden::WardenArchitecture architecture)
    {
        warden::Key16 challenge{};
        if (!StartArchitecture(challenge))
            return false;
        std::optional<warden::ArchitectureProof> const proof =
            warden::DeriveArchitectureProof(architecture, challenge);
        if (!proof)
            return false;

        warden::Bytes response(1 + proof->digest.size());
        response[0] = 4;
        std::copy(proof->digest.begin(), proof->digest.end(),
            response.begin() + 1);
        warden::Bytes frame = peer.EncryptClient(std::move(response));
        if (!peer.InstallArchitecture(*proof))
            return false;
        server->HandleClientFrame(warden::ByteView(frame));
        return server->GetState() == warden::WardenState::ModuleUseSent;
    }

    bool CacheHit(warden::WardenArchitecture architecture)
    {
        if (!Classify(architecture))
            return false;
        warden::Bytes const use = ReadServer();
        if (use.size() != 53 || use[0] != 0)
            return false;
        SendClient({1});
        return server->GetState() == warden::WardenState::ModuleCached;
    }

    bool ReachReadyForWorld(warden::WardenArchitecture architecture)
    {
        if (!CacheHit(architecture))
            return false;
        warden::WardenServerTestAccess::AcceptSyntheticModuleHash(
            *server, true);
        if (server->GetState() != warden::WardenState::ModuleHashVerified)
            return false;
        warden::WardenServerTestAccess::AcceptSyntheticModuleInitialization(
            *server, true);
        return server->GetState() == warden::WardenState::ReadyForWorld;
    }

    bool ReachHealthy(warden::WardenArchitecture architecture)
    {
        if (!ReachReadyForWorld(architecture))
            return false;
        server->Update(true, 0);
        if (server->GetState() != warden::WardenState::ProfileProbeSent)
            return false;
        std::vector<warden::Bytes> probe =
            architecture == warden::WardenArchitecture::X86 ?
                warden::test::X86StockFingerprint() :
                warden::test::X64StockFingerprint();
        warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
            *server, std::move(probe));
        if (server->GetState() != warden::WardenState::InitialChecksSent)
            return false;

        std::optional<warden::CheckPlan> const plan =
            warden::WardenServerTestAccess::PendingCheckPlan(*server);
        if (!plan)
            return false;
        warden::WardenEvidenceBatch batch = CleanBatch(*plan);
        warden::WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
            *server, std::move(batch));
        return server->GetState() == warden::WardenState::Healthy;
    }

    static warden::WardenEvidenceBatch CleanBatch(
        warden::CheckPlan const& plan)
    {
        warden::WardenEvidenceBatch batch;
        batch.requestId = plan.requestId;
        batch.purpose = plan.purpose;
        for (warden::WardenCheckDefinition const& check : plan.checks)
        {
            warden::WardenEvidence item;
            item.requestId = plan.requestId;
            item.checkId = warden::GetWardenCheckId(check);
            item.checkType = warden::GetWardenCheckType(check);
            item.evidenceClass = check.evidenceClass;
            item.outcome = item.checkType == warden::WardenCheckType::Timing ?
                warden::WardenCheckOutcome::Stable :
                warden::WardenCheckOutcome::Match;
            batch.evidence.push_back(item);
        }
        return batch;
    }

    std::shared_ptr<warden::WardenModuleCatalog const> modules;
    std::shared_ptr<warden::WardenCheckCatalog const> checks;
    warden::WardenManager manager;
    BootstrapPeer peer;
    bool sendSucceeds = true;
    bool reenterTerminal = false;
    std::optional<warden::WardenState> reenterState;
    std::size_t sendCalls = 0;
    std::size_t nextSent = 0;
    std::vector<warden::EncodedServerFrame> sent;
    std::vector<warden::WardenLifecycleEvent> events;
    std::vector<warden::WardenEvidenceBatch> evidence;
    std::unique_ptr<warden::WardenServer> server;
};

std::vector<warden::WardenState> States(Harness const& harness)
{
    std::vector<warden::WardenState> states;
    for (warden::WardenLifecycleEvent const& event : harness.events)
        states.push_back(event.state);
    return states;
}

std::size_t CountCommand(Harness& harness, uint8 command,
    std::vector<warden::Bytes>* bodies = nullptr)
{
    std::size_t count = 0;
    while (harness.nextSent < harness.sent.size())
    {
        warden::Bytes plain = harness.ReadServer();
        if (!plain.empty() && plain[0] == command)
        {
            ++count;
            if (bodies)
                bodies->push_back(std::move(plain));
        }
    }
    return count;
}
}

TEST(WardenServer_x86_and_x64_proofs_are_unique_and_select_only_exact_module)
{
    for (warden::WardenArchitecture architecture :
        {warden::WardenArchitecture::X86, warden::WardenArchitecture::X64})
    {
        Harness harness;
        REQUIRE(harness.Classify(architecture));
        warden::Bytes const use = harness.ReadServer();
        warden::ModuleProfile const expected =
            warden::test::SyntheticModuleProfile(architecture);
        REQUIRE(use.size() == 53u);
        CHECK_EQ(use[0], uint8(0));
        CHECK(std::equal(expected.moduleId.begin(), expected.moduleId.end(),
            use.begin() + 1));
        CHECK(std::equal(expected.moduleKey.begin(), expected.moduleKey.end(),
            use.begin() + 33));
        CHECK_EQ(ReadUint32(use.data() + 49), expected.declaredSize);
        CHECK(harness.evidence.empty());
    }
}

TEST(WardenServer_strict_state_order_reaches_healthy_only_after_clean_initial_batch)
{
    Harness harness;
    CHECK(harness.server->GetState() == warden::WardenState::Dormant);
    CHECK(harness.events.empty());
    REQUIRE(harness.ReachHealthy(warden::WardenArchitecture::X86));
    CHECK(States(harness) == std::vector<warden::WardenState>({
        warden::WardenState::ArchitectureChallengeSent,
        warden::WardenState::ArchitectureClassified,
        warden::WardenState::ModuleUseSent,
        warden::WardenState::ModuleCached,
        warden::WardenState::ModuleHashVerified,
        warden::WardenState::ModuleInitialized,
        warden::WardenState::ReadyForWorld,
        warden::WardenState::ProfileProbeSent,
        warden::WardenState::ProfileClassified,
        warden::WardenState::InitialChecksSent,
        warden::WardenState::Healthy}));

    harness.server->Update(true, 1000);
    CHECK(harness.server->GetState() == warden::WardenState::Recurring);
    CHECK(!harness.evidence.empty());
    CHECK(harness.evidence.front().purpose ==
        warden::CheckPlanPurpose::Initial);
}

TEST(WardenServer_in_world_gate_and_complete_batch_gate_prevent_early_healthy)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X64));
    harness.server->Update(false, 60000);
    CHECK(harness.server->GetState() == warden::WardenState::ReadyForWorld);
    std::vector<warden::WardenState> const states = States(harness);
    CHECK(std::find(states.begin(), states.end(),
        warden::WardenState::Healthy) == states.end());

    harness.server->Update(true, 0);
    std::vector<warden::Bytes> probe = warden::test::X64StockFingerprint();
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *harness.server, std::move(probe));
    REQUIRE(harness.server->GetState() ==
        warden::WardenState::InitialChecksSent);
    std::optional<warden::CheckPlan> const plan =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(plan && plan->checks.size() > 1);
    warden::WardenEvidenceBatch partial = Harness::CleanBatch(*plan);
    partial.evidence.pop_back();
    warden::WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
        *harness.server, std::move(partial));
    CHECK(harness.server->GetState() != warden::WardenState::Healthy);

    Harness mismatch;
    REQUIRE(mismatch.ReachReadyForWorld(warden::WardenArchitecture::X86));
    mismatch.server->Update(true, 0);
    probe = warden::test::X86StockFingerprint();
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *mismatch.server, std::move(probe));
    std::optional<warden::CheckPlan> const mismatchPlan =
        warden::WardenServerTestAccess::PendingCheckPlan(
        *mismatch.server);
    REQUIRE(mismatchPlan && !mismatchPlan->checks.empty());
    warden::WardenEvidenceBatch notClean =
        Harness::CleanBatch(*mismatchPlan);
    notClean.evidence.back().outcome =
        warden::WardenCheckOutcome::Mismatch;
    warden::WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
        *mismatch.server, std::move(notClean));
    CHECK(mismatch.server->GetState() != warden::WardenState::Healthy);
}

TEST(WardenServer_cache_miss_fragments_500_byte_transfer_and_allows_one_retry)
{
    Harness harness;
    REQUIRE(harness.Classify(warden::WardenArchitecture::X86));
    REQUIRE(!harness.ReadServer().empty()); // ModuleUse.
    harness.SendClient({0});
    CHECK(harness.server->GetState() == warden::WardenState::ModuleTransfer);

    std::vector<warden::Bytes> first;
    CHECK_EQ(CountCommand(harness, 1, &first), std::size_t(3));
    REQUIRE(first.size() == 3u);
    CHECK_EQ(std::size_t(first[0][1] | (uint16(first[0][2]) << 8)), 500u);
    CHECK_EQ(std::size_t(first[1][1] | (uint16(first[1][2]) << 8)), 500u);
    CHECK_EQ(std::size_t(first[2][1] | (uint16(first[2][2]) << 8)), 201u);

    warden::Bytes reconstructed;
    for (warden::Bytes const& chunk : first)
        reconstructed.insert(reconstructed.end(), chunk.begin() + 3,
            chunk.end());
    CHECK(reconstructed == warden::test::SyntheticModuleProfile(
        warden::WardenArchitecture::X86).container);

    harness.SendClient({0});
    CHECK_EQ(CountCommand(harness, 1), std::size_t(3));
    harness.SendClient({0});
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() != warden::WardenFailure::None);
    CHECK_EQ(CountCommand(harness, 1), std::size_t(0));
    CHECK(harness.evidence.empty());
}

TEST(WardenServer_cache_hit_and_transfer_completion_converge_on_module_cached)
{
    Harness hit;
    REQUIRE(hit.CacheHit(warden::WardenArchitecture::X86));
    CHECK(hit.server->GetState() == warden::WardenState::ModuleCached);

    Harness miss;
    REQUIRE(miss.Classify(warden::WardenArchitecture::X64));
    REQUIRE(!miss.ReadServer().empty());
    miss.SendClient({0});
    REQUIRE(CountCommand(miss, 1) == 3u);
    miss.SendClient({1});
    CHECK(miss.server->GetState() == warden::WardenState::ModuleCached);
}

TEST(WardenServer_direct_bootstrap_commands_are_0_1_2_5_and_never_3_or_4)
{
    Harness harness;
    REQUIRE(harness.CacheHit(warden::WardenArchitecture::X86));
    std::string const text = "synthetic-bootstrap-health";
    REQUIRE(warden::WardenServerTestAccess::SendBootstrapStringHash(
        *harness.server, text));
    warden::Bytes const request = harness.ReadServer();
    REQUIRE(request.size() == text.size() + 1);
    CHECK_EQ(request[0], uint8(2));
    CHECK(std::equal(text.begin(), text.end(), request.begin() + 1));

    warden::Digest20 sha1{};
    warden::Digest32 sha256{};
    REQUIRE(Digest(EVP_sha1(), warden::ByteView(
        reinterpret_cast<uint8 const*>(text.data()), text.size()),
        sha1.data(), sha1.size()));
    REQUIRE(Digest(EVP_sha256(), warden::ByteView(
        reinterpret_cast<uint8 const*>(text.data()), text.size()),
        sha256.data(), sha256.size()));
    warden::Bytes response = {2};
    response.insert(response.end(), sha1.begin(), sha1.end());
    response.insert(response.end(), sha256.begin(), sha256.end());
    harness.SendClient(std::move(response));
    CHECK(harness.server->GetState() == warden::WardenState::ModuleCached);
    CHECK(harness.evidence.empty());

    Harness command3;
    REQUIRE(command3.Classify(warden::WardenArchitecture::X86));
    REQUIRE(!command3.ReadServer().empty());
    command3.SendClient({3});
    CHECK(command3.server->GetState() == warden::WardenState::Failed);

    Harness late4;
    REQUIRE(late4.Classify(warden::WardenArchitecture::X64));
    REQUIRE(!late4.ReadServer().empty());
    late4.SendClient(warden::Bytes(21, 4));
    CHECK(late4.server->GetState() == warden::WardenState::Failed);
}

TEST(WardenServer_neither_ambiguous_and_replayed_architecture_proofs_are_terminal)
{
    Harness neither;
    warden::Key16 challenge{};
    REQUIRE(neither.StartArchitecture(challenge));
    warden::Bytes noMatch(21, 0);
    noMatch[0] = 4;
    neither.SendClient(std::move(noMatch));
    CHECK(neither.server->GetState() == warden::WardenState::Failed);
    CHECK(neither.evidence.empty());

    Harness ambiguous;
    REQUIRE(ambiguous.StartArchitecture(challenge));
    std::optional<warden::ArchitectureProof> const proof =
        warden::DeriveArchitectureProof(
            warden::WardenArchitecture::X86, challenge);
    REQUIRE(proof);
    warden::WardenServerTestAccess::ForceNextArchitectureMatches(
        *ambiguous.server, true, true);
    warden::Bytes response = {4};
    response.insert(response.end(), proof->digest.begin(), proof->digest.end());
    ambiguous.SendClient(std::move(response));
    CHECK(ambiguous.server->GetState() == warden::WardenState::Failed);
    CHECK(ambiguous.evidence.empty());

    Harness replay;
    REQUIRE(replay.Classify(warden::WardenArchitecture::X86));
    REQUIRE(!replay.ReadServer().empty());
    replay.SendClient(warden::Bytes(21, 4));
    CHECK(replay.server->GetState() == warden::WardenState::Failed);
    CHECK(replay.evidence.empty());
}

TEST(WardenServer_wrong_state_module_failure_bad_hash_and_timeout_are_operational)
{
    Harness wrongState;
    REQUIRE(wrongState.server->Start());
    wrongState.SendClient({1});
    CHECK(wrongState.server->GetState() == warden::WardenState::Failed);
    CHECK(wrongState.evidence.empty());

    Harness moduleFailure;
    REQUIRE(moduleFailure.Classify(warden::WardenArchitecture::X86));
    REQUIRE(!moduleFailure.ReadServer().empty());
    moduleFailure.SendClient({5});
    CHECK(moduleFailure.server->GetState() == warden::WardenState::Failed);
    CHECK(moduleFailure.evidence.empty());

    Harness badHash;
    REQUIRE(badHash.CacheHit(warden::WardenArchitecture::X64));
    warden::WardenServerTestAccess::AcceptSyntheticModuleHash(
        *badHash.server, false);
    CHECK(badHash.server->GetState() == warden::WardenState::Failed);
    CHECK(badHash.evidence.empty());

    Harness timeout;
    REQUIRE(timeout.server->Start());
    timeout.server->Update(false, std::numeric_limits<uint32>::max());
    CHECK(timeout.server->GetState() == warden::WardenState::Failed);
    CHECK(timeout.evidence.empty());
}

TEST(WardenServer_rejected_decrypted_frame_rolls_back_rc4_before_terminal_state)
{
    Harness harness;
    REQUIRE(harness.Classify(warden::WardenArchitecture::X86));
    REQUIRE(!harness.ReadServer().empty());

    warden::Bytes const plain = {3, 0xAA, 0x55};
    warden::Bytes const cipher = harness.peer.EncryptClientBody(plain);
    warden::Bytes frame;
    AppendUint32(frame, uint32(cipher.size()));
    frame.insert(frame.end(), cipher.begin(), cipher.end());
    harness.server->HandleClientFrame(warden::ByteView(frame));
    REQUIRE(harness.server->GetState() == warden::WardenState::Failed);

    warden::Bytes preview;
    REQUIRE(warden::WardenServerTestAccess::PreviewCommittedClientPlaintext(
        *harness.server, warden::ByteView(cipher), preview));
    CHECK(preview == plain);
    CHECK(harness.evidence.empty());
}

TEST(WardenServer_disconnect_and_reentrant_terminal_observer_notify_once)
{
    Harness harness(false);
    harness.reenterTerminal = true;
    CHECK(!harness.server->Start());
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK_EQ(harness.sendCalls, std::size_t(1));
    CHECK_EQ(harness.events.size(), std::size_t(1));
    CHECK(harness.events[0].state == warden::WardenState::Failed);

    harness.server->Update(false, std::numeric_limits<uint32>::max());
    harness.server->HandleClientFrame(warden::ByteView());
    CHECK_EQ(harness.sendCalls, std::size_t(1));
    CHECK_EQ(harness.events.size(), std::size_t(1));
    CHECK(harness.evidence.empty());
}

TEST(WardenServer_duplicate_start_and_late_commands_are_absorbed_or_terminal_once)
{
    Harness duplicate;
    CHECK(duplicate.server->Start());
    CHECK(duplicate.server->Start());
    CHECK_EQ(duplicate.sendCalls, std::size_t(1));

    Harness late;
    REQUIRE(late.ReachReadyForWorld(warden::WardenArchitecture::X86));
    late.SendClient({1});
    CHECK(late.server->GetState() == warden::WardenState::Failed);
    std::size_t const eventCount = late.events.size();
    late.SendClient({1});
    late.server->Update(true, std::numeric_limits<uint32>::max());
    CHECK_EQ(late.events.size(), eventCount);
    CHECK(late.evidence.empty());
}

TEST(WardenServer_confirmation_and_aggressive_controls_forward_exact_plan_purpose)
{
    Harness confirmation;
    REQUIRE(confirmation.ReachHealthy(warden::WardenArchitecture::X86));
    REQUIRE(confirmation.server->QueueConfirmation(2002));
    confirmation.server->Update(true, 0);
    std::optional<warden::CheckPlan> plan =
        warden::WardenServerTestAccess::PendingCheckPlan(
            *confirmation.server);
    REQUIRE(plan);
    CHECK(plan->purpose == warden::CheckPlanPurpose::Confirmation);
    REQUIRE(plan->checks.size() == 1u);
    CHECK_EQ(warden::GetWardenCheckId(plan->checks[0]), uint32(2002));

    Harness aggressive;
    REQUIRE(aggressive.ReachHealthy(warden::WardenArchitecture::X64));
    aggressive.server->SetAggressive(true);
    aggressive.server->Update(true, 0);
    plan = warden::WardenServerTestAccess::PendingCheckPlan(
        *aggressive.server);
    REQUIRE(plan);
    CHECK(plan->purpose == warden::CheckPlanPurpose::AggressiveImmediate);
    CHECK(!plan->checks.empty());
}

TEST(WardenServer_second_recurring_plan_receives_a_fresh_deadline)
{
    Harness harness;
    REQUIRE(harness.ReachHealthy(warden::WardenArchitecture::X86));

    harness.server->Update(true, 1000);
    std::optional<warden::CheckPlan> first =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(first);
    REQUIRE(first->purpose == warden::CheckPlanPurpose::Recurring);
    warden::WardenEvidenceBatch batch = Harness::CleanBatch(*first);
    warden::WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
        *harness.server, std::move(batch));
    REQUIRE(harness.server->GetState() == warden::WardenState::Recurring);

    harness.server->Update(true, 1000);
    std::optional<warden::CheckPlan> second =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(second);
    REQUIRE(second->purpose == warden::CheckPlanPurpose::Recurring);

    // A newly armed plan must survive a zero-time update. A stale zero
    // deadline makes this call fail immediately even though no time elapsed.
    harness.server->Update(true, 0);
    CHECK(harness.server->GetState() == warden::WardenState::Recurring);
    CHECK(warden::WardenServerTestAccess::PendingCheckPlan(
        *harness.server).has_value());
}

TEST(WardenServer_nonclean_initial_batch_enters_recurring_for_confirmation)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
    harness.server->Update(true, 0);
    std::vector<warden::Bytes> probe =
        warden::test::X86StockFingerprint();
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *harness.server, std::move(probe));

    std::optional<warden::CheckPlan> initial =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(initial);
    REQUIRE(initial->purpose == warden::CheckPlanPurpose::Initial);
    warden::WardenEvidenceBatch batch = Harness::CleanBatch(*initial);
    REQUIRE(batch.evidence.size() == 2u);
    batch.evidence[1].outcome = warden::WardenCheckOutcome::Mismatch;
    warden::WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
        *harness.server, std::move(batch));

    CHECK(harness.server->GetState() == warden::WardenState::Recurring);
    REQUIRE(harness.server->QueueConfirmation(2002));
    harness.server->Update(true, 0);
    std::optional<warden::CheckPlan> confirmation =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(confirmation);
    CHECK(confirmation->purpose == warden::CheckPlanPurpose::Confirmation);
}

TEST(WardenServer_legacy_grunt_failure_preserves_bounded_variant_token)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X64));
    harness.server->Update(true, 0);
    std::vector<warden::Bytes> probe =
        warden::test::X64LegacyGruntFingerprint();
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *harness.server, std::move(probe));

    REQUIRE(harness.server->GetState() == warden::WardenState::Failed);
    REQUIRE(!harness.events.empty());
    CHECK(harness.events.back().variant == warden::ClientVariant::LegacyGrunt);
}

TEST(WardenServer_profile_probe_rejects_generic_evidence_completion)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X64));
    harness.server->Update(true, 0);
    REQUIRE(harness.server->GetState() ==
        warden::WardenState::ProfileProbeSent);

    std::optional<warden::CheckPlan> const plan =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(plan.has_value());
    warden::WardenEvidenceBatch batch = Harness::CleanBatch(*plan);
    warden::WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
        *harness.server, std::move(batch));

    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::UnexpectedCommand);
}

TEST(WardenServer_nonterminal_observer_failure_stops_follow_on_mutation)
{
    Harness transfer;
    REQUIRE(transfer.Classify(warden::WardenArchitecture::X86));
    REQUIRE(!transfer.ReadServer().empty());
    transfer.reenterState = warden::WardenState::ModuleTransfer;
    transfer.SendClient({0});
    CHECK(transfer.server->GetState() == warden::WardenState::Failed);
    CHECK_EQ(CountCommand(transfer, 1), std::size_t(0));

    Harness profile;
    REQUIRE(profile.ReachReadyForWorld(warden::WardenArchitecture::X64));
    profile.server->Update(true, 0);
    REQUIRE(profile.server->GetState() ==
        warden::WardenState::ProfileProbeSent);
    profile.reenterState = warden::WardenState::ProfileClassified;
    std::vector<warden::Bytes> probe =
        warden::test::X64StockFingerprint();
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *profile.server, std::move(probe));
    CHECK(profile.server->GetState() == warden::WardenState::Failed);
    CHECK(!warden::WardenServerTestAccess::PendingCheckPlan(
        *profile.server).has_value());
    CHECK(profile.evidence.empty());

    Harness healthy;
    REQUIRE(healthy.ReachReadyForWorld(warden::WardenArchitecture::X86));
    healthy.server->Update(true, 0);
    probe = warden::test::X86StockFingerprint();
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *healthy.server, std::move(probe));
    std::optional<warden::CheckPlan> const initial =
        warden::WardenServerTestAccess::PendingCheckPlan(*healthy.server);
    REQUIRE(initial.has_value());
    healthy.reenterState = warden::WardenState::Healthy;
    warden::WardenEvidenceBatch batch = Harness::CleanBatch(*initial);
    warden::WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
        *healthy.server, std::move(batch));
    CHECK(healthy.server->GetState() == warden::WardenState::Failed);
    CHECK(healthy.evidence.empty());
}
