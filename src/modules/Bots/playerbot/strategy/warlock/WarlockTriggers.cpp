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

bool MoltenCoreTrigger::IsActive()
{
    return ai->HasAura(71165u, bot) || ai->HasAura(71164u, bot) || ai->HasAura(71162u, bot);
}

bool DecimationTrigger::IsActive()
{
    return ai->HasAura(63167u, bot) || ai->HasAura(63165u, bot);
}

bool ImprovedSoulFireTrigger::IsActive()
{
    return (ai->HasAura(18119u, bot) || ai->HasAura(18120u, bot)) && !ai->HasAura(85383u, bot);
}
