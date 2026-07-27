#pragma once
#include "../actions/GenericActions.h"

namespace ai
{
    //-----------------------------------------------------------------------
    // Presences
    //-----------------------------------------------------------------------

    class CastBloodPresenceAction : public CastBuffSpellAction
    {
    public:
        CastBloodPresenceAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "blood presence") {}
    };

    class CastUnholyPresenceAction : public CastBuffSpellAction
    {
    public:
        CastUnholyPresenceAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "unholy presence") {}
    };

    class CastFrostPresenceAction : public CastBuffSpellAction
    {
    public:
        CastFrostPresenceAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "frost presence") {}
    };

    //-----------------------------------------------------------------------
    // Disease application / spread
    //-----------------------------------------------------------------------

    class CastIcyTouchAction : public CastDebuffSpellAction
    {
    public:
        CastIcyTouchAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "icy touch") {}
    };

    class CastPlagueStrikeAction : public CastMeleeSpellAction
    {
    public:
        CastPlagueStrikeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "plague strike") {}
    };

    /// Applies both Frost Fever and Blood Plague in one cast
    class CastOutbreakAction : public CastDebuffSpellAction
    {
    public:
        CastOutbreakAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "outbreak") {}
    };

    class CastPestilenceAction : public CastSpellAction
    {
    public:
        CastPestilenceAction(PlayerbotAI* ai) : CastSpellAction(ai, "pestilence") {}
    };

    //-----------------------------------------------------------------------
    // Single-target strikes
    //-----------------------------------------------------------------------

    /// Frost/Unholy(/Death) rune spender; the core's rune-cost check
    /// (Spell::CheckRuneCost, via CanCastSpell) already handles Death rune
    /// wildcards, so no manual resource gate is needed here
    class CastDeathStrikeAction : public CastMeleeSpellAction
    {
    public:
        CastDeathStrikeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "death strike") {}
    };

    /// Blood rune spender; must NOT be allowed to burn a Death rune reserved
    /// for Death Strike, so only fire when a real Blood rune is up
    class CastHeartStrikeAction : public CastMeleeSpellAction
    {
    public:
        CastHeartStrikeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "heart strike") {}
        virtual bool isPossible()
        {
            if (AI_VALUE2(uint8, "rune count", "blood") < 1)
            {
                return false;
            }
            return CastMeleeSpellAction::isPossible();
        }
    };

    class CastObliterateAction : public CastMeleeSpellAction
    {
    public:
        CastObliterateAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "obliterate") {}
    };

    class CastFrostStrikeAction : public CastMeleeSpellAction
    {
    public:
        CastFrostStrikeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "frost strike") {}
    };

    /// Cata 4.3.4: auto-applies Frost Fever; proc'd free via Rime
    class CastHowlingBlastAction : public CastSpellAction
    {
    public:
        CastHowlingBlastAction(PlayerbotAI* ai) : CastSpellAction(ai, "howling blast") {}
    };

    class CastScourgeStrikeAction : public CastMeleeSpellAction
    {
    public:
        CastScourgeStrikeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "scourge strike") {}
    };

    /// Requires a real Blood rune AND a real Frost rune - both wildcard-costed
    /// slots, so gate manually the same way as Heart Strike
    class CastFesteringStrikeAction : public CastMeleeSpellAction
    {
    public:
        CastFesteringStrikeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "festering strike") {}
        virtual bool isPossible()
        {
            if (AI_VALUE2(uint8, "rune count", "blood") < 1 || AI_VALUE2(uint8, "rune count", "frost") < 1)
            {
                return false;
            }
            return CastMeleeSpellAction::isPossible();
        }
    };

    /// Runic Power dump; only worth it once a same-type rune pair is
    /// depleted (otherwise the rotation should keep spending runes instead)
    class CastRuneStrikeAction : public CastMeleeSpellAction
    {
    public:
        CastRuneStrikeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "rune strike") {}
        virtual bool isPossible()
        {
            if (AI_VALUE2(uint8, "runic power", "self target") < 30)
            {
                return false;
            }
            if (!bot->IsBaseRuneSlotsOnCooldown(RUNE_BLOOD) &&
                !bot->IsBaseRuneSlotsOnCooldown(RUNE_FROST) &&
                !bot->IsBaseRuneSlotsOnCooldown(RUNE_UNHOLY))
            {
                return false;
            }
            return CastMeleeSpellAction::isPossible();
        }
    };

    /// Runic Power dump; free/instant on the Sudden Doom proc, bypassing the
    /// normal 40 Runic Power spend gate (mirrors the paladin Divine Purpose
    /// bypass pattern)
    class CastDeathCoilAction : public CastSpellAction
    {
    public:
        CastDeathCoilAction(PlayerbotAI* ai) : CastSpellAction(ai, "death coil") {}
        virtual bool isPossible()
        {
            // runtime-verify: Sudden Doom proc 81340
            if (AI_VALUE2(uint8, "runic power", "self target") < 40 && !ai->HasAura(81340u, bot))
            {
                return false;
            }
            return CastSpellAction::isPossible();
        }
    };

    //-----------------------------------------------------------------------
    // AoE
    //-----------------------------------------------------------------------

    /// Ground-targeted; keep this out of single-target nodes
    class CastDeathAndDecayAction : public CastSpellAction
    {
    public:
        CastDeathAndDecayAction(PlayerbotAI* ai) : CastSpellAction(ai, "death and decay") {}
    };

    class CastBloodBoilAction : public CastSpellAction
    {
    public:
        CastBloodBoilAction(PlayerbotAI* ai) : CastSpellAction(ai, "blood boil") {}
    };

    //-----------------------------------------------------------------------
    // Utility / mobility / CC
    //-----------------------------------------------------------------------

    class CastDeathGripAction : public CastSpellAction
    {
    public:
        CastDeathGripAction(PlayerbotAI* ai) : CastSpellAction(ai, "death grip") {}
    };

    class CastDarkCommandAction : public CastSpellAction
    {
    public:
        CastDarkCommandAction(PlayerbotAI* ai) : CastSpellAction(ai, "dark command") {}
    };

    class CastChainsOfIceAction : public CastSnareSpellAction
    {
    public:
        CastChainsOfIceAction(PlayerbotAI* ai) : CastSnareSpellAction(ai, "chains of ice") {}
    };

    class CastMindFreezeAction : public CastSpellAction
    {
    public:
        CastMindFreezeAction(PlayerbotAI* ai) : CastSpellAction(ai, "mind freeze") {}
    };

    class CastMindFreezeOnEnemyHealerAction : public CastSpellOnEnemyHealerAction
    {
    public:
        CastMindFreezeOnEnemyHealerAction(PlayerbotAI* ai) : CastSpellOnEnemyHealerAction(ai, "mind freeze") {}
    };

    //-----------------------------------------------------------------------
    // Self-buffs / cooldowns
    //-----------------------------------------------------------------------

    class CastHornOfWinterAction : public CastBuffSpellAction
    {
    public:
        CastHornOfWinterAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "horn of winter") {}
    };

    /// Blood-tree ability; not every spec has it learned, so gate on HasSpell
    /// rather than letting a always-false SpellCastUsefulValue mask the reason
    class CastBoneShieldAction : public CastBuffSpellAction
    {
    public:
        CastBoneShieldAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "bone shield") {}
        virtual bool isUseful()
        {
            return bot->HasSpell(49222) && CastBuffSpellAction::isUseful();
        }
    };

    class CastIceboundFortitudeAction : public CastBuffSpellAction
    {
    public:
        CastIceboundFortitudeAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "icebound fortitude") {}
    };

    class CastVampiricBloodAction : public CastBuffSpellAction
    {
    public:
        CastVampiricBloodAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "vampiric blood") {}
    };

    class CastAntiMagicShellAction : public CastBuffSpellAction
    {
    public:
        CastAntiMagicShellAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "anti-magic shell") {}
    };

    class CastPillarOfFrostAction : public CastBuffSpellAction
    {
    public:
        CastPillarOfFrostAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "pillar of frost") {}
    };

    class CastUnholyFrenzyAction : public CastBuffSpellAction
    {
    public:
        CastUnholyFrenzyAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "unholy frenzy") {}
    };

    class CastDancingRuneWeaponAction : public CastBuffSpellAction
    {
    public:
        CastDancingRuneWeaponAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "dancing rune weapon") {}
    };

    class CastLichborneAction : public CastBuffSpellAction
    {
    public:
        CastLichborneAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "lichborne") {}
    };

    /// Frost treats this as a combat cooldown as well as a pet summon
    class CastRaiseDeadAction : public CastBuffSpellAction
    {
    public:
        CastRaiseDeadAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "raise dead") {}
    };

    class CastSummonGargoyleAction : public CastBuffSpellAction
    {
    public:
        CastSummonGargoyleAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "summon gargoyle") {}
    };

    class CastDarkTransformationAction : public CastBuffSpellAction
    {
    public:
        CastDarkTransformationAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "dark transformation") {}
    };

    class CastArmyOfTheDeadAction : public CastBuffSpellAction
    {
    public:
        CastArmyOfTheDeadAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "army of the dead") {}
    };

    class CastEmpowerRuneWeaponAction : public CastBuffSpellAction
    {
    public:
        CastEmpowerRuneWeaponAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "empower rune weapon") {}
    };

    class CastBloodTapAction : public CastBuffSpellAction
    {
    public:
        CastBloodTapAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "blood tap") {}
    };

    //-----------------------------------------------------------------------
    // Self-heals
    //-----------------------------------------------------------------------

    class CastRuneTapAction : public CastHealingSpellAction
    {
    public:
        CastRuneTapAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "rune tap") {}
    };

    class CastDeathPactAction : public CastHealingSpellAction
    {
    public:
        CastDeathPactAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "death pact") {}
    };
}
