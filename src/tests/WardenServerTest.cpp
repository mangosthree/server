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
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace warden
{
class WardenCheckCatalogTestAccess
{
public:
    static WardenCheckCatalog PublishUnchecked(
        std::vector<WardenCheckProfile> profiles)
    {
        WardenCheckCatalog catalog;
        catalog.m_profiles = std::move(profiles);
        return catalog;
    }
};
}

namespace
{
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

bool BuildClientCheckResult(warden::CheckPlan const& plan,
    std::vector<warden::Bytes> const& probeBytes, warden::Bytes& plaintext,
    bool timingStable = true)
{
    warden::Bytes body;
    std::size_t probeIndex = 0;
    for (warden::WardenCheckDefinition const& definition : plan.checks)
    {
        if (std::holds_alternative<warden::TimingCheckProfile>(
                definition.payload))
        {
            body.push_back(timingStable ? 1 : 0);
            AppendUint32(body, 0x12345678);
            continue;
        }
        if (warden::LuaCheckProfile const* lua =
                std::get_if<warden::LuaCheckProfile>(&definition.payload))
        {
            if (lua->expectedText.size() > 0xFF)
                return false;
            body.push_back(0);
            body.push_back(uint8(lua->expectedText.size()));
            body.insert(body.end(), lua->expectedText.begin(),
                lua->expectedText.end());
            continue;
        }
        if (warden::MpqCheckProfile const* mpq =
                std::get_if<warden::MpqCheckProfile>(&definition.payload))
        {
            body.push_back(0);
            body.insert(body.end(), mpq->expectedSha1.begin(),
                mpq->expectedSha1.end());
            continue;
        }

        warden::MemCheckProfile const* memory =
            std::get_if<warden::MemCheckProfile>(&definition.payload);
        if (!memory)
            return false;
        warden::Bytes const* bytes = &memory->expectedBytes;
        if (plan.purpose == warden::CheckPlanPurpose::ProfileProbe)
        {
            if (probeIndex >= probeBytes.size())
                return false;
            bytes = &probeBytes[probeIndex++];
        }
        if (bytes->size() != memory->length)
            return false;
        body.push_back(0);
        body.insert(body.end(), bytes->begin(), bytes->end());
    }
    if (plan.purpose == warden::CheckPlanPurpose::ProfileProbe &&
        probeIndex != probeBytes.size())
    {
        return false;
    }
    if (body.size() > 0xFFFF)
        return false;

    warden::Digest20 digest{};
    unsigned int digestSize = 0;
    if (EVP_Digest(body.data(), body.size(), digest.data(), &digestSize,
            EVP_sha1(), nullptr) != 1 || digestSize != digest.size())
    {
        return false;
    }
    uint32 checksum = 0;
    for (std::size_t offset = 0; offset < digest.size(); offset += 4)
        checksum ^= ReadUint32(digest.data() + offset);

    plaintext = {2, uint8(body.size()), uint8(body.size() >> 8)};
    AppendUint32(plaintext, checksum);
    plaintext.insert(plaintext.end(), body.begin(), body.end());
    return true;
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

    bool InstallModule(warden::ModuleRekeyVector const& rekey)
    {
        return m_crypto.InstallDirectionalKeys(
            rekey.clientToServer, rekey.serverToClient);
    }

private:
    warden::WardenCryptoContext m_crypto;
    bool m_ready = false;
};

struct Harness
{
    explicit Harness(bool allowSend = true,
        std::string locale = "enUS",
        warden::WardenEnforcementMode enforcementMode =
            warden::WardenEnforcementMode::Observe,
        std::shared_ptr<warden::WardenCheckCatalog const> checkOverride = {},
        uint32 normalMinSeconds = 1, uint32 normalMaxSeconds = 1,
        uint32 aggressiveMinSeconds = 1,
        uint32 aggressiveMaxSeconds = 1)
        : modules(std::make_shared<warden::WardenModuleCatalog const>(
              warden::test::BuildSyntheticModuleCatalog())),
          checks(checkOverride ? std::move(checkOverride) :
              std::make_shared<warden::WardenCheckCatalog const>(
                  warden::test::BuildSyntheticCheckCatalog())),
          manager(modules, checks), peer(warden::test::SyntheticSessionKey()),
          sendSucceeds(allowSend)
    {
        warden::WardenCreationOptions options;
        options.build = 15595;
        options.clientOs = "Win";
        options.locale = std::move(locale);
        options.sessionKey = warden::test::SyntheticSessionKey();
        options.configuration.enforcementMode = enforcementMode;
        options.configuration.normalMinSeconds = normalMinSeconds;
        options.configuration.normalMaxSeconds = normalMaxSeconds;
        options.configuration.aggressiveMinSeconds = aggressiveMinSeconds;
        options.configuration.aggressiveMaxSeconds = aggressiveMaxSeconds;

        server = manager.Create(std::move(options),
            [this](warden::EncodedServerFrame const& frame)
            {
                ++sendCalls;
                if (!sendSucceeds)
                    return false;
                sent.push_back(frame);
                if (reenterSend && server)
                {
                    reenterSend = false;
                    server->HandleClientFrame(warden::ByteView());
                }
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

    bool CompleteModuleHash(warden::WardenArchitecture architecture)
    {
        warden::ModuleProfile const* profile =
            modules->FindExact({15595, architecture});
        if (!profile)
            return false;

        warden::Bytes const request = ReadServer();
        if (request.size() != 17 || request[0] != 5 ||
            !std::equal(profile->rekey.seed.begin(), profile->rekey.seed.end(),
                request.begin() + 1))
        {
            return false;
        }

        warden::Bytes response = {4};
        response.insert(response.end(), profile->rekey.expectedResponse.begin(),
            profile->rekey.expectedResponse.end());
        warden::Bytes frame = peer.EncryptClient(std::move(response));
        if (!peer.InstallModule(profile->rekey))
            return false;
        server->HandleClientFrame(warden::ByteView(frame));
        return true;
    }

    bool ReachReadyForWorld(warden::WardenArchitecture architecture)
    {
        if (!CacheHit(architecture) || !CompleteModuleHash(architecture))
            return false;
        if (server->GetState() != warden::WardenState::ReadyForWorld)
        {
            return false;
        }
        warden::Bytes const initialization = ReadServer();
        warden::Bytes const expected =
            architecture == warden::WardenArchitecture::X86 ?
                warden::Bytes({
                    0x03, 0x0C, 0x00, 0xE9, 0xAB, 0x2B, 0xD1,
                    0x04, 0x00, 0x00, 0x10, 0xD3, 0x43, 0x00,
                    0x30, 0xC2, 0x43, 0x00, 0x01,
                    0x03, 0x08, 0x00, 0x4C, 0xA0, 0x9E, 0x6C,
                    0x01, 0x01, 0x00, 0x40, 0x97, 0x47, 0x00, 0x01}) :
                warden::Bytes({
                    0x03, 0x0C, 0x00, 0xF7, 0x17, 0xB3, 0x83,
                    0x04, 0x00, 0x00, 0x10, 0x81, 0x56, 0x00,
                    0xC0, 0x6B, 0x56, 0x00, 0x01,
                    0x03, 0x14, 0x00, 0x50, 0x56, 0xE5, 0xDB,
                    0x01, 0x00, 0x02, 0x00, 0x10, 0xEA, 0x49,
                    0x00, 0xF0, 0x9D, 0x49, 0x00, 0x50, 0xB5,
                    0x49, 0x00, 0x10, 0xB6, 0x49, 0x00,
                    0x03, 0x08, 0x00, 0x99, 0x06, 0x76, 0x7A,
                    0x01, 0x01, 0x00, 0x00, 0x25, 0x5B, 0x00, 0x01});
        return initialization == expected;
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
    bool reenterSend = false;
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

warden::WardenCheckCatalog BuildCatalog(
    std::vector<warden::WardenCheckRowInput> const& rows)
{
    warden::WardenCheckCatalogBuilder builder;
    warden::WardenCheckDiagnostic diagnostic;
    for (warden::WardenCheckRowInput const& row : rows)
    {
        if (builder.Add(row, diagnostic) !=
            warden::CheckCatalogValidation::Valid)
        {
            return {};
        }
    }

    warden::WardenCheckCatalog catalog;
    if (builder.Build(catalog, diagnostic) !=
        warden::CheckCatalogValidation::Valid)
    {
        return {};
    }
    return catalog;
}

warden::WardenCheckCatalog BuildSyntheticMpqCheckCatalog()
{
    std::vector<warden::WardenCheckRowInput> rows =
        warden::test::CompleteSyntheticRows();
    for (warden::ClientVariant const variant :
        {warden::ClientVariant::Stock, warden::ClientVariant::Grunt})
    {
        warden::WardenCheckRowInput mpq = warden::test::MakeRow(
            warden::WardenArchitecture::X64, variant, 2004,
            warden::WardenCheckType::Mpq, 25,
            warden::WardenEvidenceClass::Corroboration,
            warden::PhaseInitial | warden::PhaseRecurring,
            warden::WardenAddressKind::None);
        mpq.requestHex =
            "444246696C6573436C69656E745C4974656D2E646232";
        mpq.expectedHex =
            "4706FF83D9B611644A87DE79C244B414612EF4F2";
        rows.push_back(std::move(mpq));
    }

    return BuildCatalog(rows);
}

warden::WardenCheckCatalog BuildProductionShapedX86Catalog()
{
    std::vector<warden::WardenCheckRowInput> rows =
        warden::test::ProfileProbeRows(warden::WardenArchitecture::X86);
    std::vector<warden::WardenCheckRowInput> x64 =
        warden::test::ProfileProbeRows(warden::WardenArchitecture::X64);
    for (warden::ClientVariant const variant :
        {warden::ClientVariant::Stock, warden::ClientVariant::Grunt})
    {
        std::vector<warden::WardenCheckRowInput> classified =
            warden::test::ClassifiedRows(
                warden::WardenArchitecture::X64, variant);
        x64.insert(x64.end(), classified.begin(), classified.end());
    }
    rows.insert(rows.end(), x64.begin(), x64.end());

    for (warden::ClientVariant const variant :
        {warden::ClientVariant::Stock, warden::ClientVariant::LegacyGrunt,
            warden::ClientVariant::Grunt})
    {
        warden::WardenCheckRowInput timing = warden::test::MakeRow(
            warden::WardenArchitecture::X86, variant, 2001,
            warden::WardenCheckType::Timing, 10,
            warden::WardenEvidenceClass::ProtocolHealth,
            warden::PhaseInitial | warden::PhaseRecurring,
            warden::WardenAddressKind::None);
        rows.push_back(std::move(timing));

        warden::WardenCheckRowInput lua = warden::test::MakeRow(
            warden::WardenArchitecture::X86, variant, 2003,
            warden::WardenCheckType::Lua, 30,
            warden::WardenEvidenceClass::Corroboration,
            warden::PhaseInitial | warden::PhaseRecurring,
            warden::WardenAddressKind::None);
        lua.requestHex = "4F4B4159";   // OKAY
        lua.expectedHex = "4F6B6179";  // Okay
        rows.push_back(std::move(lua));

        if (variant == warden::ClientVariant::Grunt)
        {
            warden::WardenCheckRowInput mpq = warden::test::MakeRow(
                warden::WardenArchitecture::X86, variant, 2002,
                warden::WardenCheckType::Mpq, 35,
                warden::WardenEvidenceClass::Corroboration,
                warden::PhaseInitial | warden::PhaseRecurring,
                warden::WardenAddressKind::None);
            mpq.requestHex =
                "444246696C6573436C69656E745C4974656D2E646232";
            mpq.expectedHex =
                "4706FF83D9B611644A87DE79C244B414612EF4F2";
            rows.push_back(std::move(mpq));
        }

        warden::WardenCheckRowInput memory = warden::test::MakeRow(
            warden::WardenArchitecture::X86, variant, 2004,
            warden::WardenCheckType::Mem, 40,
            warden::WardenEvidenceClass::IntegrityInvariant,
            warden::PhaseInitial | warden::PhaseRecurring |
                warden::PhaseAggressive,
            warden::WardenAddressKind::ModuleRelativeRva);
        memory.moduleHex = "576F772E657865"; // Wow.exe
        memory.address = 0x0043C257;
        memory.length = 12;
        memory.expectedHex = "538B5D08568BC3578D50018A";
        rows.push_back(std::move(memory));
    }

    return BuildCatalog(rows);
}
}

TEST(WardenManager_requires_observe_until_every_module_is_production_approved)
{
    Harness observe;
    CHECK(observe.server != nullptr);

    Harness kick(true, "enUS", warden::WardenEnforcementMode::Kick);
    CHECK(kick.server == nullptr);

    Harness kickAndBan(
        true, "enUS", warden::WardenEnforcementMode::KickAndBan);
    CHECK(kickAndBan.server == nullptr);
}

TEST(WardenManager_defers_exact_locale_profiles_until_architecture_selection)
{
    Harness harness(true, "frFR");
    REQUIRE(harness.server != nullptr);

    CHECK(!harness.Classify(warden::WardenArchitecture::X86));
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::UnsupportedProfile);
}

TEST(WardenManager_rejects_non_alpha_or_non_exact_locale_identity)
{
    Harness shortLocale(true, "enU");
    CHECK(shortLocale.server == nullptr);

    Harness nonAlphaLocale(true, "en1S");
    CHECK(nonAlphaLocale.server == nullptr);
}

TEST(WardenManager_rejects_incomplete_profiles_for_a_full_module)
{
    warden::WardenCheckCatalog const complete =
        warden::test::BuildSyntheticCheckCatalog();
    std::vector<warden::WardenCheckProfile> profiles = complete.Profiles();
    profiles.erase(std::remove_if(profiles.begin(), profiles.end(),
        [](warden::WardenCheckProfile const& profile)
        {
            return profile.key.architecture ==
                    warden::WardenArchitecture::X64 &&
                profile.key.variant == warden::ClientVariant::Grunt;
        }), profiles.end());
    auto invalid = std::make_shared<warden::WardenCheckCatalog const>(
        warden::WardenCheckCatalogTestAccess::PublishUnchecked(
            std::move(profiles)));

    Harness harness(true, "enUS", warden::WardenEnforcementMode::Observe,
        std::move(invalid));
    CHECK(harness.server == nullptr);
}

TEST(WardenServer_x86_real_module_check_flow_reaches_healthy)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));

    harness.server->Update(true, 0);
    std::optional<warden::CheckPlan> probe =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(probe.has_value());
    warden::Bytes const probeRequest = harness.ReadServer();
    REQUIRE(!probeRequest.empty());
    CHECK_EQ(probeRequest[0], uint8(2));

    warden::Bytes probeResult;
    REQUIRE(BuildClientCheckResult(*probe,
        warden::test::X86StockFingerprint(), probeResult));
    harness.SendClient(std::move(probeResult));
    CHECK(harness.server->GetState() ==
        warden::WardenState::InitialChecksSent);

    std::optional<warden::CheckPlan> initial =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(initial.has_value());
    warden::Bytes const initialRequest = harness.ReadServer();
    REQUIRE(!initialRequest.empty());
    CHECK_EQ(initialRequest[0], uint8(2));

    warden::Bytes initialResult;
    REQUIRE(BuildClientCheckResult(*initial, {}, initialResult));
    harness.SendClient(std::move(initialResult));
    CHECK(harness.server->GetState() == warden::WardenState::Healthy);
    REQUIRE(harness.evidence.size() == 1u);
    CHECK(warden::IsCompleteCleanOperatorBatch(harness.evidence.front()));
}

TEST(WardenServer_x86_current_grunt_initializes_filesystem_before_mpq_plan)
{
    auto const checks = std::make_shared<warden::WardenCheckCatalog const>(
        BuildProductionShapedX86Catalog());
    Harness harness(true, "enUS", warden::WardenEnforcementMode::Observe,
        checks);
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
    harness.server->Update(true, 0);
    REQUIRE(!harness.ReadServer().empty()); // Profile-probe request.

    std::vector<warden::Bytes> probe =
        warden::test::X86GruntFingerprint();
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *harness.server, std::move(probe));
    CHECK(harness.server->GetState() ==
        warden::WardenState::InitialChecksSent);

    warden::Bytes const filesystem = harness.ReadServer();
    CHECK_HEX(filesystem.data(), filesystem.size(),
        "0314008f1f12ad01000100508c3a0070513a0088ff3b0000663a00");

    std::optional<warden::CheckPlan> const initial =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(initial.has_value());
    CHECK(initial->profileKey.variant == warden::ClientVariant::Grunt);
    std::vector<warden::WardenCheckType> types;
    std::transform(initial->checks.begin(), initial->checks.end(),
        std::back_inserter(types),
        [](warden::WardenCheckDefinition const& check)
        {
            return warden::GetWardenCheckType(check);
        });
    CHECK(types == std::vector<warden::WardenCheckType>({
        warden::WardenCheckType::Timing,
        warden::WardenCheckType::Lua,
        warden::WardenCheckType::Mpq,
        warden::WardenCheckType::Mem}));
    CHECK(std::any_of(initial->checks.begin(), initial->checks.end(),
        [](warden::WardenCheckDefinition const& check)
        {
            return warden::GetWardenCheckType(check) ==
                warden::WardenCheckType::Mpq;
        }));

    warden::Bytes const request = harness.ReadServer();
    REQUIRE(!request.empty());
    CHECK_EQ(request.front(), uint8(2));
    std::string const pathText = "DBFilesClient\\Item.db2";
    warden::Bytes const path(pathText.begin(), pathText.end());
    CHECK(std::search(request.begin(), request.end(), path.begin(),
        path.end()) != request.end());
}

TEST(WardenServer_x86_legacy_grunt_remains_healthy_without_filesystem_or_mpq)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
    harness.server->Update(true, 0);
    REQUIRE(!harness.ReadServer().empty()); // Profile-probe request.

    std::vector<warden::Bytes> probe =
        warden::test::X86LegacyGruntFingerprint();
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *harness.server, std::move(probe));
    CHECK(harness.server->GetState() ==
        warden::WardenState::InitialChecksSent);

    std::optional<warden::CheckPlan> const initial =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(initial.has_value());
    CHECK(initial->profileKey.variant == warden::ClientVariant::LegacyGrunt);
    CHECK(std::none_of(initial->checks.begin(), initial->checks.end(),
        [](warden::WardenCheckDefinition const& check)
        {
            return warden::GetWardenCheckType(check) ==
                warden::WardenCheckType::Mpq;
        }));

    // The only new outbound body is the initial check request. A filesystem
    // command would appear here first and fail this assertion.
    warden::Bytes const request = harness.ReadServer();
    REQUIRE(!request.empty());
    CHECK_EQ(request.front(), uint8(2));

    warden::WardenEvidenceBatch batch = Harness::CleanBatch(*initial);
    warden::WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
        *harness.server, std::move(batch));
    CHECK(harness.server->GetState() == warden::WardenState::Healthy);
}

TEST(WardenServer_x86_late_filesystem_send_failure_is_operational)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
    harness.server->Update(true, 0);
    REQUIRE(!harness.ReadServer().empty());
    harness.sendSucceeds = false;

    std::vector<warden::Bytes> probe =
        warden::test::X86GruntFingerprint();
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *harness.server, std::move(probe));
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::SendFailure);
    CHECK(!warden::WardenServerTestAccess::PendingCheckPlan(
        *harness.server).has_value());
    CHECK(harness.evidence.empty());
}

TEST(WardenServer_x86_mutated_adapter_probe_fails_before_filesystem_init)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
    harness.server->Update(true, 0);
    REQUIRE(!harness.ReadServer().empty());
    std::size_t const sentBeforeResult = harness.sent.size();

    std::vector<warden::Bytes> probe =
        warden::test::X86GruntFingerprint();
    probe[3][0] ^= 0x01;
    warden::WardenServerTestAccess::CompleteSyntheticProfileProbe(
        *harness.server, std::move(probe));
    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::ProfileUnclassified);
    CHECK_EQ(harness.sent.size(), sentBeforeResult);
    CHECK(harness.evidence.empty());
}

TEST(WardenServer_x86_malformed_real_check_result_is_operational)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
    harness.server->Update(true, 0);
    std::optional<warden::CheckPlan> probe =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(probe.has_value());
    REQUIRE(!harness.ReadServer().empty());

    warden::Bytes probeResult;
    REQUIRE(BuildClientCheckResult(*probe,
        warden::test::X86StockFingerprint(), probeResult));
    probeResult[3] ^= 0x01;
    harness.SendClient(std::move(probeResult));

    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() ==
        warden::WardenFailure::MalformedPayload);
    CHECK(harness.evidence.empty());
}

TEST(WardenServer_reentrant_send_failure_cannot_publish_a_pending_plan)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
    harness.reenterSend = true;

    harness.server->Update(true, 0);

    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(!warden::WardenServerTestAccess::PendingCheckPlan(
        *harness.server).has_value());
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

TEST(WardenServer_schedule_selects_inside_each_configured_interval)
{
    Harness harness(true, "enUS", warden::WardenEnforcementMode::Observe,
        {}, 7, 11, 2, 4);
    std::vector<std::pair<uint32, uint32>> bounds;
    warden::WardenServerTestAccess::SetScheduleSecondsSelector(
        *harness.server, [&bounds](uint32 minimum, uint32 maximum)
        {
            bounds.emplace_back(minimum, maximum);
            return maximum;
        });

    REQUIRE(harness.ReachHealthy(warden::WardenArchitecture::X86));
    CHECK_EQ(warden::WardenServerTestAccess::RemainingScheduleMs(
        *harness.server), uint32(11000));
    harness.server->Update(true, 10999);
    CHECK(harness.server->GetState() == warden::WardenState::Healthy);
    harness.server->Update(true, 1);
    std::optional<warden::CheckPlan> const recurring =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(recurring);
    warden::WardenEvidenceBatch batch = Harness::CleanBatch(*recurring);
    warden::WardenServerTestAccess::CompleteSyntheticEvidenceBatch(
        *harness.server, std::move(batch));
    CHECK_EQ(warden::WardenServerTestAccess::RemainingScheduleMs(
        *harness.server), uint32(11000));

    harness.server->SetAggressive(true);
    CHECK_EQ(warden::WardenServerTestAccess::RemainingScheduleMs(
        *harness.server), uint32(4000));
    std::vector<std::pair<uint32, uint32>> const expectedBounds = {
        {7, 11}, {7, 11}, {2, 4}};
    CHECK(bounds == expectedBounds);
}

TEST(WardenServer_production_schedule_draw_stays_inside_inclusive_bounds)
{
    Harness harness(true, "enUS", warden::WardenEnforcementMode::Observe,
        {}, 7, 11, 2, 4);

    for (uint32 draw = 0; draw < 32; ++draw)
    {
        harness.server->SetAggressive(false);
        uint32 const milliseconds =
            warden::WardenServerTestAccess::RemainingScheduleMs(
                *harness.server);
        CHECK(milliseconds >= 7000);
        CHECK(milliseconds <= 11000);
    }

    for (uint32 draw = 0; draw < 32; ++draw)
    {
        harness.server->SetAggressive(true);
        uint32 const milliseconds =
            warden::WardenServerTestAccess::RemainingScheduleMs(
                *harness.server);
        CHECK(milliseconds >= 2000);
        CHECK(milliseconds <= 4000);
    }
}

TEST(WardenServer_plan_send_failure_preserves_transport_reason)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
    harness.sendSucceeds = false;
    harness.server->Update(true, 0);

    CHECK(harness.server->GetState() == warden::WardenState::Failed);
    CHECK(harness.server->GetFailure() == warden::WardenFailure::SendFailure);
}

TEST(WardenServer_in_world_gate_and_complete_batch_gate_prevent_early_healthy)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
    harness.server->Update(false, 60000);
    CHECK(harness.server->GetState() == warden::WardenState::ReadyForWorld);
    std::vector<warden::WardenState> const states = States(harness);
    CHECK(std::find(states.begin(), states.end(),
        warden::WardenState::Healthy) == states.end());

    harness.server->Update(true, 0);
    std::vector<warden::Bytes> probe = warden::test::X86StockFingerprint();
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

TEST(WardenServer_cache_hit_sends_the_selected_module_hash_request)
{
    Harness harness;
    REQUIRE(harness.CacheHit(warden::WardenArchitecture::X86));
    warden::ModuleProfile const* profile = harness.modules->FindExact(
        {15595, warden::WardenArchitecture::X86});
    REQUIRE(profile != nullptr);
    warden::Bytes const request = harness.ReadServer();
    REQUIRE(request.size() == 17u);
    CHECK_EQ(request[0], uint8(5));
    CHECK(std::equal(profile->rekey.seed.begin(), profile->rekey.seed.end(),
        request.begin() + 1));
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
    warden::ModuleProfile const* profile = badHash.modules->FindExact(
        {15595, warden::WardenArchitecture::X64});
    REQUIRE(profile != nullptr);
    REQUIRE(!badHash.ReadServer().empty());
    warden::Bytes badResponse = {4};
    badResponse.insert(badResponse.end(),
        profile->rekey.expectedResponse.begin(),
        profile->rekey.expectedResponse.end());
    badResponse.back() ^= 0x01;
    badHash.SendClient(std::move(badResponse));
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
    REQUIRE(aggressive.ReachHealthy(warden::WardenArchitecture::X86));
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

TEST(WardenServer_profile_probe_rejects_generic_evidence_completion)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X86));
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
    REQUIRE(profile.ReachReadyForWorld(warden::WardenArchitecture::X86));
    profile.server->Update(true, 0);
    REQUIRE(profile.server->GetState() ==
        warden::WardenState::ProfileProbeSent);
    profile.reenterState = warden::WardenState::ProfileClassified;
    std::vector<warden::Bytes> probe =
        warden::test::X86StockFingerprint();
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

TEST(WardenServer_x64_real_module_check_flow_reaches_healthy)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X64));

    harness.server->Update(true, 0);
    std::optional<warden::CheckPlan> probe =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(probe.has_value());
    warden::Bytes const probeRequest = harness.ReadServer();
    REQUIRE(!probeRequest.empty());
    CHECK_EQ(probeRequest[0], uint8(2));

    warden::Bytes probeResult;
    REQUIRE(BuildClientCheckResult(*probe,
        warden::test::X64StockFingerprint(), probeResult));
    harness.SendClient(std::move(probeResult));
    CHECK(harness.server->GetState() ==
        warden::WardenState::InitialChecksSent);

    std::optional<warden::CheckPlan> initial =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(initial.has_value());
    REQUIRE(initial->checks.size() == 3u);
    warden::Bytes const initialRequest = harness.ReadServer();
    CHECK_HEX(initialRequest.data(), initialRequest.size(),
        "02044f4b41590a576f772d36342e65786500"
        "bf0401630206136c561055");

    warden::Bytes initialResult;
    REQUIRE(BuildClientCheckResult(*initial, {}, initialResult));
    CHECK_HEX(initialResult.data(), initialResult.size(),
        "021c0000e6163b017856341200044f6b617900"
        "4883c9ff33c0488bfdbaf0d8fffff2ae");
    harness.SendClient(std::move(initialResult));
    CHECK(harness.server->GetState() == warden::WardenState::Healthy);
    REQUIRE(harness.evidence.size() == 1u);
    CHECK(warden::IsCompleteCleanOperatorBatch(harness.evidence.front()));

    std::vector<warden::WardenState> const states = States(harness);
    CHECK(std::find(states.begin(), states.end(),
        warden::WardenState::ProvisionalTimingProbeSent) == states.end());
    CHECK(std::find(states.begin(), states.end(),
        warden::WardenState::ProvisionalValidated) == states.end());
}

TEST(WardenServer_x64_mpq_checks_compose_end_to_end)
{
    auto const checks = std::make_shared<warden::WardenCheckCatalog const>(
        BuildSyntheticMpqCheckCatalog());
    Harness harness(true, "enUS", warden::WardenEnforcementMode::Observe,
        checks);
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X64));

    harness.server->Update(true, 0);
    std::optional<warden::CheckPlan> probe =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(probe.has_value());
    REQUIRE(!harness.ReadServer().empty());

    warden::Bytes probeResult;
    REQUIRE(BuildClientCheckResult(*probe,
        warden::test::X64StockFingerprint(), probeResult));
    harness.SendClient(std::move(probeResult));

    std::optional<warden::CheckPlan> initial =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(initial.has_value());
    auto const plannedMpq = std::find_if(initial->checks.begin(),
        initial->checks.end(), [](warden::WardenCheckDefinition const& check)
        {
            return warden::GetWardenCheckType(check) ==
                warden::WardenCheckType::Mpq;
        });
    REQUIRE(plannedMpq != initial->checks.end());

    warden::Bytes const request = harness.ReadServer();
    std::string const pathText = "DBFilesClient\\Item.db2";
    warden::Bytes const path(pathText.begin(), pathText.end());
    CHECK(std::search(request.begin(), request.end(), path.begin(),
        path.end()) != request.end());

    warden::Bytes result;
    REQUIRE(BuildClientCheckResult(*initial, {}, result));
    harness.SendClient(std::move(result));
    CHECK(harness.server->GetState() == warden::WardenState::Healthy);
    REQUIRE(harness.evidence.size() == 1u);
    auto const mpqEvidence = std::find_if(
        harness.evidence.front().evidence.begin(),
        harness.evidence.front().evidence.end(),
        [](warden::WardenEvidence const& item)
        {
            return item.checkType == warden::WardenCheckType::Mpq;
        });
    REQUIRE(mpqEvidence != harness.evidence.front().evidence.end());
    CHECK(mpqEvidence->evidenceClass ==
        warden::WardenEvidenceClass::Corroboration);
    CHECK(mpqEvidence->outcome == warden::WardenCheckOutcome::Match);
}

TEST(WardenServer_x64_unstable_timing_is_nonactionable_and_recurs)
{
    Harness harness;
    REQUIRE(harness.ReachReadyForWorld(warden::WardenArchitecture::X64));
    harness.server->Update(true, 0);

    std::optional<warden::CheckPlan> probe =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(probe.has_value());
    REQUIRE(!harness.ReadServer().empty());
    warden::Bytes probeResult;
    REQUIRE(BuildClientCheckResult(*probe,
        warden::test::X64StockFingerprint(), probeResult));
    harness.SendClient(std::move(probeResult));

    std::optional<warden::CheckPlan> initial =
        warden::WardenServerTestAccess::PendingCheckPlan(*harness.server);
    REQUIRE(initial.has_value());
    REQUIRE(!harness.ReadServer().empty());
    warden::Bytes initialResult;
    REQUIRE(BuildClientCheckResult(*initial, {}, initialResult, false));
    harness.SendClient(std::move(initialResult));

    CHECK(harness.server->GetState() == warden::WardenState::Recurring);
    REQUIRE(harness.evidence.size() == 1u);
    auto const timing = std::find_if(harness.evidence.front().evidence.begin(),
        harness.evidence.front().evidence.end(),
        [](warden::WardenEvidence const& item)
        {
            return item.checkType == warden::WardenCheckType::Timing;
        });
    REQUIRE(timing != harness.evidence.front().evidence.end());
    CHECK(timing->outcome == warden::WardenCheckOutcome::Unstable);
    CHECK(!warden::IsActionableEvidenceClass(timing->evidenceClass));
    CHECK(!harness.server->QueueConfirmation(timing->checkId));
    std::vector<warden::WardenState> const states = States(harness);
    CHECK(std::find(states.begin(), states.end(),
        warden::WardenState::Healthy) == states.end());
}

TEST(WardenServer_x86_hash_rekeys_and_sends_exact_initialization)
{
    Harness harness;
    REQUIRE(harness.CacheHit(warden::WardenArchitecture::X86));
    REQUIRE(harness.CompleteModuleHash(warden::WardenArchitecture::X86));
    CHECK(harness.server->GetState() == warden::WardenState::ReadyForWorld);

    warden::Bytes const initialization = harness.ReadServer();
    CHECK_HEX(initialization.data(), initialization.size(),
        "030c00e9ab2bd104000010d3430030c2430001"
        "0308004ca09e6c0101004097470001");
    CHECK(harness.evidence.empty());
}
