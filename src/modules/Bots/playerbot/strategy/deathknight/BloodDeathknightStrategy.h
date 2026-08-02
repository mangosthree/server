#pragma once

#include "GenericDeathknightStrategy.h"

namespace ai
{
    class BloodDeathknightStrategy : public GenericDeathknightStrategy
    {
    public:
        BloodDeathknightStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "tank"; }
        virtual NextAction** getDefaultActions();
        virtual int GetType() { return STRATEGY_TYPE_TANK | STRATEGY_TYPE_MELEE; }
    };
}
