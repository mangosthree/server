#pragma once
#include "../actions/GenericActions.h"

namespace ai
{
    /// Cata 4.3.4: the per-buff judgements were merged into a single Judgement
    class CastJudgementAction : public CastMeleeSpellAction
    {
    public:
        CastJudgementAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "judgement") {}
        /// Judgement requires an active seal (SPELL_FAILED_CASTER_AURASTATE otherwise); mirrors SealTrigger::IsActive
        virtual bool isPossible()
        {
            if (!ai->HasAura("seal of justice", bot) &&
                !ai->HasAura("seal of command", bot) &&
                !ai->HasAura("seal of truth", bot) &&
                !ai->HasAura("seal of righteousness", bot) &&
                !ai->HasAura("seal of insight", bot))
            {
                return false;
            }
            return CastMeleeSpellAction::isPossible();
        }
    };

    class CastRighteousFuryAction : public CastBuffSpellAction
    {
    public:
        CastRighteousFuryAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "righteous fury") {}
    };

    class CastDevotionAuraAction : public CastBuffSpellAction
    {
    public:
        CastDevotionAuraAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "devotion aura") {}
    };

    class CastRetributionAuraAction : public CastBuffSpellAction
    {
    public:
        CastRetributionAuraAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "retribution aura") {}
    };

    class CastConcentrationAuraAction : public CastBuffSpellAction
    {
    public:
        CastConcentrationAuraAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "concentration aura") {}
    };

    class CastDivineStormAction : public CastBuffSpellAction
    {
    public:
        CastDivineStormAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "divine storm") {}
    };

    class CastCrusaderStrikeAction : public CastMeleeSpellAction
    {
    public:
        CastCrusaderStrikeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "crusader strike") {}
    };

    /// Cata 4.3.4: fire/frost/shadow resistance auras were merged into one
    class CastResistanceAuraAction : public CastBuffSpellAction
    {
    public:
        CastResistanceAuraAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "resistance aura") {}
    };

    class CastCrusaderAuraAction : public CastBuffSpellAction
    {
    public:
        CastCrusaderAuraAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "crusader aura") {}
    };

    class CastSealOfRighteousnessAction : public CastBuffSpellAction
    {
    public:
        CastSealOfRighteousnessAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "seal of righteousness") {}
    };

    class CastSealOfJusticeAction : public CastBuffSpellAction
    {
    public:
        CastSealOfJusticeAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "seal of justice") {}
    };


    /// Cata 4.3.4: Seal of Light/Wisdom were merged into Seal of Insight
    class CastSealOfInsightAction : public CastBuffSpellAction
    {
    public:
        CastSealOfInsightAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "seal of insight") {}
    };

    class CastSealOfCommandAction : public CastBuffSpellAction
    {
    public:
        CastSealOfCommandAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "seal of command") {}
    };

    /// Cata 4.3.4: Seal of Vengeance was renamed Seal of Truth
    class CastSealOfTruthAction : public CastBuffSpellAction
    {
    public:
        CastSealOfTruthAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "seal of truth") {}
    };


    class CastBlessingOfMightAction : public CastBuffSpellAction
    {
    public:
        CastBlessingOfMightAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "blessing of might") {}
    };

    class CastBlessingOfMightOnPartyAction : public BuffOnPartyAction
    {
    public:
        CastBlessingOfMightOnPartyAction(PlayerbotAI* ai) : BuffOnPartyAction(ai, "blessing of might") {}
        virtual string getName() { return "blessing of might on party";}
    };

    class CastBlessingOfKingsAction : public CastBuffSpellAction
    {
    public:
        CastBlessingOfKingsAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "blessing of kings") {}
    };

    class CastBlessingOfKingsOnPartyAction : public BuffOnPartyAction
    {
    public:
        CastBlessingOfKingsOnPartyAction(PlayerbotAI* ai) : BuffOnPartyAction(ai, "blessing of kings") {}
        virtual string getName() { return "blessing of kings on party";}
    };

    class CastHolyLightAction : public CastHealingSpellAction
    {
    public:
        CastHolyLightAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "holy light") {}
    };

    class CastHolyLightOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastHolyLightOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "holy light") {}

        virtual string getName() { return "holy light on party"; }
    };

    class CastFlashOfLightAction : public CastHealingSpellAction
    {
    public:
        CastFlashOfLightAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "flash of light") {}
    };

    class CastFlashOfLightOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastFlashOfLightOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "flash of light") {}

        virtual string getName() { return "flash of light on party"; }
    };

    class CastHolyShockAction : public CastHealingSpellAction
    {
    public:
        CastHolyShockAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "holy shock") {}
    };

    class CastHolyShockOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastHolyShockOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "holy shock") {}

        virtual string getName() { return "holy shock on party"; }
    };

    class CastDivineLightAction : public CastHealingSpellAction
    {
    public:
        CastDivineLightAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "divine light") {}
    };

    class CastDivineLightOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastDivineLightOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "divine light") {}

        virtual string getName() { return "divine light on party"; }
    };

    /// Maintains Beacon of Light on the main tank rather than a rotating party member
    class CastBeaconOfLightOnTankAction : public CastBuffSpellAction
    {
    public:
        CastBeaconOfLightOnTankAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "beacon of light") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "beacon of light on tank"; }
    };

    class CastLayOnHandsAction : public CastHealingSpellAction
    {
    public:
        CastLayOnHandsAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "lay on hands") {}
    };

    class CastLayOnHandsOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastLayOnHandsOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "lay on hands") {}

        virtual string getName() { return "lay on hands on party"; }
    };

    class CastDivineProtectionAction : public CastBuffSpellAction
    {
    public:
        CastDivineProtectionAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "divine protection") {}
    };

    class CastDivineProtectionOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastDivineProtectionOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "divine protection") {}

        virtual string getName() { return "divine protection on party"; }
    };

    class CastDivineShieldAction: public CastBuffSpellAction
    {
    public:
        CastDivineShieldAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "divine shield") {}
    };

    class CastConsecrationAction : public CastMeleeSpellAction
    {
    public:
        CastConsecrationAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "consecration") {}
    };

    class CastHolyWrathAction : public CastMeleeSpellAction
    {
    public:
        CastHolyWrathAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "holy wrath") {}
    };

    class CastHammerOfJusticeAction : public CastMeleeSpellAction
    {
    public:
        CastHammerOfJusticeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "hammer of justice") {}
    };

    class CastHammerOfWrathAction : public CastMeleeSpellAction
    {
    public:
        CastHammerOfWrathAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "hammer of wrath") {}
    };

    class CastHammerOfTheRighteousAction : public CastMeleeSpellAction
    {
    public:
        CastHammerOfTheRighteousAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "hammer of the righteous") {}
    };

    class CastHandOfReckoningAction : public CastSpellAction
    {
    public:
        CastHandOfReckoningAction(PlayerbotAI* ai) : CastSpellAction(ai, "hand of reckoning") {}
    };

    class CastCleansePoisonAction : public CastCureSpellAction
    {
    public:
        CastCleansePoisonAction(PlayerbotAI* ai) : CastCureSpellAction(ai, "cleanse") {}
    };

    class CastCleanseDiseaseAction : public CastCureSpellAction
    {
    public:
        CastCleanseDiseaseAction(PlayerbotAI* ai) : CastCureSpellAction(ai, "cleanse") {}
    };

    class CastCleanseMagicAction : public CastCureSpellAction
    {
    public:
        CastCleanseMagicAction(PlayerbotAI* ai) : CastCureSpellAction(ai, "cleanse") {}
    };

    class CastCleansePoisonOnPartyAction : public CurePartyMemberAction
    {
    public:
        CastCleansePoisonOnPartyAction(PlayerbotAI* ai) : CurePartyMemberAction(ai, "cleanse", DISPEL_POISON) {}

        virtual string getName() { return "cleanse poison on party"; }
    };

    class CastCleanseDiseaseOnPartyAction : public CurePartyMemberAction
    {
    public:
        CastCleanseDiseaseOnPartyAction(PlayerbotAI* ai) : CurePartyMemberAction(ai, "cleanse", DISPEL_DISEASE) {}

        virtual string getName() { return "cleanse disease on party"; }
    };

    class CastCleanseMagicOnPartyAction : public CurePartyMemberAction
    {
    public:
        CastCleanseMagicOnPartyAction(PlayerbotAI* ai) : CurePartyMemberAction(ai, "cleanse", DISPEL_MAGIC) {}

        virtual string getName() { return "cleanse magic on party"; }
    };

    BEGIN_SPELL_ACTION(CastAvengersShieldAction, "avenger's shield")
    END_SPELL_ACTION()

    BEGIN_SPELL_ACTION(CastExorcismAction, "exorcism")
    END_SPELL_ACTION()

    class CastHolyShieldAction : public CastBuffSpellAction
    {
    public:
        CastHolyShieldAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "holy shield") {}
    };

    class CastRedemptionAction : public ResurrectPartyMemberAction
    {
    public:
        CastRedemptionAction(PlayerbotAI* ai) : ResurrectPartyMemberAction(ai, "redemption") {}
    };

    class CastHammerOfJusticeOnEnemyHealerAction : public CastSpellOnEnemyHealerAction
    {
    public:
        CastHammerOfJusticeOnEnemyHealerAction(PlayerbotAI* ai) : CastSpellOnEnemyHealerAction(ai, "hammer of justice") {}
    };

    class CastHammerOfJusticeSnareAction : public CastSnareSpellAction
    {
    public:
        CastHammerOfJusticeSnareAction(PlayerbotAI* ai) : CastSnareSpellAction(ai, "hammer of justice") {}
    };

    class CastBlessingOfFreedomAction : public CastBuffSpellAction
    {
    public:
        CastBlessingOfFreedomAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "blessing of freedom") {}
    };

    /// Ret finisher; requires 3 Holy Power. Mirrors CastJudgementAction's isPossible gate style
    class CastTemplarsVerdictAction : public CastMeleeSpellAction
    {
    public:
        CastTemplarsVerdictAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "templar's verdict") {}
        virtual bool isPossible()
        {
            if (AI_VALUE2(uint8, "holy power", "self target") < 3)
            {
                return false;
            }
            return CastMeleeSpellAction::isPossible();
        }
    };

    /// Spends 3 Holy Power to refresh the crit buff; self-gates on the aura via
    /// CastAuraSpellAction::isUseful so it only fires while the buff is down
    class CastInquisitionAction : public CastBuffSpellAction
    {
    public:
        CastInquisitionAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "inquisition") {}
        virtual bool isPossible()
        {
            if (AI_VALUE2(uint8, "holy power", "self target") < 3)
            {
                return false;
            }
            return CastBuffSpellAction::isPossible();
        }
    };

    class CastAvengingWrathAction : public CastBuffSpellAction
    {
    public:
        CastAvengingWrathAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "avenging wrath") {}
    };

    class CastRebukeAction : public CastSpellAction
    {
    public:
        CastRebukeAction(PlayerbotAI* ai) : CastSpellAction(ai, "rebuke") {}
    };

    /// Prot Holy Power spender: threat + mitigation
    class CastShieldOfTheRighteousAction : public CastMeleeSpellAction
    {
    public:
        CastShieldOfTheRighteousAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "shield of the righteous") {}
        virtual bool isPossible()
        {
            if (AI_VALUE2(uint8, "holy power", "self target") < 3)
            {
                return false;
            }
            return CastMeleeSpellAction::isPossible();
        }
    };

    /// Prot Holy Power self-heal; competes with Shield of the Righteous via relevance
    class CastWordOfGloryAction : public CastHealingSpellAction
    {
    public:
        CastWordOfGloryAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "word of glory") {}
        virtual bool isPossible()
        {
            if (AI_VALUE2(uint8, "holy power", "self target") < 3)
            {
                return false;
            }
            return CastHealingSpellAction::isPossible();
        }
    };

    class CastGuardianOfAncientKingsAction : public CastBuffSpellAction
    {
    public:
        CastGuardianOfAncientKingsAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "guardian of ancient kings") {}
    };

    class CastArdentDefenderAction : public CastBuffSpellAction
    {
    public:
        CastArdentDefenderAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "ardent defender") {}
    };
}
