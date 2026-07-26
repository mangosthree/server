#pragma once

#include "../triggers/GenericTriggers.h"

namespace ai
{
    BEGIN_TRIGGER(HunterNoStingsActiveTrigger, Trigger)
    END_TRIGGER()

    class HunterAspectOfTheHawkTrigger : public BuffTrigger
    {
    public:
        HunterAspectOfTheHawkTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "aspect of the hawk") {
            checkInterval = 1;
        }
    };

    class HunterAspectOfTheWildTrigger : public BuffTrigger
    {
    public:
        HunterAspectOfTheWildTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "aspect of the wild") {
            checkInterval = 1;
        }
    };

    class HunterAspectOfThePackTrigger : public BuffTrigger
    {
    public:
        HunterAspectOfThePackTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "aspect of the pack") {}
        virtual bool IsActive() {
            return BuffTrigger::IsActive() && !ai->HasAura("aspect of the cheetah", GetTarget());
        };
    };

    /// Cata: hunters run on focus; fires when there is enough for a spender
    class HighFocusTrigger : public StatAvailable
    {
    public:
        HighFocusTrigger(PlayerbotAI* ai) : StatAvailable(ai, 60, "high focus") {}
        virtual bool IsActive();
    };

    BEGIN_TRIGGER(HuntersPetDeadTrigger, Trigger)
    END_TRIGGER()

    BEGIN_TRIGGER(HuntersPetLowHealthTrigger, Trigger)
    END_TRIGGER()

    class BlackArrowTrigger : public DebuffTrigger
    {
    public:
        BlackArrowTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "black arrow") {}
    };

    class HuntersMarkTrigger : public DebuffTrigger
    {
    public:
        HuntersMarkTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "hunter's mark") {}
    };

    class FreezingTrapTrigger : public HasCcTargetTrigger
    {
    public:
        FreezingTrapTrigger(PlayerbotAI* ai) : HasCcTargetTrigger(ai, "freezing trap") {}
    };

    class RapidFireTrigger : public BoostTrigger
    {
    public:
        RapidFireTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "rapid fire") {}
    };

    class BestialWrathTrigger : public BoostTrigger
    {
    public:
        BestialWrathTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "bestial wrath") {}
    };

    class ChimeraShotAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        ChimeraShotAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "chimera shot") {}
    };

    class SilencingShotInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        SilencingShotInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "silencing shot") {}
    };

    /// Maintains Misdirection on the tank rather than a rotating party member
    class MisdirectionOnTankTrigger : public BuffTrigger
    {
    public:
        MisdirectionOnTankTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "misdirection") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("tank target"); }
        virtual string getName() { return "misdirection on tank"; }
    };

    class TrueshotAuraTrigger : public BuffTrigger
    {
    public:
        TrueshotAuraTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "trueshot aura") {}
    };

    class SerpentStingOnAttackerTrigger : public DebuffOnAttackerTrigger
    {
    public:
        SerpentStingOnAttackerTrigger(PlayerbotAI* ai) : DebuffOnAttackerTrigger(ai, "serpent sting") {}
    };


    class ConsussiveShotSnareTrigger : public SnareTargetTrigger
    {
    public:
        ConsussiveShotSnareTrigger(PlayerbotAI* ai) : SnareTargetTrigger(ai, "concussive shot") {}
    };

    /// Fires once a feigned bot has shed its attackers, so it can stand back up
    class FeignDeathTrigger : public Trigger
    {
    public:
        FeignDeathTrigger(PlayerbotAI* ai) : Trigger(ai, "has feign death", 1) {}
        virtual bool IsActive();
    };
}
