#pragma once
#include "../triggers/GenericTriggers.h"

namespace ai
{
    class DemonArmorTrigger : public BuffTrigger
    {
    public:
        DemonArmorTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "demon armor") {}
        virtual bool IsActive();
    };

    DEBUFF_TRIGGER(CurseOfAgonyTrigger, "bane of agony", "bane of agony"); ///< Cata: Bane of Agony
    DEBUFF_TRIGGER(CorruptionTrigger, "corruption", "corruption");

    class CorruptionOnAttackerTrigger : public DebuffOnAttackerTrigger
    {
    public:
        CorruptionOnAttackerTrigger(PlayerbotAI* ai) : DebuffOnAttackerTrigger(ai, "corruption") {}
    };

    DEBUFF_TRIGGER(ImmolateTrigger, "immolate", "immolate");
    DEBUFF_TRIGGER(UnstableAfflictionTrigger, "unstable affliction", "unstable affliction");
    DEBUFF_TRIGGER(HauntTrigger, "haunt", "haunt");

    class MetamorphosisTrigger : public BoostTrigger
    {
    public:
        MetamorphosisTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "metamorphosis") {}
    };

    class HandOfGuldanTrigger : public SpellCanBeCastTrigger
    {
    public:
        HandOfGuldanTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "hand of gul'dan") {}
    };

    class ImmolationAuraTrigger : public BuffTrigger
    {
    public:
        ImmolationAuraTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "immolation aura") {}
    };

    class ChaosBoltTrigger : public SpellCanBeCastTrigger
    {
    public:
        ChaosBoltTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "chaos bolt") {}
    };

    /// Own cooldown gate for conflagrate; only active while immolate is on the target.
    class ConflagrateTrigger : public SpellCanBeCastTrigger
    {
    public:
        ConflagrateTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "conflagrate") {}
        virtual bool IsActive();
    };

    class ShadowTranceTrigger : public HasAuraTrigger
    {
    public:
        ShadowTranceTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "shadow trance") {}
    };

    class BacklashTrigger : public HasAuraTrigger
    {
    public:
        BacklashTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "backlash") {}
    };

    class BanishTrigger : public HasCcTargetTrigger
    {
    public:
        BanishTrigger(PlayerbotAI* ai) : HasCcTargetTrigger(ai, "banish") {}
    };

    class WarlockConjuredItemTrigger : public ItemCountTrigger
    {
    public:
        WarlockConjuredItemTrigger(PlayerbotAI* ai, string item) : ItemCountTrigger(ai, item, 1) {}

        // Cata: conjuring stones no longer consumes soul shard items
        virtual bool IsActive() { return ItemCountTrigger::IsActive(); }
    };

    class HasHealthstoneTrigger : public WarlockConjuredItemTrigger
    {
    public:
        HasHealthstoneTrigger(PlayerbotAI* ai) : WarlockConjuredItemTrigger(ai, "healthstone") {}
    };

    class FearTrigger : public HasCcTargetTrigger
    {
    public:
        FearTrigger(PlayerbotAI* ai) : HasCcTargetTrigger(ai, "fear") {}
    };

}
