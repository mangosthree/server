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

    class HotStreakTrigger : public HasAuraTrigger {
    public:
        HotStreakTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "hot streak") {}
    };

    /// Cata proc aura (the WotLK "missile barrage" name is gone); fires the free
    /// instant Arcane Missiles cast.
    class ArcaneMissilesProcTrigger : public HasAuraTrigger {
    public:
        ArcaneMissilesProcTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "arcane missiles!") {}
    };

    /// Active while the self "arcane blast" buff is below its 4-stack cap, so the
    /// bot keeps stacking instead of alternating AB/Barrage every cycle.
    class ArcaneBlastTrigger : public BuffTrigger {
    public:
        ArcaneBlastTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "arcane blast") {}
        virtual bool IsActive();
    };

    /// Active once "arcane blast" is at its 4-stack cap; dumps via Barrage.
    class ArcaneBlastCappedTrigger : public BuffTrigger {
    public:
        ArcaneBlastCappedTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "arcane blast") {}
        virtual bool IsActive();
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

    /// core-proc-dependent, runtime-verify: fires ice lance / deep freeze; the
    /// core must actually proc the "fingers of frost" self-buff.
    class FingersOfFrostTrigger : public HasAuraTrigger
    {
    public:
        FingersOfFrostTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "fingers of frost") {}
    };

    /// core-proc-dependent, runtime-verify: fires frostfire bolt; the core must
    /// actually proc the "brain freeze" self-buff.
    class BrainFreezeTrigger : public HasAuraTrigger
    {
    public:
        BrainFreezeTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "brain freeze") {}
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
}
