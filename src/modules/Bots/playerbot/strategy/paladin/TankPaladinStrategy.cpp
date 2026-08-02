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
        "righteous fury",
        NextAction::array(0, new NextAction("righteous fury", 28.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "holy shield",
        NextAction::array(0, new NextAction("holy shield", 27.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "low mana",
        NextAction::array(0, new NextAction("divine plea", 26.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("inquisition", 25.0f), new NextAction("hammer of the righteous", 22.5f), NULL)));

    // Word of Glory outranks Shield of the Righteous but its isUseful() health
    // gate (CastHealingSpellAction) keeps it out of the basket when not hurt
    triggers.push_back(new TriggerNode(
        "holy power available",
        NextAction::array(0, new NextAction("word of glory", 24.0f), new NextAction("shield of the righteous", 23.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "crusader strike available",
        NextAction::array(0, new NextAction("crusader strike", 22.0f), NULL)));

    // Covers Grand Crusader (proc resets the cooldown server-side)
    triggers.push_back(new TriggerNode(
        "avenger's shield available",
        NextAction::array(0, new NextAction("avenger's shield", 21.5f), NULL)));

    triggers.push_back(new TriggerNode(
        "judgement",
        NextAction::array(0, new NextAction("judgement", 20.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("consecration", 19.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "lose aggro",
        NextAction::array(0, new NextAction("hand of reckoning", ACTION_HIGH + 7), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of kings", ACTION_HIGH + 9), NULL)));

    triggers.push_back(new TriggerNode(
        "critical health",
        NextAction::array(0, new NextAction("guardian of ancient kings", ACTION_EMERGENCY + 2), new NextAction("ardent defender", ACTION_EMERGENCY + 1), NULL)));
}
