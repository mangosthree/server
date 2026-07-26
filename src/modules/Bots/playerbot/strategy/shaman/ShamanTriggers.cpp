#include "botpch.h"
#include "../../playerbot.h"
#include "ShamanTriggers.h"
#include "ShamanActions.h"

using namespace ai;

list<string> ShamanWeaponTrigger::spells;

bool ShamanWeaponTrigger::IsActive()
{
    if (spells.empty())
    {
        spells.push_back("frostbrand weapon");
        spells.push_back("rockbiter weapon");
        spells.push_back("flametongue weapon");
        spells.push_back("earthliving weapon");
        spells.push_back("windfury weapon");
    }

    for (list<string>::iterator i = spells.begin(); i != spells.end(); ++i)
    {
        uint32 spellId = AI_VALUE2(uint32, "spell id", *i);
        if (!spellId)
        {
            continue;
        }

        if (AI_VALUE2(Item*, "item for spell", spellId))
        {
            return true;
        }
    }

    return false;
}

bool ShockTrigger::IsActive()
{
    return SpellTrigger::IsActive() && !ai->HasAura("flame shock", GetTarget());
}

bool LavaBurstTrigger::IsActive()
{
    Unit* target = GetTarget();
    return SpellCanBeCastTrigger::IsActive() && target && ai->HasAura("flame shock", target);
}

bool FireNovaTrigger::IsActive()
{
    Unit* target = GetTarget();
    return AI_VALUE(uint8, "attacker count") >= 3 &&
            SpellCanBeCastTrigger::IsActive() &&
            target && ai->HasAura("flame shock", target);
}

bool MaelstromWeaponTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return false;
    }

    uint32 spellId = AI_VALUE2(uint32, "spell id", "maelstrom weapon");
    if (!spellId)
    {
        return false;
    }

    for (uint32 effect = EFFECT_INDEX_0; effect <= EFFECT_INDEX_2; effect++)
    {
        Aura* aura = target->GetAura(spellId, (SpellEffectIndex)effect);
        if (aura && aura->GetStackAmount() >= 5)
        {
            return true;
        }
    }

    return false;
}
