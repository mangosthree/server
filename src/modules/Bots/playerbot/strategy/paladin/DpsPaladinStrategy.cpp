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
        "avenging wrath",
        NextAction::array(0, new NextAction("avenging wrath", 29.0f), NULL)));

    // Zealotry and Guardian of Ancient Kings are AW-paired; queued only while the
    // wings aura is up (the boost trigger deactivates once AW is active)
    triggers.push_back(new TriggerNode(
        "avenging wrath active",
        NextAction::array(0, new NextAction("zealotry", 28.0f), new NextAction("guardian of ancient kings", 27.0f), NULL)));

    // Inquisition refreshes above the finisher; CastAuraSpellAction::isUseful
    // no-ops it once the buff is up, so Templar's Verdict takes over
    triggers.push_back(new TriggerNode(
        "holy power available",
        NextAction::array(0, new NextAction("inquisition", 26.0f), new NextAction("templar's verdict", 20.0f), NULL)));

    // Divine Purpose proc: free finisher (bypasses the 3 Holy Power gate)
    triggers.push_back(new TriggerNode(
        "divine purpose",
        NextAction::array(0, new NextAction("inquisition", 26.0f), new NextAction("templar's verdict", 23.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("divine storm", 25.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "crusader strike available",
        NextAction::array(0, new NextAction("crusader strike", 24.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "art of war",
        NextAction::array(0, new NextAction("exorcism", 22.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "judgement",
        NextAction::array(0, new NextAction("judgement", 19.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "high aoe",
        NextAction::array(0, new NextAction("consecration", 18.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "rebuke interrupt",
        NextAction::array(0, new NextAction("rebuke", ACTION_INTERRUPT), NULL)));
}
