#pragma once

namespace ai
{
    class CastComboAction : public CastMeleeSpellAction
    {
    public:
        CastComboAction(PlayerbotAI* ai, string name) : CastMeleeSpellAction(ai, name) {}

        virtual bool isUseful()
        {
            return CastMeleeSpellAction::isUseful() && AI_VALUE2(uint8, "combo", "current target") < 5;
        }
    };

    class CastSinisterStrikeAction : public CastComboAction
    {
    public:
        CastSinisterStrikeAction(PlayerbotAI* ai) : CastComboAction(ai, "sinister strike") {}
    };

    class CastMutilateAction : public CastComboAction
    {
    public:
        CastMutilateAction(PlayerbotAI* ai) : CastComboAction(ai, "mutilate") {}
    };

    class CastGougeAction : public CastComboAction
    {
    public:
        CastGougeAction(PlayerbotAI* ai) : CastComboAction(ai, "gouge") {}
    };

    class CastBackstabAction : public CastComboAction
    {
    public:
        CastBackstabAction(PlayerbotAI* ai) : CastComboAction(ai, "backstab") {}
    };

    // Subtlety opener/builder. Usable while stealthed or during Shadow Dance
    // (51713), and only from behind the target.
    class CastAmbushAction : public CastComboAction
    {
    public:
        CastAmbushAction(PlayerbotAI* ai) : CastComboAction(ai, "ambush") {}

        virtual bool isUseful()
        {
            return CastComboAction::isUseful()
                && (ai->HasAura("stealth", bot) || ai->HasAura(51713u, bot))
                && AI_VALUE2(bool, "behind", "current target");
        }
    };

    class CastHemorrhageAction : public CastComboAction
    {
    public:
        CastHemorrhageAction(PlayerbotAI* ai) : CastComboAction(ai, "hemorrhage") {}
    };
}
