#include "botpch.h"
#include "../../playerbot.h"
#include "FrostDeathknightStrategy.h"

using namespace ai;

FrostDeathknightStrategy::FrostDeathknightStrategy(PlayerbotAI* ai) : GenericDeathknightStrategy(ai)
{
}

NextAction** FrostDeathknightStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("obliterate", ACTION_NORMAL + 1), new NextAction("melee", ACTION_NORMAL), NULL);
}

void FrostDeathknightStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericDeathknightStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "killing machine",
        NextAction::array(0, new NextAction("obliterate", 26.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "freezing fog",
        NextAction::array(0, new NextAction("howling blast", 25.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "obliterate pair",
        NextAction::array(0, new NextAction("obliterate", 24.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "runic power available",
        NextAction::array(0, new NextAction("frost strike", 23.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "rune pair depleted",
        NextAction::array(0, new NextAction("frost strike", 19.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "howling blast available",
        NextAction::array(0, new NextAction("howling blast", 18.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "pillar of frost",
        NextAction::array(0, new NextAction("pillar of frost", 29.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "no pet",
        NextAction::array(0, new NextAction("raise dead", 21.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("howling blast", 25.5f), new NextAction("death and decay", 24.5f), NULL)));

    triggers.push_back(new TriggerNode(
        "horn of winter",
        NextAction::array(0, new NextAction("horn of winter", 14.0f), NULL)));
}
