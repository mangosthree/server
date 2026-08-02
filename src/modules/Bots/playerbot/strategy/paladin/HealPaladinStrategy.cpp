#include "botpch.h"
#include "../../playerbot.h"
#include "PaladinMultipliers.h"
#include "HealPaladinStrategy.h"

using namespace ai;

HealPaladinStrategy::HealPaladinStrategy(PlayerbotAI* ai) : GenericPaladinStrategy(ai)
{
}

NextAction** HealPaladinStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("crusader strike", 11.0f), new NextAction("melee", ACTION_NORMAL), NULL);
}

void HealPaladinStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericPaladinStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "seal",
        NextAction::array(0, new NextAction("seal of insight", 88.0f), NULL)));

    // Emergency band: Flash of Light is fast, expensive triage; keep it out of
    // the medium band (cheap Holy Light fills there instead)
    triggers.push_back(new TriggerNode(
        "low health",
        NextAction::array(0, new NextAction("flash of light", 87.0f), new NextAction("divine light", 83.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "party member critical health",
        NextAction::array(0, new NextAction("guardian of ancient kings", 88.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "party member low health",
        NextAction::array(0, new NextAction("flash of light on party", 86.0f), new NextAction("divine favor", 85.0f), new NextAction("avenging wrath", 84.0f), new NextAction("divine light on party", 82.0f), NULL)));

    // Daybreak proc: instant follow-up Holy Shock
    triggers.push_back(new TriggerNode(
        "daybreak",
        NextAction::array(0, new NextAction("holy shock on party", 76.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium health",
        NextAction::array(0, new NextAction("word of glory", 75.0f), new NextAction("holy shock", 73.0f), new NextAction("holy light", 71.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "party member medium health",
        NextAction::array(0, new NextAction("word of glory on party", 74.0f), new NextAction("holy shock on party", 72.0f), new NextAction("holy light on party", 70.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "beacon of light on tank",
        NextAction::array(0, new NextAction("beacon of light on tank", 28.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of kings", 29.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "low mana",
        NextAction::array(0, new NextAction("divine plea", 13.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "judgement",
        NextAction::array(0, new NextAction("judgement", 12.0f), NULL)));
}
