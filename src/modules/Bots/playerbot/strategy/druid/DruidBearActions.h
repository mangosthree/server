#pragma once

namespace ai {
    class CastFeralChargeBearAction : public CastReachTargetSpellAction
    {
    public:
        CastFeralChargeBearAction(PlayerbotAI* ai) : CastReachTargetSpellAction(ai, "feral charge", 1.5f) {}
    };

    class CastGrowlAction : public CastSpellAction
    {
    public:
        CastGrowlAction(PlayerbotAI* ai) : CastSpellAction(ai, "growl") {}
    };

    class CastMaulAction : public CastMeleeSpellAction
    {
    public:
        CastMaulAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "maul") {}
        // Preserve the rage-cap-avoidance gate for the normal case, but a
        // Clearcasting (16870) proc makes Maul free -- let it through then too.
        virtual bool isUseful() { return CastMeleeSpellAction::isUseful() && (AI_VALUE2(uint8, "rage", "self target") >= 45 || ai->HasAura(16870u, bot)); }
    };

    class CastBashAction : public CastMeleeSpellAction
    {
    public:
        CastBashAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "bash") {}
    };

    class CastSwipeAction : public CastMeleeSpellAction
    {
    public:
        CastSwipeAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "swipe") {}
    };

    class CastDemoralizingRoarAction : public CastDebuffSpellAction
    {
    public:
        CastDemoralizingRoarAction(PlayerbotAI* ai) : CastDebuffSpellAction(ai, "demoralizing roar") {}
    };

    class CastMangleBearAction : public CastMeleeSpellAction
    {
    public:
        CastMangleBearAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "mangle") {}
    };

    class CastSwipeBearAction : public CastMeleeSpellAction
    {
    public:
        CastSwipeBearAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "swipe") {}
    };

    class CastLacerateAction : public CastMeleeSpellAction
    {
    public:
        CastLacerateAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "lacerate") {}
    };

    class CastBashOnEnemyHealerAction : public CastSpellOnEnemyHealerAction
    {
    public:
        CastBashOnEnemyHealerAction(PlayerbotAI* ai) : CastSpellOnEnemyHealerAction(ai, "bash") {}
    };

    class CastThrashAction : public CastMeleeSpellAction
    {
    public:
        CastThrashAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "thrash") {}
    };

    class CastFrenziedRegenerationAction : public CastBuffSpellAction
    {
    public:
        CastFrenziedRegenerationAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "frenzied regeneration") {}
    };

    class CastSkullBashAction : public CastMeleeSpellAction
    {
    public:
        CastSkullBashAction(PlayerbotAI* ai) : CastMeleeSpellAction(ai, "skull bash") {}
    };
}
