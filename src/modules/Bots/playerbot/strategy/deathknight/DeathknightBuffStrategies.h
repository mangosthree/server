#pragma once

#include "GenericDeathknightStrategy.h"

namespace ai
{
    /// Blood (tank): keeps Blood Presence up in and out of combat
    class DeathknightBuffThreatStrategy : public Strategy
    {
    public:
        DeathknightBuffThreatStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "bthreat"; }
    };

    /// Frost/Unholy (dps): keeps Unholy Presence up in and out of combat
    class DeathknightBuffDpsStrategy : public Strategy
    {
    public:
        DeathknightBuffDpsStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "bdps"; }
    };
}
