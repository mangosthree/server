#pragma once

#include "../triggers/GenericTriggers.h"

namespace ai
{
    class PowerWordFortitudeOnPartyTrigger : public BuffOnPartyTrigger {
    public:
        PowerWordFortitudeOnPartyTrigger(PlayerbotAI* ai) : BuffOnPartyTrigger(ai, "power word: fortitude", 7) {}
    };

    class PowerWordFortitudeTrigger : public BuffTrigger {
    public:
        PowerWordFortitudeTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "power word: fortitude", 5) {}
    };



    BUFF_TRIGGER(InnerFireTrigger, "inner fire", "inner fire")
    BUFF_TRIGGER(VampiricEmbraceTrigger, "vampiric embrace", "vampiric embrace")

    class PowerWordPainOnAttackerTrigger : public DebuffOnAttackerTrigger
    {
    public:
        PowerWordPainOnAttackerTrigger(PlayerbotAI* ai) : DebuffOnAttackerTrigger(ai, "shadow word: pain") {}
    };

    DEBUFF_TRIGGER(PowerWordPainTrigger, "shadow word: pain", "shadow word: pain")
    DEBUFF_TRIGGER(DevouringPlagueTrigger, "devouring plague", "devouring plague")
    DEBUFF_TRIGGER(VampiricTouchTrigger, "vampiric touch", "vampiric touch")

    class DispelMagicTrigger : public NeedCureTrigger
    {
    public:
        DispelMagicTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "dispel magic", DISPEL_MAGIC) {}
    };

    class DispelMagicPartyMemberTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        DispelMagicPartyMemberTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "dispel magic", DISPEL_MAGIC) {}
    };

    class CureDiseaseTrigger : public NeedCureTrigger
    {
    public:
        CureDiseaseTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "cure disease", DISPEL_DISEASE) {}
    };

    class PartyMemberCureDiseaseTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        PartyMemberCureDiseaseTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "cure disease", DISPEL_DISEASE) {}
    };

    class ShadowformTrigger : public BuffTrigger {
    public:
        ShadowformTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "shadowform") {}
        virtual bool IsActive() { return !ai->HasAura("shadowform", bot); }
    };

    /// Maintains Prayer of Mending on the tank instead of rotating party members
    class PrayerOfMendingOnTankTrigger : public BuffTrigger
    {
    public:
        PrayerOfMendingOnTankTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "prayer of mending") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "prayer of mending on tank"; }
    };

    /// Fires Pain Suppression / Guardian Spirit when the tank is critically low
    class TankCriticalHealthTrigger : public HealthInRangeTrigger
    {
    public:
        TankCriticalHealthTrigger(PlayerbotAI* ai) :
            HealthInRangeTrigger(ai, "tank critical health", sPlayerbotAIConfig.criticalHealth, 0) {}

        virtual string GetTargetName() { return "party tank"; }
    };
}
