/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2022 MaNGOS <https://getmangos.eu>
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

/**
 * ScriptData
 * SDName:      Item_Scripts
 * SD%Complete: 100
 * SDComment: Items for a range of different items. See content below (in script)
 * SDCategory:  Items
 * EndScriptData
 */

/**
 * ContentData
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
 * item_arcane_charges                 Prevent use if player is not flying (cannot cast while on ground)
 * item_flying_machine(i34060,i34061)  Engineering crafted flying machines
 * item_gor_dreks_ointment(i30175)     Protecting Our Own(q10488)
 * item_ogre_brew                       Prevent use if player not in 3522 area
#endif
 * EndContentData
 */

#include "precompiled.h"

/*
    In order to work properly you must use a false spell to trigger the OnUse Method => you can assign spell 18282 to the object bound to this script.

*/
struct item_gossip_test : public ItemScript
{
    item_gossip_test() : ItemScript("item_gossip_test") {}

    bool OnUse(Player* pPlayer, Item* pItem, const SpellCastTargets& pTargets) override
    {
        // Logging
        sLog.outString("Item [item_gossip_test] %s was used by %s ! ", pItem->GetProto()->Name1, pPlayer->GetName());

        pPlayer->PlayerTalkClass->ClearMenus();

        uint32 gossip_menu_id = 1;

        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_INTERACT_1, "Option with GOSSIP_ICON_INTERACT_1", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, " Option with GOSSIP_ICON_CHAT", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_VENDOR, " Option with GOSSIP_ICON_VENDOR", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_TAXI, " Option with GOSSIP_ICON_TAXI", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_TRAINER, " Option with GOSSIP_ICON_TRAINER", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_INTERACT_1, " Option with GOSSIP_ICON_INTERACT_1", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_INTERACT_2, " Option with GOSSIP_ICON_INTERACT_2", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_MONEY_BAG, " Option with GOSSIP_ICON_MONEY_BAG", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_TALK, "Option with GOSSIP_ICON_TALK", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_TABARD, "Option with GOSSIP_ICON_TABARD", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_BATTLE, "Option with GOSSIP_ICON_BATTLE", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_DOT, "Option with GOSSIP_ICON_DOT", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT_11, "Option with GOSSIP_ICON_CHAT_11", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT_12, "Option with GOSSIP_ICON_CHAT_12", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
#if defined(CLASSIC) || defined(TBC)
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_DOT_13, "Option with GOSSIP_ICON_DOT_13", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
#else
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT_13, "Option with GOSSIP_ICON_DOT_13", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
#endif
        // Max gossip optiosn seem to be 15
      /*  pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_DOT_14, "Option with GOSSIP_ICON_DOT_14", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_DOT_15, "Option with GOSSIP_ICON_DOT_15", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_DOT_16, "Option with GOSSIP_ICON_DOT_16", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_DOT_17, "Option with GOSSIP_ICON_DOT_17", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_DOT_18, "Option with GOSSIP_ICON_DOT_18", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_DOT_19, "Option with GOSSIP_ICON_DOT_19", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_DOT_20, "Option with GOSSIP_ICON_DOT_20", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
        */

       // pPlayer->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ICON_INTERACT_1, "Code test option", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2, true);

        pPlayer->SEND_GOSSIP_MENU(gossip_menu_id, pItem->GetObjectGuid());

        return false; // return FALSE because item would be stuck (spell is not processed) !
    }

    bool OnGossipSelect(Player* pPlayer, Item* pItem, uint32 uiSender, uint32 uiAction)
    {
        switch (uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 1:
            {
                uint32 playerGUID = pPlayer->GetGUIDLow();

                sLog.outString("Item [item_gossip_test] %s was used by %s and choose action %u ", pItem->GetProto()->Name1, pPlayer->GetName(), uiAction);

                pPlayer->CLOSE_GOSSIP_MENU();

                break;
            }

        }
        return true;
    }

    bool OnGossipSelectWithCode(Player* pPlayer, Item* pItem, uint32 uiSender, uint32 uiAction, const char* code)
    {
        sLog.outString("Item [item_gossip_test] %s was used by %s and choose action %u with code %s ", pItem->GetProto()->Name1, pPlayer->GetName(), uiAction, code);

        switch (uiAction)
        {
            case GOSSIP_ACTION_INFO_DEF + 2:
            {
                sLog.outString("OK GOSSIP_ACTION_INFO_DEF + 2 !");

                break;
            }
        }

        pPlayer->CLOSE_GOSSIP_MENU();
        return true;
    }
};

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined (MISTS)
#include "Spell.h" // This include is not needed in Zero, but is in the rest

/*#####
# item_ogre_brew
#####*/

enum
{
    BLUE_OGRE_BREW  = 32783,
    RED_OGRE_BREW   = 32784,
    TRANCHANTES     = 3522
};

struct item_ogre_brew : public ItemScript
{
    item_ogre_brew() : ItemScript("item_ogre_brew") {}

    bool OnUse(Player* pPlayer, Item* pItem, const SpellCastTargets&) override
    {
    uint32 itemId = pItem->GetEntry();
    uint32 pZoneId = pPlayer->GetZoneId();

    if (itemId == BLUE_OGRE_BREW || itemId == RED_OGRE_BREW)    //an excessive check of DB sanity, may be dropped
    {
        if (pZoneId == TRANCHANTES)
        {
            return false;   // allowing cast, i.e. this script did not handle it
        }

        debug_log("SD3: Player attempt to use item %u, but did not meet zone requirement : %u", itemId, pZoneId);
        if (const SpellEntry* pSpellInfo = GetSpellStore()->LookupEntry(pItem->GetProto()->Spells[0].SpellId))
        {
            Spell::SendCastResult(pPlayer, pSpellInfo, 1, SPELL_FAILED_NOT_HERE);
        }
    }
    return true;
    }
};

/*#####
# item_arcane_charges
#####*/

enum
{
    SPELL_ARCANE_CHARGES    = 45072
};

struct item_arcane_charges : public ItemScript
{
    item_arcane_charges() : ItemScript("item_arcane_charges") {}

    bool OnUse(Player* pPlayer, Item* pItem, const SpellCastTargets& /*pTargets*/) override
    {
        if (pPlayer->IsTaxiFlying())
        {
            return false;
        }

        pPlayer->SendEquipError(EQUIP_ERR_NONE, pItem, nullptr);

        if (const SpellEntry* pSpellInfo = GetSpellStore()->LookupEntry(SPELL_ARCANE_CHARGES))
        {
            Spell::SendCastResult(pPlayer, pSpellInfo, 1, SPELL_FAILED_ERROR);
        }
        return true;
    }
};

/*#####
# item_flying_machine
#####*/

struct item_flying_machine : public ItemScript
{
    item_flying_machine() : ItemScript("item_flying_machine") {}

    bool OnUse(Player* pPlayer, Item* pItem, const SpellCastTargets& /*pTargets*/) override
    {
        uint32 itemId = pItem->GetEntry();

        if (itemId == 34060)
        if (pPlayer->GetBaseSkillValue(SKILL_RIDING) >= 225)
        {
            return false;
        }

        if (itemId == 34061)
        if (pPlayer->GetBaseSkillValue(SKILL_RIDING) == 300)
        {
            return false;
        }

        debug_log("SD3: Player attempt to use item %u, but did not meet riding requirement", itemId);
        pPlayer->SendEquipError(EQUIP_ERR_CANT_EQUIP_SKILL, pItem, nullptr);
        return true;
    }
};

/*#####
# item_gor_dreks_ointment
#####*/

enum
{
    NPC_TH_DIRE_WOLF        = 20748,
    SPELL_GORDREKS_OINTMENT = 32578
};

struct item_gor_dreks_ointment : public ItemScript
{
    item_gor_dreks_ointment() : ItemScript("item_gor_dreks_ointment") {}

    bool OnUse(Player* pPlayer, Item* pItem, const SpellCastTargets& pTargets) override
    {
        if (pTargets.getUnitTarget() && pTargets.getUnitTarget()->GetTypeId() == TYPEID_UNIT && pTargets.getUnitTarget()->HasAura(SPELL_GORDREKS_OINTMENT))
        {
            pPlayer->SendEquipError(EQUIP_ERR_NONE, pItem, nullptr);

            if (const SpellEntry* pSpellInfo = GetSpellStore()->LookupEntry(SPELL_GORDREKS_OINTMENT))
            {
                Spell::SendCastResult(pPlayer, pSpellInfo, 1, SPELL_FAILED_TARGET_AURASTATE);
            }

            return true;
        }

        return false;
    }
};
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
/*#####
# item_petrov_cluster_bombs
#####*/

struct item_petrov_cluster_bombs : public ItemScript
{
    item_petrov_cluster_bombs() : ItemScript("item_petrov_cluster_bombs") {}

    enum
    {
        SPELL_PETROV_BOMB = 42406,
        AREA_ID_SHATTERED_STRAITS = 4064,
        ZONE_ID_HOWLING = 495
    };

    bool OnUse(Player* pPlayer, Item* pItem, const SpellCastTargets& /*pTargets*/)
    {
        if (pPlayer->GetZoneId() != ZONE_ID_HOWLING)
        {
            return false;
        }

        if (!pPlayer->GetTransport() || pPlayer->GetAreaId() != AREA_ID_SHATTERED_STRAITS)
        {
            pPlayer->SendEquipError(EQUIP_ERR_NONE, pItem, nullptr);

            if (const SpellEntry* pSpellInfo = GetSpellStore()->LookupEntry(SPELL_PETROV_BOMB))
            {
                Spell::SendCastResult(pPlayer, pSpellInfo, 1, SPELL_FAILED_NOT_HERE);
            }

            return true;
        }

        return false;
    }
};
#endif

void AddSC_item_scripts()
{
#if defined (CLASSIC) || (TBC) || defined (WOTLK) || defined (CATA) || defined (MISTS)
    Script* s;
    s = new item_gossip_test();
    s->RegisterSelf();
#endif

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined (MISTS)
    s = new item_ogre_brew();
    s->RegisterSelf();
    s = new item_arcane_charges();
    s->RegisterSelf();
    s = new item_flying_machine();
    s->RegisterSelf();
    s = new item_gor_dreks_ointment();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "item_arcane_charges";
    //pNewScript->pItemUse = &ItemUse_item_arcane_charges;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "item_flying_machine";
    //pNewScript->pItemUse = &ItemUse_item_flying_machine;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "item_gor_dreks_ointment";
    //pNewScript->pItemUse = &ItemUse_item_gor_dreks_ointment;
    //pNewScript->RegisterSelf();
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new item_petrov_cluster_bombs();
    s->RegisterSelf();

//    pNewScript = new Script;
//    pNewScript->Name = "item_petrov_cluster_bombs";
//    pNewScript->pItemUse = &ItemUse_item_petrov_cluster_bombs;
//    pNewScript->RegisterSelf();
#endif
}
