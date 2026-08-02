#pragma once

#include "GenericDeathknightStrategy.h"

namespace ai
{
    class FrostDeathknightStrategy : public GenericDeathknightStrategy
    {
    public:
        FrostDeathknightStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "dps"; }
        virtual NextAction** getDefaultActions();
        virtual int GetType() { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_DPS | STRATEGY_TYPE_MELEE; }
    };
}
