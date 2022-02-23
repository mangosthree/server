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
SDName: Magisters_Terrace
SD%Complete: 100
SDComment: Quest support: 11490(post-event)
SDCategory: Magisters Terrace
EndScriptData */

/* ContentData
npc_kalecgos
EndContentData */

#include "precompiled.h"
#include "magisters_terrace.h"

/*######
## npc_kalecgos
######*/

enum
{
    SPELL_TRANSFORM_TO_KAEL     = 44670,
    SPELL_ORB_KILL_CREDIT       = 46307,
    NPC_KALECGOS                = 24848,                    // human form entry

    MAP_ID_MAGISTER             = 585,
};

static const float afKaelLandPoint[4] = {200.36f, -270.77f, -8.73f, 0.01f};

// This is friendly keal that appear after used Orb.
// If we assume DB handle summon, summon appear somewhere outside the platform where Orb is
struct npc_kalecgos : public CreatureScript
{
    npc_kalecgos() : CreatureScript("npc_kalecgos") {}

    struct npc_kalecgosAI : public ScriptedAI
    {
        npc_kalecgosAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

        uint32 m_uiTransformTimer;

        void Reset() override
        {
            // Check the map id because the same creature entry is involved in other scripted event in other instance
            if (m_creature->GetMapId() != MAP_ID_MAGISTER)
            {
                return;
            }

            m_uiTransformTimer = 0;

            // Move the dragon to landing point
            m_creature->GetMotionMaster()->MovePoint(1, afKaelLandPoint[0], afKaelLandPoint[1], afKaelLandPoint[2]);
        }

        void MovementInform(uint32 uiType, uint32 uiPointId) override
        {
            if (uiType != POINT_MOTION_TYPE)
            {
                return;
            }

            if (uiPointId)
            {
                m_creature->SetLevitate(false);
                m_creature->SetFacingTo(afKaelLandPoint[3]);
                m_uiTransformTimer = MINUTE * IN_MILLISECONDS;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiTransformTimer)
            {
                if (m_uiTransformTimer <= uiDiff)
                {
                    // Transform and update entry, now ready for quest/read gossip
                    if (DoCastSpellIfCan(m_creature, SPELL_TRANSFORM_TO_KAEL) == CAST_OK)
                    {
                        DoCastSpellIfCan(m_creature, SPELL_ORB_KILL_CREDIT, CAST_TRIGGERED);
                        m_creature->UpdateEntry(NPC_KALECGOS);

                        m_uiTransformTimer = 0;
                    }
                }
                else
                {
                    m_uiTransformTimer -= uiDiff;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_kalecgosAI(pCreature);
    }
};

struct event_go_scrying_orb : public MapEventScript //TODO something with this mechanic, dropping the script
{
    event_go_scrying_orb() : MapEventScript("event_go_scrying_orb") {}

    bool OnReceived(uint32 /*uiEventId*/, Object* pSource, Object* /*pTarget*/, bool bIsStart) override
    {
        if (bIsStart && pSource->GetTypeId() == TYPEID_PLAYER)
        {
            if (ScriptedInstance* pInstance = (ScriptedInstance*)((Player*)pSource)->GetInstanceData())
            {
                // Check if the Dragon is already spawned and don't allow it to spawn it multiple times
                if (pInstance->GetSingleCreatureFromStorage(NPC_KALECGOS_DRAGON, true))
                {
                    return true;
                }
            }
        }
        return false;
    }
};

void AddSC_magisters_terrace()
{
    Script* s;

    s = new npc_kalecgos();
    s->RegisterSelf();
    s = new event_go_scrying_orb();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_kalecgos";
    //pNewScript->GetAI = &GetAI_npc_kalecgos;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "event_go_scrying_orb";
    //pNewScript->pProcessEventId = &ProcessEventId_event_go_scrying_orb;
    //pNewScript->RegisterSelf();
}
