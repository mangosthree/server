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

#include "Player.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "SkillDiscovery.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "UpdateData.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Pet.h"
#include "Util.h"
#include "Transports.h"
#include "Weather.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGroundAV.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "ArenaTeam.h"
#include "Chat.h"
#include "revision_data.h"
#include "Database/DatabaseImpl.h"
#include "Spell.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "AchievementMgr.h"
#include "Mail.h"
#include "SpellAuras.h"
#include "DBCStores.h"
#include "DB2Stores.h"
#include "SQLStorages.h"
#include "Vehicle.h"
#include "Calendar.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

#include <cmath>

void Player::TakeExtendedCost(uint32 extendedCostId)
{
    ItemExtendedCostEntry const* extendedCost = sItemExtendedCostStore.LookupEntry(extendedCostId);

    for (uint8 i = 0; i < MAX_EXTENDED_COST_ITEMS; ++i)
    {
        if (extendedCost->reqitem[i])
        {
            DestroyItemCount(extendedCost->reqitem[i], extendedCost->reqitemcount[i], true);
        }
    }

    for (int i = 0; i < MAX_EXTENDED_COST_CURRENCIES; ++i)
    {
        if (extendedCost->reqcur[i] == CURRENCY_NONE)
        {
            continue;
        }

        if (extendedCost->IsSeasonCurrencyRequirement(i))
        {
            continue;
        }

        CurrencyTypesEntry const * entry = sCurrencyTypesStore.LookupEntry(extendedCost->reqcur[i]);
        if (!entry)
        {
            continue;
        }

        int32 cost = int32(extendedCost->reqcurrcount[i]);
        ModifyCurrencyCount(entry->ID, -cost);
    }
}

// Return true is the bought item has a max count to force refresh of window by caller
bool Player::BuyItemFromVendorSlot(ObjectGuid vendorGuid, uint32 vendorslot, uint32 item, uint32 count, uint8 bag, uint8 slot)
{
    // cheating attempt
    if (count < 1)
    {
        count = 1;
    }

    // cheating attempt
    if (bag != NULL_BAG && bag != INVENTORY_SLOT_BAG_0 && slot > MAX_BAG_SIZE && slot != NULL_SLOT)
    {
        return false;
    }

    if (!IsAlive())
    {
        return false;
    }

    ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(item);
    if (!pProto)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, NULL, item, 0);
        return false;
    }

    Creature* pCreature = GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_VENDOR);
    if (!pCreature)
    {
        DEBUG_LOG("WORLD: BuyItemFromVendor - %s not found or you can't interact with him.", vendorGuid.GetString().c_str());
        SendBuyError(BUY_ERR_DISTANCE_TOO_FAR, NULL, item, 0);
        return false;
    }

    VendorItemData const* vItems = pCreature->GetVendorItems();
    VendorItemData const* tItems = pCreature->GetVendorTemplateItems();
    if ((!vItems || vItems->Empty()) && (!tItems || tItems->Empty()))
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    uint32 vCount = vItems ? vItems->GetItemCount() : 0;
    uint32 tCount = tItems ? tItems->GetItemCount() : 0;

    if (vendorslot >= vCount + tCount)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    VendorItem const* crItem = vendorslot < vCount ? vItems->GetItem(vendorslot) : tItems->GetItem(vendorslot - vCount);
    if (!crItem)                                            // store diff item (cheating)
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    if (crItem->item != item)                               // store diff item (cheating or special convert)
    {
        bool converted = false;

        // possible item converted for BoA case
        ItemPrototype const* crProto = ObjectMgr::GetItemPrototype(crItem->item);
        if (crProto->Flags & ITEM_FLAG_BOA && crProto->RequiredReputationFaction &&
                uint32(GetReputationRank(crProto->RequiredReputationFaction)) >= crProto->RequiredReputationRank)
            converted = (sObjectMgr.GetItemConvert(crItem->item, getRaceMask()) != 0);

        if (!converted)
        {
            SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
            return false;
        }
    }

    uint32 totalCount = count;

    // check current item amount if it limited
    if (crItem->maxcount != 0)
    {
        if (pCreature->GetVendorItemCurrentCount(crItem) < totalCount)
        {
            SendBuyError(BUY_ERR_ITEM_ALREADY_SOLD, pCreature, item, 0);
            return false;
        }
    }

    if (uint32(GetReputationRank(pProto->RequiredReputationFaction)) < pProto->RequiredReputationRank)
    {
        SendBuyError(BUY_ERR_REPUTATION_REQUIRE, pCreature, item, 0);
        return false;
    }

    if (uint32 extendedCostId = crItem->ExtendedCost)
    {
        if (pProto->BuyCount != count)
        {
            SendEquipError(EQUIP_ERR_CANT_BUY_QUANTITY, NULL, NULL);
            return false;
        }

        ItemExtendedCostEntry const* iece = sItemExtendedCostStore.LookupEntry(extendedCostId);
        if (!iece)
        {
            sLog.outError("Item %u have wrong ExtendedCost field value %u", pProto->ItemId, extendedCostId);
            return false;
        }

        // item base price
        for (uint8 i = 0; i < MAX_EXTENDED_COST_ITEMS; ++i)
        {
            if (iece->reqitem[i] && !HasItemCount(iece->reqitem[i], iece->reqitemcount[i]))
            {
                SendEquipError(EQUIP_ERR_VENDOR_MISSING_TURNINS, NULL, NULL);
                return false;
            }
        }

        // currency price
        for (uint8 i = 0; i < MAX_EXTENDED_COST_CURRENCIES; ++i)
        {
            if (iece->reqcur[i] == CURRENCY_NONE)
            {
                continue;
            }

            CurrencyTypesEntry const * costCurrency = sCurrencyTypesStore.LookupEntry(iece->reqcur[i]);
            if (!costCurrency)
            {
                sLog.outError("Item %u has ExtendedCost %u with unexistent currency id %u", pProto->ItemId, extendedCostId, iece->reqcur[i]);
                continue;
            }

            int32 cost = int32(iece->reqcurrcount[i]);

            bool hasCount = iece->IsSeasonCurrencyRequirement(i) ? HasCurrencySeasonCount(iece->reqcur[i], cost) : HasCurrencyCount(iece->reqcur[i], cost);
            if (!hasCount)
            {
                SendEquipError(EQUIP_ERR_VENDOR_MISSING_TURNINS, NULL);
                return false;
            }
        }

        // check for personal arena rating requirement
        if (GetMaxPersonalArenaRatingRequirement(iece->reqarenaslot) < iece->reqpersonalarenarating)
        {
            // probably not the proper equip err
            SendEquipError(EQUIP_ERR_CANT_EQUIP_RANK, NULL, NULL);
            return false;
        }
    }

    if (crItem->conditionId && !isGameMaster() && !sObjectMgr.IsPlayerMeetToCondition(crItem->conditionId, this, pCreature->GetMap(), pCreature, CONDITION_FROM_VENDOR))
    {
        SendBuyError(BUY_ERR_CANT_FIND_ITEM, pCreature, item, 0);
        return false;
    }

    uint64 price = (crItem->ExtendedCost == 0 || pProto->Flags2 & ITEM_FLAG2_EXT_COST_REQUIRES_GOLD) ? pProto->BuyPrice * count : 0;
    if (pProto->BuyCount > 1)
    {
        price = uint64(price / float(pProto->BuyCount) + 0.5f);
    }

    // reputation discount
    if (price)
    {
        price = uint64(floor(price * GetReputationPriceDiscount(pCreature)));
    }

    if (GetMoney() < price)
    {
        SendBuyError(BUY_ERR_NOT_ENOUGHT_MONEY, pCreature, item, 0);
        return false;
    }

    Item* pItem = NULL;

    if ((bag == NULL_BAG && slot == NULL_SLOT) || IsInventoryPos(bag, slot))
    {
        ItemPosCountVec dest;
        InventoryResult msg = CanStoreNewItem(bag, slot, dest, item, totalCount);
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError(msg, NULL, NULL, item);
            return false;
        }

        ModifyMoney(-int64(price));

        if (crItem->ExtendedCost)
        {
            TakeExtendedCost(crItem->ExtendedCost);
        }

        pItem = StoreNewItem(dest, item, true);
    }
    else if (IsEquipmentPos(bag, slot))
    {
        if (totalCount != 1)
        {
            SendEquipError(EQUIP_ERR_ITEM_CANT_BE_EQUIPPED, NULL, NULL);
            return false;
        }

        uint16 dest;
        InventoryResult msg = CanEquipNewItem(slot, dest, item, false);
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError(msg, NULL, NULL, item);
            return false;
        }

        ModifyMoney(-int64(price));

        if (crItem->ExtendedCost)
        {
            TakeExtendedCost(crItem->ExtendedCost);
        }

        pItem = EquipNewItem(dest, item, true);

        if (pItem)
        {
            AutoUnequipOffhandIfNeed();
        }
    }
    else
    {
        SendEquipError(EQUIP_ERR_ITEM_DOESNT_GO_TO_SLOT, NULL, NULL);
        return false;
    }

    if (!pItem)
    {
        return false;
    }

    uint32 new_count = pCreature->UpdateVendorItemCurrentCount(crItem, totalCount);

    WorldPacket data(SMSG_BUY_ITEM, 8 + 4 + 4 + 4);
    data << pCreature->GetObjectGuid();
    data << uint32(vendorslot + 1);                 // numbered from 1 at client
    data << uint32(crItem->maxcount > 0 ? new_count : 0xFFFFFFFF);
    data << uint32(count);
    GetSession()->SendPacket(&data);

    SendNewItem(pItem, totalCount, true, false, false);

    return crItem->maxcount != 0;
}

bool Player::BuyCurrencyFromVendorSlot(ObjectGuid vendorGuid, uint32 vendorslot, uint32 currencyId, uint32 count)
{
    // cheating attempt
    if (count < 1) count = 1;

    if (!IsAlive())
    {
        return false;
    }

    CurrencyTypesEntry const* pCurrency = sCurrencyTypesStore.LookupEntry(currencyId);
    if (!pCurrency)
    {
        return false;
    }

    if (pCurrency->Category == CURRENCY_CATEGORY_META)
    {
        return false;
    }

    Creature* pCreature = GetNPCIfCanInteractWith(vendorGuid, UNIT_NPC_FLAG_VENDOR);
    if (!pCreature)
    {
        DEBUG_LOG("WORLD: BuyCurrencyFromVendorSlot - %s not found or you can't interact with him.", vendorGuid.GetString().c_str());
        return false;
    }

    VendorItemData const* vItems = pCreature->GetVendorItems();
    VendorItemData const* tItems = pCreature->GetVendorTemplateItems();
    if ((!vItems || vItems->Empty()) && (!tItems || tItems->Empty()))
    {
        return false;
    }

    uint32 vCount = vItems ? vItems->GetItemCount() : 0;
    uint32 tCount = tItems ? tItems->GetItemCount() : 0;

    if (vendorslot >= vCount + tCount)
    {
        return false;
    }

    VendorItem const* crItem = vendorslot < vCount ? vItems->GetItem(vendorslot) : tItems->GetItem(vendorslot - vCount);
    if (!crItem)                                            // store diff item (cheating)
    {
        return false;
    }

    if (crItem->item != currencyId)                         // store diff item (cheating)
    {
        return false;
    }

    if (!crItem->maxcount)
    {
        DEBUG_LOG("WORLD: BuyCurrencyFromVendorSlot - %s: crItem->maxcount (%u) == 0 for currency %u and player %s.",
            vendorGuid.GetString().c_str(), crItem->maxcount, currencyId, GetGuidStr().c_str());
        return false;
    }

    if (uint32 extendedCostId = crItem->ExtendedCost)
    {
        if (crItem->maxcount != count)
        {
            SendEquipError(EQUIP_ERR_CANT_BUY_QUANTITY, NULL, NULL);
            return false;
        }

        ItemExtendedCostEntry const* iece = sItemExtendedCostStore.LookupEntry(extendedCostId);
        if (!iece)
        {
            sLog.outError("WORLD: BuyCurrencyFromVendorSlot: Currency %u have wrong ExtendedCost field value %u for %s", currencyId, extendedCostId, vendorGuid.GetString().c_str());
            return false;
        }

        // item base price
        for (uint8 i = 0; i < MAX_EXTENDED_COST_ITEMS; ++i)
        {
            if (iece->reqitem[i] && !HasItemCount(iece->reqitem[i], iece->reqitemcount[i]))
            {
                SendEquipError(EQUIP_ERR_VENDOR_MISSING_TURNINS, NULL, NULL);
                return false;
            }
        }

        // currency price
        for (uint8 i = 0; i < MAX_EXTENDED_COST_CURRENCIES; ++i)
        {
            if (iece->reqcur[i] == CURRENCY_NONE)
            {
                continue;
            }

            CurrencyTypesEntry const * costCurrency = sCurrencyTypesStore.LookupEntry(iece->reqcur[i]);
            if (!costCurrency)
            {
                sLog.outError("Currency %u has ExtendedCost %u with unexistent currency id %u", currencyId, extendedCostId, iece->reqcur[i]);
                continue;
            }

            int32 cost = int32(iece->reqcurrcount[i]);
            bool hasCount = iece->IsSeasonCurrencyRequirement(i) ? HasCurrencySeasonCount(iece->reqcur[i], cost) : HasCurrencyCount(iece->reqcur[i], cost);
            if (!hasCount)
            {
                SendEquipError(EQUIP_ERR_VENDOR_MISSING_TURNINS, NULL);
                return false;
            }
        }

        // check for personal arena rating requirement
        if (GetMaxPersonalArenaRatingRequirement(iece->reqarenaslot) < iece->reqpersonalarenarating)
        {
            // probably not the proper equip err
            SendEquipError(EQUIP_ERR_CANT_EQUIP_RANK, NULL, NULL);
            return false;
        }
    }
    else
    {
        SendBuyError(BUY_ERR_ITEM_SOLD_OUT, 0, 0, 0);
        return false;
    }

    if (uint32 totalCap = GetCurrencyTotalCap(pCurrency))
    {
        if (GetCurrencyCount(currencyId) >= totalCap)
        {

            SendBuyError(BUY_ERR_CANT_CARRY_MORE, 0, 0, 0);
            return false;
        }
    }

    if (uint32 weekCap = GetCurrencyWeekCap(pCurrency))
    {
        if (GetCurrencyWeekCount(currencyId) >= weekCap)
        {
            SendBuyError(BUY_ERR_CANT_CARRY_MORE, 0, 0, 0);
            return false;
        }
    }

    if (crItem->ExtendedCost)
    {
        TakeExtendedCost(crItem->ExtendedCost);
    }

    ModifyCurrencyCount(currencyId, crItem->maxcount, true, false, true);


    DEBUG_LOG("WORLD: BuyCurrencyFromVendorSlot - %s: Player %s buys currency %u amount %u count %u.",
        vendorGuid.GetString().c_str(), GetGuidStr().c_str(), currencyId, crItem->maxcount, count);

    return true;
}

uint32 Player::GetMaxPersonalArenaRatingRequirement(uint32 minarenaslot)
{
    // returns the maximal personal arena rating that can be used to purchase items requiring this condition
    // the personal rating of the arena team must match the required limit as well
    // so return max[in arenateams](min(personalrating[teamtype], teamrating[teamtype]))
    uint32 max_personal_rating = 0;
    for (int i = minarenaslot; i < MAX_ARENA_SLOT; ++i)
    {
        if (ArenaTeam* at = sObjectMgr.GetArenaTeamById(GetArenaTeamId(i)))
        {
            uint32 p_rating = GetArenaPersonalRating(i);
            uint32 t_rating = at->GetRating();
            p_rating = p_rating < t_rating ? p_rating : t_rating;
            if (max_personal_rating < p_rating)
            {
                max_personal_rating = p_rating;
            }
        }
    }
    return max_personal_rating;
}
