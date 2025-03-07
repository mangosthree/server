/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013 ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2025 MaNGOS <https://www.getmangos.eu>
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
SDName: Instance_Halls_of_Stone
SD%Complete: 50%
SDComment:
SDCategory: Halls of Stone
EndScriptData */

#include "precompiled.h"
#include "halls_of_stone.h"

enum
{
    // KADDRAK
    SPELL_GLARE_OF_THE_TRIBUNAL         = 50988,
    SPELL_GLARE_OF_THE_TRIBUNAL_H       = 59870,

    // MARNAK
    // Spells are handled in individual script

    // ABEDNEUM
    SPELL_SUMMON_SEARING_GAZE_TARGET    = 51146,                // The other spells are handled in individual script

    SPELL_KILL_TRIBUNAL_ADD             = 51288,                // Cleanup event on finish
    SPELL_ACHIEVEMENT_CHECK             = 59046,                // Doesn't exist in client dbc - added in spell_template

    SPELL_SUMMON_PROTECTOR              = 51780,                // all spells are casted by stalker npcs 28130
    SPELL_SUMMON_STORMCALLER            = 51050,
    SPELL_SUMMON_CUSTODIAN              = 51051,
};

struct Face
{
    Face() : m_bIsActive(false), m_uiTimer(1000) {}

    ObjectGuid m_leftEyeGuid;
    ObjectGuid m_rightEyeGuid;
    ObjectGuid m_goFaceGuid;
    ObjectGuid m_speakerGuid;
    bool m_bIsActive;
    uint32 m_uiTimer;
};

// Small Helper-function
static void GetValidNPCsOfList(Map* pMap, GuidList& lGUIDs, std::list<Creature*>& lNPCs)
{
    lNPCs.clear();
    for (GuidList::const_iterator itr = lGUIDs.begin(); itr != lGUIDs.end(); ++itr)
    {
        if (Creature* pMob = pMap->GetCreature(*itr))
        {
            lNPCs.push_back(pMob);
        }
    }
}

struct is_halls_of_stone : public InstanceScript
{
    is_halls_of_stone() : InstanceScript("instance_halls_of_stone") {}

    class  instance_halls_of_stone : public ScriptedInstance
    {
    public:
        instance_halls_of_stone(Map* pMap) : ScriptedInstance(pMap),
            m_uiIronSludgeKilled(0),
            m_bIsBrannSpankin(false)
        {
            Initialize();
        }

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_KADDRAK:          m_lKaddrakGUIDs.push_back(pCreature->GetObjectGuid());      break;
            case NPC_ABEDNEUM:         m_lAbedneumGUIDs.push_back(pCreature->GetObjectGuid());     break;
            case NPC_MARNAK:           m_lMarnakGUIDs.push_back(pCreature->GetObjectGuid());       break;
            case NPC_TRIBUNAL_OF_AGES: m_lTribunalGUIDs.push_back(pCreature->GetObjectGuid());     break;
            case NPC_WORLDTRIGGER:     m_lWorldtriggerGUIDs.push_back(pCreature->GetObjectGuid()); break;
            case NPC_LIGHTNING_STALKER:
                // Sort the dwarf summoning stalkers
                if (pCreature->GetPositionY() > 400.0f)
                {
                    m_protectorStalkerGuid = pCreature->GetObjectGuid();
                }
                else if (pCreature->GetPositionY() > 380.0f)
                {
                    m_stormcallerStalkerGuid = pCreature->GetObjectGuid();
                }
                else
                {
                    m_custodianStalkerGuid = pCreature->GetObjectGuid();
                }
                break;
            case NPC_DARK_MATTER:
            case NPC_SJONNIR:
            case NPC_BRANN_HOS:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_TRIBUNAL_CHEST:
            case GO_TRIBUNAL_CHEST_H:
                if (m_auiEncounter[TYPE_TRIBUNAL] == DONE)
                {
                    pGo->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                }
                break;
            case GO_TRIBUNAL_HEAD_RIGHT:
                m_aFaces[FACE_MARNAK].m_goFaceGuid = pGo->GetObjectGuid();
                return;
            case GO_TRIBUNAL_HEAD_CENTER:
                m_aFaces[FACE_ABEDNEUM].m_goFaceGuid = pGo->GetObjectGuid();
                return;
            case GO_TRIBUNAL_HEAD_LEFT:
                m_aFaces[FACE_KADDRAK].m_goFaceGuid = pGo->GetObjectGuid();
                return;
            case GO_DOOR_TO_TRIBUNAL:
            case GO_DOOR_MAIDEN:
            case GO_DOOR_SJONNIR:
            case GO_DOOR_TRIBUNAL:
            case GO_TRIBUNAL_CONSOLE:
            case GO_TRIBUNAL_FLOOR:
            case GO_SJONNIR_CONSOLE:
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_IRON_SLUDGE && GetData(TYPE_SJONNIR) == IN_PROGRESS)
            {
                ++m_uiIronSludgeKilled;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_TRIBUNAL:
                m_auiEncounter[uiType] = uiData;
                switch (uiData)
                {
                case IN_PROGRESS:
                    SortFaces();
                    break;
                case DONE:
                    // Cast achiev check spell - Note: it's not clear who casts this spell, but for the moment we'll use Abedneum
                    if (Creature* pEye = instance->GetCreature(m_aFaces[1].m_leftEyeGuid))
                    {
                        pEye->CastSpell(pEye, SPELL_ACHIEVEMENT_CHECK, true);
                    }
                    // Spawn the loot
                    DoRespawnGameObject(instance->IsRegularDifficulty() ? GO_TRIBUNAL_CHEST : GO_TRIBUNAL_CHEST_H, 30 * MINUTE);
                    DoToggleGameObjectFlags(instance->IsRegularDifficulty() ? GO_TRIBUNAL_CHEST : GO_TRIBUNAL_CHEST_H, GO_FLAG_NO_INTERACT, false);
                    // Door workaround because of the missing Bran event
                    DoUseDoorOrButton(GO_DOOR_SJONNIR);
                    break;
                case FAIL:
                    for (uint8 i = 0; i < MAX_FACES; ++i)
                    {
                        // Shut down the faces
                        if (m_aFaces[i].m_bIsActive)
                        {
                            DoUseDoorOrButton(m_aFaces[i].m_goFaceGuid);
                        }
                        m_aFaces[i].m_bIsActive = false;
                        m_aFaces[i].m_uiTimer = 1000;
                    }
                    break;
                case SPECIAL:
                    for (uint8 i = 0; i < MAX_FACES; ++i)
                    {
                        m_aFaces[i].m_bIsActive = false;
                        m_aFaces[i].m_uiTimer = 1000;
                        // TODO - Check which stay red and how long (also find out how they get red..)

                        // Cleanup when finished
                        if (Creature* pEye = instance->GetCreature(m_aFaces[i].m_leftEyeGuid))
                        {
                            pEye->CastSpell(pEye, SPELL_KILL_TRIBUNAL_ADD, true);
                        }
                        if (Creature* pEye = instance->GetCreature(m_aFaces[i].m_rightEyeGuid))
                        {
                            pEye->CastSpell(pEye, SPELL_KILL_TRIBUNAL_ADD, true);
                        }
                    }
                    break;
                }
                break;
            case TYPE_MAIDEN:
                m_auiEncounter[uiType] = uiData;
                if (uiData == IN_PROGRESS)
                {
                    DoStartTimedAchievement(ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE, ACHIEV_START_MAIDEN_ID);
                }
                break;
            case TYPE_KRYSTALLUS:
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_SJONNIR:
                m_auiEncounter[uiType] = uiData;
                DoUseDoorOrButton(GO_DOOR_SJONNIR);
                if (uiData == IN_PROGRESS)
                {
                    m_uiIronSludgeKilled = 0;
                }
                break;
            case TYPE_ACHIEV_BRANN_SPANKIN:
                m_bIsBrannSpankin = bool(uiData);
                return;
            case TYPE_DO_SPAWN_DWARF:
                // each case has an individual spawn stalker
                switch (uiData)
                {
                case NPC_DARK_RUNE_PROTECTOR:
                    if (Creature* pStalker = instance->GetCreature(m_protectorStalkerGuid))
                    {
                        uint32 uiSpawnNumber = (instance->IsRegularDifficulty() ? 2 : 3);
                        for (uint8 i = 0; i < uiSpawnNumber; ++i)
                        {
                            pStalker->CastSpell(pStalker, SPELL_SUMMON_PROTECTOR, true, nullptr, nullptr, m_mNpcEntryGuidStore[NPC_BRANN_HOS]);
                        }
                        pStalker->CastSpell(pStalker, SPELL_SUMMON_STORMCALLER, true, nullptr, nullptr, m_mNpcEntryGuidStore[NPC_BRANN_HOS]);
                    }
                    break;
                case NPC_DARK_RUNE_STORMCALLER:
                    if (Creature* pStalker = instance->GetCreature(m_stormcallerStalkerGuid))
                    {
                        for (uint8 i = 0; i < 2; ++i)
                        {
                            pStalker->CastSpell(pStalker, SPELL_SUMMON_STORMCALLER, true, nullptr, nullptr, m_mNpcEntryGuidStore[NPC_BRANN_HOS]);
                        }
                    }
                    break;
                case NPC_IRON_GOLEM_CUSTODIAN:
                    if (Creature* pStalker = instance->GetCreature(m_custodianStalkerGuid))
                    {
                        pStalker->CastSpell(pStalker, SPELL_SUMMON_CUSTODIAN, true, nullptr, nullptr, m_mNpcEntryGuidStore[NPC_BRANN_HOS]);
                    }
                    break;
                }
                return;
            case TYPE_DO_ABEDNEUM_SPEAK:
            case TYPE_DO_KADDRAK_SPEAK:
            case TYPE_DO_MARNAK_SPEAK:
                DoFaceSpeak(uiType - TYPE_DO_MARNAK_SPEAK, -int32(uiData));
                return;
            case TYPE_DO_KADDRAK_ACTIVATE:
            case TYPE_DO_ABEDNEUM_ACTIVATE:
            case TYPE_DO_MARNAK_ACTIVATE:
                ActivateFace(uiType - TYPE_DO_MARNAK_ACTIVATE, bool(uiData));
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " " << m_auiEncounter[3];

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
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void Update(uint32 uiDiff) override
        {
            if (m_auiEncounter[TYPE_TRIBUNAL] == IN_PROGRESS)
            {
                for (uint8 i = 0; i < MAX_FACES; ++i)
                {
                    if (!m_aFaces[i].m_bIsActive)
                    {
                        continue;
                    }

                    if (m_aFaces[i].m_uiTimer < uiDiff)
                    {
                        ProcessFace(i);
                    }
                    else
                    {
                        m_aFaces[i].m_uiTimer -= uiDiff;
                    }
                }
            }
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_BRANN:
                return m_bIsBrannSpankin;
            case ACHIEV_CRIT_ABUSE_OOZE:
                return m_uiIronSludgeKilled >= MAX_ACHIEV_SLUDGES;

            default:
                return false;
            }
        }

    private:
        void ActivateFace(uint8 uiFace, bool bAfterEvent)   //@TODO get rid of the method
        {
            if (uiFace >= MAX_FACES)
            {
                return;
            }

            if (bAfterEvent)
            {
                DoUseDoorOrButton(m_aFaces[uiFace].m_goFaceGuid);
            }
            else
            {
                // TODO: How to get them red?
                DoUseDoorOrButton(m_aFaces[uiFace].m_goFaceGuid);
                m_aFaces[uiFace].m_bIsActive = true;
            }
        }

        void DoFaceSpeak(uint8 uiFace, int32 iTextId)   //@TODO get rid of the method
        {
            if (uiFace >= MAX_FACES)
            {
                return;
            }

            if (Creature* pSpeaker = instance->GetCreature(m_aFaces[uiFace].m_speakerGuid))
            {
                DoScriptText(iTextId, pSpeaker);
            }
        }

        void SortFaces()
        {
            std::list<Creature*> lPossibleEyes;

            // FACE_MARNAK
            if (GameObject* pFace = instance->GetGameObject(m_aFaces[FACE_MARNAK].m_goFaceGuid))
            {
                // Find Marnak NPCs
                GetValidNPCsOfList(instance, m_lMarnakGUIDs, lPossibleEyes);
                if (lPossibleEyes.size() > 1)
                {
                    lPossibleEyes.sort(SortHelper(pFace));
                    std::list<Creature*>::const_iterator itr = lPossibleEyes.begin();
                    m_aFaces[FACE_MARNAK].m_leftEyeGuid = (*itr)->GetObjectGuid();
                    ++itr;
                    m_aFaces[FACE_MARNAK].m_speakerGuid = (*itr)->GetObjectGuid();
                }
                // Find Worldtrigger NPC
                GetValidNPCsOfList(instance, m_lWorldtriggerGUIDs, lPossibleEyes);
                if (!lPossibleEyes.empty())
                {
                    lPossibleEyes.sort(SortHelper(pFace));
                    m_aFaces[FACE_MARNAK].m_rightEyeGuid = (*lPossibleEyes.begin())->GetObjectGuid();
                }
            }

            // FACE_ABEDNEUM
            if (GameObject* pFace = instance->GetGameObject(m_aFaces[FACE_ABEDNEUM].m_goFaceGuid))
            {
                // Find Abedneum NPCs
                GetValidNPCsOfList(instance, m_lAbedneumGUIDs, lPossibleEyes);
                if (lPossibleEyes.size() > 1)
                {
                    lPossibleEyes.sort(SortHelper(pFace));
                    std::list<Creature*>::const_iterator itr = lPossibleEyes.begin();
                    m_aFaces[FACE_ABEDNEUM].m_leftEyeGuid = (*itr)->GetObjectGuid();
                    ++itr;
                    m_aFaces[FACE_ABEDNEUM].m_speakerGuid = (*itr)->GetObjectGuid();
                }
                // Find Worldtrigger NPC
                GetValidNPCsOfList(instance, m_lWorldtriggerGUIDs, lPossibleEyes);
                if (!lPossibleEyes.empty())
                {
                    lPossibleEyes.sort(SortHelper(pFace));
                    m_aFaces[FACE_ABEDNEUM].m_rightEyeGuid = (*lPossibleEyes.begin())->GetObjectGuid();
                }
            }

            // FACE_KADDRAK
            if (GameObject* pFace = instance->GetGameObject(m_aFaces[FACE_KADDRAK].m_goFaceGuid))
            {
                // Find Marnak NPCs
                GetValidNPCsOfList(instance, m_lKaddrakGUIDs, lPossibleEyes);
                if (lPossibleEyes.size() > 1)
                {
                    lPossibleEyes.sort(SortHelper(pFace));
                    std::list<Creature*>::const_iterator itr = lPossibleEyes.begin();
                    m_aFaces[FACE_KADDRAK].m_leftEyeGuid = (*itr)->GetObjectGuid();
                    ++itr;
                    m_aFaces[FACE_KADDRAK].m_speakerGuid = (*itr)->GetObjectGuid();
                }
                // Find Tribunal NPC
                GetValidNPCsOfList(instance, m_lTribunalGUIDs, lPossibleEyes);
                if (!lPossibleEyes.empty())
                {
                    lPossibleEyes.sort(SortHelper(pFace));
                    m_aFaces[FACE_KADDRAK].m_rightEyeGuid = (*lPossibleEyes.begin())->GetObjectGuid();
                }
            }

            // Clear GUIDs
            m_lKaddrakGUIDs.clear();
            m_lAbedneumGUIDs.clear();
            m_lMarnakGUIDs.clear();
            m_lTribunalGUIDs.clear();
            m_lWorldtriggerGUIDs.clear();
        }

        void ProcessFace(uint8 uiFace)
        {
            // Cast dmg spell from face eyes, and reset timer for face
            switch (uiFace)
            {
            case FACE_KADDRAK:
                if (Creature* pEye = instance->GetCreature(m_aFaces[uiFace].m_leftEyeGuid))
                {
                    pEye->CastSpell(pEye, instance->IsRegularDifficulty() ? SPELL_GLARE_OF_THE_TRIBUNAL : SPELL_GLARE_OF_THE_TRIBUNAL_H, true);
                }
                if (Creature* pEye = instance->GetCreature(m_aFaces[uiFace].m_rightEyeGuid))
                {
                    pEye->CastSpell(pEye, instance->IsRegularDifficulty() ? SPELL_GLARE_OF_THE_TRIBUNAL : SPELL_GLARE_OF_THE_TRIBUNAL_H, true, nullptr, nullptr, m_aFaces[uiFace].m_leftEyeGuid);
                }
                m_aFaces[uiFace].m_uiTimer = urand(1000, 2000);
                break;
            case FACE_MARNAK:
                if (Creature* pDarkMatter = GetSingleCreatureFromStorage(NPC_DARK_MATTER))
                {
                    pDarkMatter->CastSpell(pDarkMatter, SPELL_DARK_MATTER_START, true);
                }
                // Note: Marnak doesn't cast anything directly. Keep this code for reference only.
                // if (Creature* pEye = instance->GetCreature(m_aFaces[uiFace].m_leftEyeGuid))
                //    pEye->CastSpell(pEye, SPELL_SUMMON_DARK_MATTER_TARGET, true);
                // One should be enough..
                // if (Creature* pEye = instance->GetCreature(m_aFaces[uiFace].m_rightEyeGuid))
                //    pEye->CastSpell(pEye, SPELL_SUMMON_DARK_MATTER_TARGET, true);
                m_aFaces[uiFace].m_uiTimer = urand(21000, 30000);
                break;
            case FACE_ABEDNEUM:
                if (Creature* pEye = instance->GetCreature(m_aFaces[uiFace].m_leftEyeGuid))
                {
                    pEye->CastSpell(pEye, SPELL_SUMMON_SEARING_GAZE_TARGET, true);
                }
                // One should be enough..
                // if (Creature* pEye = instance->GetCreature(m_aFaces[uiFace].m_rightEyeGuid))
                //    pEye->CastSpell(pEye, SPELL_SUMMON_SEARING_GAZE_TARGET, true);
                m_aFaces[uiFace].m_uiTimer = urand(15000, 20000);
                break;
            default:
                return;
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        Face m_aFaces[MAX_FACES];
        std::string m_strInstData;

        uint8 m_uiIronSludgeKilled;
        bool m_bIsBrannSpankin;

        ObjectGuid m_protectorStalkerGuid;
        ObjectGuid m_stormcallerStalkerGuid;
        ObjectGuid m_custodianStalkerGuid;

        GuidList m_lKaddrakGUIDs;
        GuidList m_lAbedneumGUIDs;
        GuidList m_lMarnakGUIDs;
        GuidList m_lTribunalGUIDs;
        GuidList m_lWorldtriggerGUIDs;

        struct SortHelper
        {
            SortHelper(WorldObject const* pRef) : m_pRef(pRef) {}
            bool operator()(WorldObject* pLeft, WorldObject* pRight)
            {
                return m_pRef->GetDistanceOrder(pLeft, pRight);
            }
            WorldObject const* m_pRef;
        };

    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_halls_of_stone(pMap);
    }
};

void AddSC_instance_halls_of_stone()
{
    Script* s;

    s = new is_halls_of_stone();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_halls_of_stone";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_halls_of_stone;
    //pNewScript->RegisterSelf();
}
