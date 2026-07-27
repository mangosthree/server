#include "botpch.h"
#include "../../playerbot.h"
#include "WarlockTriggers.h"
#include "WarlockMultipliers.h"
#include "DpsWarlockStrategy.h"
#include "WarlockActions.h"

using namespace ai;

class DpsWarlockStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    DpsWarlockStrategyActionNodeFactory()
    {
        creators["shadow bolt"] = &shadow_bolt;
    }
private:
    static ActionNode* shadow_bolt(PlayerbotAI* ai)
    {
        return new ActionNode ("shadow bolt",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("shoot"), NULL),
            /*C*/ NULL);
    }
};

DpsWarlockStrategy::DpsWarlockStrategy(PlayerbotAI* ai) : GenericWarlockStrategy(ai)
{
    actionNodeFactories.Add(new DpsWarlockStrategyActionNodeFactory());
}


NextAction** DpsWarlockStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("incinerate", 10.0f), new NextAction("shadow bolt", 10.0f), NULL);
}

void DpsWarlockStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericWarlockStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "shadow trance",
        NextAction::array(0, new NextAction("shadow bolt", 20.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "backlash",
        NextAction::array(0, new NextAction("shadow bolt", 20.0f), NULL)));

    // Destro: Empowered Imp proc.
    // runtime-verify: aura 47283 confirmed DEAD in m3 core — safe no-op.
    triggers.push_back(new TriggerNode(
        "empowered imp",
        NextAction::array(0, new NextAction("soul fire", 20.8f), NULL)));

    // Destro: Improved Soul Fire talent-passive gate.
    // runtime-verify: core aura 85383 confirmed never applied in m3 — safe no-op;
    // trigger's own 20s throttle caps worst case at ~1 Soul Fire/20s (tauri cadence).
    triggers.push_back(new TriggerNode(
        "improved soul fire",
        NextAction::array(0, new NextAction("soul fire", 19.2f), NULL)));
}

void DpsAoeWarlockStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "high aoe",
        NextAction::array(0, new NextAction("rain of fire", 30.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("seed of corruption", 31.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("shadowfury", 29.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "corruption on attacker",
        NextAction::array(0, new NextAction("corruption on attacker", 28.0f), NULL)));

}

void DpsWarlockDebuffStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "corruption",
        NextAction::array(0, new NextAction("corruption", 12.0f), NULL)));
}
