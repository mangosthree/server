#include "botpch.h"
#include "../../playerbot.h"
#include "DruidActions.h"

using namespace ai;

bool CastCasterFormAction::Execute(Event event)
{
    ai->RemoveShapeshift();
    return true;
}

Value<Unit*>* CastEntanglingRootsCcAction::GetTargetValue()
{
    return context->GetValue<Unit*>("cc target", "entangling roots");
}

bool CastEntanglingRootsCcAction::Execute(Event event)
{
    return ai->CastSpell("entangling roots", GetTarget());
}
