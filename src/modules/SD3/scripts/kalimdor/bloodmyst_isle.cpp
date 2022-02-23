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
SDName: Bloodmyst_Isle
SD%Complete: 80
SDComment: Quest support: 9670
SDCategory: Bloodmyst Isle
EndScriptData */

/* ContentData
mob_webbed_creature
EndContentData */

#include "precompiled.h"

/*######
## mob_webbed_creature
######*/

enum
{
    NPC_EXPEDITION_RESEARCHER       = 17681,
};

// possible creatures to be spawned (too many to be added to enum)
const uint32 possibleSpawns[31] = {17322, 17661, 17496, 17522, 17340, 17352, 17333, 17524, 17654, 17348, 17339, 17345, 17353, 17336, 17550, 17330, 17701, 17321, 17325, 17320, 17683, 17342, 17715, 17334, 17341, 17338, 17337, 17346, 17344, 17327};

struct mob_webbed_creature : public CreatureScript
{
    mob_webbed_creature() : CreatureScript("mob_webbed_creature") {}

    struct mob_webbed_creatureAI : public Scripted_NoMovementAI
    {
        mob_webbed_creatureAI(Creature* pCreature) : Scripted_NoMovementAI(pCreature) { }

        void AttackStart(Unit* /*pWho*/) override { }
        void MoveInLineOfSight(Unit* /*pWho*/) override { }

        void JustDied(Unit* pKiller) override
        {
            uint32 uiSpawnCreatureEntry = 0;

            switch (urand(0, 2))
            {
            case 0:
                uiSpawnCreatureEntry = NPC_EXPEDITION_RESEARCHER;
                if (pKiller->GetTypeId() == TYPEID_PLAYER)
                {
                    ((Player*)pKiller)->KilledMonsterCredit(uiSpawnCreatureEntry, m_creature->GetObjectGuid());
                }
                break;
            case 1:
            case 2:
                uiSpawnCreatureEntry = possibleSpawns[urand(0, 30)];
                break;
            }

            if (uiSpawnCreatureEntry)
            {
                m_creature->SummonCreature(uiSpawnCreatureEntry, 0.0f, 0.0f, 0.0f, m_creature->GetOrientation(), TEMPSPAWN_TIMED_OOC_DESPAWN, 25000);
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new mob_webbed_creatureAI(pCreature);
    }
};

void AddSC_bloodmyst_isle()
{
    Script* s;

    s = new mob_webbed_creature();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "mob_webbed_creature";
    //pNewScript->GetAI = &GetAI_mob_webbed_creature;
    //pNewScript->RegisterSelf();
}
