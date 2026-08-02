#include "botpch.h"
#include "../../playerbot.h"
#include "BloodDeathknightStrategy.h"

using namespace ai;

BloodDeathknightStrategy::BloodDeathknightStrategy(PlayerbotAI* ai) : GenericDeathknightStrategy(ai)
{
}

NextAction** BloodDeathknightStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("death strike", ACTION_NORMAL + 2), new NextAction("heart strike", ACTION_NORMAL + 1), new NextAction("melee", ACTION_NORMAL), NULL);
}

void BloodDeathknightStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericDeathknightStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "lose aggro",
        NextAction::array(0, new NextAction("dark command", ACTION_HIGH + 7), new NextAction("death grip", ACTION_HIGH + 6), NULL)));

    triggers.push_back(new TriggerNode(
        "critical health",
        NextAction::array(0, new NextAction("vampiric blood", ACTION_EMERGENCY + 2), new NextAction("rune tap", ACTION_EMERGENCY + 1), new NextAction("death pact", ACTION_EMERGENCY), NULL)));

    triggers.push_back(new TriggerNode(
        "low health",
        NextAction::array(0, new NextAction("rune tap", ACTION_CRITICAL_HEAL + 2), new NextAction("death strike", ACTION_CRITICAL_HEAL + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "death strike available",
        NextAction::array(0, new NextAction("death strike", 23.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "crimson scourge",
        NextAction::array(0, new NextAction("blood boil", 24.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "rune strike available",
        NextAction::array(0, new NextAction("rune strike", 20.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("pestilence", 25.0f), new NextAction("death and decay", 24.5f), new NextAction("blood boil", 24.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "dancing rune weapon",
        NextAction::array(0, new NextAction("dancing rune weapon", 28.0f), NULL)));
}
