#include "botpch.h"
#include "../../playerbot.h"
#include "WarriorMultipliers.h"
#include "DpsWarriorStrategy.h"

using namespace ai;

class DpsWarriorStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    DpsWarriorStrategyActionNodeFactory()
    {
        creators["overpower"] = &overpower;
        creators["melee"] = &melee;
        creators["charge"] = &charge;
        creators["bloodthirst"] = &bloodthirst;
        creators["rend"] = &rend;
        creators["death wish"] = &death_wish;
        creators["execute"] = &execute;
    }
private:
    static ActionNode* overpower(PlayerbotAI* ai)
    {
        // no "melee" alternative: a failed overpower must not fall through
        // to a no-op auto-attack and starve the rest of the tick
        return new ActionNode ("overpower",
            /*P*/ NULL,
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* melee(PlayerbotAI* ai)
    {
        return new ActionNode ("melee",
            /*P*/ NextAction::array(0, new NextAction("charge"), NULL),
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* charge(PlayerbotAI* ai)
    {
        return new ActionNode ("charge",
            /*P*/ NextAction::array(0, new NextAction("battle stance"), NULL),
            /*A*/ NextAction::array(0, new NextAction("reach melee"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* bloodthirst(PlayerbotAI* ai)
    {
        return new ActionNode ("bloodthirst",
            /*P*/ NextAction::array(0, new NextAction("battle stance"), NULL),
            /*A*/ NextAction::array(0, new NextAction("heroic strike"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* rend(PlayerbotAI* ai)
    {
        return new ActionNode ("rend",
            /*P*/ NextAction::array(0, new NextAction("battle stance"), NULL),
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* death_wish(PlayerbotAI* ai)
    {
        return new ActionNode ("death wish",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("berserker rage"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* execute(PlayerbotAI* ai)
    {
        return new ActionNode ("execute",
            /*P*/ NextAction::array(0, new NextAction("battle stance"), NULL),
            /*A*/ NextAction::array(0, new NextAction("heroic strike"), NULL),
            /*C*/ NULL);
    }
};

DpsWarriorStrategy::DpsWarriorStrategy(PlayerbotAI* ai) : GenericWarriorStrategy(ai)
{
    actionNodeFactories.Add(new DpsWarriorStrategyActionNodeFactory());
}

NextAction** DpsWarriorStrategy::getDefaultActions()
{
    // Bloodthirst first (Fury); falls through to plain melee for Arms
    // (Bloodthirst unknown -> IMPOSSIBLE) so Arms always has an auto-attack starter.
    return NextAction::array(0, new NextAction("bloodthirst", ACTION_NORMAL + 1), new NextAction("melee", ACTION_NORMAL + 1), NULL);
}

void DpsWarriorStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericWarriorStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "enemy out of melee",
        NextAction::array(0, new NextAction("charge", ACTION_NORMAL + 9), NULL)));

    // Arms rend, moved off the Generic default priority
    triggers.push_back(new TriggerNode(
        "rend",
        NextAction::array(0, new NextAction("rend", ACTION_NORMAL + 6), NULL)));

    triggers.push_back(new TriggerNode(
        "target critical health",
        NextAction::array(0, new NextAction("execute", ACTION_NORMAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "hamstring",
        NextAction::array(0, new NextAction("hamstring", ACTION_INTERRUPT), NULL)));

    triggers.push_back(new TriggerNode(
        "victory rush",
        NextAction::array(0, new NextAction("victory rush", ACTION_HIGH + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "death wish",
        NextAction::array(0, new NextAction("death wish", ACTION_HIGH + 2), NULL)));

    // Arms: real rotation instead of degrading to Heroic Strike spam; no-op for Fury (spell unknown)
    triggers.push_back(new TriggerNode(
        "mortal strike",
        NextAction::array(0, new NextAction("mortal strike", ACTION_NORMAL + 5), NULL)));

    triggers.push_back(new TriggerNode(
        "colossus smash",
        NextAction::array(0, new NextAction("colossus smash", ACTION_NORMAL + 4), NULL)));

    // Arms Overpower: id-based proc (taste for blood), "melee" alt removed
    triggers.push_back(new TriggerNode(
        "taste for blood",
        NextAction::array(0, new NextAction("overpower", ACTION_NORMAL + 3), NULL)));

    // Arms hard-cast filler; Fury's instant Slam is wired on the Bloodsurge proc below
    triggers.push_back(new TriggerNode(
        "medium rage available",
        NextAction::array(0, new NextAction("slam", ACTION_NORMAL + 1), NULL)));

    // Fury: no-op for Arms (spell unknown)
    triggers.push_back(new TriggerNode(
        "raging blow",
        NextAction::array(0, new NextAction("raging blow", ACTION_NORMAL + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "recklessness",
        NextAction::array(0, new NextAction("recklessness", ACTION_HIGH + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "deadly calm",
        NextAction::array(0, new NextAction("deadly calm", ACTION_HIGH + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "high rage available",
        NextAction::array(0, new NextAction("heroic strike", ACTION_NORMAL + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "battle trance",
        NextAction::array(0, new NextAction("heroic strike", ACTION_NORMAL + 2), NULL)));

    // Fury bootstrap: Raging Blow/Recklessness/Berserker Rage need this stance
    triggers.push_back(new TriggerNode(
        "berserker stance",
        NextAction::array(0, new NextAction("berserker stance", ACTION_NORMAL + 6), NULL)));

    triggers.push_back(new TriggerNode(
        "bloodthirst",
        NextAction::array(0, new NextAction("bloodthirst", ACTION_NORMAL + 5), NULL)));

    // Fury enrage-weave
    triggers.push_back(new TriggerNode(
        "not enraged",
        NextAction::array(0, new NextAction("berserker rage", ACTION_NORMAL + 3), NULL)));

    // Fury Bloodsurge: instant Slam only, never a hard-cast
    triggers.push_back(new TriggerNode(
        "bloodsurge",
        NextAction::array(0, new NextAction("slam", ACTION_NORMAL + 2), NULL)));
}


void DpsWarrirorAoeStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "rend on attacker",
        NextAction::array(0, new NextAction("rend on attacker", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("thunder clap", ACTION_HIGH + 2), new NextAction("demoralizing shout", ACTION_HIGH + 2),
            new NextAction("sweeping strikes", ACTION_HIGH + 3), new NextAction("cleave", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("bladestorm", ACTION_HIGH + 4), NULL)));

    triggers.push_back(new TriggerNode(
        "high aoe",
        NextAction::array(0, new NextAction("whirlwind", ACTION_HIGH + 2), NULL)));
}
