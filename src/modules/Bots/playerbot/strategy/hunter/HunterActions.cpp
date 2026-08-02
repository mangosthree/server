#include "botpch.h"
#include "../../playerbot.h"
#include "../actions/GenericActions.h"
#include "HunterActions.h"

using namespace ai;

bool CastSerpentStingAction::isUseful()
{
    return AI_VALUE2(uint8, "health", "current target") > 50;
}

bool CastAspectOfTheCheetahAction::isUseful()
{
    return !ai->HasAnyAuraOf(GetTarget(), "aspect of the cheetah", "aspect of the pack", NULL);
}

Value<Unit*>* CastFreezingTrap::GetTargetValue()
{
    return context->GetValue<Unit*>("cc target", "freezing trap");
}

