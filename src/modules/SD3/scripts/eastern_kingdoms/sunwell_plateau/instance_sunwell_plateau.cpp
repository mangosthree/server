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
SDName: Instance_Sunwell_Plateau
SD%Complete: 70%
SDComment:
SDCategory: Sunwell_Plateau
EndScriptData */

#include "precompiled.h"
#include "sunwell_plateau.h"

/* Sunwell Plateau:
0 - Kalecgos and Sathrovarr
1 - Brutallus
2 - Felmyst
3 - Eredar Twins (Alythess and Sacrolash)
4 - M'uru
5 - Kil'Jaeden
*/

static const DialogueEntry aFelmystOutroDialogue[] =
{
    {NPC_KALECGOS_MADRIGOSA, 0,                        10000},
    {SAY_KALECGOS_OUTRO,     NPC_KALECGOS_MADRIGOSA,   5000},
    {NPC_FELMYST,            0,                        5000},
    {SPELL_OPEN_BACK_DOOR,   0,                        9000},
    {NPC_BRUTALLUS,          0,                        0},
    {0, 0, 0},
};

static const EventLocations aKalecLoc[] =
{
    { 1573.146f, 755.2025f, 99.524f, 3.59f },         // spawn loc
#if defined (TBC)
    { 1474.235f, 624.0703f, 29.325f },                // first move
    { 1511.655f, 550.7028f, 25.510f },                // open door
    { 1648.255f, 519.377f, 165.848f },                // fly away
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    {1474.235f, 624.0703f, 29.325f, 0},             // first move
    {1511.655f, 550.7028f, 25.510f, 0},             // open door
    {1648.255f, 519.377f, 165.848f, 0},             // fly away
#endif
};

struct is_sunwell_plateau : public InstanceScript
{
    is_sunwell_plateau() : InstanceScript("instance_sunwell_plateau") {}

    class instance_sunwell_plateau : public ScriptedInstance, private DialogueHelper
    {
    public:
        instance_sunwell_plateau(Map* pMap) : ScriptedInstance(pMap), DialogueHelper(aFelmystOutroDialogue),
            m_uiDeceiversKilled(0),
            m_uiSpectralRealmTimer(5000),
            m_uiKalecRespawnTimer(0),
            m_uiMuruBerserkTimer(0),
            m_uiKiljaedenYellTimer(90000),
            m_uiFelmystCorruption(0)
        {
            Initialize();
        }

        ~instance_sunwell_plateau() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
            InitializeDialogueHelper(this);
        }

        bool IsEncounterInProgress() const override
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    return true;
                }
            }

            return false;
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
#if defined (CLASSIC) || defined (TBC)
            // Return if Felmyst already dead, or Brutallus alive
            if (m_auiEncounter[TYPE_BRUTALLUS] != DONE || m_auiEncounter[TYPE_FELMYST] == DONE)
            {
                return;
            }

            // Return if already summoned
            if (GetSingleCreatureFromStorage(NPC_FELMYST, true))
            {
                return;
            }

            // Summon Felmyst in reload case
            pPlayer->SummonCreature(NPC_FELMYST, aMadrigosaLoc[0].m_fX, aMadrigosaLoc[0].m_fY, aMadrigosaLoc[0].m_fZ, aMadrigosaLoc[0].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0);
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
    // Spawn Felmyst if not already dead and Brutallus is complete
    if (m_auiEncounter[TYPE_BRUTALLUS] == DONE && m_auiEncounter[TYPE_FELMYST] != DONE)
    {
        // Summon Felmyst in reload case if not already summoned
        if (!GetSingleCreatureFromStorage(NPC_FELMYST, true))
        {
            pPlayer->SummonCreature(NPC_FELMYST, aMadrigosaLoc[0].m_fX, aMadrigosaLoc[0].m_fY, aMadrigosaLoc[0].m_fZ, aMadrigosaLoc[0].m_fO, TEMPSPAWN_DEAD_DESPAWN, 0, true);
        }
    }

    // Spawn M'uru after the Eredar Twins
    if (m_auiEncounter[TYPE_EREDAR_TWINS] == DONE && m_auiEncounter[TYPE_MURU] != DONE)
    {
        if (!GetSingleCreatureFromStorage(NPC_MURU, true))
        {
            pPlayer->SummonCreature(NPC_MURU, afMuruSpawnLoc[0], afMuruSpawnLoc[1], afMuruSpawnLoc[2], afMuruSpawnLoc[3], TEMPSPAWN_DEAD_DESPAWN, 0, true);
        }
    }
#endif
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_FORCEFIELD:
            case GO_BOSS_COLLISION_1:
            case GO_BOSS_COLLISION_2:
            case GO_ICE_BARRIER:
                break;
            case GO_FIRE_BARRIER:
                if (m_auiEncounter[TYPE_KALECGOS] == DONE && m_auiEncounter[TYPE_BRUTALLUS] == DONE && m_auiEncounter[TYPE_FELMYST] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_FIRST_GATE:
                break;
            case GO_SECOND_GATE:
                if (m_auiEncounter[TYPE_EREDAR_TWINS] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_MURU_ENTER_GATE:
                if (m_auiEncounter[TYPE_EREDAR_TWINS] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_MURU_EXIT_GATE:
                if (m_auiEncounter[TYPE_MURU] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_THIRD_GATE:
                if (m_auiEncounter[TYPE_MURU] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_ORB_BLUE_FLIGHT_1:
            case GO_ORB_BLUE_FLIGHT_2:
            case GO_ORB_BLUE_FLIGHT_3:
            case GO_ORB_BLUE_FLIGHT_4:
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_KALECGOS_DRAGON:
            case NPC_KALECGOS_HUMAN:
            case NPC_SATHROVARR:
            case NPC_FLIGHT_TRIGGER_LEFT:
            case NPC_FLIGHT_TRIGGER_RIGHT:
            case NPC_MADRIGOSA:
            case NPC_BRUTALLUS:
            case NPC_FELMYST:
            case NPC_KALECGOS_MADRIGOSA:
            case NPC_ALYTHESS:
            case NPC_SACROLASH:
            case NPC_MURU:
            case NPC_ENTROPIUS:
            case NPC_KILJAEDEN_CONTROLLER:
            case NPC_KILJAEDEN:
            case NPC_KALECGOS:
            case NPC_ANVEENA:
            case NPC_VELEN:
            case NPC_LIADRIN:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_DECEIVER:
                m_lDeceiversGuidList.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_WORLD_TRIGGER:
                // sort triggers for flightpath
                if (pCreature->GetPositionZ() < 51.0f)
                {
                    m_lAllFlightTriggersList.push_back(pCreature->GetObjectGuid());
                }
                break;
            case NPC_WORLD_TRIGGER_LARGE:
                if (pCreature->GetPositionY() < 523.0f)
                {
                    m_lBackdoorTriggersList.push_back(pCreature->GetObjectGuid());
                }
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_DECEIVER)
            {
                ++m_uiDeceiversKilled;
                // Spawn Kiljaeden when all deceivers are killed
                if (m_uiDeceiversKilled == MAX_DECEIVERS)
                {
                    if (Creature* pController = GetSingleCreatureFromStorage(NPC_KILJAEDEN_CONTROLLER))
                    {
                        if (Creature* pKiljaeden = pController->SummonCreature(NPC_KILJAEDEN, pController->GetPositionX(), pController->GetPositionY(), pController->GetPositionZ(), pController->GetOrientation(), TEMPSPAWN_DEAD_DESPAWN, 0))
                        {
                            pKiljaeden->SetInCombatWithZone();
                        }

                        pController->RemoveAurasDueToSpell(SPELL_ANVEENA_DRAIN);
                    }
                }
            }
        }

        void OnCreatureEvade(Creature* pCreature)
        {
            // Reset encounter if raid wipes at deceivers
            if (pCreature->GetEntry() == NPC_DECEIVER)
            {
                SetData(TYPE_KILJAEDEN, FAIL);
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_KALECGOS:
                m_auiEncounter[uiType] = uiData;
                // combat doors
                DoUseDoorOrButton(GO_FORCEFIELD);
                DoUseDoorOrButton(GO_BOSS_COLLISION_1);
                DoUseDoorOrButton(GO_BOSS_COLLISION_2);
                if (uiData == FAIL)
                {
                    m_uiKalecRespawnTimer = 20000;

                    if (Creature* pKalecDragon = GetSingleCreatureFromStorage(NPC_KALECGOS_DRAGON))
                    {
                        pKalecDragon->ForcedDespawn();
                    }
                    if (Creature* pKalecHuman = GetSingleCreatureFromStorage(NPC_KALECGOS_HUMAN))
                    {
                        pKalecHuman->ForcedDespawn();
                    }
                    if (Creature* pSathrovarr = GetSingleCreatureFromStorage(NPC_SATHROVARR))
                    {
                        pSathrovarr->AI()->EnterEvadeMode();
                    }
                }
                break;
            case TYPE_BRUTALLUS:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_FELMYST:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    StartNextDialogueText(NPC_KALECGOS_MADRIGOSA);
                }
                else if (uiData == IN_PROGRESS)
                {
                    DoSortFlightTriggers();
                }
                break;
            case TYPE_EREDAR_TWINS:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
#if defined (TBC)
                    DoUseDoorOrButton(GO_SECOND_GATE);
                    DoUseDoorOrButton(GO_MURU_ENTER_GATE);
#endif
#if defined (WOTLK) || defined (CATA) || defined(MISTS)
                if (Player* pPlayer = GetPlayerInMap())
                {
                    pPlayer->SummonCreature(NPC_MURU, afMuruSpawnLoc[0], afMuruSpawnLoc[1], afMuruSpawnLoc[2], afMuruSpawnLoc[3], TEMPSPAWN_DEAD_DESPAWN, 0, true);
                }
#endif
                }
                break;
            case TYPE_MURU:
                m_auiEncounter[uiType] = uiData;
                // combat door
                DoUseDoorOrButton(GO_MURU_ENTER_GATE);
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_MURU_EXIT_GATE);
#if defined (TBC)
                    DoUseDoorOrButton(GO_THIRD_GATE);
#endif
                }
                else if (uiData == IN_PROGRESS)
                {
                    m_uiMuruBerserkTimer = 10 * MINUTE * IN_MILLISECONDS;
                }
                break;
            case TYPE_KILJAEDEN:
                m_auiEncounter[uiType] = uiData;
                if (uiData == FAIL)
                {
                    m_uiDeceiversKilled = 0;

                    // Reset Orbs
                    DoToggleGameObjectFlags(GO_ORB_BLUE_FLIGHT_1, GO_FLAG_NO_INTERACT, true);
                    DoToggleGameObjectFlags(GO_ORB_BLUE_FLIGHT_2, GO_FLAG_NO_INTERACT, true);
                    DoToggleGameObjectFlags(GO_ORB_BLUE_FLIGHT_3, GO_FLAG_NO_INTERACT, true);
                    DoToggleGameObjectFlags(GO_ORB_BLUE_FLIGHT_4, GO_FLAG_NO_INTERACT, true);

                    // Respawn deceivers
                    for (GuidList::const_iterator itr = m_lDeceiversGuidList.begin(); itr != m_lDeceiversGuidList.end(); ++itr)
                    {
                        if (Creature* pDeceiver = instance->GetCreature(*itr))
                        {
                            if (!pDeceiver->IsAlive())
                            {
                                pDeceiver->Respawn();
                            }
                        }
                    }
                }
                break;
            case TYPE_FELMYST_TRIGGER:
                m_uiFelmystCorruption = uiData;
                return;
            case TYPE_KALECGOS_EJECT_SPECTRAL:
                DoEjectSpectralPlayers();
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
            {
                return m_auiEncounter[uiType];
            }

            return 0;
        }

        void SetData64(uint32 type, uint64 data) override
        {
            switch (type)
            {
            case TYPE_KALECGOS:
                m_spectralRealmPlayers.insert(ObjectGuid(data));
                break;
            case TYPE_KALECGOS_EJECT_SPECTRAL:
                m_spectralRealmPlayers.erase(ObjectGuid(data));
                break;
            default:
                break;
            }
        }

        uint64 GetData64(uint32 type) const override
        {
            switch (type)
            {
            case TYPE_FELMYST_TRIGGER_RIGHT:
                return SelectFelmystFlightTrigger(false).GetRawValue();
            case TYPE_FELMYST_TRIGGER_LEFT:
                return SelectFelmystFlightTrigger(true).GetRawValue();
            default:
                break;
            }
            return 0;
        }

        void Update(uint32 uiDiff) override
        {
            DialogueUpdate(uiDiff);

            if (m_uiKalecRespawnTimer)
            {
                if (m_uiKalecRespawnTimer <= uiDiff)
                {
                    if (Creature* pKalecDragon = GetSingleCreatureFromStorage(NPC_KALECGOS_DRAGON))
                    {
                        pKalecDragon->Respawn();
                    }
                    if (Creature* pKalecHuman = GetSingleCreatureFromStorage(NPC_KALECGOS_HUMAN))
                    {
                        pKalecHuman->Respawn();
                    }
                    m_uiKalecRespawnTimer = 0;
                }
                else
                {
                    m_uiKalecRespawnTimer -= uiDiff;
                }
            }

            // Muru berserk timer; needs to be done here because it involves two distinct creatures
            if (m_auiEncounter[TYPE_MURU] == IN_PROGRESS)
            {
                if (m_uiMuruBerserkTimer < uiDiff)
                {
                    if (Creature* pEntrpius = GetSingleCreatureFromStorage(NPC_ENTROPIUS, true))
                    {
                        pEntrpius->CastSpell(pEntrpius, SPELL_MURU_BERSERK, true);
                    }
                    else if (Creature* pMuru = GetSingleCreatureFromStorage(NPC_MURU))
                    {
                        pMuru->CastSpell(pMuru, SPELL_MURU_BERSERK, true);
                    }

                    m_uiMuruBerserkTimer = 10 * MINUTE * IN_MILLISECONDS;
                }
                else
                {
                    m_uiMuruBerserkTimer -= uiDiff;
                }
            }

            if (m_auiEncounter[TYPE_KILJAEDEN] == NOT_STARTED || m_auiEncounter[TYPE_KILJAEDEN] == FAIL)
            {
                if (m_uiKiljaedenYellTimer < uiDiff)
                {
                    switch (urand(0, 4))
                    {
                    case 0: DoOrSimulateScriptTextForThisInstance(SAY_ORDER_1, NPC_KILJAEDEN_CONTROLLER); break;
                    case 1: DoOrSimulateScriptTextForThisInstance(SAY_ORDER_2, NPC_KILJAEDEN_CONTROLLER); break;
                    case 2: DoOrSimulateScriptTextForThisInstance(SAY_ORDER_3, NPC_KILJAEDEN_CONTROLLER); break;
                    case 3: DoOrSimulateScriptTextForThisInstance(SAY_ORDER_4, NPC_KILJAEDEN_CONTROLLER); break;
                    case 4: DoOrSimulateScriptTextForThisInstance(SAY_ORDER_5, NPC_KILJAEDEN_CONTROLLER); break;
                    }
                    m_uiKiljaedenYellTimer = 90000;
                }
                else
                {
                    m_uiKiljaedenYellTimer -= uiDiff;
                }
            }
        }

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* in) override
        {
            if (!in)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(in);

            std::istringstream loadStream(in);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >>
                m_auiEncounter[3] >> m_auiEncounter[4] >> m_auiEncounter[5];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void JustDidDialogueStep(int32 iEntry) override
        {
            switch (iEntry)
            {
            case NPC_KALECGOS_MADRIGOSA:
                if (Creature* pTrigger = GetSingleCreatureFromStorage(NPC_FLIGHT_TRIGGER_LEFT))
                {
                    if (Creature* pKalec = pTrigger->SummonCreature(NPC_KALECGOS_MADRIGOSA, aKalecLoc[0].m_fX, aKalecLoc[0].m_fY, aKalecLoc[0].m_fZ, aKalecLoc[0].m_fO, TEMPSPAWN_CORPSE_DESPAWN, 0))
                    {
                        pKalec->SetWalk(false);
                        pKalec->SetLevitate(true);
                        pKalec->GetMotionMaster()->MovePoint(0, aKalecLoc[1].m_fX, aKalecLoc[1].m_fY, aKalecLoc[1].m_fZ, false);
                    }
                }
                break;
            case NPC_FELMYST:
                if (Creature* pKalec = GetSingleCreatureFromStorage(NPC_KALECGOS_MADRIGOSA))
                {
                    pKalec->GetMotionMaster()->MovePoint(0, aKalecLoc[2].m_fX, aKalecLoc[2].m_fY, aKalecLoc[2].m_fZ, false);
                }
                break;
            case SPELL_OPEN_BACK_DOOR:
                if (Creature* pKalec = GetSingleCreatureFromStorage(NPC_KALECGOS_MADRIGOSA))
                {
                    // ToDo: update this when the AoE spell targeting will support many explicit target. Kalec should target all creatures from the list
                    if (Creature* pTrigger = instance->GetCreature(m_lBackdoorTriggersList.front()))
                    {
                        pKalec->CastSpell(pTrigger, SPELL_OPEN_BACK_DOOR, true);
                    }
                }
                break;
            case NPC_BRUTALLUS:
                if (Creature* pKalec = GetSingleCreatureFromStorage(NPC_KALECGOS_MADRIGOSA))
                {
                    pKalec->ForcedDespawn(10000);
                    pKalec->GetMotionMaster()->MovePoint(0, aKalecLoc[3].m_fX, aKalecLoc[3].m_fY, aKalecLoc[3].m_fZ, false);
                }
                break;
            }
        }

    private:
        void DoSortFlightTriggers()
        {
            if (m_lAllFlightTriggersList.empty())
            {
                script_error_log("Instance Sunwell Plateau: ERROR Failed to load flight triggers for creature id %u.", NPC_FELMYST);
                return;
            }

            std::list<Creature*> lTriggers;                         // Valid pointers, only used locally
            for (GuidList::const_iterator itr = m_lAllFlightTriggersList.begin(); itr != m_lAllFlightTriggersList.end(); ++itr)
            {
                if (Creature* pTrigger = instance->GetCreature(*itr))
                {
                    lTriggers.push_back(pTrigger);
                }
            }

            if (lTriggers.empty())
            {
                return;
            }

            // sort the flight triggers; first by position X, then group them by Y (left and right)
            lTriggers.sort(sortByPositionX);
            for (std::list<Creature*>::iterator itr = lTriggers.begin(); itr != lTriggers.end(); ++itr)
            {
                if ((*itr)->GetPositionY() < 600.0f)
                {
                    m_vRightFlightTriggersVect.push_back((*itr)->GetObjectGuid());
                }
                else
                {
                    m_vLeftFlightTriggersVect.push_back((*itr)->GetObjectGuid());
                }
            }
        }

        static bool sortByPositionX(Creature* pFirst, Creature* pSecond)
        {
            return pFirst && pSecond && pFirst->GetPositionX() > pSecond->GetPositionX();
        }

        ObjectGuid SelectFelmystFlightTrigger(bool bLeftSide) const
        {
            // Return the flight trigger from the selected index
            GuidVector vTemp = bLeftSide ? m_vLeftFlightTriggersVect : m_vRightFlightTriggersVect;

            if (m_uiFelmystCorruption >= vTemp.size())
            {
                return ObjectGuid();
            }

            return vTemp[m_uiFelmystCorruption];
        }

        void DoEjectSpectralPlayers()
        {
            for (GuidSet::const_iterator itr = m_spectralRealmPlayers.begin(); itr != m_spectralRealmPlayers.end(); ++itr)
            {
                if (Player* pPlayer = instance->GetPlayer(*itr))
                {
                    if (!pPlayer->HasAura(SPELL_SPECTRAL_REALM_AURA))
                    {
                        continue;
                    }

                    pPlayer->CastSpell(pPlayer, SPELL_TELEPORT_NORMAL_REALM, true);
                    pPlayer->CastSpell(pPlayer, SPELL_SPECTRAL_EXHAUSTION, true);
                    pPlayer->RemoveAurasDueToSpell(SPELL_SPECTRAL_REALM_AURA);
                }
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        // Misc
        uint8 m_uiDeceiversKilled;
        uint32 m_uiSpectralRealmTimer;
        uint32 m_uiKalecRespawnTimer;
        uint32 m_uiMuruBerserkTimer;
        uint32 m_uiKiljaedenYellTimer;
        uint32 m_uiFelmystCorruption;

        GuidSet m_spectralRealmPlayers;
        GuidVector m_vRightFlightTriggersVect;
        GuidVector m_vLeftFlightTriggersVect;
        GuidList m_lAllFlightTriggersList;
        GuidList m_lBackdoorTriggersList;
        GuidList m_lDeceiversGuidList;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_sunwell_plateau(pMap);
    }
};

struct at_sunwell_plateau : public AreaTriggerScript
{
    at_sunwell_plateau() : AreaTriggerScript("at_sunwell_plateau") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* pAt) override
    {
        if (pAt->id == AREATRIGGER_TWINS)
        {
            if (pPlayer->isGameMaster() || pPlayer->IsDead())
            {
                return false;
            }

            InstanceData* pInstance = pPlayer->GetInstanceData();

            if (pInstance && pInstance->GetData(TYPE_EREDAR_TWINS) == NOT_STARTED)
            {
                pInstance->SetData(TYPE_EREDAR_TWINS, SPECIAL);
            }
        }

        return false;
    }
};

void AddSC_instance_sunwell_plateau()
{
    Script* s;

    s = new is_sunwell_plateau();
    s->RegisterSelf();
    s = new at_sunwell_plateau();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_sunwell_plateau";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_sunwell_plateau;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "at_sunwell_plateau";
    //pNewScript->pAreaTrigger = &AreaTrigger_at_sunwell_plateau;
    //pNewScript->RegisterSelf();
}
