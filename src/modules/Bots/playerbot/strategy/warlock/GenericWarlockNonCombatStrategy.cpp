#include "botpch.h"
#include "../../playerbot.h"
#include "WarlockMultipliers.h"
#include "GenericWarlockNonCombatStrategy.h"

using namespace ai;

class GenericWarlockNonCombatStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    GenericWarlockNonCombatStrategyActionNodeFactory()
    {
        creators["fel armor"] = &fel_armor;
        creators["demon armor"] = &demon_armor;
    }
private:
    static ActionNode* fel_armor(PlayerbotAI* ai)
    {
        return new ActionNode ("fel armor",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("demon armor"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* demon_armor(PlayerbotAI* ai)
    {
        return new ActionNode ("demon armor",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("demon skin"), NULL),
            /*C*/ NULL);
    }
};

GenericWarlockNonCombatStrategy::GenericWarlockNonCombatStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai)
{
    actionNodeFactories.Add(new GenericWarlockNonCombatStrategyActionNodeFactory());
}

void GenericWarlockNonCombatStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    NonCombatStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "demon armor",
        NextAction::array(0, new NextAction("fel armor", 21.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "no healthstone",
        NextAction::array(0, new NextAction("create healthstone", 15.0f), NULL)));

    // Demonology bots know "summon felguard" and take it (higher relevance);
    // isPossible() no-ops it for everyone else, so the queue falls through to imp.
    triggers.push_back(new TriggerNode(
        "no pet",
        NextAction::array(0, new NextAction("summon felguard", 11.0f), new NextAction("summon imp", 10.0f), NULL)));
}
