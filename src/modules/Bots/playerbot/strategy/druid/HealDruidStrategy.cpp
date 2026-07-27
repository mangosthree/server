#include "botpch.h"
#include "../../playerbot.h"
#include "DruidMultipliers.h"
#include "HealDruidStrategy.h"

using namespace ai;

class HealDruidStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    HealDruidStrategyActionNodeFactory()
    {
    }
private:
};

HealDruidStrategy::HealDruidStrategy(PlayerbotAI* ai) : GenericDruidStrategy(ai)
{
    actionNodeFactories.Add(new HealDruidStrategyActionNodeFactory());
}

void HealDruidStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericDruidStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "enemy out of spell",
        NextAction::array(0, new NextAction("reach spell", ACTION_NORMAL + 9), NULL)));

    triggers.push_back(new TriggerNode(
        "tree form",
        NextAction::array(0, new NextAction("tree form", ACTION_HIGH + 1), NULL)));

    // Cata resto's HoT anchor: keep Lifebloom stacked on the tank rather
    // than rotating it like the other party HoTs.
    triggers.push_back(new TriggerNode(
        "lifebloom on tank",
        NextAction::array(0, new NextAction("lifebloom on tank", ACTION_HIGH + 8), NULL)));

    // Swiftmend consumes an existing Rejuvenation/Regrowth HoT for a burst
    // heal; try it ahead of Regrowth.
    triggers.push_back(new TriggerNode(
        "medium health",
        NextAction::array(0, new NextAction("swiftmend", ACTION_MEDIUM_HEAL + 4), new NextAction("regrowth", ACTION_MEDIUM_HEAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "party member medium health",
        NextAction::array(0, new NextAction("swiftmend on party", ACTION_MEDIUM_HEAL + 3), new NextAction("regrowth on party", ACTION_MEDIUM_HEAL + 1), NULL)));

    // Nature's Swiftness: instant-cast the next heal for a party member in
    // a critical-health emergency, ahead of GenericDruidStrategy's
    // healing-touch-on-party fallback.
    triggers.push_back(new TriggerNode(
        "party member critical health",
        NextAction::array(0, new NextAction("nature's swiftness", ACTION_CRITICAL_HEAL + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "almost full health",
        NextAction::array(0, new NextAction("rejuvenation", ACTION_LIGHT_HEAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "party member almost full health",
        NextAction::array(0, new NextAction("rejuvenation on party", ACTION_LIGHT_HEAL + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe heal",
        NextAction::array(0, new NextAction("tranquility", ACTION_MEDIUM_HEAL + 3), NULL)));

    // Wild Growth is the cheaper, non-channeled AoE heal; try it on the
    // lighter aoe-heal tier before committing to Tranquility's channel.
    triggers.push_back(new TriggerNode(
        "low aoe heal",
        NextAction::array(0, new NextAction("wild growth", ACTION_MEDIUM_HEAL + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "entangling roots",
        NextAction::array(0, new NextAction("entangling roots on cc", ACTION_HIGH + 1), NULL)));
}
