#include "botpch.h"
#include "../../playerbot.h"
#include "WarlockActions.h"
#include "WarlockAiObjectContext.h"
#include "DpsWarlockStrategy.h"
#include "GenericWarlockNonCombatStrategy.h"
#include "TankWarlockStrategy.h"
#include "../generic/PullStrategy.h"
#include "WarlockTriggers.h"
#include "../NamedObjectContext.h"
#include "../actions/UseItemAction.h"

using namespace ai;

namespace ai
{
    namespace warlock
    {
        using namespace ai;

        class StrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            StrategyFactoryInternal()
            {
                creators["nc"] = &warlock::StrategyFactoryInternal::nc;
                creators["pull"] = &warlock::StrategyFactoryInternal::pull;
                creators["aoe"] = &warlock::StrategyFactoryInternal::aoe;
                creators["dps debuff"] = &warlock::StrategyFactoryInternal::dps_debuff;
            }

        private:
            static Strategy* nc(PlayerbotAI* ai) { return new GenericWarlockNonCombatStrategy(ai); }
            static Strategy* aoe(PlayerbotAI* ai) { return new DpsAoeWarlockStrategy(ai); }
            static Strategy* dps_debuff(PlayerbotAI* ai) { return new DpsWarlockDebuffStrategy(ai); }
            static Strategy* pull(PlayerbotAI* ai) { return new PullStrategy(ai, "shoot"); }
        };

        class CombatStrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            CombatStrategyFactoryInternal() : NamedObjectContext<Strategy>(false, true)
            {
                creators["dps"] = &warlock::CombatStrategyFactoryInternal::dps;
                creators["tank"] = &warlock::CombatStrategyFactoryInternal::tank;
            }

        private:
            static Strategy* tank(PlayerbotAI* ai) { return new TankWarlockStrategy(ai); }
            static Strategy* dps(PlayerbotAI* ai) { return new DpsWarlockStrategy(ai); }
        };
    };
};

namespace ai
{
    namespace warlock
    {
        using namespace ai;

        class TriggerFactoryInternal : public NamedObjectContext<Trigger>
        {
        public:
            TriggerFactoryInternal()
            {
                creators["shadow trance"] = &TriggerFactoryInternal::shadow_trance;
                creators["demon armor"] = &TriggerFactoryInternal::demon_armor;
                creators["no healthstone"] = &TriggerFactoryInternal::HasHealthstone;
                creators["corruption"] = &TriggerFactoryInternal::corruption;
                creators["corruption on attacker"] = &TriggerFactoryInternal::corruption_on_attacker;
                creators["bane of agony"] = &TriggerFactoryInternal::curse_of_agony;
                creators["banish"] = &TriggerFactoryInternal::banish;
                creators["backlash"] = &TriggerFactoryInternal::backlash;
                creators["fear"] = &TriggerFactoryInternal::fear;
                creators["immolate"] = &TriggerFactoryInternal::immolate;
                creators["unstable affliction"] = &TriggerFactoryInternal::unstable_affliction;
                creators["haunt"] = &TriggerFactoryInternal::haunt;
                creators["metamorphosis"] = &TriggerFactoryInternal::metamorphosis;
                creators["hand of gul'dan"] = &TriggerFactoryInternal::hand_of_guldan;
                creators["immolation aura"] = &TriggerFactoryInternal::immolation_aura;
                creators["chaos bolt"] = &TriggerFactoryInternal::chaos_bolt;
                creators["conflagrate"] = &TriggerFactoryInternal::conflagrate;

            }

        private:
            static Trigger* shadow_trance(PlayerbotAI* ai) { return new ShadowTranceTrigger(ai); }
            static Trigger* demon_armor(PlayerbotAI* ai) { return new DemonArmorTrigger(ai); }
            static Trigger* HasHealthstone(PlayerbotAI* ai) { return new HasHealthstoneTrigger(ai); }
            static Trigger* corruption(PlayerbotAI* ai) { return new CorruptionTrigger(ai); }
            static Trigger* corruption_on_attacker(PlayerbotAI* ai) { return new CorruptionOnAttackerTrigger(ai); }
            static Trigger* curse_of_agony(PlayerbotAI* ai) { return new CurseOfAgonyTrigger(ai); }
            static Trigger* banish(PlayerbotAI* ai) { return new BanishTrigger(ai); }
            static Trigger* backlash(PlayerbotAI* ai) { return new BacklashTrigger(ai); }
            static Trigger* fear(PlayerbotAI* ai) { return new FearTrigger(ai); }
            static Trigger* immolate(PlayerbotAI* ai) { return new ImmolateTrigger(ai); }
            static Trigger* unstable_affliction(PlayerbotAI* ai) { return new UnstableAfflictionTrigger(ai); }
            static Trigger* haunt(PlayerbotAI* ai) { return new HauntTrigger(ai); }
            static Trigger* metamorphosis(PlayerbotAI* ai) { return new MetamorphosisTrigger(ai); }
            static Trigger* hand_of_guldan(PlayerbotAI* ai) { return new HandOfGuldanTrigger(ai); }
            static Trigger* immolation_aura(PlayerbotAI* ai) { return new ImmolationAuraTrigger(ai); }
            static Trigger* chaos_bolt(PlayerbotAI* ai) { return new ChaosBoltTrigger(ai); }
            static Trigger* conflagrate(PlayerbotAI* ai) { return new ConflagrateTrigger(ai); }

        };
    };
};

namespace ai
{
    namespace warlock
    {
        using namespace ai;

        class AiObjectContextInternal : public NamedObjectContext<Action>
        {
        public:
            AiObjectContextInternal()
            {
                creators["summon imp"] = &AiObjectContextInternal::summon_imp;
                creators["fel armor"] = &AiObjectContextInternal::fel_armor;
                creators["demon armor"] = &AiObjectContextInternal::demon_armor;
                creators["demon skin"] = &AiObjectContextInternal::demon_skin;
                creators["create healthstone"] = &AiObjectContextInternal::create_healthstone;
                creators["summon voidwalker"] = &AiObjectContextInternal::summon_voidwalker;
                creators["summon felguard"] = &AiObjectContextInternal::summon_felguard;
                creators["immolate"] = &AiObjectContextInternal::immolate;
                creators["corruption"] = &AiObjectContextInternal::corruption;
                creators["corruption on attacker"] = &AiObjectContextInternal::corruption_on_attacker;
                creators["curse of agony"] = &AiObjectContextInternal::curse_of_agony;
                creators["shadow bolt"] = &AiObjectContextInternal::shadow_bolt;
                creators["drain soul"] = &AiObjectContextInternal::drain_soul;
                creators["drain life"] = &AiObjectContextInternal::drain_life;
                creators["banish"] = &AiObjectContextInternal::banish;
                creators["seed of corruption"] = &AiObjectContextInternal::seed_of_corruption;
                creators["rain of fire"] = &AiObjectContextInternal::rain_of_fire;
                creators["shadowfury"] = &AiObjectContextInternal::shadowfury;
                creators["life tap"] = &AiObjectContextInternal::life_tap;
                creators["fear"] = &AiObjectContextInternal::fear;
                creators["fear on cc"] = &AiObjectContextInternal::fear_on_cc;
                creators["incinerate"] = &AiObjectContextInternal::incinerate;
                creators["conflagrate"] = &AiObjectContextInternal::conflagrate;
                creators["unstable affliction"] = &AiObjectContextInternal::unstable_affliction;
                creators["haunt"] = &AiObjectContextInternal::haunt;
                creators["metamorphosis"] = &AiObjectContextInternal::metamorphosis;
                creators["hand of gul'dan"] = &AiObjectContextInternal::hand_of_guldan;
                creators["immolation aura"] = &AiObjectContextInternal::immolation_aura;
                creators["chaos bolt"] = &AiObjectContextInternal::chaos_bolt;
                creators["shadowburn"] = &AiObjectContextInternal::shadowburn;
            }

        private:
            static Action* unstable_affliction(PlayerbotAI* ai) { return new CastUnstableAfflictionAction(ai); }
            static Action* haunt(PlayerbotAI* ai) { return new CastHauntAction(ai); }
            static Action* metamorphosis(PlayerbotAI* ai) { return new CastMetamorphosisAction(ai); }
            static Action* hand_of_guldan(PlayerbotAI* ai) { return new CastHandOfGuldanAction(ai); }
            static Action* immolation_aura(PlayerbotAI* ai) { return new CastImmolationAuraAction(ai); }
            static Action* chaos_bolt(PlayerbotAI* ai) { return new CastChaosBoltAction(ai); }
            static Action* shadowburn(PlayerbotAI* ai) { return new CastShadowburnAction(ai); }
            static Action* conflagrate(PlayerbotAI* ai) { return new CastConflagrateAction(ai); }
            static Action* incinerate(PlayerbotAI* ai) { return new CastIncinerateAction(ai); }
            static Action* fear_on_cc(PlayerbotAI* ai) { return new CastFearOnCcAction(ai); }
            static Action* fear(PlayerbotAI* ai) { return new CastFearAction(ai); }
            static Action* immolate(PlayerbotAI* ai) { return new CastImmolateAction(ai); }
            static Action* summon_imp(PlayerbotAI* ai) { return new CastSummonImpAction(ai); }
            static Action* fel_armor(PlayerbotAI* ai) { return new CastFelArmorAction(ai); }
            static Action* demon_armor(PlayerbotAI* ai) { return new CastDemonArmorAction(ai); }
            static Action* demon_skin(PlayerbotAI* ai) { return new CastDemonSkinAction(ai); }
            static Action* create_healthstone(PlayerbotAI* ai) { return new CastCreateHealthstoneAction(ai); }
            static Action* summon_voidwalker(PlayerbotAI* ai) { return new CastSummonVoidwalkerAction(ai); }
            static Action* summon_felguard(PlayerbotAI* ai) { return new CastSummonFelguardAction(ai); }
            static Action* corruption(PlayerbotAI* ai) { return new CastCorruptionAction(ai); }
            static Action* corruption_on_attacker(PlayerbotAI* ai) { return new CastCorruptionOnAttackerAction(ai); }
            static Action* curse_of_agony(PlayerbotAI* ai) { return new CastCurseOfAgonyAction(ai); }
            static Action* shadow_bolt(PlayerbotAI* ai) { return new CastShadowBoltAction(ai); }
            static Action* drain_soul(PlayerbotAI* ai) { return new CastDrainSoulAction(ai); }
            static Action* drain_life(PlayerbotAI* ai) { return new CastDrainLifeAction(ai); }
            static Action* banish(PlayerbotAI* ai) { return new CastBanishAction(ai); }
            static Action* seed_of_corruption(PlayerbotAI* ai) { return new CastSeedOfCorruptionAction(ai); }
            static Action* rain_of_fire(PlayerbotAI* ai) { return new CastRainOfFireAction(ai); }
            static Action* shadowfury(PlayerbotAI* ai) { return new CastShadowfuryAction(ai); }
            static Action* life_tap(PlayerbotAI* ai) { return new CastLifeTapAction(ai); }

        };
    };
};



WarlockAiObjectContext::WarlockAiObjectContext(PlayerbotAI* ai) : AiObjectContext(ai)
{
    strategyContexts.Add(new ai::warlock::StrategyFactoryInternal());
    strategyContexts.Add(new ai::warlock::CombatStrategyFactoryInternal());
    actionContexts.Add(new ai::warlock::AiObjectContextInternal());
    triggerContexts.Add(new ai::warlock::TriggerFactoryInternal());
}
