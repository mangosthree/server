#include "botpch.h"
#include "../../playerbot.h"
#include "HunterTriggers.h"
#include "HunterActions.h"

using namespace ai;

bool HunterNoStingsActiveTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    return target && AI_VALUE2(uint8, "health", "current target") > 40 &&
        !ai->HasAura("serpent sting", target);
}

bool HighFocusTrigger::IsActive()
{
    return AI_VALUE2(uint8, "focus", "self target") >= amount;
}

bool HuntersPetDeadTrigger::IsActive()
{
    return AI_VALUE(bool, "pet dead") && !AI_VALUE2(bool, "mounted", "self target");
}

bool HuntersPetLowHealthTrigger::IsActive()
{
    Unit* pet = AI_VALUE(Unit*, "pet target");
    return pet && AI_VALUE2(uint8, "health", "pet target") < 40 &&
        !AI_VALUE2(bool, "dead", "pet target") && !AI_VALUE2(bool, "mounted", "self target");
}

bool FeignDeathTrigger::IsActive()
{
    if (!bot->hasUnitState(UNIT_STAT_DIED))
    {
        return false;
    }

    if (AI_VALUE(uint8, "attacker count") > 0)
    {
        return false;
    }

    Unit::AuraList const& auras = bot->GetAurasByType(SPELL_AURA_FEIGN_DEATH);
    if (auras.empty())
    {
        return false;
    }

    Aura* aura = auras.front();
    int32 maxDuration = aura->GetAuraMaxDuration();
    int32 remaining = aura->GetAuraDuration();

    if (maxDuration > 0)
    {
        return (maxDuration - remaining) >= 5000;
    }

    return true;
}


