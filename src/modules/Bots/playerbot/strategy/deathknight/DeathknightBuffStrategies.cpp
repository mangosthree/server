#include "botpch.h"
#include "../../playerbot.h"
#include "DeathknightBuffStrategies.h"

using namespace ai;

void DeathknightBuffThreatStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "blood presence",
        NextAction::array(0, new NextAction("blood presence", 90.0f), NULL)));
}

void DeathknightBuffDpsStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "unholy presence",
        NextAction::array(0, new NextAction("unholy presence", 90.0f), NULL)));
}
