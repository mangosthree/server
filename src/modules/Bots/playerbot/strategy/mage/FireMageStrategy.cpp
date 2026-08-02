#include "botpch.h"
#include "../../playerbot.h"
#include "MageMultipliers.h"
#include "FireMageStrategy.h"

using namespace ai;

NextAction** FireMageStrategy::getDefaultActions()
{
    // Fireball is the primary single-target filler; Scorch is the movement/
    // instant-cast fallback; Fire Blast is the lowest-priority instant filler.
    return NextAction::array(0, new NextAction("fireball", 7.0f), new NextAction("scorch", 6.0f), new NextAction("fire blast", 5.0f), NULL);
}

void FireMageStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericMageStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "pyroblast",
        NextAction::array(0, new NextAction("pyroblast", 10.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "hot streak",
        NextAction::array(0, new NextAction("pyroblast", 25.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "combustion",
        NextAction::array(0, new NextAction("combustion", 50.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "enemy too close for spell",
        NextAction::array(0, new NextAction("dragon's breath", 70.0f), NULL)));

    // Maintain Living Bomb as a single-target DoT too, not only in "fire aoe".
    triggers.push_back(new TriggerNode(
        "living bomb",
        NextAction::array(0, new NextAction("living bomb", 22.0f), NULL)));
}

void FireMageAoeStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("flamestrike", 20.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "living bomb",
        NextAction::array(0, new NextAction("living bomb", 25.0f), NULL)));
}

