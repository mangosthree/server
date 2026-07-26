#include "botpch.h"
#include "../../playerbot.h"
#include "../../PlayerbotAIConfig.h"
#include "MageTriggers.h"
#include "MageActions.h"

using namespace ai;

bool MageArmorTrigger::IsActive()
{
    Unit* target = GetTarget();
    return !ai->HasAura("ice armor", target) &&
        !ai->HasAura("frost armor", target) &&
        !ai->HasAura("molten armor", target) &&
        !ai->HasAura("mage armor", target);
}

/// Reads the "arcane blast" self-buff stack count (mirrors
/// shaman::MaelstromWeaponTrigger's Aura::GetStackAmount() approach) instead of
/// the base BuffTrigger !HasAura check, which only ever fires once (stack 1)
/// and caused AB/Barrage to alternate instead of AB stacking to its 4-stack cap.
bool ArcaneBlastTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return false;
    }

    if (AI_VALUE2(bool, "has mana", "self target") && AI_VALUE2(uint8, "mana", "self target") <= sPlayerbotAIConfig.lowMana)
    {
        return false;
    }

    uint32 spellId = AI_VALUE2(uint32, "spell id", "arcane blast");
    if (!spellId)
    {
        return true; // spell id unresolved -- fall through, CastSpellAction::isPossible() no-ops safely
    }

    for (uint32 effect = EFFECT_INDEX_0; effect <= EFFECT_INDEX_2; effect++)
    {
        Aura* aura = target->GetAura(spellId, (SpellEffectIndex)effect);
        if (aura)
        {
            return aura->GetStackAmount() < 4;
        }
    }

    return true; // no stacks yet -- start building
}

bool ArcaneBlastCappedTrigger::IsActive()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return false;
    }

    uint32 spellId = AI_VALUE2(uint32, "spell id", "arcane blast");
    if (!spellId)
    {
        return false;
    }

    for (uint32 effect = EFFECT_INDEX_0; effect <= EFFECT_INDEX_2; effect++)
    {
        Aura* aura = target->GetAura(spellId, (SpellEffectIndex)effect);
        if (aura && aura->GetStackAmount() >= 4)
        {
            return true;
        }
    }

    return false;
}
