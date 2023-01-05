/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013  ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2023 MaNGOS <https://getmangos.eu>
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
 * SDName:      Tirisfal_Glades
 * SD%Complete: 100
 * SDComment:   Quest support: 590, 1819.
 * SDCategory:  Tirisfal Glades
 * EndScriptData
 */

/**
 * ContentData
 * go_mausoleum_door
 * go_mausoleum_trigger
 * npc_calvin_montague
 * EndContentData
 */

#include "precompiled.h"

/*######
## go_mausoleum_door
## go_mausoleum_trigger
######*/

enum
{
    QUEST_ULAG      = 1819,
    NPC_ULAG        = 6390,
    GO_TRIGGER      = 104593,
    GO_DOOR         = 176594
};

struct go_mausoleum_door : public GameObjectScript
{
    go_mausoleum_door() : GameObjectScript("go_mausoleum_door") {}

    bool OnUse(Player* pPlayer, GameObject* /*pGo*/) override
    {
        if (pPlayer->GetQuestStatus(QUEST_ULAG) != QUEST_STATUS_INCOMPLETE)
        {
            return false;
        }

        if (GameObject* pTrigger = GetClosestGameObjectWithEntry(pPlayer, GO_TRIGGER, 30.0f))
        {
            pTrigger->SetGoState(GO_STATE_READY);
            pPlayer->SummonCreature(NPC_ULAG, 2390.26f, 336.47f, 40.01f, 2.26f, TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, 300000);
            return false;
        }

        return false;
    }
};

struct go_mausoleum_trigger : public GameObjectScript
{
    go_mausoleum_trigger() : GameObjectScript("go_mausoleum_trigger") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (pPlayer->GetQuestStatus(QUEST_ULAG) != QUEST_STATUS_INCOMPLETE)
        {
            return false;
        }

        if (GameObject* pDoor = GetClosestGameObjectWithEntry(pPlayer, GO_DOOR, 30.0f))
        {
            pGo->SetGoState(GO_STATE_ACTIVE);
            pDoor->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
            return true;
        }

        return false;
    }
};

/*######
## npc_calvin_montague
######*/

enum
{
    SAY_COMPLETE        = -1000356,
    SPELL_DRINK         = 2639,                             // possibly not correct spell (but iconId is correct)
    QUEST_590           = 590,
    FACTION_HOSTILE     = 168
};

struct npc_calvin_montague : public CreatureScript
{
    npc_calvin_montague() : CreatureScript("npc_calvin_montague") {}

    struct npc_calvin_montagueAI : public ScriptedAI
    {
        npc_calvin_montagueAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
        }

        uint32 m_uiPhase;
        uint32 m_uiPhaseTimer;
        ObjectGuid m_playerGuid;

        void Reset() override
        {
            m_uiPhase = 0;
            m_uiPhaseTimer = 5000;
            m_playerGuid.Clear();
            if (!m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE))
            {
                m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
            }
        }

        void AttackedBy(Unit* pAttacker) override
        {
            if (m_creature->getVictim() || m_creature->IsFriendlyTo(pAttacker))
            {
                return;
            }

            AttackStart(pAttacker);
        }

        void DamageTaken(Unit* pDoneBy, uint32& uiDamage) override
        {
            if (uiDamage > m_creature->GetHealth() || ((m_creature->GetHealth() - uiDamage) * 100 / m_creature->GetMaxHealth() < 15))
            {
                uiDamage = 0;

                m_creature->CombatStop(true);
                m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                m_uiPhase = 1;

                if (pDoneBy->GetTypeId() == TYPEID_PLAYER)
                {
                    m_playerGuid = pDoneBy->GetObjectGuid();
                }
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiPhase)
            {
                if (m_uiPhaseTimer < uiDiff)
                {
                    m_uiPhaseTimer = 7500;
                }
                else
                {
                    m_uiPhaseTimer -= uiDiff;
                    return;
                }

                switch (m_uiPhase)
                {
                case 1:
                    DoScriptText(SAY_COMPLETE, m_creature);
                    ++m_uiPhase;
                    break;
                case 2:
                    if (Player* pPlayer = m_creature->GetMap()->GetPlayer(m_playerGuid))
                    {
                        pPlayer->AreaExploredOrEventHappens(QUEST_590);
                    }

                    m_creature->CastSpell(m_creature, SPELL_DRINK, true);
                    ++m_uiPhase;
                    break;
                case 3:
                    m_creature->SetStandState(UNIT_STAND_STATE_STAND); //otherwise he is sitting until server restart
                    EnterEvadeMode();
                    break;
                }

                return;
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_calvin_montagueAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_590)
        {
            pCreature->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_COMBAT_STOP | TEMPFACTION_RESTORE_RESPAWN);
            if (pCreature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE))
            {
                pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
            }
            pCreature->AI()->AttackStart(pPlayer);
            return true;
        }
        return false;
    }
};

void AddSC_tirisfal_glades()
{
    Script* s;
    s = new go_mausoleum_door();
    s->RegisterSelf();
    s = new go_mausoleum_trigger();
    s->RegisterSelf();
    s = new npc_calvin_montague();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_mausoleum_door";
    //pNewScript->pGOUse = &GOUse_go_mausoleum_door;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_mausoleum_trigger";
    //pNewScript->pGOUse = &GOUse_go_mausoleum_trigger;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_calvin_montague";
    //pNewScript->GetAI = &GetAI_npc_calvin_montague;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_calvin_montague;
    //pNewScript->RegisterSelf();
}
