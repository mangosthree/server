#pragma once

#include "../actions/GenericActions.h"

namespace ai
{
    /// Cata 4.3.4: Lesser Healing Wave was renamed Healing Surge
    class CastHealingSurgeAction : public CastHealingSpellAction {
    public:
        CastHealingSurgeAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "healing surge") {}
    };

    class CastHealingSurgeOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastHealingSurgeOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "healing surge") {}
    };


    class CastHealingWaveAction : public CastHealingSpellAction {
    public:
        CastHealingWaveAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "healing wave") {}
    };

    class CastHealingWaveOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastHealingWaveOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "healing wave") {}
    };

    /// Cata 4.3.4: big, slow single-target heal (77472); pairs with Tidal Waves.
    class CastGreaterHealingWaveAction : public CastHealingSpellAction {
    public:
        CastGreaterHealingWaveAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "greater healing wave") {}
    };

    class CastGreaterHealingWaveOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastGreaterHealingWaveOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "greater healing wave") {}
    };

    class CastChainHealAction : public CastAoeHealSpellAction {
    public:
        CastChainHealAction(PlayerbotAI* ai) : CastAoeHealSpellAction(ai, "chain heal") {}
    };

    class CastRiptideAction : public CastHealingSpellAction {
    public:
        CastRiptideAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "riptide") {}
    };

    class CastRiptideOnPartyAction : public HealPartyMemberAction
    {
    public:
        CastRiptideOnPartyAction(PlayerbotAI* ai) : HealPartyMemberAction(ai, "riptide") {}
    };


    class CastEarthShieldAction : public CastBuffSpellAction {
    public:
        CastEarthShieldAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "earth shield") {}
    };

    class CastEarthShieldOnPartyAction : public BuffOnPartyAction
    {
    public:
        CastEarthShieldOnPartyAction(PlayerbotAI* ai) : BuffOnPartyAction(ai, "earth shield") {}
    };

    /// Maintains Earth Shield on the main tank rather than a rotating party member.
    class CastEarthShieldOnTankAction : public CastBuffSpellAction
    {
    public:
        CastEarthShieldOnTankAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "earth shield") {}
        virtual Value<Unit*>* GetTargetValue() { return context->GetValue<Unit*>("party tank"); }
        virtual string getName() { return "earth shield on tank"; }
    };

    class CastHealingRainAction : public CastAoeHealSpellAction {
    public:
        CastHealingRainAction(PlayerbotAI* ai) : CastAoeHealSpellAction(ai, "healing rain") {}
    };

    class CastWaterShieldAction : public CastBuffSpellAction {
    public:
        CastWaterShieldAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "water shield") {}
    };

    class CastLightningShieldAction : public CastBuffSpellAction {
    public:
        CastLightningShieldAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "lightning shield") {}
    };

    class CastEarthlivingWeaponAction : public CastEnchantItemAction {
    public:
        CastEarthlivingWeaponAction(PlayerbotAI* ai) : CastEnchantItemAction(ai, "earthliving weapon") {}
    };

    class CastRockbiterWeaponAction : public CastEnchantItemAction {
    public:
        CastRockbiterWeaponAction(PlayerbotAI* ai) : CastEnchantItemAction(ai, "rockbiter weapon") {}
    };

    class CastFlametongueWeaponAction : public CastEnchantItemAction {
    public:
        CastFlametongueWeaponAction(PlayerbotAI* ai) : CastEnchantItemAction(ai, "flametongue weapon") {}
    };

    class CastFrostbrandWeaponAction : public CastEnchantItemAction {
    public:
        CastFrostbrandWeaponAction(PlayerbotAI* ai) : CastEnchantItemAction(ai, "frostbrand weapon") {}
    };

    class CastWindfuryWeaponAction : public CastEnchantItemAction {
    public:
        CastWindfuryWeaponAction(PlayerbotAI* ai) : CastEnchantItemAction(ai, "windfury weapon") {}
    };

    class CastTotemAction : public CastBuffSpellAction
    {
    public:
        CastTotemAction(PlayerbotAI* ai, string spell) : CastBuffSpellAction(ai, spell) {}
        virtual bool isUseful() { return CastBuffSpellAction::isUseful() && !AI_VALUE2(bool, "has totem", name); }
    };

    class CastStoneskinTotemAction : public CastTotemAction
    {
    public:
        CastStoneskinTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "stoneskin totem") {}
    };

    class CastEarthbindTotemAction : public CastTotemAction
    {
    public:
        CastEarthbindTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "earthbind totem") {}
    };

    class CastStrengthOfEarthTotemAction : public CastTotemAction
    {
    public:
        CastStrengthOfEarthTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "strength of earth totem") {}
    };

    class CastManaSpringTotemAction : public CastTotemAction
    {
    public:
        CastManaSpringTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "mana spring totem") {}
        virtual bool isUseful() { return CastTotemAction::isUseful() && !AI_VALUE2(bool, "has totem", "healing stream totem"); }
    };

    class CastManaTideTotemAction : public CastTotemAction
    {
    public:
        CastManaTideTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "mana tide totem") {}
        virtual string GetTargetName() { return "self target"; }
    };

    class CastHealingStreamTotemAction : public CastTotemAction
    {
    public:
        CastHealingStreamTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "healing stream totem") {}
    };

    class CastFlametongueTotemAction : public CastTotemAction
    {
    public:
        CastFlametongueTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "flametongue totem") {}
        virtual bool isUseful() { return CastTotemAction::isUseful() && !AI_VALUE2(bool, "has totem", "magma totem"); }
    };

    class CastWindfuryTotemAction : public CastTotemAction
    {
    public:
        CastWindfuryTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "windfury totem") {}
    };

    class CastSearingTotemAction : public CastTotemAction
    {
    public:
        CastSearingTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "searing totem") {}
        virtual string GetTargetName() { return "self target"; }
        virtual bool isUseful() { return CastTotemAction::isUseful() && !AI_VALUE2(bool, "has totem", "flametongue totem"); }
    };

    class CastMagmaTotemAction : public CastMeleeSpellAction
    {
    public:
        CastMagmaTotemAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "magma totem") {}
        virtual string GetTargetName() { return "self target"; }
        virtual bool isUseful() { return CastMeleeSpellAction::isUseful() && !AI_VALUE2(bool, "has totem", name); }
    };

    class CastFireNovaAction : public CastSpellAction {
    public:
        CastFireNovaAction(PlayerbotAI* ai) : CastSpellAction(ai, "fire nova") {}
    };

    class CastWindShearAction : public CastSpellAction {
    public:
        CastWindShearAction(PlayerbotAI* ai) : CastSpellAction(ai, "wind shear") {}
    };

    class CastAncestralSpiritAction : public ResurrectPartyMemberAction
    {
    public:
        CastAncestralSpiritAction(PlayerbotAI* ai) : ResurrectPartyMemberAction(ai, "ancestral spirit") {}
    };


    class CastPurgeAction : public CastSpellAction
    {
    public:
        CastPurgeAction(PlayerbotAI* ai) : CastSpellAction(ai, "purge") {}
    };

    class CastStormstrikeAction : public CastMeleeSpellAction {
    public:
        CastStormstrikeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "stormstrike") {}
    };

    class CastLavaLashAction : public CastMeleeSpellAction {
    public:
        CastLavaLashAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "lava lash") {}
    };

    class CastWaterBreathingAction : public CastBuffSpellAction {
    public:
        CastWaterBreathingAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "water breathing") {}
    };

    class CastWaterWalkingAction : public CastBuffSpellAction {
    public:
        CastWaterWalkingAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "water walking") {}
    };

    class CastWaterBreathingOnPartyAction : public BuffOnPartyAction {
    public:
        CastWaterBreathingOnPartyAction(PlayerbotAI* ai) : BuffOnPartyAction(ai, "water breathing") {}
    };

    class CastWaterWalkingOnPartyAction : public BuffOnPartyAction {
    public:
        CastWaterWalkingOnPartyAction(PlayerbotAI* ai) : BuffOnPartyAction(ai, "water walking") {}
    };


    class CastCleanseSpiritAction : public CastCureSpellAction {
    public:
        CastCleanseSpiritAction(PlayerbotAI* ai) : CastCureSpellAction(ai, "cleanse spirit") {}
    };

    class CastCleanseSpiritCurseOnPartyAction : public CurePartyMemberAction
    {
    public:
        CastCleanseSpiritCurseOnPartyAction(PlayerbotAI* ai) : CurePartyMemberAction(ai, "cleanse spirit", DISPEL_CURSE) {}

        virtual string getName() { return "cleanse spirit curse on party"; }
    };

    class CastFlameShockAction : public CastDebuffSpellAction
    {
    public:
        CastFlameShockAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "flame shock") {}
    };

    class CastEarthShockAction : public CastDebuffSpellAction
    {
    public:
        CastEarthShockAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "earth shock") {}
    };

    class CastFrostShockAction : public CastSnareSpellAction
    {
    public:
        CastFrostShockAction(PlayerbotAI* ai) : CastSnareSpellAction(ai, "frost shock") {}
    };

    class CastChainLightningAction : public CastSpellAction
    {
    public:
        CastChainLightningAction(PlayerbotAI* ai) : CastSpellAction(ai, "chain lightning") {}
    };

    class CastLightningBoltAction : public CastSpellAction
    {
    public:
        CastLightningBoltAction(PlayerbotAI* ai) : CastSpellAction(ai, "lightning bolt") {}
    };

    class CastLavaBurstAction : public CastSpellAction
    {
    public:
        CastLavaBurstAction(PlayerbotAI* ai) : CastSpellAction(ai, "lava burst") {}
    };

    class CastThunderstormAction : public CastMeleeSpellAction
    {
    public:
        CastThunderstormAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "thunderstorm") {}
    };

    class CastHeroismAction : public CastBuffSpellAction
    {
    public:
        CastHeroismAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "heroism") {}
    };

    class CastBloodlustAction : public CastBuffSpellAction
    {
    public:
        CastBloodlustAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "bloodlust") {}
    };

    class CastWindShearOnEnemyHealerAction : public CastSpellOnEnemyHealerAction
    {
    public:
        CastWindShearOnEnemyHealerAction(PlayerbotAI* ai) : CastSpellOnEnemyHealerAction(ai, "wind shear") {}
    };

    class CastUnleashElementsAction : public CastBuffSpellAction
    {
    public:
        CastUnleashElementsAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "unleash elements") {}
    };

    class CastFeralSpiritAction : public CastBuffSpellAction
    {
    public:
        CastFeralSpiritAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "feral spirit") {}
    };

    /// Elemental cooldown pet totem (2894).
    class CastFireElementalTotemAction : public CastTotemAction
    {
    public:
        CastFireElementalTotemAction(PlayerbotAI* ai) : CastTotemAction(ai, "fire elemental totem") {}
    };

}
