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
 * SDName:      Wetlands
 * SD%Complete: 100
 * SDComment:   Quest support: 1249.
 * SDCategory:  Wetlands
 * EndScriptData
 */

/**
 * ContentData
 * npc_mikhail
 * npc_tapoke_slim_jahn
 * EndContentData
 */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_tapoke_slim_jahn
######*/

enum
{
    SAY_SLIM_AGGRO              = -1000977,
    SAY_SLIM_DEFEAT             = -1000978,
    SAY_FRIEND_DEFEAT           = -1000979,
    SAY_SLIM_NOTES              = -1000980,

    QUEST_MISSING_DIPLO_PT11    = 1249,
    FACTION_ENEMY               = 168,                      // ToDo: faction needs to be confirmed!

    SPELL_STEALTH               = 1785,
    SPELL_CALL_FRIENDS          = 16457,                    // summons 1x friend
    NPC_SLIMS_FRIEND            = 4971,
    NPC_TAPOKE_SLIM_JAHN        = 4962,
    WAYPOINT_MAILBOX            = 2,
    WAYPOINT_GATE                = 6
};

static const DialogueEntry aDiplomatDialogue[] =
{
    {SAY_SLIM_DEFEAT,           NPC_TAPOKE_SLIM_JAHN,   4000},
    {SAY_SLIM_NOTES,            NPC_TAPOKE_SLIM_JAHN,   7000},
    {QUEST_MISSING_DIPLO_PT11,  0,                      0},
    {0, 0, 0},
};

struct npc_tapoke_slim_jahn : public CreatureScript
{
    npc_tapoke_slim_jahn() : CreatureScript("npc_tapoke_slim_jahn") {}

    struct npc_tapoke_slim_jahnAI : public npc_escortAI, private DialogueHelper
    {
        npc_tapoke_slim_jahnAI(Creature* pCreature) : m_bFriendSummoned(false), m_bEventComplete(false),
        friendGUID(ObjectGuid()), npc_escortAI(pCreature), DialogueHelper(aDiplomatDialogue)
        {
        }

        void Reset() override
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                friendGUID.Clear();
                m_bFriendSummoned = false;
                m_bEventComplete = false;
            }
        }

        void AttackedBy(Unit* pAttacker) override
        {
            if (m_creature->getVictim())
            {
                return;
            }

            if (m_creature->IsFriendlyTo(pAttacker))
            {
                return;
            }

            AttackStart(pAttacker);
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case WAYPOINT_MAILBOX:
                SetRun();
                m_creature->RemoveAurasDueToSpell(SPELL_STEALTH);
                m_creature->SetFactionTemporary(FACTION_ENEMY, TEMPFACTION_RESTORE_RESPAWN | TEMPFACTION_RESTORE_COMBAT_STOP);
                break;

            case WAYPOINT_GATE:
                // fail the quest if he escapes
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    JustDied(pPlayer);
                }
                m_creature->ForcedDespawn(5000);
                break;
            }
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (HasEscortState(STATE_ESCORT_ESCORTING) && !m_bFriendSummoned)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_CALL_FRIENDS) == CAST_OK)
                {
                    DoScriptText(SAY_SLIM_AGGRO, m_creature);
                    m_bFriendSummoned = true;
                }
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            // Note: may not work on guardian pets
            if (Player* pPlayer = GetPlayerForEscort())
            {
                pSummoned->AI()->AttackStart(pPlayer);
            }
            friendGUID = pSummoned->GetObjectGuid();
        }

        void DamageTaken(Unit* /*pDoneBy*/, uint32& uiDamage) override
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                return;
            }

            if (m_creature->HealthBelowPctDamaged(20, uiDamage))
            {
                uiDamage = 0;

                 if (Player* pPlayer = GetPlayerForEscort())
                 {
                     pPlayer->GroupEventHappens(QUEST_MISSING_DIPLO_PT11, m_creature);
                     if(Creature* pFriend = m_creature->GetMap()->GetCreature(friendGUID))
                     {
                         pFriend->ForcedDespawn(0);
                     }
                 }
                // despawn friend - Note: may not work on guardian pets

                // set escort on pause and evade
                m_bEventComplete = true;
                SetEscortPaused(true);
                EnterEvadeMode();
            }
        }

        void MovementInform(uint32 uiMoveType, uint32 uiPointId) override
        {
            if (uiMoveType != POINT_MOTION_TYPE || !HasEscortState(STATE_ESCORT_ESCORTING))
            {
                return;
            }

            npc_escortAI::MovementInform(uiMoveType, uiPointId);

            // after the npc is defeated, start the dialog right after it reaches the evade point
            if (m_bEventComplete)
            {
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    m_creature->SetFacingToObject(pPlayer);
                }

                StartNextDialogueText(SAY_SLIM_DEFEAT);
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/, Unit* pInvoker, uint32 uiMiscValue) override
        {
            // start escort
            if (eventType == AI_EVENT_START_ESCORT && pInvoker->GetTypeId() == TYPEID_PLAYER)
            {
                Start(false, (Player*)pInvoker, GetQuestTemplateStore(uiMiscValue), true);
            }
        }

        void JustDidDialogueStep(int32 iEntry) override
        {
            if (iEntry == QUEST_MISSING_DIPLO_PT11)
            {
                // complete quest
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    pPlayer->GroupEventHappens(QUEST_MISSING_DIPLO_PT11, m_creature);
                }

                // despawn and respawn at inn
                m_creature->ForcedDespawn(1000);
                m_creature->SetRespawnDelay(2);
            }
        }

        Creature* GetSpeakerByEntry(uint32 uiEntry) override
        {
            if (uiEntry == NPC_TAPOKE_SLIM_JAHN)
            {
                return m_creature;
            }

            return nullptr;
        }


        void UpdateEscortAI(const uint32 uiDiff) override
        {
            DialogueUpdate(uiDiff);

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }

    private:
        ObjectGuid friendGUID;
        bool m_bFriendSummoned;
        bool m_bEventComplete;
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_tapoke_slim_jahnAI(pCreature);
    }
};

/*######
## npc_mikhail
######*/

struct npc_mikhail : public CreatureScript
{
    npc_mikhail() : CreatureScript("npc_mikhail") {}

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_MISSING_DIPLO_PT11)
        {
            Creature* pSlim = GetClosestCreatureWithEntry(pCreature, NPC_TAPOKE_SLIM_JAHN, 25.0f);
            if (!pSlim)
            {
                return false;
            }

            if (!pSlim->HasAura(SPELL_STEALTH))
            {
                pSlim->CastSpell(pSlim, SPELL_STEALTH, true);
            }

            pCreature->AI()->SendAIEvent(AI_EVENT_START_ESCORT, pPlayer, pSlim, pQuest->GetQuestId());
            return true;
        }

        return false;
    }
};

/*######
## AddSC
######*/

void AddSC_wetlands()
{
    Script* s;
    s = new npc_tapoke_slim_jahn();
    s->RegisterSelf();
    s = new npc_mikhail();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_tapoke_slim_jahn";
    //pNewScript->GetAI = &GetAI_npc_tapoke_slim_jahn;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_mikhail";
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_mikhail;
    //pNewScript->RegisterSelf();
}
