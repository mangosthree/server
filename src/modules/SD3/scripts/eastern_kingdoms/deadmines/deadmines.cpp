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
 * SDName:      Deadmines
 * SD%Complete: 100
 * SDComment:   Contains GO for Iron Clad door event
 * SDCategory:  Deadmines
 * EndScriptData
 */

#include "precompiled.h"
#include "deadmines.h"

struct go_defias_cannon : public GameObjectScript
{
    go_defias_cannon() : GameObjectScript("go_defias_cannon") {}

    bool OnUse(Player* /*pPlayer*/, GameObject* pGo) override
    {
        ScriptedInstance* pInstance = (ScriptedInstance*)pGo->GetInstanceData();

        if (!pInstance)
        {
            return false;
        }

        if (pInstance->GetData(TYPE_IRON_CLAD_DOOR) == DONE)
        {
            return false;
        }

        pInstance->SetData(TYPE_IRON_CLAD_DOOR, DONE);
        return false;   //TODO check it; true?
    }
};

void AddSC_deadmines()
{
    Script *s;
    s = new go_defias_cannon();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_defias_cannon";
    //pNewScript->pGOUse = &GOUse_go_defias_cannon;
    //pNewScript->RegisterSelf();
}
