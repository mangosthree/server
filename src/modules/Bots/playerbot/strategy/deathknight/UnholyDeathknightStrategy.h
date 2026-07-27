#pragma once

#include "GenericDeathknightStrategy.h"

namespace ai
{
    class UnholyDeathknightStrategy : public GenericDeathknightStrategy
    {
    public:
        UnholyDeathknightStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "unholy"; }
        virtual NextAction** getDefaultActions();
        virtual int GetType() { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_DPS | STRATEGY_TYPE_MELEE; }
    };
}
