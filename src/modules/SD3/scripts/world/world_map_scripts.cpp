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
 * SDName:      world_map_scripts
 * SD%Complete: 100
 * SDComment:   Quest support: 4740, 11538
 * SDCategory:  World Map Scripts
 * EndScriptData
 */

#include "precompiled.h"
#include "world_map_scripts.h"

// for non-instantiable maps, ZoneScript provides excessive mechanic, but nevertheless

/* *********************************************************
 *                  EASTERN KINGDOMS
 */
struct map_eastern_kingdoms : public ZoneScript
{
    map_eastern_kingdoms() : ZoneScript("world_map_eastern_kingdoms") {}

    struct world_map_eastern_kingdoms : public ScriptedMap
    {
        world_map_eastern_kingdoms(Map* pMap) : ScriptedMap(pMap) {}

        void OnCreatureCreate(Creature* pCreature)
        {
            switch (pCreature->GetEntry())
            {
            case NPC_JONATHAN:
            case NPC_WRYNN:
            case NPC_BOLVAR:
            case NPC_PRESTOR:
            case NPC_WINDSOR:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
            }
        }

        void SetData(uint32 /*uiType*/, uint32 /*uiData*/) {}
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new world_map_eastern_kingdoms(pMap);
    }
};

/* *********************************************************
 *                     KALIMDOR
 */
struct map_kalimdor : public ZoneScript
{
    map_kalimdor() : ZoneScript("world_map_kalimdor") {}

    struct world_map_kalimdor : public ScriptedMap
    {
        world_map_kalimdor(Map* pMap) : ScriptedMap(pMap) { Initialize(); }

        uint8 m_uiMurkdeepAdds_KilledAddCount;

        void Initialize()
        {
            m_uiMurkdeepAdds_KilledAddCount = 0;
        }

        void OnCreatureCreate(Creature* pCreature)
        {
            if (pCreature->GetEntry() == NPC_MURKDEEP)
            {
                m_mNpcEntryGuidStore[NPC_MURKDEEP] = pCreature->GetObjectGuid();
            }
        }

        void OnCreatureDeath(Creature* pCreature)
        {
            switch (pCreature->GetEntry())
            {
            case NPC_GREYMIST_COASTRUNNNER:
                if (pCreature->IsTemporarySummon())         // Only count the ones summoned for Murkdeep quest
                {
                    ++m_uiMurkdeepAdds_KilledAddCount;

                    // If all 3 coastrunners are killed, summon 2 warriors
                    if (m_uiMurkdeepAdds_KilledAddCount == 3)
                    {
                        float fX, fY, fZ;
                        for (uint8 i = 0; i < 2; ++i)
                        {
                            pCreature->GetRandomPoint(aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][0], aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][1], aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][2], 5.0f, fX, fY, fZ);

                            if (Creature* pTemp = pCreature->SummonCreature(NPC_GREYMIST_WARRIOR, fX, fY, fZ, aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][3], TEMPSPAWN_DEAD_DESPAWN, 0))
                            {
                                pTemp->SetWalk(false);
                                pTemp->GetRandomPoint(aSpawnLocations[POS_IDX_MURKDEEP_MOVE][0], aSpawnLocations[POS_IDX_MURKDEEP_MOVE][1], aSpawnLocations[POS_IDX_MURKDEEP_MOVE][2], 5.0f, fX, fY, fZ);
                                pTemp->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
                            }
                        }

                        m_uiMurkdeepAdds_KilledAddCount = 0;
                    }
                }
                break;
            case NPC_GREYMIST_WARRIOR:
                if (pCreature->IsTemporarySummon())         // Only count the ones summoned for Murkdeep quest
                {
                    ++m_uiMurkdeepAdds_KilledAddCount;

                    // After the 2 warriors are killed, Murkdeep spawns, along with a hunter
                    if (m_uiMurkdeepAdds_KilledAddCount == 2)
                    {
                        float fX, fY, fZ;
                        for (uint8 i = 0; i < 2; ++i)
                        {
                            pCreature->GetRandomPoint(aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][0], aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][1], aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][2], 5.0f, fX, fY, fZ);

                            if (Creature* pTemp = pCreature->SummonCreature(!i ? NPC_MURKDEEP : NPC_GREYMIST_HUNTER, fX, fY, fZ, aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][3], TEMPSPAWN_DEAD_DESPAWN, 0))
                            {
                                pTemp->SetWalk(false);
                                pTemp->GetRandomPoint(aSpawnLocations[POS_IDX_MURKDEEP_MOVE][0], aSpawnLocations[POS_IDX_MURKDEEP_MOVE][1], aSpawnLocations[POS_IDX_MURKDEEP_MOVE][2], 5.0f, fX, fY, fZ);
                                pTemp->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
                            }
                        }

                        m_uiMurkdeepAdds_KilledAddCount = 0;
                    }
                }
                break;
            }
        }

        void SetData(uint32 /*uiType*/, uint32 /*uiData*/) {}
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new world_map_kalimdor(pMap);
    }
};

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
/* *********************************************************
 *                     OUTLAND
 */
struct map_outland : public ZoneScript
{
    map_outland() : ZoneScript("world_map_outland") {}

    struct world_map_outland : public ScriptedMap
    {
        world_map_outland(Map* pMap) : ScriptedMap(pMap) { Initialize(); }

        uint8 m_uiEmissaryOfHate_KilledAddCount;

        void Initialize()
        {
            m_uiEmissaryOfHate_KilledAddCount = 0;
        }

        void OnCreatureCreate(Creature* pCreature)
        {
            if (pCreature->GetEntry() == NPC_EMISSARY_OF_HATE)
            {
                m_mNpcEntryGuidStore[NPC_EMISSARY_OF_HATE] = pCreature->GetObjectGuid();
            }
        }

        void OnCreatureDeath(Creature* pCreature)
        {
            switch (pCreature->GetEntry())
            {
            case NPC_IRESPEAKER:
            case NPC_UNLEASHED_HELLION:
                if (!GetSingleCreatureFromStorage(NPC_EMISSARY_OF_HATE, true))
                {
                    ++m_uiEmissaryOfHate_KilledAddCount;
                    if (m_uiEmissaryOfHate_KilledAddCount == 6)
                    {
                        pCreature->SummonCreature(NPC_EMISSARY_OF_HATE, aSpawnLocations[POS_IDX_EMISSARY_SPAWN][0], aSpawnLocations[POS_IDX_EMISSARY_SPAWN][1], aSpawnLocations[POS_IDX_EMISSARY_SPAWN][2], aSpawnLocations[POS_IDX_EMISSARY_SPAWN][3], TEMPSPAWN_DEAD_DESPAWN, 0);
                        m_uiEmissaryOfHate_KilledAddCount = 0;
                    }
                }
                break;
            }
        }

        void SetData(uint32 /*uiType*/, uint32 /*uiData*/) {}
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new world_map_outland(pMap);
    }
};
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
/* *********************************************************
 *                     NORTHREND
 */
struct  map_northrend : public ZoneScript
{
    map_northrend() : ZoneScript("world_map_northrend") {}

    struct world_map_northrend : public ScriptedMap
    {
        world_map_northrend(Map* pMap) : ScriptedMap(pMap) {}

        void SetData(uint32 /*uiType*/, uint32 /*uiData*/) {}
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new world_map_northrend(pMap);
    }
};
#endif

void AddSC_world_map_scripts()
{
    Script* s;
    s = new map_eastern_kingdoms();
    s->RegisterSelf();
    s = new map_kalimdor();
    s->RegisterSelf();
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new map_outland();
    s->RegisterSelf();
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new map_northrend();
    s->RegisterSelf();
#endif

    //pNewScript = new Script;
    //pNewScript->Name = "world_map_eastern_kingdoms";
    //pNewScript->GetInstanceData = &GetInstanceData_world_map_eastern_kingdoms;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "world_map_kalimdor";
    //pNewScript->GetInstanceData = &GetInstanceData_world_map_kalimdor;
    //pNewScript->RegisterSelf();

//#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    //pNewScript = new Script;
    //pNewScript->Name = "world_map_outland";
    //pNewScript->GetInstanceData = &GetInstanceData_world_map_outland;
    //pNewScript->RegisterSelf();
//#endif
//#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    //pNewScript = new Script;
    //pNewScript->Name = "world_map_northrend";
    //pNewScript->GetInstanceData = &GetInstanceData_world_map_northrend;
    //pNewScript->RegisterSelf();
//#endif
}
