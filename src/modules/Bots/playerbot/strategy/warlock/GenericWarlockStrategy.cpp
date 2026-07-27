#include "botpch.h"
#include "../../playerbot.h"
#include "WarlockMultipliers.h"
#include "GenericWarlockStrategy.h"

using namespace ai;

class GenericWarlockStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    GenericWarlockStrategyActionNodeFactory()
    {
        creators["summon voidwalker"] = &summon_voidwalker;
        creators["banish"] = &banish;
    }
private:
    static ActionNode* summon_voidwalker(PlayerbotAI* ai)
    {
        return new ActionNode ("summon voidwalker",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("drain soul"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* banish(PlayerbotAI* ai)
    {
        return new ActionNode ("banish",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("fear"), NULL),
            /*C*/ NULL);
    }
};

GenericWarlockStrategy::GenericWarlockStrategy(PlayerbotAI* ai) : RangedCombatStrategy(ai)
{
    actionNodeFactories.Add(new GenericWarlockStrategyActionNodeFactory());
}

NextAction** GenericWarlockStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("shoot", 10.0f), NULL);
}

void GenericWarlockStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    RangedCombatStrategy::InitTriggers(triggers);

    triggers.push_back(new TriggerNode(
        "bane of agony",
        NextAction::array(0, new NextAction("curse of agony", 11.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "medium health",
        NextAction::array(0, new NextAction("drain life", 40.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "low mana",
        NextAction::array(0, new NextAction("life tap", ACTION_EMERGENCY + 5), NULL)));

    triggers.push_back(new TriggerNode(
        "target critical health",
        NextAction::array(0, new NextAction("shadowburn", 35.0f), new NextAction("drain soul", 30.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "banish",
        NextAction::array(0, new NextAction("banish", 21.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "fear",
        NextAction::array(0, new NextAction("fear on cc", 20.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "immolate",
        NextAction::array(0, new NextAction("immolate", 19.0f), NULL)));

    // Destruction: own cooldown-gated trigger so conflagrate fires on its own CD
    // instead of once per immolate application.
    triggers.push_back(new TriggerNode(
        "conflagrate",
        NextAction::array(0, new NextAction("conflagrate", 20.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "chaos bolt",
        NextAction::array(0, new NextAction("chaos bolt", 21.0f), NULL))); ///< runtime-verify spell name

    // Affliction: maintained DoTs, gated by spell availability (isPossible fails
    // silently for non-Affliction bots).
    triggers.push_back(new TriggerNode(
        "unstable affliction",
        NextAction::array(0, new NextAction("unstable affliction", 19.5f), NULL))); ///< runtime-verify spell name

    triggers.push_back(new TriggerNode(
        "haunt",
        NextAction::array(0, new NextAction("haunt", 18.5f), NULL)));

    // Demonology: big DPS cooldown + on-cooldown nuke, gated by spell availability.
    triggers.push_back(new TriggerNode(
        "metamorphosis",
        NextAction::array(0, new NextAction("metamorphosis", ACTION_HIGH + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "hand of gul'dan",
        NextAction::array(0, new NextAction("hand of gul'dan", 21.0f), NULL))); ///< runtime-verify spell name

    triggers.push_back(new TriggerNode(
        "immolation aura",
        NextAction::array(0, new NextAction("immolation aura", 17.0f), NULL))); ///< runtime-verify spell name

    triggers.push_back(new TriggerNode(
        "demon soul",
        NextAction::array(0, new NextAction("demon soul", ACTION_HIGH + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "bane of doom",
        NextAction::array(0, new NextAction("bane of doom", 12.5f), NULL)));

    triggers.push_back(new TriggerNode(
        "curse of the elements",
        NextAction::array(0, new NextAction("curse of the elements", 10.5f), NULL)));

    // Demo: Molten Core procs a free/cheap-cast Incinerate.
    // runtime-verify: safe no-op if the m3 core never applies the proc aura.
    triggers.push_back(new TriggerNode(
        "molten core",
        NextAction::array(0, new NextAction("incinerate", 20.5f), NULL)));

    // Demo: Decimation drops Soul Fire's cast time/cost below 35% target health.
    triggers.push_back(new TriggerNode(
        "decimation",
        NextAction::array(0, new NextAction("soul fire", 20.2f), NULL)));
}
