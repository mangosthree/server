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

    /// Cata: Bane of Agony. Banes share a single debuff slot; back off once the
    /// bot also knows Bane of Doom so the two curses stop fighting over it.
    class CurseOfAgonyTrigger : public DebuffTrigger
    {
    public:
        CurseOfAgonyTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "bane of agony") {}
        virtual bool IsActive() { return DebuffTrigger::IsActive() && !bot->HasSpell(603); }
    };

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

    class ShadowTranceTrigger : public HasAuraIdTrigger
    {
    public:
        ShadowTranceTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "shadow trance", 17941) {}
    };

    /// ID-based: talent auras 34935/34938/34939 share the "backlash" display
    /// name with the proc itself, so a name-based check fired every tick and
    /// pinned Shadow Bolt at relevance 20 over Incinerate for every Destro bot.
    class BacklashTrigger : public HasAuraIdTrigger
    {
    public:
        BacklashTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "backlash", 34936) {}
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

    DEBUFF_TRIGGER(BaneOfDoomTrigger, "bane of doom", "bane of doom");

    class CurseOfTheElementsTrigger : public DebuffTrigger
    {
    public:
        CurseOfTheElementsTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "curse of the elements") {}
    };

    class DemonSoulTrigger : public BoostTrigger
    {
    public:
        DemonSoulTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "demon soul") {}
    };

    /// Demo: Molten Core (Immolate/Corruption/Incinerate crit) procs 71165/71164/71162.
    /// ID-based on self; the talent (47245/47246/47247) shares the display name.
    /// runtime-verify: unconfirmed whether m3 core ever applies these aura ids —
    /// wired as a safe no-op either way.
    class MoltenCoreTrigger : public Trigger
    {
    public:
        MoltenCoreTrigger(PlayerbotAI* ai) : Trigger(ai, "molten core") {}
        virtual bool IsActive();
    };

    /// Demo: Decimation (63167 talented / 63165 base) — free/cheap Soul Fire
    /// below 35% target health. Core-supported: UnitAuraProcHandler.cpp ~3956-3971.
    class DecimationTrigger : public Trigger
    {
    public:
        DecimationTrigger(PlayerbotAI* ai) : Trigger(ai, "decimation") {}
        virtual bool IsActive();
    };

    /// Destro: Empowered Imp instant-cast-Soul-Fire proc.
    /// runtime-verify: aura 47283 confirmed DEAD in m3 core (never applied) —
    /// wired as a safe no-op, future-proof.
    class EmpoweredImpTrigger : public HasAuraIdTrigger
    {
    public:
        EmpoweredImpTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "empowered imp", 47283) {}
    };

    /// Destro: talent-passive gate (18119/18120) with buff-missing check (85383).
    /// runtime-verify: core aura 85383 confirmed never applied in m3 — wired as a
    /// safe no-op. Throttled: unthrottled would spam-hardcast Soul Fire every tick.
    class ImprovedSoulFireTrigger : public Trigger
    {
    public:
        ImprovedSoulFireTrigger(PlayerbotAI* ai) : Trigger(ai, "improved soul fire", 20) {}
        virtual bool IsActive();
    };

}
