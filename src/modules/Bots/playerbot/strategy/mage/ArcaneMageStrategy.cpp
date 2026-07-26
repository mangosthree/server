#include "botpch.h"
#include "../../playerbot.h"
#include "MageMultipliers.h"
#include "ArcaneMageStrategy.h"

using namespace ai;

class ArcaneMageStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    ArcaneMageStrategyActionNodeFactory()
    {
        creators["arcane blast"] = &arcane_blast;
        creators["arcane barrage"] = &arcane_barrage;
        creators["arcane missiles"] = &arcane_missiles;
    }
private:
    static ActionNode* arcane_blast(PlayerbotAI* ai)
    {
        return new ActionNode ("arcane blast",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("arcane missiles"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* arcane_barrage(PlayerbotAI* ai)
    {
        return new ActionNode ("arcane barrage",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("arcane missiles"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* arcane_missiles(PlayerbotAI* ai)
    {
        return new ActionNode ("arcane missiles",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("shoot"), NULL),
            /*C*/ NULL);
    }
};

ArcaneMageStrategy::ArcaneMageStrategy(PlayerbotAI* ai) : GenericMageStrategy(ai)
{
    actionNodeFactories.Add(new ArcaneMageStrategyActionNodeFactory());
}

NextAction** ArcaneMageStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("arcane barrage", 10.0f), NULL);
}

void ArcaneMageStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericMageStrategy::InitTriggers(triggers);

    // Stacks Arcane Blast to its 4-stack cap; ArcaneBlastTrigger goes inactive
    // once capped so this and the "capped" dump below never fire together.
    triggers.push_back(new TriggerNode(
        "arcane blast",
        NextAction::array(0, new NextAction("arcane blast", 15.0f), NULL)));

    // Capped at 4 stacks with no proc up: dump via Barrage instead of re-stacking.
    triggers.push_back(new TriggerNode(
        "arcane blast capped",
        NextAction::array(0, new NextAction("arcane barrage", 18.0f), NULL)));

    // Cata proc ("Arcane Missiles!"), replaces the dead WotLK "missile barrage"
    // trigger; free instant cast, so it outranks everything else in the rotation.
    triggers.push_back(new TriggerNode(
        "arcane missiles proc",
        NextAction::array(0, new NextAction("arcane missiles", 26.0f), NULL)));
}

