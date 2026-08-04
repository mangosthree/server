#include "botpch.h"
#include "../../playerbot.h"
#include "PriestActions.h"

using namespace ai;

bool CastHolyNovaAction::isUseful()
{
    if (!CastSpellAction::isUseful() || ai->HasAura("shadowform", AI_VALUE(Unit*, "self target")))
    {
        return false;
    }
    if (!ai->HasStrategy("cautious"))
    {
        return true;
    }
    return !ai->HasNonCombatantInRange(10.0f);
}

