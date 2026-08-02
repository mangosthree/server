#pragma once
#include "../triggers/GenericTriggers.h"

namespace ai
{
    class ArcaneIntellectOnPartyTrigger : public BuffOnPartyTrigger {
    public:
        ArcaneIntellectOnPartyTrigger(PlayerbotAI* ai) : BuffOnPartyTrigger(ai, "arcane intellect", 7) {}
    };

    class ArcaneIntellectTrigger : public BuffTrigger {
    public:
        ArcaneIntellectTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "arcane intellect", 5) {}
    };

    class MageArmorTrigger : public BuffTrigger {
    public:
        MageArmorTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "mage armor", 5) {}
        virtual bool IsActive();
    };

    class LivingBombTrigger : public DebuffTrigger {
    public:
        LivingBombTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "living bomb") {}
    };

    class FireballTrigger : public DebuffTrigger {
    public:
        FireballTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "fireball") {}
    };

    class PyroblastTrigger : public DebuffTrigger {
    public:
        PyroblastTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "pyroblast") {}
    };

    /// ID-based (48108): the proc shares its display name with a permanent
    /// talent passive, so name-based lookup reported "up" every tick.
    class HotStreakTrigger : public HasAuraIdTrigger {
    public:
        HotStreakTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "hot streak", 48108) {}
    };

    /// Cata proc aura (the WotLK "missile barrage" name is gone); fires the free
    /// instant Arcane Missiles cast.
    class ArcaneMissilesProcTrigger : public HasAuraTrigger {
    public:
        ArcaneMissilesProcTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "arcane missiles!") {}
    };

    /// Active while the self "arcane blast" buff is below its 4-stack cap, so the
    /// bot keeps stacking instead of alternating AB/Barrage every cycle.
    /// runtime-verify: ID-based on the stacking aura (36032), not the 30451
    /// cast spell the old "spell id" lookup indirectly resolved through.
    class ArcaneBlastTrigger : public BuffTrigger {
    public:
        ArcaneBlastTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "arcane blast") {}
        virtual bool IsActive();
    };

    /// Capped at 4 stacks: dumps via Barrage. Sole stack dump -- this core
    /// has no Arcane Missiles! proc chain (0 hits for 79683/79808 in
    /// src/game). ID-based on the stacking aura (36032), runtime-verify.
    class ArcaneBlastCappedTrigger : public HasAuraStacksTrigger
    {
    public:
        ArcaneBlastCappedTrigger(PlayerbotAI* ai) : HasAuraStacksTrigger(ai, "arcane blast", 36032, 4) {}
    };

    class CounterspellInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        CounterspellInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "counterspell") {}
    };

    class CombustionTrigger : public BoostTrigger
    {
    public:
        CombustionTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "combustion") {}
    };

    class IcyVeinsTrigger : public BoostTrigger
    {
    public:
        IcyVeinsTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "icy veins") {}
    };

    /// ID-based, runtime-verify 74396 vs 44544 (using 74396): the talent
    /// passive shares "fingers of frost" with the proc, so name lookup
    /// reported "up" every tick.
    class FingersOfFrostTrigger : public HasAuraIdTrigger
    {
    public:
        FingersOfFrostTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "fingers of frost", 74396) {}
    };

    /// ID-based (57761): same name-collision issue as Fingers of Frost above.
    class BrainFreezeTrigger : public HasAuraIdTrigger
    {
    public:
        BrainFreezeTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "brain freeze", 57761) {}
    };

    class PolymorphTrigger : public HasCcTargetTrigger
    {
    public:
        PolymorphTrigger(PlayerbotAI* ai) : HasCcTargetTrigger(ai, "polymorph") {}
    };

    class RemoveCurseTrigger : public NeedCureTrigger
    {
    public:
        RemoveCurseTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "remove curse", DISPEL_CURSE) {}
    };

    class PartyMemberRemoveCurseTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        PartyMemberRemoveCurseTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "remove curse", DISPEL_CURSE) {}
    };

    class SpellstealTrigger : public TargetAuraDispelTrigger
    {
    public:
        SpellstealTrigger(PlayerbotAI* ai) : TargetAuraDispelTrigger(ai, "spellsteal", DISPEL_MAGIC) {}
    };

    class CounterspellEnemyHealerTrigger : public InterruptEnemyHealerTrigger
    {
    public:
        CounterspellEnemyHealerTrigger(PlayerbotAI* ai) : InterruptEnemyHealerTrigger(ai, "counterspell") {}
    };

    /// Arcane burn/conserve model (tauri): once mana drops to ~30% mid-burn,
    /// Evocation refills it instead of running the bot dry. Neither shared
    /// LowManaTrigger (15%) nor MediumManaTrigger (40%) matches this threshold,
    /// so this is Arcane-local.
    class ArcaneBurnEvocationTrigger : public Trigger
    {
    public:
        ArcaneBurnEvocationTrigger(PlayerbotAI* ai) : Trigger(ai, "arcane burn evocation") {}
        virtual string GetTargetName() { return "self target"; }
        virtual bool IsActive();
    };

    /// Burn-opener cooldown; mirrors CombustionTrigger/IcyVeinsTrigger's
    /// BoostTrigger wiring. runtime-verify "arcane power" resolves.
    class ArcanePowerTrigger : public BoostTrigger
    {
    public:
        ArcanePowerTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "arcane power") {}
    };

    /// Shared cooldown filler (Fire/Frost/Arcane) via GenericMageStrategy;
    /// mirrors shaman's EarthShockReadyTrigger/LavaLashReadyTrigger
    /// SpellCanBeCastTrigger pattern. runtime-verify "flame orb" resolves.
    class FlameOrbReadyTrigger : public SpellCanBeCastTrigger
    {
    public:
        FlameOrbReadyTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "flame orb") {}
    };
}
