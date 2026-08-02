#include "botpch.h"
#include "../../playerbot.h"
#include "RogueActions.h"
#include "RogueTriggers.h"
#include "../triggers/ChatCommandTrigger.h"
#include "RogueAiObjectContext.h"
#include "DpsRogueStrategy.h"
#include "GenericRogueNonCombatStrategy.h"
#include "RogueAmbushStrategy.h"
#include "RogueSapStrategy.h"
#include "../NamedObjectContext.h"

using namespace ai;


namespace ai
{
    namespace rogue
    {
        using namespace ai;

        class StrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            StrategyFactoryInternal()
            {
                creators["dps"] = &rogue::StrategyFactoryInternal::dps;
                creators["nc"] = &rogue::StrategyFactoryInternal::nc;
                creators["ambush"] = &rogue::StrategyFactoryInternal::ambush;
                creators["sap"] = &rogue::StrategyFactoryInternal::sap_strategy;
            }

        private:
            static Strategy* dps(PlayerbotAI* ai) { return new DpsRogueStrategy(ai); }
            static Strategy* nc(PlayerbotAI* ai) { return new GenericRogueNonCombatStrategy(ai); }
            static Strategy* ambush(PlayerbotAI* ai) { return new RogueAmbushStrategy(ai); }
            static Strategy* sap_strategy(PlayerbotAI* ai) { return new RogueSapStrategy(ai); }
        };
    };
};

namespace ai
{
    namespace rogue
    {
        using namespace ai;

        class TriggerFactoryInternal : public NamedObjectContext<Trigger>
        {
        public:
            TriggerFactoryInternal()
            {
                creators["kick"] = &TriggerFactoryInternal::kick;
                creators["rupture"] = &TriggerFactoryInternal::rupture;
                creators["slice and dice"] = &TriggerFactoryInternal::slice_and_dice;
                creators["expose armor"] = &TriggerFactoryInternal::expose_armor;
                creators["kick on enemy healer"] = &TriggerFactoryInternal::kick_on_enemy_healer;
                creators["combo points for target available"] = &TriggerFactoryInternal::combo_points_for_target_available;
                creators["stealth"] = &TriggerFactoryInternal::stealth;
                creators["sap"] = &TriggerFactoryInternal::sap;
                creators["revealing strike"] = &TriggerFactoryInternal::revealing_strike;
                creators["blade flurry"] = &TriggerFactoryInternal::blade_flurry;
                creators["adrenaline rush"] = &TriggerFactoryInternal::adrenaline_rush;
                creators["killing spree"] = &TriggerFactoryInternal::killing_spree;
                creators["vendetta"] = &TriggerFactoryInternal::vendetta;
                creators["cold blood"] = &TriggerFactoryInternal::cold_blood;
                creators["shadow dance"] = &TriggerFactoryInternal::shadow_dance;
                creators["shadow dance active"] = &TriggerFactoryInternal::shadow_dance_active;
                creators["hemorrhage"] = &TriggerFactoryInternal::hemorrhage;
                creators["tricks of the trade on party"] = &TriggerFactoryInternal::tricks_of_the_trade_on_party;

            }

        private:
            static Trigger* kick(PlayerbotAI* ai) { return new KickInterruptSpellTrigger(ai); }
            static Trigger* rupture(PlayerbotAI* ai) { return new RuptureTrigger(ai); }
            static Trigger* slice_and_dice(PlayerbotAI* ai) { return new SliceAndDiceTrigger(ai); }
            static Trigger* expose_armor(PlayerbotAI* ai) { return new ExposeArmorTrigger(ai); }
            static Trigger* kick_on_enemy_healer(PlayerbotAI* ai) { return new KickInterruptEnemyHealerSpellTrigger(ai); }
            static Trigger* combo_points_for_target_available(PlayerbotAI* ai) { return new ComboPointsForTargetAvailableTrigger(ai); }
            static Trigger* stealth(PlayerbotAI* ai) { return new StealthTrigger(ai); }
            static Trigger* sap(PlayerbotAI* ai) { return new ChatCommandTrigger(ai, "sap"); }
            static Trigger* revealing_strike(PlayerbotAI* ai) { return new RevealingStrikeTrigger(ai); }
            static Trigger* blade_flurry(PlayerbotAI* ai) { return new BladeFlurryTrigger(ai); }
            static Trigger* adrenaline_rush(PlayerbotAI* ai) { return new AdrenalineRushTrigger(ai); }
            static Trigger* killing_spree(PlayerbotAI* ai) { return new KillingSpreeTrigger(ai); }
            static Trigger* vendetta(PlayerbotAI* ai) { return new VendettaTrigger(ai); }
            static Trigger* cold_blood(PlayerbotAI* ai) { return new ColdBloodTrigger(ai); }
            static Trigger* shadow_dance(PlayerbotAI* ai) { return new ShadowDanceTrigger(ai); }
            static Trigger* shadow_dance_active(PlayerbotAI* ai) { return new ShadowDanceActiveTrigger(ai); }
            static Trigger* hemorrhage(PlayerbotAI* ai) { return new HemorrhageTrigger(ai); }
            static Trigger* tricks_of_the_trade_on_party(PlayerbotAI* ai) { return new BuffOnPartyTrigger(ai, "tricks of the trade"); }
        };
    };
};


namespace ai
{
    namespace rogue
    {
        using namespace ai;

        class AiObjectContextInternal : public NamedObjectContext<Action>
        {
        public:
            AiObjectContextInternal()
            {
                creators["mutilate"] = &AiObjectContextInternal::mutilate;
                creators["sinister strike"] = &AiObjectContextInternal::sinister_strike;
                creators["kidney shot"] = &AiObjectContextInternal::kidney_shot;
                creators["rupture"] = &AiObjectContextInternal::rupture;
                creators["slice and dice"] = &AiObjectContextInternal::slice_and_dice;
                creators["eviscerate"] = &AiObjectContextInternal::eviscerate;
                creators["vanish"] = &AiObjectContextInternal::vanish;
                creators["evasion"] = &AiObjectContextInternal::evasion;
                creators["kick"] = &AiObjectContextInternal::kick;
                creators["feint"] = &AiObjectContextInternal::feint;
                creators["backstab"] = &AiObjectContextInternal::backstab;
                creators["expose armor"] = &AiObjectContextInternal::expose_armor;
                creators["kick on enemy healer"] = &AiObjectContextInternal::kick_on_enemy_healer;
                creators["sap"] = &AiObjectContextInternal::sap;
                creators["begin sap"] = &AiObjectContextInternal::begin_sap;
                creators["end sap"] = &AiObjectContextInternal::end_sap;
                creators["garrote"] = &AiObjectContextInternal::garrote;
                creators["cheap shot"] = &AiObjectContextInternal::cheap_shot;
                creators["stealth"] = &AiObjectContextInternal::stealth;
                creators["begin ambush"] = &AiObjectContextInternal::begin_ambush;
                creators["end ambush"] = &AiObjectContextInternal::end_ambush;
                creators["revealing strike"] = &AiObjectContextInternal::revealing_strike;
                creators["blade flurry"] = &AiObjectContextInternal::blade_flurry;
                creators["adrenaline rush"] = &AiObjectContextInternal::adrenaline_rush;
                creators["killing spree"] = &AiObjectContextInternal::killing_spree;
                creators["envenom"] = &AiObjectContextInternal::envenom;
                creators["vendetta"] = &AiObjectContextInternal::vendetta;
                creators["cold blood"] = &AiObjectContextInternal::cold_blood;
                creators["shadow dance"] = &AiObjectContextInternal::shadow_dance;
                creators["ambush"] = &AiObjectContextInternal::ambush;
                creators["hemorrhage"] = &AiObjectContextInternal::hemorrhage;
                creators["fan of knives"] = &AiObjectContextInternal::fan_of_knives;
                creators["tricks of the trade on party"] = &AiObjectContextInternal::tricks_of_the_trade_on_party;
            }

        private:
            static Action* mutilate(PlayerbotAI* ai) { return new CastMutilateAction(ai); }
            static Action* sinister_strike(PlayerbotAI* ai) { return new CastSinisterStrikeAction(ai); }
            static Action* kidney_shot(PlayerbotAI* ai) { return new CastKidneyShotAction(ai); }
            static Action* rupture(PlayerbotAI* ai) { return new CastRuptureAction(ai); }
            static Action* slice_and_dice(PlayerbotAI* ai) { return new CastSliceAndDiceAction(ai); }
            static Action* eviscerate(PlayerbotAI* ai) { return new CastEviscerateAction(ai); }
            static Action* vanish(PlayerbotAI* ai) { return new CastVanishAction(ai); }
            static Action* evasion(PlayerbotAI* ai) { return new CastEvasionAction(ai); }
            static Action* kick(PlayerbotAI* ai) { return new CastKickAction(ai); }
            static Action* feint(PlayerbotAI* ai) { return new CastFeintAction(ai); }
            static Action* backstab(PlayerbotAI* ai) { return new CastBackstabAction(ai); }
            static Action* expose_armor(PlayerbotAI* ai) { return new CastExposeArmorAction(ai); }
            static Action* kick_on_enemy_healer(PlayerbotAI* ai) { return new CastKickOnEnemyHealerAction(ai); }
            static Action* sap(PlayerbotAI* ai) { return new CastSapAction(ai); }
            static Action* begin_sap(PlayerbotAI* ai) { return new BeginSapAction(ai); }
            static Action* end_sap(PlayerbotAI* ai) { return new EndSapAction(ai); }
            static Action* garrote(PlayerbotAI* ai) { return new CastGarroteAction(ai); }
            static Action* cheap_shot(PlayerbotAI* ai) { return new CastCheapShotAction(ai); }
            static Action* stealth(PlayerbotAI* ai) { return new CastStealthAction(ai); }
            static Action* begin_ambush(PlayerbotAI* ai) { return new BeginAmbushAction(ai); }
            static Action* end_ambush(PlayerbotAI* ai) { return new RogueEndAmbushAction(ai); }
            static Action* revealing_strike(PlayerbotAI* ai) { return new CastRevealingStrikeAction(ai); }
            static Action* blade_flurry(PlayerbotAI* ai) { return new CastBladeFlurryAction(ai); }
            static Action* adrenaline_rush(PlayerbotAI* ai) { return new CastAdrenalineRushAction(ai); }
            static Action* killing_spree(PlayerbotAI* ai) { return new CastKillingSpreeAction(ai); }
            static Action* envenom(PlayerbotAI* ai) { return new CastEnvenomAction(ai); }
            static Action* vendetta(PlayerbotAI* ai) { return new CastVendettaAction(ai); }
            static Action* cold_blood(PlayerbotAI* ai) { return new CastColdBloodAction(ai); }
            static Action* shadow_dance(PlayerbotAI* ai) { return new CastShadowDanceAction(ai); }
            static Action* ambush(PlayerbotAI* ai) { return new CastAmbushAction(ai); }
            static Action* hemorrhage(PlayerbotAI* ai) { return new CastHemorrhageAction(ai); }
            static Action* fan_of_knives(PlayerbotAI* ai) { return new CastFanOfKnivesAction(ai); }
            static Action* tricks_of_the_trade_on_party(PlayerbotAI* ai) { return new CastTricksOfTheTradeAction(ai); }
        };
    };
};

RogueAiObjectContext::RogueAiObjectContext(PlayerbotAI* ai) : AiObjectContext(ai)
{
    strategyContexts.Add(new ai::rogue::StrategyFactoryInternal());
    actionContexts.Add(new ai::rogue::AiObjectContextInternal());
    triggerContexts.Add(new ai::rogue::TriggerFactoryInternal());
}
