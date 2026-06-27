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
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Item.h"
#include "UpdateData.h"
#include "Chat.h"

/**
 * @file ItemHandlerVendor.cpp
 * @brief Cohesion split of ItemHandler.cpp -- vendor and bank opcode handlers: sell/buyback/buy, list-inventory, bag/bank auto-store, buy bank slot and set-ammo. Same WorldSession class; no behaviour change. CMake file(GLOB) picks this file up automatically; WorldSession.h is unchanged.
 */

/**
 * @brief Sells an item stack to a vendor.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleSellItemOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_SELL_ITEM");

    ObjectGuid vendorGuid;
    ObjectGuid itemGuid;
    uint32 count;

    recv_data >> vendorGuid;
    recv_data >> itemGuid;
    recv_data >> count;

    if (!itemGuid)
    {
        return;
    }

    Creature* pCreature = GetPlayer()->GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_VENDOR);
    if (!pCreature)
    {
        DEBUG_LOG("WORLD: HandleSellItemOpcode - %s not found or you can't interact with him.", vendorGuid.GetString().c_str());
        _player->SendSellError(SELL_ERR_CANT_FIND_VENDOR, NULL, itemGuid, 0);
        return;
    }

    Item* pItem = _player->GetItemByGuid(itemGuid);
    if (pItem)
    {
        // prevent sell not owner item
        if (_player->GetObjectGuid() != pItem->GetOwnerGuid())
        {
            _player->SendSellError(SELL_ERR_CANT_SELL_ITEM, pCreature, itemGuid, 0);
            return;
        }

        // prevent sell non empty bag by drag-and-drop at vendor's item list
        if (pItem->IsBag() && !((Bag*)pItem)->IsEmpty())
        {
            _player->SendSellError(SELL_ERR_CANT_SELL_ITEM, pCreature, itemGuid, 0);
            return;
        }

        // prevent sell currently looted item
        if (_player->GetLootGuid() == pItem->GetObjectGuid())
        {
            _player->SendSellError(SELL_ERR_CANT_SELL_ITEM, pCreature, itemGuid, 0);
            return;
        }

        // special case at auto sell (sell all)
        if (count == 0)
        {
            count = pItem->GetCount();
        }
        else
        {
            // prevent sell more items that exist in stack (possible only not from client)
            if (count > pItem->GetCount())
            {
                _player->SendSellError(SELL_ERR_CANT_SELL_ITEM, pCreature, itemGuid, 0);
                return;
            }
        }

        ItemPrototype const* pProto = pItem->GetProto();
        if (pProto)
        {
            if (pProto->SellPrice > 0)
            {
                uint64 money = pProto->SellPrice * count;
                if (_player->GetMoney() >= MAX_MONEY_AMOUNT - money) // prevent exceeding gold limit
                {
                    _player->SendEquipError(EQUIP_ERR_TOO_MUCH_GOLD, nullptr, nullptr);
                    _player->SendSellError(SELL_ERR_UNK, pCreature, itemGuid, 0);
                    return;
                }

                if (count < pItem->GetCount())              // need split items
                {
                    Item* pNewItem = pItem->CloneItem(count, _player);
                    if (!pNewItem)
                    {
                        sLog.outError("WORLD: HandleSellItemOpcode - could not create clone of item %u; count = %u", pItem->GetEntry(), count);
                        _player->SendSellError(SELL_ERR_CANT_SELL_ITEM, pCreature, itemGuid, 0);
                        return;
                    }

                    pItem->SetCount(pItem->GetCount() - count);
                    _player->ItemRemovedQuestCheck(pItem->GetEntry(), count);
                    if (_player->IsInWorld())
                    {
                        pItem->SendCreateUpdateToPlayer(_player);
                    }
                    pItem->SetState(ITEM_CHANGED, _player);

                    _player->AddItemToBuyBackSlot(pNewItem);
                    if (_player->IsInWorld())
                    {
                        pNewItem->SendCreateUpdateToPlayer(_player);
                    }
                }
                else
                {
                    _player->ItemRemovedQuestCheck(pItem->GetEntry(), pItem->GetCount());
                    _player->RemoveItem(pItem->GetBagSlot(), pItem->GetSlot(), true);
                    pItem->RemoveFromUpdateQueueOf(_player);
                    _player->AddItemToBuyBackSlot(pItem);
                }

                _player->ModifyMoney(money);
                _player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS, money);
            }
            else
            {
                _player->SendSellError(SELL_ERR_CANT_SELL_ITEM, pCreature, itemGuid, 0);
            }
            return;
        }
    }
    _player->SendSellError(SELL_ERR_CANT_FIND_ITEM, pCreature, itemGuid, 0);
    return;
}

/**
 * @brief Buys back a previously sold item.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleBuybackItem(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_BUYBACK_ITEM");
    ObjectGuid vendorGuid;
    uint32 slot;

    recv_data >> vendorGuid >> slot;

    Creature* pCreature = GetPlayer()->GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_VENDOR);
    if (!pCreature)
    {
        DEBUG_LOG("WORLD: HandleBuybackItem - %s not found or you can't interact with him.", vendorGuid.GetString().c_str());
        _player->SendSellError(SELL_ERR_CANT_FIND_VENDOR, NULL, ObjectGuid(), 0);
        return;
    }

    Item* pItem = _player->GetItemFromBuyBackSlot(slot);
    if (pItem)
    {
        uint64 price = _player->GetUInt32Value(PLAYER_FIELD_BUYBACK_PRICE_1 + slot - BUYBACK_SLOT_START);
        if (_player->GetMoney() < price)
        {
            _player->SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, pCreature, pItem->GetEntry(), 0);
            return;
        }

        ItemPosCountVec dest;
        InventoryResult msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, pItem, false);
        if (msg == EQUIP_ERR_OK)
        {
            _player->ModifyMoney(-(int64)price);
            _player->RemoveItemFromBuyBackSlot(slot, false);
            _player->ItemAddedQuestCheck(pItem->GetEntry(), pItem->GetCount());
            _player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM, pItem->GetEntry(), pItem->GetCount());
            _player->StoreItem(dest, pItem, true);
        }
        else
        {
            _player->SendEquipError(msg, pItem, NULL);
        }
        return;
    }
    else
    {
        _player->SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, 0, 0);
    }
}

/**
 * @brief Buys an item from a vendor into automatic storage.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleBuyItemOpcode(WorldPacket& recv_data)
{
    ObjectGuid vendorGuid, bagGuid;
    uint32 item, slot, count;
    uint8 type, bagSlot;

    recv_data >> vendorGuid >> type >> item >> slot >> count >> bagGuid >> bagSlot;
    DEBUG_LOG("WORLD: Received opcode CMSG_BUY_ITEM, vendorguid: %s, type: %u, item: %u, slot: %u, count: %u, bagGuid: %s, bagSlog: %u",
        vendorGuid.GetString().c_str(), type, item, slot, count, bagGuid.GetString().c_str(), bagSlot);

    // client side expected counting from 1, and we send to client vendorslot+1 already
    if (slot > 0)
    {
        --slot;
    }
    else
    {
        return;                                             // cheating
    }

    switch (type)
    {
        case VENDOR_ITEM_TYPE_NONE:
            break;
        case VENDOR_ITEM_TYPE_ITEM:
        {
            uint8 bag = NULL_BAG;                           // init for case invalid bagGUID

            // find bag slot by bag guid
            if (bagGuid == _player->GetObjectGuid())
            {
                bag = INVENTORY_SLOT_BAG_0;
            }
            else
            {
                for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
                {
                    if (Bag* pBag = (Bag*)_player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    {
                        if (bagGuid == pBag->GetObjectGuid())
                        {
                            bag = i;
                            break;
                        }
                    }
                }
            }

            GetPlayer()->BuyItemFromVendorSlot(vendorGuid, slot, item, count, bag, bagSlot);
            break;
        }
        case VENDOR_ITEM_TYPE_CURRENCY:
        {
            GetPlayer()->BuyCurrencyFromVendorSlot(vendorGuid, slot, item, count);
            break;
        }
    }
}

/**
 * @brief Requests the inventory list of a vendor.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleListInventoryOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;

    recv_data >> guid;

    if (!GetPlayer()->IsAlive())
    {
        return;
    }

    DEBUG_LOG("WORLD: Received opcode CMSG_LIST_INVENTORY");

    SendListInventory(guid);
}

/**
 * @brief Sends the available inventory of a vendor to the client.
 *
 * @param vendorguid The vendor guid.
 */
void WorldSession::SendListInventory(ObjectGuid vendorguid)
{
    DEBUG_LOG("WORLD: Sent SMSG_LIST_INVENTORY");

    Creature* pCreature = GetPlayer()->GetNPCIfCanInteractWith(vendorguid, UNIT_NPC_FLAG_VENDOR);

    if (!pCreature)
    {
        DEBUG_LOG("WORLD: SendListInventory - %s not found or you can't interact with him.", vendorguid.GetString().c_str());
        _player->SendSellError(SELL_ERR_CANT_FIND_VENDOR, NULL, ObjectGuid(), 0);
        return;
    }

    // Stop the npc if moving
    pCreature->StopMoving();

    VendorItemData const* vItems = pCreature->GetVendorItems();
    VendorItemData const* tItems = pCreature->GetVendorTemplateItems();

    uint8 customitems = vItems ? vItems->GetItemCount() : 0;
    uint8 numitems = customitems + (tItems ? tItems->GetItemCount() : 0);

    std::vector<bool> bitFlags;

    float discountMod = _player->GetReputationPriceDiscount(pCreature);

    uint16 count = 0;
    ByteBuffer buffer;
    for (uint8 vendorslot = 0; vendorslot < numitems; ++vendorslot)
    {
        VendorItem const* crItem = vendorslot < customitems ? vItems->GetItem(vendorslot) : tItems->GetItem(vendorslot - customitems);

        if (crItem)
        {
            uint32 price, displayId, buyCount, maxDurability;
            int32 maxCount;

            if (crItem->type == VENDOR_ITEM_TYPE_ITEM)
            {
                ItemPrototype const * pProto = ObjectMgr::GetItemPrototype(crItem->item);
                if (!pProto)
                {
                    continue;
                }

                if (!_player->isGameMaster())
                {
                    // class wrong item skip only for bindable case
                    if ((pProto->AllowableClass & _player->getClassMask()) == 0 && pProto->Bonding == BIND_WHEN_PICKED_UP)
                    {
                        continue;
                    }

                    // race wrong item skip always
                    if ((pProto->Flags2 & ITEM_FLAG2_HORDE_ONLY) && _player->GetTeam() != HORDE)
                    {
                        continue;
                    }

                    if ((pProto->Flags2 & ITEM_FLAG2_ALLIANCE_ONLY) && _player->GetTeam() != ALLIANCE)
                    {
                        continue;
                    }

                    if ((pProto->AllowableRace & _player->getRaceMask()) == 0)
                    {
                        continue;
                    }

                    if (crItem->conditionId && !sObjectMgr.IsPlayerMeetToCondition(crItem->conditionId, _player, pCreature->GetMap(), pCreature, CONDITION_FROM_VENDOR))
                    {
                        continue;
                    }
                }

                // possible item coverting for BoA case
                if (pProto->Flags & ITEM_FLAG_BOA)
                {
                    // convert if can use and then buy
                    if (pProto->RequiredReputationFaction && uint32(_player->GetReputationRank(pProto->RequiredReputationFaction)) >= pProto->RequiredReputationRank)
                    {
                        // checked at convert data loading as existed
                        if (uint32 newItemId = sObjectMgr.GetItemConvert(crItem->item, _player->getRaceMask()))
                        {
                            pProto = ObjectMgr::GetItemPrototype(newItemId);
                        }
                    }
                }

                ++count;
                if (count >= MAX_VENDOR_ITEMS)
                {
                    break;
                }

                // reputation discount
                maxDurability = pProto->MaxDurability;
                price = (crItem->ExtendedCost == 0 || pProto->Flags2 & ITEM_FLAG2_EXT_COST_REQUIRES_GOLD) ? uint32(floor(pProto->BuyPrice * discountMod)) : 0;
                displayId = pProto->DisplayInfoID;
                maxCount = crItem->maxcount <= 0 ? -1 : int32(pCreature->GetVendorItemCurrentCount(crItem));
                buyCount = pProto->BuyCount;

            }
            else if (crItem->type == VENDOR_ITEM_TYPE_CURRENCY)
            {
                CurrencyTypesEntry const * pCurrency = sCurrencyTypesStore.LookupEntry(crItem->item);
                if (!pCurrency)
                {
                    continue;
                }

                if (pCurrency->Category == CURRENCY_CATEGORY_META)
                {
                    continue;
                }

                ++count;
                if (count >= MAX_VENDOR_ITEMS)
                {
                    break;
                }

                maxDurability = 0;
                price = 0;
                displayId = 0;
                maxCount = -1;
                buyCount = crItem->maxcount;
            }
            else
            {
                continue;
            }

            bitFlags.push_back(crItem->ExtendedCost == 0);
            bitFlags.push_back(true);                       // unk

            buffer << uint32(vendorslot + 1);               // client size expected counting from 1
            buffer << uint32(maxDurability);

            if (crItem->ExtendedCost)
            {
                buffer << uint32(crItem->ExtendedCost);
            }

            buffer << uint32(crItem->item);
            buffer << uint32(crItem->type);
            buffer << uint32(price);
            buffer << uint32(displayId);
            buffer << int32(maxCount);
            buffer << uint32(buyCount);
        }
    }

    WorldPacket data(SMSG_LIST_INVENTORY, (8 + 3 + 1 + 1 + numitems * 8 * 4));

    data.WriteGuidMask<1, 0>(vendorguid);
    data.WriteBits(count, 21);
    data.WriteGuidMask<3, 6, 5, 2, 7>(vendorguid);

    for (uint32 i = 0; i < bitFlags.size(); ++i)
    {
        data.WriteBit(bitFlags[i]);
    }

    data.WriteGuidMask<4>(vendorguid);
    data.FlushBits();
    data.append(buffer);

    data.WriteGuidBytes<5, 4, 1, 0, 6>(vendorguid);
    data << uint8(count == 0);
    data.WriteGuidBytes<2, 3, 7>(vendorguid);

    SendPacket(&data);
}

/**
 * @brief Auto-stores an item into a destination bag.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleAutoStoreBagItemOpcode(WorldPacket& recv_data)
{
    // DEBUG_LOG("WORLD: CMSG_AUTOSTORE_BAG_ITEM");
    uint8 srcbag, srcslot, dstbag;

    recv_data >> srcbag >> srcslot >> dstbag;
    // DEBUG_LOG("STORAGE: receive srcbag = %u, srcslot = %u, dstbag = %u", srcbag, srcslot, dstbag);

    Item* pItem = _player->GetItemByPos(srcbag, srcslot);
    if (!pItem)
    {
        return;
    }

    if (!_player->IsValidPos(dstbag, NULL_SLOT, false))     // can be autostore pos
    {
        _player->SendEquipError(EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT, NULL, NULL);
        return;
    }

    uint16 src = pItem->GetPos();

    // check unequip potability for equipped items and bank bags
    if (_player->IsEquipmentPos(src) || _player->IsBagPos(src))
    {
        InventoryResult msg = _player->CanUnequipItem(src, !_player->IsBagPos(src));
        if (msg != EQUIP_ERR_OK)
        {
            _player->SendEquipError(msg, pItem, NULL);
            return;
        }
    }

    ItemPosCountVec dest;
    InventoryResult msg = _player->CanStoreItem(dstbag, NULL_SLOT, dest, pItem, false);
    if (msg != EQUIP_ERR_OK)
    {
        _player->SendEquipError(msg, pItem, NULL);
        return;
    }

    // no-op: placed in same slot
    if (dest.size() == 1 && dest[0].pos == src)
    {
        // just remove gray item state
        _player->SendEquipError(EQUIP_ERR_NONE, pItem, NULL);
        return;
    }

    _player->RemoveItem(srcbag, srcslot, true);
    _player->StoreItem(dest, pItem, true);
}

/**
 * @brief Verifies that a guid can be used as a banker interaction target.
 *
 * @param guid The banker or player guid.
 * @return true if banking is allowed; otherwise false.
 */
bool WorldSession::CheckBanker(ObjectGuid guid)
{
    // GM case
    if (guid == GetPlayer()->GetObjectGuid())
    {
        // command case will return only if player have real access to command
        if (!ChatHandler(GetPlayer()).FindCommand("bank"))
        {
            DEBUG_LOG("%s attempt open bank in cheating way.", guid.GetString().c_str());
            return false;
        }
    }
    // banker case
    else
    {
        if (!GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_BANKER))
        {
            DEBUG_LOG("Banker %s not found or you can't interact with him.", guid.GetString().c_str());
            return false;
        }
    }

    return true;
}

/**
 * @brief Purchases the next available bank bag slot.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleBuyBankSlotOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: CMSG_BUY_BANK_SLOT");

    ObjectGuid guid;
    recvPacket >> guid;

    if (!CheckBanker(guid))
    {
        return;
    }

    uint32 slot = _player->GetBankBagSlotCount();

    // next slot
    ++slot;

    DETAIL_LOG("PLAYER: Buy bank bag slot, slot number = %u", slot);

    BankBagSlotPricesEntry const* slotEntry = sBankBagSlotPricesStore.LookupEntry(slot);

    if (!slotEntry)
    {
        return;
    }

    uint64 price = slotEntry->price;

    if (_player->GetMoney() < price)
    {
        return;
    }

    _player->SetBankBagSlotCount(slot);
    _player->ModifyMoney(-int64(price));

    _player->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT);
}

/**
 * @brief Moves an item from inventory into the bank automatically.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleAutoBankItemOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: CMSG_AUTOBANK_ITEM");
    uint8 srcbag, srcslot;

    recvPacket >> srcbag >> srcslot;
    DEBUG_LOG("STORAGE: receive srcbag = %u, srcslot = %u", srcbag, srcslot);

    Item* pItem = _player->GetItemByPos(srcbag, srcslot);
    if (!pItem)
    {
        return;
    }

    ItemPosCountVec dest;
    InventoryResult msg = _player->CanBankItem(NULL_BAG, NULL_SLOT, dest, pItem, false);
    if (msg != EQUIP_ERR_OK)
    {
        _player->SendEquipError(msg, pItem, NULL);
        return;
    }

    // no-op: placed in same slot
    if (dest.size() == 1 && dest[0].pos == pItem->GetPos())
    {
        // just remove gray item state
        _player->SendEquipError(EQUIP_ERR_NONE, pItem, NULL);
        return;
    }

    _player->RemoveItem(srcbag, srcslot, true);
    _player->BankItem(dest, pItem, true);
}

/**
 * @brief Moves an item between bank and inventory automatically.
 *
 * @param recvPacket The received opcode packet.
 */
void WorldSession::HandleAutoStoreBankItemOpcode(WorldPacket& recvPacket)
{
    DEBUG_LOG("WORLD: CMSG_AUTOSTORE_BANK_ITEM");
    uint8 srcbag, srcslot;

    recvPacket >> srcbag >> srcslot;
    DEBUG_LOG("STORAGE: receive srcbag = %u, srcslot = %u", srcbag, srcslot);

    Item* pItem = _player->GetItemByPos(srcbag, srcslot);
    if (!pItem)
    {
        return;
    }

    if (_player->IsBankPos(srcbag, srcslot))                // moving from bank to inventory
    {
        ItemPosCountVec dest;
        InventoryResult msg = _player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, pItem, false);
        if (msg != EQUIP_ERR_OK)
        {
            _player->SendEquipError(msg, pItem, NULL);
            return;
        }

        _player->RemoveItem(srcbag, srcslot, true);
        _player->StoreItem(dest, pItem, true);
    }
    else                                                    // moving from inventory to bank
    {
        ItemPosCountVec dest;
        InventoryResult msg = _player->CanBankItem(NULL_BAG, NULL_SLOT, dest, pItem, false);
        if (msg != EQUIP_ERR_OK)
        {
            _player->SendEquipError(msg, pItem, NULL);
            return;
        }

        _player->RemoveItem(srcbag, srcslot, true);
        _player->BankItem(dest, pItem, true);
    }
}

/**
 * @brief Sets or clears the player's equipped ammunition.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleSetAmmoOpcode(WorldPacket& recv_data)
{
    if (!GetPlayer()->IsAlive())
    {
        GetPlayer()->SendEquipError(EQUIP_ERR_YOU_ARE_DEAD, NULL, NULL);
        return;
    }

    DEBUG_LOG("WORLD: CMSG_SET_AMMO");
    uint32 item;

    recv_data >> item;

    if (!item)
    {
        GetPlayer()->RemoveAmmo();
    }
    else
    {
        GetPlayer()->SetAmmo(item);
    }
}
