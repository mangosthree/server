#pragma once
#include "../triggers/GenericTriggers.h"

namespace ai
{
    BUFF_TRIGGER(BloodPresenceTrigger, "blood presence", "blood presence")
    BUFF_TRIGGER(UnholyPresenceTrigger, "unholy presence", "unholy presence")
    BUFF_TRIGGER(FrostPresenceTrigger, "frost presence", "frost presence")
    BUFF_TRIGGER(BoneShieldTrigger, "bone shield", "bone shield")
    BUFF_TRIGGER(HornOfWinterTrigger, "horn of winter", "horn of winter")

    // Cata 4.3.4: disease upkeep is name-based (whichever spell applied it),
    // not id-based - the core applies whichever spell id granted the aura and
    // both Icy Touch/Outbreak and Plague Strike/Outbreak share display names
    // with their disease auras
    DEBUFF_TRIGGER(FrostFeverTrigger, "frost fever", "frost fever")
    DEBUFF_TRIGGER(BloodPlagueTrigger, "blood plague", "blood plague")

    class MindFreezeInterruptSpellTrigger : public InterruptSpellTrigger
    {
    public:
        MindFreezeInterruptSpellTrigger(PlayerbotAI* ai) : InterruptSpellTrigger(ai, "mind freeze") {}
    };

    class MindFreezeEnemyHealerTrigger : public InterruptEnemyHealerTrigger
    {
    public:
        MindFreezeEnemyHealerTrigger(PlayerbotAI* ai) : InterruptEnemyHealerTrigger(ai, "mind freeze") {}
    };

    /// Cata 4.3.4 death knight resource; core stores runic power x10, the
    /// "runic power" value already normalizes that back to a 0-100 scale
    class RunicPowerAvailableTrigger : public StatAvailable
    {
    public:
        RunicPowerAvailableTrigger(PlayerbotAI* ai, int amount, string name) : StatAvailable(ai, amount, name) {}
        virtual bool IsActive();
    };

    /// RP about to cap (76+): dump it before it is wasted
    class RunicPowerCapTrigger : public RunicPowerAvailableTrigger
    {
    public:
        RunicPowerCapTrigger(PlayerbotAI* ai) : RunicPowerAvailableTrigger(ai, 76, "runic power available") {}
    };

    /// Coarse pre-filter for Rune Strike (30 RP floor); the action's own
    /// isPossible() adds the "a same-type rune pair is depleted" gate on top
    class RuneStrikeAvailableTrigger : public RunicPowerAvailableTrigger
    {
    public:
        RuneStrikeAvailableTrigger(PlayerbotAI* ai) : RunicPowerAvailableTrigger(ai, 30, "rune strike available") {}
    };

    /// True once any one of the three base rune pairs (Blood/Frost/Unholy) is
    /// fully on cooldown - "two runes of the same type depleted"
    class RunePairDepletedTrigger : public Trigger
    {
    public:
        RunePairDepletedTrigger(PlayerbotAI* ai) : Trigger(ai, "rune pair depleted") {}
        virtual bool IsActive();
    };

    /// True when a Frost, Unholy, or Death rune pair is both up - Obliterate's
    /// "two runes of the same type available" precondition
    class RunePairAvailableTrigger : public Trigger
    {
    public:
        RunePairAvailableTrigger(PlayerbotAI* ai) : Trigger(ai, "obliterate pair") {}
        virtual bool IsActive();
    };

    class DeathStrikeAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        DeathStrikeAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "death strike") {}
    };

    // runtime-verify: unused by the default rotation (Frost leans on "killing
    // machine" / "obliterate pair" instead); registered for parity/future use
    class ObliterateAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        ObliterateAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "obliterate") {}
    };

    class ScourgeStrikeAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        ScourgeStrikeAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "scourge strike") {}
    };

    // Not in the original per-ability list, but needed: Festering Strike's own
    // rune requirement (real Blood+Frost) is disjoint from Scourge Strike's
    // (Unholy/Death), so it needs its own independent castability gate rather
    // than piggy-backing on "scourge strike available"
    class FesteringStrikeAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        FesteringStrikeAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "festering strike") {}
    };

    class HowlingBlastAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        HowlingBlastAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "howling blast") {}
    };

    class DarkTransformationAvailableTrigger : public SpellCanBeCastTrigger
    {
    public:
        DarkTransformationAvailableTrigger(PlayerbotAI* ai) : SpellCanBeCastTrigger(ai, "dark transformation") {}
    };

    // runtime-verify: Killing Machine proc id 51124 (guaranteed crit, next
    // Obliterate/Frost Strike/Howling Blast)
    class KillingMachineTrigger : public HasAuraIdTrigger
    {
    public:
        KillingMachineTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "killing machine", 51124) {}
    };

    // runtime-verify: Rime proc id 59052 (free instant Howling Blast)
    class FreezingFogTrigger : public HasAuraIdTrigger
    {
    public:
        FreezingFogTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "freezing fog", 59052) {}
    };

    // runtime-verify: Sudden Doom proc id 81340 (free instant Death Coil)
    class SuddenDoomTrigger : public HasAuraIdTrigger
    {
    public:
        SuddenDoomTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "sudden doom", 81340) {}
    };

    // runtime-verify: Crimson Scourge proc id 81141 (free instant Blood Boil)
    class CrimsonScourgeTrigger : public HasAuraIdTrigger
    {
    public:
        CrimsonScourgeTrigger(PlayerbotAI* ai) : HasAuraIdTrigger(ai, "crimson scourge", 81141) {}
    };

    class PillarOfFrostTrigger : public BoostTrigger
    {
    public:
        PillarOfFrostTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "pillar of frost") {}
    };

    /// Paired with Summon Gargoyle: fired off the Unholy Frenzy cooldown so
    /// both burst cooldowns queue together (mirrors Paladin's
    /// "avenging wrath active" -> Zealotry + Guardian of Ancient Kings pairing)
    class UnholyFrenzyTrigger : public BoostTrigger
    {
    public:
        UnholyFrenzyTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "unholy frenzy") {}
    };

    class DancingRuneWeaponTrigger : public BoostTrigger
    {
    public:
        DancingRuneWeaponTrigger(PlayerbotAI* ai) : BoostTrigger(ai, "dancing rune weapon") {}
    };
}
