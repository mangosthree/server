#include "botpch.h"
#include "../../playerbot.h"
#include "StatsValues.h"

using namespace ai;

uint8 HealthValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return 100;
    }
    return (static_cast<float> (target->GetHealth()) / target->GetMaxHealth()) * 100;
}

bool IsDeadValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return false;
    }
    return target->GetDeathState() != ALIVE;
}

bool PetIsDeadValue::Calculate()
{
   if (!bot->GetPet())
   {
      uint32 ownerid = bot->GetGUIDLow();
      QueryResult* result = CharacterDatabase.PQuery("SELECT id FROM character_pet WHERE owner = '%u'", ownerid);
      if (!result)
      {
          return false;
      }

      delete result;
      return true;
   }
   if (bot->GetPetGuid() && !bot->GetPet())
   {
       return true;
   }

    return bot->GetPet() && bot->GetPet()->GetDeathState() != ALIVE;
}

bool PetIsHappyValue::Calculate()
{
    /*PetDatabaseStatus status = Pet::GetStatusFromDB(bot);
    if (status == PET_DB_DEAD)
    {
        return true;
    }*/

    return true; // Cata 4.3.4: pet happiness removed
}


uint8 RageValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return 0;
    }
    return (static_cast<float> (target->GetPower(POWER_RAGE)));
}

uint8 EnergyValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return 0;
    }
    return (static_cast<float> (target->GetPower(POWER_ENERGY)));
}

uint8 FocusValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target || !target->GetMaxPower(POWER_FOCUS))
    {
        return 0;
    }
    return (static_cast<float> (target->GetPower(POWER_FOCUS)) / target->GetMaxPower(POWER_FOCUS)) * 100;
}

uint8 ManaValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return 100;
    }
    // Cata: hunters/warriors/rogues/DKs have max mana 0 - avoid NaN and keep
    // "low mana" style triggers quiet for mana-less classes.
    if (!target->GetMaxPower(POWER_MANA))
    {
        return 100;
    }
    return (static_cast<float> (target->GetPower(POWER_MANA)) / target->GetMaxPower(POWER_MANA)) * 100;
}

bool HasManaValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return false;
    }
    return target->GetPower(POWER_MANA);
}

uint8 SoulShardsValue::Calculate()
{
    return bot->GetPower(POWER_SOUL_SHARDS);
}


uint8 HolyPowerValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return 0;
    }
    return (static_cast<float> (target->GetPower(POWER_HOLY_POWER)));
}

uint8 RunicPowerValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return 0;
    }
    return (static_cast<float> (target->GetPower(POWER_RUNIC_POWER))) / 10;
}

uint8 AvailableRunesValue::Calculate()
{
    if (bot->getClass() != CLASS_DEATH_KNIGHT)
    {
        return 0;
    }

    RuneType type;
    if (qualifier == "blood")
    {
        type = RUNE_BLOOD;
    }
    else if (qualifier == "frost")
    {
        type = RUNE_FROST;
    }
    else if (qualifier == "unholy")
    {
        type = RUNE_UNHOLY;
    }
    else if (qualifier == "death")
    {
        type = RUNE_DEATH;
    }
    else
    {
        return 0;
    }

    uint8 count = 0;
    for (uint8 i = 0; i < MAX_RUNES; ++i)
    {
        if (!bot->GetRuneCooldown(i) && bot->GetCurrentRune(i) == type)
        {
            ++count;
        }
    }
    return count;
}

uint8 ComboPointsValue::Calculate()
{
    Unit *target = GetTarget();
    if (!target || target->GetObjectGuid() != bot->GetComboTargetGuid())
    {
        return 0;
    }

    return bot->GetComboPoints();
}

bool IsMountedValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return false;
    }

    return target->IsMounted();
}


bool IsInCombatValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return false;
    }

    return target->IsInCombat();
}

uint8 BagSpaceValue::Calculate()
{
    uint32 totalused = 0, total = 16;
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        if (bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            totalused++;
        }
    }

    uint32 totalfree = 16 - totalused;
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        const Bag* const pBag = (Bag*) bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);
        if (pBag)
        {
            ItemPrototype const* pBagProto = pBag->GetProto();
            if (pBagProto->Class == ITEM_CLASS_CONTAINER && pBagProto->SubClass == ITEM_SUBCLASS_CONTAINER)
            {
                total += pBag->GetBagSize();
                totalfree += pBag->GetFreeSlots();
            }
        }

    }

    return (static_cast<float> (totalused) / total) * 100;
}

uint8 SpeedValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
    {
        return 100;
    }

    return (uint8) (100.0f * target->GetSpeedRate(MOVE_RUN));
}

