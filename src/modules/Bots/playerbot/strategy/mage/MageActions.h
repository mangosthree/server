#pragma once

#include "../actions/GenericActions.h"

namespace ai
{
    class CastFireballAction : public CastSpellAction
    {
    public:
        CastFireballAction(PlayerbotAI* ai) : CastSpellAction(ai, "fireball") {}
    };

    class CastScorchAction : public CastSpellAction
    {
    public:
        CastScorchAction(PlayerbotAI* ai) : CastSpellAction(ai, "scorch") {}
    };

    class CastFireBlastAction : public CastSpellAction
    {
    public:
        CastFireBlastAction(PlayerbotAI* ai) : CastSpellAction(ai, "fire blast") {}
    };

    class CastArcaneBlastAction : public CastBuffSpellAction
    {
    public:
        CastArcaneBlastAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "arcane blast") {}
        virtual string GetTargetName() { return "current target"; }
    };

    class CastArcaneBarrageAction : public CastSpellAction
    {
    public:
        CastArcaneBarrageAction(PlayerbotAI* ai) : CastSpellAction(ai, "arcane barrage") {}
    };

    class CastArcaneMissilesAction : public CastSpellAction
    {
    public:
        CastArcaneMissilesAction(PlayerbotAI* ai) : CastSpellAction(ai, "arcane missiles") {}
    };

    class CastPyroblastAction : public CastSpellAction
    {
    public:
        CastPyroblastAction(PlayerbotAI* ai) : CastSpellAction(ai, "pyroblast") {}
    };

    class CastFlamestrikeAction : public CastSpellAction
    {
    public:
        CastFlamestrikeAction(PlayerbotAI* ai) : CastSpellAction(ai, "flamestrike") {}
        virtual bool isUseful();
    };

    class CastFrostNovaAction : public CastSpellAction
    {
    public:
        CastFrostNovaAction(PlayerbotAI* ai) : CastSpellAction(ai, "frost nova") {}
        virtual bool isUseful();
    };

    class CastFrostboltAction : public CastSpellAction
    {
    public:
        CastFrostboltAction(PlayerbotAI* ai) : CastSpellAction(ai, "frostbolt") {}
    };

    class CastBlizzardAction : public CastSpellAction
    {
    public:
        CastBlizzardAction(PlayerbotAI* ai) : CastSpellAction(ai, "blizzard") {}
        virtual bool isUseful();
    };

    class CastIceLanceAction : public CastSpellAction
    {
    public:
        CastIceLanceAction(PlayerbotAI* ai) : CastSpellAction(ai, "ice lance") {}
    };

    class CastFrostfireBoltAction : public CastSpellAction
    {
    public:
        CastFrostfireBoltAction(PlayerbotAI* ai) : CastSpellAction(ai, "frostfire bolt") {}
    };

    /// Talent-gated finisher; requires the target already frozen (Fingers of
    /// Frost/Frost Nova). isPossible() no-ops for non-Frost via HasSpell.
    class CastDeepFreezeAction : public CastSpellAction
    {
    public:
        CastDeepFreezeAction(PlayerbotAI* ai) : CastSpellAction(ai, "deep freeze") {}
    };

    /// Frost-only pet; isPossible() no-ops for Fire/Arcane via HasSpell.
    class CastSummonWaterElementalAction : public CastBuffSpellAction
    {
    public:
        CastSummonWaterElementalAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon water elemental") {}
    };

    class CastArcaneIntellectAction : public CastBuffSpellAction
    {
    public:
        CastArcaneIntellectAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "arcane intellect") {}
    };

    class CastArcaneIntellectOnPartyAction : public BuffOnPartyAction
    {
    public:
        CastArcaneIntellectOnPartyAction(PlayerbotAI* ai) : BuffOnPartyAction(ai, "arcane intellect") {}
    };

    class CastRemoveCurseAction : public CastCureSpellAction
    {
    public:
        CastRemoveCurseAction(PlayerbotAI* ai) : CastCureSpellAction(ai, "remove curse") {}
    };

    class CastIcyVeinsAction : public CastBuffSpellAction
    {
    public:
        CastIcyVeinsAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "icy veins") {}
    };

    class CastCombustionAction : public CastBuffSpellAction
    {
    public:
        CastCombustionAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "combustion") {}
    };

    BEGIN_SPELL_ACTION(CastCounterspellAction, "counterspell")
    END_SPELL_ACTION()

    class CastRemoveCurseOnPartyAction : public CurePartyMemberAction
    {
    public:
        CastRemoveCurseOnPartyAction(PlayerbotAI* ai) : CurePartyMemberAction(ai, "remove curse", DISPEL_CURSE) {}
    };

    /// Cata 4.3.4: Conjure Water/Food were merged into Conjure Refreshment
    class CastConjureRefreshmentAction : public CastBuffSpellAction
    {
    public:
        CastConjureRefreshmentAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "conjure refreshment") {}

        /// Don't re-conjure while already stocked -- Conjure Refreshment
        /// creates an item, not an aura, so the base "already has aura"
        /// check in CastAuraSpellAction::isUseful() never gates it.
        virtual bool isUseful() { return CastBuffSpellAction::isUseful() && AI_VALUE2(list<Item*>, "inventory items", "conjured food").empty(); }
    };

    class CastIceBlockAction : public CastBuffSpellAction
    {
    public:
        CastIceBlockAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "ice block") {}
    };

    class CastMoltenArmorAction : public CastBuffSpellAction
    {
    public:
        CastMoltenArmorAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "molten armor") {}
    };

    class CastMageArmorAction : public CastBuffSpellAction
    {
    public:
        CastMageArmorAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "mage armor") {}
    };

    class CastIceArmorAction : public CastBuffSpellAction
    {
    public:
        CastIceArmorAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "ice armor") {}
    };

    class CastFrostArmorAction : public CastBuffSpellAction
    {
    public:
        CastFrostArmorAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "frost armor") {}
    };

    class CastPolymorphAction : public CastBuffSpellAction
    {
    public:
        CastPolymorphAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "polymorph") {}
        virtual Value<Unit*>* GetTargetValue();
    };

    class CastSpellstealAction : public CastSpellAction
    {
    public:
        CastSpellstealAction(PlayerbotAI* ai) : CastSpellAction(ai, "spellsteal") {}
    };

    class CastLivingBombAction : public CastDebuffSpellAction
    {
    public:
        CastLivingBombAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "living bomb") {}
    };

    class CastDragonsBreathAction : public CastSpellAction
    {
    public:
        CastDragonsBreathAction(PlayerbotAI* ai) : CastSpellAction(ai, "dragon's breath") {}
    };

    class CastBlastWaveAction : public CastSpellAction
    {
    public:
        CastBlastWaveAction(PlayerbotAI* ai) : CastSpellAction(ai, "blast wave") {}
        virtual bool isUseful();
    };

    class CastInvisibilityAction : public CastBuffSpellAction
    {
    public:
        CastInvisibilityAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "invisibility") {}
    };

    class CastEvocationAction : public CastSpellAction
    {
    public:
        CastEvocationAction(PlayerbotAI* ai) : CastSpellAction(ai, "evocation") {}
        virtual string GetTargetName() { return "self target"; }
    };

    class CastCounterspellOnEnemyHealerAction : public CastSpellOnEnemyHealerAction
    {
    public:
        CastCounterspellOnEnemyHealerAction(PlayerbotAI* ai) : CastSpellOnEnemyHealerAction(ai, "counterspell") {}
    };

    /// Burn-opener cooldown; self buff. runtime-verify "arcane power" resolves.
    class CastArcanePowerAction : public CastBuffSpellAction
    {
    public:
        CastArcanePowerAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "arcane power") {}
    };

    /// Shared Fire/Frost/Arcane cooldown filler (82731); tauri ranks it #2 in
    /// Fire ST. runtime-verify "flame orb" resolves.
    class CastFlameOrbAction : public CastSpellAction
    {
    public:
        CastFlameOrbAction(PlayerbotAI* ai) : CastSpellAction(ai, "flame orb") {}
    };
}
