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

    // runtime-verify: Arms Overpower is enabled by the "Taste for Blood" proc
    class TasteForBloodTrigger : public HasAuraTrigger
    {
    public:
        TasteForBloodTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "taste for blood") {}
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

    class SwordAndBoardTrigger : public HasAuraTrigger
    {
    public:
        SwordAndBoardTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "sword and board") {}
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
