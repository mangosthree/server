#include "botpch.h"
#include "../../playerbot.h"
#include "WarlockTriggers.h"
#include "WarlockActions.h"

using namespace ai;

bool DemonArmorTrigger::IsActive()
{
    Unit* target = GetTarget();
    return !ai->HasAura("demon skin", target) &&
        !ai->HasAura("demon armor", target) &&
        !ai->HasAura("fel armor", target);
}

bool ConflagrateTrigger::IsActive()
{
    Unit* target = GetTarget();
    return SpellCanBeCastTrigger::IsActive() && target && ai->HasAura("immolate", target);
}
