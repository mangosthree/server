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

    class VictoryRushTrigger : public HasAuraTrigger
    {
    public:
        VictoryRushTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "victory rush") {}
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
