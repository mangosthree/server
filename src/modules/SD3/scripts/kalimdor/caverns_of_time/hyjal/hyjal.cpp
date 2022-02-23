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

/* ScriptData
SDName: Hyjal
SD%Complete: 80
SDComment: gossip text id's unknown
SDCategory: Caverns of Time, Mount Hyjal
EndScriptData */

/* ContentData
npc_jaina_proudmoore
npc_thrall
npc_tyrande_whisperwind
EndContentData */

#include "precompiled.h"
#include "hyjalAI.h"

enum
{
    GOSSIP_ITEM_JAINA_BEGIN         = -3534000,
    GOSSIP_ITEM_JAINA_ANATHERON     = -3534001,
    GOSSIP_ITEM_JAINA_SUCCCESS      = -3534002,

    GOSSIP_ITEM_THRALL_BEGIN        = -3534003,
    GOSSIP_ITEM_THRALL_AZGALOR      = -3534004,
    GOSSIP_ITEM_THRALL_SUCCESS      = -3534005,

    GOSSIP_ITEM_TYRANDE_AID         = -3534006,

    // Note: additional menu items include 9230 and 9398.
    GOSSIP_MENU_ID_DEFAULT          = 907,                  // this is wrong, but currently we don't know which are the right ids
};

struct npc_jaina_proudmoore : public CreatureScript
{
    npc_jaina_proudmoore() : CreatureScript("npc_jaina_proudmoore") {}

    CreatureAI* GetAI(Creature* pCreature) override
    {
        hyjalAI* pTempAI = new hyjalAI(pCreature);

        pTempAI->m_aSpells[0].m_uiSpellId = SPELL_BLIZZARD;
        pTempAI->m_aSpells[0].m_uiCooldown = urand(15000, 35000);
        pTempAI->m_aSpells[0].m_pType = TARGETTYPE_RANDOM;

        pTempAI->m_aSpells[1].m_uiSpellId = SPELL_PYROBLAST;
        pTempAI->m_aSpells[1].m_uiCooldown = urand(2000, 9000);
        pTempAI->m_aSpells[1].m_pType = TARGETTYPE_RANDOM;

        pTempAI->m_aSpells[2].m_uiSpellId = SPELL_SUMMON_ELEMENTALS;
        pTempAI->m_aSpells[2].m_uiCooldown = urand(15000, 45000);
        pTempAI->m_aSpells[2].m_pType = TARGETTYPE_SELF;

        return pTempAI;
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)pCreature->GetInstanceData())
        {
            if (hyjalAI* pJainaAI = dynamic_cast<hyjalAI*>(pCreature->AI()))
            {
                if (!pJainaAI->IsEventInProgress())
                {
                    // Should not happen that jaina is here now, but for safe we check
                    if (pInstance->GetData(TYPE_KAZROGAL) != DONE)
                    {
                        if (pInstance->GetData(TYPE_WINTERCHILL) == NOT_STARTED || pInstance->GetData(TYPE_WINTERCHILL) == FAIL)
                        {
                            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_JAINA_BEGIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                        }
                        else if (pInstance->GetData(TYPE_WINTERCHILL) == DONE && (pInstance->GetData(TYPE_ANETHERON) == NOT_STARTED || pInstance->GetData(TYPE_ANETHERON) == FAIL))
                        {
                            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_JAINA_ANATHERON, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                        }
                        else if (pInstance->GetData(TYPE_ANETHERON) == DONE)
                        {
                            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_JAINA_SUCCCESS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                        }

                        if (pPlayer->isGameMaster())
                        {
                            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "[GM] Toggle Debug Timers", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                        }
                    }
                }
            }
        }

        pPlayer->SEND_GOSSIP_MENU(GOSSIP_MENU_ID_DEFAULT, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (hyjalAI* pJainaAI = dynamic_cast<hyjalAI*>(pCreature->AI()))
        {
            switch (uiAction)
            {
            case GOSSIP_ACTION_INFO_DEF + 1:
                pJainaAI->StartEvent();
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                pJainaAI->m_bIsFirstBossDead = true;
                pJainaAI->m_uiWaveCount = 9;
                pJainaAI->StartEvent();
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                pJainaAI->Retreat();
                break;
            case GOSSIP_ACTION_INFO_DEF:
                pJainaAI->m_bDebugMode = !pJainaAI->m_bDebugMode;
                debug_log("SD3: HyjalAI - Debug mode has been toggled %s", pJainaAI->m_bDebugMode ? "on" : "off");
                break;
            }
        }

        pPlayer->CLOSE_GOSSIP_MENU();
        return true;
    }
};

struct npc_thrall : public CreatureScript
{
    npc_thrall() : CreatureScript("npc_thrall") {}

    CreatureAI* GetAI(Creature* pCreature) override
    {
        hyjalAI* pTempAI = new hyjalAI(pCreature);

        pTempAI->m_aSpells[0].m_uiSpellId = SPELL_CHAIN_LIGHTNING;
        pTempAI->m_aSpells[0].m_uiCooldown = urand(2000, 7000);
        pTempAI->m_aSpells[0].m_pType = TARGETTYPE_VICTIM;

        pTempAI->m_aSpells[1].m_uiSpellId = SPELL_FERAL_SPIRIT;
        pTempAI->m_aSpells[1].m_uiCooldown = urand(6000, 41000);
        pTempAI->m_aSpells[1].m_pType = TARGETTYPE_RANDOM;

        return pTempAI;
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)pCreature->GetInstanceData())
        {
            if (hyjalAI* pThrallAI = dynamic_cast<hyjalAI*>(pCreature->AI()))
            {
                if (!pThrallAI->IsEventInProgress())
                {
                    // Only let them start the Horde phases if Anetheron is dead.
                    if (pInstance->GetData(TYPE_ANETHERON) == DONE && pInstance->GetData(TYPE_ARCHIMONDE) != DONE)
                    {
                        if (pInstance->GetData(TYPE_KAZROGAL) == NOT_STARTED || pInstance->GetData(TYPE_KAZROGAL) == FAIL)
                        {
                            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_THRALL_BEGIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                        }
                        else if (pInstance->GetData(TYPE_KAZROGAL) == DONE && (pInstance->GetData(TYPE_AZGALOR) == NOT_STARTED || pInstance->GetData(TYPE_AZGALOR) == FAIL))
                        {
                            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_THRALL_AZGALOR, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                        }
                        else if (pInstance->GetData(TYPE_AZGALOR) == DONE)
                        {
                            pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_THRALL_SUCCESS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                        }

                        if (pPlayer->isGameMaster())
                        {
                            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "[GM] Toggle Debug Timers", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                        }
                    }
                }
            }
        }

        pPlayer->SEND_GOSSIP_MENU(GOSSIP_MENU_ID_DEFAULT, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (hyjalAI* pThrallAI = dynamic_cast<hyjalAI*>(pCreature->AI()))
        {
            switch (uiAction)
            {
            case GOSSIP_ACTION_INFO_DEF + 1:
                pThrallAI->StartEvent();
                break;
            case GOSSIP_ACTION_INFO_DEF + 2:
                pThrallAI->m_bIsFirstBossDead = true;
                pThrallAI->m_uiWaveCount = 9;
                pThrallAI->StartEvent();
                break;
            case GOSSIP_ACTION_INFO_DEF + 3:
                pThrallAI->Retreat();
                break;
            case GOSSIP_ACTION_INFO_DEF:
                pThrallAI->m_bDebugMode = !pThrallAI->m_bDebugMode;
                debug_log("SD3: HyjalAI - Debug mode has been toggled %s", pThrallAI->m_bDebugMode ? "on" : "off");
                break;
            }
        }

        pPlayer->CLOSE_GOSSIP_MENU();
        return true;
    }
};

struct npc_tyrande_whisperwind : public CreatureScript
{
    npc_tyrande_whisperwind() : CreatureScript("npc_tyrande_whisperwind") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (ScriptedInstance* pInstance = (ScriptedInstance*)pCreature->GetInstanceData())
        {
            // Only let them get item if Azgalor is dead.
            if (pInstance->GetData(TYPE_AZGALOR) == DONE && !pPlayer->HasItemCount(ITEM_TEAR_OF_GODDESS, 1))
            {
                pPlayer->ADD_GOSSIP_ITEM_ID(GOSSIP_ICON_CHAT, GOSSIP_ITEM_TYRANDE_AID, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            }
        }

        pPlayer->SEND_GOSSIP_MENU(GOSSIP_MENU_ID_DEFAULT, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* /*pCreature*/, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF)
        {
            if (Item* pItem = pPlayer->StoreNewItemInInventorySlot(ITEM_TEAR_OF_GODDESS, 1))
            {
                pPlayer->SendNewItem(pItem, 1, true, false);
            }
        }

        pPlayer->CLOSE_GOSSIP_MENU();
        return true;
    }
};

void AddSC_hyjal()
{
    Script* s;

    s = new npc_jaina_proudmoore();
    s->RegisterSelf();
    s = new npc_thrall();
    s->RegisterSelf();
    s = new npc_tyrande_whisperwind();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_jaina_proudmoore";
    //pNewScript->GetAI = &GetAI_npc_jaina_proudmoore;
    //pNewScript->pGossipHello = &GossipHello_npc_jaina_proudmoore;
    //pNewScript->pGossipSelect = &GossipSelect_npc_jaina_proudmoore;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_thrall";
    //pNewScript->GetAI = &GetAI_npc_thrall;
    //pNewScript->pGossipHello = &GossipHello_npc_thrall;
    //pNewScript->pGossipSelect = &GossipSelect_npc_thrall;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_tyrande_whisperwind";
    //pNewScript->pGossipHello = &GossipHello_npc_tyrande_whisperwind;
    //pNewScript->pGossipSelect = &GossipSelect_npc_tyrande_whisperwind;
    //pNewScript->RegisterSelf();
}
