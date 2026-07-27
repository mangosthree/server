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

    // Demo bots know "summon felguard" and take it (highest relevance);
    // Affliction falls to "summon felhunter"; Destro's isUseful() gate on
    // felhunter (Conflagrate-only check) drops it through to "summon imp".
    // isPossible()/isUseful() no-op each option for everyone it doesn't apply to.
    triggers.push_back(new TriggerNode(
        "no pet",
        NextAction::array(0, new NextAction("summon felguard", 11.0f), new NextAction("summon felhunter", 10.5f), new NextAction("summon imp", 10.0f), NULL)));
}
