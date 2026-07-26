#include "botpch.h"
#include "../../playerbot.h"

#include "HunterMultipliers.h"
#include "DpsHunterStrategy.h"

using namespace ai;

class DpsHunterStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    DpsHunterStrategyActionNodeFactory()
    {
        creators["aimed shot"] = &aimed_shot;
        creators["chimera shot"] = &chimera_shot;
        creators["explosive shot"] = &explosive_shot;
        creators["concussive shot"] = &concussive_shot;
        creators["cobra shot"] = &cobra_shot;
        creators["steady shot"] = &steady_shot;
        creators["kill command"] = &kill_command;
    }
private:
    static ActionNode* aimed_shot(PlayerbotAI* ai)
    {
        return new ActionNode ("aimed shot",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("chimera shot", 10.0f), NULL),
            /*C*/ NULL);
    }
    static ActionNode* chimera_shot(PlayerbotAI* ai)
    {
        return new ActionNode ("chimera shot",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("arcane shot", 10.0f), NULL),
            /*C*/ NULL);
    }
    static ActionNode* explosive_shot(PlayerbotAI* ai)
    {
        return new ActionNode ("explosive shot",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("aimed shot"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* concussive_shot(PlayerbotAI* ai)
    {
        return new ActionNode ("concussive shot",
            /*P*/ NULL,
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* cobra_shot(PlayerbotAI* ai)
    {
        return new ActionNode ("cobra shot",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("steady shot"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* steady_shot(PlayerbotAI* ai)
    {
        return new ActionNode ("steady shot",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("auto shot"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* kill_command(PlayerbotAI* ai)
    {
        return new ActionNode ("kill command",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("arcane shot"), NULL),
            /*C*/ NULL);
    }

};

DpsHunterStrategy::DpsHunterStrategy(PlayerbotAI* ai) : GenericHunterStrategy(ai)
{
    actionNodeFactories.Add(new DpsHunterStrategyActionNodeFactory());
}

NextAction** DpsHunterStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("explosive shot", 11.0f), new NextAction("cobra shot", 10.5f), new NextAction("auto shot", 10.0f), NULL);
}

void DpsHunterStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    GenericHunterStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "black arrow",
        NextAction::array(0, new NextAction("black arrow", 51.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "no pet",
        NextAction::array(0, new NextAction("call pet", 60.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "hunters pet low health",
        NextAction::array(0, new NextAction("mend pet", 60.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "hunter's mark",
        NextAction::array(0, new NextAction("hunter's mark", 52.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "freezing trap",
        NextAction::array(0, new NextAction("freezing trap", 83.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "concussive shot on snare target",
        NextAction::array(0, new NextAction("concussive shot", 83.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "high focus",
        NextAction::array(0, new NextAction("kill command", 15.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "target critical health",
        NextAction::array(0, new NextAction("kill shot", 25.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "chimera shot",
        NextAction::array(0, new NextAction("chimera shot", 21.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "bestial wrath",
        NextAction::array(0, new NextAction("bestial wrath", 56.0f), NULL)));
}

void DpsAoeHunterStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("multi-shot", 20.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "serpent sting on attacker",
        NextAction::array(0, new NextAction("serpent sting on attacker", 49.0f), NULL)));
}

void DpsHunterDebuffStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "no stings",
        NextAction::array(0, new NextAction("serpent sting", 50.0f), NULL)));
}
