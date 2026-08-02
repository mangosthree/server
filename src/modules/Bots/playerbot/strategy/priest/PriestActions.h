#pragma once

#include "../actions/GenericActions.h"

namespace ai
{
    class CastGreaterHealAction : public CastHealingSpellAction {
    public:
        CastGreaterHealAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "greater heal") {}
    };

    class CastGreaterHealOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastGreaterHealOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "greater heal") {}

        virtual string getName() { return "greater heal on party"; }
    };


    class CastFlashHealAction : public CastHealingSpellAction {
    public:
        CastFlashHealAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "flash heal") {}
    };

    class CastFlashHealOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastFlashHealOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "flash heal") {}

        virtual string getName() { return "flash heal on party"; }
    };

    class CastHealAction : public CastHealingSpellAction {
    public:
        CastHealAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "heal") {}
    };

    class CastHealOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastHealOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "heal") {}

        virtual string getName() { return "heal on party"; }
    };

    class CastRenewAction : public CastHealingSpellAction {
    public:
        CastRenewAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "renew") {}
    };

    class CastRenewOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastRenewOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "renew") {}

        virtual string getName() { return "renew on party"; }
    };

    class CastFadeAction : public CastBuffSpellAction {
    public:
        CastFadeAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "fade") {}
    };

    class CastShadowformAction : public CastBuffSpellAction {
    public:
        CastShadowformAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "shadowform") {}
    };

    class CastRemoveShadowformAction : public Action {
    public:
        CastRemoveShadowformAction(PlayerbotAI* ai) : Action(ai, "remove shadowform") {}
        virtual bool isUseful() { return ai->HasAura("shadowform", AI_VALUE(Unit*, "self target")); }
        virtual bool isPossible() { return true; }
        virtual bool Execute(Event event) {
            ai->RemoveAura("shadowform");
            return true;
        }
    };

    class CastVampiricEmbraceAction : public CastBuffSpellAction {
    public:
        CastVampiricEmbraceAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "vampiric embrace") {}
    };

    class CastPowerWordShieldAction : public CastBuffSpellAction {
    public:
        CastPowerWordShieldAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "power word: shield") {}
        /// PW:S applies Weakened Soul (blocks re-shield 15s); don't waste it
        virtual bool isUseful() { return !ai->HasAura("weakened soul", GetTarget()) && CastBuffSpellAction::isUseful(); }
    };

    class CastPowerWordShieldOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastPowerWordShieldOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "power word: shield") {}
        /// PW:S applies Weakened Soul (blocks re-shield 15s); don't waste it
        virtual bool isUseful() { return !ai->HasAura("weakened soul", GetTarget()) && HealPartyMemberAction::isUseful(); }

        virtual string getName() { return "power word: shield on party"; }
    };

    class CastPowerWordFortitudeAction : public CastBuffSpellAction {
    public:
        CastPowerWordFortitudeAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "power word: fortitude") {}
    };


    class CastInnerFireAction : public CastBuffSpellAction {
    public:
        CastInnerFireAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "inner fire") {}
    };

    BEGIN_SPELL_ACTION(CastHolyNovaAction, "holy nova")
    virtual bool isUseful() {
        return !ai->HasAura("shadowform", AI_VALUE(Unit*, "self target"));
    }
    END_SPELL_ACTION()

    BEGIN_RANGED_SPELL_ACTION(CastHolyFireAction, "holy fire")
        virtual bool isUseful() {
            return !ai->HasAura("shadowform", AI_VALUE(Unit*, "self target"));
        }
    END_SPELL_ACTION()

    BEGIN_RANGED_SPELL_ACTION(CastSmiteAction, "smite")
        virtual bool isUseful() {
            return !ai->HasAura("shadowform", AI_VALUE(Unit*, "self target"));
        }
    END_SPELL_ACTION()

    class CastPowerWordFortitudeOnPartyAction : public BuffOnPartyAction {
    public:
        CastPowerWordFortitudeOnPartyAction(PlayerbotAI* ai) : BuffOnPartyAction(ai, "power word: fortitude") {}
    };


    class CastPowerWordPainAction : public CastDebuffSpellAction
    {
    public:
        CastPowerWordPainAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "shadow word: pain") {}
    };

    class CastPowerWordPainOnAttackerAction : public CastDebuffSpellOnAttackerAction
    {
    public:
        CastPowerWordPainOnAttackerAction(PlayerbotAI* ai) : CastDebuffSpellOnAttackerAction(ai, "shadow word: pain") {}
    };

    BEGIN_DEBUFF_ACTION(CastDevouringPlagueAction, "devouring plague")
    END_SPELL_ACTION()

    BEGIN_DEBUFF_ACTION(CastVampiricTouchAction, "vampiric touch")
    END_SPELL_ACTION()

    BEGIN_RANGED_SPELL_ACTION(CastMindBlastAction, "mind blast")
    END_SPELL_ACTION()

    BEGIN_RANGED_SPELL_ACTION(CastMindFlayAction, "mind flay")
    END_SPELL_ACTION()

    class CastCureDiseaseAction : public CastCureSpellAction {
    public:
        CastCureDiseaseAction(PlayerbotAI* ai) : CastCureSpellAction(ai, "cure disease") {}
    };

    class CastCureDiseaseOnPartyAction : public CurePartyMemberAction
    {
    public:
        CastCureDiseaseOnPartyAction(PlayerbotAI* ai) : CurePartyMemberAction(ai, "cure disease", DISPEL_DISEASE) {}
        virtual string getName() { return "cure disease on party"; }
    };


    class CastDispelMagicAction : public CastCureSpellAction {
    public:
        CastDispelMagicAction(PlayerbotAI* ai) : CastCureSpellAction(ai, "dispel magic") {}
    };

    class CastDispelMagicOnTargetAction : public CastSpellAction {
    public:
        CastDispelMagicOnTargetAction(PlayerbotAI* ai) : CastSpellAction(ai, "dispel magic") {}
    };

    class CastDispelMagicOnPartyAction : public CurePartyMemberAction
    {
    public:
        CastDispelMagicOnPartyAction(PlayerbotAI* ai) : CurePartyMemberAction(ai, "dispel magic", DISPEL_MAGIC) {}
        virtual string getName() { return "dispel magic on party"; }
    };

    class CastResurrectionAction : public ResurrectPartyMemberAction
    {
    public:
        CastResurrectionAction(PlayerbotAI* ai) : ResurrectPartyMemberAction(ai, "resurrection") {}
    };

    class CastCircleOfHealingAction : public CastAoeHealSpellAction
    {
    public:
        CastCircleOfHealingAction(PlayerbotAI* ai) : CastAoeHealSpellAction(ai, "circle of healing") {}
    };

    class CastPsychicScreamAction : public CastSpellAction
    {
    public:
        CastPsychicScreamAction(PlayerbotAI* ai) : CastSpellAction(ai, "psychic scream") {}
    };

    class CastDispersionAction : public CastSpellAction
    {
    public:
        CastDispersionAction(PlayerbotAI* ai) : CastSpellAction(ai, "dispersion") {}
        virtual string GetTargetName() { return "self target"; }
    };

    /// Execute; gated by the "target critical health" trigger
    BEGIN_RANGED_SPELL_ACTION(CastShadowWordDeathAction, "shadow word: death")
    END_SPELL_ACTION()

    /// AoE channel; gated by an attacker-count trigger like other AoE nukes
    BEGIN_RANGED_SPELL_ACTION(CastMindSearAction, "mind sear")
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_AOE; }
    END_SPELL_ACTION()

    /// Mana cooldown pet; gated by "low mana"
    BEGIN_RANGED_SPELL_ACTION(CastShadowfiendAction, "shadowfiend")
    END_SPELL_ACTION()

    class CastPenanceAction : public CastHealingSpellAction {
    public:
        CastPenanceAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "penance") {}
    };

    class CastPenanceOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastPenanceOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "penance") {}

        virtual string getName() { return "penance on party"; }
    };

    /// Maintains Prayer of Mending on the tank instead of a rotating party member
    class CastPrayerOfMendingOnTankAction : public CastBuffSpellAction
    {
    public:
        CastPrayerOfMendingOnTankAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "prayer of mending") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "prayer of mending on tank"; }
    };

    /// Discipline external defensive; cast on the tank, not self/current target
    class CastPainSuppressionOnTankAction : public CastBuffSpellAction
    {
    public:
        CastPainSuppressionOnTankAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "pain suppression") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "pain suppression on tank"; }
    };

    class CastPrayerOfHealingAction : public CastAoeHealSpellAction
    {
    public:
        CastPrayerOfHealingAction(PlayerbotAI* ai) : CastAoeHealSpellAction(ai, "prayer of healing") {}
    };

    /// Holy external emergency cooldown; cast on the tank
    class CastGuardianSpiritOnTankAction : public CastBuffSpellAction
    {
    public:
        CastGuardianSpiritOnTankAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "guardian spirit") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "guardian spirit on tank"; }
    };

    /// Discipline: keep Power Word: Shield up on the tank on cooldown
    class CastPowerWordShieldOnTankAction : public CastBuffSpellAction
    {
    public:
        CastPowerWordShieldOnTankAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "power word: shield") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "power word: shield on tank"; }
    };

    /// Holy: Chakra: Serenity stance for single-target healing (state aura, cast id 81208)
    class CastChakraSerenityAction : public CastBuffSpellAction
    {
    public:
        CastChakraSerenityAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "chakra: serenity") {}
    };

}
