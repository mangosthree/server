#include "botpch.h"
#include "../../playerbot.h"
#include "MageMultipliers.h"
#include "FrostMageStrategy.h"

using namespace ai;


FrostMageStrategy::FrostMageStrategy(PlayerbotAI* ai) : GenericMageStrategy(ai)
{
}

NextAction** FrostMageStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("frostbolt", 7.0f), NULL);
}

void FrostMageStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericMageStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "icy veins",
        NextAction::array(0, new NextAction("icy veins", 50.0f), NULL)));

    // core-proc-dependent, runtime-verify: deep freeze outranks ice lance so the
    // finisher wins the moment both are up; falls back to ice lance otherwise.
    triggers.push_back(new TriggerNode(
        "fingers of frost",
        NextAction::array(0, new NextAction("deep freeze", 26.0f),
            new NextAction("ice lance", 24.0f), NULL)));

    // core-proc-dependent, runtime-verify.
    triggers.push_back(new TriggerNode(
        "brain freeze",
        NextAction::array(0, new NextAction("frostfire bolt", 23.0f), NULL)));
}

void FrostMageAoeStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "high aoe",
        NextAction::array(0, new NextAction("blizzard", 40.0f), NULL)));
}
