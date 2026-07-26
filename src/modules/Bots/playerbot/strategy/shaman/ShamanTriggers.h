#pragma once
#include "../triggers/GenericTriggers.h"

namespace ai
{
    class ShamanWeaponTrigger : public BuffTrigger {
    public:
        ShamanWeaponTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "rockbiter weapon") {}
        virtual bool IsActive();
    private:
        static list<string> spells;
    };

    class TotemTrigger : public Trigger {
    public:
        TotemTrigger(PlayerbotAI* ai, string spell, int attackerCount = 0) : Trigger(ai, spell), attackerCount(attackerCount) {}

        virtual bool IsActive()
        {
            return AI_VALUE(uint8, "attacker count") >= attackerCount && !AI_VALUE2(bool, "has totem", name);
        }

    protected:
        int attackerCount;
    };

    class WindfuryTotemTrigger : public TotemTrigger {
    public:
        WindfuryTotemTrigger(PlayerbotAI* ai) : TotemTrigger(ai, "windfury totem") {}
    };

    class ManaSpringTotemTrigger : public TotemTrigger {
    public:
        ManaSpringTotemTrigger(PlayerbotAI* ai) : TotemTrigger(ai, "mana spring totem") {}
        virtual bool IsActive()
        {
            return AI_VALUE(uint8, "attacker count") >= attackerCount &&
                    !AI_VALUE2(bool, "has totem", "mana tide totem") &&
                    !AI_VALUE2(bool, "has totem", name);
        }
    };

    class FlametongueTotemTrigger : public TotemTrigger {
    public:
        FlametongueTotemTrigger(PlayerbotAI* ai) : TotemTrigger(ai, "flametongue totem") {}
    };

    class StrengthOfEarthTotemTrigger : public TotemTrigger {
    public:
        StrengthOfEarthTotemTrigger(PlayerbotAI* ai) : TotemTrigger(ai, "strength of earth totem") {}
    };

    class MagmaTotemTrigger : public TotemTrigger {
    public:
        MagmaTotemTrigger(PlayerbotAI* ai) : TotemTrigger(ai, "magma totem", 3) {}
    };

    class SearingTotemTrigger : public TotemTrigger {
    public:
        SearingTotemTrigger(PlayerbotAI* ai) : TotemTrigger(ai, "searing totem", 1) {}
    };

    class WindShearInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        WindShearInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "wind shear") {}
    };

    class WaterShieldTrigger : public BuffTrigger
    {
    public:
        WaterShieldTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "water shield") {}
    };

    class LightningShieldTrigger : public BuffTrigger
    {
    public:
        LightningShieldTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "lightning shield") {}
    };

    class PurgeTrigger : public TargetAuraDispelTrigger
    {
    public:
        PurgeTrigger(PlayerbotAI* ai) : TargetAuraDispelTrigger(ai, "purge", DISPEL_MAGIC) {}
    };

    class WaterWalkingTrigger : public BuffTrigger {
    public:
        WaterWalkingTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "water walking", 7) {}

        virtual bool IsActive()
        {
            return BuffTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
        }
    };

    class WaterBreathingTrigger : public BuffTrigger {
    public:
        WaterBreathingTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "water breathing", 5) {}

        virtual bool IsActive()
        {
            return BuffTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
        }
    };

    class WaterWalkingOnPartyTrigger : public BuffOnPartyTrigger {
    public:
        WaterWalkingOnPartyTrigger(PlayerbotAI* ai) : BuffOnPartyTrigger(ai, "water walking on party", 7) {}

        virtual bool IsActive()
        {
            return BuffOnPartyTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
        }
    };

    class WaterBreathingOnPartyTrigger : public BuffOnPartyTrigger {
    public:
        WaterBreathingOnPartyTrigger(PlayerbotAI* ai) : BuffOnPartyTrigger(ai, "water breathing on party", 5) {}

        virtual bool IsActive()
        {
            return BuffOnPartyTrigger::IsActive() && AI_VALUE2(bool, "swimming", "self target");
        }
    };

    class CleanseSpiritCurseTrigger : public NeedCureTrigger
    {
    public:
        CleanseSpiritCurseTrigger(PlayerbotAI* ai) : NeedCureTrigger(ai, "cleanse spirit", DISPEL_CURSE) {}
    };

    class PartyMemberCleanseSpiritCurseTrigger : public PartyMemberNeedCureTrigger
    {
    public:
        PartyMemberCleanseSpiritCurseTrigger(PlayerbotAI* ai) : PartyMemberNeedCureTrigger(ai, "cleanse spirit", DISPEL_CURSE) {}
    };


    /// Flame Shock DoT upkeep: fires while the target lacks the debuff.
    class ShockTrigger : public DebuffTrigger {
    public:
        ShockTrigger(PlayerbotAI* ai) : DebuffTrigger(ai, "earth shock") {}
        virtual bool IsActive();
    };

    /// Earth Shock instant filler/nuke: independent of Flame Shock upkeep.
    class EarthShockReadyTrigger : public SpellCanBeCastTrigger
    {
    public:
        EarthShockReadyTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "earth shock") {}
    };

    /// Guaranteed crit vs a target already carrying Flame Shock.
    class LavaBurstTrigger : public SpellCanBeCastTrigger
    {
    public:
        LavaBurstTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "lava burst") {}
        virtual bool IsActive();
    };

    /// Fire Nova detonates Flame Shock; only useful once the DoT is up.
    class FireNovaTrigger : public SpellCanBeCastTrigger
    {
    public:
        FireNovaTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "fire nova") {}
        virtual bool IsActive();
    };

    class FrostShockSnareTrigger : public SnareTargetTrigger {
    public:
        FrostShockSnareTrigger(PlayerbotAI* ai) : SnareTargetTrigger(ai, "frost shock") {}
    };

    class HeroismTrigger : public BoostTrigger
    {
    public:
        HeroismTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "heroism") {}
    };

    class BloodlustTrigger : public BoostTrigger
    {
    public:
        BloodlustTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "bloodlust") {}
    };

    /// Fires only at 5 stacks, when the next cast is fully instant.
    class MaelstromWeaponTrigger : public HasAuraTrigger
    {
    public:
        MaelstromWeaponTrigger(PlayerbotAI* ai) : HasAuraTrigger(ai, "maelstrom weapon") {}
        virtual bool IsActive();
    };

    class WindShearInterruptEnemyHealerSpellTrigger : public InterruptEnemyHealerTrigger
    {
    public:
        WindShearInterruptEnemyHealerSpellTrigger(PlayerbotAI* ai) : InterruptEnemyHealerTrigger(ai, "wind shear") {}
    };

    class UnleashElementsTrigger : public SpellCanBeCastTrigger
    {
    public:
        UnleashElementsTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "unleash elements") {}
        virtual string GetTargetName() { return "self target"; }
    };

    class FeralSpiritTrigger : public BoostTrigger
    {
    public:
        FeralSpiritTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "feral spirit") {}
    };

    /// Maintains Earth Shield on the main tank rather than a rotating party member.
    class EarthShieldOnTankTrigger : public BuffTrigger
    {
    public:
        EarthShieldOnTankTrigger(PlayerbotAI* ai) : BuffTrigger(ai, "earth shield") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "earth shield on tank"; }
    };

}
