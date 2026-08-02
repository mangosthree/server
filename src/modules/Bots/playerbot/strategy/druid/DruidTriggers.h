#pragma once
#include "../triggers/GenericTriggers.h"

namespace ai {
    class MarkOfTheWildOnPartyTrigger : public BuffOnPartyTrigger
    {
    public:
        MarkOfTheWildOnPartyTrigger(PlayerbotAI* ai) : BuffOnPartyTrigger(ai, "mark of the wild", 7) {}
    };

    class MarkOfTheWildTrigger : public BuffTrigger
    {
    public:
        MarkOfTheWildTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "mark of the wild", 5) {}
    };

    class ThornsTrigger : public BuffTrigger
    {
    public:
        ThornsTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "thorns") {}
    };

    class RakeTrigger : public DebuffTrigger
    {
    public:
        RakeTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "rake") {}
    };

    class InsectSwarmTrigger : public DebuffTrigger
    {
    public:
        InsectSwarmTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "insect swarm") {}
    };

    class MoonfireTrigger : public DebuffTrigger
    {
    public:
        MoonfireTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "moonfire") {}
    };

    class FaerieFireTrigger : public DebuffTrigger
    {
    public:
        FaerieFireTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "faerie fire") {}
    };

    class FaerieFireFeralTrigger : public DebuffTrigger
    {
    public:
        FaerieFireFeralTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "faerie fire (feral)") {}
    };

    class BashInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        BashInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "bash") {}
    };

    class SkullBashInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        SkullBashInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "skull bash") {}
    };

    class TigersFuryTrigger : public BoostTrigger
    {
    public:
        TigersFuryTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "tiger's fury") {}
    };

    class BerserkTrigger : public BoostTrigger
    {
    public:
        BerserkTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "berserk") {}
    };

    class SavageRoarTrigger : public BuffTrigger
    {
    public:
        SavageRoarTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "savage roar") {}
    };

    class NaturesGraspTrigger : public BoostTrigger
    {
    public:
        NaturesGraspTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "nature's grasp") {}
    };

    class EntanglingRootsTrigger : public HasCcTargetTrigger
    {
    public:
        EntanglingRootsTrigger(PlayerbotAI* ai) : HasCcTargetTrigger(ai, "entangling roots") {}
    };

    class CurePoisonTrigger : public NeedCureTrigger
    {
    public:
        CurePoisonTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "remove corruption", DISPEL_POISON) {}
    };

    class PartyMemberCurePoisonTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        PartyMemberCurePoisonTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "remove corruption", DISPEL_POISON) {}
    };

    class BearFormTrigger : public BuffTrigger
    {
    public:
        BearFormTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "bear form") {}
        virtual bool IsActive() { return !ai->HasAnyAuraOf(bot, "bear form", "dire bear form", NULL); }
    };

    class TreeFormTrigger : public BuffTrigger
    {
    public:
        TreeFormTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "tree of life") {}
        virtual bool IsActive() { return !ai->HasAura("tree of life", bot); }
    };

    class CatFormTrigger : public BuffTrigger
    {
    public:
        CatFormTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "cat form") {}
        virtual bool IsActive() { return !ai->HasAura("cat form", bot); }
    };

    /// ID-based: Eclipse (Solar) buff (48517). POWER_ECLIPSE is unimplemented in
    /// this core (UnitPower.cpp max power TODO) and the driver aura is
    /// HandleNULL, so this never fires today; left wired for self-upgrade if
    /// the core ever implements the Eclipse bar.
    class EclipseSolarTrigger : public HasAuraIdTrigger
    {
    public:
        EclipseSolarTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "eclipse (solar)", 48517) {}
    };

    /// ID-based: Eclipse (Lunar) buff (48518). See EclipseSolarTrigger.
    class EclipseLunarTrigger : public HasAuraIdTrigger
    {
    public:
        EclipseLunarTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "eclipse (lunar)", 48518) {}
    };

    class BashInterruptEnemyHealerSpellTrigger : public InterruptEnemyHealerTrigger
    {
    public:
        BashInterruptEnemyHealerSpellTrigger(PlayerbotAI* ai) : InterruptEnemyHealerTrigger(ai, "bash") {}
    };

    /// Maintains Lifebloom on the main tank rather than a rotating party member.
    /// NOTE: 1-stack upkeep only -- BuffTrigger::IsActive() fires solely on
    /// total aura absence, so once a stack is up this never re-casts to build
    /// toward Cata's 3-stack bloom. A stack-aware version would need a custom
    /// IsActive() re-deriving BuffTrigger's mana check plus a per-effect stack
    /// scan against the party-tank Unit* (HasAuraStacksTrigger can't be reused
    /// as-is -- it hardcodes ai->GetBot() as the target). Deferred as
    /// over-engineering for this pass; accepted as a known limitation.
    class LifebloomOnTankTrigger : public BuffTrigger
    {
    public:
        LifebloomOnTankTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "lifebloom") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "lifebloom on tank"; }
    };

    /// Feral: bleed-debuff upkeep for Mangle in Cat Form (shares the "mangle"
    /// spell name with the Bear Form version; form determines the effect).
    class MangleTrigger : public DebuffTrigger
    {
    public:
        MangleTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "mangle") {}
    };

    class DemoralizingRoarTrigger : public DebuffTrigger
    {
    public:
        DemoralizingRoarTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "demoralizing roar") {}
    };

    /// ID-based: Clearcasting proc (16870) makes the next Maul free of its
    /// rage cost/gate.
    class ClearcastingTrigger : public HasAuraIdTrigger
    {
    public:
        ClearcastingTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "clearcasting", 16870) {}
    };

    /// ID-based: Predator's Swiftness (69369) -- next Regrowth/Healing Touch
    /// is instant. Runtime-verify: wired to the existing "regrowth" action,
    /// whose ActionNode caster-form prerequisite (when present) may still
    /// force a shapeshift, partly defeating the talent's no-shift benefit.
    class PredatorsSwiftnessTrigger : public HasAuraIdTrigger
    {
    public:
        PredatorsSwiftnessTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "predator's swiftness", 69369) {}
    };
}
