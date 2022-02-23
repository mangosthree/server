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
 * SDName:      Instance_Stratholme
 * SD%Complete: 70
 * SDComment:   Undead side 90% implemented, event needs better implementation, Barthildas relocation for reload case is missing, Baron Combat handling is buggy.
 * SDCategory:  Stratholme
 * EndScriptData
 */

#include "precompiled.h"
#include "stratholme.h"

struct EventLocation
{
    float m_fX, m_fY, m_fZ, m_fO;
};

static const EventLocation aStratholmeLocation[] =
{
    { 3725.577f, -3599.484f, 142.367f, 0.0f },                // Barthilas door run
    { 4068.284f, -3535.678f, 122.771f, 2.50f },               // Barthilas tele
    { 4032.643f, -3378.546f, 119.752f, 4.74f },               // Ramstein summon loc
    { 4032.843f, -3390.246f, 119.732f, 0.0f },                // Ramstein move loc
    { 3969.357f, -3391.871f, 119.116f, 5.91f },               // Skeletons summon loc
    { 4033.044f, -3431.031f, 119.055f, 0.0f },                // Skeletons move loc
    { 4032.602f, -3378.506f, 119.752f, 4.74f },               // Guards summon loc
    { 4044.78f,  -3333.68f,  117.26f,  4.15f},                 // Ysida summon loc
    { 4041.9f,   -3337.6f,   115.06f,  3.82f},                  // Ysida move/death loc
    { 3713.681f, -3427.814f, 131.198f, 6.2f }                 // The Unforgiven spawn area
};

static const EventLocation aTimmyLocation[] =
{
    { 3696.851f, -3152.736f, 127.661f, 4.024f },              // Timmy spawn loc
    { 3668.603f, -3183.314f, 126.215f, 0.0f }                 // Courtyard mobs sort point
};

struct ZigguratStore
{
    ObjectGuid m_doorGuid;
    ObjectGuid m_crystalGuid;
    GuidList m_lZigguratAcolyteGuid;
};

static bool sortByHeight(Creature* pFirst, Creature* pSecond)
{
    return pFirst && pSecond && pFirst->GetPositionZ() > pSecond->GetPositionZ();
}

struct is_stratholme : public InstanceScript
{
    is_stratholme() : InstanceScript("instance_stratholme") {}

    class instance_stratholme : public ScriptedInstance
    {
    public:
        instance_stratholme(Map* pMap) : ScriptedInstance(pMap),
            m_uiBaronRunTimer(0),
            m_uiBarthilasRunTimer(0),
            m_uiMindlessSummonTimer(0),
            m_uiSlaugtherSquareTimer(0),
            m_uiYellCounter(0),
            m_uiMindlessCount(0),
            m_uiPostboxesUsed(0),
            m_uiSilverHandKilled(0),
            m_bTheUnforgivenSpawnHasTriggered(0)
        {
            Initialize();
        }

        ~instance_stratholme() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_BARON:
        case NPC_YSIDA:
            case NPC_YSIDA_TRIGGER:
            case NPC_BARTHILAS:
            case NPC_PALADIN_QUEST_CREDIT:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;

            case NPC_CRYSTAL:
                m_luiCrystalGUIDs.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_ABOM_BILE:
            case NPC_ABOM_VENOM:
                m_sAbomnationGUID.insert(pCreature->GetObjectGuid());
                break;
            case NPC_THUZADIN_ACOLYTE:
                m_luiAcolyteGUIDs.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_CRIMSON_INITIATE:
            case NPC_CRIMSON_GALLANT:
            case NPC_CRIMSON_GUARDSMAN:
            case NPC_CRIMSON_CONJURER:
                // Only store those in the yard
                if (pCreature->IsWithinDist2d(aTimmyLocation[1].m_fX, aTimmyLocation[1].m_fY, 40.0f))
                {
                    m_suiCrimsonLowGuids.insert(pCreature->GetGUIDLow());
                }
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_SERVICE_ENTRANCE:
                break;
            case GO_GAUNTLET_GATE1:
                // TODO
                // weird, but unless flag is set, client will not respond as expected. DB bug?
                pGo->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_LOCKED);
                break;

            case GO_ZIGGURAT_DOOR_1:
                m_zigguratStorage[0].m_doorGuid = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_BARONESS] == DONE || m_auiEncounter[TYPE_BARONESS] == SPECIAL)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                return;
            case GO_ZIGGURAT_DOOR_2:
                m_zigguratStorage[1].m_doorGuid = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_NERUB] == DONE || m_auiEncounter[TYPE_NERUB] == SPECIAL)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                return;
            case GO_ZIGGURAT_DOOR_3:
                m_zigguratStorage[2].m_doorGuid = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_PALLID] == DONE || m_auiEncounter[TYPE_PALLID] == SPECIAL)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                return;

            case GO_ZIGGURAT_DOOR_4:
                if (m_auiEncounter[TYPE_RAMSTEIN] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_ZIGGURAT_DOOR_5:
                if (m_auiEncounter[TYPE_RAMSTEIN] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_PORT_GAUNTLET:
                if (m_auiEncounter[TYPE_BARONESS] == SPECIAL && m_auiEncounter[TYPE_NERUB] == SPECIAL && m_auiEncounter[TYPE_PALLID] == SPECIAL)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_PORT_SLAUGTHER:
                if (m_auiEncounter[TYPE_BARONESS] == SPECIAL && m_auiEncounter[TYPE_NERUB] == SPECIAL && m_auiEncounter[TYPE_PALLID] == SPECIAL)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_PORT_SLAUGHTER_GATE:
                if (m_auiEncounter[TYPE_RAMSTEIN] == DONE)      // Might actually be uneeded
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_PORT_ELDERS:
            case GO_YSIDA_CAGE:
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            // TODO: Remove the hard-coded indexes from array accessing
            switch (uiType)
            {
            case TYPE_BARON_RUN:
                switch (uiData)
                {
                case IN_PROGRESS:
                    if (m_auiEncounter[uiType] == IN_PROGRESS || m_auiEncounter[uiType] == FAIL)
                    {
                        break;
                    }

                    // Baron ultimatum starts: summon Ysida in the case
            // Baron ultimatum starts: summon Ysida in the cage
                    if (Creature* pBaron = GetSingleCreatureFromStorage(NPC_BARON))
                    {
                        DoOrSimulateScriptTextForThisInstance(SAY_ANNOUNCE_RUN_START, NPC_BARON);
                        pBaron->SummonCreature(NPC_YSIDA, aStratholmeLocation[7].m_fX, aStratholmeLocation[7].m_fY, aStratholmeLocation[7].m_fZ, aStratholmeLocation[7].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0);
                    }

                    m_uiBaronRunTimer = 45 * MINUTE * IN_MILLISECONDS;
                    debug_log("SD3: Instance Stratholme: Baron run in progress.");
                    break;
                case FAIL:
                    // may add code to remove aura from players, but in theory the time should be up already and removed.
                    break;
                case DONE:
                    m_uiBaronRunTimer = 0;
                    break;
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_BARONESS:
            case TYPE_NERUB:
            case TYPE_PALLID:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoSortZiggurats();
                    DoUseDoorOrButton(m_zigguratStorage[uiType - TYPE_BARONESS].m_doorGuid);
                }
                if (uiData == SPECIAL)
                {
                    StartSlaugtherSquare();
                }
                break;
            case TYPE_RAMSTEIN:
                if (uiData == SPECIAL)
                {
                    if (m_auiEncounter[uiType] != SPECIAL && m_auiEncounter[uiType] != DONE)
                    {
                        m_uiSlaugtherSquareTimer = 20000;       // TODO - unknown, also possible that this is not the very correct place..
                        DoUseDoorOrButton(GO_PORT_GAUNTLET);
                    }

                    uint32 uiCount = m_sAbomnationGUID.size();
                    for (GuidSet::iterator itr = m_sAbomnationGUID.begin(); itr != m_sAbomnationGUID.end();)
                    {
                        if (Creature* pAbom = instance->GetCreature(*itr))
                        {
                            ++itr;
                            if (!pAbom->IsAlive())
                            {
                                --uiCount;
                            }
                        }
                        else
                        {
                            // Remove obsolete guid from set and decrement count
                            m_sAbomnationGUID.erase(itr++);
                            --uiCount;
                        }
                    }

                    if (!uiCount)
                    {
                        // Old Comment: a bit itchy, it should close GO_ZIGGURAT_DOOR_4 door after 10 secs, but it doesn't. skipping it for now.
                        // However looks like that this door is no more closed
                        DoUseDoorOrButton(GO_ZIGGURAT_DOOR_4);

                        // No more handling of Abominations
                        m_uiSlaugtherSquareTimer = 0;

                        if (Creature* pBaron = GetSingleCreatureFromStorage(NPC_BARON))
                        {
                            DoScriptText(SAY_ANNOUNCE_RAMSTEIN, pBaron);
                            if (Creature* pRamstein = pBaron->SummonCreature(NPC_RAMSTEIN, aStratholmeLocation[2].m_fX, aStratholmeLocation[2].m_fY, aStratholmeLocation[2].m_fZ, aStratholmeLocation[2].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0))
                            {
                                pRamstein->GetMotionMaster()->MovePoint(0, aStratholmeLocation[3].m_fX, aStratholmeLocation[3].m_fY, aStratholmeLocation[3].m_fZ);
                            }

                            debug_log("SD3: Instance Stratholme - Slaughter event: Ramstein spawned.");
                        }
                    }
                    else
                    {
                        debug_log("SD3: Instance Stratholme - Slaughter event: %u Abomination left to kill.", uiCount);
                    }
                }
                // After fail aggroing Ramstein means wipe on Ramstein, so close door again
                if (uiData == IN_PROGRESS && m_auiEncounter[uiType] == FAIL)
                {
                    DoUseDoorOrButton(GO_PORT_GAUNTLET);
                }
                if (uiData == DONE)
                {
                    // Open side gate and start summoning skeletons
                    DoUseDoorOrButton(GO_PORT_SLAUGHTER_GATE);
                    // use this timer as a bool just to start summoning
                    m_uiMindlessSummonTimer = 500;
                    m_uiMindlessCount = 0;
                    m_luiUndeadGUIDs.clear();

                    // Summon 5 guards
                    if (Creature* pBaron = GetSingleCreatureFromStorage(NPC_BARON))
                    {
                        for (uint8 i = 0; i < 5; ++i)
                        {
                            float fX, fY, fZ;
                            pBaron->GetRandomPoint(aStratholmeLocation[6].m_fX, aStratholmeLocation[6].m_fY, aStratholmeLocation[6].m_fZ, 5.0f, fX, fY, fZ);
                            if (Creature* pTemp = pBaron->SummonCreature(NPC_BLACK_GUARD, aStratholmeLocation[6].m_fX, aStratholmeLocation[6].m_fY, aStratholmeLocation[6].m_fZ, aStratholmeLocation[6].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0))
                            {
                                m_luiGuardGUIDs.push_back(pTemp->GetObjectGuid());
                            }
                        }

                        debug_log("SD3: Instance Stratholme - Slaughter event: Summoned 5 guards.");
                    }
                }
                // Open Door again and stop Abomination
                if (uiData == FAIL && m_auiEncounter[uiType] != FAIL)
                {
                    DoUseDoorOrButton(GO_PORT_GAUNTLET);
                    m_uiSlaugtherSquareTimer = 0;

                    // Let already moving Abominations stop
                    for (GuidSet::const_iterator itr = m_sAbomnationGUID.begin(); itr != m_sAbomnationGUID.end(); ++itr)
                    {
                        Creature* pAbom = instance->GetCreature(*itr);
                        if (pAbom && pAbom->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
                        {
                            pAbom->GetMotionMaster()->MovementExpired();
                        }
                    }
                }

                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_BARON:
                if (uiData == IN_PROGRESS)
                {


                    // Close Slaughterhouse door if needed
                    if (m_auiEncounter[uiType] == FAIL)
                    {
                        DoUseDoorOrButton(GO_PORT_GAUNTLET);
                    }
                }
                if (uiData == DONE)
                {
                    // Players successfully engaged Baron within the time-limit of his ultimatum
                //     Note: UpdateAI() prevents TYPE_BARON_RUN to be marked as FAILED if the
                //     Baron is already engaged (in progress) when the ultimatum timer expires
                if (m_auiEncounter[TYPE_BARON_RUN] == IN_PROGRESS)
                    {
            SetData(TYPE_BARON_RUN, DONE);
                        Map::PlayerList const& players = instance->GetPlayers();

                        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                        {
                            if (Player* pPlayer = itr->getSource())
                            {
                                if (pPlayer->HasAura(SPELL_BARON_ULTIMATUM))
                                {
                                    pPlayer->RemoveAurasDueToSpell(SPELL_BARON_ULTIMATUM);
                                }

                                if (pPlayer->GetQuestStatus(QUEST_DEAD_MAN_PLEA) == QUEST_STATUS_INCOMPLETE)
                                {
                                    pPlayer->AreaExploredOrEventHappens(QUEST_DEAD_MAN_PLEA);

                    // Argent Dawn reputation reward
                                    pPlayer->CastSpell(pPlayer, SPELL_YSIDA_FREED, true);
                                }
                            }
                        }

                        // Open cage, finish rescue event
                        if (Creature* pYsida = GetSingleCreatureFromStorage(NPC_YSIDA))
                        {
                            DoScriptText(SAY_EPILOGUE, pYsida);
                            DoUseDoorOrButton(GO_YSIDA_CAGE);
                            pYsida->GetMotionMaster()->MovePoint(0, aStratholmeLocation[8].m_fX, aStratholmeLocation[8].m_fY, aStratholmeLocation[8].m_fZ, aStratholmeLocation[8].m_fO);
                        }
                    }

                    // Open Slaughterhouse door again
                    DoUseDoorOrButton(GO_PORT_GAUNTLET);
                }
                if (uiData == FAIL)
                {
                    DoUseDoorOrButton(GO_PORT_GAUNTLET);
                }

                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_BARTHILAS_RUN:
                if (uiData == IN_PROGRESS)
                {
                    Creature* pBarthilas = GetSingleCreatureFromStorage(NPC_BARTHILAS);
                    if (pBarthilas && pBarthilas->IsAlive() && !pBarthilas->IsInCombat())
                    {
                        DoScriptText(SAY_WARN_BARON, pBarthilas);
                        pBarthilas->SetWalk(false);
                        pBarthilas->GetMotionMaster()->MovePoint(0, aStratholmeLocation[0].m_fX, aStratholmeLocation[0].m_fY, aStratholmeLocation[0].m_fZ);

                        m_uiBarthilasRunTimer = 8000;
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_BLACK_GUARDS:
                // Prevent double action
                if (m_auiEncounter[uiType] == uiData)
                {
                    return;
                }

                // Restart after failure, close Gauntlet
                if (uiData == IN_PROGRESS && m_auiEncounter[uiType] == FAIL)
                {
                    DoUseDoorOrButton(GO_PORT_GAUNTLET);
                }
                // Wipe case - open gauntlet
                if (uiData == FAIL)
                {
                    DoUseDoorOrButton(GO_PORT_GAUNTLET);
                }
                if (uiData == DONE)
                {
                    if (Creature* pBaron = GetSingleCreatureFromStorage(NPC_BARON))
                    {
                        DoScriptText(SAY_UNDEAD_DEFEAT, pBaron);
                    }
                    DoUseDoorOrButton(GO_ZIGGURAT_DOOR_5);
                }
                m_auiEncounter[uiType] = uiData;

                // No need to save anything here, so return
                return;
            case TYPE_POSTMASTER:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    ++m_uiPostboxesUsed;

                    // After the second post box prepare to spawn the Post Master
                    if (m_uiPostboxesUsed == 2)
                    {
                        SetData(TYPE_POSTMASTER, SPECIAL);
                    }
                }
                // No need to save anything here, so return
                return;
            case TYPE_TRUE_MASTERS:
                m_auiEncounter[uiType] = uiData;
                if (uiData == SPECIAL)
                {
                    ++m_uiSilverHandKilled;

                    // When the 5th paladin is killed set data to DONE in order to give the quest credit for the last paladin
                    if (m_uiSilverHandKilled == MAX_SILVERHAND)
                    {
                        SetData(TYPE_TRUE_MASTERS, DONE);
                    }
                }
                // No need to save anything here, so return
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5] << " " << m_auiEncounter[6];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            switch (uiType)
            {
            case TYPE_BARON_RUN:
            case TYPE_BARONESS:
            case TYPE_NERUB:
            case TYPE_PALLID:
            case TYPE_RAMSTEIN:
            case TYPE_BARON:
            case TYPE_BARTHILAS_RUN:
            case TYPE_POSTMASTER:
            case TYPE_TRUE_MASTERS:
                return m_auiEncounter[uiType];
            default:
                return 0;
            }
        }

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3]
                >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            // Special Treatment for the Ziggurat-Bosses, as otherwise the event couldn't reload
            if (m_auiEncounter[TYPE_BARONESS] == DONE)
            {
                m_auiEncounter[TYPE_BARONESS] = SPECIAL;
            }
            if (m_auiEncounter[TYPE_NERUB] == DONE)
            {
                m_auiEncounter[TYPE_NERUB] = SPECIAL;
            }
            if (m_auiEncounter[TYPE_PALLID] == DONE)
            {
                m_auiEncounter[TYPE_PALLID] = SPECIAL;
            }

// Baron ultimatum succeed: summon Ysida outside the cage alive
    if (m_auiEncounter[TYPE_BARON_RUN] == DONE)
    {
        if (Creature* pBaron = GetSingleCreatureFromStorage(NPC_BARON))
        {
            pBaron->SummonCreature(NPC_YSIDA, aStratholmeLocation[9].m_fX, aStratholmeLocation[9].m_fY, aStratholmeLocation[9].m_fZ, aStratholmeLocation[9].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0);
        }
    }
    // Baron ultimatum failed: summon Ysida outside the cage dead
    if (m_auiEncounter[TYPE_BARON_RUN] == FAIL)
    {
        if (Creature* pBaron = GetSingleCreatureFromStorage(NPC_BARON))
            if (Creature* pYsida = pBaron->SummonCreature(NPC_YSIDA, aStratholmeLocation[9].m_fX, aStratholmeLocation[9].m_fY, aStratholmeLocation[9].m_fZ, aStratholmeLocation[9].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0))
            {
                pYsida->DealDamage(pYsida, pYsida->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NONE, nullptr, false);
            }
    }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_BARONESS_ANASTARI:
                SetData(TYPE_BARONESS, IN_PROGRESS);
                break;
            case NPC_MALEKI_THE_PALLID:
                SetData(TYPE_PALLID, IN_PROGRESS);
                break;
            case NPC_NERUBENKAN:
                SetData(TYPE_NERUB, IN_PROGRESS);
                break;
            case NPC_RAMSTEIN:
                SetData(TYPE_RAMSTEIN, IN_PROGRESS);
                break;
            case NPC_BARON:
        SetData(TYPE_BARON, IN_PROGRESS);
        break;
            case NPC_ABOM_BILE:
            case NPC_ABOM_VENOM:
                // Start Slaughterhouse Event
                SetData(TYPE_RAMSTEIN, SPECIAL);
                break;

            case NPC_MINDLESS_UNDEAD:
            case NPC_BLACK_GUARD:
                // Aggro in Slaughterhouse after Ramstein
                SetData(TYPE_BLACK_GUARDS, IN_PROGRESS);
                break;
            }
        }

        void OnCreatureEvade(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_BARONESS_ANASTARI:
                SetData(TYPE_BARONESS, FAIL);
                break;
            case NPC_MALEKI_THE_PALLID:
                SetData(TYPE_PALLID, FAIL);
                break;
            case NPC_NERUBENKAN:
                SetData(TYPE_NERUB, FAIL);
                break;
            case NPC_RAMSTEIN:
                SetData(TYPE_RAMSTEIN, FAIL);
                break;
            case NPC_BARON:
        SetData(TYPE_BARON, FAIL);
        break;
            case NPC_ABOM_BILE:
            case NPC_ABOM_VENOM:
                // Fail in Slaughterhouse Event before Ramstein
                SetData(TYPE_RAMSTEIN, FAIL);
                break;
            case NPC_MINDLESS_UNDEAD:
            case NPC_BLACK_GUARD:
                // Fail in Slaughterhouse after Ramstein
                SetData(TYPE_BLACK_GUARDS, FAIL);
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_BARONESS_ANASTARI:
                SetData(TYPE_BARONESS, DONE);
                break;
            case NPC_MALEKI_THE_PALLID:
                SetData(TYPE_PALLID, DONE);
                break;
            case NPC_NERUBENKAN:
                SetData(TYPE_NERUB, DONE);
                break;
            case NPC_RAMSTEIN:
                SetData(TYPE_RAMSTEIN, DONE);
                break;
            case NPC_BARON:
                SetData(TYPE_BARON, DONE);
                break;

            case NPC_THUZADIN_ACOLYTE:
                ThazudinAcolyteJustDied(pCreature);
                break;

            case NPC_ABOM_BILE:
            case NPC_ABOM_VENOM:
                // Start Slaughterhouse Event
                SetData(TYPE_RAMSTEIN, SPECIAL);
                break;

            case NPC_MINDLESS_UNDEAD:
                m_luiUndeadGUIDs.remove(pCreature->GetObjectGuid());
                if (m_luiUndeadGUIDs.empty())
                {
                    // Let the black Guards move out of the citadel
                    for (GuidList::const_iterator itr = m_luiGuardGUIDs.begin(); itr != m_luiGuardGUIDs.end(); ++itr)
                    {
                        Creature* pGuard = instance->GetCreature(*itr);
                        if (pGuard && pGuard->IsAlive() && !pGuard->IsInCombat())
                        {
                            float fX, fY, fZ;
                            pGuard->GetRandomPoint(aStratholmeLocation[5].m_fX, aStratholmeLocation[5].m_fY, aStratholmeLocation[5].m_fZ, 10.0f, fX, fY, fZ);
                            pGuard->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
                        }
                    }
                }
                break;
            case NPC_BLACK_GUARD:
                m_luiGuardGUIDs.remove(pCreature->GetObjectGuid());
                if (m_luiGuardGUIDs.empty())
                {
                    SetData(TYPE_BLACK_GUARDS, DONE);
                }

                break;

                // Timmy spawn support
            case NPC_CRIMSON_INITIATE:
            case NPC_CRIMSON_GALLANT:
            case NPC_CRIMSON_GUARDSMAN:
            case NPC_CRIMSON_CONJURER:
                if (m_suiCrimsonLowGuids.find(pCreature->GetGUIDLow()) != m_suiCrimsonLowGuids.end())
                {
                    m_suiCrimsonLowGuids.erase(pCreature->GetGUIDLow());

                    // If all courtyard mobs are dead then summon Timmy
                    if (m_suiCrimsonLowGuids.empty())
                    {
                        pCreature->SummonCreature(NPC_TIMMY_THE_CRUEL, aTimmyLocation[0].m_fX, aTimmyLocation[0].m_fY, aTimmyLocation[0].m_fZ, aTimmyLocation[0].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0);
                    }
                }
                break;
            }
        }

        void Update(uint32 uiDiff) override
        {
            if (m_uiBarthilasRunTimer)
            {
                if (m_uiBarthilasRunTimer <= uiDiff)
                {
                    Creature* pBarthilas = GetSingleCreatureFromStorage(NPC_BARTHILAS);
                    if (pBarthilas && pBarthilas->IsAlive() && !pBarthilas->IsInCombat())
                    {
                        pBarthilas->NearTeleportTo(aStratholmeLocation[1].m_fX, aStratholmeLocation[1].m_fY, aStratholmeLocation[1].m_fZ, aStratholmeLocation[1].m_fO);
                    }

                    SetData(TYPE_BARTHILAS_RUN, DONE);
                    m_uiBarthilasRunTimer = 0;
                }
                else
                {
                    m_uiBarthilasRunTimer -= uiDiff;
                }
            }

            // Check changes for Baron ultimatum timer only if Baron is not already in combat
            if (m_uiBaronRunTimer && GetData(TYPE_BARON) != IN_PROGRESS)
            {
                if (m_uiYellCounter == 0 && m_uiBaronRunTimer <= 10 * MINUTE * IN_MILLISECONDS)
                {
                    DoOrSimulateScriptTextForThisInstance(SAY_ANNOUNCE_RUN_10_MIN, NPC_BARON);
                    ++m_uiYellCounter;
                }
                else if (m_uiYellCounter == 1 && m_uiBaronRunTimer <= 5 * MINUTE * IN_MILLISECONDS)
                {
                    DoOrSimulateScriptTextForThisInstance(SAY_ANNOUNCE_RUN_5_MIN, NPC_BARON);
                    ++m_uiYellCounter;
                }

        // Used to create a delay of 10s between Baron speech and Ysida's answer
            else if (m_uiYellCounter == 2 && m_uiBaronRunTimer <= (5 * MINUTE - 10) * IN_MILLISECONDS)
            {
                    DoOrSimulateScriptTextForThisInstance(YSIDA_SAY_RUN_5_MIN, NPC_YSIDA);
                    ++m_uiYellCounter;
            }

                if (m_uiBaronRunTimer <= uiDiff)
                {
                    if (GetData(TYPE_BARON_RUN) != FAIL)
                    {
                    SetData(TYPE_BARON_RUN, FAIL);

                    // Open the cage and let Ysida face her doom
                    if (Creature* pYsida = GetSingleCreatureFromStorage(NPC_YSIDA))
                    {
                            pYsida->GetMotionMaster()->MovePoint(0, aStratholmeLocation[8].m_fX, aStratholmeLocation[8].m_fY, aStratholmeLocation[8].m_fZ, aStratholmeLocation[8].m_fO);
                            DoUseDoorOrButton(GO_YSIDA_CAGE);
                    }
            DoOrSimulateScriptTextForThisInstance(SAY_ANNOUNCE_RUN_FAIL, NPC_BARON);

                        m_uiBaronRunTimer = 8000;  // We reset the timer so the speech of Ysida is not said at the same time than the Baron's one
                    }
                    else
                    {
                        // Baron ultimatum failed: let the Baron kill her
                        if (Creature* pYsida = GetSingleCreatureFromStorage(NPC_YSIDA))
                            if (Creature* pBaron = GetSingleCreatureFromStorage(NPC_BARON))
                            {
                                pBaron->CastSpell(pYsida, SPELL_BARON_SOUL_DRAIN, true);
                            }

                        DoOrSimulateScriptTextForThisInstance(YSIDA_SAY_RUN_FAIL, NPC_YSIDA);

                        m_uiBaronRunTimer = 0;  // event done for good, no more speech
                        debug_log("SD2: Instance Stratholme: Baron run event reached end. Event has state %u.", GetData(TYPE_BARON_RUN));
                    }
                }
                else
                {
                    m_uiBaronRunTimer -= uiDiff;
                }
            }

            if (m_uiMindlessSummonTimer)
            {
                if (m_uiMindlessCount < 30)
                {
                    if (m_uiMindlessSummonTimer <= uiDiff)
                    {
                        if (Creature* pBaron = GetSingleCreatureFromStorage(NPC_BARON))
                        {
                            // Summon mindless skeletons and move them to random point in the center of the square
                            if (Creature* pTemp = pBaron->SummonCreature(NPC_MINDLESS_UNDEAD, aStratholmeLocation[4].m_fX, aStratholmeLocation[4].m_fY, aStratholmeLocation[4].m_fZ, aStratholmeLocation[4].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0))
                            {
                                float fX, fY, fZ;
                                pBaron->GetRandomPoint(aStratholmeLocation[5].m_fX, aStratholmeLocation[5].m_fY, aStratholmeLocation[5].m_fZ, 20.0f, fX, fY, fZ);
                                pTemp->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
                                m_luiUndeadGUIDs.push_back(pTemp->GetObjectGuid());
                                ++m_uiMindlessCount;
                            }
                        }
                        m_uiMindlessSummonTimer = 400;
                    }
                    else
                    {
                        m_uiMindlessSummonTimer -= uiDiff;
                    }
                }
                else
                {
                    m_uiMindlessSummonTimer = 0;
                }
            }

            if (m_uiSlaugtherSquareTimer)
            {
                if (m_uiSlaugtherSquareTimer <= uiDiff)
                {
                    // Call next Abominations
                    for (GuidSet::const_iterator itr = m_sAbomnationGUID.begin(); itr != m_sAbomnationGUID.end(); ++itr)
                    {
                        Creature* pAbom = instance->GetCreature(*itr);
                        // Skip killed and already walking Abominations
                        if (!pAbom || !pAbom->IsAlive() || pAbom->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
                        {
                            continue;
                        }

                        // Let Move to somewhere in the middle
                        if (!pAbom->IsInCombat())
                        {
                            if (GameObject* pDoor = GetSingleGameObjectFromStorage(GO_PORT_SLAUGTHER))
                            {
                                float fX, fY, fZ;
                                pAbom->GetRandomPoint(pDoor->GetPositionX(), pDoor->GetPositionY(), pDoor->GetPositionZ(), 10.0f, fX, fY, fZ);
                                pAbom->GetMotionMaster()->MovePoint(0, fX, fY, fZ);
                            }
                        }
                        break;
                    }

                    // TODO - how fast are they called?
                    m_uiSlaugtherSquareTimer = urand(15000, 30000);
                }
                else
                {
                    m_uiSlaugtherSquareTimer -= uiDiff;
                }
            }

            // Check to see if the spawning of The Unforgiven and its adds has been triggered by a player (player walking into area)
            // Once this has occurred, the respawning is dealt with via the creature object's respawn time (The Unforgiven every 30 minutes, Vengeful Phantoms every 15 minutes)
            if (!m_bTheUnforgivenSpawnHasTriggered)
            {
                // Query the players in the instance
                Map::PlayerList const& players = instance->GetPlayers();
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    if (Player* pPlayer = itr->getSource())
                    {
                        // Acquire player's coordinates
                        float fPlayerXposition = pPlayer->GetPositionX();
                        float fPlayerYposition = pPlayer->GetPositionY();
                        float fPlayerZposition = pPlayer->GetPositionZ();

                        // Check if player is near The Unforgiven
                        // If a player is near, then we do not need to check other player locations, therefore stop checking - break out of this
// TODO: IsNearWaypoint is missing from TWO

                        if (pPlayer->IsNearWaypoint(fPlayerXposition, fPlayerYposition, fPlayerZposition, aStratholmeLocation[9].m_fX, aStratholmeLocation[9].m_fY, aStratholmeLocation[9].m_fZ, 4, 4, 4))
                        {
                            Creature* pTheUnforgiven = pPlayer->SummonCreature(NPC_THE_UNFORGIVEN, aStratholmeLocation[9].m_fX, aStratholmeLocation[9].m_fY, aStratholmeLocation[9].m_fZ, aStratholmeLocation[9].m_fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 7200000);
                            pTheUnforgiven->SetRespawnTime(1800); // 30 minutes
                            // Now spawn 3 or 4 adds (NPC_VENGEFUL_PHANTOM)
                            Creature* pVengfulPhantom[4];
                            uint8 iTotalAddsToSpawn = 3 + rand() % 2;
                            for (uint8 i = 0; i < iTotalAddsToSpawn; i++)
                            {
                                pVengfulPhantom[i] = pPlayer->SummonCreature(NPC_VENGEFUL_PHANTOM, aStratholmeLocation[9].m_fX, aStratholmeLocation[9].m_fY, aStratholmeLocation[9].m_fZ, aStratholmeLocation[9].m_fO, TEMPSPAWN_CORPSE_TIMED_DESPAWN, 7200000);
                                pVengfulPhantom[i]->SetRespawnTime(900); // 15 minutes
                            }
                            m_bTheUnforgivenSpawnHasTriggered = true;
                            break;
                        }
                    }
                }
            }

        }

    private:
        bool StartSlaugtherSquare()
        {
            if (m_auiEncounter[TYPE_BARONESS] == SPECIAL && m_auiEncounter[TYPE_NERUB] == SPECIAL && m_auiEncounter[TYPE_PALLID] == SPECIAL)
            {
                DoOrSimulateScriptTextForThisInstance(SAY_ANNOUNCE_RIVENDARE, NPC_BARON);

                DoUseDoorOrButton(GO_PORT_GAUNTLET);
                DoUseDoorOrButton(GO_PORT_SLAUGTHER);

                debug_log("SD3: Instance Stratholme: Open slaughter square.");

                return true;
            }

            return false;
        }

        void DoSortZiggurats()
        {
            if (m_luiAcolyteGUIDs.empty())
            {
                return;
            }

            std::list<Creature*> lAcolytes;                         // Valid pointers, only used locally
            for (GuidList::const_iterator itr = m_luiAcolyteGUIDs.begin(); itr != m_luiAcolyteGUIDs.end(); ++itr)
            {
                if (Creature* pAcolyte = instance->GetCreature(*itr))
                {
                    lAcolytes.push_back(pAcolyte);
                }
            }
            m_luiAcolyteGUIDs.clear();

            if (lAcolytes.empty())
            {
                return;
            }

            if (!GetSingleCreatureFromStorage(NPC_THUZADIN_ACOLYTE, true))
            {
                // Sort the acolytes by height, and the one with the biggest height is the announcer (a bit outside the map)
                lAcolytes.sort(sortByHeight);
                m_mNpcEntryGuidStore[NPC_THUZADIN_ACOLYTE] = (*lAcolytes.begin())->GetObjectGuid();
                lAcolytes.erase(lAcolytes.begin());
            }

            // Sort Acolytes
            for (std::list<Creature*>::iterator itr = lAcolytes.begin(); itr != lAcolytes.end();)
            {
                bool bAlreadyIterated = false;
                for (uint8 i = 0; i < MAX_ZIGGURATS; ++i)
                {
                    if (GameObject* pZigguratDoor = instance->GetGameObject(m_zigguratStorage[i].m_doorGuid))
                    {
                        if ((*itr)->IsAlive() && (*itr)->IsWithinDistInMap(pZigguratDoor, 35.0f, false))
                        {
                            m_zigguratStorage[i].m_lZigguratAcolyteGuid.push_back((*itr)->GetObjectGuid());
                            itr = lAcolytes.erase(itr);
                            bAlreadyIterated = true;
                            break;
                        }
                    }
                }

                if (itr != lAcolytes.end() && !bAlreadyIterated)
                {
                    ++itr;
                }
            }

            // In case some mobs have not been able to be sorted, store their GUIDs again
            for (std::list<Creature*>::const_iterator itr = lAcolytes.begin(); itr != lAcolytes.end(); ++itr)
            {
                m_luiAcolyteGUIDs.push_back((*itr)->GetObjectGuid());
            }

            // Sort Crystal
            for (GuidList::iterator itr = m_luiCrystalGUIDs.begin(); itr != m_luiCrystalGUIDs.end();)
            {
                Creature* pCrystal = instance->GetCreature(*itr);
                if (!pCrystal)
                {
                    itr = m_luiCrystalGUIDs.erase(itr);
                    continue;
                }

                bool bAlreadyIterated = false;
                for (uint8 i = 0; i < MAX_ZIGGURATS; ++i)
                {
                    if (GameObject* pZigguratDoor = instance->GetGameObject(m_zigguratStorage[i].m_doorGuid))
                    {
                        if (pCrystal->IsWithinDistInMap(pZigguratDoor, 50.0f, false))
                        {
                            m_zigguratStorage[i].m_crystalGuid = pCrystal->GetObjectGuid();
                            itr = m_luiCrystalGUIDs.erase(itr);
                            bAlreadyIterated = true;
                            break;
                        }
                    }
                }

                if (itr != m_luiCrystalGUIDs.end() && !bAlreadyIterated)
                {
                    ++itr;
                }
            }
        }

        void ThazudinAcolyteJustDied(Creature* pCreature)
        {
            for (uint8 i = 0; i < MAX_ZIGGURATS; ++i)
            {
                if (m_zigguratStorage[i].m_lZigguratAcolyteGuid.empty())
                {
                    continue;    // nothing to do anymore for this ziggurat
                }

                m_zigguratStorage[i].m_lZigguratAcolyteGuid.remove(pCreature->GetObjectGuid());
                if (m_zigguratStorage[i].m_lZigguratAcolyteGuid.empty())
                {
                    // A random zone yell after one is cleared
                    int32 aAnnounceSay[MAX_ZIGGURATS] = { SAY_ANNOUNCE_ZIGGURAT_1, SAY_ANNOUNCE_ZIGGURAT_2, SAY_ANNOUNCE_ZIGGURAT_3 };
                    DoOrSimulateScriptTextForThisInstance(aAnnounceSay[i], NPC_THUZADIN_ACOLYTE);

                    // Kill Crystal
                    if (Creature* pCrystal = instance->GetCreature(m_zigguratStorage[i].m_crystalGuid))
                    {
                        pCrystal->DealDamage(pCrystal, pCrystal->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, nullptr, false);
                    }

                    switch (i)
                    {
                    case 0:
                        SetData(TYPE_BARONESS, SPECIAL);
                        break;
                    case 1:
                        SetData(TYPE_NERUB, SPECIAL);
                        break;
                    case 2:
                        SetData(TYPE_PALLID, SPECIAL);
                        break;
                    }
                }
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        uint32 m_uiBaronRunTimer;
        uint32 m_uiBarthilasRunTimer;
        uint32 m_uiMindlessSummonTimer;
        uint32 m_uiSlaugtherSquareTimer;

        uint32 m_uiYellCounter;
        uint32 m_uiMindlessCount;
        uint8 m_uiPostboxesUsed;
        uint8 m_uiSilverHandKilled;

        ZigguratStore m_zigguratStorage[MAX_ZIGGURATS];

        std::set<uint32> m_suiCrimsonLowGuids;
        GuidList m_luiCrystalGUIDs;
        GuidSet m_sAbomnationGUID;
        GuidList m_luiAcolyteGUIDs;
        GuidList m_luiUndeadGUIDs;
        GuidList m_luiGuardGUIDs;

        // this ensures that the code that deals with the initial spawning of The Unforgiven and its adds (Vengful Phantoms) is only run once
        bool m_bTheUnforgivenSpawnHasTriggered;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_stratholme(pMap);
    }
};

void AddSC_instance_stratholme()
{
    Script* s;
    s = new is_stratholme();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_stratholme";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_stratholme;
    //pNewScript->RegisterSelf();
}
