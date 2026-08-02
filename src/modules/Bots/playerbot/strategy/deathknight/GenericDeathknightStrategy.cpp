#include "botpch.h"
#include "../../playerbot.h"
#include "GenericDeathknightStrategy.h"
#include "GenericDeathknightStrategyActionNodeFactory.h"

using namespace ai;

GenericDeathknightStrategy::GenericDeathknightStrategy(PlayerbotAI* ai) : MeleeCombatStrategy(ai)
{
    actionNodeFactories.Add(new GenericDeathknightStrategyActionNodeFactory());
}

void GenericDeathknightStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    MeleeCombatStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "mind freeze interrupt",
        NextAction::array(0, new NextAction("mind freeze", ACTION_INTERRUPT), NULL)));

    triggers.push_back(new TriggerNode(
        "mind freeze on enemy healer",
        NextAction::array(0, new NextAction("mind freeze on enemy healer", ACTION_INTERRUPT), NULL)));

    triggers.push_back(new TriggerNode(
        "critical health",
        NextAction::array(0, new NextAction("icebound fortitude", ACTION_EMERGENCY + 1), NULL)));

    // Disease upkeep: Outbreak refreshes both at once; if it is on cooldown
    // the engine falls to the per-disease strike that also applies it
    triggers.push_back(new TriggerNode(
        "frost fever",
        NextAction::array(0, new NextAction("outbreak", 26.0f), new NextAction("icy touch", 25.5f), NULL)));

    triggers.push_back(new TriggerNode(
        "blood plague",
        NextAction::array(0, new NextAction("outbreak", 26.0f), new NextAction("plague strike", 25.5f), NULL)));
}
