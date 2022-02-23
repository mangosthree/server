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
 * SDName:      redridge_mountains
 * SD%Complete: 100
 * SDComment:   Quest support: 219.
 * SDCategory:  Redridge Mountains
 * EndScriptData
 */

/**
 * ContentData
 * npc_corporal_keeshan
 * EndContentData
 */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_corporal_leehsan
######*/

enum
{
    QUEST_MISSING_IN_ACTION = 219,

    SPELL_MOCKING_BLOW      = 21008,
    SPELL_SHIELD_BASH       = 11972,

    SAY_CORPORAL_KEESHAN_1  = -1000561,
    SAY_CORPORAL_KEESHAN_2  = -1000562,
    SAY_CORPORAL_KEESHAN_3  = -1000563,
    SAY_CORPORAL_KEESHAN_4  = -1000564,
    SAY_CORPORAL_KEESHAN_5  = -1000565,
};

struct npc_corporal_keeshan_escort : public CreatureScript
{
    npc_corporal_keeshan_escort() : CreatureScript("npc_corporal_keeshan") {}

    struct npc_corporal_keeshan_escortAI : public npc_escortAI
    {
        npc_corporal_keeshan_escortAI(Creature* pCreature) : npc_escortAI(pCreature) { }

        uint32 m_uiMockingBlowTimer;
        uint32 m_uiShieldBashTimer;

        void Reset() override
        {
            m_uiMockingBlowTimer = 5000;
            m_uiShieldBashTimer = 8000;
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
        {
            if (eventType == AI_EVENT_START_ESCORT && pInvoker->GetTypeId() == TYPEID_PLAYER)
            {
                DoScriptText(SAY_CORPORAL_KEESHAN_1, m_creature);
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
            m_creature->SetFactionTemporary(FACTION_ESCORT_A_NEUTRAL_ACTIVE, TEMPFACTION_RESTORE_RESPAWN);
#endif
                Start(false, (Player*)pInvoker, GetQuestTemplateStore(uiMiscValue));
            }
        }

        void WaypointStart(uint32 uiWP) override
        {
            switch (uiWP)
            {
            case 27:                                        // break outside
                DoScriptText(SAY_CORPORAL_KEESHAN_3, m_creature);
                m_creature->SetStandState(UNIT_STAND_STATE_STAND);
                break;
            case 54:                                        // say goodbye
                DoScriptText(SAY_CORPORAL_KEESHAN_5, m_creature);
                break;
            }
        }

        void WaypointReached(uint32 uiWP) override
        {
            switch (uiWP)
            {
            case 26:                                        // break outside
                m_creature->SetStandState(UNIT_STAND_STATE_SIT);
                DoScriptText(SAY_CORPORAL_KEESHAN_2, m_creature);
                break;
            case 53:                                        // quest_complete
                DoScriptText(SAY_CORPORAL_KEESHAN_4, m_creature);
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    pPlayer->GroupEventHappens(QUEST_MISSING_IN_ACTION, m_creature);
                }
                break;
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            // Combat check
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_uiMockingBlowTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature->getVictim(), SPELL_MOCKING_BLOW);
                m_uiMockingBlowTimer = 5000;
            }
            else
            {
                m_uiMockingBlowTimer -= uiDiff;
            }

            if (m_uiShieldBashTimer < uiDiff)
            {
                DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHIELD_BASH);
                m_uiShieldBashTimer = 8000;
            }
            else
            {
                m_uiShieldBashTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_corporal_keeshan_escortAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_MISSING_IN_ACTION)
        {
            pCreature->AI()->SendAIEvent(AI_EVENT_START_ESCORT, pPlayer, pCreature, pQuest->GetQuestId());
            return true;
        }

        return false;
    }
};

void AddSC_redridge_mountains()
{
    Script* s;
    s = new npc_corporal_keeshan_escort();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_corporal_keeshan";
    //pNewScript->GetAI = &GetAI_npc_corporal_keeshan;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_corporal_keeshan;
    //pNewScript->RegisterSelf();
}
