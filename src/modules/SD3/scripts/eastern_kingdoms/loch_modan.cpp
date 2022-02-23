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
 * SDName:      Loch_Modan
 * SD%Complete: 100
 * SDComment:   Quest support: 309, 3181 (only to argue with pebblebitty to get to searing gorge, before quest rewarded).
 * SDCategory:  Loch Modan
 * EndScriptData
 */

/**
 * ContentData
 * npc_mountaineer_pebblebitty
 * npc_miran
 * EndContentData
 */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_mountaineer_pebblebitty
######*/

struct npc_mountaineer_pebblebitty : public CreatureScript  //TODO localisation
{
    npc_mountaineer_pebblebitty() : CreatureScript("npc_mountaineer_pebblebitty") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        if (pCreature->IsQuestGiver())
        {
            pPlayer->PrepareQuestMenu(pCreature->GetObjectGuid());
        }

        if (!pPlayer->GetQuestRewardStatus(3181) == 1)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Open the gate please, I need to get to Searing Gorge", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetObjectGuid());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch (uiAction)
        {
        case GOSSIP_ACTION_INFO_DEF + 1:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "But I need to get there, now open the gate!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->SEND_GOSSIP_MENU(1833, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 2:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Ok, so what is this other way?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->SEND_GOSSIP_MENU(1834, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 3:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Doesn't matter, I'm invulnerable.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            pPlayer->SEND_GOSSIP_MENU(1835, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 4:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Yes...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
            pPlayer->SEND_GOSSIP_MENU(1836, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 5:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Ok, I'll try to remember that.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
            pPlayer->SEND_GOSSIP_MENU(1837, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 6:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "A key? Ok!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            pPlayer->SEND_GOSSIP_MENU(1838, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 7:
            pPlayer->CLOSE_GOSSIP_MENU();
            break;
        }
        return true;
    }
};

/*######
## npc_miran
######*/

enum
{
    QUEST_PROTECTING_THE_SHIPMENT = 309,

    SAY_MIRAN_1           = -1000571,
    SAY_DARK_IRON_DWARF   = -1000572,
    SAY_MIRAN_2           = -1000573,
    SAY_MIRAN_3           = -1000574,

    NPC_DARK_IRON_DWARF   = 2149
};

struct Location
{
    float m_fX, m_fY, m_fZ, m_fO;
};

static const Location m_afAmbushSpawn[] =
{
    { -5691.93f, -3745.91f, 319.159f, 2.21f},
    { -5706.98f, -3745.39f, 318.728f, 1.04f}
};

struct npc_miran : public CreatureScript
{
    npc_miran() : CreatureScript("npc_miran") {}

    struct npc_miranAI : public npc_escortAI
    {
        npc_miranAI(Creature* pCreature) : npc_escortAI(pCreature)
        {
        }

        uint8 m_uiDwarves;

        void Reset() override
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                m_uiDwarves = 0;
            }
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 19:
                DoScriptText(SAY_MIRAN_1, m_creature);
                m_creature->SummonCreature(NPC_DARK_IRON_DWARF, m_afAmbushSpawn[0].m_fX, m_afAmbushSpawn[0].m_fY, m_afAmbushSpawn[0].m_fZ, m_afAmbushSpawn[0].m_fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 25000);
                m_creature->SummonCreature(NPC_DARK_IRON_DWARF, m_afAmbushSpawn[1].m_fX, m_afAmbushSpawn[1].m_fY, m_afAmbushSpawn[1].m_fZ, m_afAmbushSpawn[1].m_fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 25000);
                break;
            case 23:
                DoScriptText(SAY_MIRAN_3, m_creature);
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    pPlayer->GroupEventHappens(QUEST_PROTECTING_THE_SHIPMENT, m_creature);
                }
                break;
            }
        }

        void SummonedCreatureJustDied(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_DARK_IRON_DWARF)
            {
                --m_uiDwarves;
                if (!m_uiDwarves)
                {
                    DoScriptText(SAY_MIRAN_2, m_creature);
                }
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            if (pSummoned->GetEntry() == NPC_DARK_IRON_DWARF)
            {
                if (!m_uiDwarves)
                {
                    DoScriptText(SAY_DARK_IRON_DWARF, pSummoned);
                }
                ++m_uiDwarves;
                pSummoned->AI()->AttackStart(m_creature);
            }
        }
    };

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_PROTECTING_THE_SHIPMENT)
        {
            if (npc_miranAI* pEscortAI = dynamic_cast<npc_miranAI*>(pCreature->AI()))
            {
                pEscortAI->Start(false, pPlayer, pQuest);
                return true;
            }
        }
        return false;
    }

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_miranAI(pCreature);
    }
};

void AddSC_loch_modan()
{
    Script* s;
    s = new npc_mountaineer_pebblebitty();
    s->RegisterSelf();
    s = new npc_miran();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_mountaineer_pebblebitty";
    //pNewScript->pGossipHello =  &GossipHello_npc_mountaineer_pebblebitty;
    //pNewScript->pGossipSelect = &GossipSelect_npc_mountaineer_pebblebitty;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_miran";
    //pNewScript->GetAI = &GetAI_npc_miran;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_miran;
    //pNewScript->RegisterSelf();
}
