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
 * SDName:      GO_Scripts
 * SD%Complete: 100
 * SDComment:   Quest support: 5097, 5098, Barov_journal->Teaches spell 26089
 * SDCategory:  Game Objects
 * EndScriptData
 */

/**
 * ContentData
 * go_barov_journal
#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
 * go_ethereum_prison
 * go_ethereum_stasis
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
 * go_mysterious_snow_mound
 * go_tele_to_dalaran_crystal
 * go_tele_to_violet_stand
#endif
 * go_andorhal_tower
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
 * go_scourge_enclosure
 * go_lab_work_reagents
#endif
 * EndContentData
 */

#include "precompiled.h"
#include "GameObjectAI.h"

/*######
## go_barov_journal
######*/

enum
{
    SPELL_TAILOR_FELCLOTH_BAG = 26086,
    SPELL_LEARN_FELCLOTH_BAG  = 26095
};

struct go_barov_journal : public GameObjectScript
{
    go_barov_journal() : GameObjectScript("go_barov_journal") {}

    bool OnUse(Player* pPlayer, GameObject* /*pGo*/) override
    {
        if (pPlayer->HasSkill(SKILL_TAILORING) && pPlayer->GetBaseSkillValue(SKILL_TAILORING) >= 280 && !pPlayer->HasSpell(SPELL_TAILOR_FELCLOTH_BAG))
        {
            pPlayer->CastSpell(pPlayer, SPELL_LEARN_FELCLOTH_BAG, false);
        }
        return true;
    }
};

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
/*######
## go_ethereum_prison
######*/

enum
{
    FACTION_LC     = 1011,
    FACTION_SHAT   = 935,
    FACTION_CE     = 942,
    FACTION_CON    = 933,
    FACTION_KT     = 989,
    FACTION_SPOR   = 970,

    SPELL_REP_LC   = 39456,
    SPELL_REP_SHAT = 39457,
    SPELL_REP_CE   = 39460,
    SPELL_REP_CON  = 39474,
    SPELL_REP_KT   = 39475,
    SPELL_REP_SPOR = 39476
};

const uint32 uiNpcPrisonEntry[] =
{
    22810, 22811, 22812, 22813, 22814, 22815,               // good guys
    20783, 20784, 20785, 20786, 20788, 20789, 20790         // bad guys
};

struct go_ethereum_prison : public GameObjectScript
{
    go_ethereum_prison() : GameObjectScript("go_ethereum_prison") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        uint8 uiRandom = urand(0, countof(uiNpcPrisonEntry) - 1);

        if (Creature* pCreature = pPlayer->SummonCreature(uiNpcPrisonEntry[uiRandom],
            pGo->GetPositionX(), pGo->GetPositionY(), pGo->GetPositionZ(), pGo->GetAngle(pPlayer),
            TEMPSPAWN_TIMED_OOC_DESPAWN, 30000))
        {
            if (!pCreature->IsHostileTo(pPlayer))
            {
                uint32 uiSpell = 0;

                if (FactionTemplateEntry const* pFaction = pCreature->getFactionTemplateEntry())
                {
                    switch (pFaction->faction)
                    {
                    case FACTION_LC:   uiSpell = SPELL_REP_LC;   break;
                    case FACTION_SHAT: uiSpell = SPELL_REP_SHAT; break;
                    case FACTION_CE:   uiSpell = SPELL_REP_CE;   break;
                    case FACTION_CON:  uiSpell = SPELL_REP_CON;  break;
                    case FACTION_KT:   uiSpell = SPELL_REP_KT;   break;
                    case FACTION_SPOR: uiSpell = SPELL_REP_SPOR; break;
                    }

                    if (uiSpell)
                    {
                        pCreature->CastSpell(pPlayer, uiSpell, false);
                    }
                    else
                    {
                        script_error_log("go_ethereum_prison summoned creature (entry %u) but faction (%u) are not expected by script.", pCreature->GetEntry(), pCreature->getFaction());
                    }
                }
            }
        }

        return false;
    }
};

/*######
## go_ethereum_stasis
######*/

const uint32 uiNpcStasisEntry[] =
{
    22825, 20888, 22827, 22826, 22828
};

struct go_ethereum_stasis : public GameObjectScript
{
    go_ethereum_stasis() : GameObjectScript("go_ethereum_stasis") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        uint8 uiRandom = urand(0, countof(uiNpcStasisEntry) - 1);

        pPlayer->SummonCreature(uiNpcStasisEntry[uiRandom],
            pGo->GetPositionX(), pGo->GetPositionY(), pGo->GetPositionZ(), pGo->GetAngle(pPlayer),
            TEMPSPAWN_TIMED_OOC_DESPAWN, 30000);

        return false;
    }
};

/*######
## go_jump_a_tron
######*/

enum
{
    SPELL_JUMP_A_TRON = 33382,
    NPC_JUMP_A_TRON   = 19041
};

struct go_jump_a_tron : public GameObjectScript
{
    go_jump_a_tron() : GameObjectScript("go_jump_a_tron") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (Creature* pCreature = GetClosestCreatureWithEntry(pGo, NPC_JUMP_A_TRON, INTERACTION_DISTANCE))
        {
            pCreature->CastSpell(pPlayer, SPELL_JUMP_A_TRON, false);
        }

        return false;
    }
};
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
/*######
## go_mysterious_snow_mound
######*/

struct go_mysterious_snow_mound : public GameObjectScript
{
    enum
    {
        SPELL_SUMMON_DEEP_JORMUNGAR = 66510,
        SPELL_SUMMON_MOLE_MACHINE = 66492,
        SPELL_SUMMON_MARAUDER = 66491,
    };

    go_mysterious_snow_mound() : GameObjectScript("go_mysterious_snow_mound") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (urand(0, 1))
        {
            pPlayer->CastSpell(pPlayer, SPELL_SUMMON_DEEP_JORMUNGAR, true);
        }
        else
        {
            // This is basically wrong, but added for support.
            // Mole machine would summon, along with unkonwn GO (a GO trap?) and then
            // the npc would summon with base of that location.
            pPlayer->CastSpell(pPlayer, SPELL_SUMMON_MOLE_MACHINE, true);
            pPlayer->CastSpell(pPlayer, SPELL_SUMMON_MARAUDER, true);
        }

        pGo->SetLootState(GO_JUST_DEACTIVATED);
        return true;
    }
};

/*######
## go_tele_to_dalaran_crystal
######*/

enum
{
    QUEST_LEARN_LEAVE_RETURN = 12790,
    QUEST_TELE_CRYSTAL_FLAG = 12845
};

struct go_tele_to_dalaran_crystal : public GameObjectScript
{
    go_tele_to_dalaran_crystal() : GameObjectScript("go_tele_to_dalaran_crystal") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (pPlayer->GetQuestRewardStatus(QUEST_TELE_CRYSTAL_FLAG))
        {
            return false;
        }

        // TODO: must send error message (what kind of message? On-screen?)
        return true;
    }
};

/*######
## go_tele_to_violet_stand
######*/

struct go_tele_to_violet_stand : public GameObjectScript
{
    go_tele_to_violet_stand() : GameObjectScript("go_tele_to_violet_stand") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (pPlayer->GetQuestRewardStatus(QUEST_LEARN_LEAVE_RETURN) || pPlayer->GetQuestStatus(QUEST_LEARN_LEAVE_RETURN) == QUEST_STATUS_INCOMPLETE)
        {
            return false;
        }

        // TODO: must send error message (what kind of message? On-screen?)
        return true;
    }
};
#endif

/*######
## go_andorhal_tower
######*/

enum
{
    QUEST_ALL_ALONG_THE_WATCHTOWERS_ALLIANCE = 5097,
    QUEST_ALL_ALONG_THE_WATCHTOWERS_HORDE    = 5098,
    NPC_ANDORHAL_TOWER_1                     = 10902,
    NPC_ANDORHAL_TOWER_2                     = 10903,
    NPC_ANDORHAL_TOWER_3                     = 10904,
    NPC_ANDORHAL_TOWER_4                     = 10905,
    GO_ANDORHAL_TOWER_1                      = 176094,
    GO_ANDORHAL_TOWER_2                      = 176095,
    GO_ANDORHAL_TOWER_3                      = 176096,
    GO_ANDORHAL_TOWER_4                      = 176097
};

struct go_andorhal_tower : public GameObjectScript
{
    go_andorhal_tower() : GameObjectScript("go_andorhal_tower") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (pPlayer->GetQuestStatus(QUEST_ALL_ALONG_THE_WATCHTOWERS_ALLIANCE) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(QUEST_ALL_ALONG_THE_WATCHTOWERS_HORDE) == QUEST_STATUS_INCOMPLETE)
        {
            uint32 uiKillCredit = 0;
            switch (pGo->GetEntry())
            {
            case GO_ANDORHAL_TOWER_1:
                uiKillCredit = NPC_ANDORHAL_TOWER_1;
                break;
            case GO_ANDORHAL_TOWER_2:
                uiKillCredit = NPC_ANDORHAL_TOWER_2;
                break;
            case GO_ANDORHAL_TOWER_3:
                uiKillCredit = NPC_ANDORHAL_TOWER_3;
                break;
            case GO_ANDORHAL_TOWER_4:
                uiKillCredit = NPC_ANDORHAL_TOWER_4;
                break;
            }
            if (uiKillCredit)
            {
                pPlayer->KilledMonsterCredit(uiKillCredit);
            }
        }
        return true;
    }
};

#if defined (CLASSIC) || defined (TBC)
enum
{
    GOSSIP_TABLE_THEKA = 1653,
    QUEST_SPIDER_GOD = 2936
};

struct go_table_theka : public GameObjectScript
{
    go_table_theka() : GameObjectScript("go_table_theka") {}

    bool OnGossipHello(Player* pPlayer, GameObject* pGo) override
    {
        if (pPlayer->GetQuestStatus(QUEST_SPIDER_GOD) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->AreaExploredOrEventHappens(QUEST_SPIDER_GOD);
        }

        pPlayer->SEND_GOSSIP_MENU(GOSSIP_TABLE_THEKA, pGo->GetObjectGuid());

        return true;
    }
};


// go_fixed_trap

#endif


#if defined (WOTLK) || defined (CATA) || defined(MISTS)
/*######
## go_scourge_enclosure
######*/

enum
{
    SPELL_GYMER_LOCK_EXPLOSION      = 55529,
    NPC_GYMER_LOCK_DUMMY            = 29928
};

struct go_scourge_enclosure : public GameObjectScript
{
    go_scourge_enclosure() : GameObjectScript("go_scourge_enclosure") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        std::list<Creature*> m_lResearchersList;
        GetCreatureListWithEntryInGrid(m_lResearchersList, pGo, NPC_GYMER_LOCK_DUMMY, 15.0f);
        if (!m_lResearchersList.empty())
        {
            for (std::list<Creature*>::iterator itr = m_lResearchersList.begin(); itr != m_lResearchersList.end(); ++itr)
            {
                (*itr)->CastSpell((*itr), SPELL_GYMER_LOCK_EXPLOSION, true);
            }
        }
        pPlayer->KilledMonsterCredit(NPC_GYMER_LOCK_DUMMY);
        return true;
    }
};

/*######
## go_lab_work_reagents
######*/

enum
{
    QUEST_LAB_WORK                          = 12557,

    SPELL_WIRHERED_BATWING_KILL_CREDIT      = 51226,
    SPELL_MUDDY_MIRE_MAGGOT_KILL_CREDIT     = 51227,
    SPELL_AMBERSEED_KILL_CREDIT             = 51228,
    SPELL_CHILLED_SERPENT_MUCUS_KILL_CREDIT = 51229,

    GO_AMBERSEED                            = 190459,
    GO_CHILLED_SERPENT_MUCUS                = 190462,
    GO_WITHERED_BATWING                     = 190473,
    GO_MUDDY_MIRE_MAGGOTS                   = 190478,
};

struct go_lab_work_reagents : public GameObjectScript
{
    go_lab_work_reagents() : GameObjectScript("go_lab_work_reagents") {}

    bool GOUse_go_lab_work_reagents(Player* pPlayer, GameObject* pGo)
    {
        if (pPlayer->GetQuestStatus(QUEST_LAB_WORK) == QUEST_STATUS_INCOMPLETE)
        {
            uint32 uiCreditSpellId = 0;
            switch (pGo->GetEntry())
            {
            case GO_AMBERSEED:              uiCreditSpellId = SPELL_AMBERSEED_KILL_CREDIT; break;
            case GO_CHILLED_SERPENT_MUCUS:  uiCreditSpellId = SPELL_CHILLED_SERPENT_MUCUS_KILL_CREDIT; break;
            case GO_WITHERED_BATWING:       uiCreditSpellId = SPELL_WIRHERED_BATWING_KILL_CREDIT; break;
            case GO_MUDDY_MIRE_MAGGOTS:     uiCreditSpellId = SPELL_MUDDY_MIRE_MAGGOT_KILL_CREDIT; break;
            }

            if (uiCreditSpellId)
            {
                pPlayer->CastSpell(pPlayer, uiCreditSpellId, true);
            }
        }

        return false;
    }
};
#endif

void AddSC_go_scripts()
{
    Script* s;
    s = new go_barov_journal();
    s->RegisterSelf();
    s = new go_andorhal_tower();
    s->RegisterSelf();

#if defined (CLASSIC) || defined (TBC)
    s = new go_table_theka();
    s->RegisterSelf();

#endif

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new go_ethereum_prison();
    s->RegisterSelf();

    s = new go_ethereum_stasis();
    s->RegisterSelf();

    s = new go_jump_a_tron();
    s->RegisterSelf();
#endif

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    s = new go_mysterious_snow_mound();
    s->RegisterSelf();

    s = new go_tele_to_dalaran_crystal();
    s->RegisterSelf();

    s = new go_tele_to_violet_stand();
    s->RegisterSelf();

    s = new go_scourge_enclosure();
    s->RegisterSelf();

    s = new go_lab_work_reagents();
    s->RegisterSelf();
#endif

    //pNewScript = new Script;
    //pNewScript->Name = "go_barov_journal";
    //pNewScript->pGOUse =          &GOUse_go_barov_journal;
    //pNewScript->RegisterSelf();

//#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
    //pNewScript = new Script;
    //pNewScript->Name = "go_ethereum_prison";
    //pNewScript->pGOUse =          &GOUse_go_ethereum_prison;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_ethereum_stasis";
    //pNewScript->pGOUse =          &GOUse_go_ethereum_stasis;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_jump_a_tron";
    //pNewScript->pGOUse =          &GOUse_go_jump_a_tron;
    //pNewScript->RegisterSelf();
//#endif
//#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    //pNewScript = new Script;
    //pNewScript->Name = "go_mysterious_snow_mound";
    //pNewScript->pGOUse =          &GOUse_go_mysterious_snow_mound;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_tele_to_dalaran_crystal";
    //pNewScript->pGOUse =          &GOUse_go_tele_to_dalaran_crystal;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_tele_to_violet_stand";
    //pNewScript->pGOUse =          &GOUse_go_tele_to_violet_stand;
    //pNewScript->RegisterSelf();
//#endif

    //pNewScript = new Script;
    //pNewScript->Name = "go_andorhal_tower";
    //pNewScript->pGOUse =          &GOUse_go_andorhal_tower;
    //pNewScript->RegisterSelf();

//#if defined (CLASSIC) || defined (TBC)
    //pNewScript = new Script;
    //pNewScript->Name = "go_table_theka";
    //pNewScript->pGossipHelloGO =  &GossipHelloGO_table_theka;
    //pNewScript->RegisterSelf();
//#endif

//#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    //pNewScript = new Script;
    //pNewScript->Name = "go_scourge_enclosure";
    //pNewScript->pGOUse =          &GOUse_go_scourge_enclosure;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "go_lab_work_reagents";
    //pNewScript->pGOUse =          &GOUse_go_lab_work_reagents;
    //pNewScript->RegisterSelf();
//#endif
}
