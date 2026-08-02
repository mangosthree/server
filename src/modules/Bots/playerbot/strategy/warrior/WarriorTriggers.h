#pragma once
#include "../triggers/GenericTriggers.h"

namespace ai
{
    BUFF_TRIGGER(BattleShoutTrigger, "battle shout", "battle shout")

    DEBUFF_TRIGGER(RendDebuffTrigger, "rend", "rend")
    DEBUFF_TRIGGER(DisarmDebuffTrigger, "disarm", "disarm")
    DEBUFF_TRIGGER(SunderArmorDebuffTrigger, "sunder armor", "sunder armor")

    class RendDebuffOnAttackerTrigger : public DebuffOnAttackerTrigger
    {
    public:
        RendDebuffOnAttackerTrigger(PlayerbotAI* ai) : DebuffOnAttackerTrigger(ai, "rend") {}
    };

    class RevengeAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        RevengeAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "revenge") {}
    };

    class ShieldSlamAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        ShieldSlamAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "shield slam") {}
    };

    class ShieldBlockAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        ShieldBlockAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "shield block") {}
    };

    class BloodthirstAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        BloodthirstAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "bloodthirst") {}
    };

    DEBUFF_TRIGGER(ThunderClapDebuffTrigger, "thunder clap", "thunder clap")
    DEBUFF_TRIGGER(DemoralizingShoutDebuffTrigger, "demoralizing shout", "demoralizing shout")

    // HasAuraIdTrigger now lives in the shared triggers/GenericTriggers.h so any
    // class can reuse the id-based proc check.

    // runtime-verify: Sword and Board proc id 50227 (free Shield Slam)
    class SwordAndBoardProcTrigger : public HasAuraIdTrigger
    {
    public:
        SwordAndBoardProcTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "sword and board", 50227) {}
    };

    // runtime-verify: Battle Trance proc id 85742 (free Heroic Strike)
    class BattleTranceProcTrigger : public HasAuraIdTrigger
    {
    public:
        BattleTranceProcTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "battle trance", 85742) {}
    };

    // runtime-verify: Bloodsurge proc id 46916 (instant Slam, Fury)
    class BloodsurgeProcTrigger : public HasAuraIdTrigger
    {
    public:
        BloodsurgeProcTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "bloodsurge", 46916) {}
    };

    // Fury gate: bot knows Bloodthirst (spec check) and is not yet in Berserker Stance
    class BerserkerStanceTrigger : public BuffTrigger
    {
    public:
        BerserkerStanceTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "berserker stance") {}
        virtual bool IsActive()
        {
            return BuffTrigger::IsActive() && AI_VALUE2(uint32, "spell id", "bloodthirst") != 0;
        }
    };

    // Fury enrage-weave gate: not already enraged/boosted
    class NotEnragedTrigger : public Trigger
    {
    public:
        NotEnragedTrigger(PlayerbotAI* ai) : Trigger(ai, "not enraged") {}
        virtual bool IsActive()
        {
            return AI_VALUE2(uint32, "spell id", "bloodthirst") != 0 &&
                !ai->HasAnyAuraOf(ai->GetBot(), "enrage", "death wish", "berserker rage", NULL);
        }
    };

    /// Cata 4.3.4: Shield Bash removed; Pummel interrupts in all stances
    class PummelInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        PummelInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "pummel") {}
    };

    // Cata 4.3.4: Victory Rush is enabled by the "Victorious" proc buff (32216), not an aura named "victory rush"
    class VictoryRushTrigger : public HasAuraTrigger
    {
    public:
        VictoryRushTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "victorious") {}
    };

    class MortalStrikeAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        MortalStrikeAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "mortal strike") {}
    };

    class ColossusSmashAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        ColossusSmashAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "colossus smash") {}
    };

    // runtime-verify: Arms Overpower proc id 60503 ("Taste for Blood"); the
    // display name collides with the permanent enabling talent, so this must
    // be id-based, not HasAuraTrigger(name).
    class TasteForBloodTrigger : public HasAuraIdTrigger
    {
    public:
        TasteForBloodTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "taste for blood", 60503) {}
    };

    class RagingBlowAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        RagingBlowAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "raging blow") {}
    };

    class RecklessnessTrigger : public BoostTrigger
    {
    public:
        RecklessnessTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "recklessness") {}
    };

    class DeadlyCalmTrigger : public BoostTrigger
    {
    public:
        DeadlyCalmTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "deadly calm") {}
    };

    class ConcussionBlowTrigger : public SnareTargetTrigger
    {
    public:
        ConcussionBlowTrigger(PlayerbotAI* ai) : SnareTargetTrigger(ai, "concussion blow") {}
    };

    class HamstringTrigger : public SnareTargetTrigger
    {
    public:
        HamstringTrigger(PlayerbotAI* ai) : SnareTargetTrigger(ai, "hamstring") {}
    };

    class DeathWishTrigger : public BoostTrigger
    {
    public:
        DeathWishTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "death wish") {}
    };

    class PummelInterruptEnemyHealerSpellTrigger : public InterruptEnemyHealerTrigger
    {
    public:
        PummelInterruptEnemyHealerSpellTrigger(PlayerbotAI* ai) : InterruptEnemyHealerTrigger(ai, "pummel") {}
    };

}
