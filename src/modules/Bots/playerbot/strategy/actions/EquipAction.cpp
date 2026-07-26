#include "botpch.h"
#include "../../playerbot.h"
#include "EquipAction.h"

#include "../values/ItemCountValue.h"

using namespace ai;

bool EquipAction::Execute(Event event)
{
    string text = event.getParam();
    ItemIds ids = chat->parseItems(text);
    EquipItems(ids);
    return true;
}

void EquipAction::EquipItems(ItemIds ids)
{
    for (ItemIds::iterator i =ids.begin(); i != ids.end(); i++)
    {
        FindItemByIdVisitor visitor(*i);
        EquipItem(&visitor);
    }
}

void EquipAction::EquipItem(FindItemVisitor* visitor)
{
    IterateItems(visitor);
    list<Item*> items = visitor->GetResult();
    if (!items.empty()) EquipItem(**items.begin());
}


void EquipAction::EquipItem(Item& item)
{
    uint8 bagIndex = item.GetBagSlot();
    uint8 slot = item.GetSlot();

    // Cata 4.3.4: ammo removed; INVTYPE_AMMO no longer special-cased.
    WorldPacket* const packet = new WorldPacket(CMSG_AUTOEQUIP_ITEM, 2);
    *packet << bagIndex << slot;
    bot->GetSession()->QueuePacket(packet);

    ostringstream out; out << "equipping " << chat->formatItem(item.GetProto());
    ai->TellMaster(out);
}
