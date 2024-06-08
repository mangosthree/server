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
 * SDName:      instance_dire_maul
 * SD%Complete: 30
 * SDComment:   Basic Support - Most events and quest-related stuff missing
 * SDCategory:  Dire Maul
 * EndScriptData
 */

#include "precompiled.h"
#include "dire_maul.h"
#include "GameObjectAI.h"

struct is_dire_maul : public InstanceScript
{
    is_dire_maul() : InstanceScript("instance_dire_maul") {}

    class instance_dire_maul : public ScriptedInstance
    {
    public:
        instance_dire_maul(Map* pMap) : ScriptedInstance(pMap),
            m_bWallDestroyed(false),
            m_bDoNorthBeforeWest(false)
        {
            Initialize();
        }

        ~instance_dire_maul() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            // figure where to enter to set library doors accordingly
            // Enter DM North first
            if (pPlayer->IsWithinDist2d(260.0f, -20.0f, 20.0f) && m_auiEncounter[TYPE_WARPWOOD] != DONE)
            {
                m_bDoNorthBeforeWest = true;
            }
            else
            {
                m_bDoNorthBeforeWest = false;
            }

            DoToggleGameObjectFlags(GO_WEST_LIBRARY_DOOR, GO_FLAG_NO_INTERACT, m_bDoNorthBeforeWest);
            DoToggleGameObjectFlags(GO_WEST_LIBRARY_DOOR, GO_FLAG_LOCKED, !m_bDoNorthBeforeWest);
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
                // East
            case NPC_OLD_IRONBARK:
                break;

                // West
            case NPC_PRINCE_TORTHELDRIN:
                if (m_auiEncounter[TYPE_IMMOLTHAR] == DONE)
                {
                    pCreature->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_RESPAWN | TEMPFACTION_TOGGLE_OOC_NOT_ATTACK);
                }
                break;
            case NPC_ARCANE_ABERRATION:
            case NPC_MANA_REMNANT:
                m_lGeneratorGuardGUIDs.push_back(pCreature->GetObjectGuid());
                return;
            case NPC_IMMOLTHAR:
                break;
            case NPC_HIGHBORNE_SUMMONER:
                m_luiHighborneSummonerGUIDs.push_back(pCreature->GetObjectGuid());
                return;

                // North
            case NPC_CAPTAIN_KROMCRUSH:
            {
                // Reset Npc flag to None
                pCreature->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
                break;
            }
            case NPC_CHORUSH:
            case NPC_KING_GORDOK:
            case NPC_MIZZLE_THE_CRAFTY:
            case NPC_GUARD_SLIPKIK:
            case NPC_GUARD_MOLDAR:
            case NPC_GUARD_FENGUS:
                break;

            default:
                return;
            }
            m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
                // East
            case GO_CONSERVATORY_DOOR:
                if (m_auiEncounter[TYPE_IRONBARK] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_CRUMBLE_WALL:
                if (m_bWallDestroyed || m_auiEncounter[TYPE_ALZZIN] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_CORRUPT_VINE:
                if (m_auiEncounter[TYPE_ALZZIN] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_FELVINE_SHARD:
                m_lFelvineShardGUIDs.push_back(pGo->GetObjectGuid());
                break;

                // West
            case GO_CRYSTAL_GENERATOR_1:
                m_aCrystalGeneratorGuid[0] = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_PYLON_1] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                return;
            case GO_CRYSTAL_GENERATOR_2:
                m_aCrystalGeneratorGuid[1] = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_PYLON_2] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                return;
            case GO_CRYSTAL_GENERATOR_3:
                m_aCrystalGeneratorGuid[2] = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_PYLON_3] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                return;
            case GO_CRYSTAL_GENERATOR_4:
                m_aCrystalGeneratorGuid[3] = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_PYLON_4] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                return;
            case GO_CRYSTAL_GENERATOR_5:
                m_aCrystalGeneratorGuid[4] = pGo->GetObjectGuid();
                if (m_auiEncounter[TYPE_PYLON_5] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                return;
            case GO_FORCEFIELD:
                if (CheckAllGeneratorsDestroyed())
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_PRINCES_CHEST_AURA:
                break;
            case GO_WARPWOOD_DOOR:
                if (m_auiEncounter[TYPE_WARPWOOD] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_WEST_LIBRARY_DOOR:
                pGo->SetFlag(GAMEOBJECT_FLAGS, m_bDoNorthBeforeWest ? GO_FLAG_NO_INTERACT : GO_FLAG_LOCKED);
                pGo->RemoveFlag(GAMEOBJECT_FLAGS, m_bDoNorthBeforeWest ? GO_FLAG_LOCKED : GO_FLAG_NO_INTERACT);
                break;

                // North
            case GO_NORTH_LIBRARY_DOOR:
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
                // East
            case TYPE_ZEVRIM:
                if (uiData == DONE)
                {
                    // Update Old Ironbark so he can open the conservatory door
                    if (Creature* pIronbark = GetSingleCreatureFromStorage(NPC_OLD_IRONBARK))
                    {
                        DoScriptText(SAY_IRONBARK_REDEEM, pIronbark);
                        pIronbark->UpdateEntry(NPC_IRONBARK_REDEEMED);
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_IRONBARK:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_ALZZIN:                                   // This Encounter is expected to be handled within Acid (reason handling at 50% hp)
                if (uiData == DONE)
                {
                    if (!m_bWallDestroyed)
                    {
                        DoUseDoorOrButton(GO_CRUMBLE_WALL);
                        m_bWallDestroyed = true;
                    }

                    DoUseDoorOrButton(GO_CORRUPT_VINE);

                    if (!m_lFelvineShardGUIDs.empty())
                    {
                        for (GuidList::const_iterator itr = m_lFelvineShardGUIDs.begin(); itr != m_lFelvineShardGUIDs.end(); ++itr)
                        {
                            DoRespawnGameObject(*itr);
                        }
                    }
                }
                else if (uiData == SPECIAL && !m_bWallDestroyed)
                {
                    DoUseDoorOrButton(GO_CRUMBLE_WALL);
                    m_bWallDestroyed = true;
                }
                m_auiEncounter[uiType] = uiData;
                break;

                // West
            case TYPE_WARPWOOD:
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(GO_WARPWOOD_DOOR);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_IMMOLTHAR:
                if (uiData == DONE)
                {
                    if (Creature* pPrince = GetSingleCreatureFromStorage(NPC_PRINCE_TORTHELDRIN))
                    {
                        DoScriptText(SAY_FREE_IMMOLTHAR, pPrince);
                        pPrince->SetFactionTemporary(FACTION_HOSTILE, TEMPFACTION_RESTORE_RESPAWN | TEMPFACTION_TOGGLE_OOC_NOT_ATTACK);
                        // Despawn Chest-Aura
                        if (GameObject* pChestAura = GetSingleGameObjectFromStorage(GO_PRINCES_CHEST_AURA))
                        {
                            pChestAura->Use(pPrince);
                        }
                    }
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_PRINCE:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_PYLON_1:
            case TYPE_PYLON_2:
            case TYPE_PYLON_3:
            case TYPE_PYLON_4:
            case TYPE_PYLON_5:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    DoUseDoorOrButton(m_aCrystalGeneratorGuid[uiType - TYPE_PYLON_1]);
                    if (CheckAllGeneratorsDestroyed())
                    {
                        ProcessForceFieldOpening();
                    }
                }
                break;

                // North
            case TYPE_KING_GORDOK:
                m_auiEncounter[uiType] = uiData;
                if (uiData == DONE)
                {
                    if (Creature* pOgre = GetSingleCreatureFromStorage(NPC_CHORUSH))
                    {
                        // Chorush evades and yells on king death (if alive)
                        if (pOgre->IsAlive())
                        {
                            DoScriptText(SAY_CHORUSH_KING_DEAD, pOgre);
                            pOgre->SetFactionTemporary(FACTION_FRIENDLY, TEMPFACTION_RESTORE_RESPAWN);
                            pOgre->AI()->EnterEvadeMode();
                        }
                        else
                        {
                            // Well if you kill chorush, no tribute ! He is the one who summons Mizzle the Crafty
                            break;
                        }

                        // start WP movement for Mizzle; event handled by movement and gossip dbscripts
                        if (Creature* pMizzle = pOgre->SummonCreature(NPC_MIZZLE_THE_CRAFTY, afMizzleSpawnLoc[0], afMizzleSpawnLoc[1], afMizzleSpawnLoc[2], afMizzleSpawnLoc[3], TEMPSPAWN_DEAD_DESPAWN, 0, true))
                        {
                            pMizzle->SetWalk(false);
                            pMizzle->GetMotionMaster()->MoveWaypoint();
                        }

                        // change faction to certain ogres
                        std::vector<uint32> specificOgresThatMustBecomeFriendly;
                        specificOgresThatMustBecomeFriendly.push_back(NPC_GUARD_MOLDAR);
                        specificOgresThatMustBecomeFriendly.push_back(NPC_GUARD_SLIPKIK);
                        specificOgresThatMustBecomeFriendly.push_back(NPC_CAPTAIN_KROMCRUSH);
                        specificOgresThatMustBecomeFriendly.push_back(NPC_GUARD_FENGUS);

                        for (std::vector<uint32>::iterator ogreItr = specificOgresThatMustBecomeFriendly.begin(); ogreItr != specificOgresThatMustBecomeFriendly.end(); ++ogreItr)
                        {
                            if (Creature* pOgre = GetSingleCreatureFromStorage(*ogreItr))
                            {
                                if (pOgre->IsAlive())
                                {
                                    pOgre->SetFactionTemporary(FACTION_FRIENDLY, TEMPFACTION_RESTORE_RESPAWN);

                                    bool enterEvadeMode = true;

                                    switch (*ogreItr)
                                    {
                                        case NPC_GUARD_SLIPKIK:
                                        {
                                            // Risk of interrupting "Ice Block" spell if is affected by trap
                                            if (pOgre->HasAura(SPELL_ICE_BLOCK))
                                            {
                                                enterEvadeMode = false;
                                            }
                                        }
                                        case NPC_CAPTAIN_KROMCRUSH:
                                        {
                                            // Must reactive NPC flags for komcrush
                                            pOgre->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                                            break;
                                        }
                                        default:
                                        {
                                            break;
                                        }
                                    }

                                    // only evade if required
                                    if (enterEvadeMode && pOgre->getVictim())
                                    {
                                        pOgre->AI()->EnterEvadeMode();
                                    }
                                }
                            }
                        }

                    }

                }
                break;
            case TYPE_MOLDAR:
            case TYPE_FENGUS:
            case TYPE_SLIPKIK:
            case TYPE_KROMCRUSH:
                m_auiEncounter[uiType] = uiData;
                break;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " "
                    << m_auiEncounter[3] << " " << m_auiEncounter[4] << " " << m_auiEncounter[5] << " "
                    << m_auiEncounter[6] << " " << m_auiEncounter[7] << " " << m_auiEncounter[8] << " "
                    << m_auiEncounter[9] << " " << m_auiEncounter[10] << " " << m_auiEncounter[11] << " "
                    << m_auiEncounter[12] << " " << m_auiEncounter[13] << " " << m_auiEncounter[14] << " "
                    << m_auiEncounter[15];

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

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
                // West
                // - Handling of guards of generators
            case NPC_ARCANE_ABERRATION:
            case NPC_MANA_REMNANT:
                SortPylonGuards();
                break;
                // - Set InstData for ImmolThar
            case NPC_IMMOLTHAR:
                SetData(TYPE_IMMOLTHAR, IN_PROGRESS);
                break;
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
                // East
                // - Handling Zevrim and Old Ironbark for the door event
            case NPC_ZEVRIM_THORNHOOF:
                SetData(TYPE_ZEVRIM, DONE);
                break;
            case NPC_IRONBARK_REDEEMED:
                SetData(TYPE_IRONBARK, DONE);
                break;

                // West
                // - Handling of guards of generators
            case NPC_ARCANE_ABERRATION:
            case NPC_MANA_REMNANT:
                PylonGuardJustDied(pCreature);
                break;
                // - InstData settings
            case NPC_TENDRIS_WARPWOOD:
                SetData(TYPE_WARPWOOD, DONE);
                break;
            case NPC_IMMOLTHAR:
                SetData(TYPE_IMMOLTHAR, DONE);
                break;

                // North
                // - Handling of Ogre Boss (Assume boss can be handled in Acid)
            case NPC_KING_GORDOK:
                SetData(TYPE_KING_GORDOK, DONE);
                break;
                // Handle Ogre guards for Tribute Run chest
            case NPC_GUARD_MOLDAR:
                SetData(TYPE_MOLDAR, DONE);
                break;
            case NPC_GUARD_FENGUS:
                SetData(TYPE_FENGUS, DONE);
                break;
            case NPC_GUARD_SLIPKIK:
                SetData(TYPE_SLIPKIK, DONE);
                break;
            case NPC_CAPTAIN_KROMCRUSH:
                SetData(TYPE_KROMCRUSH, DONE);
                break;
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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >>
                m_auiEncounter[3] >> m_auiEncounter[4] >> m_auiEncounter[5] >>
                m_auiEncounter[6] >> m_auiEncounter[7] >> m_auiEncounter[8] >>
                m_auiEncounter[9] >> m_auiEncounter[10] >> m_auiEncounter[11] >>
                m_auiEncounter[12] >> m_auiEncounter[13] >> m_auiEncounter[14] >>
                m_auiEncounter[15];

            if (m_auiEncounter[TYPE_ALZZIN] >= DONE)
            {
                m_bWallDestroyed = true;
            }

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        bool CheckConditionCriteriaMeet(Player const* pPlayer, uint32 uiInstanceConditionId, WorldObject const* pConditionSource, uint32 conditionSourceType) const override
        {
            switch (uiInstanceConditionId)
            {
                case INSTANCE_CONDITION_ID_NORMAL_MODE:             // No guards alive
                case INSTANCE_CONDITION_ID_HARD_MODE:               // One guard alive
                case INSTANCE_CONDITION_ID_HARD_MODE_2:             // Two guards alive
                case INSTANCE_CONDITION_ID_HARD_MODE_3:             // Three guards alive
                case INSTANCE_CONDITION_ID_HARD_MODE_4:             // All guards alive
                {
                    uint8 uiTributeRunAliveBosses = (GetData(TYPE_MOLDAR) != DONE ? 1 : 0) + (GetData(TYPE_FENGUS) != DONE ? 1 : 0) + (GetData(TYPE_SLIPKIK) != DONE ? 1 : 0)
                        + (GetData(TYPE_KROMCRUSH) != DONE ? 1 : 0);

                    return uiInstanceConditionId == uiTributeRunAliveBosses;
                }
            }

            script_error_log("instance_dire_maul::CheckConditionCriteriaMeet called with unsupported Id %u. Called with param plr %s, src %s, condition source type %u",
                uiInstanceConditionId, pPlayer ? pPlayer->GetGuidStr().c_str() : "nullptr", pConditionSource ? pConditionSource->GetGuidStr().c_str() : "nullptr", conditionSourceType);
            return false;
        }

    private:
        bool CheckAllGeneratorsDestroyed()
        {
            if (m_auiEncounter[TYPE_PYLON_1] != DONE || m_auiEncounter[TYPE_PYLON_2] != DONE || m_auiEncounter[TYPE_PYLON_3] != DONE || m_auiEncounter[TYPE_PYLON_4] != DONE || m_auiEncounter[TYPE_PYLON_5] != DONE)
            {
                return false;
            }

            return true;
        }

        void ProcessForceFieldOpening()
        {
            // 'Open' the force field
            DoUseDoorOrButton(GO_FORCEFIELD);

            // Let the summoners attack Immol'Thar
            Creature* pImmolThar = GetSingleCreatureFromStorage(NPC_IMMOLTHAR);
            if (!pImmolThar || pImmolThar->IsDead())
            {
                return;
            }

            bool bHasYelled = false;
            for (GuidList::const_iterator itr = m_luiHighborneSummonerGUIDs.begin(); itr != m_luiHighborneSummonerGUIDs.end(); ++itr)
            {
                Creature* pSummoner = instance->GetCreature(*itr);

                if (!bHasYelled && pSummoner)
                {
                    DoScriptText(SAY_KILL_IMMOLTHAR, pSummoner);
                    bHasYelled = true;
                }

                if (!pSummoner || pSummoner->IsDead())
                {
                    continue;
                }

                pSummoner->AI()->AttackStart(pImmolThar);
            }
            m_luiHighborneSummonerGUIDs.clear();
        }

        void SortPylonGuards()
        {
            if (!m_lGeneratorGuardGUIDs.empty())
            {
                for (uint8 i = 0; i < MAX_GENERATORS; ++i)
                {
                    GameObject* pGenerator = instance->GetGameObject(m_aCrystalGeneratorGuid[i]);
                    // Skip non-existing or finished generators
                    if (!pGenerator || GetData(TYPE_PYLON_1 + i) == DONE)
                    {
                        continue;
                    }

                    // Sort all remaining (alive) NPCs to unfinished generators
                    for (GuidList::iterator itr = m_lGeneratorGuardGUIDs.begin(); itr != m_lGeneratorGuardGUIDs.end();)
                    {
                        Creature* pGuard = instance->GetCreature(*itr);
                        if (!pGuard || pGuard->IsDead())    // Remove invalid guids and dead guards
                        {
                            m_lGeneratorGuardGUIDs.erase(itr++);
                            continue;
                        }

                        if (pGuard->IsWithinDist2d(pGenerator->GetPositionX(), pGenerator->GetPositionY(), 20.0f))
                        {
                            m_sSortedGeneratorGuards[i].insert(pGuard->GetGUIDLow());
                            m_lGeneratorGuardGUIDs.erase(itr++);
                        }
                        else
                        {
                            ++itr;
                        }
                    }
                }
            }
        }

        void PylonGuardJustDied(Creature* pCreature)
        {
            for (uint8 i = 0; i < MAX_GENERATORS; ++i)
            {
                // Skip already activated generators
                if (GetData(TYPE_PYLON_1 + i) == DONE)
                {
                    continue;
                }

                // Only process generator where the npc is sorted in
                if (m_sSortedGeneratorGuards[i].find(pCreature->GetGUIDLow()) != m_sSortedGeneratorGuards[i].end())
                {
                    m_sSortedGeneratorGuards[i].erase(pCreature->GetGUIDLow());
                    if (m_sSortedGeneratorGuards[i].empty())
                    {
                        SetData(TYPE_PYLON_1 + i, DONE);
                    }

                    break;
                }
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        // East
        bool m_bWallDestroyed;
        GuidList m_lFelvineShardGUIDs;

        // West
        ObjectGuid m_aCrystalGeneratorGuid[MAX_GENERATORS];

        GuidList m_luiHighborneSummonerGUIDs;
        GuidList m_lGeneratorGuardGUIDs;
        std::set<uint32> m_sSortedGeneratorGuards[MAX_GENERATORS];

        // North
        bool m_bDoNorthBeforeWest;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_dire_maul(pMap);
    }
};

struct go_fixed_trap : public GameObjectScript
{
    go_fixed_trap() : GameObjectScript("go_fixed_trap")
    {
        sLog.outString("go_fixed_trap:: Constructor Called");
    }

    bool OnUse(Unit* p, GameObject* pGo) override
    {

        Creature* slipkik = (Creature*)p;
        slipkik->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        DoScriptText(SAY_SLIPKIK_TRAP, slipkik);
        if (pGo->GetGOInfo()->trap.charges > 0)
        {
            pGo->AddUse();
        }

        pGo->SetLootState(GO_JUST_DEACTIVATED);    // Despawn the trap
        sLog.outString("go_fixed_trap:: Used by %s", p->GetName());

        return true;
    }

};


void AddSC_instance_dire_maul()
{
    Script* s;
    s = new is_dire_maul();
    s->RegisterSelf();

    s = new go_fixed_trap();
    s->RegisterSelf();
}
