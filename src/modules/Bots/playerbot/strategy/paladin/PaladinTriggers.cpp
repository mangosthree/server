#include "botpch.h"
#include "../../playerbot.h"
#include "PaladinTriggers.h"
#include "PaladinActions.h"

using namespace ai;

bool SealTrigger::IsActive()
{
    Unit* target = GetTarget();
    return !ai->HasAura("seal of justice", target) &&
        !ai->HasAura("seal of command", target) &&
        !ai->HasAura("seal of truth", target) &&
        !ai->HasAura("seal of righteousness", target) &&
        !ai->HasAura("seal of insight", target);
}

bool CrusaderAuraTrigger::IsActive()
{
    Unit* target = GetTarget();
    return AI_VALUE2(bool, "mounted", "self target") && !ai->HasAura("crusader aura", target);
}

bool HolyWrathTrigger::IsActive()
{
    return AI_VALUE(uint8, "attacker count") >= 2;
}

bool BlessingOfFreedomTrigger::IsActive()
{
    return bot->IsInRoots() || bot->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED);
}
