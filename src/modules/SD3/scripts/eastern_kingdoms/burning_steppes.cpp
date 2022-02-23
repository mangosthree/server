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
 * SDName:      Burning_Steppes
 * SD%Complete: 100
 * SDComment:   Quest support: 4121, 4122, 4866.
 * SDCategory:  Burning Steppes
 * EndScriptData
 */

/**
 * ContentData
 * npc_grark_lorkrub
 * EndContentData
 */

#include "precompiled.h"
#include "escort_ai.h"

/*######
## npc_ragged_john
######*/

struct npc_ragged_john : public CreatureScript
{
    npc_ragged_john() : CreatureScript("npc_ragged_john") {}

    struct npc_ragged_johnAI : public ScriptedAI
    {
        npc_ragged_johnAI(Creature* pCreature) : ScriptedAI(pCreature) { }

        void MoveInLineOfSight(Unit* who) override
        {
            if (who->HasAura(16468, EFFECT_INDEX_0))
            {
                if (who->GetTypeId() == TYPEID_PLAYER && m_creature->IsWithinDistInMap(who, 15) && who->isInAccessablePlaceFor(m_creature))
                {
                    DoCastSpellIfCan(who, 16472);
                    ((Player*)who)->AreaExploredOrEventHappens(4866);
                }
            }

            if (!m_creature->getVictim() && who->IsTargetableForAttack() && (m_creature->IsHostileTo(who)) && who->isInAccessablePlaceFor(m_creature))
            {
                if (!m_creature->CanFly() && m_creature->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
                {
                    return;
                }

                float attackRadius = m_creature->GetAttackDistance(who);
                if (m_creature->IsWithinDistInMap(who, attackRadius) && m_creature->IsWithinLOSInMap(who))
                {
                    who->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
                    AttackStart(who);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_ragged_johnAI(pCreature);
    }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override   //TODO localisation setup
    {
        if (pCreature->IsQuestGiver())
        {
            pPlayer->PrepareQuestMenu(pCreature->GetObjectGuid());
        }

        if (pPlayer->GetQuestStatus(4224) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Official business, John. I need some information about Marshal Windsor. Tell me about he last time you saw him.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        }

        pPlayer->SEND_GOSSIP_MENU(2713, pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 uiAction) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch (uiAction)
        {
        case GOSSIP_ACTION_INFO_DEF:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "So what did you do?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            pPlayer->SEND_GOSSIP_MENU(2714, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 1:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Start making sense, dwarf. I don't want to have anything to do with your cracker, your pappy, or any sort of 'discreditin'.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->SEND_GOSSIP_MENU(2715, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 2:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Ironfoe?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->SEND_GOSSIP_MENU(2716, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 3:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Interesting... continue, John.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            pPlayer->SEND_GOSSIP_MENU(2717, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 4:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "So that's how Windsor died...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
            pPlayer->SEND_GOSSIP_MENU(2718, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 5:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "So how did he die?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
            pPlayer->SEND_GOSSIP_MENU(2719, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 6:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Ok, so where the hell is he? Wait a minute! Are you drunk?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            pPlayer->SEND_GOSSIP_MENU(2720, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 7:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "WHY is he in Blackrock Depths?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 8);
            pPlayer->SEND_GOSSIP_MENU(2721, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 8:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "300? So the Dark Irons killed him and dragged him into the Depths?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);
            pPlayer->SEND_GOSSIP_MENU(2722, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 9:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Ahh... Ironfoe.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 10);
            pPlayer->SEND_GOSSIP_MENU(2723, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 10:
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Thanks, Ragged John. Your story was very uplifting and informative.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 11);
            pPlayer->SEND_GOSSIP_MENU(2725, pCreature->GetObjectGuid());
            break;
        case GOSSIP_ACTION_INFO_DEF + 11:
            pPlayer->CLOSE_GOSSIP_MENU();
            pPlayer->AreaExploredOrEventHappens(4224);
            break;
        }
        return true;
    }
};

/*######
## npc_grark_lorkrub
######*/

enum
{
    SAY_START                   = -1000873,
    SAY_PAY                     = -1000874,
    SAY_FIRST_AMBUSH_START      = -1000875,
    SAY_FIRST_AMBUSH_END        = -1000876,
    SAY_SEC_AMBUSH_START        = -1000877,
    SAY_SEC_AMBUSH_END          = -1000878,
    SAY_THIRD_AMBUSH_START      = -1000879,
    SAY_THIRD_AMBUSH_END        = -1000880,
    EMOTE_LAUGH                 = -1000881,
    SAY_LAST_STAND              = -1000882,
    SAY_LEXLORT_1               = -1000883,
    SAY_LEXLORT_2               = -1000884,
    EMOTE_RAISE_AXE             = -1000885,
    EMOTE_LOWER_HAND            = -1000886,
    SAY_LEXLORT_3               = -1000887,
    SAY_LEXLORT_4               = -1000888,

    EMOTE_SUBMIT                = -1000889,
    SAY_AGGRO                   = -1000890,

    SPELL_CAPTURE_GRARK             = 14250,

    NPC_BLACKROCK_AMBUSHER          = 9522,
    NPC_BLACKROCK_RAIDER            = 9605,
    NPC_FLAMESCALE_DRAGONSPAWN      = 7042,
    NPC_SEARSCALE_DRAKE             = 7046,

    NPC_GRARK_LORKRUB               = 9520,
    NPC_HIGH_EXECUTIONER_NUZARK     = 9538,
    NPC_SHADOW_OF_LEXLORT           = 9539,

    FACTION_FRIENDLY                = 35,

    QUEST_ID_PRECARIOUS_PREDICAMENT = 4121
};

static const DialogueEntry aOutroDialogue[] =
{
    {SAY_LAST_STAND,    NPC_GRARK_LORKRUB,              5000},
    {SAY_LEXLORT_1,     NPC_SHADOW_OF_LEXLORT,          3000},
    {SAY_LEXLORT_2,     NPC_SHADOW_OF_LEXLORT,          5000},
    {EMOTE_RAISE_AXE,   NPC_HIGH_EXECUTIONER_NUZARK,    4000},
    {EMOTE_LOWER_HAND,  NPC_SHADOW_OF_LEXLORT,          3000},
    {SAY_LEXLORT_3,     NPC_SHADOW_OF_LEXLORT,          3000},
    {NPC_GRARK_LORKRUB, 0,                              5000},
    {SAY_LEXLORT_4,     NPC_SHADOW_OF_LEXLORT,          0},
    {0, 0, 0},
};

struct npc_grark_lorkrub : public CreatureScript
{
    npc_grark_lorkrub() : CreatureScript("npc_grark_lorkrub") {}

    struct npc_grark_lorkrubAI : public npc_escortAI, private DialogueHelper
    {
        npc_grark_lorkrubAI(Creature* pCreature) : npc_escortAI(pCreature),
        DialogueHelper(aOutroDialogue)
        {
        }

        ObjectGuid m_nuzarkGuid;
        ObjectGuid m_lexlortGuid;

        GuidList m_lSearscaleGuidList;

        uint8 m_uiKilledCreatures;
        bool m_bIsFirstSearScale;

        void Reset() override
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                m_uiKilledCreatures = 0;
                m_bIsFirstSearScale = true;

                m_lSearscaleGuidList.clear();

                m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }
        }

        void Aggro(Unit* /*pWho*/) override
        {
            if (!HasEscortState(STATE_ESCORT_ESCORTING))
            {
                DoScriptText(SAY_AGGRO, m_creature);
            }
        }

        void MoveInLineOfSight(Unit* pWho) override
        {
            // No combat during escort
            if (HasEscortState(STATE_ESCORT_ESCORTING))
            {
                return;
            }

            npc_escortAI::MoveInLineOfSight(pWho);
        }

        void WaypointReached(uint32 uiPointId) override
        {
            switch (uiPointId)
            {
            case 1:
                DoScriptText(SAY_START, m_creature);
                break;
            case 7:
                DoScriptText(SAY_PAY, m_creature);
                break;
            case 12:
                DoScriptText(SAY_FIRST_AMBUSH_START, m_creature);
                SetEscortPaused(true);

                m_creature->SummonCreature(NPC_BLACKROCK_AMBUSHER, -7844.3f, -1521.6f, 139.2f, 0.0f, TEMPSPAWN_TIMED_OOC_DESPAWN, 20000);
                m_creature->SummonCreature(NPC_BLACKROCK_AMBUSHER, -7860.4f, -1507.8f, 141.0f, 6.0f, TEMPSPAWN_TIMED_OOC_DESPAWN, 20000);
                m_creature->SummonCreature(NPC_BLACKROCK_RAIDER, -7845.6f, -1508.1f, 138.8f, 6.1f, TEMPSPAWN_TIMED_OOC_DESPAWN, 20000);
                m_creature->SummonCreature(NPC_BLACKROCK_RAIDER, -7859.8f, -1521.8f, 139.2f, 6.2f, TEMPSPAWN_TIMED_OOC_DESPAWN, 20000);
                break;
            case 24:
                DoScriptText(SAY_SEC_AMBUSH_START, m_creature);
                SetEscortPaused(true);

                m_creature->SummonCreature(NPC_BLACKROCK_AMBUSHER, -8035.3f, -1222.2f, 135.5f, 5.1f, TEMPSPAWN_TIMED_OOC_DESPAWN, 20000);
                m_creature->SummonCreature(NPC_FLAMESCALE_DRAGONSPAWN, -8037.5f, -1216.9f, 135.8f, 5.1f, TEMPSPAWN_TIMED_OOC_DESPAWN, 20000);
                m_creature->SummonCreature(NPC_BLACKROCK_AMBUSHER, -8009.5f, -1222.1f, 139.2f, 3.9f, TEMPSPAWN_TIMED_OOC_DESPAWN, 20000);
                m_creature->SummonCreature(NPC_FLAMESCALE_DRAGONSPAWN, -8007.1f, -1219.4f, 140.1f, 3.9f, TEMPSPAWN_TIMED_OOC_DESPAWN, 20000);
                break;
            case 28:
                m_creature->SummonCreature(NPC_SEARSCALE_DRAKE, -7897.8f, -1123.1f, 233.4f, 3.0f, TEMPSPAWN_TIMED_OOC_DESPAWN, 60000);
                m_creature->SummonCreature(NPC_SEARSCALE_DRAKE, -7898.8f, -1125.1f, 193.9f, 3.0f, TEMPSPAWN_TIMED_OOC_DESPAWN, 60000);
                m_creature->SummonCreature(NPC_SEARSCALE_DRAKE, -7895.6f, -1119.5f, 194.5f, 3.1f, TEMPSPAWN_TIMED_OOC_DESPAWN, 60000);
                break;
            case 30:
            {
                       SetEscortPaused(true);
                       DoScriptText(SAY_THIRD_AMBUSH_START, m_creature);

                       Player* pPlayer = GetPlayerForEscort();
                       if (!pPlayer)
                       {
                           return;
                       }

                       // Set all the dragons in combat
                       for (GuidList::const_iterator itr = m_lSearscaleGuidList.begin(); itr != m_lSearscaleGuidList.end(); ++itr)
                       {
                           if (Creature* pTemp = m_creature->GetMap()->GetCreature(*itr))
                           {
                               pTemp->AI()->AttackStart(pPlayer);
                           }
                       }
                       break;
            }
            case 36:
                DoScriptText(EMOTE_LAUGH, m_creature);
                break;
            case 45:
                StartNextDialogueText(SAY_LAST_STAND);
                SetEscortPaused(true);

                m_creature->SummonCreature(NPC_HIGH_EXECUTIONER_NUZARK, -7532.3f, -1029.4f, 258.0f, 2.7f, TEMPSPAWN_TIMED_DESPAWN, 40000);
                m_creature->SummonCreature(NPC_SHADOW_OF_LEXLORT, -7532.8f, -1032.9f, 258.2f, 2.5f, TEMPSPAWN_TIMED_DESPAWN, 40000);
                break;
            }
        }

        void JustDidDialogueStep(int32 iEntry) override
        {
            switch (iEntry)
            {
            case SAY_LEXLORT_1:
                m_creature->SetStandState(UNIT_STAND_STATE_KNEEL);
                break;
            case SAY_LEXLORT_3:
                // Note: this part isn't very clear. Should he just simply attack him, or charge him?
                if (Creature* pNuzark = m_creature->GetMap()->GetCreature(m_nuzarkGuid))
                {
                    pNuzark->HandleEmote(EMOTE_ONESHOT_ATTACK2HTIGHT);
                }
                break;
            case NPC_GRARK_LORKRUB:
                // Fake death creature when the axe is lowered. This will allow us to finish the event
                m_creature->InterruptNonMeleeSpells(true);
                m_creature->SetHealth(1);
                m_creature->StopMoving();
                m_creature->ClearComboPointHolders();
                m_creature->RemoveAllAurasOnDeath();
                m_creature->ModifyAuraState(AURA_STATE_HEALTHLESS_20_PERCENT, false);

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
                m_creature->ModifyAuraState(AURA_STATE_HEALTHLESS_35_PERCENT, false);
#endif

                m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                m_creature->ClearAllReactives();
                m_creature->GetMotionMaster()->Clear();
                m_creature->GetMotionMaster()->MoveIdle();
                m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
                break;
            case SAY_LEXLORT_4:
                // Finish the quest
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    pPlayer->GroupEventHappens(QUEST_ID_PRECARIOUS_PREDICAMENT, m_creature);
                }
                // Kill self
                m_creature->DealDamage(m_creature, m_creature->GetHealth(), nullptr, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NONE, nullptr, false);
                break;
            }
        }

        void JustSummoned(Creature* pSummoned) override
        {
            switch (pSummoned->GetEntry())
            {
            case NPC_HIGH_EXECUTIONER_NUZARK:
                m_nuzarkGuid = pSummoned->GetObjectGuid();
                break;
            case NPC_SHADOW_OF_LEXLORT:
                m_lexlortGuid = pSummoned->GetObjectGuid();
                break;
            case NPC_SEARSCALE_DRAKE:
                // If it's the flying drake allow him to move in circles
                if (m_bIsFirstSearScale)
                {
                    m_bIsFirstSearScale = false;

                    pSummoned->SetLevitate(true);
                    // ToDo: this guy should fly in circles above the creature
                }
                m_lSearscaleGuidList.push_back(pSummoned->GetObjectGuid());
                break;

            default:
                // The hostile mobs should attack the player only
                if (Player* pPlayer = GetPlayerForEscort())
                {
                    pSummoned->AI()->AttackStart(pPlayer);
                }
                break;
            }
        }

        void SummonedCreatureJustDied(Creature* /*pSummoned*/) override
        {
            ++m_uiKilledCreatures;

            switch (m_uiKilledCreatures)
            {
            case 4:
                DoScriptText(SAY_FIRST_AMBUSH_END, m_creature);
                SetEscortPaused(false);
                break;
            case 8:
                DoScriptText(SAY_SEC_AMBUSH_END, m_creature);
                SetEscortPaused(false);
                break;
            case 11:
                DoScriptText(SAY_THIRD_AMBUSH_END, m_creature);
                SetEscortPaused(false);
                break;
            }
        }

        Creature* GetSpeakerByEntry(uint32 uiEntry) override
        {
            switch (uiEntry)
            {
            case NPC_GRARK_LORKRUB:
                return m_creature;
            case NPC_HIGH_EXECUTIONER_NUZARK:
                return m_creature->GetMap()->GetCreature(m_nuzarkGuid);
            case NPC_SHADOW_OF_LEXLORT:
                return m_creature->GetMap()->GetCreature(m_lexlortGuid);

            default:
                return nullptr;
            }
        }

        void UpdateEscortAI(const uint32 uiDiff) override
        {
            DialogueUpdate(uiDiff);

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_grark_lorkrubAI(pCreature);
    }

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, const Quest* pQuest) override
    {
        if (pQuest->GetQuestId() == QUEST_ID_PRECARIOUS_PREDICAMENT)
        {
            if (npc_grark_lorkrubAI* pEscortAI = dynamic_cast<npc_grark_lorkrubAI*>(pCreature->AI()))
            {
                pEscortAI->Start(false, pPlayer, pQuest);
            }

            return true;
        }

        return false;
    }
};

struct spell_capture_grark : public SpellScript
{
    spell_capture_grark() : SpellScript("spell_capture_grark") {}

    bool EffectDummy(Unit* /*pCaster*/, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pCreatureTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        // always check spellid and effectindex
        if (uiSpellId == SPELL_CAPTURE_GRARK && uiEffIndex == EFFECT_INDEX_0)
        {
            // Note: this implementation needs additional research! There is a lot of guesswork involved in this!
            if (pCreatureTarget->ToCreature()->GetHealthPercent() > 25.0f)
            {
                return false;
            }

            // The faction is guesswork - needs more research
            DoScriptText(EMOTE_SUBMIT, pCreatureTarget->ToCreature());
            pCreatureTarget->ToCreature()->SetFactionTemporary(FACTION_FRIENDLY, TEMPFACTION_RESTORE_RESPAWN);
            pCreatureTarget->ToCreature()->AI()->EnterEvadeMode();

            // always return true when we are handling this spell and effect
            return true;
        }

        return false;
    }
};

/*######
## npc_franklin_the_friendly
######*/

/*######
## npc_klinfran_the_crazed
######*/

enum
{
    SPELL_FOOLS_PLIGHT = 23504,

    SPELL_DEMONIC_FRENZY = 23257,
    SPELL_ENTROPIC_STING = 23260,

    EMOTE_FRENZY = -1000001,

    NPC_KLINFRAN_THE_AMIABLE = 14529,
    GOSSIP_ITEM_KLINFRAN_THE_AMIABLE = -3509004,

    NPC_KLINFRAN_THE_CRAZED = 14534,
    NPC_THE_CLEANER = 14503,
    SAY_THE_CLEANER_AGGRO = -1001253,

    QUEST_STAVE_OF_THE_ANCIENTS = 7636
};

struct npc_klinfran_the_crazed : public CreatureScript
{
    npc_klinfran_the_crazed() : CreatureScript("npc_klinfran_the_crazed") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        // Allow to begin the event only for a player (Hunter) who have not completed the quest yet.
        if (pPlayer->GetQuestStatus(QUEST_STAVE_OF_THE_ANCIENTS) == QUEST_STATUS_INCOMPLETE)
        {
            pPlayer->ADD_GOSSIP_ITEM_ID(0, GOSSIP_ITEM_KLINFRAN_THE_AMIABLE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        }

        pPlayer->SEND_GOSSIP_MENU(pPlayer->GetGossipTextId(pCreature), pCreature->GetObjectGuid());
        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/, uint32 /*uiAction*/) override
    {
        pPlayer->CLOSE_GOSSIP_MENU();
        ((npc_klinfran_the_crazedAI*)pCreature->AI())->BeginEvent(pPlayer->GetObjectGuid());
        return true;
    }

    struct npc_klinfran_the_crazedAI : public ScriptedAI
    {
        npc_klinfran_the_crazedAI(Creature* pCreature) : ScriptedAI(pCreature), m_uiDespawn_Timer(0), m_bTransform(false)
        {
            Reset();
        }

        uint32 m_uiTransform_Timer;
        uint32 m_uiTransformEmote_Timer;
        bool m_bTransform;

        ObjectGuid m_hunterGuid;
        uint32 m_uiDemonic_Frenzy_Timer;
        uint32 m_uiDespawn_Timer;

        void Reset() override
        {
            switch (m_creature->GetEntry())
            {
                case NPC_KLINFRAN_THE_AMIABLE:
                {
                    m_creature->SetRespawnDelay(35 * MINUTE);
                    m_creature->SetRespawnTime(35 * MINUTE);
                    m_creature->NearTeleportTo(-8318.19f, -993.662f, 176.956f, 5.65024f);
                    if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != WAYPOINT_MOTION_TYPE)
                    {
                        m_creature->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
                        m_creature->GetMotionMaster()->Initialize();
                    }

                    m_creature->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                    m_uiTransform_Timer = 10000;
                    m_uiTransformEmote_Timer = 5000;
                    m_bTransform = false;
                    m_uiDespawn_Timer = 0;
                    break;
                }

                case NPC_KLINFRAN_THE_CRAZED:
                {
                    if (!m_uiDespawn_Timer)
                    {
                        m_uiDespawn_Timer = 20 * MINUTE * IN_MILLISECONDS;
                    }

                    m_hunterGuid.Clear();
                    m_uiDemonic_Frenzy_Timer = 5000;
                    break;
                }
            }
        }

        /** Franklin the Friendly */
        void Transform()
        {
            m_creature->UpdateEntry(NPC_KLINFRAN_THE_CRAZED);
            Reset();
        }

        void BeginEvent(ObjectGuid playerGuid)
        {
            m_hunterGuid = playerGuid;
            m_creature->GetMotionMaster()->Clear(false);
            m_creature->GetMotionMaster()->MoveIdle();
            m_creature->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
            m_bTransform = true;
        }

        /** Klinfran the Crazed */
        void Aggro(Unit* pWho) override
        {
            if (pWho->getClass() == CLASS_HUNTER && (m_hunterGuid.IsEmpty() || m_hunterGuid == pWho->GetObjectGuid()))
            {
                m_hunterGuid = pWho->GetObjectGuid();
            }
            else
            {
                DemonDespawn(pWho);
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            uint32 m_respawn_delay_Timer = urand(2, 3) * HOUR;
            m_creature->SetRespawnDelay(m_respawn_delay_Timer);
            m_creature->SetRespawnTime(m_respawn_delay_Timer);
            m_creature->SaveRespawnTime();
        }

        void DemonDespawn(Unit* playerFacing = nullptr, bool triggered = true)
        {
            uint32 respawnTime = urand(12, 15);
            m_creature->SetRespawnDelay(respawnTime * MINUTE);
            m_creature->SetRespawnTime(respawnTime * MINUTE);
            m_creature->SaveRespawnTime();

            if (triggered)
            {
                Creature* creature_the_cleaner = m_creature->SummonCreature(NPC_THE_CLEANER, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), m_creature->GetAngle(playerFacing), TEMPSPAWN_CORPSE_DESPAWN, 20 * MINUTE * IN_MILLISECONDS);
                if (creature_the_cleaner)
                {
                    DoScriptText(SAY_THE_CLEANER_AGGRO, creature_the_cleaner);
                    ThreatList const& tList = m_creature->GetThreatManager().getThreatList();
                    for (auto itr : tList)
                    {
                        if (Unit* pUnit = m_creature->GetMap()->GetUnit(itr->getUnitGuid()))
                        {
                            if (pUnit->IsAlive())
                            {
                                creature_the_cleaner->AI()->AttackStart(pUnit);
                            }
                        }
                    }
                }
            }

            m_creature->ForcedDespawn();
        }

        void SpellHit(Unit* /*pCaster*/, const SpellEntry* pSpell) override
        {
            if (pSpell && pSpell->Id == 14277)   // Scorpid Sting (Rank 4)
            {
                m_creature->RemoveAurasDueToSpell(SPELL_DEMONIC_FRENZY);
                DoCastSpellIfCan(m_creature, SPELL_ENTROPIC_STING, CAST_TRIGGERED);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            /** Franklin the Friendly */
            if (m_bTransform)
            {
                if (m_uiTransformEmote_Timer)
                {
                    if (m_uiTransformEmote_Timer <= uiDiff)
                    {
                        m_creature->HandleEmote(EMOTE_ONESHOT_POINT);
                        m_uiTransformEmote_Timer = 0;
                    }
                    else
                    {
                        m_uiTransformEmote_Timer -= uiDiff;
                    }
                }

                if (m_uiTransform_Timer < uiDiff)
                {
                    m_bTransform = false;
                    Transform();
                }
                else
                {
                    m_uiTransform_Timer -= uiDiff;
                }
            }

            /** Klinfran the Crazed */
            if (m_uiDespawn_Timer)
            {
                if (m_uiDespawn_Timer <= uiDiff)
                {
                    if (m_creature->IsAlive() && !m_creature->IsInCombat())
                    {
                        DemonDespawn(nullptr,false);
                    }
                }
                else
                {
                    m_uiDespawn_Timer -= uiDiff;
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_creature->GetThreatManager().getThreatList().size() > 1 /*|| pHunter->IsDead()*/)
            {
                DemonDespawn(m_creature->getVictim());
            }

            if (m_uiDemonic_Frenzy_Timer < uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_DEMONIC_FRENZY) == CAST_OK)
                {
                    DoScriptText(EMOTE_FRENZY, m_creature);
                    m_uiDemonic_Frenzy_Timer = 15000;
                }
            }
            else
            {
                m_uiDemonic_Frenzy_Timer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_klinfran_the_crazedAI(pCreature);
    }
};


void AddSC_burning_steppes()
{
    Script* s;
    s = new npc_ragged_john();
    s->RegisterSelf();
    s = new npc_grark_lorkrub();
    s->RegisterSelf();
    s = new spell_capture_grark();
    s->RegisterSelf();
    s = new npc_klinfran_the_crazed();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_ragged_john";
    //pNewScript->GetAI = &GetAI_npc_ragged_john;
    //pNewScript->pGossipHello =  &GossipHello_npc_ragged_john;
    //pNewScript->pGossipSelect = &GossipSelect_npc_ragged_john;
    //pNewScript->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "npc_grark_lorkrub";
    //pNewScript->GetAI = &GetAI_npc_grark_lorkrub;
    //pNewScript->pQuestAcceptNPC = &QuestAccept_npc_grark_lorkrub;
    //pNewScript->pEffectDummyNPC = &EffectDummyCreature_spell_capture_grark;
    //pNewScript->RegisterSelf();
}
