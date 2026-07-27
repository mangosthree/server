#pragma once

namespace ai {
    class CastFeralChargeCatAction : public CastReachTargetSpellAction
    {
    public:
        CastFeralChargeCatAction(PlayerbotAI* ai) : CastReachTargetSpellAction(ai, "feral charge", 1.5f) {}
    };

    class CastCowerAction : public CastBuffSpellAction
    {
    public:
        CastCowerAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "cower") {}
    };


    class CastBerserkAction : public CastBuffSpellAction
    {
    public:
        CastBerserkAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "berserk") {}
    };

    class CastTigersFuryAction : public CastBuffSpellAction
    {
    public:
        CastTigersFuryAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "tiger's fury") {}
        virtual bool isUseful() { return CastBuffSpellAction::isUseful() && AI_VALUE2(uint8, "energy", "self target") < 40; }
    };

    class CastRakeAction : public CastDebuffSpellAction
    {
    public:
        CastRakeAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "rake") {}
    };


    // Claw was removed in 4.0.1; Shred is the live cat-form combo builder.
    class CastShredAction : public CastMeleeSpellAction {
    public:
        CastShredAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "shred") {}
    };

    class CastMangleCatAction : public CastMeleeSpellAction {
    public:
        CastMangleCatAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "mangle") {}
    };

    class CastSwipeCatAction : public CastMeleeSpellAction {
    public:
        CastSwipeCatAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "swipe") {}
    };

    class CastFerociousBiteAction : public CastMeleeSpellAction {
    public:
        CastFerociousBiteAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "ferocious bite") {}
    };

    // Aura-gated (CastDebuffSpellAction/CastAuraSpellAction::isUseful skips
    // the cast while the bleed is still up) so Rip is maintained rather than
    // reapplied every combo-point dump.
    class CastRipAction : public CastDebuffSpellAction {
    public:
        CastRipAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "rip") {}
    };

    class CastSavageRoarAction : public CastBuffSpellAction {
    public:
        CastSavageRoarAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "savage roar") {}
    };
}
