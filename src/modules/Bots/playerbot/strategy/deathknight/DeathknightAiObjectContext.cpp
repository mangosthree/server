#include "botpch.h"
#include "../../playerbot.h"
#include "DeathknightActions.h"
#include "DeathknightTriggers.h"
#include "DeathknightAiObjectContext.h"
#include "GenericDeathknightNonCombatStrategy.h"
#include "BloodDeathknightStrategy.h"
#include "FrostDeathknightStrategy.h"
#include "UnholyDeathknightStrategy.h"
#include "DeathknightBuffStrategies.h"
#include "../NamedObjectContext.h"

using namespace ai;

namespace ai
{
    namespace deathknight
    {
        using namespace ai;

        class StrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            StrategyFactoryInternal()
            {
                creators["nc"] = &deathknight::StrategyFactoryInternal::nc;
            }

        private:
            static Strategy* nc(PlayerbotAI* ai) { return new GenericDeathknightNonCombatStrategy(ai); }
        };

        class BuffStrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            BuffStrategyFactoryInternal() : NamedObjectContext<Strategy>(false, true)
            {
                creators["bthreat"] = &deathknight::BuffStrategyFactoryInternal::bthreat;
                creators["bdps"] = &deathknight::BuffStrategyFactoryInternal::bdps;
            }

        private:
            static Strategy* bthreat(PlayerbotAI* ai) { return new DeathknightBuffThreatStrategy(ai); }
            static Strategy* bdps(PlayerbotAI* ai) { return new DeathknightBuffDpsStrategy(ai); }
        };

        class CombatStrategyFactoryInternal : public NamedObjectContext<Strategy>
        {
        public:
            CombatStrategyFactoryInternal() : NamedObjectContext<Strategy>(false, true)
            {
                creators["tank"] = &deathknight::CombatStrategyFactoryInternal::tank;
                creators["dps"] = &deathknight::CombatStrategyFactoryInternal::dps;
                creators["unholy"] = &deathknight::CombatStrategyFactoryInternal::unholy;
            }

        private:
            static Strategy* tank(PlayerbotAI* ai) { return new BloodDeathknightStrategy(ai); }
            static Strategy* dps(PlayerbotAI* ai) { return new FrostDeathknightStrategy(ai); }
            static Strategy* unholy(PlayerbotAI* ai) { return new UnholyDeathknightStrategy(ai); }
        };
    };
};

namespace ai
{
    namespace deathknight
    {
        using namespace ai;

        class TriggerFactoryInternal : public NamedObjectContext<Trigger>
        {
        public:
            TriggerFactoryInternal()
            {
                creators["blood presence"] = &TriggerFactoryInternal::blood_presence;
                creators["unholy presence"] = &TriggerFactoryInternal::unholy_presence;
                creators["frost presence"] = &TriggerFactoryInternal::frost_presence;
                creators["bone shield"] = &TriggerFactoryInternal::bone_shield;
                creators["horn of winter"] = &TriggerFactoryInternal::horn_of_winter;
                creators["frost fever"] = &TriggerFactoryInternal::frost_fever;
                creators["blood plague"] = &TriggerFactoryInternal::blood_plague;
                creators["mind freeze interrupt"] = &TriggerFactoryInternal::mind_freeze_interrupt;
                creators["mind freeze on enemy healer"] = &TriggerFactoryInternal::mind_freeze_on_enemy_healer;
                creators["runic power available"] = &TriggerFactoryInternal::runic_power_available;
                creators["rune strike available"] = &TriggerFactoryInternal::rune_strike_available;
                creators["rune pair depleted"] = &TriggerFactoryInternal::rune_pair_depleted;
                creators["obliterate pair"] = &TriggerFactoryInternal::obliterate_pair;
                creators["death strike available"] = &TriggerFactoryInternal::death_strike_available;
                creators["obliterate available"] = &TriggerFactoryInternal::obliterate_available;
                creators["scourge strike available"] = &TriggerFactoryInternal::scourge_strike_available;
                creators["festering strike available"] = &TriggerFactoryInternal::festering_strike_available;
                creators["howling blast available"] = &TriggerFactoryInternal::howling_blast_available;
                creators["dark transformation available"] = &TriggerFactoryInternal::dark_transformation_available;
                creators["killing machine"] = &TriggerFactoryInternal::killing_machine;
                creators["freezing fog"] = &TriggerFactoryInternal::freezing_fog;
                creators["sudden doom"] = &TriggerFactoryInternal::sudden_doom;
                creators["crimson scourge"] = &TriggerFactoryInternal::crimson_scourge;
                creators["pillar of frost"] = &TriggerFactoryInternal::pillar_of_frost;
                creators["unholy frenzy"] = &TriggerFactoryInternal::unholy_frenzy;
                creators["dancing rune weapon"] = &TriggerFactoryInternal::dancing_rune_weapon;
            }

        private:
            static Trigger* blood_presence(PlayerbotAI* ai) { return new BloodPresenceTrigger(ai); }
            static Trigger* unholy_presence(PlayerbotAI* ai) { return new UnholyPresenceTrigger(ai); }
            static Trigger* frost_presence(PlayerbotAI* ai) { return new FrostPresenceTrigger(ai); }
            static Trigger* bone_shield(PlayerbotAI* ai) { return new BoneShieldTrigger(ai); }
            static Trigger* horn_of_winter(PlayerbotAI* ai) { return new HornOfWinterTrigger(ai); }
            static Trigger* frost_fever(PlayerbotAI* ai) { return new FrostFeverTrigger(ai); }
            static Trigger* blood_plague(PlayerbotAI* ai) { return new BloodPlagueTrigger(ai); }
            static Trigger* mind_freeze_interrupt(PlayerbotAI* ai) { return new MindFreezeInterruptSpellTrigger(ai); }
            static Trigger* mind_freeze_on_enemy_healer(PlayerbotAI* ai) { return new MindFreezeEnemyHealerTrigger(ai); }
            static Trigger* runic_power_available(PlayerbotAI* ai) { return new RunicPowerCapTrigger(ai); }
            static Trigger* rune_strike_available(PlayerbotAI* ai) { return new RuneStrikeAvailableTrigger(ai); }
            static Trigger* rune_pair_depleted(PlayerbotAI* ai) { return new RunePairDepletedTrigger(ai); }
            static Trigger* obliterate_pair(PlayerbotAI* ai) { return new RunePairAvailableTrigger(ai); }
            static Trigger* death_strike_available(PlayerbotAI* ai) { return new DeathStrikeAvailableTrigger(ai); }
            static Trigger* obliterate_available(PlayerbotAI* ai) { return new ObliterateAvailableTrigger(ai); }
            static Trigger* scourge_strike_available(PlayerbotAI* ai) { return new ScourgeStrikeAvailableTrigger(ai); }
            static Trigger* festering_strike_available(PlayerbotAI* ai) { return new FesteringStrikeAvailableTrigger(ai); }
            static Trigger* howling_blast_available(PlayerbotAI* ai) { return new HowlingBlastAvailableTrigger(ai); }
            static Trigger* dark_transformation_available(PlayerbotAI* ai) { return new DarkTransformationAvailableTrigger(ai); }
            static Trigger* killing_machine(PlayerbotAI* ai) { return new KillingMachineTrigger(ai); }
            static Trigger* freezing_fog(PlayerbotAI* ai) { return new FreezingFogTrigger(ai); }
            static Trigger* sudden_doom(PlayerbotAI* ai) { return new SuddenDoomTrigger(ai); }
            static Trigger* crimson_scourge(PlayerbotAI* ai) { return new CrimsonScourgeTrigger(ai); }
            static Trigger* pillar_of_frost(PlayerbotAI* ai) { return new PillarOfFrostTrigger(ai); }
            static Trigger* unholy_frenzy(PlayerbotAI* ai) { return new UnholyFrenzyTrigger(ai); }
            static Trigger* dancing_rune_weapon(PlayerbotAI* ai) { return new DancingRuneWeaponTrigger(ai); }
        };
    };
};

namespace ai
{
    namespace deathknight
    {
        using namespace ai;

        class AiObjectContextInternal : public NamedObjectContext<Action>
        {
        public:
            AiObjectContextInternal()
            {
                creators["blood presence"] = &AiObjectContextInternal::blood_presence;
                creators["unholy presence"] = &AiObjectContextInternal::unholy_presence;
                creators["frost presence"] = &AiObjectContextInternal::frost_presence;
                creators["icy touch"] = &AiObjectContextInternal::icy_touch;
                creators["plague strike"] = &AiObjectContextInternal::plague_strike;
                creators["outbreak"] = &AiObjectContextInternal::outbreak;
                creators["pestilence"] = &AiObjectContextInternal::pestilence;
                creators["death strike"] = &AiObjectContextInternal::death_strike;
                creators["heart strike"] = &AiObjectContextInternal::heart_strike;
                creators["obliterate"] = &AiObjectContextInternal::obliterate;
                creators["frost strike"] = &AiObjectContextInternal::frost_strike;
                creators["howling blast"] = &AiObjectContextInternal::howling_blast;
                creators["scourge strike"] = &AiObjectContextInternal::scourge_strike;
                creators["festering strike"] = &AiObjectContextInternal::festering_strike;
                creators["rune strike"] = &AiObjectContextInternal::rune_strike;
                creators["death coil"] = &AiObjectContextInternal::death_coil;
                creators["death and decay"] = &AiObjectContextInternal::death_and_decay;
                creators["blood boil"] = &AiObjectContextInternal::blood_boil;
                creators["death grip"] = &AiObjectContextInternal::death_grip;
                creators["dark command"] = &AiObjectContextInternal::dark_command;
                creators["chains of ice"] = &AiObjectContextInternal::chains_of_ice;
                creators["mind freeze"] = &AiObjectContextInternal::mind_freeze;
                creators["mind freeze on enemy healer"] = &AiObjectContextInternal::mind_freeze_on_enemy_healer;
                creators["horn of winter"] = &AiObjectContextInternal::horn_of_winter;
                creators["bone shield"] = &AiObjectContextInternal::bone_shield;
                creators["icebound fortitude"] = &AiObjectContextInternal::icebound_fortitude;
                creators["vampiric blood"] = &AiObjectContextInternal::vampiric_blood;
                creators["anti-magic shell"] = &AiObjectContextInternal::anti_magic_shell;
                creators["pillar of frost"] = &AiObjectContextInternal::pillar_of_frost;
                creators["unholy frenzy"] = &AiObjectContextInternal::unholy_frenzy;
                creators["dancing rune weapon"] = &AiObjectContextInternal::dancing_rune_weapon;
                creators["lichborne"] = &AiObjectContextInternal::lichborne;
                creators["raise dead"] = &AiObjectContextInternal::raise_dead;
                creators["summon gargoyle"] = &AiObjectContextInternal::summon_gargoyle;
                creators["dark transformation"] = &AiObjectContextInternal::dark_transformation;
                creators["army of the dead"] = &AiObjectContextInternal::army_of_the_dead;
                creators["empower rune weapon"] = &AiObjectContextInternal::empower_rune_weapon;
                creators["blood tap"] = &AiObjectContextInternal::blood_tap;
                creators["rune tap"] = &AiObjectContextInternal::rune_tap;
                creators["death pact"] = &AiObjectContextInternal::death_pact;
            }

        private:
            static Action* blood_presence(PlayerbotAI* ai) { return new CastBloodPresenceAction(ai); }
            static Action* unholy_presence(PlayerbotAI* ai) { return new CastUnholyPresenceAction(ai); }
            static Action* frost_presence(PlayerbotAI* ai) { return new CastFrostPresenceAction(ai); }
            static Action* icy_touch(PlayerbotAI* ai) { return new CastIcyTouchAction(ai); }
            static Action* plague_strike(PlayerbotAI* ai) { return new CastPlagueStrikeAction(ai); }
            static Action* outbreak(PlayerbotAI* ai) { return new CastOutbreakAction(ai); }
            static Action* pestilence(PlayerbotAI* ai) { return new CastPestilenceAction(ai); }
            static Action* death_strike(PlayerbotAI* ai) { return new CastDeathStrikeAction(ai); }
            static Action* heart_strike(PlayerbotAI* ai) { return new CastHeartStrikeAction(ai); }
            static Action* obliterate(PlayerbotAI* ai) { return new CastObliterateAction(ai); }
            static Action* frost_strike(PlayerbotAI* ai) { return new CastFrostStrikeAction(ai); }
            static Action* howling_blast(PlayerbotAI* ai) { return new CastHowlingBlastAction(ai); }
            static Action* scourge_strike(PlayerbotAI* ai) { return new CastScourgeStrikeAction(ai); }
            static Action* festering_strike(PlayerbotAI* ai) { return new CastFesteringStrikeAction(ai); }
            static Action* rune_strike(PlayerbotAI* ai) { return new CastRuneStrikeAction(ai); }
            static Action* death_coil(PlayerbotAI* ai) { return new CastDeathCoilAction(ai); }
            static Action* death_and_decay(PlayerbotAI* ai) { return new CastDeathAndDecayAction(ai); }
            static Action* blood_boil(PlayerbotAI* ai) { return new CastBloodBoilAction(ai); }
            static Action* death_grip(PlayerbotAI* ai) { return new CastDeathGripAction(ai); }
            static Action* dark_command(PlayerbotAI* ai) { return new CastDarkCommandAction(ai); }
            static Action* chains_of_ice(PlayerbotAI* ai) { return new CastChainsOfIceAction(ai); }
            static Action* mind_freeze(PlayerbotAI* ai) { return new CastMindFreezeAction(ai); }
            static Action* mind_freeze_on_enemy_healer(PlayerbotAI* ai) { return new CastMindFreezeOnEnemyHealerAction(ai); }
            static Action* horn_of_winter(PlayerbotAI* ai) { return new CastHornOfWinterAction(ai); }
            static Action* bone_shield(PlayerbotAI* ai) { return new CastBoneShieldAction(ai); }
            static Action* icebound_fortitude(PlayerbotAI* ai) { return new CastIceboundFortitudeAction(ai); }
            static Action* vampiric_blood(PlayerbotAI* ai) { return new CastVampiricBloodAction(ai); }
            static Action* anti_magic_shell(PlayerbotAI* ai) { return new CastAntiMagicShellAction(ai); }
            static Action* pillar_of_frost(PlayerbotAI* ai) { return new CastPillarOfFrostAction(ai); }
            static Action* unholy_frenzy(PlayerbotAI* ai) { return new CastUnholyFrenzyAction(ai); }
            static Action* dancing_rune_weapon(PlayerbotAI* ai) { return new CastDancingRuneWeaponAction(ai); }
            static Action* lichborne(PlayerbotAI* ai) { return new CastLichborneAction(ai); }
            static Action* raise_dead(PlayerbotAI* ai) { return new CastRaiseDeadAction(ai); }
            static Action* summon_gargoyle(PlayerbotAI* ai) { return new CastSummonGargoyleAction(ai); }
            static Action* dark_transformation(PlayerbotAI* ai) { return new CastDarkTransformationAction(ai); }
            static Action* army_of_the_dead(PlayerbotAI* ai) { return new CastArmyOfTheDeadAction(ai); }
            static Action* empower_rune_weapon(PlayerbotAI* ai) { return new CastEmpowerRuneWeaponAction(ai); }
            static Action* blood_tap(PlayerbotAI* ai) { return new CastBloodTapAction(ai); }
            static Action* rune_tap(PlayerbotAI* ai) { return new CastRuneTapAction(ai); }
            static Action* death_pact(PlayerbotAI* ai) { return new CastDeathPactAction(ai); }
        };
    };
};


DeathknightAiObjectContext::DeathknightAiObjectContext(PlayerbotAI* ai) : AiObjectContext(ai)
{
    strategyContexts.Add(new ai::deathknight::StrategyFactoryInternal());
    strategyContexts.Add(new ai::deathknight::CombatStrategyFactoryInternal());
    strategyContexts.Add(new ai::deathknight::BuffStrategyFactoryInternal());
    actionContexts.Add(new ai::deathknight::AiObjectContextInternal());
    triggerContexts.Add(new ai::deathknight::TriggerFactoryInternal());
}
