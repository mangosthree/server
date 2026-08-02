#include "botpch.h"
#include "../../playerbot.h"
#include "GenericDeathknightNonCombatStrategy.h"
#include "GenericDeathknightStrategyActionNodeFactory.h"

using namespace ai;

GenericDeathknightNonCombatStrategy::GenericDeathknightNonCombatStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai)
{
    actionNodeFactories.Add(new GenericDeathknightStrategyActionNodeFactory());
}

void GenericDeathknightNonCombatStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    NonCombatStrategy::InitTriggers(triggers);

    // Presence upkeep is handled by the bthreat/bdps buff strategies (added to
    // both the combat and non-combat engines), not here.

    triggers.push_back(new TriggerNode(
        "horn of winter",
        NextAction::array(0, new NextAction("horn of winter", 14.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "bone shield",
        NextAction::array(0, new NextAction("bone shield", 15.0f), NULL)));

    // NOTE: deliberately not gated to a single spec here - Frost's own combat
    // strategy also queues "raise dead" as a combat cooldown (same registered
    // Action instance), so an isUseful() gate restricted to Unholy would
    // silently no-op Frost's cooldown usage too. Blood simply never adds a
    // "no pet" node of its own, so it is unaffected either way.
    triggers.push_back(new TriggerNode(
        "no pet",
        NextAction::array(0, new NextAction("raise dead", 30.0f), NULL)));
}
