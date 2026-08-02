#pragma once

#include "../generic/NonCombatStrategy.h"

namespace ai
{
    class GenericDeathknightNonCombatStrategy : public NonCombatStrategy
    {
    public:
        GenericDeathknightNonCombatStrategy(PlayerbotAI* ai);
        virtual string getName() { return "nc"; }

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
    };
}
