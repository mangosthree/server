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
