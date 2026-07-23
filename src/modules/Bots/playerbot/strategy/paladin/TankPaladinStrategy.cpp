#include "botpch.h"
#include "../../playerbot.h"
#include "PaladinMultipliers.h"
#include "TankPaladinStrategy.h"

using namespace ai;

TankPaladinStrategy::TankPaladinStrategy(PlayerbotAI* ai) : GenericPaladinStrategy(ai)
{
}

NextAction** TankPaladinStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("crusader strike", ACTION_NORMAL + 1), new NextAction("melee", ACTION_NORMAL), NULL);
}

void TankPaladinStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericPaladinStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "judgement",
        NextAction::array(0, new NextAction("judgement", ACTION_NORMAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "righteous fury",
        NextAction::array(0, new NextAction("righteous fury", ACTION_HIGH + 8), NULL)));

    triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("hammer of the righteous", ACTION_HIGH + 6), new NextAction("avenger's shield", ACTION_HIGH + 6), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("consecration", ACTION_HIGH + 6), NULL)));

    triggers.push_back(new TriggerNode(
        "lose aggro",
        NextAction::array(0, new NextAction("hand of reckoning", ACTION_HIGH + 7), NULL)));

    triggers.push_back(new TriggerNode(
        "holy shield",
        NextAction::array(0, new NextAction("holy shield", ACTION_HIGH + 7), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of kings", ACTION_HIGH + 9), NULL)));

    // Word of Glory outranks Shield of the Righteous but its isUseful() health
    // gate (CastHealingSpellAction) keeps it out of the basket when not hurt
    triggers.push_back(new TriggerNode(
        "holy power available",
        NextAction::array(0, new NextAction("word of glory", ACTION_HIGH + 2), new NextAction("shield of the righteous", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "critical health",
        NextAction::array(0, new NextAction("guardian of ancient kings", ACTION_EMERGENCY + 2), new NextAction("ardent defender", ACTION_EMERGENCY + 1), NULL)));
}
