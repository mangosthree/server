#include "botpch.h"
#include "../../playerbot.h"
#include "UnholyDeathknightStrategy.h"

using namespace ai;

UnholyDeathknightStrategy::UnholyDeathknightStrategy(PlayerbotAI* ai) : GenericDeathknightStrategy(ai)
{
}

NextAction** UnholyDeathknightStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("scourge strike", ACTION_NORMAL + 1), new NextAction("melee", ACTION_NORMAL), NULL);
}

void UnholyDeathknightStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericDeathknightStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "dark transformation available",
        NextAction::array(0, new NextAction("dark transformation", 27.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "sudden doom",
        NextAction::array(0, new NextAction("death coil", 25.5f), NULL)));

    triggers.push_back(new TriggerNode(
        "scourge strike available",
        NextAction::array(0, new NextAction("scourge strike", 24.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "festering strike available",
        NextAction::array(0, new NextAction("festering strike", 23.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "runic power available",
        NextAction::array(0, new NextAction("death coil", 22.0f), NULL)));

    // Unholy Frenzy and Summon Gargoyle are paired burst cooldowns; Unholy
    // Frenzy first, Gargoyle right after (mirrors the paladin
    // "avenging wrath active" -> Zealotry + Guardian of Ancient Kings pairing)
    triggers.push_back(new TriggerNode(
        "unholy frenzy",
        NextAction::array(0, new NextAction("unholy frenzy", 29.0f), new NextAction("summon gargoyle", 28.5f), NULL)));

    triggers.push_back(new TriggerNode(
        "no pet",
        NextAction::array(0, new NextAction("raise dead", 30.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("pestilence", 25.0f), new NextAction("blood boil", 23.5f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("death and decay", 24.5f), NULL)));
}
