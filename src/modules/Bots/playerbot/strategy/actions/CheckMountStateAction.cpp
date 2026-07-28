#include "botpch.h"
#include "../../playerbot.h"
#include "CheckMountStateAction.h"

using namespace ai;

uint64 extractGuid(WorldPacket& packet);

bool CheckMountStateAction::Execute(Event event)
{
    Player* master = GetMaster();
    if (!bot->GetGroup() || !master)
    {
        return false;
    }

    if (bot->IsTaxiFlying())
    {
        return false;
    }
    if (master->IsTaxiFlying())
    {
        return false;  // not the kind of mounting this is supposed to react to
    }

    if (master->IsMounted() && !bot->IsMounted())
    {
        return Mount();
    }
    else if (!master->IsMounted() && bot->IsMounted())
    {
        WorldPacket emptyPacket;
        bot->GetSession()->HandleCancelMountAuraOpcode(emptyPacket);
        return true;
    }
    return false;
}

bool CheckMountStateAction::Mount()
{
    if (bot->IsNonMeleeSpellCasted(true))
    {
        return false;
    }

    Player* master = GetMaster();
    ai->RemoveShapeshift();
    Unit::AuraList const& auras = master->GetAurasByType(SPELL_AURA_MOUNTED);
    if (auras.empty())
    {
        return false;
    }

    const SpellEntry* masterSpell = auras.front()->GetSpellProto();

    // A dual-purpose mount's aura carries both a ground (index 1) and a flight
    // (index 2) speed bonus; compare only the one the master is actually
    // using, or a grounded bot's ground-speed mount always loses to the much
    // larger flight bonus and the bot never mounts while the master flies.
    SpellEffectIndex speedIndex = master->IsFlying() ? SpellEffectIndex(2) : SpellEffectIndex(1);
    int32 masterSpeed = masterSpell->CalculateSimpleValue(speedIndex);

    map<int32, vector<uint32> > spells;
    for (PlayerSpellMap::iterator itr = bot->GetSpellMap().begin(); itr != bot->GetSpellMap().end(); ++itr)
    {
        uint32 spellId = itr->first;
        if (itr->second.state == PLAYERSPELL_REMOVED || itr->second.disabled || IsPassiveSpell(spellId))
  {
      continue;
  }

        const SpellEntry* spellInfo = sSpellStore.LookupEntry(spellId);
        if (!spellInfo || spellInfo->GetEffectApplyAuraNameByIndex(SpellEffectIndex(0)) != SPELL_AURA_MOUNTED)
  {
      continue;
  }

        int32 effect = spellInfo->CalculateSimpleValue(speedIndex);
        if (effect < masterSpeed)
  {
      continue;
  }

        spells[effect].push_back(spellId);
    }

    for (map<int32, vector<uint32> >::iterator i = spells.begin(); i != spells.end(); ++i)
    {
        vector<uint32>& ids = i->second;
        int index = urand(0, ids.size() - 1);
        if (index >= ids.size())
  {
      continue;
  }

        ai->CastSpell(ids[index], bot);
        ai->SetNextCheckDelay(4000);
        return true;
    }

    return false;
}
