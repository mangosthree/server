#include "botpch.h"
#include "../../playerbot.h"
#include "MageActions.h"

using namespace ai;

Value<Unit*>* CastPolymorphAction::GetTargetValue()
{
    return context->GetValue<Unit*>("cc target", getName());
}

bool CastFlamestrikeAction::isUseful()
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
    if (target && ai->HasNonCombatantInRange(5.0f,
        target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()))
    {
        return false;
    }
    return true;
}

bool CastFrostNovaAction::isUseful()
{
    if (!CastSpellAction::isUseful())
    {
        return false;
    }
    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
    {
        return false;
    }
    if (target->getVictim() != bot &&
        (!bot->GetGroup() || !target->getVictim() ||
            !target->getVictim()->GetObjectGuid().IsPlayer()))
    {
        return false;
    }
    if (!ai->HasStrategy("cautious"))
    {
        return true;
    }
    if (ai->HasNonCombatantInRange(10.0f) ||
        AI_VALUE2(float, "distance", GetTargetName()) > sPlayerbotAIConfig.tooCloseDistance)
    {
        return false;
    }
    return true;
}

bool CastBlizzardAction::isUseful()
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

bool CastBlastWaveAction::isUseful()
{
    if (!CastSpellAction::isUseful())
    {
        return false;
    }
    if (!ai->HasStrategy("cautious"))
    {
        return true;
    }
    if (ai->HasNonCombatantInRange(10.0f))
    {
        return false;
    }
    return true;
}
