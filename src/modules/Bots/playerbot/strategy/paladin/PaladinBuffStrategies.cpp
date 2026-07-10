#include "botpch.h"
#include "../../playerbot.h"
#include "PaladinMultipliers.h"
#include "PaladinBuffStrategies.h"

using namespace ai;

void PaladinBuffManaStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "seal",
        NextAction::array(0, new NextAction("seal of insight", 90.0f), NULL)));

    // Cata: Blessing of Wisdom was folded into Blessing of Might
    triggers.push_back(new TriggerNode(
        "blessing on party",
        NextAction::array(0, new NextAction("blessing of might on party", 11.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of might", ACTION_HIGH + 8), NULL)));
}

void PaladinBuffHealthStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "seal",
        NextAction::array(0, new NextAction("seal of insight", 90.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing on party",
        NextAction::array(0, new NextAction("blessing of kings on party", 11.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of kings", ACTION_HIGH + 8), NULL)));
}

void PaladinBuffSpeedStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "crusader aura",
        NextAction::array(0, new NextAction("crusader aura", 40.0f), NULL)));
}

void PaladinBuffDpsStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "seal",
        NextAction::array(0, new NextAction("seal of truth", 89.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "retribution aura",
        NextAction::array(0, new NextAction("retribution aura", 90.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing on party",
        NextAction::array(0, new NextAction("blessing of might on party", 11.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of might", ACTION_HIGH + 8), NULL)));
}

// Cata 4.3.4: the three resistance auras were merged into one Resistance Aura
void PaladinShadowResistanceStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "resistance aura",
        NextAction::array(0, new NextAction("resistance aura", 90.0f), NULL)));
}

void PaladinFrostResistanceStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "resistance aura",
        NextAction::array(0, new NextAction("resistance aura", 90.0f), NULL)));
}

void PaladinFireResistanceStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "resistance aura",
        NextAction::array(0, new NextAction("resistance aura", 90.0f), NULL)));
}


void PaladinBuffArmorStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "devotion aura",
        NextAction::array(0, new NextAction("devotion aura", 90.0f), NULL)));
}

void PaladinBuffThreatStrategy::InitTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "seal",
        NextAction::array(0, new NextAction("seal of insight", 89.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "retribution aura",
        NextAction::array(0, new NextAction("retribution aura", 90.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing on party",
        NextAction::array(0, new NextAction("blessing of kings on party", 11.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "blessing",
        NextAction::array(0, new NextAction("blessing of kings", ACTION_HIGH + 8), NULL)));
}
