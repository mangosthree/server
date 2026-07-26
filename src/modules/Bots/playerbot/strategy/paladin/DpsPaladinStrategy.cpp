#include "botpch.h"
#include "../../playerbot.h"
#include "PaladinMultipliers.h"
#include "DpsPaladinStrategy.h"

using namespace ai;

class DpsPaladinStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    DpsPaladinStrategyActionNodeFactory()
    {
        creators["seal of truth"] = &seal_of_truth;
        creators["seal of command"] = &seal_of_command;
        creators["blessing of might"] = &blessing_of_might;
        creators["crusader strike"] = &crusader_strike;
    }

private:
    static ActionNode* seal_of_truth(PlayerbotAI* ai)
    {
        return new ActionNode ("seal of truth",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("seal of command"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* seal_of_command(PlayerbotAI* ai)
    {
        return new ActionNode ("seal of command",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("seal of insight"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* blessing_of_might(PlayerbotAI* ai)
    {
        return new ActionNode ("blessing of might",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("blessing of kings"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* crusader_strike(PlayerbotAI* ai)
    {
        return new ActionNode ("crusader strike",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("melee"), NULL),
            /*C*/ NULL);
    }
};

DpsPaladinStrategy::DpsPaladinStrategy(PlayerbotAI* ai) : GenericPaladinStrategy(ai)
{
    actionNodeFactories.Add(new DpsPaladinStrategyActionNodeFactory());
}

NextAction** DpsPaladinStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("crusader strike", ACTION_NORMAL + 1), NULL);
}

void DpsPaladinStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericPaladinStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "low health",
        NextAction::array(0, new NextAction("divine shield", ACTION_CRITICAL_HEAL + 2), new NextAction("holy light", ACTION_CRITICAL_HEAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "judgement",
        NextAction::array(0, new NextAction("judgement", ACTION_NORMAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("divine storm", ACTION_HIGH + 1), new NextAction("consecration", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "art of war",
        NextAction::array(0, new NextAction("exorcism", ACTION_HIGH + 2), NULL)));

    // Inquisition is ranked above the finisher; CastAuraSpellAction::isUseful
    // already no-ops it once the buff is up, so Templar's Verdict takes over
    triggers.push_back(new TriggerNode(
        "holy power available",
        NextAction::array(0, new NextAction("inquisition", ACTION_HIGH + 2), new NextAction("templar's verdict", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "avenging wrath",
        NextAction::array(0, new NextAction("avenging wrath", ACTION_HIGH + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "rebuke interrupt",
        NextAction::array(0, new NextAction("rebuke", ACTION_INTERRUPT), NULL)));
}
