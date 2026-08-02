#include "botpch.h"
#include "../../playerbot.h"
#include "DruidMultipliers.h"
#include "BearTankDruidStrategy.h"

using namespace ai;

class BearTankDruidStrategyActionNodeFactory : public NamedObjectFactory<ActionNode>
{
public:
    BearTankDruidStrategyActionNodeFactory()
    {
        creators["melee"] = &melee;
        creators["feral charge - bear"] = &feral_charge_bear;
        creators["swipe (bear)"] = &swipe_bear;
        creators["faerie fire (feral)"] = &faerie_fire_feral;
        creators["bear form"] = &bear_form;
        creators["dire bear form"] = &dire_bear_form;
        creators["mangle (bear)"] = &mangle_bear;
        creators["maul"] = &maul;
        creators["bash"] = &bash;
        creators["swipe"] = &swipe;
        creators["lacerate"] = &lacerate;
        creators["demoralizing roar"] = &demoralizing_roar;
        creators["thrash"] = &thrash;
        creators["frenzied regeneration"] = &frenzied_regeneration;
        creators["skull bash"] = &skull_bash;
    }
private:
    static ActionNode* melee(PlayerbotAI* ai)
    {
        return new ActionNode ("melee",
            /*P*/ NextAction::array(0, new NextAction("feral charge - bear"), NULL),
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* feral_charge_bear(PlayerbotAI* ai)
    {
        return new ActionNode ("feral charge - bear",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("reach melee"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* swipe_bear(PlayerbotAI* ai)
    {
        return new ActionNode ("swipe (bear)",
            /*P*/ NULL,
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* faerie_fire_feral(PlayerbotAI* ai)
    {
        return new ActionNode ("faerie fire (feral)",
            /*P*/ NextAction::array(0, new NextAction("feral charge - bear"), NULL),
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* bear_form(PlayerbotAI* ai)
    {
        return new ActionNode ("bear form",
            /*P*/ NULL,
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* dire_bear_form(PlayerbotAI* ai)
    {
        return new ActionNode ("dire bear form",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("bear form"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* mangle_bear(PlayerbotAI* ai)
    {
        return new ActionNode ("mangle (bear)",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("lacerate"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* maul(PlayerbotAI* ai)
    {
        return new ActionNode ("maul",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("melee"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* bash(PlayerbotAI* ai)
    {
        return new ActionNode ("bash",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("melee"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* swipe(PlayerbotAI* ai)
    {
        return new ActionNode ("swipe",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("melee"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* lacerate(PlayerbotAI* ai)
    {
        return new ActionNode ("lacerate",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("maul"), NULL),
            /*C*/ NULL);
    }
    static ActionNode* growl(PlayerbotAI* ai)
    {
        return new ActionNode ("growl",
            /*P*/ NULL,
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* demoralizing_roar(PlayerbotAI* ai)
    {
        return new ActionNode ("demoralizing roar",
            /*P*/ NULL,
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* thrash(PlayerbotAI* ai)
    {
        return new ActionNode ("thrash",
            /*P*/ NULL,
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* frenzied_regeneration(PlayerbotAI* ai)
    {
        return new ActionNode ("frenzied regeneration",
            /*P*/ NULL,
            /*A*/ NULL,
            /*C*/ NULL);
    }
    static ActionNode* skull_bash(PlayerbotAI* ai)
    {
        return new ActionNode ("skull bash",
            /*P*/ NULL,
            /*A*/ NextAction::array(0, new NextAction("bash"), NULL),
            /*C*/ NULL);
    }
};

BearTankDruidStrategy::BearTankDruidStrategy(PlayerbotAI* ai) : FeralDruidStrategy(ai)
{
    actionNodeFactories.Add(new BearTankDruidStrategyActionNodeFactory());
}

NextAction** BearTankDruidStrategy::getDefaultActions()
{
    return NextAction::array(0,
            new NextAction("mangle (bear)", ACTION_NORMAL + 5),
            new NextAction("thrash", ACTION_NORMAL + 4),
            new NextAction("lacerate", ACTION_NORMAL + 3),
            new NextAction("maul", ACTION_NORMAL + 2),
            new NextAction("faerie fire (feral)", ACTION_NORMAL + 1),
            new NextAction("melee", ACTION_NORMAL),
            NULL);
}

void BearTankDruidStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    FeralDruidStrategy::InitTriggers(triggers);

    // Thorns is a non-combat raid buff cast from caster form; it must NOT be
    // wired here. ThornsTrigger has no combat-state check, so wiring it at
    // combat relevance made the bot leave Bear Form (dumping rage/armor) to
    // recast Thorns every time the aura lapsed, then shift back -- a
    // form-dance bug. Thorns stays unwired in combat; cast it pre-pull if
    // ever needed via the non-combat strategy instead.

    triggers.push_back(new TriggerNode(
        "bear form",
        NextAction::array(0, new NextAction("dire bear form", ACTION_HIGH + 8), NULL)));

    triggers.push_back(new TriggerNode(
        "faerie fire (feral)",
        NextAction::array(0, new NextAction("faerie fire (feral)", ACTION_HIGH + 7), NULL)));

    triggers.push_back(new TriggerNode(
        "lose aggro",
        NextAction::array(0, new NextAction("growl", ACTION_HIGH + 8), NULL)));

    triggers.push_back(new TriggerNode(
        "demoralizing roar",
        NextAction::array(0, new NextAction("demoralizing roar", ACTION_NORMAL + 6), NULL)));

    triggers.push_back(new TriggerNode(
        "berserk",
        NextAction::array(0, new NextAction("berserk", ACTION_HIGH + 4), NULL)));

    // Clearcasting (16870) makes Maul free; CastMaulAction::isUseful() also
    // relaxes its rage>=45 gate for this case.
    triggers.push_back(new TriggerNode(
        "clearcasting",
        NextAction::array(0, new NextAction("maul", ACTION_HIGH + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "medium aoe",
        NextAction::array(0, new NextAction("thrash", ACTION_HIGH + 6), new NextAction("demoralizing roar", ACTION_HIGH + 6), new NextAction("swipe (bear)", ACTION_HIGH + 6), NULL)));

    triggers.push_back(new TriggerNode(
        "light aoe",
        NextAction::array(0, new NextAction("thrash", ACTION_HIGH + 5), new NextAction("swipe (bear)", ACTION_HIGH + 5), NULL)));

    triggers.push_back(new TriggerNode(
        "low health",
        NextAction::array(0, new NextAction("frenzied regeneration", ACTION_EMERGENCY), NULL)));

    triggers.push_back(new TriggerNode(
        "skull bash",
        NextAction::array(0, new NextAction("skull bash", ACTION_INTERRUPT + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "bash",
        NextAction::array(0, new NextAction("bash", ACTION_INTERRUPT + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "bash on enemy healer",
        NextAction::array(0, new NextAction("bash on enemy healer", ACTION_INTERRUPT + 1), NULL)));

}
