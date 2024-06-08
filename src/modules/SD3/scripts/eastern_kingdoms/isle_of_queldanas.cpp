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

/* ScriptData
SDName: Isle_of_Queldanas
SD%Complete: 100
SDComment: Quest support: 11524, 11525
SDCategory: Isle Of Quel'Danas
EndScriptData */

/* ContentData
npc_converted_sentry
EndContentData */

#include "precompiled.h"

/*######
## npc_converted_sentry
######*/

enum
{
    SAY_CONVERTED_1             = -1000188,
    SAY_CONVERTED_2             = -1000189,

    SPELL_CONVERT_CREDIT        = 45009,
    TIME_PET_DURATION           = 7500
};

struct npc_converted_sentry : public CreatureScript
{
    npc_converted_sentry() : CreatureScript("npc_converted_sentry") {}

    struct npc_converted_sentryAI : public ScriptedAI
    {
        npc_converted_sentryAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        uint32 m_uiCreditTimer;

        void Reset() override
        {
            m_uiCreditTimer = 2500;
        }

        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiCreditTimer)
            {
                if (m_uiCreditTimer <= uiDiff)
                {
                    DoScriptText(urand(0, 1) ? SAY_CONVERTED_1 : SAY_CONVERTED_2, m_creature);

                    DoCastSpellIfCan(m_creature, SPELL_CONVERT_CREDIT);
                    ((Pet*)m_creature)->SetDuration(TIME_PET_DURATION);
                    m_uiCreditTimer = 0;
                }
                else
                {
                    m_uiCreditTimer -= uiDiff;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_converted_sentryAI(pCreature);
    }
};

void AddSC_isle_of_queldanas()
{
    Script* s;

    s = new npc_converted_sentry();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_converted_sentry";
    //pNewScript->GetAI = &GetAI_npc_converted_sentry;
    //pNewScript->RegisterSelf();
}
