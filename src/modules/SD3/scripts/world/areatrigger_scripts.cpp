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
 * SDName:      Areatrigger_Scripts
 * SD%Complete: 100
 * SDComment:   Quest support: 4291, 6681, 7632, 10589/10604
 * SDCategory:  Areatrigger
 * EndScriptData
 */

/**
 * ContentData
#if defined (WOTLK)
 * at_aldurthar_gate                5284, 5285, 5286, 5287
#endif
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
 * at_coilfang_waterfall            4591
 * at_legion_teleporter             4560 Teleporter TO Invasion Point: Cataclysm
#endif
 * at_ravenholdt
#if defined (WOTLK)
 * at_spearborn_encampment          5030
 * at_warsong_farms
 * at_stormwright_shelf             5108
#endif
 * at_childrens_week_spot           3546, 3547, 3548, 3549, 3550, 3552
 * at_scent_larkorwi                1726, 1727, 1728, 1729, 1730, 1731, 1732, 1733, 1734, 1735, 1736, 1737, 1738, 1739, 1740
 * at_murkdeep                      1966
#if defined (WOTLK)
 * at_hot_on_the_trail              5710, 5711, 5712, 5714, 5715, 5716
 * at_ancient_leaf                  3587
#endif
 * EndContentData
 */

#include "precompiled.h"
#include "world_map_scripts.h"

static uint32 TriggerOrphanSpell[6][3] =
{
    {3546, 14305, 65056},                                   // The Bough of the Eternals
    {3547, 14444, 65059},                                   // Lordaeron Throne Room
    {3548, 14305, 65055},                                   // The Stonewrought Dam
    {3549, 14444, 65058},                                   // Gateway to the Frontier
    {3550, 14444, 65057},                                   // Down at the Docks
    {3552, 14305, 65054}                                    // Spooky Lighthouse
};

struct at_childrens_week_spot : public AreaTriggerScript
{
    at_childrens_week_spot() : AreaTriggerScript("at_childrens_week_spot") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        for (uint8 i = 0; i < 6; ++i)
        {
            if (pAt->id == TriggerOrphanSpell[i][0] &&
                pPlayer->GetMiniPet() && pPlayer->GetMiniPet()->GetEntry() == TriggerOrphanSpell[i][1])
            {
                pPlayer->CastSpell(pPlayer, TriggerOrphanSpell[i][2], true);
                return true;
            }
        }
        return false;
    }
};

#if defined (WOTLK) || defined(CATA) || defined(MISTS)
/*######
## Quest 13315/13351
######*/

enum
{
    TRIGGER_SOUTH               = 5284,
    TRIGGER_CENTRAL             = 5285,
    TRIGGER_NORTH               = 5286,
    TRIGGER_NORTHWEST           = 5287,

    NPC_SOUTH_GATE              = 32195,
    NPC_CENTRAL_GATE            = 32196,
    NPC_NORTH_GATE              = 32197,
    NPC_NORTHWEST_GATE          = 32199
};

struct at_aldurthar_gate : public AreaTriggerScript
{
    at_aldurthar_gate() : AreaTriggerScript("at_aldurthar_gate") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        switch (pAt->id)
        {
            case TRIGGER_SOUTH:     pPlayer->KilledMonsterCredit(NPC_SOUTH_GATE);     break;
            case TRIGGER_CENTRAL:   pPlayer->KilledMonsterCredit(NPC_CENTRAL_GATE);   break;
            case TRIGGER_NORTH:     pPlayer->KilledMonsterCredit(NPC_NORTH_GATE);     break;
            case TRIGGER_NORTHWEST: pPlayer->KilledMonsterCredit(NPC_NORTHWEST_GATE); break;
        }
        return false;
    }
};
#endif
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
/*######
## at_coilfang_waterfall
######*/

enum
{
    GO_COILFANG_WATERFALL   = 184212
};

struct at_coilfang_waterfall : public AreaTriggerScript
{
    at_coilfang_waterfall() : AreaTriggerScript("at_coilfang_waterfall") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* /*pAt*/) override
    {
        if (GameObject* pGo = GetClosestGameObjectWithEntry(pPlayer, GO_COILFANG_WATERFALL, 35.0f))
        {
            if (pGo->getLootState() == GO_READY)
            {
                pGo->UseDoorOrButton();
            }
        }
        return false;
    }
};

/*######
## at_legion_teleporter
######*/

enum
{
    SPELL_TELE_A_TO         = 37387,
    QUEST_GAINING_ACCESS_A  = 10589,

    SPELL_TELE_H_TO         = 37389,
    QUEST_GAINING_ACCESS_H  = 10604
};

struct at_legion_teleporter : public AreaTriggerScript
{
    at_legion_teleporter() : AreaTriggerScript("at_legion_teleporter") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* /*pAt*/) override
    {
        if (pPlayer->IsAlive() && !pPlayer->IsInCombat())
        {
            if (pPlayer->GetTeam() == ALLIANCE && pPlayer->GetQuestRewardStatus(QUEST_GAINING_ACCESS_A))
            {
                pPlayer->CastSpell(pPlayer, SPELL_TELE_A_TO, false);
                return true;
            }

            if (pPlayer->GetTeam() == HORDE && pPlayer->GetQuestRewardStatus(QUEST_GAINING_ACCESS_H))
            {
                pPlayer->CastSpell(pPlayer, SPELL_TELE_H_TO, false);
                return true;
            }
            return false;
        }
        return false;
    }
};
#endif

/*######
## at_ravenholdt
######*/

enum
{
    QUEST_MANOR_RAVENHOLDT  = 6681,
    NPC_RAVENHOLDT          = 13936
};

struct at_ravenholdt : public AreaTriggerScript
{
    at_ravenholdt() : AreaTriggerScript("at_ravenholdt") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* /*pAt*/) override
    {
        if (pPlayer->GetQuestStatus(QUEST_MANOR_RAVENHOLDT) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->KilledMonsterCredit(NPC_RAVENHOLDT);
        }

        return false;
    }
};

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
/*######
## at_spearborn_encampment
######*/

enum
{
    QUEST_MISTWHISPER_TREASURE          = 12575,
    NPC_TARTEK                          = 28105,
};

// 5030
struct at_spearborn_encampment : public AreaTriggerScript
{
    at_spearborn_encampment() : AreaTriggerScript("at_spearborn_encampment") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override

    {
        if (pPlayer->GetQuestStatus(QUEST_MISTWHISPER_TREASURE) == QUEST_STATUS_INCOMPLETE &&
                pPlayer->GetReqKillOrCastCurrentCount(QUEST_MISTWHISPER_TREASURE, NPC_TARTEK) == 0)
        {
            // can only spawn one at a time, it's not a too good solution
            if (GetClosestCreatureWithEntry(pPlayer, NPC_TARTEK, 50.0f))
            {
                return false;
            }

            pPlayer->SummonCreature(NPC_TARTEK, pAt->x, pAt->y, pAt->z, 0.0f, TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, MINUTE * IN_MILLISECONDS);
        }

        return false;
    }
};

/*######
## at_warsong_farms
######*/

enum
{
    QUEST_THE_WARSONG_FARMS     = 11686,
    NPC_CREDIT_SLAUGHTERHOUSE   = 25672,
    NPC_CREDIT_GRAINERY         = 25669,
    NPC_CREDIT_TORP_FARM        = 25671,

    AT_SLAUGHTERHOUSE           = 4873,
    AT_GRAINERY                 = 4871,
    AT_TORP_FARM                = 4872
};

struct at_warsong_farms : public AreaTriggerScript
{
    at_warsong_farms() : AreaTriggerScript("at_warsong_farms") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (!pPlayer->IsDead() && pPlayer->GetQuestStatus(QUEST_THE_WARSONG_FARMS) == QUEST_STATUS_INCOMPLETE)
        {
            switch (pAt->id)
            {
                case AT_SLAUGHTERHOUSE: pPlayer->KilledMonsterCredit(NPC_CREDIT_SLAUGHTERHOUSE); break;
                case AT_GRAINERY:       pPlayer->KilledMonsterCredit(NPC_CREDIT_GRAINERY);       break;
                case AT_TORP_FARM:      pPlayer->KilledMonsterCredit(NPC_CREDIT_TORP_FARM);      break;
            }
        }

        return false;
    }
};

/*######
## Quest 12548
######*/

enum
{
    SPELL_SHOLOZAR_TO_UNGORO_TELEPORT = 52056,
    SPELL_UNGORO_TO_SHOLOZAR_TELEPORT = 52057,
    AT_WAYGATE_SHOLOZAR               = 5046,
    AT_WAYGATE_UNGORO                 = 5047,
    QUEST_THE_MARKERS_OVERLOOK        = 12613,
    QUEST_THE_MARKERS_PERCH           = 12559
};

struct at_waygate : public AreaTriggerScript
{
    at_waygate() : AreaTriggerScript("at_waygate") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (!pPlayer->IsDead() && pPlayer->GetQuestStatus(QUEST_THE_MARKERS_OVERLOOK) == QUEST_STATUS_COMPLETE && pPlayer->GetQuestStatus(QUEST_THE_MARKERS_PERCH) == QUEST_STATUS_COMPLETE)
        {
            switch (pAt->id)
            {
                case AT_WAYGATE_SHOLOZAR: pPlayer->CastSpell(pPlayer, SPELL_SHOLOZAR_TO_UNGORO_TELEPORT, false); break;
                case AT_WAYGATE_UNGORO: pPlayer->CastSpell(pPlayer, SPELL_UNGORO_TO_SHOLOZAR_TELEPORT, false); break;
            }
        }

        return false;
    }
};


/*######
## Quest 12741
######*/

enum
{
    QUEST_STRENGTH_OF_THE_TEMPEST            = 12741,
    SPELL_CREATE_TRUE_POWER_OF_THE_TEMPEST   = 53067
};

struct at_stormwright_shelf : public AreaTriggerScript
{
    at_stormwright_shelf() : AreaTriggerScript("at_stormwright_shelf") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* /*pAt*/) override
    {
        if (!pPlayer->IsDead() && pPlayer->GetQuestStatus(QUEST_STRENGTH_OF_THE_TEMPEST) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->CastSpell(pPlayer, SPELL_CREATE_TRUE_POWER_OF_THE_TEMPEST, false);
        }

        return false;
    }
};
#endif

/*######
## at_scent_larkorwi
######*/

enum
{
    QUEST_SCENT_OF_LARKORWI     = 4291,
    NPC_LARKORWI_MATE           = 9683
};

struct at_scent_larkorwi : public AreaTriggerScript
{
    at_scent_larkorwi() : AreaTriggerScript("at_scent_larkorwi") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (pPlayer->IsAlive() && !pPlayer->isGameMaster() && pPlayer->GetQuestStatus(QUEST_SCENT_OF_LARKORWI) == QUEST_STATUS_INCOMPLETE)
        {
            if (!GetClosestCreatureWithEntry(pPlayer, NPC_LARKORWI_MATE, 25.0f, false, false))
            {
                pPlayer->SummonCreature(NPC_LARKORWI_MATE, pAt->x, pAt->y, pAt->z, 3.3f, TEMPSPAWN_TIMED_OOC_DESPAWN, 2 * MINUTE * IN_MILLISECONDS);
            }
        }

        return false;
    }
};

/*######
## at_murkdeep
######*/

struct at_murkdeep : public AreaTriggerScript
{
    at_murkdeep() : AreaTriggerScript("at_murkdeep") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* /*pAt*/) override
    {
        // Handle Murkdeep event start
        // The area trigger summons 3 Greymist Coastrunners; The rest of the event is handled by world map scripts
        if (pPlayer->IsAlive() && !pPlayer->isGameMaster() && pPlayer->GetQuestStatus(QUEST_WANTED_MURKDEEP) == QUEST_STATUS_INCOMPLETE)
        {
            ScriptedMap* pScriptedMap = (ScriptedMap*)pPlayer->GetInstanceData();
            if (!pScriptedMap)
            {
                return false;
            }

            // If Murkdeep is already spawned, skip the rest
            if (pScriptedMap->GetSingleCreatureFromStorage(NPC_MURKDEEP, true))
            {
                return true;
            }

            // Check if there are already coastrunners (dead or alive) around the area
            if (GetClosestCreatureWithEntry(pPlayer, NPC_GREYMIST_COASTRUNNNER, 60.0f, false, false))
            {
                return true;
            }

            float fX, fY, fZ;
            for (uint8 i = 0; i < 3; ++i)
            {
                // Spawn locations are defined in World Maps Scripts.h
                pPlayer->GetRandomPoint(aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][0], aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][1], aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][2], 5.0f, fX, fY, fZ);

                if (Creature* pTemp = pPlayer->SummonCreature(NPC_GREYMIST_COASTRUNNNER, fX, fY, fZ, aSpawnLocations[POS_IDX_MURKDEEP_SPAWN][3], TEMPSPAWN_DEAD_DESPAWN, 0))
                {
                    pTemp->SetWalk(false);
                    pTemp->GetRandomPoint(aSpawnLocations[POS_IDX_MURKDEEP_MOVE][0], aSpawnLocations[POS_IDX_MURKDEEP_MOVE][1], aSpawnLocations[POS_IDX_MURKDEEP_MOVE][2], 5.0f, fX, fY, fZ);
                    pTemp->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
                }
            }
        }

        return false;
    }
};

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
/*######
## at_hot_on_the_trail
######*/

struct HotOnTrailData
{
    uint32 uiAtEntry, uiQuestEntry, uiCreditEntry, uiSpellEntry;
};

static const HotOnTrailData aHotOnTrailValues[6] =
{
    {5710, 24849, 38340, 71713},                    // Stormwind Bank
    {5711, 24849, 38341, 71745},                    // Stormwind Auction House
    {5712, 24849, 38342, 71752},                    // Stormwind Barber Shop
    {5714, 24851, 38341, 71760},                    // Orgrimmar Auction House
    {5715, 24851, 38340, 71759},                    // Orgrimmar Bank
    {5716, 24851, 38342, 71758},                    // Orgrimmar Barber Shop
};

struct at_hot_on_the_trail : public AreaTriggerScript
{
    at_hot_on_the_trail() : AreaTriggerScript("at_hot_on_the_trail") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (pPlayer->isGameMaster() || !pPlayer->IsAlive())
        {
            return false;
        }

        for (uint8 i = 0; i < 6; ++i)
        {
            if (pAt->id == aHotOnTrailValues[i].uiAtEntry)
            {
                if (pPlayer->GetQuestStatus(aHotOnTrailValues[i].uiQuestEntry) == QUEST_STATUS_INCOMPLETE &&
                    pPlayer->GetReqKillOrCastCurrentCount(aHotOnTrailValues[i].uiQuestEntry, aHotOnTrailValues[i].uiCreditEntry) == 0)
                {
                    pPlayer->CastSpell(pPlayer, aHotOnTrailValues[i].uiSpellEntry, true);
                    return true;
                }
            }
        }

        return false;
    }
};

/*######
## at_ancient_leaf
######*/

enum
{
    QUEST_ANCIENT_LEAF              = 7632,

    NPC_VARTRUS                     = 14524,
    NPC_STOMA                       = 14525,
    NPC_HASTAT                      = 14526,

    MAX_ANCIENTS                    = 3,
};

struct AncientSpawn
{
    uint32 uiEntry;
    float fX, fY, fZ, fO;
};

static const AncientSpawn afSpawnLocations[MAX_ANCIENTS] =
{
    { NPC_VARTRUS, 6204.051758f, -1172.575684f, 370.079224f, 0.86052f },    // Vartus the Ancient
    { NPC_STOMA,   6246.953613f, -1155.985718f, 366.182953f, 2.90269f },    // Stoma the Ancient
    { NPC_HASTAT,  6193.449219f, -1137.834106f, 366.260529f, 5.77332f },    // Hastat the Ancient
};

struct at_ancient_leaf : public AreaTriggerScript
{
    at_ancient_leaf() : AreaTriggerScript("at_ancient_leaf") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (pPlayer->isGameMaster() || !pPlayer->IsAlive())
        {
            return false;
        }

        // Handle Call Ancients event start - The area trigger summons 3 ancients
        if (pPlayer->GetQuestStatus(QUEST_ANCIENT_LEAF) == QUEST_STATUS_COMPLETE)
        {
            // If ancients are already spawned, skip the rest
            if (GetClosestCreatureWithEntry(pPlayer, NPC_VARTRUS, 50.0f) || GetClosestCreatureWithEntry(pPlayer, NPC_STOMA, 50.0f) || GetClosestCreatureWithEntry(pPlayer, NPC_HASTAT, 50.0f))
            {
                return true;
            }

            for (uint8 i = 0; i < MAX_ANCIENTS; ++i)
            {
                pPlayer->SummonCreature(afSpawnLocations[i].uiEntry, afSpawnLocations[i].fX, afSpawnLocations[i].fY, afSpawnLocations[i].fZ, afSpawnLocations[i].fO, TEMPSPAWN_TIMED_DESPAWN, 5 * MINUTE * IN_MILLISECONDS);
            }
        }

        return false;
    }
};
#endif

void AddSC_areatrigger_scripts()
{
    Script* s;
    s = new at_childrens_week_spot();
    s->RegisterSelf();

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new at_aldurthar_gate();
    s->RegisterSelf();
#endif
    s = new at_ravenholdt();
    s->RegisterSelf();

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new at_spearborn_encampment();
    s->RegisterSelf();

    s = new at_warsong_farms();
    s->RegisterSelf();

    s = new at_waygate();
    s->RegisterSelf();

    s = new at_stormwright_shelf();
    s->RegisterSelf();
#endif

    s = new at_scent_larkorwi();
    s->RegisterSelf();
    s = new at_murkdeep();
    s->RegisterSelf();

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new at_coilfang_waterfall();
    s->RegisterSelf();

    s = new at_legion_teleporter();
    s->RegisterSelf();
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new at_ancient_leaf();
    s->RegisterSelf();
    s = new at_hot_on_the_trail();
    s->RegisterSelf();
#endif

    //pNewScript = new Script;
    //pNewScript->Name = "at_childrens_week_spot";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_childrens_week_spot;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_coilfang_waterfall";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_coilfang_waterfall;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_legion_teleporter";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_legion_teleporter;

    //pNewScript = new Script;
    //pNewScript->Name = "at_ravenholdt";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_ravenholdt;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_scent_larkorwi";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_scent_larkorwi;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_murkdeep";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_murkdeep;
    //pNewScript->RegisterSelf();
}
