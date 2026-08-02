#include "botpch.h"
#include "../../playerbot.h"
#include "DeathknightTriggers.h"

using namespace ai;

bool RunicPowerAvailableTrigger::IsActive()
{
    return AI_VALUE2(uint8, "runic power", "self target") >= amount;
}

bool RunePairDepletedTrigger::IsActive()
{
    if (bot->getClass() != CLASS_DEATH_KNIGHT)
    {
        return false;
    }

    return bot->IsBaseRuneSlotsOnCooldown(RUNE_BLOOD) ||
        bot->IsBaseRuneSlotsOnCooldown(RUNE_FROST) ||
        bot->IsBaseRuneSlotsOnCooldown(RUNE_UNHOLY);
}

bool RunePairAvailableTrigger::IsActive()
{
    return AI_VALUE2(uint8, "rune count", "frost") >= 2 ||
        AI_VALUE2(uint8, "rune count", "unholy") >= 2 ||
        AI_VALUE2(uint8, "rune count", "death") >= 2;
}
