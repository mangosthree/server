#pragma once

#include "../Strategy.h"
#include "DeathknightAiObjectContext.h"
#include "../generic/MeleeCombatStrategy.h"

namespace ai
{
    class GenericDeathknightStrategy : public MeleeCombatStrategy
    {
    public:
        GenericDeathknightStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<TriggerNode*> &triggers);
        virtual string getName() { return "deathknight"; }
    };
}
