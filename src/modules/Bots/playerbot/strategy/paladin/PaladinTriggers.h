#pragma once
#include "../triggers/GenericTriggers.h"

namespace ai
{
    BUFF_TRIGGER(HolyShieldTrigger, "holy shield", "holy shield")
    BUFF_TRIGGER(RighteousFuryTrigger, "righteous fury", "righteous fury")

    BUFF_TRIGGER(RetributionAuraTrigger, "retribution aura", "retribution aura")

    class CrusaderAuraTrigger : public BuffTrigger
    {
    public:
        CrusaderAuraTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "crusader aura") {}
        virtual bool IsActive();
    };

    class SealTrigger : public BuffTrigger
    {
    public:
        SealTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "seal of justice") {}
        virtual bool IsActive();
    };

    // Cata 4.3.4: single Judgement; no per-buff debuff remains, so this fires
    // whenever the spell is ready (CanCastSpell handles the cooldown)
    DEBUFF_TRIGGER(JudgementTrigger, "judgement", "judgement")

    class BlessingOnPartyTrigger : public BuffOnPartyTrigger
    {
    public:
        BlessingOnPartyTrigger(PlayerbotAI* ai) : BuffOnPartyTrigger(ai, "blessing of kings", 7) {}
    };

    class BlessingTrigger : public BuffTrigger
    {
    public:
        BlessingTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "blessing of kings", 5) {}
    };

    class HammerOfJusticeInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        HammerOfJusticeInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "hammer of justice") {}
    };

    class RebukeInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        RebukeInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "rebuke") {}
    };

    /// Cata 4.3.4 paladin resource; modeled on ComboPointsAvailableTrigger but the
    /// stat lives on the bot itself, not on the current target
    class HolyPowerAvailableTrigger : public StatAvailable
    {
    public:
        HolyPowerAvailableTrigger(PlayerbotAI* ai, int amount = 3) : StatAvailable(ai, amount, "holy power available") {}
        virtual bool IsActive();
    };

    /// Maintains Beacon of Light on the tank instead of rotating party members
    class BeaconOfLightOnTankTrigger : public BuffTrigger
    {
    public:
        BeaconOfLightOnTankTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "beacon of light") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "beacon of light on tank"; }
    };

    class AvengingWrathTrigger : public BoostTrigger
    {
    public:
        AvengingWrathTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "avenging wrath") {}
    };

    class HammerOfJusticeSnareTrigger : public SnareTargetTrigger
    {
    public:
        HammerOfJusticeSnareTrigger(PlayerbotAI* ai) : SnareTargetTrigger(ai, "hammer of justice") {}
    };

    // runtime-verify: Art of War proc id 59578 (free instant Exorcism). The Ret
    // talent that enables it is a passive sharing this display name, so a
    // name-based HasAuraTrigger would fire every tick (Exorcism spam / stall).
    class ArtOfWarTrigger : public HasAuraIdTrigger
    {
    public:
        ArtOfWarTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "art of war", 59578) {}
    };

    // runtime-verify: Divine Purpose proc id 90174 (free Holy Power finisher)
    class DivinePurposeTrigger : public HasAuraIdTrigger
    {
    public:
        DivinePurposeTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "divine purpose", 90174) {}
    };

    // runtime-verify: Avenging Wrath aura id 31884. Fires the AW-paired burst
    // cooldowns while the wings are up; the boost trigger deactivates once AW is
    // active, so a separate active-aura trigger is needed to queue them.
    class AvengingWrathActiveTrigger : public HasAuraIdTrigger
    {
    public:
        AvengingWrathActiveTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "avenging wrath active", 31884) {}
    };

    class CrusaderStrikeAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        CrusaderStrikeAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "crusader strike") {}
    };

    // Covers Grand Crusader too: the proc resets Avenger's Shield's cooldown
    // server-side, so "can it be cast" already captures the free-cast window.
    class AvengersShieldAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        AvengersShieldAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "avenger's shield") {}
    };

    // runtime-verify: Daybreak proc id 88819 (free/instant Holy Shock follow-up)
    class DaybreakTrigger : public HasAuraIdTrigger
    {
    public:
        DaybreakTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "daybreak", 88819) {}
    };

    class ResistanceAuraTrigger : public BuffTrigger
    {
    public:
        ResistanceAuraTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "resistance aura") {}
    };

    class DevotionAuraTrigger : public BuffTrigger
    {
    public:
        DevotionAuraTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "devotion aura") {}
    };

    class CleanseCureDiseaseTrigger : public NeedCureTrigger
    {
    public:
        CleanseCureDiseaseTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "cleanse", DISPEL_DISEASE) {}
    };

    class CleanseCurePartyMemberDiseaseTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        CleanseCurePartyMemberDiseaseTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "cleanse", DISPEL_DISEASE) {}
    };

    class CleanseCurePoisonTrigger : public NeedCureTrigger
    {
    public:
        CleanseCurePoisonTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "cleanse", DISPEL_POISON) {}
    };

    class CleanseCurePartyMemberPoisonTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        CleanseCurePartyMemberPoisonTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "cleanse", DISPEL_POISON) {}
    };

    class CleanseCureMagicTrigger : public NeedCureTrigger
    {
    public:
        CleanseCureMagicTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "cleanse", DISPEL_MAGIC) {}
    };

    class CleanseCurePartyMemberMagicTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        CleanseCurePartyMemberMagicTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "cleanse", DISPEL_MAGIC) {}
    };

    class HammerOfJusticeEnemyHealerTrigger : public InterruptEnemyHealerTrigger
    {
    public:
        HammerOfJusticeEnemyHealerTrigger(PlayerbotAI* ai) : InterruptEnemyHealerTrigger(ai, "hammer of justice") {}
    };

    /// Cata 4.3.4: Holy Wrath hits all nearby enemies (only its stun is undead/demon-limited)
    class HolyWrathTrigger : public Trigger
    {
    public:
        HolyWrathTrigger(PlayerbotAI* ai) : Trigger(ai, "holy wrath") {}
        virtual bool IsActive();
    };

    /// Fires when the bot itself is rooted or snared, to self-cast Blessing of Freedom
    class BlessingOfFreedomTrigger : public Trigger
    {
    public:
        BlessingOfFreedomTrigger(PlayerbotAI* ai) : Trigger(ai, "blessing of freedom") {}
        virtual bool IsActive();
    };
}
