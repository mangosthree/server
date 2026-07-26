#include "botpch.h"
#include "../../playerbot.h"
#include "PaladinActions.h"
#include "PaladinTriggers.h"
#include "PaladinAiObjectContext.h"
#include "GenericPaladinNonCombatStrategy.h"
#include "TankPaladinStrategy.h"
#include "DpsPaladinStrategy.h"
#include "PaladinBuffStrategies.h"
#include "../NamedObjectContext.h"
#include "HealPaladinStrategy.h"

using namespace ai;

namespace ai
{
    namespace paladin
    {
        using namespace ai;

        class StrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            StrategyFactoryInternal()
            {
                creators["nc"] = &paladin::StrategyFactoryInternal::nc;
                creators["cure"] = &paladin::StrategyFactoryInternal::cure;
            }

        private:
            static Strategy* nc(PlayerbotAI* ai) { return new GenericPaladinNonCombatStrategy(ai); }
            static Strategy* cure(PlayerbotAI* ai) { return new PaladinCureStrategy(ai); }
        };

        class ResistanceStrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            ResistanceStrategyFactoryInternal() : NamedObjectContext<Strategy>(false, true)
            {
                creators["rshadow"] = &paladin::ResistanceStrategyFactoryInternal::rshadow;
                creators["rfrost"] = &paladin::ResistanceStrategyFactoryInternal::rfrost;
                creators["rfire"] = &paladin::ResistanceStrategyFactoryInternal::rfire;
            }

        private:
            static Strategy* rshadow(PlayerbotAI* ai) { return new PaladinShadowResistanceStrategy(ai); }
            static Strategy* rfrost(PlayerbotAI* ai) { return new PaladinFrostResistanceStrategy(ai); }
            static Strategy* rfire(PlayerbotAI* ai) { return new PaladinFireResistanceStrategy(ai); }
        };

        class BuffStrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            BuffStrategyFactoryInternal() : NamedObjectContext<Strategy>(false, true)
            {
                creators["bhealth"] = &paladin::BuffStrategyFactoryInternal::bhealth;
                creators["bmana"] = &paladin::BuffStrategyFactoryInternal::bmana;
                creators["bdps"] = &paladin::BuffStrategyFactoryInternal::bdps;
                creators["barmor"] = &paladin::BuffStrategyFactoryInternal::barmor;
                creators["bspeed"] = &paladin::BuffStrategyFactoryInternal::bspeed;
                creators["bthreat"] = &paladin::BuffStrategyFactoryInternal::bthreat;
            }

        private:
            static Strategy* bhealth(PlayerbotAI* ai) { return new PaladinBuffHealthStrategy(ai); }
            static Strategy* bmana(PlayerbotAI* ai) { return new PaladinBuffManaStrategy(ai); }
            static Strategy* bdps(PlayerbotAI* ai) { return new PaladinBuffDpsStrategy(ai); }
            static Strategy* barmor(PlayerbotAI* ai) { return new PaladinBuffArmorStrategy(ai); }
            static Strategy* bspeed(PlayerbotAI* ai) { return new PaladinBuffSpeedStrategy(ai); }
            static Strategy* bthreat(PlayerbotAI* ai) { return new PaladinBuffThreatStrategy(ai); }
        };

        class CombatStrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            CombatStrategyFactoryInternal() : NamedObjectContext<Strategy>(false, true)
            {
                creators["tank"] = &paladin::CombatStrategyFactoryInternal::tank;
                creators["dps"] = &paladin::CombatStrategyFactoryInternal::dps;
                creators["heal"] = &paladin::CombatStrategyFactoryInternal::heal;
            }

        private:
            static Strategy* tank(PlayerbotAI* ai) { return new TankPaladinStrategy(ai); }
            static Strategy* dps(PlayerbotAI* ai) { return new DpsPaladinStrategy(ai); }
            static Strategy* heal(PlayerbotAI* ai) { return new HealPaladinStrategy(ai); }
        };
    };
};

namespace ai
{
    namespace paladin
    {
        using namespace ai;

        class TriggerFactoryInternal : public NamedObjectContext<Trigger>
        {
        public:
            TriggerFactoryInternal()
            {
                creators["judgement"] = &TriggerFactoryInternal::judgement;
                creators["blessing"] = &TriggerFactoryInternal::blessing;
                creators["seal"] = &TriggerFactoryInternal::seal;
                creators["art of war"] = &TriggerFactoryInternal::art_of_war;
                creators["blessing on party"] = &TriggerFactoryInternal::blessing_on_party;
                creators["crusader aura"] = &TriggerFactoryInternal::crusader_aura;
                creators["retribution aura"] = &TriggerFactoryInternal::retribution_aura;
                creators["devotion aura"] = &TriggerFactoryInternal::devotion_aura;
                creators["resistance aura"] = &TriggerFactoryInternal::resistance_aura;
                creators["hammer of justice snare"] = &TriggerFactoryInternal::hammer_of_justice_snare;
                creators["hammer of justice interrupt"] = &TriggerFactoryInternal::hammer_of_justice_interrupt;
                creators["cleanse cure disease"] = &TriggerFactoryInternal::CleanseCureDisease;
                creators["cleanse party member cure disease"] = &TriggerFactoryInternal::CleanseCurePartyMemberDisease;
                creators["cleanse cure poison"] = &TriggerFactoryInternal::CleanseCurePoison;
                creators["cleanse party member cure poison"] = &TriggerFactoryInternal::CleanseCurePartyMemberPoison;
                creators["cleanse cure magic"] = &TriggerFactoryInternal::CleanseCureMagic;
                creators["cleanse party member cure magic"] = &TriggerFactoryInternal::CleanseCurePartyMemberMagic;
                creators["righteous fury"] = &TriggerFactoryInternal::righteous_fury;
                creators["holy shield"] = &TriggerFactoryInternal::holy_shield;
                creators["hammer of justice on enemy healer"] = &TriggerFactoryInternal::hammer_of_justice_on_enemy_target;
                creators["hammer of justice on snare target"] = &TriggerFactoryInternal::hammer_of_justice_on_snare_target;
                creators["holy wrath"] = &TriggerFactoryInternal::holy_wrath;
                creators["blessing of freedom"] = &TriggerFactoryInternal::blessing_of_freedom;
                creators["holy power available"] = &TriggerFactoryInternal::holy_power_available;
                creators["beacon of light on tank"] = &TriggerFactoryInternal::beacon_of_light_on_tank;
                creators["avenging wrath"] = &TriggerFactoryInternal::avenging_wrath;
                creators["rebuke interrupt"] = &TriggerFactoryInternal::rebuke_interrupt;
            }

        private:
            static Trigger* holy_wrath(PlayerbotAI* ai) { return new HolyWrathTrigger(ai); }
            static Trigger* blessing_of_freedom(PlayerbotAI* ai) { return new BlessingOfFreedomTrigger(ai); }
            static Trigger* holy_power_available(PlayerbotAI* ai) { return new HolyPowerAvailableTrigger(ai); }
            static Trigger* beacon_of_light_on_tank(PlayerbotAI* ai) { return new BeaconOfLightOnTankTrigger(ai); }
            static Trigger* avenging_wrath(PlayerbotAI* ai) { return new AvengingWrathTrigger(ai); }
            static Trigger* rebuke_interrupt(PlayerbotAI* ai) { return new RebukeInterruptSpellTrigger(ai); }
            static Trigger* holy_shield(PlayerbotAI* ai) { return new HolyShieldTrigger(ai); }
            static Trigger* righteous_fury(PlayerbotAI* ai) { return new RighteousFuryTrigger(ai); }
            static Trigger* judgement(PlayerbotAI* ai) { return new JudgementTrigger(ai); }
            static Trigger* blessing(PlayerbotAI* ai) { return new BlessingTrigger(ai); }
            static Trigger* seal(PlayerbotAI* ai) { return new SealTrigger(ai); }
            static Trigger* art_of_war(PlayerbotAI* ai) { return new ArtOfWarTrigger(ai); }
            static Trigger* blessing_on_party(PlayerbotAI* ai) { return new BlessingOnPartyTrigger(ai); }
            static Trigger* crusader_aura(PlayerbotAI* ai) { return new CrusaderAuraTrigger(ai); }
            static Trigger* retribution_aura(PlayerbotAI* ai) { return new RetributionAuraTrigger(ai); }
            static Trigger* devotion_aura(PlayerbotAI* ai) { return new DevotionAuraTrigger(ai); }
            static Trigger* resistance_aura(PlayerbotAI* ai) { return new ResistanceAuraTrigger(ai); }
            static Trigger* hammer_of_justice_snare(PlayerbotAI* ai) { return new HammerOfJusticeSnareTrigger(ai); }
            static Trigger* hammer_of_justice_interrupt(PlayerbotAI* ai) { return new HammerOfJusticeInterruptSpellTrigger(ai); }
            static Trigger* CleanseCureDisease(PlayerbotAI* ai) { return new CleanseCureDiseaseTrigger(ai); }
            static Trigger* CleanseCurePartyMemberDisease(PlayerbotAI* ai) { return new CleanseCurePartyMemberDiseaseTrigger(ai); }
            static Trigger* CleanseCurePoison(PlayerbotAI* ai) { return new CleanseCurePoisonTrigger(ai); }
            static Trigger* CleanseCurePartyMemberPoison(PlayerbotAI* ai) { return new CleanseCurePartyMemberPoisonTrigger(ai); }
            static Trigger* CleanseCureMagic(PlayerbotAI* ai) { return new CleanseCureMagicTrigger(ai); }
            static Trigger* CleanseCurePartyMemberMagic(PlayerbotAI* ai) { return new CleanseCurePartyMemberMagicTrigger(ai); }
            static Trigger* hammer_of_justice_on_enemy_target(PlayerbotAI* ai) { return new HammerOfJusticeEnemyHealerTrigger(ai); }
            static Trigger* hammer_of_justice_on_snare_target(PlayerbotAI* ai) { return new HammerOfJusticeSnareTrigger(ai); }
        };
    };
};

namespace ai
{
    namespace paladin
    {
        using namespace ai;

        class AiObjectContextInternal : public NamedObjectContext<Action>
        {
        public:
            AiObjectContextInternal()
            {
                creators["seal of command"] = &AiObjectContextInternal::seal_of_command;
                creators["seal of truth"] = &AiObjectContextInternal::seal_of_truth;
                creators["blessing of might"] = &AiObjectContextInternal::blessing_of_might;
                creators["blessing of kings"] = &AiObjectContextInternal::blessing_of_kings;
                creators["divine storm"] = &AiObjectContextInternal::divine_storm;
                creators["blessing of kings on party"] = &AiObjectContextInternal::blessing_of_kings_on_party;
                creators["blessing of might on party"] = &AiObjectContextInternal::blessing_of_might_on_party;
                creators["redemption"] = &AiObjectContextInternal::redemption;
                creators["crusader strike"] = &AiObjectContextInternal::crusader_strike;
                creators["crusader aura"] = &AiObjectContextInternal::crusader_aura;
                creators["seal of insight"] = &AiObjectContextInternal::seal_of_insight;
                creators["devotion aura"] = &AiObjectContextInternal::devotion_aura;
                creators["holy wrath"] = &AiObjectContextInternal::holy_wrath;
                creators["consecration"] = &AiObjectContextInternal::consecration;
                creators["cleanse disease"] = &AiObjectContextInternal::cleanse_disease;
                creators["cleanse poison"] = &AiObjectContextInternal::cleanse_poison;
                creators["cleanse magic"] = &AiObjectContextInternal::cleanse_magic;
                creators["cleanse poison on party"] = &AiObjectContextInternal::cleanse_poison_on_party;
                creators["cleanse disease on party"] = &AiObjectContextInternal::cleanse_disease_on_party;
                creators["cleanse magic on party"] = &AiObjectContextInternal::cleanse_magic_on_party;
                creators["seal of justice"] = &AiObjectContextInternal::seal_of_justice;
                creators["seal of righteousness"] = &AiObjectContextInternal::seal_of_righteousness;
                creators["flash of light"] = &AiObjectContextInternal::flash_of_light;
                creators["hand of reckoning"] = &AiObjectContextInternal::hand_of_reckoning;
                creators["avenger's shield"] = &AiObjectContextInternal::avengers_shield;
                creators["exorcism"] = &AiObjectContextInternal::exorcism;
                creators["judgement"] = &AiObjectContextInternal::judgement;
                creators["divine shield"] = &AiObjectContextInternal::divine_shield;
                creators["divine protection"] = &AiObjectContextInternal::divine_protection;
                creators["divine protection on party"] =&AiObjectContextInternal::divine_protection_on_party;
                creators["hammer of justice"] = &AiObjectContextInternal::hammer_of_justice;
                creators["flash of light on party"] = &AiObjectContextInternal::flash_of_light_on_party;
                creators["holy light"] = &AiObjectContextInternal::holy_light;
                creators["holy light on party"] = &AiObjectContextInternal::holy_light_on_party;
                creators["lay on hands"] = &AiObjectContextInternal::lay_on_hands;
                creators["lay on hands on party"] = &AiObjectContextInternal::lay_on_hands_on_party;
                creators["hammer of wrath"] = &AiObjectContextInternal::hammer_of_wrath;
                creators["holy shield"] = &AiObjectContextInternal::holy_shield;
                creators["hammer of the righteous"] = &AiObjectContextInternal::hammer_of_the_righteous;
                creators["retribution aura"] = &AiObjectContextInternal::retribution_aura;
                creators["resistance aura"] = &AiObjectContextInternal::resistance_aura;
                creators["righteous fury"] = &AiObjectContextInternal::righteous_fury;
                creators["hammer of justice on enemy healer"] = &AiObjectContextInternal::hammer_of_justice_on_enemy_healer;
                creators["hammer of justice on snare target"] = &AiObjectContextInternal::hammer_of_justice_on_snare_target;
                creators["blessing of freedom"] = &AiObjectContextInternal::blessing_of_freedom;
                creators["holy shock"] = &AiObjectContextInternal::holy_shock;
                creators["holy shock on party"] = &AiObjectContextInternal::holy_shock_on_party;
                creators["divine light"] = &AiObjectContextInternal::divine_light;
                creators["divine light on party"] = &AiObjectContextInternal::divine_light_on_party;
                creators["beacon of light on tank"] = &AiObjectContextInternal::beacon_of_light_on_tank;
                creators["templar's verdict"] = &AiObjectContextInternal::templars_verdict;
                creators["inquisition"] = &AiObjectContextInternal::inquisition;
                creators["avenging wrath"] = &AiObjectContextInternal::avenging_wrath;
                creators["rebuke"] = &AiObjectContextInternal::rebuke;
                creators["shield of the righteous"] = &AiObjectContextInternal::shield_of_the_righteous;
                creators["word of glory"] = &AiObjectContextInternal::word_of_glory;
                creators["guardian of ancient kings"] = &AiObjectContextInternal::guardian_of_ancient_kings;
                creators["ardent defender"] = &AiObjectContextInternal::ardent_defender;
            }

        private:
            static Action* righteous_fury(PlayerbotAI* ai) { return new CastRighteousFuryAction(ai); }
            static Action* seal_of_command(PlayerbotAI* ai) { return new CastSealOfCommandAction(ai); }
            static Action* seal_of_truth(PlayerbotAI* ai) { return new CastSealOfTruthAction(ai); }
            static Action* blessing_of_might(PlayerbotAI* ai) { return new CastBlessingOfMightAction(ai); }
            static Action* blessing_of_kings(PlayerbotAI* ai) { return new CastBlessingOfKingsAction(ai); }
            static Action* divine_storm(PlayerbotAI* ai) { return new CastDivineStormAction(ai); }
            static Action* blessing_of_kings_on_party(PlayerbotAI* ai) { return new CastBlessingOfKingsOnPartyAction(ai); }
            static Action* blessing_of_might_on_party(PlayerbotAI* ai) { return new CastBlessingOfMightOnPartyAction(ai); }
            static Action* redemption(PlayerbotAI* ai) { return new CastRedemptionAction(ai); }
            static Action* crusader_strike(PlayerbotAI* ai) { return new CastCrusaderStrikeAction(ai); }
            static Action* crusader_aura(PlayerbotAI* ai) { return new CastCrusaderAuraAction(ai); }
            static Action* seal_of_insight(PlayerbotAI* ai) { return new CastSealOfInsightAction(ai); }
            static Action* devotion_aura(PlayerbotAI* ai) { return new CastDevotionAuraAction(ai); }
            static Action* holy_wrath(PlayerbotAI* ai) { return new CastHolyWrathAction(ai); }
            static Action* consecration(PlayerbotAI* ai) { return new CastConsecrationAction(ai); }
            static Action* cleanse_poison(PlayerbotAI* ai) { return new CastCleansePoisonAction(ai); }
            static Action* cleanse_disease(PlayerbotAI* ai) { return new CastCleanseDiseaseAction(ai); }
            static Action* cleanse_magic(PlayerbotAI* ai) { return new CastCleanseMagicAction(ai); }
            static Action* cleanse_poison_on_party(PlayerbotAI* ai) { return new CastCleansePoisonOnPartyAction(ai); }
            static Action* cleanse_disease_on_party(PlayerbotAI* ai) { return new CastCleanseDiseaseOnPartyAction(ai); }
            static Action* cleanse_magic_on_party(PlayerbotAI* ai) { return new CastCleanseMagicOnPartyAction(ai); }
            static Action* seal_of_justice(PlayerbotAI* ai) { return new CastSealOfJusticeAction(ai); }
            static Action* seal_of_righteousness(PlayerbotAI* ai) { return new CastSealOfRighteousnessAction(ai); }
            static Action* flash_of_light(PlayerbotAI* ai) { return new CastFlashOfLightAction(ai); }
            static Action* hand_of_reckoning(PlayerbotAI* ai) { return new CastHandOfReckoningAction(ai); }
            static Action* avengers_shield(PlayerbotAI* ai) { return new CastAvengersShieldAction(ai); }
            static Action* exorcism(PlayerbotAI* ai) { return new CastExorcismAction(ai); }
            static Action* judgement(PlayerbotAI* ai) { return new CastJudgementAction(ai); }
            static Action* divine_shield(PlayerbotAI* ai) { return new CastDivineShieldAction(ai); }
            static Action* divine_protection(PlayerbotAI* ai) { return new CastDivineProtectionAction(ai); }
            static Action* divine_protection_on_party(PlayerbotAI* ai) { return new CastDivineProtectionOnPartyAction(ai); }
            static Action* hammer_of_justice(PlayerbotAI* ai) { return new CastHammerOfJusticeAction(ai); }
            static Action* flash_of_light_on_party(PlayerbotAI* ai) { return new CastFlashOfLightOnPartyAction(ai); }
            static Action* holy_light(PlayerbotAI* ai) { return new CastHolyLightAction(ai); }
            static Action* holy_light_on_party(PlayerbotAI* ai) { return new CastHolyLightOnPartyAction(ai); }
            static Action* lay_on_hands(PlayerbotAI* ai) { return new CastLayOnHandsAction(ai); }
            static Action* lay_on_hands_on_party(PlayerbotAI* ai) { return new CastLayOnHandsOnPartyAction(ai); }
            static Action* hammer_of_wrath(PlayerbotAI* ai) { return new CastHammerOfWrathAction(ai); }
            static Action* holy_shield(PlayerbotAI* ai) { return new CastHolyShieldAction(ai); }
            static Action* hammer_of_the_righteous(PlayerbotAI* ai) { return new CastHammerOfTheRighteousAction(ai); }
            static Action* retribution_aura(PlayerbotAI* ai) { return new CastRetributionAuraAction(ai); }
            static Action* resistance_aura(PlayerbotAI* ai) { return new CastResistanceAuraAction(ai); }
            static Action* hammer_of_justice_on_enemy_healer(PlayerbotAI* ai) { return new CastHammerOfJusticeOnEnemyHealerAction(ai); }
            static Action* hammer_of_justice_on_snare_target(PlayerbotAI* ai) { return new CastHammerOfJusticeSnareAction(ai); }
            static Action* blessing_of_freedom(PlayerbotAI* ai) { return new CastBlessingOfFreedomAction(ai); }
            static Action* holy_shock(PlayerbotAI* ai) { return new CastHolyShockAction(ai); }
            static Action* holy_shock_on_party(PlayerbotAI* ai) { return new CastHolyShockOnPartyAction(ai); }
            static Action* divine_light(PlayerbotAI* ai) { return new CastDivineLightAction(ai); }
            static Action* divine_light_on_party(PlayerbotAI* ai) { return new CastDivineLightOnPartyAction(ai); }
            static Action* beacon_of_light_on_tank(PlayerbotAI* ai) { return new CastBeaconOfLightOnTankAction(ai); }
            static Action* templars_verdict(PlayerbotAI* ai) { return new CastTemplarsVerdictAction(ai); }
            static Action* inquisition(PlayerbotAI* ai) { return new CastInquisitionAction(ai); }
            static Action* avenging_wrath(PlayerbotAI* ai) { return new CastAvengingWrathAction(ai); }
            static Action* rebuke(PlayerbotAI* ai) { return new CastRebukeAction(ai); }
            static Action* shield_of_the_righteous(PlayerbotAI* ai) { return new CastShieldOfTheRighteousAction(ai); }
            static Action* word_of_glory(PlayerbotAI* ai) { return new CastWordOfGloryAction(ai); }
            static Action* guardian_of_ancient_kings(PlayerbotAI* ai) { return new CastGuardianOfAncientKingsAction(ai); }
            static Action* ardent_defender(PlayerbotAI* ai) { return new CastArdentDefenderAction(ai); }
        };
    };
};


PaladinAiObjectContext::PaladinAiObjectContext(PlayerbotAI* ai) : AiObjectContext(ai)
{
    strategyContexts.Add(new ai::paladin::StrategyFactoryInternal());
    strategyContexts.Add(new ai::paladin::CombatStrategyFactoryInternal());
    strategyContexts.Add(new ai::paladin::BuffStrategyFactoryInternal());
    strategyContexts.Add(new ai::paladin::ResistanceStrategyFactoryInternal());
    actionContexts.Add(new ai::paladin::AiObjectContextInternal());
    triggerContexts.Add(new ai::paladin::TriggerFactoryInternal());
}
