#include "botpch.h"
#include "../../playerbot.h"
#include "WarlockActions.h"

using namespace ai;

bool CastRainOfFireAction::isUseful()
{
    if (!CastSpellAction::isUseful())
    {
        return false;
    }
    if (!ai->HasStrategy("cautious"))
    {
        return true;
    }
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target || ai->HasNonCombatantInRange(8.0f,
        target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()))
    {
        return false;
    }
    return true;
}
