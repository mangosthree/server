#pragma once

#include "../actions/GenericActions.h"

namespace ai
{
    class CastDemonSkinAction : public CastBuffSpellAction {
    public:
        CastDemonSkinAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "demon skin") {}
    };

    class CastDemonArmorAction : public CastBuffSpellAction
    {
    public:
        CastDemonArmorAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "demon armor") {}
    };

    class CastFelArmorAction : public CastBuffSpellAction
    {
    public:
        CastFelArmorAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "fel armor") {}
    };

    BEGIN_RANGED_SPELL_ACTION(CastShadowBoltAction, "shadow bolt")
    END_SPELL_ACTION()

    class CastDrainSoulAction : public CastSpellAction
    {
    public:
        CastDrainSoulAction(PlayerbotAI* ai) : CastSpellAction(ai, "drain soul") {}
        ///< Execute range is gated by the "target critical health" trigger; no shard-count gate here.
    };

    class CastDrainLifeAction : public CastSpellAction
    {
    public:
        CastDrainLifeAction(PlayerbotAI* ai) : CastSpellAction(ai, "drain life") {}
    };

    class CastCurseOfAgonyAction : public CastDebuffSpellAction
    {
    public:
        CastCurseOfAgonyAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "bane of agony") {} ///< Cata: Bane of Agony
    };

    class CastCurseOfWeaknessAction : public CastDebuffSpellAction
    {
    public:
        CastCurseOfWeaknessAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "curse of weakness") {}
    };

    class CastCorruptionAction : public CastDebuffSpellAction
    {
    public:
        CastCorruptionAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "corruption") {}
    };

    class CastCorruptionOnAttackerAction : public CastDebuffSpellOnAttackerAction
    {
    public:
        CastCorruptionOnAttackerAction(PlayerbotAI* ai) : CastDebuffSpellOnAttackerAction(ai, "corruption") {}
    };


    class CastSummonVoidwalkerAction : public CastBuffSpellAction
    {
    public:
        CastSummonVoidwalkerAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon voidwalker") {}
    };

    class CastSummonFelguardAction : public CastBuffSpellAction
    {
    public:
        CastSummonFelguardAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon felguard") {}
    };

    class CastSummonImpAction : public CastBuffSpellAction
    {
    public:
        CastSummonImpAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon imp") {}
    };

    class CastCreateHealthstoneAction : public CastBuffSpellAction
    {
    public:
        CastCreateHealthstoneAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "create healthstone") {}
    };

    class CastBanishAction : public CastBuffSpellAction
    {
    public:
        CastBanishAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "banish on cc") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("cc target", "banish"); }
        virtual bool Execute(Event event) { return ai->CastSpell("banish", GetTarget()); }
    };

    class CastSeedOfCorruptionAction : public CastDebuffSpellAction
    {
    public:
        CastSeedOfCorruptionAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "seed of corruption") {}
    };

    class CastRainOfFireAction : public CastSpellAction
    {
    public:
        CastRainOfFireAction(PlayerbotAI* ai) : CastSpellAction(ai, "rain of fire") {}
        virtual bool isUseful();
    };

    class CastShadowfuryAction : public CastSpellAction
    {
    public:
        CastShadowfuryAction(PlayerbotAI* ai) : CastSpellAction(ai, "shadowfury") {}
    };

    class CastImmolateAction : public CastDebuffSpellAction
    {
    public:
        CastImmolateAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "immolate") {}
    };

    class CastConflagrateAction : public CastSpellAction
    {
    public:
        CastConflagrateAction(PlayerbotAI* ai) : CastSpellAction(ai, "conflagrate") {}
    };

    class CastIncinerateAction : public CastSpellAction
    {
    public:
        CastIncinerateAction(PlayerbotAI* ai) : CastSpellAction(ai, "incinerate") {}
    };

    class CastFearAction : public CastDebuffSpellAction
    {
    public:
        CastFearAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "fear") {}
        virtual bool isUseful()
        {
            return CastDebuffSpellAction::isUseful() &&
                (!ai->HasStrategy("cautious") || !ai->GetGroupTank(bot));
        }
    };

    class CastFearOnCcAction : public CastBuffSpellAction
    {
    public:
        CastFearOnCcAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "fear on cc") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("cc target", "fear"); }
        virtual bool Execute(Event event) { return ai->CastSpell("fear", GetTarget()); }
        virtual bool isUseful()
        {
            return CastBuffSpellAction::isUseful() &&
                (!ai->HasStrategy("cautious") || !ai->GetGroupTank(bot));
        }
    };

    class CastLifeTapAction: public CastSpellAction
    {
    public:
        CastLifeTapAction(PlayerbotAI* ai) : CastSpellAction(ai, "life tap") {}
        virtual string GetTargetName() { return "self target"; }
        virtual bool isUseful() { return AI_VALUE2(uint8, "health", "self target") > sPlayerbotAIConfig.lowHealth; }
    };

    /// Affliction: maintained DoT, runtime-verify exact 4.3.4 spell name.
    class CastUnstableAfflictionAction : public CastDebuffSpellAction
    {
    public:
        CastUnstableAfflictionAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "unstable affliction") {}
    };

    /// Affliction: DoT-amp debuff, recast whenever it falls off (also cooldown-gated).
    class CastHauntAction : public CastDebuffSpellAction
    {
    public:
        CastHauntAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "haunt") {}
    };

    /// Demonology: big DPS cooldown, self buff.
    class CastMetamorphosisAction : public CastBuffSpellAction
    {
    public:
        CastMetamorphosisAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "metamorphosis") {}
    };

    /// Demonology: on-cooldown nuke, runtime-verify apostrophe/spacing.
    class CastHandOfGuldanAction : public CastSpellAction
    {
    public:
        CastHandOfGuldanAction(PlayerbotAI* ai) : CastSpellAction(ai, "hand of gul'dan") {}
    };

    /// Demonology: short self buff only usable in Metamorphosis form, runtime-verify.
    class CastImmolationAuraAction : public CastBuffSpellAction
    {
    public:
        CastImmolationAuraAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "immolation aura") {}
    };

    /// Destruction: on-cooldown nuke.
    class CastChaosBoltAction : public CastSpellAction
    {
    public:
        CastChaosBoltAction(PlayerbotAI* ai) : CastSpellAction(ai, "chaos bolt") {}
    };

    /// Destruction: execute, gated by "target critical health" trigger.
    class CastShadowburnAction : public CastSpellAction
    {
    public:
        CastShadowburnAction(PlayerbotAI* ai) : CastSpellAction(ai, "shadowburn") {}
    };

    /// Demonology: filler/execute nuke, also the Decimation/Improved Soul Fire payoff.
    class CastSoulFireAction : public CastSpellAction
    {
    public:
        CastSoulFireAction(PlayerbotAI* ai) : CastSpellAction(ai, "soul fire") {}
    };

    /// Demonology: maintained curse, low-level-lock fallback for Bane of Agony.
    class CastBaneOfDoomAction : public CastDebuffSpellAction
    {
    public:
        CastBaneOfDoomAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "bane of doom") {}
    };

    /// Raid utility curse, maintained on the current target.
    class CastCurseOfTheElementsAction : public CastDebuffSpellAction
    {
    public:
        CastCurseOfTheElementsAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "curse of the elements") {}
    };

    /// Demonology: self buff cooldown consuming a Demon Soul.
    class CastDemonSoulAction : public CastBuffSpellAction
    {
    public:
        CastDemonSoulAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "demon soul") {}
    };

    /// Affliction pet. Conflagrate (17962) is Destro-only in Cata; gate it out
    /// for Destro bots so the "no pet" chain falls through to Imp.
    class CastSummonFelhunterAction : public CastBuffSpellAction
    {
    public:
        CastSummonFelhunterAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon felhunter") {}
        virtual bool isUseful() { return CastBuffSpellAction::isUseful() && !bot->HasSpell(17962); }
    };

}
