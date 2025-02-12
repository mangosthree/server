/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include "Common.h"
#include "WorldPacket.h"
#include "Log.h"
#include "Corpse.h"
#include "GameObject.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "WorldSession.h"
#include "Item.h"
#include "LootMgr.h"
#include "Object.h"
#include "Group.h"
#include "World.h"
#include "Util.h"
#include "Unit.h"
#include "DBCStores.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

void WorldSession::HandleAutostoreLootItemOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: %s", LookupOpcodeName(recv_data.GetOpcode()));
    Player*  player = GetPlayer();
    ObjectGuid lguid = player->GetLootGuid();
    Loot* loot = nullptr;
    uint8 lootSlot = 0;

    recv_data >> lootSlot;

    if (lguid.IsGameObject())
    {
        GameObject* go = player->GetMap()->GetGameObject(lguid);

        /* Checking if the player is in range of the object. */
        if (!go || ((go->GetOwnerGuid() != _player->GetObjectGuid() && go->GetGoType() != GAMEOBJECT_TYPE_FISHINGHOLE) &&
            !go->IsWithinDistInMap(_player, INTERACTION_DISTANCE)))
        {
            player->SendLootRelease(lguid);
            return;
        }

        loot = &go->loot;
    }
    else if (lguid.IsItem())
    {
        Item* pItem = player->GetItemByGuid(lguid);
        if (!pItem)
        {
            player->SendLootRelease(lguid);
            return;
        }

        loot = &pItem->loot;
    }
    else if (lguid.IsCorpse())
    {
        // need to change this function to ObjectAccessor::GetCorpse() when implemented
        Corpse* bones = player->GetMap()->GetCorpse(lguid);
        if (!bones)
        {
            player->SendLootRelease(lguid);
            return;
        }

        loot = &bones->loot;
    }
    else
    {
        Creature* creature = GetPlayer()->GetMap()->GetCreature(lguid);

        /* Checking if the player is a rogue and if the creature is alive. */
        bool lootAllowed = creature && creature->IsAlive() == (player->getClass() == CLASS_ROGUE && creature->loot.loot_type == LOOT_PICKPOCKETING);
        if (!lootAllowed || !creature->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
        {
            player->SendLootRelease(lguid);
            return;
        }

        loot = &creature->loot;
    }

    /* Checking if the loot is looted and if the guid is an item. If it is, it will release the loot. */
    if (loot->isLooted() && lguid.IsItem())
    {
        player->GetSession()->DoLootRelease(lguid);
    }

    QuestItem* qitem = NULL;
    QuestItem* ffaitem = NULL;
    QuestItem* conditem = NULL;
    QuestItem* currency = NULL;

    LootItem* item = loot->LootItemInSlot(lootSlot, player, &qitem, &ffaitem, &conditem, &currency);

    if (!item)
    {
        player->SendEquipError(EQUIP_ERR_ALREADY_LOOTED, NULL, NULL);
        return;
    }

    // questitems use the blocked field for other purposes
    // ToDo: this call is handled else where, not here.
    if (!qitem && item->is_blocked)
    {
        player->SendLootRelease(lguid);
        return;
    }

    if (currency)
    {
        if (CurrencyTypesEntry const * currencyEntry = sCurrencyTypesStore.LookupEntry(item->itemid))
        {
            player->ModifyCurrencyCount(item->itemid, int32(item->count * currencyEntry->GetPrecision()));
        }

        player->SendNotifyLootItemRemoved(lootSlot, true);
        currency->is_looted = true;
        --loot->unlootedCount;
        return;
    }

    ItemPosCountVec dest;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item->itemid, item->count);
    if (msg == EQUIP_ERR_OK)
    {
        Item* newitem = player->StoreNewItem(dest, item->itemid, true, item->randomPropertyId);

        if (qitem)
        {
            qitem->is_looted = true;
            // freeforall is 1 if everyone's supposed to get the quest item.
            if (item->freeforall || loot->GetPlayerQuestItems().size() == 1)
            {
                player->SendNotifyLootItemRemoved(lootSlot);
            }
            else
            {
                loot->NotifyQuestItemRemoved(qitem->index);
            }
        }
        else
        {
            if (ffaitem)
            {
                // freeforall case, notify only one player of the removal
                ffaitem->is_looted = true;
                player->SendNotifyLootItemRemoved(lootSlot);
            }
            else
            {
                // not freeforall, notify everyone
                if (conditem)
                {
                    conditem->is_looted = true;
                }
                loot->NotifyItemRemoved(lootSlot);
            }
        }

        // if only one person is supposed to loot the item, then set it to looted
        if (!item->freeforall)
        {
            item->is_looted = true;
        }

        --loot->unlootedCount;

        player->SendNewItem(newitem, uint32(item->count), false, false, true);

#ifdef ENABLE_ELUNA
        if (Eluna* e = player->GetEluna())
        {
            e->OnLootItem(player, newitem, item->count, lguid);
        }
#endif /* ENABLE_ELUNA */

        player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM, item->itemid, item->count);
        player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE, loot->loot_type, item->count);
        player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM, item->itemid, item->count);
    }
    else
    {
        player->SendEquipError(msg, NULL, NULL, item->itemid);
    }
}

void WorldSession::HandleLootMoneyOpcode(WorldPacket & /*recv_data*/)
{
    DEBUG_LOG("WORLD: CMSG_LOOT_MONEY");

    Player* player = GetPlayer();
    ObjectGuid guid = player->GetLootGuid();
    if (!guid)
    {
        return;
    }

    Loot* pLoot = NULL;
    Item* pItem = NULL;
    bool shareMoney = true;

    switch (guid.GetHigh())
    {
        case HIGHGUID_GAMEOBJECT:
        {
            GameObject* pGameObject = GetPlayer()->GetMap()->GetGameObject(guid);

            // not check distance for GO in case owned GO (fishing bobber case, for example)
            if (pGameObject && (pGameObject->GetOwnerGuid() == _player->GetObjectGuid() || pGameObject->IsWithinDistInMap(_player, INTERACTION_DISTANCE)))
            {
                pLoot = &pGameObject->loot;
            }

            break;
        }
        case HIGHGUID_CORPSE:                               // remove insignia ONLY in BG
        {
            Corpse* bones = _player->GetMap()->GetCorpse(guid);

            if (bones && bones->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
            {
                pLoot = &bones->loot;
                shareMoney = false;

                // Used by Eluna
                #ifdef ENABLE_ELUNA
                if (Eluna* e = player->GetEluna())
                {
                    e->OnLootMoney(player, pLoot->gold);
                }
                #endif /* ENABLE_ELUNA */
            }

            break;
        }
        case HIGHGUID_ITEM:
        {
            if (Item* item = GetPlayer()->GetItemByGuid(guid))
            {
                pLoot = &item->loot;
                shareMoney = false;
            }
            break;
        }
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
        {
            Creature* pCreature = GetPlayer()->GetMap()->GetCreature(guid);

            bool ok_loot = pCreature && pCreature->IsAlive() == (player->getClass() == CLASS_ROGUE && pCreature->lootForPickPocketed);

            if (ok_loot && pCreature->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
            {
                pLoot = &pCreature->loot;
                if (pCreature->IsAlive())
                {
                    shareMoney = false;
                }
            }

            break;
        }
        default:
            return;                                         // unlootable type
    }

    if (pLoot)
    {
        pLoot->NotifyMoneyRemoved();
        if (shareMoney && player->GetGroup())           // item, pickpocket and players can be looted only single player
        {
            Group* group = player->GetGroup();

            std::vector<Player*> playersNear;
            for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
            {
                Player* playerGroup = itr->getSource();
                if (!playerGroup)
                {
                    continue;
                }

                if (player->IsAtGroupRewardDistance(playerGroup))
                {
                    playersNear.push_back(playerGroup);
                }
            }

            uint64 money_per_player = uint32((pLoot->gold) / (playersNear.size()));

            for (std::vector<Player*>::const_iterator i = playersNear.begin(); i != playersNear.end(); ++i)
            {
                (*i)->ModifyMoney(money_per_player);
                (*i)->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY, money_per_player);

                WorldPacket data(SMSG_LOOT_MONEY_NOTIFY, 4 + 1);
                data << uint32(money_per_player);
                // Controls the text displayed in chat. 0 is "Your share is..." and 1 is "You loot..."
                data << uint8(playersNear.size() <= 1);
                (*i)->SendDirectMessage(&data);
            }
        }
        else
        {
            player->ModifyMoney(pLoot->gold);
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY, pLoot->gold);

            WorldPacket data(SMSG_LOOT_MONEY_NOTIFY, 4 + 1);
            data << uint32(pLoot->gold);
            data << uint8(1); // 1 is "you loot..."
            SendPacket(&data);
        }

        // Used by Eluna
#ifdef ENABLE_ELUNA
        if (Eluna* e = player->GetEluna())
        {
            e->OnLootMoney(player, pLoot->gold);
        }
#endif /* ENABLE_ELUNA */

        pLoot->gold = 0;

        /* Checking if the loot is looted and if the guid is an item. If it is, it will release the loot. */
        if (pLoot->isLooted() && guid.IsItem())
        {
            player->GetSession()->DoLootRelease(guid);
        }
    }
}

void WorldSession::HandleLootOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: CMSG_LOOT");

    ObjectGuid guid;
    recv_data >> guid;

    // Check possible cheat
    if (!GetPlayer()->IsAlive() || !guid.IsCreatureOrVehicle())
    {
        return;
    }

    GetPlayer()->SendLoot(guid, LOOT_CORPSE);

    /* Checking if the player is casting a spell, and if so, it is interrupting it. */
    if (GetPlayer()->IsNonMeleeSpellCasted(false))
    {
        GetPlayer()->InterruptNonMeleeSpells(false);
    }
}

void WorldSession::HandleLootReleaseOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: CMSG_LOOT_RELEASE");

    // cheaters can modify lguid to prevent correct apply loot release code and re-loot
    // use internal stored guid
    recv_data.read_skip<uint64>();                          // guid;

    if (ObjectGuid lootGuid = GetPlayer()->GetLootGuid())
    {
        DoLootRelease(lootGuid);
    }
}

void WorldSession::DoLootRelease(ObjectGuid lguid)
{
    Player* player = GetPlayer();
    Loot* loot;

    player->SetLootGuid(ObjectGuid());
    player->SendLootRelease(lguid);

    player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_LOOTING);

    if (!player->IsInWorld())
    {
        return;
    }

    if (lguid.IsGameObject())
    {
        GameObject* go = GetPlayer()->GetMap()->GetGameObject(lguid);

        /* Checking if the player is in range of the object. */
        if (!go || ((go->GetOwnerGuid() != _player->GetObjectGuid() && go->GetGoType() != GAMEOBJECT_TYPE_FISHINGHOLE) &&
            !go->IsWithinDistInMap(_player, INTERACTION_DISTANCE)))
            return;

        loot = &go->loot;

        /* The above code is checking if the gameobject is a door, if it is then it uses the door. If
         * the gameobject is a fishing node, then it checks if it has been looted, if it has then it
         * checks if the max success opens is greater than 0, if it is then it sets the loot state to
         * just deactivated, if not then it sets the loot state to ready. If the gameobject is not a
         * door or a fishing node, then it sets the loot state to activated.
         */
        if (go->GetGoType() == GAMEOBJECT_TYPE_DOOR)
        {
            go->UseDoorOrButton();
        }
        else if (loot->isLooted() || go->GetGoType() == GAMEOBJECT_TYPE_FISHINGNODE)
        {
            if (go->GetUseCount() >= go->GetGoType() == GAMEOBJECT_TYPE_FISHINGHOLE)
            {
                go->AddUse();
                /* Checking if the maxSuccessOpens is set, if it is then it sets the loot state to
                 * GO_JUST_DEACTIVATED, otherwise it sets it to GO_READY.
                 */
                if (go->GetGOInfo()->fishinghole.maxSuccessOpens)
                {
                    go->SetLootState(GO_JUST_DEACTIVATED);
                }
                else
                {
                    go->SetLootState(GO_READY);
                }
            }
            else
            {
                go->SetLootState(GO_JUST_DEACTIVATED);
            }

            /* Clearing the loot vector. */
            loot->clear();
        }
        else
        {
            go->SetLootState(GO_ACTIVATED);
        }
    }
    else if (lguid.IsCorpse()) // ONLY remove insignia at BG
    {
        Corpse* corpse = _player->GetMap()->GetCorpse(lguid);
        if (!corpse || !corpse->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
        {
            return;
        }

        loot = &corpse->loot;

        /* Checking if the corpse is looted, if it is, it clears the loot and removes the lootable flag. */
        if (loot->isLooted())
        {
            /* Removing the lootable flag from the corpse. */
            loot->clear();
            corpse->RemoveFlag(CORPSE_FIELD_DYNAMIC_FLAGS, CORPSE_DYNFLAG_LOOTABLE);
        }
    }
    else if (lguid.IsItem())
    {
        Item* pItem = player->GetItemByGuid(lguid);
        if (!pItem)
        {
            return;
        }

        /* Checking if the item is prospectable or millable, if it is, it will clear the loot, then it
         * will check if the count is greater than 5, if it is, it will set the count to 5, then it
         * will destroy the item.
         */
        if (pItem->loot.loot_type == ITEM_FLAG_PROSPECTABLE || pItem->loot.loot_type == ITEM_FLAG_MILLABLE)
        {
            /* Clearing the loot vector of the item. */
            pItem->loot.clear();

            uint32 count = pItem->GetCount();
            if (count > 5)
            {
                count = 5;
            }

            /* Destroying the item count. */
            player->DestroyItemCount(pItem, count, true);
        }
        else
        {
            /* Checking if the item is looted or not. If it is looted, it will destroy the item. */
            if (pItem->loot.isLooted() || !pItem->loot.loot_type == ITEM_FLAG_LOOTABLE)
            {
                /* Destroying the item in the player's inventory. */
                player->DestroyItem(pItem->GetBagSlot(), pItem->GetSlot(), true);
            }
        }
        return; // item can be looted only single player
    }
    else
    {
        Creature* creature = GetPlayer()->GetMap()->GetCreature(lguid);

        /* Checking if the creature is alive and if the player is a rogue. */
        bool lootAllowed = creature && creature->IsAlive() == (player->getClass() == CLASS_ROGUE && creature->loot.loot_type == LOOT_PICKPOCKETING);
        if (!lootAllowed || !creature->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
        {
            return;
        }

        loot = &creature->loot;

        /* Checking if the creature is looted, if it is then it removes the lootable flag and clears the loot. */
        if (loot->isLooted())
        {
            creature->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);

            if (!creature->IsAlive())
            {
                creature->AllLootRemovedFromCorpse();
            }

            loot->clear();
        }
        else
        {
            /* Forcing the creature to update its dynamic flags. */
            creature->ForceValuesUpdateAtIndex(UNIT_DYNAMIC_FLAGS);
        }
    }

    /* Removing the player from the looter list. */
    loot->RemoveLooter(player->GetObjectGuid());
}

void WorldSession::HandleLootMasterGiveOpcode(WorldPacket& recv_data)
{
    uint8 slotid;
    ObjectGuid lootguid;
    ObjectGuid target_playerguid;

    recv_data >> lootguid >> slotid >> target_playerguid;

    if (!_player->GetGroup() || _player->GetGroup()->GetLooterGuid() != _player->GetObjectGuid())
    {
        _player->SendLootRelease(GetPlayer()->GetLootGuid());
        return;
    }

    Player* target = sObjectAccessor.FindPlayer(target_playerguid);
    if (!target)
    {
        return;
    }

    DEBUG_LOG("WorldSession::HandleLootMasterGiveOpcode (CMSG_LOOT_MASTER_GIVE, 0x02A3) Target = %s [%s].", target_playerguid.GetString().c_str(), target->GetName());

    if (_player->GetLootGuid() != lootguid)
    {
        return;
    }

    /* Checking if the player is in the same raid as the target and if the player is in the same map as the target. */
    if (!_player->IsInSameRaidWith(target->ToPlayer()) || !_player->IsInMap(target))
    {
        sLog.outBasic("MasterLootItem: Player %s tried to give an item to ineligible player %s!", GetPlayer()->GetName(), target->GetName());
        return;
    }

    Loot* pLoot = NULL;

    if (GetPlayer()->GetLootGuid().IsCreatureOrVehicle())
    {
        Creature* pCreature = GetPlayer()->GetMap()->GetCreature(lootguid);
        if (!pCreature)
        {
            return;
        }

        pLoot = &pCreature->loot;
    }
    else if (GetPlayer()->GetLootGuid().IsGameObject())
    {
        GameObject* pGO = GetPlayer()->GetMap()->GetGameObject(lootguid);
        if (!pGO)
        {
            return;
        }

        pLoot = &pGO->loot;
    }

    if (!pLoot)
    {
        return;
    }

    if (slotid >= pLoot->items.size())
    {
        DEBUG_LOG("AutoLootItem: Player %s might be using a hack! (slot %d, size %zu)", GetPlayer()->GetName(), slotid, (unsigned long)pLoot->items.size());
        return;
    }

    LootItem& item = pLoot->items[slotid];
    if (item.currency)
    {
        sLog.outError("WorldSession::HandleLootMasterGiveOpcode: player %s tried to give currency via master loot! Hack alert! Slot %u, currency id %u",
            GetPlayer()->GetName(), slotid, item.itemid);
        return;
    }

    ItemPosCountVec dest;
    InventoryResult msg = target->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item.itemid, item.count);
    if (!item.AllowedForPlayer(target, nullptr))
    {
        msg = EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;
    }

    // ToDo: fix the equp error messages
    if (msg != EQUIP_ERR_OK)
    {
        target->SendEquipError(msg, NULL, NULL, item.itemid);

        // send duplicate of error massage to master looter
        _player->SendEquipError(msg, NULL, NULL, item.itemid);
        return;
    }

    // now move item from loot to target inventory
    Item* newitem = target->StoreNewItem(dest, item.itemid, true, item.randomPropertyId);
    target->SendNewItem(newitem, uint32(item.count), false, false, true);
    target->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM, item.itemid, item.count);
    target->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE, pLoot->loot_type, item.count);
    target->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM, item.itemid, item.count);

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = target->GetEluna())
    {
        e->OnLootItem(target, newitem, item.count, lootguid);
    }
#endif /* ENABLE_ELUNA */

    // mark as looted
    item.count = 0;
    item.is_looted = true;

    pLoot->NotifyItemRemoved(slotid);
    --pLoot->unlootedCount;
}
