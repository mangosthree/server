#include "botpch.h"
#include "../../playerbot.h"
#include "PriestMultipliers.h"
#include "HealPriestStrategy.h"

using namespace ai;

NextAction** HealPriestStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("shoot", 10.0f), NULL);
}

void HealPriestStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericPriestStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "enemy out of spell",
        NextAction::array(0, new NextAction("reach spell", ACTION_NORMAL + 9), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe heal",
        NextAction::array(0, new NextAction("circle of healing", 27.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "almost full health",
        NextAction::array(0, new NextAction("renew", 15.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "party member almost full health",
        NextAction::array(0, new NextAction("renew on party", 10.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "enemy too close for spell",
        NextAction::array(0, new NextAction("fade", 50.0f), new NextAction("flee", 49.0f), NULL)));

    // Discipline: fast efficient heal, availability-gated (talent-locked spell)
    triggers.push_back(new TriggerNode(
        "critical health",
        NextAction::array(0, new NextAction("penance", ACTION_CRITICAL_HEAL), NULL)));

    triggers.push_back(new TriggerNode(
        "party member critical health",
        NextAction::array(0, new NextAction("penance on party", ACTION_CRITICAL_HEAL), NULL)));

    // Disc/Holy: proactive raid-wide AoE heal, availability-gated
    triggers.push_back(new TriggerNode(
        "low aoe heal",
        NextAction::array(0, new NextAction("prayer of healing", 28.0f), NULL)));

    // Disc/Holy: maintain Prayer of Mending on the tank rather than a rotating target
    triggers.push_back(new TriggerNode(
        "prayer of mending on tank",
        NextAction::array(0, new NextAction("prayer of mending on tank", ACTION_HIGH + 8), NULL)));

    // Discipline: external mitigation; Holy: external emergency heal, both on the tank
    triggers.push_back(new TriggerNode(
        "tank critical health",
        NextAction::array(0, new NextAction("guardian spirit on tank", ACTION_EMERGENCY + 5), new NextAction("pain suppression on tank", ACTION_EMERGENCY), NULL)));
}
