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

#include "WardenEnforcementPolicy.h"
#include "WardenServer.h"

#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>

namespace
{
warden::WardenEvidence Evidence(uint32 checkId, warden::WardenCheckType type,
    warden::WardenEvidenceClass evidenceClass,
    warden::WardenCheckOutcome outcome)
{
    return {7, checkId, type, evidenceClass, outcome, 0};
}

warden::WardenEvidenceBatch Batch(warden::CheckPlanPurpose purpose,
    std::initializer_list<warden::WardenEvidence> evidence)
{
    warden::WardenEvidenceBatch batch;
    batch.requestId = 7;
    batch.purpose = purpose;
    batch.evidence.assign(evidence.begin(), evidence.end());
    return batch;
}

std::vector<warden::WardenPolicyDecision> Confirm(
    warden::WardenEnforcementMode mode, warden::WardenEvidence first,
    warden::WardenEvidence second)
{
    warden::WardenEnforcementPolicy policy(mode, true);
    std::vector<warden::WardenPolicyDecision> const queued =
        policy.EvaluateBatch(Batch(warden::CheckPlanPurpose::Recurring,
            {first}));
    if (queued.size() != 1u || queued[0].action !=
            warden::WardenPolicyAction::QueueConfirmation)
        return {};
    return policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Confirmation, {second}));
}
}

TEST(WardenEvidence_shared_confirmation_and_disposition_predicates)
{
    using warden::WardenCheckOutcome;
    using warden::WardenCheckType;
    using warden::WardenConfirmedDisposition;
    using warden::WardenEnforcementMode;
    using warden::WardenEvidenceClass;

    warden::WardenEvidence integrity = Evidence(1, WardenCheckType::Mem,
        WardenEvidenceClass::IntegrityInvariant,
        WardenCheckOutcome::Mismatch);
    CHECK(warden::NeedsConfirmation(integrity));
    CHECK(warden::ClassifyConfirmedEvidence(WardenEnforcementMode::Observe,
        integrity) == WardenConfirmedDisposition::Audit);
    CHECK(warden::ClassifyConfirmedEvidence(WardenEnforcementMode::Kick,
        integrity) == WardenConfirmedDisposition::Incident);

    integrity.outcome = WardenCheckOutcome::Unavailable;
    CHECK(!warden::NeedsConfirmation(integrity));
    CHECK(warden::ClassifyConfirmedEvidence(
        WardenEnforcementMode::KickAndBan, integrity) ==
        WardenConfirmedDisposition::Audit);
    integrity.outcome = WardenCheckOutcome::Match;
    CHECK(!warden::NeedsConfirmation(integrity));
    CHECK(warden::ClassifyConfirmedEvidence(WardenEnforcementMode::Kick,
        integrity) == WardenConfirmedDisposition::Cleared);

    warden::WardenEvidence corroboration = Evidence(2,
        WardenCheckType::Lua, WardenEvidenceClass::Corroboration,
        WardenCheckOutcome::Mismatch);
    CHECK(!warden::NeedsConfirmation(corroboration));
    CHECK(warden::ClassifyConfirmedEvidence(WardenEnforcementMode::Kick,
        corroboration) == WardenConfirmedDisposition::Audit);

    warden::WardenEvidence timing = Evidence(65536,
        WardenCheckType::Timing, WardenEvidenceClass::ProtocolHealth,
        WardenCheckOutcome::Stable);
    CHECK(!warden::NeedsConfirmation(timing));
    CHECK(warden::ClassifyConfirmedEvidence(WardenEnforcementMode::Kick,
        timing) == WardenConfirmedDisposition::Invalid);

    warden::WardenEvidence illegal = Evidence(2, WardenCheckType::Lua,
        WardenEvidenceClass::ThreatSignature, WardenCheckOutcome::Mismatch);
    CHECK(!warden::NeedsConfirmation(illegal));
    CHECK(warden::ClassifyConfirmedEvidence(WardenEnforcementMode::Kick,
        illegal) == WardenConfirmedDisposition::Invalid);
}

TEST(WardenEvidence_healthy_summary_requires_complete_clean_initial_batch)
{
    warden::WardenEvidence timing = Evidence(65536,
        warden::WardenCheckType::Timing,
        warden::WardenEvidenceClass::ProtocolHealth,
        warden::WardenCheckOutcome::Stable);
    warden::WardenEvidence mpq = Evidence(1,
        warden::WardenCheckType::Mpq,
        warden::WardenEvidenceClass::Corroboration,
        warden::WardenCheckOutcome::Match);

    CHECK(warden::IsCompleteCleanOperatorBatch(Batch(
        warden::CheckPlanPurpose::Initial, {timing, mpq})));
    CHECK(!warden::IsCompleteCleanOperatorBatch(Batch(
        warden::CheckPlanPurpose::AggressiveImmediate, {mpq})));
    CHECK(!warden::IsCompleteCleanOperatorBatch(Batch(
        warden::CheckPlanPurpose::Recurring, {timing, mpq})));
    CHECK(!warden::IsCompleteCleanOperatorBatch(Batch(
        warden::CheckPlanPurpose::Initial, {})));

    timing.outcome = warden::WardenCheckOutcome::Unstable;
    CHECK(!warden::IsCompleteCleanOperatorBatch(Batch(
        warden::CheckPlanPurpose::Initial, {timing, mpq})));
    timing.outcome = warden::WardenCheckOutcome::Stable;
    mpq.outcome = warden::WardenCheckOutcome::Unavailable;
    CHECK(!warden::IsCompleteCleanOperatorBatch(Batch(
        warden::CheckPlanPurpose::Initial, {timing, mpq})));
    mpq.outcome = warden::WardenCheckOutcome::Mismatch;
    CHECK(!warden::IsCompleteCleanOperatorBatch(Batch(
        warden::CheckPlanPurpose::Initial, {timing, mpq})));
    mpq.outcome = warden::WardenCheckOutcome::Stable;
    CHECK(!warden::IsCompleteCleanOperatorBatch(Batch(
        warden::CheckPlanPurpose::Initial, {timing, mpq})));
    mpq.outcome = warden::WardenCheckOutcome::Match;
    mpq.checkId = 0;
    CHECK(!warden::IsCompleteCleanOperatorBatch(Batch(
        warden::CheckPlanPurpose::Initial, {timing, mpq})));
}

TEST(WardenEnforcementPolicy_routes_cata_actionability_by_mode)
{
    for (warden::WardenEnforcementMode mode :
        {warden::WardenEnforcementMode::Observe,
            warden::WardenEnforcementMode::Kick,
            warden::WardenEnforcementMode::KickAndBan})
    {
        warden::WardenEnforcementPolicy policy(mode, true);
        auto const decisions = policy.EvaluateBatch(Batch(
            warden::CheckPlanPurpose::Initial,
            {
                Evidence(1, warden::WardenCheckType::Mpq,
                    warden::WardenEvidenceClass::Corroboration,
                    warden::WardenCheckOutcome::Mismatch),
                Evidence(2, warden::WardenCheckType::Lua,
                    warden::WardenEvidenceClass::Corroboration,
                    warden::WardenCheckOutcome::Mismatch),
                Evidence(3, warden::WardenCheckType::Mem,
                    warden::WardenEvidenceClass::ThreatSignature,
                    warden::WardenCheckOutcome::Mismatch),
                Evidence(65536, warden::WardenCheckType::Timing,
                    warden::WardenEvidenceClass::ProtocolHealth,
                    warden::WardenCheckOutcome::Unstable)
            }));

        REQUIRE(decisions.size() == 3u);
        CHECK_EQ(decisions[0].checkId, uint32(1));
        CHECK(decisions[0].action ==
            warden::WardenPolicyAction::PersistAudit);
        CHECK_EQ(decisions[1].checkId, uint32(2));
        CHECK(decisions[1].action ==
            warden::WardenPolicyAction::PersistAudit);
        CHECK_EQ(decisions[2].checkId, uint32(3));
        CHECK(decisions[2].action ==
            (mode == warden::WardenEnforcementMode::Observe ?
                warden::WardenPolicyAction::PersistAudit :
                warden::WardenPolicyAction::QueueConfirmation));

        auto const abandoned = policy.AbortPendingConfirmations();
        CHECK_EQ(abandoned.size(), mode == warden::WardenEnforcementMode::Observe ?
            std::size_t(0) : std::size_t(1));
    }
}

TEST(WardenEnforcementPolicy_profile_probe_never_queues_confirmation)
{
    for (warden::WardenEnforcementMode mode :
        {warden::WardenEnforcementMode::Observe,
            warden::WardenEnforcementMode::Kick,
            warden::WardenEnforcementMode::KickAndBan})
    {
        warden::WardenEnforcementPolicy policy(mode, true);
        auto const decisions = policy.EvaluateBatch(Batch(
            warden::CheckPlanPurpose::ProfileProbe,
            {Evidence(9, warden::WardenCheckType::Mem,
                warden::WardenEvidenceClass::ThreatSignature,
                warden::WardenCheckOutcome::Mismatch)}));

        REQUIRE(decisions.size() == 1u);
        CHECK(decisions[0].action ==
            warden::WardenPolicyAction::PersistAudit);
        CHECK_EQ(decisions[0].checkId, uint32(9));
        CHECK(policy.AbortPendingConfirmations().empty());
    }
}

TEST(WardenEnforcementPolicy_matching_confirmation_clears_actionable_types)
{
    struct Case
    {
        uint32 id;
        warden::WardenCheckType type;
        warden::WardenEvidenceClass evidenceClass;
    };
    Case const cases[] =
    {
        {2, warden::WardenCheckType::Mem,
            warden::WardenEvidenceClass::IntegrityInvariant},
        {3, warden::WardenCheckType::Mem,
            warden::WardenEvidenceClass::ThreatSignature}
    };

    for (Case const& test : cases)
    {
        auto const decisions = Confirm(
            warden::WardenEnforcementMode::KickAndBan,
            Evidence(test.id, test.type, test.evidenceClass,
                warden::WardenCheckOutcome::Mismatch),
            Evidence(test.id, test.type, test.evidenceClass,
                warden::WardenCheckOutcome::Match));
        REQUIRE(decisions.size() == 1u);
        CHECK(decisions[0].action ==
            warden::WardenPolicyAction::ConfirmationCleared);
        CHECK_EQ(decisions[0].checkId, test.id);
    }
}

TEST(WardenEnforcementPolicy_confirmed_actionable_mismatch_kicks)
{
    for (warden::WardenEnforcementMode mode :
        {warden::WardenEnforcementMode::Kick,
            warden::WardenEnforcementMode::KickAndBan})
    {
        auto const mismatch = Evidence(3, warden::WardenCheckType::Mem,
            warden::WardenEvidenceClass::ThreatSignature,
            warden::WardenCheckOutcome::Mismatch);
        auto const decisions = Confirm(mode, mismatch, mismatch);
        REQUIRE(decisions.size() == 1u);
        CHECK(decisions[0].action ==
            warden::WardenPolicyAction::PersistAndKick);
    }
}

TEST(WardenEnforcementPolicy_direct_operational_audits_are_deduplicated)
{
    warden::WardenEnforcementPolicy policy(
        warden::WardenEnforcementMode::Kick, true);
    auto const corroboration = Evidence(2, warden::WardenCheckType::Lua,
        warden::WardenEvidenceClass::Corroboration,
        warden::WardenCheckOutcome::Mismatch);
    auto unavailable = Evidence(3, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::ThreatSignature,
        warden::WardenCheckOutcome::Unavailable);

    auto decisions = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring,
        {corroboration, unavailable}));
    REQUIRE(decisions.size() == 2u);
    CHECK(decisions[0].action == warden::WardenPolicyAction::PersistAudit);
    CHECK(decisions[1].action == warden::WardenPolicyAction::PersistAudit);

    CHECK(policy.EvaluateBatch(Batch(warden::CheckPlanPurpose::Recurring,
        {corroboration, unavailable})).empty());

    unavailable.outcome = warden::WardenCheckOutcome::Mismatch;
    decisions = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring, {unavailable}));
    REQUIRE(decisions.size() == 1u);
    CHECK(decisions[0].action ==
        warden::WardenPolicyAction::QueueConfirmation);
}

TEST(WardenEnforcementPolicy_observe_audits_without_pending_confirmation)
{
    warden::WardenEnforcementPolicy policy(
        warden::WardenEnforcementMode::Observe, true);
    auto const mismatch = Evidence(3, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::ThreatSignature,
        warden::WardenCheckOutcome::Mismatch);

    auto decisions = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring, {mismatch}));
    REQUIRE(decisions.size() == 1u);
    CHECK(decisions[0].action == warden::WardenPolicyAction::PersistAudit);
    CHECK(policy.AbortPendingConfirmations().empty());
    CHECK(policy.EvaluateBatch(Batch(warden::CheckPlanPurpose::Recurring,
        {mismatch})).empty());

    decisions = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Confirmation, {mismatch}));
    REQUIRE(decisions.size() == 1u);
    CHECK(decisions[0].action == warden::WardenPolicyAction::Disengage);
}

TEST(WardenEnforcementPolicy_confirmation_unavailable_is_audited_not_punished)
{
    warden::WardenEnforcementPolicy policy(
        warden::WardenEnforcementMode::KickAndBan, true);
    auto mismatch = Evidence(3, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::ThreatSignature,
        warden::WardenCheckOutcome::Mismatch);
    auto unavailable = mismatch;
    unavailable.outcome = warden::WardenCheckOutcome::Unavailable;

    auto queued = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring, {mismatch}));
    REQUIRE(queued.size() == 1u);
    CHECK(queued[0].action ==
        warden::WardenPolicyAction::QueueConfirmation);

    auto audited = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Confirmation, {unavailable}));
    REQUIRE(audited.size() == 1u);
    CHECK(audited[0].action == warden::WardenPolicyAction::PersistAudit);
    CHECK(audited[0].outcome == warden::WardenCheckOutcome::Unavailable);

    queued = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring, {mismatch}));
    REQUIRE(queued.size() == 1u);
    CHECK(queued[0].action ==
        warden::WardenPolicyAction::QueueConfirmation);
}

TEST(WardenEnforcementPolicy_confirmation_contract_checks_id_type_and_class)
{
    auto const pending = Evidence(3, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::IntegrityInvariant,
        warden::WardenCheckOutcome::Mismatch);

    for (uint32 mutation = 0; mutation < 4; ++mutation)
    {
        warden::WardenEnforcementPolicy policy(
            warden::WardenEnforcementMode::Kick, true);
        REQUIRE(policy.EvaluateBatch(Batch(
            warden::CheckPlanPurpose::Recurring, {pending})).size() == 1u);
        warden::WardenEvidence wrong = pending;
        if (mutation == 0)
            wrong.checkId = 9999;
        else if (mutation == 1)
            wrong.checkType = warden::WardenCheckType::Mpq;
        else if (mutation == 2)
            wrong.evidenceClass =
                warden::WardenEvidenceClass::ThreatSignature;
        else
        {
            wrong.checkType = warden::WardenCheckType::Timing;
            wrong.evidenceClass =
                warden::WardenEvidenceClass::ProtocolHealth;
            wrong.outcome = warden::WardenCheckOutcome::Stable;
        }

        auto const decisions = policy.EvaluateBatch(Batch(
            warden::CheckPlanPurpose::Confirmation, {wrong}));
        REQUIRE(decisions.size() == 2u);
        CHECK(decisions[0].action ==
            warden::WardenPolicyAction::PersistAudit);
        CHECK_EQ(decisions[0].checkId, uint32(3));
        CHECK(decisions[0].outcome ==
            warden::WardenCheckOutcome::Unavailable);
        CHECK(decisions[1].action == warden::WardenPolicyAction::Kick);
        CHECK(policy.AbortPendingConfirmations().empty());
    }
}

TEST(WardenEnforcementPolicy_confirmation_requires_exactly_one_item)
{
    auto const evidence = Evidence(3, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::ThreatSignature,
        warden::WardenCheckOutcome::Mismatch);

    for (warden::WardenEvidenceBatch const& malformed :
        {Batch(warden::CheckPlanPurpose::Confirmation, {}),
            Batch(warden::CheckPlanPurpose::Confirmation,
                {evidence, evidence})})
    {
        warden::WardenEnforcementPolicy policy(
            warden::WardenEnforcementMode::KickAndBan, true);
        REQUIRE(policy.EvaluateBatch(Batch(
            warden::CheckPlanPurpose::Recurring, {evidence})).size() == 1u);

        auto const decisions = policy.EvaluateBatch(malformed);
        REQUIRE(decisions.size() == 2u);
        CHECK(decisions[0].action ==
            warden::WardenPolicyAction::PersistAudit);
        CHECK_EQ(decisions[0].checkId, uint32(3));
        CHECK(decisions[0].outcome ==
            warden::WardenCheckOutcome::Unavailable);
        CHECK(decisions[1].action == warden::WardenPolicyAction::Kick);
    }
}

TEST(WardenEnforcementPolicy_invalid_confirmation_audits_before_close)
{
    warden::WardenEnforcementPolicy policy(
        warden::WardenEnforcementMode::Kick, true);
    auto pending = Evidence(3, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::IntegrityInvariant,
        warden::WardenCheckOutcome::Mismatch);
    REQUIRE(policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring, {pending})).size() == 1u);

    // Stable is illegal for MEM. The matched pending identity must still be
    // drained before the operational confirmation-contract failure closes.
    pending.outcome = warden::WardenCheckOutcome::Stable;
    auto const decisions = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Confirmation, {pending}));
    REQUIRE(decisions.size() == 2u);
    CHECK(decisions[0].action == warden::WardenPolicyAction::PersistAudit);
    CHECK_EQ(decisions[0].checkId, uint32(3));
    CHECK(decisions[0].outcome == warden::WardenCheckOutcome::Unavailable);
    CHECK(decisions[1].action == warden::WardenPolicyAction::Kick);
    CHECK(policy.AbortPendingConfirmations().empty());
}

TEST(WardenEnforcementPolicy_abort_drains_only_actionable_pending_metadata)
{
    warden::WardenEnforcementPolicy policy(
        warden::WardenEnforcementMode::Kick, true);
    auto const queued = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring,
        {
            Evidence(1, warden::WardenCheckType::Mem,
                warden::WardenEvidenceClass::IntegrityInvariant,
                warden::WardenCheckOutcome::Mismatch),
            Evidence(3, warden::WardenCheckType::Mem,
                warden::WardenEvidenceClass::ThreatSignature,
                warden::WardenCheckOutcome::Mismatch)
        }));
    REQUIRE(queued.size() == 2u);

    auto const audits = policy.AbortPendingConfirmations();
    REQUIRE(audits.size() == 2u);
    for (warden::WardenPolicyDecision const& audit : audits)
    {
        CHECK(audit.action == warden::WardenPolicyAction::PersistAudit);
        CHECK(audit.outcome == warden::WardenCheckOutcome::Unavailable);
    }
    CHECK(policy.AbortPendingConfirmations().empty());
}

TEST(WardenEnforcementPolicy_actionable_confirmation_preserves_other_pending)
{
    warden::WardenEnforcementPolicy policy(
        warden::WardenEnforcementMode::Kick, true);
    auto const firstMem = Evidence(1, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::IntegrityInvariant,
        warden::WardenCheckOutcome::Mismatch);
    auto const mem = Evidence(3, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::IntegrityInvariant,
        warden::WardenCheckOutcome::Mismatch);

    auto const queued = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring, {firstMem, mem}));
    REQUIRE(queued.size() == 2u);
    CHECK(queued[0].action ==
        warden::WardenPolicyAction::QueueConfirmation);
    CHECK(queued[1].action ==
        warden::WardenPolicyAction::QueueConfirmation);

    auto const confirmed = policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Confirmation, {mem}));
    REQUIRE(confirmed.size() == 1u);
    CHECK(confirmed[0].action ==
        warden::WardenPolicyAction::PersistAndKick);

    auto const abandoned = policy.AbortPendingConfirmations();
    REQUIRE(abandoned.size() == 1u);
    CHECK_EQ(abandoned[0].checkId, uint32(1));
    CHECK(abandoned[0].action == warden::WardenPolicyAction::PersistAudit);
    CHECK(abandoned[0].outcome ==
        warden::WardenCheckOutcome::Unavailable);
}

TEST(WardenEnforcementPolicy_lifecycle_failure_closes_only_enforcing_modes)
{
    warden::WardenLifecycleEvent failed;
    failed.state = warden::WardenState::Failed;
    failed.failure = warden::WardenFailure::DeadlineExpired;
    warden::WardenLifecycleEvent ready;
    ready.state = warden::WardenState::ReadyForWorld;

    for (warden::WardenEnforcementMode mode :
        {warden::WardenEnforcementMode::Observe,
            warden::WardenEnforcementMode::Kick,
            warden::WardenEnforcementMode::KickAndBan})
    {
        warden::WardenEnforcementPolicy policy(mode, true);
        warden::WardenPolicyDecision const failure =
            policy.EvaluateLifecycle(failed);
        CHECK(failure.action ==
            (mode == warden::WardenEnforcementMode::Observe ?
                warden::WardenPolicyAction::None :
                warden::WardenPolicyAction::Kick));
        CHECK(policy.EvaluateLifecycle(ready).action ==
            warden::WardenPolicyAction::None);
    }

    warden::WardenEnforcementPolicy permissive(
        warden::WardenEnforcementMode::KickAndBan, false);
    CHECK(permissive.EvaluateLifecycle(failed).action ==
        warden::WardenPolicyAction::None);

    warden::WardenEnforcementPolicy enforcing(
        warden::WardenEnforcementMode::Kick, true);
    auto const mismatch = Evidence(3, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::IntegrityInvariant,
        warden::WardenCheckOutcome::Mismatch);
    REQUIRE(enforcing.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring, {mismatch})).size() == 1u);
    CHECK(enforcing.EvaluateLifecycle(failed).action ==
        warden::WardenPolicyAction::Kick);
    auto const abandoned = enforcing.AbortPendingConfirmations();
    REQUIRE(abandoned.size() == 1u);
    CHECK_EQ(abandoned[0].checkId, uint32(3));
    CHECK(abandoned[0].outcome ==
        warden::WardenCheckOutcome::Unavailable);
}

TEST(WardenEnforcementPolicy_client_patch_requirement_closes_every_mode)
{
    warden::WardenLifecycleEvent required;
    required.state = warden::WardenState::Failed;
    required.failure = warden::WardenFailure::ClientPatchRequired;

    for (warden::WardenEnforcementMode mode :
        {warden::WardenEnforcementMode::Observe,
            warden::WardenEnforcementMode::Kick,
            warden::WardenEnforcementMode::KickAndBan})
    {
        for (bool requireExactProfile : {false, true})
        {
            warden::WardenEnforcementPolicy policy(mode,
                requireExactProfile);
            warden::WardenPolicyDecision const decision =
                policy.EvaluateLifecycle(required);
            CHECK(decision.action == warden::WardenPolicyAction::Kick);
            CHECK(policy.AbortPendingConfirmations().empty());
        }
    }
}

TEST(WardenEnforcementPolicy_permissive_exact_profile_disengages_on_contract_failure)
{
    warden::WardenEnforcementPolicy policy(
        warden::WardenEnforcementMode::KickAndBan, false);
    auto const pending = Evidence(3, warden::WardenCheckType::Mem,
        warden::WardenEvidenceClass::IntegrityInvariant,
        warden::WardenCheckOutcome::Mismatch);
    REQUIRE(policy.EvaluateBatch(Batch(
        warden::CheckPlanPurpose::Recurring, {pending})).size() == 1u);

    warden::WardenEvidence wrong = pending;
    wrong.checkId = 9999;
    std::vector<warden::WardenPolicyDecision> const decisions =
        policy.EvaluateBatch(Batch(
            warden::CheckPlanPurpose::Confirmation, {wrong}));
    REQUIRE(decisions.size() == 2u);
    CHECK(decisions[0].action == warden::WardenPolicyAction::PersistAudit);
    CHECK(decisions[1].action == warden::WardenPolicyAction::Disengage);
}
