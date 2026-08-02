#pragma once

namespace ai
{
    class GenericDeathknightStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
    {
    public:
        GenericDeathknightStrategyActionNodeFactory()
        {
            creators["outbreak"] = &outbreak;
            creators["raise dead"] = &raise_dead;
        }
    private:
        // Outbreak is the single-cast disease refresh; if it is on cooldown,
        // fall back to the per-disease strikes that also apply it
        static ActionNode* outbreak(PlayerbotAI* ai)
        {
            return new ActionNode ("outbreak",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("icy touch"), new NextAction("plague strike"), NULL),
                /*C*/ NULL);
        }
        static ActionNode* raise_dead(PlayerbotAI* ai)
        {
            return new ActionNode ("raise dead",
                /*P*/ NULL,
                /*A*/ NextAction::array(0, new NextAction("melee"), NULL),
                /*C*/ NULL);
        }
    };
};
