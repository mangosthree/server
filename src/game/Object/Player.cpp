/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

#include "Player.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "Opcodes.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "UpdateMask.h"
#include "SkillDiscovery.h"
#include "QuestDef.h"
#include "GossipDef.h"
#include "UpdateData.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "MapManager.h"
#include "MapPersistentStateMgr.h"
#include "InstanceData.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Formulas.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Pet.h"
#include "Util.h"
#include "Transports.h"
#include "Weather.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGroundAV.h"
#include "OutdoorPvP/OutdoorPvP.h"
#include "ArenaTeam.h"
#include "Chat.h"
#include "revision_data.h"
#include "Database/DatabaseImpl.h"
#include "Spell.h"
#include "ScriptMgr.h"
#include "SocialMgr.h"
#include "AchievementMgr.h"
#include "Mail.h"
#include "SpellAuras.h"
#include "DBCStores.h"
#include "DB2Stores.h"
#include "SQLStorages.h"
#include "Vehicle.h"
#include "Calendar.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

#include <cmath>

#define ZONE_UPDATE_INTERVAL (1*IN_MILLISECONDS)

enum CharacterFlags
{
    CHARACTER_FLAG_NONE                 = 0x00000000,
    CHARACTER_FLAG_UNK1                 = 0x00000001,
    CHARACTER_FLAG_UNK2                 = 0x00000002,
    CHARACTER_LOCKED_FOR_TRANSFER       = 0x00000004,
    CHARACTER_FLAG_UNK4                 = 0x00000008,
    CHARACTER_FLAG_UNK5                 = 0x00000010,
    CHARACTER_FLAG_UNK6                 = 0x00000020,
    CHARACTER_FLAG_UNK7                 = 0x00000040,
    CHARACTER_FLAG_UNK8                 = 0x00000080,
    CHARACTER_FLAG_UNK9                 = 0x00000100,
    CHARACTER_FLAG_UNK10                = 0x00000200,
    CHARACTER_FLAG_HIDE_HELM            = 0x00000400,
    CHARACTER_FLAG_HIDE_CLOAK           = 0x00000800,
    CHARACTER_FLAG_UNK13                = 0x00001000,
    CHARACTER_FLAG_GHOST                = 0x00002000,
    CHARACTER_FLAG_RENAME               = 0x00004000,
    CHARACTER_FLAG_UNK16                = 0x00008000,
    CHARACTER_FLAG_UNK17                = 0x00010000,
    CHARACTER_FLAG_UNK18                = 0x00020000,
    CHARACTER_FLAG_UNK19                = 0x00040000,
    CHARACTER_FLAG_UNK20                = 0x00080000,
    CHARACTER_FLAG_UNK21                = 0x00100000,
    CHARACTER_FLAG_UNK22                = 0x00200000,
    CHARACTER_FLAG_UNK23                = 0x00400000,
    CHARACTER_FLAG_UNK24                = 0x00800000,
    CHARACTER_FLAG_LOCKED_BY_BILLING    = 0x01000000,
    CHARACTER_FLAG_DECLINED             = 0x02000000,
    CHARACTER_FLAG_UNK27                = 0x04000000,
    CHARACTER_FLAG_UNK28                = 0x08000000,
    CHARACTER_FLAG_UNK29                = 0x10000000,
    CHARACTER_FLAG_UNK30                = 0x20000000,
    CHARACTER_FLAG_UNK31                = 0x40000000,
    CHARACTER_FLAG_UNK32                = 0x80000000
};

enum CharacterCustomizeFlags
{
    CHAR_CUSTOMIZE_FLAG_NONE            = 0x00000000,
    CHAR_CUSTOMIZE_FLAG_CUSTOMIZE       = 0x00000001,       // name, gender, etc...
    CHAR_CUSTOMIZE_FLAG_FACTION         = 0x00010000,       // name, gender, faction, etc...
    CHAR_CUSTOMIZE_FLAG_RACE            = 0x00100000        // name, gender, race, etc...
};

// corpse reclaim times
#define DEATH_EXPIRE_STEP (5*MINUTE)
#define MAX_DEATH_COUNT 3

static const uint32 corpseReclaimDelay[MAX_DEATH_COUNT] = {30, 60, 120};

//== TradeData =================================================

TradeData* TradeData::GetTraderData() const
{
    return m_trader->GetTradeData();
}

/**
 * @brief Gets the item placed in a trade slot.
 *
 * @param slot The trade slot.
 * @return The item in the slot, or null if empty.
 */
Item* TradeData::GetItem(TradeSlots slot) const
{
    return m_items[slot] ? m_player->GetItemByGuid(m_items[slot]) : NULL;
}

/**
 * @brief Checks whether a specific item is part of the current trade.
 *
 * @param item_guid The item GUID to test.
 * @return true if the item is present in a trade slot; otherwise, false.
 */
bool TradeData::HasItem(ObjectGuid item_guid) const
{
    for (int i = 0; i < TRADE_SLOT_COUNT; ++i)
        if (m_items[i] == item_guid)
        {
            return true;
        }
    return false;
}


/**
 * @brief Gets the item used as the current trade spell reagent.
 *
 * @return The spell-cast item, or null if none is set.
 */
Item* TradeData::GetSpellCastItem() const
{
    return m_spellCastItem ?  m_player->GetItemByGuid(m_spellCastItem) : NULL;
}

/**
 * @brief Sets the item assigned to a trade slot and refreshes trade state.
 *
 * @param slot The trade slot to update.
 * @param item The item to place in the slot, or null to clear it.
 */
void TradeData::SetItem(TradeSlots slot, Item* item)
{
    ObjectGuid itemGuid = item ? item->GetObjectGuid() : ObjectGuid();

    if (m_items[slot] == itemGuid)
    {
        return;
    }

    m_items[slot] = itemGuid;

    SetAccepted(false);
    GetTraderData()->SetAccepted(false);

    Update();

    // need remove possible trader spell applied to changed item
    if (slot == TRADE_SLOT_NONTRADED)
    {
        GetTraderData()->SetSpell(0);
    }

    // need remove possible player spell applied (possible move reagent)
    SetSpell(0);
}

/**
 * @brief Sets the spell cast through the trade window.
 *
 * @param spell_id The spell identifier.
 * @param castItem The optional reagent item used for the spell.
 */
void TradeData::SetSpell(uint32 spell_id, Item* castItem /*= NULL*/)
{
    ObjectGuid itemGuid = castItem ? castItem->GetObjectGuid() : ObjectGuid();

    if (m_spell == spell_id && m_spellCastItem == itemGuid)
    {
        return;
    }

    m_spell = spell_id;
    m_spellCastItem = itemGuid;

    SetAccepted(false);
    GetTraderData()->SetAccepted(false);

    Update(true);                                           // send spell info to item owner
    Update(false);                                          // send spell info to caster self
}

/**
 * @brief Sets the trade money offer and refreshes trade state.
 *
 * @param money The offered money amount.
 */
void TradeData::SetMoney(uint64 money)
{
    if (m_money == money)
    {
        return;
    }

    if (money > m_player->GetMoney())
    {
        m_player->GetSession()->SendTradeStatus(TRADE_STATUS_CLOSE_WINDOW);
        return;
    }

    m_money = money;

    SetAccepted(false);
    GetTraderData()->SetAccepted(false);

    Update();
}

/**
 * @brief Sends the current trade state to one side of the trade.
 *
 * @param for_trader true to update the trader view; false to update the player view.
 */
void TradeData::Update(bool for_trader /*= true*/)
{
    if (for_trader)
    {
        m_trader->GetSession()->SendUpdateTrade(true); // player state for trader
    }
    else
    {
        m_player->GetSession()->SendUpdateTrade(false); // player state for player
    }
}

/**
 * @brief Sets the accepted state for the trade and optionally notifies the other trader.
 *
 * @param state The new accepted state.
 * @param crosssend true to send the status to the trader instead of the owner.
 */
void TradeData::SetAccepted(bool state, bool crosssend /*= false*/)
{
    m_accepted = state;

    if (!state)
    {
        if (crosssend)
        {
            m_trader->GetSession()->SendTradeStatus(TRADE_STATUS_BACK_TO_TRADE);
        }
        else
        {
            m_player->GetSession()->SendTradeStatus(TRADE_STATUS_BACK_TO_TRADE);
        }
    }
}

//== Player ====================================================

UpdateMask Player::updateVisualBits;

/**
 * @brief Initializes a player instance and its runtime state.
 *
 * @param session The owning world session.
 */
Player::Player(WorldSession* session): Unit(), m_mover(this), m_camera(this), m_petMgr(this), m_achievementMgr(this), m_reputationMgr(this), m_glyphMgr(this), m_honorMgr(this), m_currencyMgr(this)
{
    m_transport = 0;

    m_speakTime = 0;
    m_speakCount = 0;
    m_isOptingOutOfLoot = false;

    m_objectType |= TYPEMASK_PLAYER;
    m_objectTypeId = TYPEID_PLAYER;

    m_valuesCount = PLAYER_END;

    SetActiveObjectState(true);                             // player is always active object

    m_session = session;

    m_ExtraFlags = 0;
    if (GetSession()->GetSecurity() >= SEC_GAMEMASTER)
    {
        SetAcceptTicket(true);
    }

    // players always accept
    if (GetSession()->GetSecurity() == SEC_PLAYER)
    {
        SetAcceptWhispers(true);
    }

    m_comboPoints = 0;

    m_usedTalentCount = 0;
    m_questRewardTalentCount = 0;
    m_freeTalentPoints = 0;

    m_regenTimer = 0;
    m_holyPowerRegenTimer = REGEN_TIME_HOLY_POWER;
    m_weaponChangeTimer = 0;

    m_zoneUpdateId = 0;
    m_zoneUpdateTimer = 0;
    m_positionStatusUpdateTimer = 0;

    m_areaUpdateId = 0;

    m_nextSave = sWorld.getConfig(CONFIG_UINT32_INTERVAL_SAVE);

    // randomize first save time in range [CONFIG_UINT32_INTERVAL_SAVE] around [CONFIG_UINT32_INTERVAL_SAVE]
    // this must help in case next save after mass player load after server startup
    m_nextSave = urand(m_nextSave / 2, m_nextSave * 3 / 2);

    clearResurrectRequestData();

    memset(m_items, 0, sizeof(Item*)*PLAYER_SLOTS_COUNT);

    m_social = NULL;

    // group is initialized in the reference constructor
    SetGroupInvite(NULL);
    m_groupUpdateMask = 0;
    m_auraUpdateMask = 0;

    duel = NULL;

    m_GuildIdInvited = 0;
    m_ArenaTeamIdInvited = 0;

    m_atLoginFlags = AT_LOGIN_NONE;

    mSemaphoreTeleport_Near = false;
    mSemaphoreTeleport_Far = false;

    m_DelayedOperations = 0;
    m_bCanDelayTeleport = false;
    m_bHasDelayedTeleport = false;
    m_bHasBeenAliveAtDelayedTeleport = true;                // overwrite always at setup teleport data, so not used infact
    m_teleport_options = 0;

    m_trade = NULL;

    m_cinematic = 0;

    PlayerTalkClass = new PlayerMenu(GetSession());
    m_currentBuybackSlot = BUYBACK_SLOT_START;

    m_DailyQuestChanged = false;
    m_WeeklyQuestChanged = false;

    m_lastLiquid = NULL;

    for (int i = 0; i < MAX_TIMERS; ++i)
    {
        m_MirrorTimer[i] = DISABLED_MIRROR_TIMER;
    }

    m_MirrorTimerFlags = UNDERWATER_NONE;
    m_MirrorTimerFlagsLast = UNDERWATER_NONE;

    m_isInWater = false;
    m_drunkTimer = 0;
    m_restTime = 0;
    m_deathTimer = 0;
    // Initialize death expire time to 0
    m_deathExpireTime = 0;

    // Initialize swing error message to 0
    m_swingErrorMsg = 0;

    // Initialize detection invisibility timer to 1 millisecond
    m_DetectInvTimer = 1 * IN_MILLISECONDS;

    // Initialize battleground queue IDs and invited instances
    for (int j = 0; j < PLAYER_MAX_BATTLEGROUND_QUEUES; ++j)
    {
        m_bgBattleGroundQueueID[j].bgQueueTypeId  = BATTLEGROUND_QUEUE_NONE;
        m_bgBattleGroundQueueID[j].invitedToInstance = 0;
    }

    // Set login time to current time
    m_logintime = time(NULL);
    // Set last tick time to login time
    m_Last_tick = m_logintime;
    // Initialize weapon proficiency to 0
    m_WeaponProficiency = 0;
    // Initialize armor proficiency to 0
    m_ArmorProficiency = 0;
    // Initialize parry ability to false
    m_canParry = false;
    // Initialize block ability to false
    m_canBlock = false;
    // Initialize dual wield ability to false
    m_canDualWield = false;
    m_canTitanGrip = false;
    m_ammoDPS = 0.0f;

    // m_temporaryUnsummonedPetNumber now owned by m_petMgr; initialized in its ctor.

    //////////////////// Rest System/////////////////////
    // Initialize time of entering inn to 0
    time_inn_enter = 0;
    // Initialize inn trigger ID to 0
    inn_trigger_id = 0;
    // Initialize rest bonus to 0
    m_rest_bonus = 0;
    // Initialize rest type to no rest
    rest_type = REST_TYPE_NO;
    //////////////////// Rest System/////////////////////

    // Initialize mails updated flag to false
    m_mailsUpdated = false;
    // Initialize unread mails count to 0
    unReadMails = 0;
    // Initialize next mail delivery time to 0
    m_nextMailDelivereTime = 0;

    // Initialize reset talents cost to 0
    m_resetTalentsCost = 0;
    // Initialize reset talents time to 0
    m_resetTalentsTime = 0;
    // Initialize item update queue blocked flag to false
    m_itemUpdateQueueBlocked = false;

    // Initialize forced speed changes for all move types to 0
    for (int i = 0; i < MAX_MOVE_TYPE; ++i)
    {
        m_forced_speed_changes[i] = 0;
    }

    // m_stableSlots now owned by m_petMgr; initialized in its ctor.

    /////////////////// Instance System /////////////////////
    // Initialize homebind timer to 0
    m_HomebindTimer = 0;
    // Initialize instance validity to true
    m_InstanceValid = true;
    // Initialize dungeon difficulty to normal
    m_dungeonDifficulty = DUNGEON_DIFFICULTY_NORMAL;
    m_raidDifficulty = RAID_DIFFICULTY_10MAN_NORMAL;

    m_lastPotionId = 0;

    m_activeSpec = 0;
    m_specsCount = 1;
    for (int i = 0; i < MAX_TALENT_SPEC_COUNT; ++i)
    {
        m_talentsPrimaryTree[i] = 0;
    }

    // Initialize aura base modifiers
    for (int i = 0; i < BASEMOD_END; ++i)
    {
        m_auraBaseMod[i][FLAT_MOD] = 0.0f;
        m_auraBaseMod[i][PCT_MOD] = 1.0f;
    }

    // Initialize base rating values to 0
    for (int i = 0; i < MAX_COMBAT_RATING; ++i)
    {
        m_baseRatingValue[i] = 0;
    }

    m_baseSpellPower = 0;
    m_baseHealthRegen = 0;
    m_baseManaRegen = 0;
    m_armorPenetrationPct = 0.0f;
    m_spellPenetrationItemMod = 0;

    // Honor system kill-rollover timestamp now owned by m_honorMgr; initialized in its ctor.


    // Player summoning
    // Initialize summon expire time to 0
    m_summon_expire = 0;
    // Initialize summon map ID to 0
    m_summon_mapid = 0;
    // Initialize summon coordinates to (0.0f, 0.0f, 0.0f)
    m_summon_x = 0.0f;
    m_summon_y = 0.0f;
    m_summon_z = 0.0f;

    // Initialize contested PvP timer to 0
    m_contestedPvPTimer = 0;

    // Initialize declined name to NULL
    m_declinedname = NULL;
    m_runes = NULL;

    // Initialize last fall time to 0
    m_lastFallTime = 0;
    // Initialize last fall Z coordinate to 0
    m_lastFallZ = 0;

    m_cachedGS = 0;

    m_slot = 255;
}

/**
 * @brief Destroys the player and releases owned resources.
 */
Player::~Player()
{
    // Perform cleanup before deleting the player object
    CleanupsBeforeDelete();

    // Ensure the social object is unloaded (should already be done in PlayerLogout)
    // m_social = NULL;

    // Delete all items in the player's inventory
    for (int i = 0; i < PLAYER_SLOTS_COUNT; ++i)
    {
        delete m_items[i];
    }

    // Clean up communication channels
    CleanupChannels();

    // Delete all mailed items and deallocate mail objects
    for (PlayerMails::const_iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        delete *itr;
    }

    // Delete all items in the player's item map
    for (ItemMap::const_iterator iter = mMitems.begin(); iter != mMitems.end(); ++iter)
    {
        delete iter->second; // Ensure no duplicated items to avoid crashes
    }

    // Delete the player's talk class
    delete PlayerTalkClass;

    // Remove the player from any transport they are on
    if (m_transport)
    {
        m_transport->RemovePassenger(this);
    }

    // Delete all item set effects
    for (size_t x = 0; x < ItemSetEff.size(); ++x)
    {
        delete ItemSetEff[x];
    }

    // clean up player-instance binds, may unload some instance saves
    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
        for (BoundInstancesMap::iterator itr = m_boundInstances[i].begin(); itr != m_boundInstances[i].end(); ++itr)
        {
            itr->second.state->RemovePlayer(this);
        }

    delete m_declinedname;
    delete m_runes;
}

/**
 * @brief Performs pre-destruction cleanup for trade, duel, zone, and unit state.
 */
void Player::CleanupsBeforeDelete()
{
    // Perform cleanup only if the object is fully created
    if (m_uint32Values)
    {
        // Cancel any ongoing trade
        TradeCancel(false);
        DuelComplete(DUEL_INTERRUPTED);
        // Complete any ongoing duel
    }

    // Notify zone scripts that the player is leaving the zone
    sOutdoorPvPMgr.HandlePlayerLeaveZone(this, m_zoneUpdateId);

    // Perform unit-specific cleanup
    Unit::CleanupsBeforeDelete();
}

/**
 * @brief Creates a new player character with starting data and equipment.
 *
 * @param guidlow The low GUID for the player.
 * @param name The player name.
 * @param race The race id.
 * @param class_ The class id.
 * @param gender The gender id.
 * @param skin The skin customization id.
 * @param face The face customization id.
 * @param hairStyle The hairstyle customization id.
 * @param hairColor The hair color customization id.
 * @param facialHair The facial hair customization id.
 * @param outfitId Unused outfit identifier.
 * @return true if the player was initialized successfully; otherwise, false.
 */
bool Player::Create(uint32 guidlow, const std::string& name, uint8 race, uint8 class_, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair, uint8 /*outfitId */)
{
    // FIXME: outfitId not used in player creation

    // Create the player object with the given GUID
    Object::_Create(guidlow, 0, HIGHGUID_PLAYER);

    // Set the player's name
    m_name = name;

    // Get player info based on race and class
    PlayerInfo const* info = sObjectMgr.GetPlayerInfo(race, class_);
    if (!info)
    {
        sLog.outError("Player has incorrect race/class pair. Can't be loaded.");
        return false;
    }

    // Get class entry from DBC
    ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(class_);
    if (!cEntry)
    {
        sLog.outError("Class %u not found in DBC (Wrong DBC files?)", class_);
        return false;
    }

    // Validate gender
    if (gender != uint8(GENDER_MALE) && gender != uint8(GENDER_FEMALE))
    {
        sLog.outError("Invalid gender %u at player creation", uint32(gender));
        return false;
    }

    // Initialize player items to NULL
    for (int i = 0; i < PLAYER_SLOTS_COUNT; ++i)
    {
        m_items[i] = NULL;
    }

    // Set player's initial location
    SetLocationMapId(info->mapId);
    Relocate(info->positionX, info->positionY, info->positionZ, info->orientation);

    // Set the player's map
    SetMap(sMapMgr.CreateMap(info->mapId, this));

    // Set player's power type based on class
    uint8 powertype = cEntry->powerType;

    // Set player's faction based on race
    setFactionForRace(race);

    // Set player's race, class, gender, and power type
    SetByteValue(UNIT_FIELD_BYTES_0, 0, race);
    SetByteValue(UNIT_FIELD_BYTES_0, 1, class_);
    SetByteValue(UNIT_FIELD_BYTES_0, 2, gender);
    SetByteValue(UNIT_FIELD_BYTES_0, 3, powertype);

    // Initialize player's display IDs (model, scale, and model data)
    InitDisplayIds();

    SetByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP);
    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);               // fix cast time showed in spell tooltip on client
    SetFloatValue(UNIT_FIELD_HOVERHEIGHT, 1.0f);            // default for players in 3.0.3

    // Set default watched faction index
    SetInt32Value(PLAYER_FIELD_WATCHED_FACTION_INDEX, -1); // -1 is default value

    // Set player's appearance (skin, face, hair style, hair color, facial hair)
    SetByteValue(PLAYER_BYTES, 0, skin);
    SetByteValue(PLAYER_BYTES, 1, face);
    SetByteValue(PLAYER_BYTES, 2, hairStyle);
    SetByteValue(PLAYER_BYTES, 3, hairColor);
    SetByteValue(PLAYER_BYTES_2, 0, facialHair);
    SetByteValue(PLAYER_BYTES_2, 3, REST_STATE_NORMAL);

    SetByteValue(PLAYER_BYTES_3, 0, gender);
    SetByteValue(PLAYER_BYTES_3, 3, 0);                     // BattlefieldArenaFaction (0 or 1)

    SetInGuild(0);
    SetGuildLevel(0);
    // Initialize player's guild information
    SetUInt32Value(PLAYER_GUILDRANK, 0);
    SetUInt32Value(PLAYER_GUILD_TIMESTAMP, 0);

    for (int i = 0; i < KNOWN_TITLES_SIZE; ++i)
    {
        SetUInt64Value(PLAYER__FIELD_KNOWN_TITLES + i, 0);  // 0=disabled
    }
    SetUInt32Value(PLAYER_CHOSEN_TITLE, 0);

    // Initialize player's kill counts and contributions
    SetUInt32Value(PLAYER_FIELD_KILLS, 0);
    SetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS, 0);

    // set starting level
    uint32 start_level = getClass() != CLASS_DEATH_KNIGHT
                         ? sWorld.getConfig(CONFIG_UINT32_START_PLAYER_LEVEL)
                         : sWorld.getConfig(CONFIG_UINT32_START_HEROIC_PLAYER_LEVEL);

    if (GetSession()->GetSecurity() >= SEC_MODERATOR)
    {
        uint32 gm_level = sWorld.getConfig(CONFIG_UINT32_START_GM_LEVEL);
        if (gm_level > start_level)
        {
            start_level = gm_level;
        }
    }

    SetUInt32Value(UNIT_FIELD_LEVEL, start_level);

    InitRunes();

    SetUInt64Value(PLAYER_FIELD_COINAGE, sWorld.getConfig(CONFIG_UINT64_START_PLAYER_MONEY));
    SetCurrencyCount(CURRENCY_HONOR_POINTS,sWorld.getConfig(CONFIG_UINT32_CURRENCY_START_HONOR_POINTS));
    SetCurrencyCount(CURRENCY_CONQUEST_POINTS, sWorld.getConfig(CONFIG_UINT32_CURRENCY_START_CONQUEST_POINTS));

    // Initialize played time
    m_Last_tick = time(NULL);
    m_Played_time[PLAYED_TIME_TOTAL] = 0;
    m_Played_time[PLAYED_TIME_LEVEL] = 0;

    // Initialize base stats and related field values
    InitStatsForLevel();
    InitTaxiNodesForLevel();
    InitGlyphsForLevel();
    InitTalentForLevel();
    InitPrimaryProfessions(); // To max set before any spell added

    // Apply original stats mods before spell loading or item equipment
    UpdateMaxHealth(); // Update max Health (for add bonus from stamina)
    SetHealth(GetMaxHealth());

    if (GetPowerType() == POWER_MANA)
    {
        UpdateMaxPower(POWER_MANA); // Update max Mana (for add bonus from intellect)
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }

    if (GetPowerType() != POWER_MANA)                       // hide additional mana bar if we have no mana
    {
        SetPower(POWER_MANA, 0);
        SetMaxPower(POWER_MANA, 0);
    }

    // original spells
    learnDefaultSpells();

    // Initialize action bar with default actions
    for (PlayerCreateInfoActions::const_iterator action_itr = info->action.begin(); action_itr != info->action.end(); ++action_itr)
    {
        addActionButton(0, action_itr->button, action_itr->action, action_itr->type);
    }

    // Initialize player's starting items
    uint32 raceClassGender = GetUInt32Value(UNIT_FIELD_BYTES_0) & 0x00FFFFFF;

    CharStartOutfitEntry const* oEntry = NULL;
    for (uint32 i = 1; i < sCharStartOutfitStore.GetNumRows(); ++i)
    {
        if (CharStartOutfitEntry const* entry = sCharStartOutfitStore.LookupEntry(i))
        {
            if (entry->RaceClassGender == raceClassGender)
            {
                oEntry = entry;
                break;
            }
        }
    }

    if (oEntry)
    {
        for (int j = 0; j < MAX_OUTFIT_ITEMS; ++j)
        {
            if (oEntry->ItemId[j] <= 0)
            {
                continue;
            }

            uint32 item_id = oEntry->ItemId[j];

            // Just skip, reported in ObjectMgr::LoadItemPrototypes
            ItemPrototype const* iProto = ObjectMgr::GetItemPrototype(item_id);
            if (!iProto)
            {
                continue;
            }

            // BuyCount by default
            int32 count = iProto->BuyCount;

            // Special amount for food/drink
            if (iProto->Class == ITEM_CLASS_CONSUMABLE && iProto->SubClass == ITEM_SUBCLASS_FOOD)
            {
                switch (iProto->Spells[0].SpellCategory)
                {
                    case 11:                                // food
                        count = getClass() == CLASS_DEATH_KNIGHT ? 10 : 4;
                        break;
                    case 59:                                // drink
                        count = 2;
                        break;
                }
                if (iProto->Stackable < count)
                {
                    count = iProto->Stackable;
                }
            }

            StoreNewItemInBestSlots(item_id, count);
        }
    }

    for (PlayerCreateInfoItems::const_iterator item_id_itr = info->item.begin(); item_id_itr != info->item.end(); ++item_id_itr)
    {
        StoreNewItemInBestSlots(item_id_itr->item_id, item_id_itr->item_amount);
    }

    // Equip bags and main-hand weapon
    // Second pass for not equipped items (offhand weapon/shield if it attempted to equip before main-hand weapon)
    // or ammo not equipped in special bag
    for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            uint16 eDest;
            // Equip offhand weapon/shield if it attempted to equip before main-hand weapon
            InventoryResult msg = CanEquipItem(NULL_SLOT, eDest, pItem, false);
            if (msg == EQUIP_ERR_OK)
            {
                RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                EquipItem(eDest, pItem, true);
            }
            // Move other items to more appropriate slots (ammo not equipped in special bag)
            else
            {
                ItemPosCountVec sDest;
                msg = CanStoreItem(NULL_BAG, NULL_SLOT, sDest, pItem, false);
                if (msg == EQUIP_ERR_OK)
                {
                    RemoveItem(INVENTORY_SLOT_BAG_0, i, true);
                    pItem = StoreItem(sDest, pItem, true);
                }

                // If this is ammo then use it
                msg = CanUseAmmo(pItem->GetEntry());
                if (msg == EQUIP_ERR_OK)
                {
                    SetAmmo(pItem->GetEntry());
                }
            }
        }
    }
    // All item positions resolved

    if (info->phaseMap != 0)
    {
        CharacterDatabase.PExecute("REPLACE INTO `character_phase_data` (`guid`, `map`) VALUES (%u, %u)", guidlow, info->phaseMap);
    }

    return true;
}

/**
 * @brief Equips or stores a newly created item in the best available slots.
 *
 * @param titem_id The item entry id.
 * @param titem_amount The amount to create.
 * @return true if all remaining items were equipped or stored; otherwise, false.
 */
bool Player::StoreNewItemInBestSlots(uint32 titem_id, uint32 titem_amount)
{
    DEBUG_LOG("STORAGE: Creating initial item, itemId = %u, count = %u", titem_id, titem_amount);

    // Attempt to equip the item one by one
    while (titem_amount > 0)
    {
        uint16 eDest;
        uint8 msg = CanEquipNewItem(NULL_SLOT, eDest, titem_id, false);
        if (msg != EQUIP_ERR_OK)
        {
            break;
        }

        // Equip the new item
        EquipNewItem(eDest, titem_id, true);
        AutoUnequipOffhandIfNeed();
        --titem_amount;
    }

    if (titem_amount == 0)
    {
        return true; // All items equipped
    }

    // Attempt to store the remaining items
    ItemPosCountVec sDest;
    // Store in the main bag to simplify the second pass (special bags may not be equipped yet)
    uint8 msg = CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, sDest, titem_id, titem_amount);
    if (msg == EQUIP_ERR_OK)
    {
        StoreNewItem(sDest, titem_id, true, Item::GenerateItemRandomPropertyId(titem_id));
        return true; // Items stored
    }

    // Item cannot be added
    sLog.outError("STORAGE: Can't equip or store initial item %u for race %u class %u , error msg = %u", titem_id, getRace(), getClass(), msg);
    return false;
}

// Helper function, mainly for script side, but can be used for simple tasks in MaNGOS as well
Item* Player::StoreNewItemInInventorySlot(uint32 itemEntry, uint32 amount)
{
    ItemPosCountVec vDest;

    uint8 msg = CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, vDest, itemEntry, amount);

    if (msg == EQUIP_ERR_OK)
    {
        if (Item* pItem = StoreNewItem(vDest, itemEntry, true, Item::GenerateItemRandomPropertyId(itemEntry)))
        {
            return pItem;
        }
    }

    return NULL;
}


/**
 * @brief Updates player state, timers, combat, saving, and delayed actions.
 *
 * @param update_diff The elapsed update time in milliseconds.
 * @param p_time The elapsed update time used by some player timers.
 */
void Player::Update(uint32 update_diff, uint32 p_time)
{
    // If the player is not in the world, return early
    if (!IsInWorld())
    {
        return;
    }

    // Remove failed timed Achievements
    GetAchievementMgr().DoFailedTimedAchievementCriterias();

    // Undelivered mail
    if (m_nextMailDelivereTime && m_nextMailDelivereTime <= time(NULL))
    {
        SendNewMail();
        ++unReadMails;

        // It will be recalculate at mailbox open (for unReadMails important non-0 until mailbox open, it also will be recalculated)
        m_nextMailDelivereTime = 0;
    }

    // Used to implement delayed far teleports
    SetCanDelayTeleport(true);
    Unit::Update(update_diff, p_time);
    SetCanDelayTeleport(false);

    // Update player-only attacks
    if (uint32 ranged_att = getAttackTimer(RANGED_ATTACK))
    {
        setAttackTimer(RANGED_ATTACK, (update_diff >= ranged_att ? 0 : ranged_att - update_diff));
    }

    time_t now = time(NULL);

    // Update PvP flag
    UpdatePvPFlag(now);

    // Update contested PvP state
    UpdateContestedPvP(update_diff);

    // Update duel flag
    UpdateDuelFlag(now);

    // Check duel distance
    CheckDuelDistance(now);

    // Update AFK report
    UpdateAfkReport(now);

    // Update items with limited lifetime
    if (now > m_Last_tick)
    {
        UpdateItemDuration(uint32(now - m_Last_tick));
    }

    // Update timed quests
    if (!m_timedquests.empty())
    {
        QuestSet::iterator iter = m_timedquests.begin();
        while (iter != m_timedquests.end())
        {
            QuestStatusData& q_status = mQuestStatus[*iter];
            if (q_status.m_timer <= update_diff)
            {
                uint32 quest_id  = *iter;
                ++iter; // Current iter will be removed in FailQuest
                FailQuest(quest_id);
            }
            else
            {
                q_status.m_timer -= update_diff;
                if (q_status.uState != QUEST_NEW)
                {
                    q_status.uState = QUEST_CHANGED;
                }
                ++iter;
            }
        }
    }

    // Update melee attacking state
    if (hasUnitState(UNIT_STAT_MELEE_ATTACKING))
    {
        UpdateMeleeAttackingState();

        Unit* pVictim = getVictim();
        if (pVictim && !IsNonMeleeSpellCasted(false))
        {
            Player* vOwner = pVictim->GetCharmerOrOwnerPlayerOrPlayerItself();
            if (vOwner && vOwner->IsPvP() && !IsInDuelWith(vOwner))
            {
                UpdatePvP(true);
                RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
            }
        }
    }

    // Speed collect rest bonus (section/in hour)
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING))
    {
        if (GetTimeInnEnter() > 0) // Freeze update
        {
            time_t time_inn = now - GetTimeInnEnter();
            if (time_inn >= 10) // Freeze update
            {
                SetRestBonus(GetRestBonus() + ComputeRest(time_inn));
                UpdateInnerTime(now);
            }
        }
    }

    // Update regeneration timer
    if (m_regenTimer)
    {
        if (update_diff >= m_regenTimer)
        {
            m_regenTimer = 0;
        }
        else
        {
            m_regenTimer -= update_diff;
        }
    }

    // Update position status timer
    if (m_positionStatusUpdateTimer)
    {
        if (update_diff >= m_positionStatusUpdateTimer)
        {
            m_positionStatusUpdateTimer = 0;
        }
        else
        {
            m_positionStatusUpdateTimer -= update_diff;
        }
    }

    // Update weapon change timer
    if (m_weaponChangeTimer > 0)
    {
        if (update_diff >= m_weaponChangeTimer)
        {
            m_weaponChangeTimer = 0;
        }
        else
        {
            m_weaponChangeTimer -= update_diff;
        }
    }

    // Update zone timer
    if (m_zoneUpdateTimer > 0)
    {
        if (update_diff >= m_zoneUpdateTimer)
        {
            uint32 newzone, newarea;
            GetZoneAndAreaId(newzone, newarea);

            if (m_zoneUpdateId != newzone)
            {
                UpdateZone(newzone, newarea); // Also update area
            }
            else
            {
                // Use area updates as well
                // Needed for free-for-all arenas, for example
                if (m_areaUpdateId != newarea)
                {
                    UpdateArea(newarea);
                }

                m_zoneUpdateTimer = ZONE_UPDATE_INTERVAL;
            }
        }
        else
        {
            m_zoneUpdateTimer -= update_diff;
        }
    }

    // Update time sync timer
    if (m_timeSyncTimer > 0)
    {
        if (update_diff >= m_timeSyncTimer)
        {
            SendTimeSync();
        }
        else
        {
            m_timeSyncTimer -= update_diff;
        }
    }

    // Regenerate all if the player is alive
    if (IsAlive())
    {
        if (!HasAuraType(SPELL_AURA_STOP_NATURAL_MANA_REGEN))
        {
            SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER);
        }

        if (!m_regenTimer)
        {
            RegenerateAll();
        }
    }


    // Handle player death
    if (m_deathState == JUST_DIED)
    {
        KillPlayer();
    }

    // Handle periodic saving
    if (m_nextSave > 0)
    {
        if (update_diff >= m_nextSave)
        {
            // m_nextSave reset in SaveToDB call
            // Used by Eluna
#ifdef ENABLE_ELUNA
            if (Eluna* e = GetEluna())
            {
                e->OnSave(this);
            }
#endif /* ENABLE_ELUNA */
            SaveToDB();
            DETAIL_LOG("Player '%s' (GUID: %u) saved", GetName(), GetGUIDLow());
        }
        else
        {
            m_nextSave -= update_diff;
        }
    }

    // Handle water/drowning
    HandleDrowning(update_diff);

    // Handle detect stealth players
    if (m_DetectInvTimer > 0)
    {
        if (update_diff >= m_DetectInvTimer)
        {
            HandleStealthedUnitsDetection();
            m_DetectInvTimer = 3000;
        }
        else
        {
            m_DetectInvTimer -= update_diff;
        }
    }

    // Update played time
    if (now > m_Last_tick)
    {
        uint32 elapsed = uint32(now - m_Last_tick);
        m_Played_time[PLAYED_TIME_TOTAL] += elapsed; // Total played time
        m_Played_time[PLAYED_TIME_LEVEL] += elapsed; // Level played time
        m_Last_tick = now;
    }

    if (GetDrunkValue())
    {
        m_drunkTimer += update_diff;

        if (m_drunkTimer > 9 * IN_MILLISECONDS)
        {
            HandleSobering();
        }
    }

    // Not auto-free ghost from body in instances; also check for resurrection prevention
    if (m_deathTimer > 0  && !GetMap()->Instanceable() && !HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
    {
        if (p_time >= m_deathTimer)
        {
            m_deathTimer = 0;
            BuildPlayerRepop();
            RepopAtGraveyard();
        }
        else
        {
            m_deathTimer -= p_time;
        }
    }

    // Update enchant time
    UpdateEnchantTime(update_diff);

    // Update homebind time
    UpdateHomebindTime(update_diff);

    // Group update
    SendUpdateToOutOfRangeGroupMembers();

    // Handle pet unsummoning if out of range
    Pet* pet = GetPet();
    if (pet && !pet->IsWithinDistInMap(this, GetMap()->GetVisibilityDistance()) && (GetCharmGuid() && (pet->GetObjectGuid() != GetCharmGuid())))
    {
        pet->Unsummon(PET_SAVE_REAGENTS, this);
    }

    // Handle delayed teleport
    if (IsHasDelayedTeleport())
    {
        TeleportTo(m_teleport_dest, m_teleport_options);
    }
}

/**
 * @brief Changes the player's death state and handles player-specific death logic.
 *
 * @param s The new death state.
 */
void Player::SetDeathState(DeathState s)
{
    uint32 ressSpellId = 0;

    bool cur = IsAlive();

    if (s == JUST_DIED && cur)
    {
        // drunken state is cleared on death
        SetDrunkValue(0);
        // lost combo points at any target (targeted combo points clear in Unit::SetDeathState)
        ClearComboPoints();

        clearResurrectRequestData();

        // remove form before other mods to prevent incorrect stats calculation
        RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);

        // FIXME: is pet dismissed at dying or releasing spirit? if second, add SetDeathState(DEAD) to HandleRepopRequestOpcode and define pet unsummon here with (s == DEAD)
        RemovePet(PET_SAVE_REAGENTS);

        // save value before aura remove in Unit::SetDeathState
        ressSpellId = GetUInt32Value(PLAYER_SELF_RES_SPELL);

        // passive spell
        if (!ressSpellId)
        {
            ressSpellId = GetResurrectionSpellId();
        }

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP, 1);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATH, 1);
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON, 1);

        if (InstanceData* mapInstance = GetInstanceData())
        {
            mapInstance->OnPlayerDeath(this);
        }
    }

    Unit::SetDeathState(s);

    // restore resurrection spell id for player after aura remove
    if (s == JUST_DIED && cur && ressSpellId)
    {
        SetUInt32Value(PLAYER_SELF_RES_SPELL, ressSpellId);
    }

    if (IsAlive() && !cur)
    {
        // clear aura case after resurrection by another way (spells will be applied before next death)
        SetUInt32Value(PLAYER_SELF_RES_SPELL, 0);

        // restore default warrior stance
        if (getClass() == CLASS_WARRIOR)
        {
            CastSpell(this, SPELL_ID_PASSIVE_BATTLE_STANCE, true);
        }
    }
}

/**
 * @brief Builds character-enumeration data for the character selection screen.
 *
 * @param result The database row for the character.
 * @param data The packet being populated.
 * @param buffer Secondary byte buffer for trailing variable-length fields.
 * @return true if the enum data was built successfully; otherwise, false.
 */
bool Player::BuildEnumData(QueryResult* result, ByteBuffer* data, ByteBuffer* buffer)
{
    //             0               1                2                3                 4                  5                       6                        7
    //    "SELECT characters.guid, characters.name, characters.race, characters.class, characters.gender, characters.playerBytes, characters.playerBytes2, characters.level, "
    //     8                9               10                     11                     12                     13                    14
    //    "characters.zone, characters.map, characters.position_x, characters.position_y, characters.position_z, guild_member.guildid, characters.playerFlags, "
    //    15                    16                   17                     18                   19                         20               21
    //    "characters.at_login, character_pet.entry, character_pet.modelid, character_pet.level, characters.equipmentCache, characters.slot, character_declinedname.genitive "

    Field* fields = result->Fetch();
    ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, fields[0].GetUInt32());
    uint8 pRace = fields[2].GetUInt8();
    uint8 pClass = fields[3].GetUInt8();

    PlayerInfo const* info = sObjectMgr.GetPlayerInfo(pRace, pClass);
    if (!info)
    {
        sLog.outError("%s has incorrect race/class pair. Don't build enum.", guid.GetString().c_str());
        return false;
    }

    ObjectGuid guildGuid = ObjectGuid(HIGHGUID_GUILD, fields[13].GetUInt32());
    std::string name = fields[1].GetCppString();
    uint8 gender = fields[4].GetUInt8();
    uint32 playerBytes = fields[5].GetUInt32();
    uint8 level = fields[7].GetUInt8();
    uint32 playerFlags = fields[14].GetUInt32();
    uint32 atLoginFlags = fields[15].GetUInt32();
    uint32 zone = fields[8].GetUInt32();
    uint32 petDisplayId = 0;
    uint32 petLevel   = 0;
    uint32 petFamily  = 0;
    uint32 char_flags = 0;

    data->WriteGuidMask<3>(guid);
    data->WriteGuidMask<1, 7, 2>(guildGuid);
    data->WriteBits(name.length(), 7);
    data->WriteGuidMask<4, 7>(guid);
    data->WriteGuidMask<3>(guildGuid);
    data->WriteGuidMask<5>(guid);
    data->WriteGuidMask<6>(guildGuid);
    data->WriteGuidMask<1>(guid);
    data->WriteGuidMask<5, 4>(guildGuid);
    data->WriteBit(atLoginFlags & AT_LOGIN_FIRST);
    data->WriteGuidMask<0, 2, 6>(guid);
    data->WriteGuidMask<0>(guildGuid);

    // show pet at selection character in character list only for non-ghost character
    if (result && !(playerFlags & PLAYER_FLAGS_GHOST) && (pClass == CLASS_WARLOCK || pClass == CLASS_HUNTER || pClass == CLASS_DEATH_KNIGHT))
    {
        uint32 entry = fields[16].GetUInt32();
        CreatureInfo const* cInfo = sCreatureStorage.LookupEntry<CreatureInfo>(entry);
        if (cInfo)
        {
            petDisplayId = fields[17].GetUInt32();
            petLevel     = fields[18].GetUInt32();
            petFamily    = cInfo->Family;
        }
    }

    if (playerFlags & PLAYER_FLAGS_HIDE_HELM)
    {
        char_flags |= CHARACTER_FLAG_HIDE_HELM;
    }
    if (playerFlags & PLAYER_FLAGS_HIDE_CLOAK)
    {
        char_flags |= CHARACTER_FLAG_HIDE_CLOAK;
    }
    if (playerFlags & PLAYER_FLAGS_GHOST)
    {
        char_flags |= CHARACTER_FLAG_GHOST;
    }
    if (atLoginFlags & AT_LOGIN_RENAME)
    {
        char_flags |= CHARACTER_FLAG_RENAME;
    }
    if (sWorld.getConfig(CONFIG_BOOL_DECLINED_NAMES_USED))
    {
        if (!fields[21].GetCppString().empty())
        {
            char_flags |= CHARACTER_FLAG_DECLINED;
        }
    }
    else
    {
        char_flags |= CHARACTER_FLAG_DECLINED;
    }

    *buffer << uint8(pClass);                                // class

    Tokens tdata = StrSplit(fields[19].GetCppString(), " ");
    for (uint8 slot = 0; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        uint32 visualbase = slot * 2;                       // entry, perm ench., temp ench.
        uint32 item_id = GetUInt32ValueFromArray(tdata, visualbase);
        const ItemPrototype* proto = ObjectMgr::GetItemPrototype(item_id);
        if (!proto)
        {
            *buffer << uint8(0);
            *buffer << uint32(0);
            *buffer << uint32(0);
            continue;
        }

        SpellItemEnchantmentEntry const* enchant = NULL;

        uint32 enchants = GetUInt32ValueFromArray(tdata, visualbase + 1);
        for (uint8 enchantSlot = PERM_ENCHANTMENT_SLOT; enchantSlot <= TEMP_ENCHANTMENT_SLOT; ++enchantSlot)
        {
            // values stored in 2 uint16
            uint32 enchantId = 0x0000FFFF & (enchants >> enchantSlot * 16);
            if (!enchantId)
            {
                continue;
            }

            if ((enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId)))
            {
                break;
            }
        }

        *buffer << uint8(proto->InventoryType);
        *buffer << uint32(proto->DisplayInfoID);
        *buffer << uint32(enchant ? enchant->aura_id : 0);
    }

    for (int32 i = 0; i < 4; i++)
    {
        *buffer << uint8(0);
        *buffer << uint32(0);
        *buffer << uint32(0);
    }

    *buffer << uint32(petFamily);                           // Pet DisplayID
    buffer->WriteGuidBytes<2>(guildGuid);
    *buffer << uint8(fields[20].GetUInt8());                // char order id
    *buffer << uint8((playerBytes >> 16) & 0xFF);           // Hair style
    buffer->WriteGuidBytes<3>(guildGuid);
    *buffer << uint32(petDisplayId);                        // Pet DisplayID
    *buffer << uint32(char_flags);                          // character flags
    *buffer << uint8((playerBytes >> 24) & 0xFF);           // Hair color
    buffer->WriteGuidBytes<4>(guid);
    *buffer << uint32(fields[9].GetUInt32());               // map
    buffer->WriteGuidBytes<5>(guildGuid);
    *buffer << fields[12].GetFloat();                       // z
    buffer->WriteGuidBytes<6>(guildGuid);
    *buffer << uint32(petLevel);                            // pet level
    buffer->WriteGuidBytes<3>(guid);
    *buffer << fields[11].GetFloat();                       // y
    // character customize flags
    *buffer << uint32(atLoginFlags & AT_LOGIN_CUSTOMIZE ? CHAR_CUSTOMIZE_FLAG_CUSTOMIZE : CHAR_CUSTOMIZE_FLAG_NONE);

    uint32 playerBytes2 = fields[6].GetUInt32();
    *buffer << uint8(playerBytes2 & 0xFF);                  // facial hair
    buffer->WriteGuidBytes<7>(guid);
    *buffer << uint8(gender);                               // Gender
    buffer->append(name.c_str(), name.length());
    *buffer << uint8((playerBytes >> 8) & 0xFF);            // face

    buffer->WriteGuidBytes<0, 2>(guid);
    buffer->WriteGuidBytes<1, 7>(guildGuid);

    *buffer << fields[10].GetFloat();                       // x
    *buffer << uint8(playerBytes & 0xFF);                   // skin
    *buffer << uint8(pRace);                                // Race
    *buffer << uint8(level);                                // Level
    buffer->WriteGuidBytes<6>(guid);
    buffer->WriteGuidBytes<4, 0>(guildGuid);
    buffer->WriteGuidBytes<5, 1>(guid);

    *buffer << uint32(zone);                                // Zone id

    return true;
}

/**
 * @brief Toggles AFK status and leaves battlegrounds when entering AFK.
 */
void Player::ToggleAFK()
{
    ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK);

    // afk player not allowed in battleground
    if (isAFK() && InBattleGround() && !InArena())
    {
        LeaveBattleground();
    }
}

/**
 * @brief Toggles DND status.
 */
void Player::ToggleDND()
{
    ToggleFlag(PLAYER_FLAGS, PLAYER_FLAGS_DND);
}

/**
 * @brief Gets the active chat tag displayed for the player.
 *
 * @return The chat tag flags for GM, AFK, DND, or none.
 */
ChatTagFlags Player::GetChatTag() const
{
    ChatTagFlags tag = CHAT_TAG_NONE;

    if (isAFK())
    {
        tag |= CHAT_TAG_AFK;
    }
    if (isDND())
    {
        tag |= CHAT_TAG_DND;
    }
    if (isGMChat())
    {
        tag |= CHAT_TAG_GM;
    }
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_COMMENTATOR))
    {
        tag |= CHAT_TAG_COM;
    }
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_DEVELOPER))
    {
        tag |= CHAT_TAG_DEV;
    }

    return tag;
}

void Player::SendTeleportPacket(float oldX, float oldY, float oldZ, float oldO)
{
    ObjectGuid guid = GetObjectGuid();
    ObjectGuid transportGuid = m_movementInfo.GetTransportGuid();

    WorldPacket data(SMSG_MOVE_TELEPORT, 38);
    data.WriteGuidMask<6, 0, 3, 2>(guid);
    data.WriteBit(0);       // unknown
    data.WriteBit(!transportGuid.IsEmpty());
    data.WriteGuidMask<1>(guid);
    if (transportGuid)
    {
        data.WriteGuidMask<1, 3, 2, 5, 0, 7, 6, 4>(transportGuid);
    }

    data.WriteGuidMask<4, 7, 5>(guid);

    if (transportGuid)
    {
        data.WriteGuidBytes<5, 6, 1, 7, 0, 2, 4, 3>(transportGuid);
    }

    data << uint32(0);  // counter
    data.WriteGuidBytes<1, 2, 3, 5>(guid);
    data << float(GetPositionX());
    data.WriteGuidBytes<4>(guid);
    data << float(GetOrientation());
    data.WriteGuidBytes<7>(guid);
    data << float(GetPositionZ());
    data.WriteGuidBytes<0, 6>(guid);
    data << float(GetPositionY());

    Relocate(oldX, oldY, oldZ, oldO);
    SendDirectMessage(&data);
}

/**
 * @brief Teleports the player to a target location, handling near and far cases.
 *
 * @param mapid The destination map id.
 * @param x The destination x coordinate.
 * @param y The destination y coordinate.
 * @param z The destination z coordinate.
 * @param orientation The destination orientation.
 * @param options Teleport option flags.
 * @param at Optional area trigger that initiated the teleport.
 * @return true if teleport setup succeeded; otherwise, false.
 */
bool Player::TeleportTo(uint32 mapid, float x, float y, float z, float orientation, uint32 options /*=0*/, AreaTrigger const* at /*=NULL*/)
{
    orientation = NormalizeOrientation(orientation);

    if (!MapManager::IsValidMapCoord(mapid, x, y, z, orientation))
    {
        sLog.outError("TeleportTo: invalid map %d or absent instance template.", mapid);
        return false;
    }

    MapEntry const* mEntry = sMapStore.LookupEntry(mapid);  // Validity checked in IsValidMapCoord

    if (!isGameMaster() && DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP, mapid, this))
    {
        sLog.outDebug("Player (GUID: %u, name: %s) tried to enter a forbidden map %u", GetGUIDLow(), GetName(), mapid);
        SendTransferAbortedByLockStatus(mEntry, AREA_LOCKSTATUS_NOT_ALLOWED);
        return false;
    }

    // preparing unsummon pet if lost (we must get pet before teleportation or will not find it later)
    Pet* pet = GetPet();

    // don't let enter battlegrounds without assigned battleground id (for example through areatrigger)...
    // don't let gm level > 1 either
    if (!InBattleGround() && mEntry->IsBattleGroundOrArena())
    {
        return false;
    }

    // Get MapEntrance trigger if teleport to other -nonBG- map
    bool assignedAreaTrigger = false;
    if (GetMapId() != mapid && !mEntry->IsBattleGroundOrArena() && !at)
    {
        at = sObjectMgr.GetMapEntranceTrigger(mapid);
        assignedAreaTrigger = true;
    }

    // Check requirements for teleport
    if (at)
    {
        uint32 miscRequirement = 0;
        AreaLockStatus lockStatus = GetAreaTriggerLockStatus(at, GetDifficulty(mEntry->IsRaid()), miscRequirement);
        if (lockStatus != AREA_LOCKSTATUS_OK)
        {
            // Teleport not requested by area-trigger
            // TODO - Assume a player with expansion 0 travels from BootyBay to Ratched, and he is attempted to be teleported to outlands
            //        then he will repop near BootyBay instead of normally continuing his journey
            // This code is probably added to catch passengers on ships to northrend who shouldn't go there
            if (lockStatus == AREA_LOCKSTATUS_INSUFFICIENT_EXPANSION && !assignedAreaTrigger && GetTransport())
            {
                RepopAtGraveyard();                         // Teleport to near graveyard if on transport, looks blizz like :)
            }

            SendTransferAbortedByLockStatus(mEntry, lockStatus, miscRequirement);
            return false;
        }
    }

    if (Group* grp = GetGroup())                            // TODO: Verify that this is correct place
    {
        grp->SetPlayerMap(GetObjectGuid(), mapid);
    }

    // if we were on a transport, leave
    if (!(options & TELE_TO_NOT_LEAVE_TRANSPORT) && m_transport)
    {
        m_transport->RemovePassenger(this);
        m_transport = NULL;
        m_movementInfo.ClearTransportData();
    }

    // The player was ported to another map and looses the duel immediately.
    // We have to perform this check before the teleport, otherwise the
    // ObjectAccessor won't find the flag.
    if (duel && GetMapId() != mapid)
        if (GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER)))
        {
            DuelComplete(DUEL_FLED);
        }

    // reset movement flags at teleport, because player will continue move with these flags after teleport
    m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
    DisableSpline();

    if ((GetMapId() == mapid) && (!m_transport))            // TODO the !m_transport might have unexpected effects when teleporting from transport to other place on same map
    {
        // lets reset far teleport flag if it wasn't reset during chained teleports
        SetSemaphoreTeleportFar(false);
        // setup delayed teleport flag
        // if teleport spell is casted in Unit::Update() func
        // then we need to delay it until update process will be finished
        if (SetDelayedTeleportFlagIfCan())
        {
            SetSemaphoreTeleportNear(true);
            // lets save teleport destination for player
            m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
            m_teleport_options = options;
            return true;
        }

        if (!(options & TELE_TO_NOT_UNSUMMON_PET))
        {
            // same map, only remove pet if out of range for new position
            if (pet && !pet->IsWithinDist3d(x, y, z, GetMap()->GetVisibilityDistance()))
            {
                UnsummonPetTemporaryIfAny();
            }
        }

        if (!(options & TELE_TO_NOT_LEAVE_COMBAT))
        {
            CombatStop();
        }

        // this will be used instead of the current location in SaveToDB
        m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
        SetFallInformation(0, z);

        // code for finish transfer called in WorldSession::HandleMovementOpcodes()
        // at client packet CMSG_MOVE_TELEPORT_ACK
        SetSemaphoreTeleportNear(true);
        // near teleport, triggering send CMSG_MOVE_TELEPORT_ACK from client at landing
        if (!GetSession()->PlayerLogout())
        {
            float oldX, oldY, oldZ;
            float oldO = GetOrientation();
            GetPosition(oldX, oldY, oldZ);;
            Relocate(x, y, z, orientation);
            SendTeleportPacket(oldX, oldY, oldZ, oldO);
        }
    }
    else
    {
        // far teleport to another map
        Map* oldmap = IsInWorld() ? GetMap() : NULL;
        // check if we can enter before stopping combat / removing pet / totems / interrupting spells

        // If the map is not created, assume it is possible to enter it.
        // It will be created in the WorldPortAck.
        DungeonPersistentState* state = GetBoundInstanceSaveForSelfOrGroup(mapid);
        Map* map = sMapMgr.FindMap(mapid, state ? state->GetInstanceId() : 0);
        if (!map || map->CanEnter(this))
        {
            // lets reset near teleport flag if it wasn't reset during chained teleports
            SetSemaphoreTeleportNear(false);
            // setup delayed teleport flag
            // if teleport spell is casted in Unit::Update() func
            // then we need to delay it until update process will be finished
            if (SetDelayedTeleportFlagIfCan())
            {
                SetSemaphoreTeleportFar(true);
                // lets save teleport destination for player
                m_teleport_dest = WorldLocation(mapid, x, y, z, orientation);
                m_teleport_options = options;
                return true;
            }

            SetSelectionGuid(ObjectGuid());

            CombatStop();

            ResetContestedPvP();

            // remove player from battleground on far teleport (when changing maps)
            if (BattleGround const* bg = GetBattleGround())
            {
                // Note: at battleground join battleground id set before teleport
                // and we already will found "current" battleground
                // just need check that this is targeted map or leave
                if (bg->GetMapId() != mapid)
                {
                    LeaveBattleground(false); // don't teleport to entry point
                }
            }

            // remove pet on map change
            if (pet)
            {
                UnsummonPetTemporaryIfAny();
            }

            // remove vehicle accessories on map change
            if (IsVehicle())
            {
                GetVehicleInfo()->RemoveAccessoriesFromMap();
            }

            // remove all dyn objects
            RemoveAllDynObjects();

            // stop spellcasting
            // not attempt interrupt teleportation spell at caster teleport
            if (!(options & TELE_TO_SPELL))
                if (IsNonMeleeSpellCasted(true))
                {
                    InterruptNonMeleeSpells(true);
                }

            // remove auras before removing from map...
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_CHANGE_MAP | AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING);

            if (!GetSession()->PlayerLogout())
            {
                // send transfer packet to display load screen
                WorldPacket data(SMSG_TRANSFER_PENDING, (4 + 4 + 4));
                data.WriteBit(0);       // unknown
                if (m_transport)
                {
                    data.WriteBit(1);   // has transport
                    data << uint32(GetMapId());
                    data << uint32(m_transport->GetEntry());
                }
                else
                {
                    data.WriteBit(0);   // has transport
                }

                data << uint32(mapid);
                GetSession()->SendPacket(&data);
            }

            // remove from old map now
            if (oldmap)
            {
                oldmap->Remove(this, false);
            }

            // new final coordinates
            float final_x = x;
            float final_y = y;
            float final_z = z;
            float final_o = orientation;

            Position const* transportPosition = m_movementInfo.GetTransportPos();

            if (m_transport)
            {
                final_x += transportPosition->x;
                final_y += transportPosition->y;
                final_z += transportPosition->z;
                final_o += transportPosition->o;
            }

            m_teleport_dest = WorldLocation(mapid, final_x, final_y, final_z, final_o);
            SetFallInformation(0, final_z);
            // if the player is saved before worldport ack (at logout for example)
            // this will be used instead of the current location in SaveToDB

            // move packet sent by client always after far teleport
            // code for finish transfer to new map called in WorldSession::HandleMoveWorldportAckOpcode at client packet
            SetSemaphoreTeleportFar(true);

            if (!GetSession()->PlayerLogout())
            {
                // transfer finished, inform client to start load
                WorldPacket data(SMSG_NEW_WORLD, 20);
                if (m_transport)
                {
                    data << float(transportPosition->x);
                    data << float(transportPosition->o);
                    data << float(transportPosition->y);
                }
                else
                {
                    data << float(final_x);
                    data << float(NormalizeOrientation(final_o));
                    data << float(final_y);
                }

                data << uint32(mapid);

                if (m_transport)
                {
                    data << float(transportPosition->z);
                }
                else
                {
                    data << float(final_z);
                }

                GetSession()->SendPacket(&data);
                SendSavedInstances();
            }
        }
        else                                                // !map->CanEnter(this)
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief Teleports the player back to the saved battleground entry point.
 *
 * @return true if the teleport was initiated successfully; otherwise, false.
 */
bool Player::TeleportToBGEntryPoint()
{
    ScheduleDelayedOperation(DELAYED_BG_MOUNT_RESTORE);
    ScheduleDelayedOperation(DELAYED_BG_TAXI_RESTORE);
    return TeleportTo(m_bgData.joinPos);
}

/**
 * @brief Executes queued delayed player operations.
 */
void Player::ProcessDelayedOperations()
{
    if (m_DelayedOperations == 0)
    {
        return;
    }

    if (m_DelayedOperations & DELAYED_RESURRECT_PLAYER)
    {
        ResurrectPlayer(0.0f, false);

        if (GetMaxHealth() > m_resurrectHealth)
        {
            SetHealth(m_resurrectHealth);
        }
        else
        {
            SetHealth(GetMaxHealth());
        }

        if (GetMaxPower(POWER_MANA) > m_resurrectMana)
        {
            SetPower(POWER_MANA, m_resurrectMana);
        }
        else
        {
            SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
        }

        SetPower(POWER_RAGE, 0);
        SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));

        SpawnCorpseBones();
    }

    if (m_DelayedOperations & DELAYED_SAVE_PLAYER)
    {
        SaveToDB();
    }

    if (m_DelayedOperations & DELAYED_SPELL_CAST_DESERTER)
    {
        CastSpell(this, 26013, true);               // Deserter
    }

    if (m_DelayedOperations & DELAYED_BG_MOUNT_RESTORE)
    {
        if (m_bgData.mountSpell)
        {
            CastSpell(this, m_bgData.mountSpell, true);
            m_bgData.mountSpell = 0;
        }
    }

    if (m_DelayedOperations & DELAYED_BG_TAXI_RESTORE)
    {
        if (m_bgData.HasTaxiPath())
        {
            m_taxi.AddTaxiDestination(m_bgData.taxiPath[0]);
            m_taxi.AddTaxiDestination(m_bgData.taxiPath[1]);
            m_bgData.ClearTaxiPath();

            ContinueTaxiFlight();
        }
    }

    // we have executed ALL delayed ops, so clear the flag
    m_DelayedOperations = 0;
}

/**
 * @brief Adds the player and equipped items to the world.
 */
void Player::AddToWorld()
{
    ///- Do not add/remove the player from the object storage
    ///- It will crash when updating the ObjectAccessor
    ///- The player should only be added when logging in
    Unit::AddToWorld();

    for (int i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; ++i)
    {
        if (m_items[i])
        {
            m_items[i]->AddToWorld();
        }
    }
}

/**
 * @brief Removes the player and equipped items from the world.
 */
void Player::RemoveFromWorld()
{
    for (int i = PLAYER_SLOT_START; i < PLAYER_SLOT_END; ++i)
    {
        if (m_items[i])
        {
            m_items[i]->RemoveFromWorld();
        }
    }

    ///- Do not add/remove the player from the object storage
    ///- It will crash when updating the ObjectAccessor
    ///- The player should only be removed when logging out
    if (IsInWorld())
    {
        GetCamera().ResetView();
    }

    Unit::RemoveFromWorld();
}

/**
 * @brief Awards rage generated from dealing or receiving damage.
 *
 * @param damage The raw damage amount used for rage generation.
 * @param attacker True when the player dealt the damage; false when the player received it.
 */
void Player::RewardRage(uint32 damage, uint32 weaponSpeedHitFactor, bool attacker)
{
    float addRage;

    float rageconversion = float((0.0091107836 * getLevel() * getLevel()) + 3.225598133 * getLevel()) + 4.2652911f;

    if (attacker)
    {
        addRage = ((damage / rageconversion * 7.5f + weaponSpeedHitFactor) / 2.0f);

        // talent who gave more rage on attack
        addRage *= 1.0f + GetTotalAuraModifier(SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT) / 100.0f;
    }
    else
    {
        addRage = damage / rageconversion * 2.5f;

        // Berserker Rage effect
        if (HasAura(18499, EFFECT_INDEX_0))
        {
            addRage *= 1.3f;
        }
    }

    addRage *= sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RAGE_INCOME);

    ModifyPower(POWER_RAGE, uint32(addRage * 10));
}

/**
 * @brief Regenerates the player's health and power resources for the current tick.
 */
void Player::RegenerateAll(uint32 diff)
{
    // Not in combat or they have regeneration
    if (!IsInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT) ||
            HasAuraType(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT) || IsPolymorphed() || m_baseHealthRegen)
    {
        RegenerateHealth(diff);
        if (!IsInCombat() && !HasAuraType(SPELL_AURA_INTERRUPT_REGEN))
        {
            Regenerate(POWER_RAGE, diff);
            if (getClass() == CLASS_DEATH_KNIGHT)
            {
                Regenerate(POWER_RUNIC_POWER, diff);
            }
        }
    }

    Regenerate(POWER_ENERGY, diff);

    Regenerate(POWER_MANA, diff);

    if (getClass() == CLASS_DEATH_KNIGHT)
    {
        Regenerate(POWER_RUNE, diff);
    }

    if (getClass() == CLASS_HUNTER)
    {
        Regenerate(POWER_FOCUS, diff);
    }

    if (getClass() == CLASS_PALADIN)
    {
        if (IsInCombat())
        {
            ResetHolyPowerRegenTimer();
        }
        else if (m_holyPowerRegenTimer <= diff)
        {
            m_holyPowerRegenTimer = 0;
        }
        else
        {
            m_holyPowerRegenTimer -= diff;
        }

        if (!m_holyPowerRegenTimer)
        {
            Regenerate(POWER_HOLY_POWER, diff);
            ResetHolyPowerRegenTimer();
        }
    }

    m_regenTimer = REGEN_TIME_FULL;
}

/**
 * @brief Regenerates or decays a specific player power type.
 *
 * @param power The power type to update.
 */
// diff contains the time in milliseconds since last regen.
void Player::Regenerate(Powers power, uint32 diff)
{
    uint32 powerIndex = GetPowerIndex(power);
    if (powerIndex == INVALID_POWER_INDEX)
    {
        return;
    }

    uint32 maxValue = GetMaxPowerByIndex(powerIndex);
    if (!maxValue)
    {
        return;
    }

    uint32 curValue = GetPowerByIndex(powerIndex);

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_MANA:
        {
            if (HasAuraType(SPELL_AURA_STOP_NATURAL_MANA_REGEN))
            {
                break;
            }
            float ManaIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_MANA);

            if (IsInCombat())
            {
                // Mangos Updates Mana in intervals of 2s, which is correct
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER) *  ManaIncreaseRate * 2.00f;
            }
            else
            {
                addvalue = GetFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER) * ManaIncreaseRate * 2.00f;
            }
            break;
        }
        case POWER_RAGE:                                    // Regenerate rage
        {
            float RageDecreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RAGE_LOSS);
            addvalue = 20 * RageDecreaseRate;               // 2 rage by tick (= 2 seconds => 1 rage/sec)
            break;
        }
        case POWER_FOCUS:
            addvalue = 12;
            break;
        case POWER_HOLY_POWER:
            if (!m_holyPowerRegenTimer)
            {
                addvalue = 1;
            }
            else
            {
                return;
            }
            break;
        case POWER_ENERGY:                                  // Regenerate energy (rogue)
        {
            float EnergyRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_ENERGY);
            addvalue = 20 * EnergyRate;
            break;
        }
        case POWER_RUNIC_POWER:
        {
            float RunicPowerDecreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_POWER_RUNICPOWER_LOSS);
            addvalue = 30 * RunicPowerDecreaseRate;         // 3 RunicPower by tick
            break;
        }
        case POWER_RUNE:
        {
            if (getClass() != CLASS_DEATH_KNIGHT)
            {
                break;
            }

            for (uint8 rune = 0; rune < MAX_RUNES; rune += 2)
            {
                uint32 cd_diff = diff;
                uint8 runeToRegen = rune;
                uint32 cd = GetRuneCooldown(rune);
                uint32 secondRuneCd = GetRuneCooldown(rune + 1);
                // Regenerate second rune of the same type only after first rune is off the cooldown
                if (secondRuneCd && (cd > secondRuneCd || !cd))
                {
                    ++runeToRegen;
                    cd = secondRuneCd;
                }

                if (cd)
                {
                    SetRuneCooldown(rune, (cd < cd_diff) ? 0 : cd - cd_diff);
                }
            }
            break;
        }
        case POWER_HEALTH:
            break;
        default:
            break;
    }

    // Mana regen calculated in Player::UpdateManaRegen()
    // Exist only for POWER_MANA, POWER_ENERGY, POWER_FOCUS auras
    if (power != POWER_MANA)
    {
        AuraList const& ModPowerRegenPCTAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
        for (AuraList::const_iterator i = ModPowerRegenPCTAuras.begin(); i != ModPowerRegenPCTAuras.end(); ++i)
            if ((*i)->GetModifier()->m_miscvalue == int32(power))
            {
                addvalue *= ((*i)->GetModifier()->m_amount + 100) / 100.0f;
            }
    }

    // addvalue computed on a 2sec basis. => update to diff time
    addvalue *= float(diff) / REGEN_TIME_FULL;

    if (power != POWER_RAGE && power != POWER_RUNIC_POWER && power != POWER_HOLY_POWER)
    {
        curValue += uint32(addvalue);
        if (curValue > maxValue)
        {
            curValue = maxValue;
        }
    }
    else
    {
        if (curValue <= uint32(addvalue))
        {
            curValue = 0;
        }
        else
        {
            curValue -= uint32(addvalue);
        }
    }

    SetPower(power, curValue);
}

/**
 * @brief Regenerates the player's health based on state, auras, and rates.
 */
void Player::RegenerateHealth(uint32 diff)
{
    uint32 curValue = GetHealth();
    uint32 maxValue = GetMaxHealth();

    if (curValue >= maxValue)
    {
        return;
    }

    float HealthIncreaseRate = sWorld.getConfig(CONFIG_FLOAT_RATE_HEALTH);
    // This needs fixing
    HealthIncreaseRate = 1.0f; // having to do this as the above constantly results in 0 - mangosd.conf is obviously not being read properly

    float addvalue = 0.0f;

    // polymorphed case
    if (IsPolymorphed())
    {
        addvalue = (float)GetMaxHealth() / 3;
    }
    // normal regen case (maybe partly in combat case)
    else if (!IsInCombat() || HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT))
    {
        addvalue = HealthIncreaseRate;
        if (!IsInCombat())
        {
            if (getLevel() < 15)
            {
                addvalue = 0.20f * GetMaxHealth() * addvalue / getLevel();
            }
            else
            {
                addvalue = 0.015f * GetMaxHealth() * addvalue;
            }

            AuraList const& mModHealthRegenPct = GetAurasByType(SPELL_AURA_MOD_HEALTH_REGEN_PERCENT);
            for (AuraList::const_iterator i = mModHealthRegenPct.begin(); i != mModHealthRegenPct.end(); ++i)
            {
                addvalue *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
            }

            addvalue += GetTotalAuraModifier(SPELL_AURA_MOD_REGEN) * 2.0f / 5.0f;
        }
        else if (HasAuraType(SPELL_AURA_MOD_REGEN_DURING_COMBAT))
        {
            addvalue *= GetTotalAuraModifier(SPELL_AURA_MOD_REGEN_DURING_COMBAT) / 100.0f;
        }

        if (!IsStandState())
        {
            addvalue *= 1.33f;
        }
    }

    // always regeneration bonus (including combat)
    addvalue += GetTotalAuraModifier(SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT);

    addvalue += m_baseHealthRegen / 2.5f;

    if (addvalue < 0)
    {
        addvalue = 0;
    }

    addvalue *= (float)diff / REGEN_TIME_FULL;

    ModifyHealth(int32(addvalue));
}

/**
 * @brief Gets an NPC the player can currently interact with.
 *
 * @param guid The target creature GUID.
 * @param NpcFlagsmask Optional NPC flag mask that must be present on the creature.
 * @return The interactable creature, or null if interaction is not allowed.
 */
Creature* Player::GetNPCIfCanInteractWith(ObjectGuid guid, uint32 NpcFlagsmask)
{
    // some basic checks
    if (!guid || !IsInWorld() || IsTaxiFlying())
    {
        return NULL;
    }

    // set player as interacting
    DoInteraction(guid);

    // not in interactive state
    if (hasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
    {
        return NULL;
    }

    // exist (we need look pets also for some interaction (quest/etc)
    Creature* unit = GetMap()->GetAnyTypeCreature(guid);
    if (!unit)
    {
        return NULL;
    }

    // appropriate npc type
    if (NpcFlagsmask && !unit->HasFlag(UNIT_NPC_FLAGS, NpcFlagsmask))
    {
        return NULL;
    }

    if (NpcFlagsmask == UNIT_NPC_FLAG_STABLEMASTER)
    {
        if (getClass() != CLASS_HUNTER)
        {
            return NULL;
        }
    }

    // if a dead unit should be able to talk - the creature must be alive and have special flags
    if (!unit->IsAlive())
    {
        return NULL;
    }

    if (IsAlive() && unit->IsInvisibleForAlive())
    {
        return NULL;
    }

    // not allow interaction under control, but allow with own pets
    if (unit->GetCharmerGuid())
    {
        return NULL;
    }

    // not enemy
    if (unit->IsHostileTo(this))
    {
        return NULL;
    }

    // not too far
    if (!unit->IsWithinDistInMap(this, INTERACTION_DISTANCE))
    {
        return NULL;
    }

    return unit;
}

/**
 * @brief Gets a game object the player can currently interact with.
 *
 * @param guid The target game object GUID.
 * @param gameobject_type The required game object type, or MAX_GAMEOBJECT_TYPE for any type.
 * @return The interactable game object, or null if interaction is not allowed.
 */
GameObject* Player::GetGameObjectIfCanInteractWith(ObjectGuid guid, uint32 gameobject_type)
{
    // some basic checks
    if (!guid || !IsInWorld() || IsTaxiFlying())
    {
        return NULL;
    }

    // set player as interacting
    DoInteraction(guid);

    // not in interactive state
    if (hasUnitState(UNIT_STAT_CAN_NOT_REACT_OR_LOST_CONTROL))
    {
        return NULL;
    }

    if (GameObject* go = GetMap()->GetGameObject(guid))
    {
        if (uint32(go->GetGoType()) == gameobject_type || gameobject_type == MAX_GAMEOBJECT_TYPE)
        {
            float maxdist = go->GetInteractionDistance();
            if (go->IsWithinDistInMap(this, maxdist) && go->isSpawned())
            {
                return go;
            }

            sLog.outError("GetGameObjectIfCanInteractWith: GameObject '%s' [GUID: %u] is too far away from player %s [GUID: %u] to be used by him (distance=%f, maximal %f is allowed)",
                          go->GetGOInfo()->name,  go->GetGUIDLow(), GetName(), GetGUIDLow(), go->GetDistance(this), maxdist);
        }
    }
    return NULL;
}

/**
 * @brief Checks whether the player is currently underwater.
 *
 * @return True if the player is underwater; otherwise, false.
 */
bool Player::IsUnderWater() const
{
    return GetTerrain()->IsUnderWater(GetPositionX(), GetPositionY(), GetPositionZ() + 2);
}

/**
 * @brief Updates the player's in-water state.
 *
 * @param apply True if the player is entering water; false if leaving it.
 */
void Player::SetInWater(bool apply)
{
    if (m_isInWater == apply)
    {
        return;
    }

    // define player in water by opcodes
    // move player's guid into HateOfflineList of those mobs
    // which can't swim and move guid back into ThreatList when
    // on surface.
    // TODO: exist also swimming mobs, and function must be symmetric to enter/leave water
    m_isInWater = apply;

    // remove auras that need water/land
    RemoveAurasWithInterruptFlags(apply ? AURA_INTERRUPT_FLAG_NOT_ABOVEWATER : AURA_INTERRUPT_FLAG_NOT_UNDERWATER);

    GetHostileRefManager().updateThreatTables();
}

struct SetGameMasterOnHelper
{
    explicit SetGameMasterOnHelper() {}
    void operator()(Unit* unit) const
    {
        unit->setFaction(35);
        unit->GetHostileRefManager().setOnlineOfflineState(false);
    }
};

struct SetGameMasterOffHelper
{
    explicit SetGameMasterOffHelper(uint32 _faction) : faction(_faction) {}
    void operator()(Unit* unit) const
    {
        unit->setFaction(faction);
        unit->GetHostileRefManager().setOnlineOfflineState(true);
    }
    uint32 faction;
};

/**
 * @brief Enables or disables game master mode for the player.
 *
 * @param on True to enable GM mode; false to disable it.
 */
void Player::SetGameMaster(bool on)
{
    if (on)
    {
        m_ExtraFlags |= PLAYER_EXTRA_GM_ON;
        //setFaction(35);
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_0);
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM);
        CallForAllControlledUnits(SetGameMasterOnHelper(), CONTROLLED_PET | CONTROLLED_TOTEMS | CONTROLLED_GUARDIANS | CONTROLLED_CHARM);

        SetFFAPvP(false);
        ResetContestedPvP();

        GetHostileRefManager().setOnlineOfflineState(false);
        CombatStopWithPets();

        if (Pet* pet = GetPet())
        {
            if (m_ExtraFlags |= PLAYER_EXTRA_GM_ON)
                pet->setFaction(35);
            pet->GetHostileRefManager().setOnlineOfflineState(false);
        }

        SetPhaseMask(PHASEMASK_ANYWHERE, false);            // see and visible in all phases
    }
    else
    {
        m_ExtraFlags &= ~ PLAYER_EXTRA_GM_ON;
        //setFactionForRace(getRace());
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_0);
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GM);

        // restore phase
        AuraList const& phases = GetAurasByType(SPELL_AURA_PHASE);
        AuraList const& phases2 = GetAurasByType(SPELL_AURA_PHASE_2);

        if (!phases.empty())
        {
            SetPhaseMask(phases.front()->GetMiscValue(), false);
        }
        else if (!phases2.empty())
        {
            SetPhaseMask(phases2.front()->GetMiscValue(), false);
        }
        else
        {
            SetPhaseMask(PHASEMASK_NORMAL, false);
        }

        if (Pet* pet = GetPet())
        {
            pet->setFaction(getFaction());
            pet->GetHostileRefManager().setOnlineOfflineState(true);
        }

        CallForAllControlledUnits(SetGameMasterOffHelper(getFaction()), CONTROLLED_PET | CONTROLLED_TOTEMS | CONTROLLED_GUARDIANS | CONTROLLED_CHARM);

        // restore FFA PvP Server state
        if (sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(true);
        }

        // restore FFA PvP area state, remove not allowed for GM mounts
        UpdateArea(m_areaUpdateId);

        GetHostileRefManager().setOnlineOfflineState(true);
    }

    m_camera.UpdateVisibilityForOwner();
    UpdateObjectVisibility();
    UpdateForQuestWorldObjects();
}

/**
 * @brief Sets whether a game master is visible to other players.
 *
 * @param on True to make the GM visible; false to hide them.
 */
void Player::SetGMVisible(bool on)
{
    if (on)
    {
        m_ExtraFlags &= ~PLAYER_EXTRA_GM_INVISIBLE;         // remove flag

        // Reapply stealth/invisibility if active or show if not any
        if (HasAuraType(SPELL_AURA_MOD_STEALTH))
        {
            SetVisibility(VISIBILITY_GROUP_STEALTH);
        }
        else if (HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
        {
            SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        }
        else
        {
            SetVisibility(VISIBILITY_ON);
        }
    }
    else
    {
        m_ExtraFlags |= PLAYER_EXTRA_GM_INVISIBLE;          // add flag

        SetAcceptWhispers(false);
        SetGameMaster(true);

        SetVisibility(VISIBILITY_OFF);
    }
}

/**
 * @brief Checks whether another player should be visible through group visibility rules.
 *
 * @param p The player to test visibility against.
 * @return True if the player is group-visible; otherwise, false.
 */
bool Player::IsGroupVisibleFor(Player* p) const
{
    switch (sWorld.getConfig(CONFIG_UINT32_GROUP_VISIBILITY))
    {
        default:
            return IsInSameGroupWith(p);
        case 1:
            return IsInSameRaidWith(p);
        case 2:
            return GetTeam() == p->GetTeam();
    }
}

/**
 * @brief Checks whether this player and another player are in the same subgroup.
 *
 * @param p The other player to compare.
 * @return True if both players share the same group context; otherwise, false.
 */
bool Player::IsInSameGroupWith(Player const* p) const
{
    return (p == this || (GetGroup() != NULL &&
                          GetGroup()->SameSubGroup(this, p)));
}

///- If the player is invited, remove him. If the group if then only 1 person, disband the group.
/// \todo Shouldn't we also check if there is no other invitees before disbanding the group?
void Player::UninviteFromGroup()
{
    Group* group = GetGroupInvite();
    if (!group)
    {
        return;
    }

    group->RemoveInvite(this);

    if (group->GetMembersCount() <= 1)                      // group has just 1 member => disband
    {
        if (group->IsCreated())
        {
            group->Disband(true);
            sObjectMgr.RemoveGroup(group);
        }
        else
        {
            group->RemoveAllInvites();
        }

        delete group;
    }
}

/**
 * @brief Removes a member from a group and disposes of the group if it becomes empty.
 *
 * @param group The group to remove the member from.
 * @param guid The GUID of the member to remove.
 */
void Player::RemoveFromGroup(Group* group, ObjectGuid guid)
{
    if (group)
    {
        if (group->RemoveMember(guid, 0) <= 1)
        {
            // group->Disband(); already disbanded in RemoveMember
            sObjectMgr.RemoveGroup(group);
            delete group;
            // RemoveMember sets the player's group pointer to NULL
        }
    }
}

/**
 * @brief Sends the experience gain log packet to the client.
 *
 * @param GivenXP The base amount of experience awarded.
 * @param victim The kill source, or null for non-kill experience.
 * @param RestXP The rested bonus experience amount.
 */
void Player::SendLogXPGain(uint32 GivenXP, Unit* victim, uint32 RestXP)
{
    WorldPacket data(SMSG_LOG_XPGAIN, 21);
    data << (victim ? victim->GetObjectGuid() : ObjectGuid());// guid
    data << uint32(GivenXP + RestXP);                       // given experience
    data << uint8(victim ? 0 : 1);                          // 00-kill_xp type, 01-non_kill_xp type
    if (victim)
    {
        data << uint32(GivenXP);                            // experience without rested bonus
        data << float(1);                                   // 1 - none 0 - 100% group bonus output
    }
    data << uint8(0);                                       // new 2.4.0
    GetSession()->SendPacket(&data);
}

/**
 * @brief Awards experience to the player and handles level-ups.
 *
 * @param xp The experience amount to award.
 * @param victim The unit responsible for kill-based experience, if any.
 */
void Player::GiveXP(uint32 xp, Unit* victim)
{
    if (xp < 1)
    {
        return;
    }

    if (!IsAlive())
    {
        return;
    }

    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_XP_USER_DISABLED))
    {
        return;
    }

    uint32 level = getLevel();

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnGiveXP(this, xp, victim);
    }
#endif /* ENABLE_ELUNA */

    // XP to money conversion processed in Player::RewardQuest
    if (level >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        return;
    }

    if (victim)
    {
        // handle SPELL_AURA_MOD_KILL_XP_PCT auras
        Unit::AuraList const& ModXPPctAuras = GetAurasByType(SPELL_AURA_MOD_KILL_XP_PCT);
        for (Unit::AuraList::const_iterator i = ModXPPctAuras.begin(); i != ModXPPctAuras.end(); ++i)
        {
            xp = uint32(xp * (1.0f + (*i)->GetModifier()->m_amount / 100.0f));
        }
    }
    else
    {
        // handle SPELL_AURA_MOD_QUEST_XP_PCT auras
        Unit::AuraList const& ModXPPctAuras = GetAurasByType(SPELL_AURA_MOD_QUEST_XP_PCT);
        for (Unit::AuraList::const_iterator i = ModXPPctAuras.begin(); i != ModXPPctAuras.end(); ++i)
        {
            xp = uint32(xp * (1.0f + (*i)->GetModifier()->m_amount / 100.0f));
        }
    }

    // XP resting bonus for kill
    uint32 rested_bonus_xp = victim ? GetXPRestBonus(xp) : 0;

    SendLogXPGain(xp, victim, rested_bonus_xp);

    uint32 curXP = GetUInt32Value(PLAYER_XP);
    uint32 nextLvlXP = GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    uint32 newXP = curXP + xp + rested_bonus_xp;

    while (newXP >= nextLvlXP && level < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        newXP -= nextLvlXP;

        if (level < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
        {
            GiveLevel(level + 1);
        }

        level = getLevel();
        nextLvlXP = GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    }

    SetUInt32Value(PLAYER_XP, newXP);
}

/**
 * @brief Advances the player to a new level and reapplies level-based stats.
 *
 * @param level The new level to assign.
 */
void Player::GiveLevel(uint32 level)
{
    if (level == getLevel())
    {
        return;
    }

    PlayerLevelInfo info;
    sObjectMgr.GetPlayerLevelInfo(getRace(), getClass(), level, &info);

    uint32 basehp = 0, basemana = 0;
    sObjectMgr.GetPlayerClassLevelInfo(getClass(), level, basehp, basemana);

    // send levelup info to client
    WorldPacket data(SMSG_LEVELUP_INFO, (4 + 4 + MAX_STORED_POWERS * 4 + MAX_STATS * 4));
    data << uint32(level);
    data << uint32(int32(basehp) - int32(GetCreateHealth()));
    // for(int i = 0; i < MAX_POWERS; ++i)                  // Powers loop (0-4)
    data << uint32(int32(basemana)   - int32(GetCreateMana()));
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    data << uint32(0);
    // end for
    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)         // Stats loop (0-4)
    {
        data << uint32(int32(info.stats[i]) - GetCreateStat(Stats(i)));
    }

    GetSession()->SendPacket(&data);

    SetUInt32Value(PLAYER_NEXT_LEVEL_XP, sObjectMgr.GetXPForLevel(level));

    // update level, max level of skills
    m_Played_time[PLAYED_TIME_LEVEL] = 0;                   // Level Played Time reset

    _ApplyAllLevelScaleItemMods(false);

    SetLevel(level);

    UpdateSkillsForLevel();

    // save base values (bonuses already included in stored stats
    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        SetCreateStat(Stats(i), info.stats[i]);
    }

    SetCreateHealth(basehp);
    SetCreateMana(basemana);

    InitTalentForLevel();
    InitTaxiNodesForLevel();
    InitGlyphsForLevel();

    UpdateAllStats();

    // set current level health and mana/energy to maximum after applying all mods.
    if (IsAlive())
    {
        SetHealth(GetMaxHealth());
    }
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));
    if (GetPower(POWER_RAGE) > GetMaxPower(POWER_RAGE))
    {
        SetPower(POWER_RAGE, GetMaxPower(POWER_RAGE));
    }
    SetPower(POWER_FOCUS, 0);

    _ApplyAllLevelScaleItemMods(true);

    // update level to hunter/summon pet
    if (Pet* pet = GetPet())
    {
        pet->SynchronizeLevelWithOwner();
    }

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnLevelChanged(this, getLevel());
    }
#endif /* ENABLE_ELUNA */

    if (MailLevelReward const* mailReward = sObjectMgr.GetMailLevelReward(level, getRaceMask()))
    {
        MailDraft(mailReward->mailTemplateId).SendMailTo(this, MailSender(MAIL_CREATURE, mailReward->senderEntry));
    }

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL);

    // resend quests status directly
    SendQuestGiverStatusMultiple();
}

/**
 * @brief Recalculates the player's free talent points for the current level.
 *
 * @param resetIfNeed True to reset talents when the allocation is invalid.
 */
void Player::UpdateFreeTalentPoints(bool resetIfNeed)
{
    uint32 level = getLevel();
    // talents base at level diff ( talents = level - 9 but some can be used already)
    if (level < 10)
    {
        // Remove all talent points
        if (m_usedTalentCount > 0)                          // Free any used talents
        {
            if (resetIfNeed)
            {
                resetTalents(true);
            }
            SetFreeTalentPoints(0);
        }
    }
    else
    {
        if (m_specsCount == 0)
        {
            m_specsCount = 1;
            m_activeSpec = 0;
        }

        uint32 talentPointsForLevel = CalculateTalentsPoints();

        // if used more that have then reset
        if (m_usedTalentCount > talentPointsForLevel)
        {
            if (resetIfNeed && GetSession()->GetSecurity() < SEC_ADMINISTRATOR)
            {
                resetTalents(true);
            }
            else
            {
                SetFreeTalentPoints(0);
            }
        }
        // else update amount of free points
        else
        {
            SetFreeTalentPoints(talentPointsForLevel - m_usedTalentCount);
        }
    }
}

/**
 * @brief Initializes level-based talent availability for the player.
 */
void Player::InitTalentForLevel()
{
    UpdateFreeTalentPoints();

    if (!GetSession()->PlayerLoading())
    {
        SendTalentsInfoData(false);                         // update at client
    }
}

/**
 * @brief Initializes the player's base stats and resources for the current level.
 *
 * @param reapplyMods True to remove and reapply stat modifiers during initialization.
 */
void Player::InitStatsForLevel(bool reapplyMods)
{
    if (reapplyMods)                                        // reapply stats values only on .reset stats (level) command
    {
        _RemoveAllStatBonuses();
    }

    uint32 basehp = 0, basemana = 0;
    sObjectMgr.GetPlayerClassLevelInfo(getClass(), getLevel(), basehp, basemana);

    PlayerLevelInfo info;
    sObjectMgr.GetPlayerLevelInfo(getRace(), getClass(), getLevel(), &info);

    SetUInt32Value(PLAYER_FIELD_MAX_LEVEL, sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
    SetUInt32Value(PLAYER_NEXT_LEVEL_XP, sObjectMgr.GetXPForLevel(getLevel()));

    // reset before any aura state sources (health set/aura apply)
    SetUInt32Value(UNIT_FIELD_AURASTATE, 0);

    UpdateSkillsForLevel();

    // set default cast time multiplier
    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);

    // save base values (bonuses already included in stored stats
    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        SetCreateStat(Stats(i), info.stats[i]);
    }

    for (int i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        SetStat(Stats(i), info.stats[i]);
    }

    SetCreateHealth(basehp);

    // set create powers
    SetCreateMana(basemana);

    SetArmor(int32(m_createStats[STAT_AGILITY] * 2));

    InitStatBuffMods();

    // reset rating fields values
    for (uint16 index = PLAYER_FIELD_COMBAT_RATING_1; index < PLAYER_FIELD_COMBAT_RATING_1 + MAX_COMBAT_RATING; ++index)
    {
        SetUInt32Value(index, 0);
    }

    SetUInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS, 0);
    SetFloatValue(PLAYER_FIELD_MOD_HEALING_PCT, 1.0f);
    SetFloatValue(PLAYER_FIELD_MOD_HEALING_DONE_PCT, 1.0f);
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, 0);
        SetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, 0);
        SetFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, 1.00f);
    }

    SetFloatValue(PLAYER_FIELD_MOD_SPELL_POWER_PCT, 1.0f);

    // reset attack power, damage and attack speed fields
    SetFloatValue(UNIT_FIELD_BASEATTACKTIME, 2000.0f);
    SetFloatValue(UNIT_FIELD_BASEATTACKTIME + 1, 2000.0f);  // offhand attack time
    SetFloatValue(UNIT_FIELD_RANGEDATTACKTIME, 2000.0f);

    SetFloatValue(UNIT_FIELD_MINDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MAXDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE, 0.0f);
    SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, 0.0f);
    SetFloatValue(PLAYER_FIELD_WEAPON_DMG_MULTIPLIERS, 1.0f);

    SetInt32Value(UNIT_FIELD_ATTACK_POWER,            0);
    SetInt32Value(UNIT_FIELD_ATTACK_POWER_MOD_POS,    0 );
    SetInt32Value(UNIT_FIELD_ATTACK_POWER_MOD_NEG,    0 );
    SetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER, 0.0f);
    SetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER,     0);
    SetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MOD_POS,0 );
    SetInt32Value(UNIT_FIELD_RANGED_ATTACK_POWER_MOD_NEG,0 );
    SetFloatValue(UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER, 0.0f);

    // Base crit values (will be recalculated in UpdateAllStats() at loading and in _ApplyAllStatBonuses() at reset
    SetFloatValue(PLAYER_CRIT_PERCENTAGE, 0.0f);
    SetFloatValue(PLAYER_OFFHAND_CRIT_PERCENTAGE, 0.0f);
    SetFloatValue(PLAYER_RANGED_CRIT_PERCENTAGE, 0.0f);

    // Init spell schools (will be recalculated in UpdateAllStats() at loading and in _ApplyAllStatBonuses() at reset
    for (uint8 i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + i, 0.0f);
    }

    SetFloatValue(PLAYER_PARRY_PERCENTAGE, 0.0f);
    SetFloatValue(PLAYER_BLOCK_PERCENTAGE, 0.0f);
    SetUInt32Value(PLAYER_SHIELD_BLOCK, uint32(BASE_BLOCK_DAMAGE_PERCENT));

    // Dodge percentage
    SetFloatValue(PLAYER_DODGE_PERCENTAGE, 0.0f);

    // set armor (resistance 0) to original value (create_agility*2)
    SetArmor(int32(m_createStats[STAT_AGILITY] * 2));
    SetResistanceBuffMods(SpellSchools(0), true, 0.0f);
    SetResistanceBuffMods(SpellSchools(0), false, 0.0f);
    // set other resistance to original value (0)
    for (int i = 1; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetResistance(SpellSchools(i), 0);
        SetResistanceBuffMods(SpellSchools(i), true, 0.0f);
        SetResistanceBuffMods(SpellSchools(i), false, 0.0f);
    }

    SetUInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, 0);
    SetUInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE, 0);
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetUInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, 0);
        SetFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, 0.0f);
    }
    // Reset no reagent cost field
    for (int i = 0; i < 3; ++i)
    {
        SetUInt32Value(PLAYER_NO_REAGENT_COST_1 + i, 0);
    }
    // Init data for form but skip reapply item mods for form
    InitDataForForm(reapplyMods);

    // save new stats
    for (int i = POWER_MANA; i < MAX_POWERS; ++i)
    {
        SetMaxPower(Powers(i), GetCreateMaxPowers(Powers(i)));
    }

    SetMaxHealth(basehp);                     // stamina bonus will applied later

    // cleanup mounted state (it will set correctly at aura loading if player saved at mount.
    SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0);

    // cleanup unit flags (will be re-applied if need at aura load).
    RemoveFlag(UNIT_FIELD_FLAGS,
               UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_ATTACKABLE_1 |
               UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_PASSIVE  | UNIT_FLAG_LOOTING          |
               UNIT_FLAG_PET_IN_COMBAT  | UNIT_FLAG_SILENCED     | UNIT_FLAG_PACIFIED         |
               UNIT_FLAG_STUNNED        | UNIT_FLAG_IN_COMBAT    | UNIT_FLAG_DISARMED         |
               UNIT_FLAG_CONFUSED       | UNIT_FLAG_FLEEING      | UNIT_FLAG_NOT_SELECTABLE   |
               UNIT_FLAG_SKINNABLE      | UNIT_FLAG_MOUNT        | UNIT_FLAG_TAXI_FLIGHT);
    SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);    // must be set

    SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER); // must be set

    // cleanup player flags (will be re-applied if need at aura load), to avoid have ghost flag without ghost aura, for example.
    RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_AFK | PLAYER_FLAGS_DND | PLAYER_FLAGS_GM | PLAYER_FLAGS_GHOST);

    RemoveStandFlags(UNIT_STAND_FLAGS_ALL);                 // one form stealth modified bytes
    RemoveByteFlag(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP);

    // restore if need some important flags
    SetUInt32Value(PLAYER_FIELD_BYTES2, 0);                 // flags empty by default

    if (reapplyMods)                                        // reapply stats values only on .reset stats (level) command
    {
        _ApplyAllStatBonuses();
    }

    // set current level health and mana/energy to maximum after applying all mods.
    SetHealth(GetMaxHealth());
    SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));
    if (GetPower(POWER_RAGE) > GetMaxPower(POWER_RAGE))
    {
        SetPower(POWER_RAGE, GetMaxPower(POWER_RAGE));
    }
    SetPower(POWER_FOCUS, 0);
    SetPower(POWER_RUNIC_POWER, 0);

    // update level to hunter/summon pet
    if (Pet* pet = GetPet())
    {
        pet->SynchronizeLevelWithOwner();
    }
}

/**
 * @brief Sends the initial spellbook and cooldown state to the client.
 */
void Player::SendInitialSpells()
{
    time_t curTime = time(NULL);
    time_t infTime = curTime + infinityCooldownDelayCheck;

    /* * * * * * * * * * * * * * * * *
     * * START OF PACKET STRUCTURE * *
     * * * * * * * * * * * * * * * * */
    uint16 spellCount = 0;

    WorldPacket data(SMSG_INITIAL_SPELLS, (1 + 2 + 4 * m_spells.size() + 2 + m_spellCooldowns.size() * (2 + 2 + 2 + 4 + 4)));
    data << uint8(0);

    /* * * * * * * * * * * * * * * * *
     * *  END OF PACKET STRUCTURE  * *
     * * * * * * * * * * * * * * * * */
    size_t countPos = data.wpos();
    data << uint16(spellCount);                             // spell count placeholder

    /* For each spell the player knows */
    for (PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        /* If the spell is marked as removed, don't send it */
        PlayerSpell const& playerSpell = itr->second;

        if (playerSpell.state == PLAYERSPELL_REMOVED)
        {
            continue;
        }

        if (!playerSpell.active || playerSpell.disabled)
        {
            continue;
        }

        /* Insert spell into vector for insertion into packet */
        data << uint32(itr->first);
        data << uint16(0);                                  // it's not slot id

        /* Increase spell counter by 1 (sent in packet) */
        spellCount += 1;
    }

    data.put<uint16>(countPos, spellCount);                 // write real count value

    /* For each spell the player has on cooldown */
    uint16 spellCooldowns = m_spellCooldowns.size();
    data << uint16(spellCooldowns);
    for (SpellCooldowns::const_iterator itr = m_spellCooldowns.begin(); itr != m_spellCooldowns.end(); ++itr)
    {
        /* If the spell doesn't exist in the spellbook, just ignore it */
        SpellEntry const* sEntry = sSpellStore.LookupEntry(itr->first);
        if (!sEntry)
        {
            continue;
        }

        SpellCooldown const& spellCooldown = itr->second;

        data << uint32(itr->first);

        data << uint32(spellCooldown.itemid);                 // cast item id
        data << uint16(sEntry->GetCategory());              // spell category

        /* send infinity cooldown in special format */
        if (spellCooldown.end >= infTime)
        {
            data << uint32(1);                              // cooldown
            data << uint32(0x80000000);                     // category cooldown
            continue;
        }

        time_t cooldown = spellCooldown.end > curTime ? (spellCooldown.end - curTime) * IN_MILLISECONDS : 0;

        if (sEntry->GetCategory())                           // may be wrong, but anyway better than nothing...
        {
            data << uint32(0);                              // cooldown
            data << uint32(cooldown);                       // category cooldown
        }
        else
        {
            data << uint32(cooldown);                       // cooldown
            data << uint32(0);                              // category cooldown
        }
    }

    GetSession()->SendPacket(&data);

    DETAIL_LOG("CHARACTER: Sent Initial Spells");
}

/**
 * @brief Removes a mail entry from the player's in-memory mailbox.
 *
 * @param id The message identifier to remove.
 */
void Player::RemoveMail(uint32 id)
{
    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if ((*itr)->messageID == id)
        {
            // do not delete item, because Player::removeMail() is called when returning mail to sender.
            m_mail.erase(itr);
            return;
        }
    }
}

/**
 * @brief Sends the result of a mail operation to the client.
 *
 * @param mailId The mail message identifier.
 * @param mailAction The mail action that was processed.
 * @param mailError The result code for the action.
 * @param equipError The equipment error code used for equip-related failures.
 * @param item_guid The related item GUID low part.
 * @param item_count The related item count.
 */
void Player::SendMailResult(uint32 mailId, MailResponseType mailAction, MailResponseResult mailError, uint32 equipError, uint32 item_guid, uint32 item_count)
{
    WorldPacket data(SMSG_SEND_MAIL_RESULT, (4 + 4 + 4 + (mailError == MAIL_ERR_EQUIP_ERROR ? 4 : (mailAction == MAIL_ITEM_TAKEN ? 4 + 4 : 0))));
    data << (uint32) mailId;
    data << (uint32) mailAction;
    data << (uint32) mailError;
    if (mailError == MAIL_ERR_EQUIP_ERROR)
    {
        data << (uint32) equipError;
    }
    else if (mailAction == MAIL_ITEM_TAKEN)
    {
        data << (uint32) item_guid;                         // item guid low?
        data << (uint32) item_count;                        // item count?
    }
    GetSession()->SendPacket(&data);
}

/**
 * @brief Notifies the client that new mail is available.
 */
void Player::SendNewMail()
{
    // deliver undelivered mail
    WorldPacket data(SMSG_RECEIVED_MAIL, 4);
    data << float(0.0f);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Recalculates unread mail count and the next pending delivery time.
 */
void Player::UpdateNextMailTimeAndUnreads()
{
    // calculate next delivery time (min. from non-delivered mails
    // and recalculate unReadMail
    time_t cTime = time(NULL);
    m_nextMailDelivereTime = 0;
    unReadMails = 0;
    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if ((*itr)->deliver_time > cTime)
        {
            if (!m_nextMailDelivereTime || m_nextMailDelivereTime > (*itr)->deliver_time)
            {
                m_nextMailDelivereTime = (*itr)->deliver_time;
            }
        }
        else if (((*itr)->checked & MAIL_CHECK_MASK_READ) == 0)
        {
            ++unReadMails;
        }
    }
}

/**
 * @brief Tracks a newly scheduled mail delivery time.
 *
 * @param deliver_time The time when the mail should be delivered.
 */
void Player::AddNewMailDeliverTime(time_t deliver_time)
{
    if (deliver_time <= time(NULL))                         // ready now
    {
        ++unReadMails;
        SendNewMail();
    }
    else                                                    // not ready and no have ready mails
    {
        if (!m_nextMailDelivereTime || m_nextMailDelivereTime > deliver_time)
        {
            m_nextMailDelivereTime =  deliver_time;
        }
    }
}


/**
 * @brief Finds a mail entry by message identifier.
 *
 * @param id The message identifier to search for.
 * @return The matching mail entry, or null if none exists.
 */
Mail* Player::GetMail(uint32 id)
{
    for (PlayerMails::iterator itr = m_mail.begin(); itr != m_mail.end(); ++itr)
    {
        if ((*itr)->messageID == id)
        {
            return (*itr);
        }
    }
    return NULL;
}

/**
 * @brief Populates the create update mask for a target player.
 *
 * @param updateMask The update mask being built.
 * @param target The player receiving the create data.
 */
void Player::_SetCreateBits(UpdateMask* updateMask, Player* target) const
{
    if (target == this)
    {
        Object::_SetCreateBits(updateMask, target);
    }
    else
    {
        for (uint16 index = 0; index < m_valuesCount; ++index)
        {
            if (GetUInt32Value(index) != 0 && updateVisualBits.GetBit(index))
            {
                updateMask->SetBit(index);
            }
        }
    }
}

/**
 * @brief Populates the incremental update mask for a target player.
 *
 * @param updateMask The update mask being built.
 * @param target The player receiving the update data.
 */
void Player::_SetUpdateBits(UpdateMask* updateMask, Player* target) const
{
    if (target == this)
    {
        Object::_SetUpdateBits(updateMask, target);
    }
    else
    {
        Object::_SetUpdateBits(updateMask, target);
        *updateMask &= updateVisualBits;
    }
}

/**
 * @brief Initializes the set of player fields visible to other clients.
 */
void Player::InitVisibleBits()
{
    updateVisualBits.SetCount(PLAYER_END);

    updateVisualBits.SetBit(OBJECT_FIELD_GUID + 0);
    updateVisualBits.SetBit(OBJECT_FIELD_GUID + 1);
    updateVisualBits.SetBit(OBJECT_FIELD_TYPE);
    updateVisualBits.SetBit(OBJECT_FIELD_ENTRY);
    updateVisualBits.SetBit(OBJECT_FIELD_DATA + 0);
    updateVisualBits.SetBit(OBJECT_FIELD_DATA + 1);
    updateVisualBits.SetBit(OBJECT_FIELD_SCALE_X);
    updateVisualBits.SetBit(UNIT_FIELD_CHARM + 0);
    updateVisualBits.SetBit(UNIT_FIELD_CHARM + 1);
    updateVisualBits.SetBit(UNIT_FIELD_SUMMON + 0);
    updateVisualBits.SetBit(UNIT_FIELD_SUMMON + 1);
    updateVisualBits.SetBit(UNIT_FIELD_CHARMEDBY + 0);
    updateVisualBits.SetBit(UNIT_FIELD_CHARMEDBY + 1);
    updateVisualBits.SetBit(UNIT_FIELD_TARGET + 0);
    updateVisualBits.SetBit(UNIT_FIELD_TARGET + 1);
    updateVisualBits.SetBit(UNIT_FIELD_CHANNEL_OBJECT + 0);
    updateVisualBits.SetBit(UNIT_FIELD_CHANNEL_OBJECT + 1);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_0);
    updateVisualBits.SetBit(UNIT_FIELD_HEALTH);
    updateVisualBits.SetBit(UNIT_FIELD_POWER1);
    updateVisualBits.SetBit(UNIT_FIELD_POWER2);
    updateVisualBits.SetBit(UNIT_FIELD_POWER3);
    updateVisualBits.SetBit(UNIT_FIELD_POWER4);
    updateVisualBits.SetBit(UNIT_FIELD_POWER5);
    updateVisualBits.SetBit(UNIT_FIELD_MAXHEALTH);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER1);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER2);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER3);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER4);
    updateVisualBits.SetBit(UNIT_FIELD_MAXPOWER5);
    updateVisualBits.SetBit(UNIT_FIELD_LEVEL);
    updateVisualBits.SetBit(UNIT_FIELD_FACTIONTEMPLATE);
    updateVisualBits.SetBit(UNIT_VIRTUAL_ITEM_SLOT_ID + 0);
    updateVisualBits.SetBit(UNIT_VIRTUAL_ITEM_SLOT_ID + 1);
    updateVisualBits.SetBit(UNIT_VIRTUAL_ITEM_SLOT_ID + 2);
    updateVisualBits.SetBit(UNIT_FIELD_FLAGS);
    updateVisualBits.SetBit(UNIT_FIELD_FLAGS_2);
    updateVisualBits.SetBit(UNIT_FIELD_AURASTATE);
    updateVisualBits.SetBit(UNIT_FIELD_BASEATTACKTIME + 0);
    updateVisualBits.SetBit(UNIT_FIELD_BASEATTACKTIME + 1);
    updateVisualBits.SetBit(UNIT_FIELD_BOUNDINGRADIUS);
    updateVisualBits.SetBit(UNIT_FIELD_COMBATREACH);
    updateVisualBits.SetBit(UNIT_FIELD_DISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_NATIVEDISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_MOUNTDISPLAYID);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_1);
    updateVisualBits.SetBit(UNIT_FIELD_PETNUMBER);
    updateVisualBits.SetBit(UNIT_FIELD_PET_NAME_TIMESTAMP);
    updateVisualBits.SetBit(UNIT_DYNAMIC_FLAGS);
    updateVisualBits.SetBit(UNIT_CHANNEL_SPELL);
    updateVisualBits.SetBit(UNIT_MOD_CAST_SPEED);
    updateVisualBits.SetBit(UNIT_NPC_FLAGS);
    updateVisualBits.SetBit(UNIT_FIELD_BASE_MANA);
    updateVisualBits.SetBit(UNIT_FIELD_BYTES_2);
    updateVisualBits.SetBit(UNIT_FIELD_HOVERHEIGHT);

    updateVisualBits.SetBit(PLAYER_DUEL_ARBITER + 0);
    updateVisualBits.SetBit(PLAYER_DUEL_ARBITER + 1);
    updateVisualBits.SetBit(PLAYER_FLAGS);
    //updateVisualBits.SetBit(PLAYER_GUILDID);
    updateVisualBits.SetBit(PLAYER_GUILDRANK);
    updateVisualBits.SetBit(PLAYER_GUILDLEVEL);
    updateVisualBits.SetBit(PLAYER_BYTES);
    updateVisualBits.SetBit(PLAYER_BYTES_2);
    updateVisualBits.SetBit(PLAYER_BYTES_3);
    updateVisualBits.SetBit(PLAYER_DUEL_TEAM);
    updateVisualBits.SetBit(PLAYER_GUILD_TIMESTAMP);
    updateVisualBits.SetBit(UNIT_NPC_FLAGS);

    // PLAYER_QUEST_LOG_x also visible bit on official (but only on party/raid)...
    for (uint16 i = PLAYER_QUEST_LOG_1_1; i < PLAYER_QUEST_LOG_25_2; i += MAX_QUEST_OFFSET)
    {
        updateVisualBits.SetBit(i);
    }

    // Players visible items are not inventory stuff
    for (uint16 i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        uint32 offset = i * 2;

        // item entry
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_ENTRYID + offset);
        // enchant
        updateVisualBits.SetBit(PLAYER_VISIBLE_ITEM_1_ENCHANTMENT + offset);
    }

    updateVisualBits.SetBit(PLAYER_CHOSEN_TITLE);
}

/**
 * @brief Builds the create update block for a player observer.
 *
 * @param data The update data buffer to append to.
 * @param target The player receiving the create block.
 */
void Player::BuildCreateUpdateBlockForPlayer(UpdateData* data, Player* target) const
{
    if (target == this)
    {
        for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
        {
            if (m_items[i] == NULL)
            {
                continue;
            }

            m_items[i]->BuildCreateUpdateBlockForPlayer(data, target);
        }
        for (int i = INVENTORY_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (m_items[i] == NULL)
            {
                continue;
            }

            m_items[i]->BuildCreateUpdateBlockForPlayer(data, target);
        }
    }

    Unit::BuildCreateUpdateBlockForPlayer(data, target);
}

/**
 * @brief Builds destroy updates for the player and visible inventory objects.
 *
 * @param target The player that should receive the destroy updates.
 */
void Player::DestroyForPlayer(Player* target, bool anim) const
{
    Unit::DestroyForPlayer(target, anim);

    for (int i = 0; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (m_items[i] == NULL)
        {
            continue;
        }

        m_items[i]->DestroyForPlayer(target);
    }

    if (target == this)
    {
        for (int i = INVENTORY_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (m_items[i] == NULL)
            {
                continue;
            }

            m_items[i]->DestroyForPlayer(target);
        }
    }
}

/**
 * @brief Checks whether the player knows a spell.
 *
 * @param spell The spell identifier to test.
 * @return True if the spell exists and is not removed or disabled; otherwise, false.
 */
bool Player::HasSpell(uint32 spell) const
{
    PlayerSpellMap::const_iterator itr = m_spells.find(spell);
    return (itr != m_spells.end() && itr->second.state != PLAYERSPELL_REMOVED &&
            !itr->second.disabled);
}

bool Player::HasTalent(uint32 spell, uint8 spec) const
{
    PlayerTalentMap::const_iterator itr = m_talents[spec].find(spell);
    return (itr != m_talents[spec].end() && itr->second.state != PLAYERSPELL_REMOVED);
}

/**
 * @brief Checks whether the player has a spell active in the spellbook.
 *
 * @param spell The spell identifier to test.
 * @return True if the spell is known, active, and not disabled; otherwise, false.
 */
bool Player::HasActiveSpell(uint32 spell) const
{
    PlayerSpellMap::const_iterator itr = m_spells.find(spell);
    return (itr != m_spells.end() && itr->second.state != PLAYERSPELL_REMOVED &&
            itr->second.active && !itr->second.disabled);
}

/**
 * @brief Evaluates whether a trainer spell can be learned by the player.
 *
 * @param trainer_spell The trainer spell entry being checked.
 * @param reqLevel An optional override for the required level.
 * @return The trainer spell state to display to the client.
 */
TrainerSpellState Player::GetTrainerSpellState(TrainerSpell const* trainer_spell, uint32 reqLevel) const
{
    if (!trainer_spell)
    {
        return TRAINER_SPELL_RED;
    }

    if (!trainer_spell->learnedSpell)
    {
        return TRAINER_SPELL_RED;
    }

    // known spell
    if (HasSpell(trainer_spell->learnedSpell))
    {
        return TRAINER_SPELL_GRAY;
    }

    // check race/class requirement
    if (!IsSpellFitByClassAndRace(trainer_spell->learnedSpell))
    {
        return TRAINER_SPELL_RED;
    }

    if (SpellChainNode const* spell_chain = sSpellMgr.GetSpellChainNode(trainer_spell->learnedSpell))
    {
        // check prev.rank requirement
        if (spell_chain->prev && !HasSpell(spell_chain->prev))
        {
            return TRAINER_SPELL_RED;
        }

        // check additional spell requirement
        if (spell_chain->req && !HasSpell(spell_chain->req))
        {
            return TRAINER_SPELL_RED;
        }
    }

    // check level requirement
    //
    // The previous check `trainer_spell->reqLevel < reqLevel` compared two
    // values that both derive from trainer_spell itself (the right-hand
    // `reqLevel` parameter is computed in NPCHandler.cpp as
    // `max(class-fit reqLevel, trainer_spell->reqLevel)`), so the
    // comparison was effectively `X < X` -- always false -- and the
    // player's actual level was never checked. Mirror mangostwo /
    // TC-Preservation and compare the player's level against the
    // effective required level instead.
    //
    // The bare `prof ||` short-circuit-RED has been dropped: it forced
    // profession spells to RED unconditionally, which never matched
    // observed behaviour (the destructive bug was that the level check
    // was a no-op, masking that line entirely for class spells where
    // prof == false). mangostwo additionally gates the level check on a
    // GM-trade-skill-bypass config; that is out of scope here.
    bool prof = SpellMgr::IsProfessionSpell(trainer_spell->spell);
    if (getLevel() < reqLevel)
    {
        return TRAINER_SPELL_RED;
    }

    // check skill requirement
    if (prof || trainer_spell->reqSkill && GetBaseSkillValue(trainer_spell->reqSkill) < trainer_spell->reqSkillValue)
    {
        return TRAINER_SPELL_RED;
    }

    // exist, already checked at loading
    SpellEntry const* spell = sSpellStore.LookupEntry(trainer_spell->learnedSpell);

    // secondary prof. or not prof. spell
    SpellEffectEntry const* spellEffect = spell->GetSpellEffect(EFFECT_INDEX_1);
    uint32 skill = spellEffect ? spellEffect->EffectMiscValue : 0;

    if (spellEffect && (spellEffect->Effect != SPELL_EFFECT_SKILL || !IsPrimaryProfessionSkill(skill)))
    {
        return TRAINER_SPELL_GREEN;
    }

    // check primary prof. limit
    if (sSpellMgr.IsPrimaryProfessionFirstRankSpell(spell->Id) && GetFreePrimaryProfessionPoints() == 0)
    {
        return TRAINER_SPELL_GREEN_DISABLED;
    }

    return TRAINER_SPELL_GREEN;
}

/**
 * Deletes a character from the database
 *
 * The way, how the characters will be deleted is decided based on the config option.
 *
 * @see Player::DeleteOldCharacters
 *
 * @param playerguid       the low-GUID from the player which should be deleted
 * @param accountId        the account id from the player
 * @param updateRealmChars when this flag is set, the amount of characters on that realm will be updated in the realmlist
 * @param deleteFinally    if this flag is set, the config option will be ignored and the character will be permanently removed from the database
 */
void Player::DeleteFromDB(ObjectGuid playerguid, uint32 accountId, bool updateRealmChars, bool deleteFinally)
{
    //Make sure to delete unresolved tickets so they don't take up place in the open tickets list
    CharacterDatabase.PExecute("DELETE FROM `character_ticket` "
                               "WHERE `resolved` = 0 AND `guid` = %u",
                               playerguid.GetCounter());

    // for nonexistent account avoid update realm
    if (accountId == 0)
    {
        updateRealmChars = false;
    }

    uint32 charDelete_method = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_METHOD);
    uint32 charDelete_minLvl = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_MIN_LEVEL);

    // if we want to finally delete the character or the character does not meet the level requirement, we set it to mode 0
    if (deleteFinally || Player::GetLevelFromDB(playerguid) < charDelete_minLvl)
    {
        charDelete_method = 0;
    }

    uint32 lowguid = playerguid.GetCounter();

    // convert corpse to bones if exist (to prevent exiting Corpse in World without DB entry)
    // bones will be deleted by corpse/bones deleting thread shortly
    sObjectAccessor.ConvertCorpseForPlayer(playerguid);

    // remove from guild
    if (uint32 guildId = GetGuildIdFromDB(playerguid))
    {
        if (Guild* guild = sGuildMgr.GetGuildById(guildId))
        {
            if (guild->DelMember(playerguid))
            {
                guild->Disband();
                delete guild;
            }
        }
    }

    // remove from arena teams
    LeaveAllArenaTeams(playerguid);

    // the player was uninvited already on logout so just remove from group
    QueryResult* resultGroup = CharacterDatabase.PQuery("SELECT `groupId` FROM `group_member` WHERE `memberGuid`='%u'", lowguid);
    if (resultGroup)
    {
        uint32 groupId = (*resultGroup)[0].GetUInt32();
        delete resultGroup;
        if (Group* group = sObjectMgr.GetGroupById(groupId))
        {
            RemoveFromGroup(group, playerguid);
        }
    }

    // remove signs from petitions (also remove petitions if owner);
    RemovePetitionsAndSigns(playerguid);

    switch (charDelete_method)
    {
            // completely remove from the database
        case 0:
        {
            // return back all mails with COD and Item                  0    1             2                3        4         5      6       7
            QueryResult* resultMail = CharacterDatabase.PQuery("SELECT `id`,`messageType`,`mailTemplateId`,`sender`,`subject`,`body`,`money`,`has_items` FROM `mail` WHERE `receiver`='%u' AND `has_items`<>0 AND `cod`<>0", lowguid);
            if (resultMail)
            {
                do
                {
                    Field* fields = resultMail->Fetch();

                    uint32 mail_id       = fields[0].GetUInt32();
                    uint16 mailType      = fields[1].GetUInt16();
                    uint16 mailTemplateId = fields[2].GetUInt16();
                    uint32 sender        = fields[3].GetUInt32();
                    std::string subject  = fields[4].GetCppString();
                    std::string body     = fields[5].GetCppString();
                    uint64 money         = fields[6].GetUInt32();
                    bool has_items       = fields[7].GetBool();

                    // we can return mail now
                    // so firstly delete the old one
                    CharacterDatabase.PExecute("DELETE FROM `mail` WHERE `id` = '%u'", mail_id);

                    // mail not from player
                    if (mailType != MAIL_NORMAL)
                    {
                        if (has_items)
                        {
                            CharacterDatabase.PExecute("DELETE FROM `mail_items` WHERE `mail_id` = '%u'", mail_id);
                        }
                        continue;
                    }

                    MailDraft draft;
                    if (mailTemplateId)
                    {
                        draft.SetMailTemplate(mailTemplateId, false); // items already included
                    }
                    else
                    {
                        draft.SetSubjectAndBody(subject, body);
                    }

                    if (has_items)
                    {
                        // data needs to be at first place for Item::LoadFromDB
                        //                                                           0      1      2           3
                        QueryResult* resultItems = CharacterDatabase.PQuery("SELECT `data`,`text`,`item_guid`,`item_template` FROM `mail_items` JOIN `item_instance` ON `item_guid` = `guid` WHERE `mail_id`='%u'", mail_id);
                        if (resultItems)
                        {
                            do
                            {
                                Field* fields2 = resultItems->Fetch();

                                uint32 item_guidlow = fields2[2].GetUInt32();
                                uint32 item_template = fields2[3].GetUInt32();

                                ItemPrototype const* itemProto = ObjectMgr::GetItemPrototype(item_template);
                                if (!itemProto)
                                {
                                    CharacterDatabase.PExecute("DELETE FROM `item_instance` WHERE `guid` = '%u'", item_guidlow);
                                    continue;
                                }

                                Item* pItem = NewItemOrBag(itemProto);
                                if (!pItem->LoadFromDB(item_guidlow, fields2, playerguid))
                                {
                                    pItem->FSetState(ITEM_REMOVED);
                                    pItem->SaveToDB();      // it also deletes item object !
                                    continue;
                                }

                                draft.AddItem(pItem);
                            }
                            while (resultItems->NextRow());

                            delete resultItems;
                        }
                    }

                    CharacterDatabase.PExecute("DELETE FROM `mail_items` WHERE `mail_id` = '%u'", mail_id);

                    uint32 pl_account = sObjectMgr.GetPlayerAccountIdByGUID(playerguid);

                    draft.SetMoney(money).SendReturnToSender(pl_account, playerguid, ObjectGuid(HIGHGUID_PLAYER, sender));
                }
                while (resultMail->NextRow());

                delete resultMail;
            }

            // unsummon and delete for pets in world is not required: player deleted from CLI or character list with not loaded pet.
            // Get guids of character's pets, will deleted in transaction
            QueryResult* resultPets = CharacterDatabase.PQuery("SELECT `id` FROM `character_pet` WHERE `owner` = '%u'", lowguid);

            // delete char from friends list when selected chars is online (non existing - error)
            QueryResult* resultFriend = CharacterDatabase.PQuery("SELECT DISTINCT `guid` FROM `character_social` WHERE `friend` = '%u'", lowguid);

            // NOW we can finally clear other DB data related to character
            CharacterDatabase.BeginTransaction();
            if (resultPets)
            {
                do
                {
                    Field* fields3 = resultPets->Fetch();
                    uint32 petguidlow = fields3[0].GetUInt32();
                    // do not create separate transaction for pet delete otherwise we will get fatal error!
                    Pet::DeleteFromDB(petguidlow, false);
                }
                while (resultPets->NextRow());
                delete resultPets;
            }

            // cleanup friends for online players, offline case will cleanup later in code
            if (resultFriend)
            {
                do
                {
                    Field* fieldsFriend = resultFriend->Fetch();
                    if (Player* sFriend = sObjectAccessor.FindPlayer(ObjectGuid(HIGHGUID_PLAYER, fieldsFriend[0].GetUInt32())))
                    {
                        if (sFriend->IsInWorld())
                        {
                            sFriend->GetSocial()->RemoveFromSocialList(playerguid, false);
                            sSocialMgr.SendFriendStatus(sFriend, FRIEND_REMOVED, playerguid, false);
                        }
                    }
                }
                while (resultFriend->NextRow());
                delete resultFriend;
            }

            CharacterDatabase.PExecute("DELETE FROM `characters` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_account_data` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_declinedname` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_action` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_aura` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_battleground_data` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_gifts` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_glyphs` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_homebind` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_instance` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `group_instance` WHERE `leaderGuid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_inventory` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_queststatus` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_queststatus_daily` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_queststatus_weekly` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_reputation` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_skills` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_spell` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_spell_cooldown` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_talent` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_ticket` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `item_instance` WHERE `owner_guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_social` WHERE `guid` = '%u' OR `friend`='%u'", lowguid, lowguid);
            CharacterDatabase.PExecute("DELETE FROM `mail` WHERE `receiver` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `mail_items` WHERE `receiver` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_pet` WHERE `owner` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_pet_declinedname` WHERE `owner` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_achievement` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_achievement_progress` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_equipmentsets` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `guild_eventlog` WHERE `PlayerGuid1` = '%u' OR `PlayerGuid2` = '%u'", lowguid, lowguid);
            CharacterDatabase.PExecute("DELETE FROM `guild_bank_eventlog` WHERE `PlayerGuid` = '%u'", lowguid);
            CharacterDatabase.PExecute("DELETE FROM `character_currencies` WHERE `guid` = '%u'", lowguid);
            CharacterDatabase.CommitTransaction();
            break;
        }
        // The character gets unlinked from the account, the name gets freed up and appears as deleted ingame
        case 1:
            CharacterDatabase.PExecute("UPDATE `characters` SET `deleteInfos_Name`=`name`, `deleteInfos_Account`=`account`, `deleteDate`='" UI64FMTD "', `name`='', `account`=0 WHERE `guid`=%u", uint64(time(NULL)), lowguid);
            break;
        default:
            sLog.outError("Player::DeleteFromDB: Unsupported delete method: %u.", charDelete_method);
    }

    if (updateRealmChars)
    {
        sWorld.UpdateRealmCharCount(accountId);
    }
}

/**
 * Characters which were kept back in the database after being deleted and are now too old (see config option "CharDelete.KeepDays"), will be completely deleted.
 *
 * @see Player::DeleteFromDB
 */
void Player::DeleteOldCharacters()
{
    uint32 keepDays = sWorld.getConfig(CONFIG_UINT32_CHARDELETE_KEEP_DAYS);
    if (!keepDays)
    {
        return;
    }

    Player::DeleteOldCharacters(keepDays);
}

/**
 * Characters which were kept back in the database after being deleted and are older than the specified amount of days, will be completely deleted.
 *
 * @see Player::DeleteFromDB
 *
 * @param keepDays overrite the config option by another amount of days
 */
void Player::DeleteOldCharacters(uint32 keepDays)
{
    sLog.outString("Player::DeleteOldChars: Deleting all characters which have been deleted %u days before...", keepDays);

    QueryResult* resultChars = CharacterDatabase.PQuery("SELECT `guid`, `deleteInfos_Account` FROM `characters` WHERE `deleteDate` IS NOT NULL AND `deleteDate` < '" UI64FMTD "'", uint64(time(NULL) - time_t(keepDays * DAY)));
    if (resultChars)
    {
        sLog.outString("Player::DeleteOldChars: Found %u character(s) to delete", uint32(resultChars->GetRowCount()));
        do
        {
            Field* charFields = resultChars->Fetch();
            ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, charFields[0].GetUInt32());
            Player::DeleteFromDB(guid, charFields[1].GetUInt32(), true, true);
        }
        while (resultChars->NextRow());
        delete resultChars;
    }
    sLog.outString();
}

/**
 * @brief Forces or clears rooted movement for the player.
 *
 * @param enable True to root the player; false to unroot them.
 */
void Player::SetRoot(bool enable)
{
    WorldPacket data;
    BuildForceMoveRootPacket(&data, enable, 0);
    SendMessageToSet(&data, true);
}

/**
 * @brief Enables or disables water walking for the player.
 *
 * @param enable True to enable water walking; false to restore normal movement.
 */
void Player::SetWaterWalk(bool enable)
{
    WorldPacket data;
    BuildMoveWaterWalkPacket(&data, enable, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Placeholder for levitation support on this client version.
 *
 * @param enable Unused levitation state flag.
 */
void Player::SetLevitate(bool enable)
{
    WorldPacket data;
    BuildMoveLevitatePacket(&data, enable, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Enables or disables flying movement flags for the player.
 *
 * @param enable True to enable flight-related movement flags; false to clear them.
 */
void Player::SetCanFly(bool enable)
{
    WorldPacket data;
    BuildMoveSetCanFlyPacket(&data, enable, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Enables or disables feather fall movement for the player.
 *
 * @param enable True to enable feather fall; false to restore normal falling.
 */
void Player::SetFeatherFall(bool enable)
{
    WorldPacket data;
    BuildMoveFeatherFallPacket(&data, enable, 0);
    SendMessageToSet(&data, true);

    // start fall from current height
    if (!enable)
    {
        SetFallInformation(0, GetPositionZ());
    }
}

/**
 * @brief Enables or disables hover movement for the player.
 *
 * @param enable True to enable hovering; false to disable it.
 */
void Player::SetHover(bool enable)
{
    WorldPacket data;
    BuildMoveHoverPacket(&data, enable, 0);
    GetSession()->SendPacket(&data);
}

/* Preconditions:
  - a resurrectable corpse must not be loaded for the player (only bones)
  - the player must be in world
*/
void Player::BuildPlayerRepop()
{
    WorldPacket data(SMSG_PRE_RESURRECT, GetPackGUID().size());
    data << GetPackGUID();
    GetSession()->SendPacket(&data);

    if (getRace() == RACE_NIGHTELF)
    {
        CastSpell(this, 20584, true); // auras SPELL_AURA_INCREASE_SPEED(+speed in wisp form), SPELL_AURA_INCREASE_SWIM_SPEED(+swim speed in wisp form), SPELL_AURA_TRANSFORM (to wisp form)
    }
    CastSpell(this, 8326, true);                            // auras SPELL_AURA_GHOST, SPELL_AURA_INCREASE_SPEED(why?), SPELL_AURA_INCREASE_SWIM_SPEED(why?)

    // there must be SMSG.FORCE_RUN_SPEED_CHANGE, SMSG.FORCE_SWIM_SPEED_CHANGE, SMSG.MOVE_WATER_WALK
    // there must be SMSG.STOP_MIRROR_TIMER
    // there we must send 888 opcode

    // the player can not have a corpse already, only bones which are not returned by GetCorpse
    if (GetCorpse())
    {
        sLog.outError("BuildPlayerRepop: player %s(%d) already has a corpse", GetName(), GetGUIDLow());
        MANGOS_ASSERT(false);
    }

    // create a corpse and place it at the player's location
    Corpse* corpse = CreateCorpse();
    if (!corpse)
    {
        sLog.outError("Error creating corpse for Player %s [%u]", GetName(), GetGUIDLow());
        return;
    }
    GetMap()->Add(corpse);

    // convert player body to ghost
    SetHealth(1);

    SetWaterWalk(true);
    if (!GetSession()->isLogingOut())
    {
        SetRoot(false);
    }

    // BG - remove insignia related
    RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

    SendCorpseReclaimDelay();

    // to prevent cheating
    corpse->ResetGhostTime();

    StopMirrorTimers();                                     // disable timers(bars)

    // set and clear other
    SetByteValue(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND);
}

/**
 * @brief Restores the player to life and reapplies post-resurrection effects.
 *
 * @param restore_percent The fraction of health and resources to restore.
 * @param applySickness True to apply resurrection sickness when appropriate.
 */
void Player::ResurrectPlayer(float restore_percent, bool applySickness)
{
    WorldPacket data(SMSG_DEATH_RELEASE_LOC, 4 * 4);        // remove spirit healer position
    data << uint32(-1);
    data << float(0);
    data << float(0);
    data << float(0);
    GetSession()->SendPacket(&data);

    // speed change, land walk

    // remove death flag + set aura
    SetByteValue(UNIT_FIELD_BYTES_1, 3, 0x00);

    SetDeathState(ALIVE);

    if (getRace() == RACE_NIGHTELF)
    {
        RemoveAurasDueToSpell(20584); // speed bonuses
    }
    RemoveAurasDueToSpell(8326);                            // SPELL_AURA_GHOST

    SetWaterWalk(false);
    SetRoot(false);

    // set health/powers (0- will be set in caller)
    if (restore_percent > 0.0f)
    {
        SetHealth(uint32(GetMaxHealth()*restore_percent));
        SetPower(POWER_MANA, uint32(GetMaxPower(POWER_MANA)*restore_percent));
        SetPower(POWER_RAGE, 0);
        SetPower(POWER_ENERGY, uint32(GetMaxPower(POWER_ENERGY)*restore_percent));
    }

    // trigger update zone for alive state zone updates
    uint32 newzone, newarea;
    GetZoneAndAreaId(newzone, newarea);
    UpdateZone(newzone, newarea);

    m_deathTimer = 0;

    // update visibility of world around viewpoint
    m_camera.UpdateVisibilityForOwner();
    // update visibility of player for nearby cameras
    UpdateObjectVisibility();

#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnResurrect(this);
    }
#endif /* ENABLE_ELUNA */

    if (!applySickness)
    {
        return;
    }

    // Characters from level 1-10 are not affected by resurrection sickness.
    // Characters from level 11-19 will suffer from one minute of sickness
    // for each level they are above 10.
    // Characters level 20 and up suffer from ten minutes of sickness.
    int32 startLevel = sWorld.getConfig(CONFIG_INT32_DEATH_SICKNESS_LEVEL);

    if (int32(getLevel()) >= startLevel)
    {
        // set resurrection sickness
        CastSpell(this, SPELL_ID_PASSIVE_RESURRECTION_SICKNESS, true);

        // not full duration
        if (int32(getLevel()) < startLevel + 9)
        {
            int32 delta = (int32(getLevel()) - startLevel + 1) * MINUTE;

            if (SpellAuraHolder* holder = GetSpellAuraHolder(SPELL_ID_PASSIVE_RESURRECTION_SICKNESS))
            {
                holder->SetAuraDuration(delta * IN_MILLISECONDS);
                holder->SendAuraUpdate(false);
            }
        }
    }
}

/**
 * @brief Transitions the player into the corpse state after death.
 */
void Player::KillPlayer()
{
    SetRoot(true);

    StopMirrorTimers();                                     // disable timers(bars)

    SetDeathState(CORPSE);
    // SetFlag( UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_IN_PVP );

    SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);
    ApplyModByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_RELEASE_TIMER, !sMapStore.LookupEntry(GetMapId())->Instanceable() && !HasAuraType(SPELL_AURA_PREVENT_RESURRECTION));

    // 6 minutes until repop at graveyard
    m_deathTimer = 6 * MINUTE * IN_MILLISECONDS;

    UpdateCorpseReclaimDelay();                             // dependent at use SetDeathPvP() call before kill

    // don't create corpse at this moment, player might be falling

    // update visibility
    UpdateObjectVisibility();
}

/**
 * @brief Creates a corpse object for the player's current death state.
 *
 * @return The created corpse, or null if creation failed.
 */
Corpse* Player::CreateCorpse()
{
    // prevent existence 2 corpse for player
    SpawnCorpseBones();

    Corpse* corpse = new Corpse((m_ExtraFlags & PLAYER_EXTRA_PVP_DEATH) ? CORPSE_RESURRECTABLE_PVP : CORPSE_RESURRECTABLE_PVE);
    SetPvPDeath(false);

    if (!corpse->Create(sObjectMgr.GenerateCorpseLowGuid(), this))
    {
        delete corpse;
        return NULL;
    }

    uint8 skin       = GetByteValue(PLAYER_BYTES, 0);
    uint8 face       = GetByteValue(PLAYER_BYTES, 1);
    uint8 hairstyle  = GetByteValue(PLAYER_BYTES, 2);
    uint8 haircolor  = GetByteValue(PLAYER_BYTES, 3);
    uint8 facialhair = GetByteValue(PLAYER_BYTES_2, 0);

    corpse->SetByteValue(CORPSE_FIELD_BYTES_1, 1, getRace());
    corpse->SetByteValue(CORPSE_FIELD_BYTES_1, 2, getGender());
    corpse->SetByteValue(CORPSE_FIELD_BYTES_1, 3, skin);

    corpse->SetByteValue(CORPSE_FIELD_BYTES_2, 0, face);
    corpse->SetByteValue(CORPSE_FIELD_BYTES_2, 1, hairstyle);
    corpse->SetByteValue(CORPSE_FIELD_BYTES_2, 2, haircolor);
    corpse->SetByteValue(CORPSE_FIELD_BYTES_2, 3, facialhair);

    uint32 flags = CORPSE_FLAG_UNK2;
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM))
    {
        flags |= CORPSE_FLAG_HIDE_HELM;
    }
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK))
    {
        flags |= CORPSE_FLAG_HIDE_CLOAK;
    }
    if (InBattleGround() && !InArena())
    {
        flags |= CORPSE_FLAG_LOOTABLE; // to be able to remove insignia
    }
    corpse->SetUInt32Value(CORPSE_FIELD_FLAGS, flags);

    corpse->SetUInt32Value(CORPSE_FIELD_DISPLAY_ID, GetNativeDisplayId());

    uint32 iDisplayID;
    uint32 iIventoryType;
    uint32 _cfi;
    for (int i = 0; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (m_items[i])
        {
            iDisplayID = m_items[i]->GetProto()->DisplayInfoID;
            iIventoryType = m_items[i]->GetProto()->InventoryType;

            _cfi =  iDisplayID | (iIventoryType << 24);
            corpse->SetUInt32Value(CORPSE_FIELD_ITEM + i, _cfi);
        }
    }

    // we not need saved corpses for BG/arenas
    if (!GetMap()->IsBattleGroundOrArena())
    {
        corpse->SaveToDB();
    }

    // register for player, but not show
    sObjectAccessor.AddCorpse(corpse);
    return corpse;
}

/**
 * @brief Converts an existing corpse into bones and persists the ghost state if needed.
 */
void Player::SpawnCorpseBones()
{
    if (sObjectAccessor.ConvertCorpseForPlayer(GetObjectGuid()))
        if (!GetSession()->PlayerLogoutWithSave())          // at logout we will already store the player
        {
            SaveToDB(); // prevent loading as ghost without corpse
        }
}

/**
 * @brief Gets the player's current corpse object.
 *
 * @return The player's corpse, or null if none exists.
 */
Corpse* Player::GetCorpse() const
{
    return sObjectAccessor.GetCorpseForPlayerGUID(GetObjectGuid());
}


/**
 * @brief Moves the player to the nearest valid graveyard.
 */
void Player::RepopAtGraveyard()
{
    // note: this can be called also when the player is alive
    // for example from WorldSession::HandleMovementOpcodes

    AreaTableEntry const* zone = GetAreaEntryByAreaID(GetAreaId());

    // Such zones are considered unreachable as a ghost and the player must be automatically revived
    if ((!IsAlive() && zone && zone->flags & AREA_FLAG_NEED_FLY) || GetTransport())
    {
        ResurrectPlayer(0.5f);
        SpawnCorpseBones();
    }

    WorldSafeLocsEntry const* ClosestGrave = NULL;

    // Special handle for battleground maps
    if (BattleGround* bg = GetBattleGround())
    {
        ClosestGrave = bg->GetClosestGraveYard(this);
    }
    else
    {
        ClosestGrave = sObjectMgr.GetClosestGraveYard(GetPositionX(), GetPositionY(), GetPositionZ(), GetMapId(), GetTeam());
    }

    // stop countdown until repop
    m_deathTimer = 0;

    // if no grave found, stay at the current location
    // and don't show spirit healer location
    if (ClosestGrave)
    {
        bool updateVisibility = IsInWorld() && GetMapId() == ClosestGrave->map_id;
        TeleportTo(ClosestGrave->map_id, ClosestGrave->x, ClosestGrave->y, ClosestGrave->z, GetOrientation());
        if (IsDead())                                       // not send if alive, because it used in TeleportTo()
        {
            WorldPacket data(SMSG_DEATH_RELEASE_LOC, 4 * 4);// show spirit healer position on minimap
            data << ClosestGrave->map_id;
            data << ClosestGrave->x;
            data << ClosestGrave->y;
            data << ClosestGrave->z;
            GetSession()->SendPacket(&data);
        }
        if (updateVisibility && IsInWorld())
        {
            UpdateVisibilityAndView();
        }
    }
}

/**
 * @brief Registers a joined chat channel on the player.
 *
 * @param c The channel that was joined.
 */
void Player::JoinedChannel(Channel* c)
{
    m_channels.push_back(c);
}

/**
 * @brief Removes a chat channel from the player's joined channel list.
 *
 * @param c The channel that was left.
 */
void Player::LeftChannel(Channel* c)
{
    m_channels.remove(c);
}

/**
 * @brief Leaves and cleans up all channels currently joined by the player.
 */
void Player::CleanupChannels()
{
    while (!m_channels.empty())
    {
        Channel* ch = *m_channels.begin();
        m_channels.erase(m_channels.begin());               // remove from player's channel list
        ch->Leave(this, false);                             // not send to client, not remove from player's channel list
        if (ChannelMgr* cMgr = channelMgr(GetTeam()))
        {
            cMgr->LeftChannel(ch->GetName()); // deleted channel if empty
        }
    }
    DEBUG_LOG("Player: channels cleaned up!");
}

/**
 * @brief Updates built-in local channels after a zone change.
 *
 * @param newZone The new area identifier used for localized channel names.
 */
void Player::UpdateLocalChannels(uint32 newZone)
{
    if (m_channels.empty())
    {
        return;
    }

    AreaTableEntry const* current_zone = GetAreaEntryByAreaID(newZone);
    if (!current_zone)
    {
        return;
    }

    ChannelMgr* cMgr = channelMgr(GetTeam());
    if (!cMgr)
    {
        return;
    }

    std::string current_zone_name = current_zone->area_name[GetSession()->GetSessionDbcLocale()];

    for (JoinedChannelsList::iterator i = m_channels.begin(), next; i != m_channels.end(); i = next)
    {
        next = i; ++next;

        // skip non built-in channels
        if (!(*i)->IsConstant())
        {
            continue;
        }

        ChatChannelsEntry const* ch = GetChannelEntryFor((*i)->GetChannelId());
        if (!ch)
        {
            continue;
        }

        if ((ch->flags & 4) == 4)                           // global channel without zone name in pattern
        {
            continue;
        }

        //  new channel
        char new_channel_name_buf[100];
        snprintf(new_channel_name_buf, 100, ch->pattern[m_session->GetSessionDbcLocale()], current_zone_name.c_str());
        Channel* new_channel = cMgr->GetJoinChannel(new_channel_name_buf, ch->ChannelID);

        if ((*i) != new_channel)
        {
            new_channel->Join(this, "");                    // will output Changed Channel: N. Name

            // leave old channel
            (*i)->Leave(this, false);                       // not send leave channel, it already replaced at client
            std::string name = (*i)->GetName();             // store name, (*i)erase in LeftChannel
            LeftChannel(*i);                                // remove from player's channel list
            cMgr->LeftChannel(name);                        // delete if empty
        }
    }
    DEBUG_LOG("Player: channels cleaned up!");
}

/**
 * @brief Leaves the currently joined looking-for-group channel, if any.
 */
void Player::LeaveLFGChannel()
{
    for (JoinedChannelsList::iterator i = m_channels.begin(); i != m_channels.end(); ++i)
    {
        if ((*i)->IsLFG())
        {
            (*i)->Leave(this);
            break;
        }
    }
}


/* Called from Player::SendInitialPacketsBeforeAddToMap */
/**
 * @brief Sends the player's initial action bar state to the client.
 */
void Player::SendInitialActionButtons() const
{
    DETAIL_LOG("Initializing Action Buttons for '%u' spec '%u'", GetGUIDLow(), m_activeSpec);

    WorldPacket data(SMSG_ACTION_BUTTONS, 1 + (MAX_ACTION_BUTTONS * 4));
    ActionButtonList const& currentActionButtonList = m_actionButtons[m_activeSpec];
    for (uint8 button = 0; button < MAX_ACTION_BUTTONS; ++button)
    {
        /* Try and get each action button the player could have */
        ActionButtonList::const_iterator itr = currentActionButtonList.find(button);

        /* If the button is valid and not deleted */
        if (itr != currentActionButtonList.end() && itr->second.uState != ACTIONBUTTON_DELETED)
        {
            /* Send the data */
            data << uint32(itr->second.packedData);
        }
        else
        {
            /* Nothing to send, so just send 0 */
            data << uint32(0);
        }
    }
    data << uint8(1);                                       // talent spec amount (in packet)
    GetSession()->SendPacket(&data);
    DETAIL_LOG("Action Buttons for '%u' spec '%u' Initialized", GetGUIDLow(), m_activeSpec);
}

void Player::SendLockActionButtons() const
{
    DETAIL_LOG("Locking Action Buttons for '%u' spec '%u'", GetGUIDLow(), m_activeSpec);
    WorldPacket data(SMSG_ACTION_BUTTONS, 1);
    // sending 2 locks actions bars, neither user can remove buttons, nor client removes buttons at spell unlearn
    // they remain locked until server sends new action buttons
    data << uint8(2);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Validates action bar data before it is stored or loaded.
 *
 * @param button The action bar button index.
 * @param action The action identifier assigned to the button.
 * @param type The action button type.
 * @param player The player being validated for, or null for template data.
 * @return True if the action button data is valid; otherwise, false.
 */
bool Player::IsActionButtonDataValid(uint8 button, uint32 action, uint8 type, Player* player, bool msg)
{
    if (button >= MAX_ACTION_BUTTONS)
    {
        if (msg)
        {
            if (player)
                sLog.outError("Action %u not added into button %u for player %s: button must be < %u", action, button, player->GetName(), MAX_ACTION_BUTTONS);
            else
            {
                sLog.outError("Table `playercreateinfo_action` have action %u into button %u : button must be < %u", action, button, MAX_ACTION_BUTTONS);
            }
        }
        return false;
    }

    if (action >= MAX_ACTION_BUTTON_ACTION_VALUE)
    {
        if (msg)
        {
            if (player)
                sLog.outError("Action %u not added into button %u for player %s: action must be < %u", action, button, player->GetName(), MAX_ACTION_BUTTON_ACTION_VALUE);
            else
            {
                sLog.outError("Table `playercreateinfo_action` have action %u into button %u : action must be < %u", action, button, MAX_ACTION_BUTTON_ACTION_VALUE);
            }
        }
        return false;
    }

    switch (type)
    {
        case ACTION_BUTTON_SPELL:
        {
            SpellEntry const* spellProto = sSpellStore.LookupEntry(action);
            if (!spellProto)
            {
                if (msg)
                {
                    if (player)
                        sLog.outError("Spell action %u not added into button %u for player %s: spell not exist", action, button, player->GetName());
                    else
                    {
                        sLog.outError("Table `playercreateinfo_action` have spell action %u into button %u: spell not exist", action, button);
                    }
                }
                return false;
            }

            if (player)
            {
                if (!player->HasSpell(spellProto->Id))
                {
                    if (msg)
                        sLog.outError("Spell action %u not added into button %u for player %s: player don't known this spell", action, button, player->GetName());
                    return false;
                }
                else if (IsPassiveSpell(spellProto))
                {
                    if (msg)
                        sLog.outError("Spell action %u not added into button %u for player %s: spell is passive", action, button, player->GetName());
                    return false;
                }
                // current range for button of totem bar is from ACTION_BUTTON_SHAMAN_TOTEMS_BAR to (but not including) ACTION_BUTTON_SHAMAN_TOTEMS_BAR + 12
                else if (button >= ACTION_BUTTON_SHAMAN_TOTEMS_BAR && button < (ACTION_BUTTON_SHAMAN_TOTEMS_BAR + 12)
                         && !spellProto->HasAttribute(SPELL_ATTR_EX7_TOTEM_SPELL))
                {
                    if (msg)
                        sLog.outError("Spell action %u not added into button %u for player %s: attempt to add non totem spell to totem bar", action, button, player->GetName());
                    return false;
                }
            }
            break;
        }
        case ACTION_BUTTON_ITEM:
        {
            if (!ObjectMgr::GetItemPrototype(action))
            {
                if (msg)
                {
                    if (player)
                        sLog.outError("Item action %u not added into button %u for player %s: item not exist", action, button, player->GetName());
                    else
                    {
                        sLog.outError("Table `playercreateinfo_action` have item action %u into button %u: item not exist", action, button);
                    }
                }
                return false;
            }
            break;
        }
        default:
            break;                                          // other cases not checked at this moment
    }

    return true;
}

/**
 * @brief Adds or updates an action bar button entry.
 *
 * @param button The action bar button index.
 * @param action The action identifier to bind.
 * @param type The action button type.
 * @return The updated action button, or null if validation fails.
 */
ActionButton* Player::addActionButton(uint8 spec, uint8 button, uint32 action, uint8 type)
{
    // check action only for active spec (so not check at copy/load passive spec)
    if (spec == GetActiveSpec() && !IsActionButtonDataValid(button, action, type, this))
    {
        return NULL;
    }

    // it create new button (NEW state) if need or return existing
    ActionButton& ab = m_actionButtons[spec][button];

    // set data and update to CHANGED if not NEW
    ab.SetActionAndType(action, ActionButtonType(type));

    DETAIL_LOG("Player '%u' Added Action '%u' (type %u) to Button '%u' for spec %u", GetGUIDLow(), action, uint32(type), button, spec);
    return &ab;
}

/**
 * @brief Removes an action bar button entry.
 *
 * @param button The action bar button index to remove.
 */
void Player::removeActionButton(uint8 spec, uint8 button)
{
    ActionButtonList& currentActionButtonList = m_actionButtons[spec];
    ActionButtonList::iterator buttonItr = currentActionButtonList.find(button);
    if (buttonItr == currentActionButtonList.end() || buttonItr->second.uState == ACTIONBUTTON_DELETED)
    {
        return;
    }

    if (buttonItr->second.uState == ACTIONBUTTON_NEW)
    {
        currentActionButtonList.erase(buttonItr);           // new and not saved
    }
    else
    {
        buttonItr->second.uState = ACTIONBUTTON_DELETED; // saved, will deleted at next save
    }

    DETAIL_LOG("Action Button '%u' Removed from Player '%u' for spec %u", button, GetGUIDLow(), spec);
}

ActionButton const* Player::GetActionButton(uint8 button)
{
    ActionButtonList& currentActionButtonList = m_actionButtons[m_activeSpec];
    ActionButtonList::iterator buttonItr = currentActionButtonList.find(button);
    if (buttonItr == currentActionButtonList.end() || buttonItr->second.uState == ACTIONBUTTON_DELETED)
    {
        return NULL;
    }

    return &buttonItr->second;
}

/**
 * @brief Moves the player to a new position and updates related state.
 *
 * @param x The destination X coordinate.
 * @param y The destination Y coordinate.
 * @param z The destination Z coordinate.
 * @param orientation The destination facing angle.
 * @param teleport True if the move should be treated as a teleport.
 * @return True if the position update succeeded; otherwise, false.
 */
bool Player::SetPosition(float x, float y, float z, float orientation, bool teleport)
{
    // prevent crash when a bad coord is sent by the client
    if (!MaNGOS::IsValidMapCoord(x, y, z, orientation))
    {
        DEBUG_LOG("Player::SetPosition(%f, %f, %f, %f, %d) .. bad coordinates for player %d!", x, y, z, orientation, teleport, GetGUIDLow());
        return false;
    }

    Map* m = GetMap();

    const float old_x = GetPositionX();
    const float old_y = GetPositionY();
    const float old_z = GetPositionZ();
    const float old_r = GetOrientation();

    if (teleport || old_x != x || old_y != y || old_z != z || old_r != orientation)
    {
        if (teleport || old_x != x || old_y != y || old_z != z)
        {
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_MOVE | AURA_INTERRUPT_FLAG_TURNING);
        }
        else
        {
            RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TURNING);
        }

        RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);

        // move and update visible state if need
        m->PlayerRelocation(this, x, y, z, orientation);

        // reread after Map::Relocation
        m = GetMap();
        x = GetPositionX();
        y = GetPositionY();
        z = GetPositionZ();

        // group update
        if (GetGroup() && (old_x != x || old_y != y))
        {
            SetGroupUpdateFlag(GROUP_UPDATE_FLAG_POSITION);
        }
    }

    if (m_positionStatusUpdateTimer)                        // Update position's state only on interval
    {
        return true;
    }
    m_positionStatusUpdateTimer = 100;

    // code block for underwater state update
    UpdateUnderwaterState(m, x, y, z);

    // code block for outdoor state and area-explore check
    CheckAreaExploreAndOutdoor();

    return true;
}

/**
 * @brief Saves the player's current position as the recall location.
 */
void Player::SaveRecallPosition()
{
    m_recallMap = GetMapId();
    m_recallX = GetPositionX();
    m_recallY = GetPositionY();
    m_recallZ = GetPositionZ();
    m_recallO = GetOrientation();
}

/**
 * @brief Broadcasts a packet to nearby clients and optionally to the player.
 *
 * @param data The packet to send.
 * @param self True to also send the packet to the player session.
 */
void Player::SendMessageToSet(WorldPacket* data, bool self) const
{
    if (IsInWorld())
    {
        GetMap()->MessageBroadcast(this, data, false);
    }

    // if player is not in world and map in not created/already destroyed
    // no need to create one, just send packet for itself!
    if (self)
    {
        GetSession()->SendPacket(data);
    }
}

/**
 * @brief Broadcasts a packet to nearby clients within a distance and optionally to the player.
 *
 * @param data The packet to send.
 * @param dist The maximum broadcast distance.
 * @param self True to also send the packet to the player session.
 */
void Player::SendMessageToSetInRange(WorldPacket* data, float dist, bool self) const
{
    if (IsInWorld())
    {
        GetMap()->MessageDistBroadcast(this, data, dist, false);
    }

    if (self)
    {
        GetSession()->SendPacket(data);
    }
}

/**
 * @brief Broadcasts a packet within range with optional team filtering.
 *
 * @param data The packet to send.
 * @param dist The maximum broadcast distance.
 * @param self True to also send the packet to the player session.
 * @param own_team_only True to restrict delivery to the player's team.
 */
void Player::SendMessageToSetInRange(WorldPacket* data, float dist, bool self, bool own_team_only) const
{
    if (IsInWorld())
    {
        GetMap()->MessageDistBroadcast(this, data, dist, false, own_team_only);
    }

    if (self)
    {
        GetSession()->SendPacket(data);
    }
}

/**
 * @brief Sends a packet directly to the player's session.
 *
 * @param data The packet to send.
 */
void Player::SendDirectMessage(WorldPacket* data) const
{
    GetSession()->SendPacket(data);
}

/**
 * @brief Starts a cinematic sequence for the player client.
 *
 * @param CinematicSequenceId The cinematic sequence identifier.
 */
void Player::SendCinematicStart(uint32 CinematicSequenceId)
{
    WorldPacket data(SMSG_TRIGGER_CINEMATIC, 4);
    data << uint32(CinematicSequenceId);
    SendDirectMessage(&data);
}

#if defined (WOTLK) || defined (CATA) || defined (MISTS)
/**
 * @brief Starts a movie sequence for the player client.
 *
 * @param MovieId The movie identifier.
 */
void Player::SendMovieStart(uint32 MovieId)
{
    WorldPacket data(SMSG_TRIGGER_MOVIE, 4);
    data << uint32(MovieId);
    SendDirectMessage(&data);
}
#endif

/**
 * @brief Updates outdoor-only effects and exploration discovery for the current position.
 */
void Player::CheckAreaExploreAndOutdoor()
{
    if (!IsAlive())
    {
        return;
    }

    if (IsTaxiFlying())
    {
        return;
    }

    bool isOutdoor;
    uint16 areaFlag = GetTerrain()->GetAreaFlag(GetPositionX(), GetPositionY(), GetPositionZ(), &isOutdoor);

    if (isOutdoor)
    {
        if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) && GetRestType() == REST_TYPE_IN_TAVERN)
        {
            AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(inn_trigger_id);
            if (!at || !IsPointInAreaTriggerZone(at, GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ()))
            {
                // Player left inn (REST_TYPE_IN_CITY overrides REST_TYPE_IN_TAVERN, so just clear rest)
                SetRestType(REST_TYPE_NO);
            }
        }
        // Check if we need to reaply outdoor only passive spells
        const PlayerSpellMap& sp_list = GetSpellMap();
        for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
        {
            if (itr->second.state == PLAYERSPELL_REMOVED)
            {
                continue;
            }
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(itr->first);
            if (!spellInfo || !IsNeedCastSpellAtOutdoor(spellInfo) || HasAura(itr->first))
            {
                continue;
            }

            SpellShapeshiftEntry const* shapeShift = spellInfo->GetSpellShapeshift();

            if (!shapeShift || (shapeShift->Stances || shapeShift->StancesNot) && !IsNeedCastSpellAtFormApply(spellInfo, GetShapeshiftForm()))
            {
                continue;
            }
            CastSpell(this, itr->first, true, NULL);
        }
    }
    else if (sWorld.getConfig(CONFIG_BOOL_VMAP_INDOOR_CHECK) && !isGameMaster())
    {
        RemoveAurasWithAttribute(SPELL_ATTR_OUTDOORS_ONLY);
    }

    if (areaFlag == 0xffff)
    {
        return;
    }
    int offset = areaFlag / 32;

    if (offset >= PLAYER_EXPLORED_ZONES_SIZE)
    {
        sLog.outError("Wrong area flag %u in map data for (X: %f Y: %f) point to field PLAYER_EXPLORED_ZONES_1 + %u ( %u must be < %u ).", areaFlag, GetPositionX(), GetPositionY(), offset, offset, PLAYER_EXPLORED_ZONES_SIZE);
        return;
    }

    uint32 val = (uint32)(1 << (areaFlag % 32));
    uint32 currFields = GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset);

    if (!(currFields & val))
    {
        SetUInt32Value(PLAYER_EXPLORED_ZONES_1 + offset, (uint32)(currFields | val));

        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA);

        AreaTableEntry const* p = GetAreaEntryByAreaFlagAndMap(areaFlag, GetMapId());
        if (!p)
        {
            sLog.outError("PLAYER: Player %u discovered unknown area (x: %f y: %f map: %u", GetGUIDLow(), GetPositionX(), GetPositionY(), GetMapId());
        }
        else if (p->area_level > 0)
        {
            uint32 area = p->ID;
            if (getLevel() >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
            {
                SendExplorationExperience(area, 0);
            }
            else
            {
                int32 diff = int32(getLevel()) - p->area_level;
                uint32 XP = 0;
                if (diff < -5)
                {
                    XP = uint32(sObjectMgr.GetBaseXP(getLevel() + 5) * sWorld.getConfig(CONFIG_FLOAT_RATE_XP_EXPLORE));
                }
                else if (diff > 5)
                {
                    int32 exploration_percent = (100 - ((diff - 5) * 5));
                    if (exploration_percent > 100)
                    {
                        exploration_percent = 100;
                    }
                    else if (exploration_percent < 0)
                    {
                        exploration_percent = 0;
                    }

                    XP = uint32(sObjectMgr.GetBaseXP(p->area_level) * exploration_percent / 100 * sWorld.getConfig(CONFIG_FLOAT_RATE_XP_EXPLORE));
                }
                else
                {
                    XP = uint32(sObjectMgr.GetBaseXP(p->area_level) * sWorld.getConfig(CONFIG_FLOAT_RATE_XP_EXPLORE));
                }

                GiveXP(XP, NULL);
                SendExplorationExperience(area, XP);
            }
            DETAIL_LOG("PLAYER: Player %u discovered a new area: %u", GetGUIDLow(), area);
        }
    }
}

/**
 * @brief Gets the faction team associated with a race.
 *
 * @param race The race identifier to evaluate.
 * @return The team assigned to the race.
 */
Team Player::TeamForRace(uint8 race)
{
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    if (!rEntry)
    {
        sLog.outError("Race %u not found in DBC: wrong DBC files?", uint32(race));
        return ALLIANCE;
    }

    switch (rEntry->TeamID)
    {
        case 7: return ALLIANCE;
        case 1: return HORDE;
    }

    sLog.outError("Race %u have wrong teamid %u in DBC: wrong DBC files?", uint32(race), rEntry->TeamID);
    return TEAM_NONE;
}

/**
 * @brief Gets the faction template associated with a race.
 *
 * @param race The race identifier to evaluate.
 * @return The faction template identifier for the race.
 */
uint32 Player::getFactionForRace(uint8 race)
{
    ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(race);
    if (!rEntry)
    {
        sLog.outError("Race %u not found in DBC: wrong DBC files?", uint32(race));
        return 0;
    }

    return rEntry->FactionID;
}

/**
 * @brief Sets the player's team and faction from the specified race.
 *
 * @param race The race identifier to apply.
 */
void Player::setFactionForRace(uint8 race)
{
    m_team = TeamForRace(race);
    setFaction(getFactionForRace(race));
}

/**
 * @brief Gets the player's current reputation rank with a faction.
 *
 * @param faction The faction identifier to query.
 * @return The current reputation rank.
 */
ReputationRank Player::GetReputationRank(uint32 faction) const
{
    FactionEntry const* factionEntry = sFactionStore.LookupEntry(faction);
    return GetReputationMgr().GetRank(factionEntry);
}

// Calculate total reputation percent player gain with quest/creature level
int32 Player::CalculateReputationGain(ReputationSource source, int32 rep, int32 faction, uint32 creatureOrQuestLevel, bool noAuraBonus)
{
    float percent = 100.0f;

    float repMod = noAuraBonus ? 0.0f : (float)GetTotalAuraModifier(SPELL_AURA_MOD_REPUTATION_GAIN);

    // faction specific auras only seem to apply to kills
    if (source == REPUTATION_SOURCE_KILL)
    {
        repMod += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_FACTION_REPUTATION_GAIN, faction);
    }

    percent += rep > 0 ? repMod : -repMod;

    float rate;
    switch (source)
    {
        case REPUTATION_SOURCE_KILL:
            rate = sWorld.getConfig(CONFIG_FLOAT_RATE_REPUTATION_LOWLEVEL_KILL);
            break;
        case REPUTATION_SOURCE_QUEST:
            rate = sWorld.getConfig(CONFIG_FLOAT_RATE_REPUTATION_LOWLEVEL_QUEST);
            break;
        case REPUTATION_SOURCE_SPELL:
        default:
            rate = 1.0f;
            break;
    }

    if (rate != 1.0f && creatureOrQuestLevel <= MaNGOS::XP::GetGrayLevel(getLevel()))
    {
        percent *= rate;
    }

    if (percent <= 0.0f)
    {
        return 0;
    }

    // Multiply result with the faction specific rate
    if (const RepRewardRate* repData = sObjectMgr.GetRepRewardRate(faction))
    {
        float repRate = 0.0f;
        switch (source)
        {
            case REPUTATION_SOURCE_KILL:
                repRate = repData->creature_rate;
                break;
            case REPUTATION_SOURCE_QUEST:
                repRate = repData->quest_rate;
                break;
            case REPUTATION_SOURCE_SPELL:
                repRate = repData->spell_rate;
                break;
        }

        // for custom, a rate of 0.0 will totally disable reputation gain for this faction/type
        if (repRate <= 0.0f)
        {
            return 0;
        }

        percent *= repRate;
    }

    return int32(sWorld.getConfig(CONFIG_FLOAT_RATE_REPUTATION_GAIN) * rep * percent / 100.0f);
}

// Calculates how many reputation points player gains in victim's enemy factions
void Player::RewardReputation(Unit* pVictim, float rate)
{
    if (!pVictim || pVictim->GetTypeId() == TYPEID_PLAYER)
    {
        return;
    }

    // used current difficulty creature entry instead normal version (GetEntry())
    ReputationOnKillEntry const* Rep = sObjectMgr.GetReputationOnKillEntry(((Creature*)pVictim)->GetCreatureInfo()->Entry);

    if (!Rep)
    {
        return;
    }

    uint32 repFaction1 = Rep->repfaction1;
    uint32 repFaction2 = Rep->repfaction2;

    // Championning tabard reputation system
    // Aura 57818 is a hidden aura common to tabards allowing championning.
    if (pVictim->GetMap()->IsNonRaidDungeon() && HasAura(57818))
    {
        MapEntry const* storedMap = sMapStore.LookupEntry(GetMapId());
        InstanceTemplate const* instance = ObjectMgr::GetInstanceTemplate(GetMapId());
        Item const* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_TABARD);
        if (storedMap && instance && pItem)
        {
            ItemPrototype const* pProto = pItem->GetProto();// Checked on load
            // The required MinLevel for the tabard to work is related to the item level of the tabard
            if ((instance->levelMin + 1 >= pProto->ItemLevel || !GetMap()->IsRegularDifficulty())
                    // For ItemLevel == 75 (or 85) need to check expansion
                    && (pProto->ItemLevel == 75 && storedMap->Expansion() == EXPANSION_WOTLK))
            {
                if (uint32 tabardFactionID = pItem->GetProto()->RequiredReputationFaction)
                {
                    repFaction1 = tabardFactionID;
                    repFaction2 = tabardFactionID;
                }
            }
        }
    }

    if (repFaction1 && (!Rep->team_dependent || GetTeam() == ALLIANCE))
    {
        int32 donerep1 = CalculateReputationGain(REPUTATION_SOURCE_KILL, Rep->repvalue1, repFaction1, pVictim->getLevel());
        donerep1 = int32(donerep1 * rate);
        FactionEntry const* factionEntry1 = sFactionStore.LookupEntry(repFaction1);
        uint32 current_reputation_rank1 = GetReputationMgr().GetRank(factionEntry1);
        if (factionEntry1 && current_reputation_rank1 <= Rep->reputation_max_cap1)
        {
            GetReputationMgr().ModifyReputation(factionEntry1, donerep1);
        }

        // Wiki: Team factions value divided by 2
        if (factionEntry1 && Rep->is_teamaward1)
        {
            FactionEntry const* team1_factionEntry = sFactionStore.LookupEntry(factionEntry1->team);
            if (team1_factionEntry)
            {
                GetReputationMgr().ModifyReputation(team1_factionEntry, donerep1 / 2);
            }
        }
    }

    if (repFaction2 && (!Rep->team_dependent || GetTeam() == HORDE))
    {
        int32 donerep2 = CalculateReputationGain(REPUTATION_SOURCE_KILL, Rep->repvalue2, repFaction2, pVictim->getLevel());
        donerep2 = int32(donerep2 * rate);
        FactionEntry const* factionEntry2 = sFactionStore.LookupEntry(repFaction2);
        uint32 current_reputation_rank2 = GetReputationMgr().GetRank(factionEntry2);
        if (factionEntry2 && current_reputation_rank2 <= Rep->reputation_max_cap2)
        {
            GetReputationMgr().ModifyReputation(factionEntry2, donerep2);
        }

        // Wiki: Team factions value divided by 2
        if (factionEntry2 && Rep->is_teamaward2)
        {
            FactionEntry const* team2_factionEntry = sFactionStore.LookupEntry(factionEntry2->team);
            if (team2_factionEntry)
            {
                GetReputationMgr().ModifyReputation(team2_factionEntry, donerep2 / 2);
            }
        }
    }
}

// Calculate how many reputation points player gain with the quest
void Player::RewardReputation(Quest const* pQuest)
{
    // quest reputation reward/loss
    for (int i = 0; i < QUEST_REPUTATIONS_COUNT; ++i)
    {
        if (!pQuest->RewRepFaction[i])
        {
            continue;
        }

        // No diplomacy mod are applied to the final value (flat). Note the formula (finalValue = DBvalue/100)
        if (pQuest->RewRepValue[i])
        {
            int32 rep = CalculateReputationGain(REPUTATION_SOURCE_QUEST, pQuest->RewRepValue[i] / 100, pQuest->RewRepFaction[i], GetQuestLevelForPlayer(pQuest), true);

            if (FactionEntry const* factionEntry = sFactionStore.LookupEntry(pQuest->RewRepFaction[i]))
            {
                GetReputationMgr().ModifyReputation(factionEntry, rep);
            }
        }
        else
        {
            uint32 row = ((pQuest->RewRepValueId[i] < 0) ? 1 : 0) + 1;
            uint32 field = abs(pQuest->RewRepValueId[i]);

            if (const QuestFactionRewardEntry* pRow = sQuestFactionRewardStore.LookupEntry(row))
            {
                int32 repPoints = pRow->rewardValue[field];

                if (!repPoints)
                {
                    continue;
                }

                repPoints = CalculateReputationGain(REPUTATION_SOURCE_QUEST, repPoints, pQuest->RewRepFaction[i], GetQuestLevelForPlayer(pQuest));

                if (const FactionEntry* factionEntry = sFactionStore.LookupEntry(pQuest->RewRepFaction[i]))
                {
                    GetReputationMgr().ModifyReputation(factionEntry, repPoints);
                }
            }
        }
    }

    // TODO: implement reputation spillover
}

// Player::UpdateHonorKills moved to HonorMgr::UpdateKills (2026-05-12); thin delegating wrapper lives inline in Player.h.

/// Calculate the amount of honor gained based on the victim
/// and the size of the group for which the honor is divided
/// An exact honor value can also be given (overriding the calcs)
// Player::RewardHonor moved to HonorMgr::Reward (2026-05-12); thin delegating wrapper lives inline in Player.h.

void Player::SetInGuild(uint32 GuildId)
{
    if (GuildId)
    {
        SetGuidValue(OBJECT_FIELD_DATA, ObjectGuid(HIGHGUID_GUILD, 0, GuildId));
    }
    else
    {
        SetGuidValue(OBJECT_FIELD_DATA, ObjectGuid());
    }

    ApplyModFlag(PLAYER_FLAGS, PLAYER_FLAGS_GUILD_LEVELING_ENABLED, GuildId != 0 && sWorld.getConfig(CONFIG_BOOL_GUILD_LEVELING_ENABLED));
    SetUInt16Value(OBJECT_FIELD_TYPE, 1, GuildId != 0);
}

std::string Player::GetGuildName() const
{
    if (Guild* guild = sGuildMgr.GetGuildById(GetGuildId()))
    {
        return guild->GetName();
    }

    return "";
}

time_t Player::GetTalentResetTime() const
{
    return m_resetTalentsTime;
}

uint32 Player::GetTalentResetCost()    const
{
    return resetTalentsCost(); // this function added in dev21 - remove this comment if this line works
}

/**
 * @brief Loads a player's guild identifier from the database.
 *
 * @param guid The player GUID to query.
 * @return The guild identifier, or zero if none is found.
 */
uint32 Player::GetGuildIdFromDB(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();

    QueryResult* result = CharacterDatabase.PQuery("SELECT `guildid` FROM `guild_member` WHERE `guid`='%u'", lowguid);
    if (!result)
    {
        return 0;
    }

    uint32 id = result->Fetch()[0].GetUInt32();
    delete result;
    return id;
}

ObjectGuid Player::GetGuildGuidFromDB(ObjectGuid guid)
{
    if (uint32 guildId = GetGuildIdFromDB(guid))
    {
        return ObjectGuid(HIGHGUID_GUILD, GetGuildIdFromDB(guid));
    }
    else
    {
        return ObjectGuid();
    }
}

/**
 * @brief Loads a player's guild rank from the database.
 *
 * @param guid The player GUID to query.
 * @return The guild rank, or zero if none is found.
 */
uint32 Player::GetRankFromDB(ObjectGuid guid)
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT `rank` FROM `guild_member` WHERE `guid`='%u'", guid.GetCounter());
    if (result)
    {
        uint32 v = result->Fetch()[0].GetUInt32();
        delete result;
        return v;
    }
    else
    {
        return 0;
    }
}

void Player::SendGuildDeclined(std::string name, bool autodecline)
{
    WorldPacket data(SMSG_GUILD_DECLINE, 10);
    data << name;
    data << uint8(autodecline);
    GetSession()->SendPacket(&data);
}


uint32 Player::GetArenaTeamIdFromDB(ObjectGuid guid, ArenaType type)
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT `arena_team_member`.`arenateamid` FROM `arena_team_member` JOIN `arena_team` ON `arena_team_member`.`arenateamid` = `arena_team`.`arenateamid` WHERE `guid`='%u' AND `type`='%u' LIMIT 1", guid.GetCounter(), type);
    if (!result)
    {
        return 0;
    }

    uint32 id = (*result)[0].GetUInt32();
    delete result;
    return id;
}

/**
 * @brief Loads or derives a player's saved zone identifier from the database.
 *
 * @param guid The player GUID to query.
 * @return The resolved zone identifier, or zero on failure.
 */
uint32 Player::GetZoneIdFromDB(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();
    QueryResult* result = CharacterDatabase.PQuery("SELECT `zone` FROM `characters` WHERE `guid`='%u'", lowguid);
    if (!result)
    {
        return 0;
    }
    Field* fields = result->Fetch();
    uint32 zone = fields[0].GetUInt32();
    delete result;

    if (!zone)
    {
        // stored zone is zero, use generic and slow zone detection
        result = CharacterDatabase.PQuery("SELECT `map`,`position_x`,`position_y`,`position_z` FROM `characters` WHERE `guid`='%u'", lowguid);
        if (!result)
        {
            return 0;
        }
        fields = result->Fetch();
        uint32 map = fields[0].GetUInt32();
        float posx = fields[1].GetFloat();
        float posy = fields[2].GetFloat();
        float posz = fields[3].GetFloat();
        delete result;

        zone = sTerrainMgr.GetZoneId(map, posx, posy, posz);

        if (zone > 0)
        {
            CharacterDatabase.PExecute("UPDATE `characters` SET `zone`='%u' WHERE `guid`='%u'", zone, lowguid);
        }
    }

    return zone;
}

/**
 * @brief Loads a player's level from the database.
 *
 * @param guid The player GUID to query.
 * @return The stored level, or zero on failure.
 */
uint32 Player::GetLevelFromDB(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();

    QueryResult* result = CharacterDatabase.PQuery("SELECT `level` FROM `characters` WHERE `guid`='%u'", lowguid);
    if (!result)
    {
        return 0;
    }

    Field* fields = result->Fetch();
    uint32 level = fields[0].GetUInt32();
    delete result;

    return level;
}

/**
 * @brief Updates area-specific player state and auras.
 *
 * @param newArea The new area identifier.
 */
void Player::UpdateArea(uint32 newArea)
{
    m_areaUpdateId    = newArea;

    AreaTableEntry const* area = GetAreaEntryByAreaID(newArea);

    // FFA_PVP flags are area and not zone id dependent
    // so apply them accordingly
    if (area && (area->flags & AREA_FLAG_ARENA))
    {
        if (!isGameMaster())
        {
            SetFFAPvP(true);
        }
    }
    else
    {
        // remove ffa flag only if not ffapvp realm
        // removal in sanctuaries and capitals is handled in zone update
        if (IsFFAPvP() && !sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(false);
        }
    }

    UpdateAreaDependentAuras();
}

/**
 * @brief Checks whether the player is eligible to interact with a capture point.
 *
 * @return True if the player can use the capture point; otherwise, false.
 */
bool Player::CanUseCapturePoint()
{
    return IsAlive() &&                                     // living
           !HasStealthAura() &&                             // not stealthed
           !HasInvisibilityAura() &&                        // visible
           (IsPvP() || sWorld.IsPvPRealm()) &&
           !HasMovementFlag(MOVEFLAG_FLYING) &&
           !IsTaxiFlying() &&
           !isGameMaster();
}

/**
 * @brief Updates zone and area state after the player changes location.
 *
 * @param newZone The new zone identifier.
 * @param newArea The new area identifier.
 */
void Player::UpdateZone(uint32 newZone, uint32 newArea)
{
    /* If we're trying to update into a zone that doesn't exist, just return */
    AreaTableEntry const* zone = GetAreaEntryByAreaID(newZone);
    if (!zone)
    {
        return;
    }

    /* If we're moving into a different zone */
    if (m_zoneUpdateId != newZone)
    {
        // handle outdoor pvp zones
        sOutdoorPvPMgr.HandlePlayerLeaveZone(this, m_zoneUpdateId);
        sOutdoorPvPMgr.HandlePlayerEnterZone(this, newZone);

        SendInitWorldStates(newZone, newArea);              // only if really enters to new zone, not just area change, works strange...

        if (sWorld.getConfig(CONFIG_BOOL_WEATHER))
        {
            Weather* wth = GetMap()->GetWeatherSystem()->FindOrCreateWeather(newZone);
            wth->SendWeatherUpdateToPlayer(this);
        }
    }

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnUpdateZone(this, newZone, newArea);
    }
#endif /* ENABLE_ELUNA */

    m_zoneUpdateId    = newZone;
    m_zoneUpdateTimer = ZONE_UPDATE_INTERVAL;

    // zone changed, so area changed as well, update it
    UpdateArea(newArea);

    // in PvP, any not controlled zone (except zone->team == 6, default case)
    // in PvE, only opposition team capital
    switch (zone->team)
    {
        case AREATEAM_ALLY:
            pvpInfo.inHostileArea = GetTeam() != ALLIANCE && (sWorld.IsPvPRealm() || zone->flags & AREA_FLAG_CAPITAL);
            break;
        case AREATEAM_HORDE:
            pvpInfo.inHostileArea = GetTeam() != HORDE && (sWorld.IsPvPRealm() || zone->flags & AREA_FLAG_CAPITAL);
            break;
        case AREATEAM_NONE:
            // overwrite for battlegrounds, maybe batter some zone flags but current known not 100% fit to this
            pvpInfo.inHostileArea = sWorld.IsPvPRealm() || InBattleGround();
            break;
        default:                                            // 6 in fact
            pvpInfo.inHostileArea = false;
            break;
    }

    if (pvpInfo.inHostileArea)                              // in hostile area
    {
        if (!IsPvP() || pvpInfo.endTimer != 0)
        {
            UpdatePvP(true, true);
        }
    }
    else                                                    // in friendly area
    {
        if (IsPvP() && !HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP) && pvpInfo.endTimer == 0)
        {
            pvpInfo.endTimer = time(0); // start toggle-off
        }
    }

    if (zone->flags & AREA_FLAG_SANCTUARY)                  // in sanctuary
    {
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_SANCTUARY);
        if (sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(false);
        }
    }
    else
    {
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_SANCTUARY);
    }

    if (zone->flags & AREA_FLAG_CAPITAL)                    // in capital city
    {
        SetRestType(REST_TYPE_IN_CITY);
    }
    else if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING) && GetRestType() != REST_TYPE_IN_TAVERN)
    {
        // resting and not in tavern (leave city then); tavern leave handled in CheckAreaExploreAndOutdoor
        SetRestType(REST_TYPE_NO);
    }

    // remove items with area/map limitations (delete only for alive player to allow back in ghost mode)
    // if player resurrected at teleport this will be applied in resurrect code
    if (IsAlive())
    {
        DestroyZoneLimitedItem(true, newZone);
    }

    // check some item equip limitations (in result lost CanTitanGrip at talent reset, for example)
    AutoUnequipOffhandIfNeed();

    // recent client version not send leave/join channel packets for built-in local channels
    UpdateLocalChannels(newZone);

    // group update
    if (GetGroup())
    {
        SetGroupUpdateFlag(GROUP_UPDATE_FLAG_ZONE);
    }

    UpdateZoneDependentAuras();
    UpdateZoneDependentPets();
}

// If players are too far way of duel flag... then player loose the duel
void Player::CheckDuelDistance(time_t currTime)
{
    if (!duel)
    {
        return;
    }

    GameObject* obj = GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER));
    if (!obj)
    {
        // player not at duel start map
        DuelComplete(DUEL_FLED);
        return;
    }

    if (duel->outOfBound == 0)
    {
        if (!IsWithinDistInMap(obj, 50))
        {
            duel->outOfBound = currTime;

            WorldPacket data(SMSG_DUEL_OUTOFBOUNDS, 0);
            GetSession()->SendPacket(&data);
        }
    }
    else
    {
        if (IsWithinDistInMap(obj, 40))
        {
            duel->outOfBound = 0;

            WorldPacket data(SMSG_DUEL_INBOUNDS, 0);
            GetSession()->SendPacket(&data);
        }
        else if (currTime >= (duel->outOfBound + 10))
        {
            DuelComplete(DUEL_FLED);
        }
    }
}

/**
 * @brief Finalizes an active duel and cleans up all duel state.
 *
 * @param type The duel completion result.
 */
void Player::DuelComplete(DuelCompleteType type)
{
    // duel not requested
    if (!duel)
    {
        return;
    }

    Player* opponent = duel->opponent;

    // opponent may have been removed (disconnect during duel)
    if (!opponent)
    {
        // Remove Duel Flag object
        if (GameObject* obj = GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER)))
        {
            duel->initiator->RemoveGameObject(obj, true);
        }

        SetGuidValue(PLAYER_DUEL_ARBITER, ObjectGuid());
        SetUInt32Value(PLAYER_DUEL_TEAM, 0);
        delete duel;
        duel = NULL;
        return;
    }

    WorldPacket data(SMSG_DUEL_COMPLETE, (1));
    data << (uint8)((type != DUEL_INTERRUPTED) ? 1 : 0);
    GetSession()->SendPacket(&data);
    opponent->GetSession()->SendPacket(&data);

    if (type != DUEL_INTERRUPTED)
    {
        data.Initialize(SMSG_DUEL_WINNER, (1 + 20));          // we guess size
        data << (uint8)((type == DUEL_WON) ? 0 : 1);          // 0 = just won; 1 = fled
        data << opponent->GetName();
        data << GetName();
        SendMessageToSet(&data, true);
    }

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnDuelEnd(opponent, this, type);
    }
#endif /* ENABLE_ELUNA */

    if (type == DUEL_WON)
    {
        GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL, 1);
        opponent->GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL, 1);
    }

    // Remove Duel Flag object
    if (GameObject* obj = GetMap()->GetGameObject(GetGuidValue(PLAYER_DUEL_ARBITER)))
    {
        duel->initiator->RemoveGameObject(obj, true);
    }

    /* remove auras */
    // TODO: Needs a simpler method
    std::vector<uint32> auras2remove;
    SpellAuraHolderMap const& vAuras = opponent->GetSpellAuraHolderMap();
    for (SpellAuraHolderMap::const_iterator i = vAuras.begin(); i != vAuras.end(); ++i)
    {
        SpellAuraHolder const* aura = i->second;
        if (!aura->IsPositive() && aura->GetCasterGuid() == GetObjectGuid() && aura->GetAuraApplyTime() >= duel->startTime)
        {
            auras2remove.push_back(aura->GetId());
        }
    }

    for (size_t i = 0; i < auras2remove.size(); ++i)
    {
        opponent->RemoveAurasDueToSpell(auras2remove[i]);
    }

    auras2remove.clear();
    SpellAuraHolderMap const& auras = GetSpellAuraHolderMap();
    for (SpellAuraHolderMap::const_iterator i = auras.begin(); i != auras.end(); ++i)
    {
        SpellAuraHolder const* aura = i->second;
        if (!aura->IsPositive() && aura->GetCasterGuid() == opponent->GetObjectGuid() && aura->GetAuraApplyTime() >= duel->startTime)
        {
            auras2remove.push_back(aura->GetId());
        }
    }
    for (size_t i = 0; i < auras2remove.size(); ++i)
    {
        RemoveAurasDueToSpell(auras2remove[i]);
    }

    // cleanup combo points
    if (GetComboTargetGuid() == opponent->GetObjectGuid())
    {
        ClearComboPoints();
    }
    else if (GetComboTargetGuid() == opponent->GetPetGuid())
    {
        ClearComboPoints();
    }

    if (opponent->GetComboTargetGuid() == GetObjectGuid())
    {
        opponent->ClearComboPoints();
    }
    else if (opponent->GetComboTargetGuid() == GetPetGuid())
    {
        opponent->ClearComboPoints();
    }

    // cleanups
    SetGuidValue(PLAYER_DUEL_ARBITER, ObjectGuid());
    SetUInt32Value(PLAYER_DUEL_TEAM, 0);
    opponent->SetGuidValue(PLAYER_DUEL_ARBITER, ObjectGuid());
    opponent->SetUInt32Value(PLAYER_DUEL_TEAM, 0);

    delete opponent->duel;
    opponent->duel = NULL;
    delete duel;
    duel = NULL;
}

void Player::RemovedInsignia(Player* looterPlr)
{
    if (!GetBattleGroundId())
    {
        return;
    }

    // If not released spirit, do it !
    if (m_deathTimer > 0)
    {
        m_deathTimer = 0;
        BuildPlayerRepop();
        RepopAtGraveyard();
    }

    Corpse* corpse = GetCorpse();
    if (!corpse)
    {
        return;
    }

    // We have to convert player corpse to bones, not to be able to resurrect there
    // SpawnCorpseBones isn't handy, 'cos it saves player while he in BG
    Corpse* bones = sObjectAccessor.ConvertCorpseForPlayer(GetObjectGuid(), true);
    if (!bones)
    {
        return;
    }

    // Now we must make bones lootable, and send player loot
    bones->SetFlag(CORPSE_FIELD_DYNAMIC_FLAGS, CORPSE_DYNFLAG_LOOTABLE);

    // We store the level of our player in the gold field
    // We retrieve this information at Player::SendLoot()
    bones->loot.gold = getLevel();
    bones->lootRecipient = looterPlr;
    looterPlr->SendLoot(bones->GetObjectGuid(), LOOT_INSIGNIA);
}

/**
 * @brief Sends a loot release response for a loot object.
 *
 * @param guid The GUID of the released loot source.
 */
void Player::SendLootRelease(ObjectGuid guid)
{
    WorldPacket data(SMSG_LOOT_RELEASE_RESPONSE, (8 + 1));
    data << guid;
    data << uint8(1);
    SendDirectMessage(&data);
}

/**
 * @brief Opens and sends loot contents for a supported loot source.
 *
 * @param guid The GUID of the loot source.
 * @param loot_type The requested loot interaction type.
 */
void Player::SendLoot(ObjectGuid guid, LootType loot_type)
{
    if (ObjectGuid lootGuid = GetLootGuid())
    {
        m_session->DoLootRelease(lootGuid);
    }

    Loot* loot = NULL;
    PermissionTypes permission = ALL_PERMISSION;

    DEBUG_LOG("Player::SendLoot");
    switch (guid.GetHigh())
    {
        case HIGHGUID_GAMEOBJECT:
        {
            DEBUG_LOG("       IS_GAMEOBJECT_GUID(guid)");
            GameObject* go = GetMap()->GetGameObject(guid);

            // not check distance for GO in case owned GO (fishing bobber case, for example)
            // And permit out of range GO with no owner in case fishing hole
            if (!go || (loot_type != LOOT_FISHINGHOLE && (loot_type != LOOT_FISHING && loot_type != LOOT_FISHING_FAIL || go->GetOwnerGuid() != GetObjectGuid()) && !go->IsWithinDistInMap(this, INTERACTION_DISTANCE)))
            {
                SendLootRelease(guid);
                return;
            }

            GameObjectInfo const* goInfo = go->GetGOInfo();

            loot = &go->loot;

            Player* recipient = go->GetLootRecipient();
            if (!recipient)
            {
                go->SetLootRecipient(this);
                recipient = this;
            }

            // generate loot only if ready for open and spawned in world and not already looted once.
            if (go->getLootState() == GO_READY && go->isSpawned())
            {
                uint32 lootid = goInfo->GetLootId();
                if ((go->GetEntry() == BG_AV_OBJECTID_MINE_N || go->GetEntry() == BG_AV_OBJECTID_MINE_S))
                {
                    if (BattleGround* bg = GetBattleGround())
                        if (bg->GetTypeID() == BATTLEGROUND_AV)
                            if (!(((BattleGroundAV*)bg)->PlayerCanDoMineQuest(go->GetEntry(), GetTeam())))
                            {
                                SendLootRelease(guid);
                                return;
                            }
                }

                loot->clear();
                switch (loot_type)
                {
                        // Entry 0 in fishing loot template used for store junk fish loot at fishing fail it junk allowed by config option
                        // this is overwrite fishinghole loot for example
                    case LOOT_FISHING_FAIL:
                        loot->FillLoot(0, LootTemplates_Fishing, this, true);
                        break;
                    case LOOT_FISHING:
                        uint32 zone, subzone;
                        go->GetZoneAndAreaId(zone, subzone);
                        // if subzone loot exist use it
                        if (!loot->FillLoot(subzone, LootTemplates_Fishing, this, true, (subzone != zone)) && subzone != zone)
                            // else use zone loot (if zone diff. from subzone, must exist in like case)
                        {
                            loot->FillLoot(zone, LootTemplates_Fishing, this, true);
                        }
                        break;
                    default:
                        if (!lootid)
                        {
                            break;
                        }
                        DEBUG_LOG("       send normal GO loot");

                        loot->FillLoot(lootid, LootTemplates_Gameobject, this, false);
                        loot->generateMoneyLoot(goInfo->MinMoneyLoot, goInfo->MaxMoneyLoot);

                        if (go->GetGoType() == GAMEOBJECT_TYPE_CHEST && goInfo->chest.groupLootRules)
                        {
                            if (Group* group = go->GetGroupLootRecipient())
                            {
                                group->UpdateLooterGuid(go, true);

                                switch (group->GetLootMethod())
                                {
                                    case GROUP_LOOT:
                                        // GroupLoot delete items over threshold (threshold even not implemented), and roll them. Items with quality<threshold, round robin
                                        group->GroupLoot(go, loot);
                                        permission = GROUP_PERMISSION;
                                        break;
                                    case NEED_BEFORE_GREED:
                                        group->NeedBeforeGreed(go, loot);
                                        permission = GROUP_PERMISSION;
                                        break;
                                    case MASTER_LOOT:
                                        group->MasterLoot(go, loot);
                                        permission = MASTER_PERMISSION;
                                        break;
                                    default:
                                        break;
                                }
                            }
                        }
                        break;
                }
                go->SetLootState(GO_ACTIVATED);
            }

            if (go->getLootState() == GO_ACTIVATED && go->GetGoType() == GAMEOBJECT_TYPE_CHEST && go->GetGOInfo()->chest.groupLootRules)
            {
                if (Group* group = go->GetGroupLootRecipient())
                {
                    if (group == GetGroup())
                    {
                        if (group->GetLootMethod() == FREE_FOR_ALL)
                        {
                            permission = ALL_PERMISSION;
                        }
                        else if (group->GetLooterGuid() == GetObjectGuid())
                        {
                            if (group->GetLootMethod() == MASTER_LOOT)
                            {
                                permission = MASTER_PERMISSION;
                            }
                            else
                            {
                                permission = ALL_PERMISSION;
                            }
                        }
                        else
                        {
                            permission = GROUP_PERMISSION;
                        }
                    }
                    else
                    {
                        permission = NONE_PERMISSION;
                    }
                }
                else if (recipient == this)
                {
                    permission = ALL_PERMISSION;
                }
                else
                {
                    permission = NONE_PERMISSION;
                }
            }
            break;
        }
        case HIGHGUID_ITEM:
        {
            Item* item = GetItemByGuid(guid);

            if (!item)
            {
                SendLootRelease(guid);
                return;
            }

            permission = OWNER_PERMISSION;

            loot = &item->loot;

            if (!item->HasGeneratedLoot())
            {
                item->loot.clear();
                ItemPrototype const* itemProto = item->GetProto();

                switch (loot_type)
                {
                    case LOOT_DISENCHANTING:
                        loot->FillLoot(itemProto->DisenchantID, LootTemplates_Disenchant, this, true);
                        item->SetLootState(ITEM_LOOT_TEMPORARY);
                        break;
                    case LOOT_PROSPECTING:
                        loot->FillLoot(item->GetEntry(), LootTemplates_Prospecting, this, true);
                        item->SetLootState(ITEM_LOOT_TEMPORARY);
                        break;
                    case LOOT_MILLING:
                        loot->FillLoot(item->GetEntry(), LootTemplates_Milling, this, true);
                        item->SetLootState(ITEM_LOOT_TEMPORARY);
                        break;
                    default:
                        loot->FillLoot(item->GetEntry(), LootTemplates_Item, this, true, itemProto->MaxMoneyLoot == 0);
                        loot->generateMoneyLoot(itemProto->MinMoneyLoot, itemProto->MaxMoneyLoot);
                        item->SetLootState(ITEM_LOOT_CHANGED);
                        break;
                }
            }
            break;
        }
        case HIGHGUID_CORPSE:                               // remove insignia
        {
            Corpse* bones = GetMap()->GetCorpse(guid);

            if (!bones || !((loot_type == LOOT_CORPSE) || (loot_type == LOOT_INSIGNIA)) || (bones->GetType() != CORPSE_BONES))
            {
                SendLootRelease(guid);
                return;
            }

            loot = &bones->loot;

            if (!bones->lootForBody)
            {
                bones->lootForBody = true;
                uint32 pLevel = bones->loot.gold;
                bones->loot.clear();
                if (GetBattleGround()->GetTypeID() == BATTLEGROUND_AV)
                {
                    loot->FillLoot(0, LootTemplates_Creature, this, false);
                }
                // It may need a better formula
                // Now it works like this: lvl10: ~6copper, lvl70: ~9silver
                bones->loot.gold = (uint32)(urand(50, 150) * 0.016f * pow(((float)pLevel) / 5.76f, 2.5f) * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY));
            }

            if (bones->lootRecipient != this)
            {
                permission = NONE_PERMISSION;
            }
            else
            {
                permission = OWNER_PERMISSION;
            }
            break;
        }
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
        {
            Creature* creature = GetMap()->GetCreature(guid);

            // must be in range and creature must be alive for pickpocket and must be dead for another loot
            if (!creature || creature->IsAlive() != (loot_type == LOOT_PICKPOCKETING) || !creature->IsWithinDistInMap(this, INTERACTION_DISTANCE))
            {
                SendLootRelease(guid);
                return;
            }

            if (loot_type == LOOT_PICKPOCKETING && IsFriendlyTo(creature))
            {
                SendLootRelease(guid);
                return;
            }

            loot = &creature->loot;
            CreatureInfo const* creatureInfo = creature->GetCreatureInfo();

            if (loot_type == LOOT_PICKPOCKETING)
            {
                if (!creature->lootForPickPocketed)
                {
                    creature->lootForPickPocketed = true;
                    loot->clear();

                    if (uint32 lootid = creatureInfo->PickpocketLootId)
                    {
                        loot->FillLoot(lootid, LootTemplates_Pickpocketing, this, false);
                    }

                    // Generate extra money for pick pocket loot
                    const uint32 a = urand(0, creature->getLevel() / 2);
                    const uint32 b = urand(0, getLevel() / 2);
                    loot->gold = uint32(10 * (a + b) * sWorld.getConfig(CONFIG_FLOAT_RATE_DROP_MONEY));
                    permission = OWNER_PERMISSION;
                }
            }
            else
            {
                // the player whose group may loot the corpse
                Player* recipient = creature->GetLootRecipient();
                Group* recipientGroup = creature->GetGroupLootRecipient();
                // no recipient means the kill had no player participation (e.g. NPC-only kill).
                // Deny loot rather than silently transferring ownership to the first clicker.
                if (!recipient && !recipientGroup)
                {
                    SendLootRelease(guid);
                    return;
                }

                if (creature->lootForPickPocketed)
                {
                    creature->lootForPickPocketed = false;
                    loot->clear();
                }

                if (!creature->lootForBody)
                {
                    creature->lootForBody = true;
                    loot->clear();

                    if (uint32 lootid = creatureInfo->LootId)
                    {
                        loot->FillLoot(lootid, LootTemplates_Creature, recipient, false);
                    }

                    loot->generateMoneyLoot(creatureInfo->MinLootGold, creatureInfo->MaxLootGold);

                    if (Group* group = creature->GetGroupLootRecipient())
                    {
                        group->UpdateLooterGuid(creature, true);

                        switch (group->GetLootMethod())
                        {
                            case GROUP_LOOT:
                                // GroupLoot delete items over threshold (threshold even not implemented), and roll them. Items with quality<threshold, round robin
                                group->GroupLoot(creature, loot);
                                break;
                            case NEED_BEFORE_GREED:
                                group->NeedBeforeGreed(creature, loot);
                                break;
                            case MASTER_LOOT:
                                group->MasterLoot(creature, loot);
                                break;
                            default:
                                break;
                        }
                    }
                }

                // possible only if creature->lootForBody && loot->empty() at spell cast check
                if (loot_type == LOOT_SKINNING)
                {
                    if (!creature->lootForSkin)
                    {
                        creature->lootForSkin = true;
                        loot->clear();
                        loot->FillLoot(creatureInfo->SkinningLootId, LootTemplates_Skinning, this, false);

                        // let reopen skinning loot if will closed.
                        if (!loot->empty())
                        {
                            creature->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                        }

                        permission = OWNER_PERMISSION;

                        // Inform Instance Data, may be scripts related to OnSkinning like The Beast in UBRS
                        if (InstanceData* mapInstance = creature->GetInstanceData())
                        {
                            mapInstance->OnCreatureLooted(creature, LOOT_SKINNING);
                        }
                    }
                }
                // set group rights only for loot_type != LOOT_SKINNING
                else
                {
                    if (Group* group = creature->GetGroupLootRecipient())
                    {
                        if (group == GetGroup())
                        {
                            if (group->GetLootMethod() == FREE_FOR_ALL)
                            {
                                permission = ALL_PERMISSION;
                            }
                            else if (group->GetLooterGuid() == GetObjectGuid())
                            {
                                if (group->GetLootMethod() == MASTER_LOOT)
                                {
                                    permission = MASTER_PERMISSION;
                                }
                                else
                                {
                                    permission = ALL_PERMISSION;
                                }
                            }
                            else
                            {
                                permission = GROUP_PERMISSION;
                            }
                        }
                        else
                        {
                            permission = NONE_PERMISSION;
                        }
                    }
                    else if (recipient == this)
                    {
                        permission = OWNER_PERMISSION;
                    }
                    else
                    {
                        permission = NONE_PERMISSION;
                    }
                }
            }
            break;
        }
        default:
        {
            sLog.outError("%s is unsupported for looting.", guid.GetString().c_str());
            return;
        }
    }

    SetLootGuid(guid);

    // LOOT_INSIGNIA and LOOT_FISHINGHOLE unsupported by client
    switch (loot_type)
    {
        case LOOT_INSIGNIA:     loot_type = LOOT_SKINNING; break;
        case LOOT_FISHING_FAIL: loot_type = LOOT_FISHING; break;
        case LOOT_FISHINGHOLE:  loot_type = LOOT_FISHING; break;
        default: break;
    }

    // need know merged fishing/corpse loot type for achievements
    loot->loot_type = loot_type;

    WorldPacket data(SMSG_LOOT_RESPONSE, (9 + 50));         // we guess size
    data << ObjectGuid(guid);
    data << uint8(loot_type);
    data << LootView(*loot, this, permission);
    SendDirectMessage(&data);

    // add 'this' player as one of the players that are looting 'loot'
    if (permission != NONE_PERMISSION)
    {
        loot->AddLooter(GetObjectGuid());
    }

    if (loot_type == LOOT_CORPSE && !guid.IsItem())
    {
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_LOOTING);
    }
}

/**
 * @brief Notifies the client that money was removed from the current loot.
 */
void Player::SendNotifyLootMoneyRemoved()
{
    WorldPacket data(SMSG_LOOT_CLEAR_MONEY, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Notifies the client that a loot slot was removed.
 *
 * @param lootSlot The loot slot index that was removed.
 */
void Player::SendNotifyLootItemRemoved(uint8 lootSlot, bool currency)
{
    WorldPacket data(currency ? SMSG_LOOT_CURRENCY_REMOVED : SMSG_LOOT_REMOVED, 1);
    data << uint8(lootSlot);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends a single world state update to the client.
 *
 * @param Field The world state field identifier.
 * @param Value The new field value.
 */
void Player::SendUpdateWorldState(uint32 Field, uint32 Value)
{
    WorldPacket data(SMSG_UPDATE_WORLD_STATE, 8 + 1);
    data << Field;
    data << Value;
    data << uint8(0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends the initial world state set for the player's current zone.
 *
 * @param zoneid The zone identifier used to select world states.
 */
void Player::SendInitWorldStates(uint32 zoneid, uint32 areaid)
{
    // data depends on zoneid/mapid...
    BattleGround* bg = GetBattleGround();
    uint32 mapid = GetMapId();

    DEBUG_LOG("Sending SMSG_INIT_WORLD_STATES to Map:%u, Zone: %u", mapid, zoneid);

    uint32 count = 0;                                       // count of world states in packet

    WorldPacket data(SMSG_INIT_WORLD_STATES, (4 + 4 + 4 + 2 + 8 * 8)); // guess
    data << uint32(mapid);                                  // mapid
    data << uint32(zoneid);                                 // zone id
    data << uint32(areaid);                                 // area id, new 2.1.0
    size_t count_pos = data.wpos();
    data << uint16(0);                                      // count of uint64 blocks, placeholder

    // Current arena season
    FillInitialWorldState(data, count, 0xC77, sWorld.getConfig(CONFIG_UINT32_ARENA_SEASON_ID));
    // Previous arena season
    FillInitialWorldState(data, count, 0xF3D, sWorld.getConfig(CONFIG_UINT32_ARENA_SEASON_PREVIOUS_ID));
    // 0 - Battle for Wintergrasp in progress, 1 - otherwise
    FillInitialWorldState(data, count, 0xED9, 1);
    // Time when next Battle for Wintergrasp starts
    FillInitialWorldState(data, count, 0x1102, uint32(time(NULL) + 9000));

    switch (zoneid)
    {
        case 139:                                           // Eastern Plaguelands
        case 1377:                                          // Silithus
        case 3483:                                          // Hellfire Peninsula
        case 3518:                                          // Nagrand
        case 3519:                                          // Terokkar Forest
        case 3521:                                          // Zangarmarsh
            if (OutdoorPvP* outdoorPvP = sOutdoorPvPMgr.GetScript(zoneid))
            {
                outdoorPvP->FillInitialWorldStates(data, count);
            }
            break;
        case 2597:                                          // AV
            if (bg && bg->GetTypeID() == BATTLEGROUND_AV)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3277:                                          // WS
            if (bg && bg->GetTypeID() == BATTLEGROUND_WS)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3358:                                          // AB
            if (bg && bg->GetTypeID() == BATTLEGROUND_AB)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3820:                                          // EY
            if (bg && bg->GetTypeID() == BATTLEGROUND_EY)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3698:                                          // Nagrand Arena
            if (bg && bg->GetTypeID() == BATTLEGROUND_NA)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3702:                                          // Blade's Edge Arena
            if (bg && bg->GetTypeID() == BATTLEGROUND_BE)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
        case 3968:                                          // Ruins of Lordaeron
            if (bg && bg->GetTypeID() == BATTLEGROUND_RL)
            {
                bg->FillInitialWorldStates(data, count);
            }
            break;
    }

    FillBGWeekendWorldStates(data, count);

    data.put<uint16>(count_pos, count);                     // set actual world state amount

    GetSession()->SendPacket(&data);
}

void Player::FillBGWeekendWorldStates(WorldPacket& data, uint32& count)
{
    for (uint32 i = 1; i < sBattlemasterListStore.GetNumRows(); ++i)
    {
        BattlemasterListEntry const* bl = sBattlemasterListStore.LookupEntry(i);
        if (bl && bl->HolidayWorldStateId)
        {
            if (BattleGroundMgr::IsBGWeekend(BattleGroundTypeId(bl->id)))
            {
                FillInitialWorldState(data, count, bl->HolidayWorldStateId, 1);
            }
            else
            {
                FillInitialWorldState(data, count, bl->HolidayWorldStateId, 0);
            }
        }
    }
}

/**
 * @brief Consumes and returns the rested experience bonus for an XP award.
 *
 * @param xp The base experience amount being awarded.
 * @return The rested bonus experience amount.
 */
uint32 Player::GetXPRestBonus(uint32 xp)
{
    uint32 rested_bonus = (uint32)GetRestBonus();           // xp for each rested bonus

    if (rested_bonus > xp)                                  // max rested_bonus == xp or (r+x) = 200% xp
    {
        rested_bonus = xp;
    }

    SetRestBonus(GetRestBonus() - rested_bonus);

    DETAIL_LOG("Player gain %u xp (+ %u Rested Bonus). Rested points=%f", xp + rested_bonus, rested_bonus, GetRestBonus());
    return rested_bonus;
}

/**
 * @brief Sends a bind point confirmation prompt to the client.
 *
 * @param guid The binder NPC GUID.
 */
void Player::SetBindPoint(ObjectGuid guid)
{
    WorldPacket data(SMSG_BINDER_CONFIRM, 8);
    data << ObjectGuid(guid);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends a talent reset confirmation prompt to the client.
 *
 * @param guid The trainer or source GUID for the confirmation.
 */
void Player::SendTalentWipeConfirm(ObjectGuid guid)
{
    WorldPacket data(MSG_TALENT_WIPE_CONFIRM, (8 + 4));
    data << ObjectGuid(guid);
    data << uint32(resetTalentsCost());
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends a pet talent reset confirmation prompt to the client.
 */
void Player::SendPetSkillWipeConfirm()
{
    Pet* pet = GetPet();
    if (!pet)
    {
        return;
    }
    WorldPacket data(SMSG_PET_UNLEARN_CONFIRM, (8 + 4));
    data << ObjectGuid(pet->GetObjectGuid());
    data << uint32(pet->resetTalentsCost());
    GetSession()->SendPacket(&data);
}


bool Player::IsTappedByMeOrMyGroup(Creature* creature)
{
    /* Nobody tapped the monster (solo kill by another NPC) */
    if (!creature->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED))
    {
        return false;
    }

    /* If there is a loot recipient, assign it to recipient */
    if (Player* recipient = creature->GetLootRecipient())
    {
        /* See if we're in a group */
        if (Group* plr_group = recipient->GetGroup())
        {
            /* Recipient is in a group... but is it ours? */
            if (Group* my_group = GetGroup())
            {
                /* Check groups are the same */
                if (plr_group != my_group)
                {
                    return false;  // Cheater, deny loot
                }
            }
            else
            {
                return false;  // We're not in a group, probably cheater
            }

            /* We're in the looters group, so mob is tapped by us */
            return true;
        }
        /* We're not in a group, check to make sure we're the recipient (prevent cheaters) */
        else if (recipient == this)
        {
            return true;
        }
    }
    else
        /* Don't know what happened to the recipient, probably disconnected
         * Either way, it isn't us, so mark as tapped */
         {
             return false;
         }

    return false;
}

/**
 * @brief Checks whether a creature is tapped by this player or the player's group.
 *
 * @param creature The creature to test.
 * @return True if the tap belongs to this player or group; otherwise, false.
 */
bool Player::isAllowedToLoot(Creature* creature)
{
    // never tapped by any (mob solo kill)
    if (!creature->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED))
    {
        return false;
    }

    if (Player* recipient = creature->GetLootRecipient())
    {
        if (recipient == this)
        {
            return true;
        }

        if (Group* otherGroup = recipient->GetGroup())
        {
            Group* thisGroup = GetGroup();
            if (!thisGroup)
            {
                return false;
            }

            return thisGroup == otherGroup;
        }
        return false;
    }
    else // prevent other players from looting if the recipient got disconnected
    {
        return !creature->HasLootRecipient();
    }
}


/**
 * @brief Writes the player's current combat and stat values to the debug log.
 */
void Player::outDebugStatsValues() const
{
    // optimize disabled debug output
    if (!sLog.HasLogLevelOrHigher(LOG_LVL_DEBUG) || sLog.HasLogFilter(LOG_FILTER_PLAYER_STATS))
    {
        return;
    }

    sLog.outDebug("HP is: \t\t\t%u\t\tMP is: \t\t\t%u", GetMaxHealth(), GetMaxPower(POWER_MANA));
    sLog.outDebug("AGILITY is: \t\t%f\t\tSTRENGTH is: \t\t%f", GetStat(STAT_AGILITY), GetStat(STAT_STRENGTH));
    sLog.outDebug("INTELLECT is: \t\t%f\t\tSPIRIT is: \t\t%f", GetStat(STAT_INTELLECT), GetStat(STAT_SPIRIT));
    sLog.outDebug("STAMINA is: \t\t%f", GetStat(STAT_STAMINA));
    sLog.outDebug("Armor is: \t\t%u\t\tBlock is: \t\t%f", GetArmor(), GetFloatValue(PLAYER_BLOCK_PERCENTAGE));
    sLog.outDebug("HolyRes is: \t\t%u\t\tFireRes is: \t\t%u", GetResistance(SPELL_SCHOOL_HOLY), GetResistance(SPELL_SCHOOL_FIRE));
    sLog.outDebug("NatureRes is: \t\t%u\t\tFrostRes is: \t\t%u", GetResistance(SPELL_SCHOOL_NATURE), GetResistance(SPELL_SCHOOL_FROST));
    sLog.outDebug("ShadowRes is: \t\t%u\t\tArcaneRes is: \t\t%u", GetResistance(SPELL_SCHOOL_SHADOW), GetResistance(SPELL_SCHOOL_ARCANE));
    sLog.outDebug("MIN_DAMAGE is: \t\t%f\tMAX_DAMAGE is: \t\t%f", GetFloatValue(UNIT_FIELD_MINDAMAGE), GetFloatValue(UNIT_FIELD_MAXDAMAGE));
    sLog.outDebug("MIN_OFFHAND_DAMAGE is: \t%f\tMAX_OFFHAND_DAMAGE is: \t%f", GetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE), GetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE));
    sLog.outDebug("MIN_RANGED_DAMAGE is: \t%f\tMAX_RANGED_DAMAGE is: \t%f", GetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE), GetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE));
    sLog.outDebug("ATTACK_TIME is: \t%u\t\tRANGE_ATTACK_TIME is: \t%u", GetAttackTime(BASE_ATTACK), GetAttackTime(RANGED_ATTACK));
}

/*********************************************************/
/***               FLOOD FILTER SYSTEM                 ***/
/*********************************************************/

void Player::UpdateSpeakTime()
{
    // ignore chat spam protection for GMs in any mode
    if (GetSession()->GetSecurity() > SEC_PLAYER)
    {
        return;
    }

    time_t current = time(NULL);
    if (m_speakTime > current)
    {
        uint32 max_count = sWorld.getConfig(CONFIG_UINT32_CHATFLOOD_MESSAGE_COUNT);
        if (!max_count)
        {
            return;
        }

        ++m_speakCount;
        if (m_speakCount >= max_count)
        {
            // prevent overwrite mute time, if message send just before mutes set, for example.
            time_t new_mute = current + sWorld.getConfig(CONFIG_UINT32_CHATFLOOD_MUTE_TIME);
            if (GetSession()->m_muteTime < new_mute)
            {
                GetSession()->m_muteTime = new_mute;
            }

            m_speakCount = 0;
        }
    }
    else
    {
        m_speakCount = 0;
    }

    m_speakTime = current + sWorld.getConfig(CONFIG_UINT32_CHATFLOOD_MESSAGE_DELAY);
}

/**
 * @brief Checks whether the player is currently allowed to send chat messages.
 *
 * @return True if the player's mute timer has expired; otherwise, false.
 */
bool Player::CanSpeak() const
{
    return  GetSession()->m_muteTime <= time(NULL);
}

/*********************************************************/
/***              LOW LEVEL FUNCTIONS:Notifiers        ***/
/*********************************************************/

void Player::SendAttackSwingNotInRange()
{
    WorldPacket data(SMSG_ATTACKSWING_NOTINRANGE, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Replaces a tokenized uint32 field value in a serialized array.
 *
 * @param tokens The token array to modify.
 * @param index The token index to replace.
 * @param value The new uint32 value.
 */
void Player::SetUInt32ValueInArray(Tokens& tokens, uint16 index, uint32 value)
{
    char buf[11];
    snprintf(buf, 11, "%u", value);

    if (index >= tokens.size())
    {
        return;
    }

    tokens[index] = buf;
}

void Player::Customize(ObjectGuid guid, uint8 gender, uint8 skin, uint8 face, uint8 hairStyle, uint8 hairColor, uint8 facialHair)
{
    //                                                     0
    QueryResult* result = CharacterDatabase.PQuery("SELECT `playerBytes2` FROM `characters` WHERE `guid` = '%u'", guid.GetCounter());
    if (!result)
    {
        return;
    }

    Field* fields = result->Fetch();

    uint32 player_bytes2 = fields[0].GetUInt32();
    player_bytes2 &= ~0xFF;
    player_bytes2 |= facialHair;

    CharacterDatabase.PExecute("UPDATE `characters` SET `gender` = '%u', `playerBytes` = '%u', `playerBytes2` = '%u' WHERE `guid` = '%u'", gender, skin | (face << 8) | (hairStyle << 16) | (hairColor << 24), player_bytes2, guid.GetCounter());

    delete result;
}

/**
 * @brief Sends the error packet for attempting to attack a dead target.
 */
void Player::SendAttackSwingDeadTarget()
{
    WorldPacket data(SMSG_ATTACKSWING_DEADTARGET, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends the error packet for a general inability to attack the target.
 */
void Player::SendAttackSwingCantAttack()
{
    WorldPacket data(SMSG_ATTACKSWING_CANT_ATTACK, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends the packet that cancels the player's current attack.
 */
void Player::SendAttackSwingCancelAttack()
{
    WorldPacket data(SMSG_CANCEL_COMBAT, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends the error packet for attempting to attack while facing the wrong direction.
 */
void Player::SendAttackSwingBadFacingAttack()
{
    WorldPacket data(SMSG_ATTACKSWING_BADFACING, 0);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends the packet that cancels auto-repeat attacks for the client.
 */
void Player::SendAutoRepeatCancel(Unit* target)
{
    WorldPacket data(SMSG_CANCEL_AUTO_REPEAT, target->GetPackGUID().size());
    data << target->GetPackGUID();
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends an exploration experience reward packet to the client.
 *
 * @param Area The explored area identifier.
 * @param Experience The awarded experience amount.
 */
void Player::SendExplorationExperience(uint32 Area, uint32 Experience)
{
    WorldPacket data(SMSG_EXPLORATION_EXPERIENCE, 8);
    data << uint32(Area);
    data << uint32(Experience);
    GetSession()->SendPacket(&data);
}

void Player::SendDungeonDifficulty(bool IsInGroup)
{
    uint8 val = 0x00000001;
    WorldPacket data(MSG_SET_DUNGEON_DIFFICULTY, 12);
    data << uint32(GetDungeonDifficulty());
    data << uint32(val);
    data << uint32(IsInGroup);
    GetSession()->SendPacket(&data);
}

void Player::SendRaidDifficulty(bool IsInGroup)
{
    uint8 val = 0x00000001;
    WorldPacket data(MSG_SET_RAID_DIFFICULTY, 12);
    data << uint32(GetRaidDifficulty());
    data << uint32(val);
    data << uint32(IsInGroup);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends a reset-failed notification for an instance map.
 *
 * @param mapid The map identifier that failed to reset.
 */
void Player::SendResetFailedNotify(uint32 mapid)
{
    WorldPacket data(SMSG_RESET_FAILED_NOTIFY, 4);
    data << uint32(mapid);
    GetSession()->SendPacket(&data);
}

/// Reset all solo instances and optionally send a message on success for each
void Player::ResetInstances(InstanceResetMethod method, bool isRaid)
{
    // method can be INSTANCE_RESET_ALL, INSTANCE_RESET_CHANGE_DIFFICULTY, INSTANCE_RESET_GROUP_JOIN

    // we assume that when the difficulty changes, all instances that can be reset will be
    Difficulty diff = GetDifficulty(isRaid);

    for (BoundInstancesMap::iterator itr = m_boundInstances[diff].begin(); itr != m_boundInstances[diff].end();)
    {
        DungeonPersistentState* state = itr->second.state;
        const MapEntry* entry = sMapStore.LookupEntry(itr->first);
        if (!entry || entry->IsRaid() != isRaid || !state->CanReset())
        {
            ++itr;
            continue;
        }

        if (method == INSTANCE_RESET_ALL)
        {
            // the "reset all instances" method can only reset normal maps
            if (entry->map_type == MAP_RAID || diff == DUNGEON_DIFFICULTY_HEROIC)
            {
                ++itr;
                continue;
            }
        }

        // if the map is loaded, reset it
        if (Map* map = sMapMgr.FindMap(state->GetMapId(), state->GetInstanceId()))
            if (map->IsDungeon())
            {
                ((DungeonMap*)map)->Reset(method);
            }

        // since this is a solo instance there should not be any players inside
        if (method == INSTANCE_RESET_ALL || method == INSTANCE_RESET_CHANGE_DIFFICULTY)
        {
            SendResetInstanceSuccess(state->GetMapId());
        }

        state->DeleteFromDB();
        m_boundInstances[diff].erase(itr++);

        // the following should remove the instance save from the manager and delete it as well
        state->RemovePlayer(this);
    }
}

/**
 * @brief Sends a successful instance reset notification to the client.
 *
 * @param MapId The reset map identifier.
 */
void Player::SendResetInstanceSuccess(uint32 MapId)
{
    WorldPacket data(SMSG_INSTANCE_RESET, 4);
    data << uint32(MapId);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends an instance reset failure message to the client.
 *
 * @param reason The reset failure reason code.
 * @param MapId The map identifier that failed to reset.
 */
void Player::SendResetInstanceFailed(uint32 reason, uint32 MapId)
{
    // TODO: find what other fail reasons there are besides players in the instance
    WorldPacket data(SMSG_INSTANCE_RESET_FAILED, 4);
    data << uint32(reason);
    data << uint32(MapId);
    GetSession()->SendPacket(&data);
}

/*********************************************************/
/***              Update timers                        ***/
/*********************************************************/

/// checks the 15 afk reports per 5 minutes limit
void Player::UpdateAfkReport(time_t currTime)
{
    if (m_bgData.bgAfkReportedTimer <= currTime)
    {
        m_bgData.bgAfkReportedCount = 0;
        m_bgData.bgAfkReportedTimer = currTime + 5 * MINUTE;
    }
}

void Player::UpdateContestedPvP(uint32 diff)
{
    if (!m_contestedPvPTimer || IsInCombat())
    {
        return;
    }
    if (m_contestedPvPTimer <= diff)
    {
        ResetContestedPvP();
    }
    else
    {
        m_contestedPvPTimer -= diff;
    }
}

/**
 * @brief Updates and clears the player's PvP flag when the timeout expires.
 *
 * @param currTime The current server time.
 */
void Player::UpdatePvPFlag(time_t currTime)
{
    if (!IsPvP())
    {
        return;
    }
    if (pvpInfo.endTimer == 0 || currTime < (pvpInfo.endTimer + 300))
    {
        return;
    }

    UpdatePvP(false);
}

/**
 * @brief Starts an active duel once the duel countdown completes.
 *
 * @param currTime The current server time.
 */
void Player::UpdateDuelFlag(time_t currTime)
{
    if (!duel || duel->startTimer == 0 || currTime < duel->startTimer + 3)
    {
        return;
    }

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = GetEluna())
    {
        e->OnDuelStart(this, duel->opponent);
    }
#endif /* ENABLE_ELUNA */

    SetUInt32Value(PLAYER_DUEL_TEAM, 1);
    duel->opponent->SetUInt32Value(PLAYER_DUEL_TEAM, 2);

    duel->startTimer = 0;
    duel->startTime  = currTime;
    duel->opponent->duel->startTimer = 0;
    duel->opponent->duel->startTime  = currTime;
}

/**
 * @brief Sends a say chat message from the player to nearby listeners.
 *
 * @param text The message text.
 * @param language The language used for the chat packet.
 */
void Player::Say(const std::string& text, const uint32 language)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_SAY, text.c_str(), Language(language), GetChatTag(), GetObjectGuid(), GetName());
    SendMessageToSetInRange(&data, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_SAY), true);
}

/**
 * @brief Sends a yell chat message from the player to nearby listeners.
 *
 * @param text The message text.
 * @param language The language used for the chat packet.
 */
void Player::Yell(const std::string& text, const uint32 language)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_YELL, text.c_str(), Language(language), GetChatTag(), GetObjectGuid(), GetName());
    SendMessageToSetInRange(&data, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_YELL), true);
}

/**
 * @brief Sends a text emote message from the player to nearby listeners.
 *
 * @param text The emote text.
 */
void Player::TextEmote(const std::string& text)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_EMOTE, text.c_str(), LANG_UNIVERSAL, GetChatTag(), GetObjectGuid(), GetName());
    SendMessageToSetInRange(&data, sWorld.getConfig(CONFIG_FLOAT_LISTEN_RANGE_TEXTEMOTE), true, !sWorld.getConfig(CONFIG_BOOL_ALLOW_TWO_SIDE_INTERACTION_CHAT));
}

/**
 * @brief Logs a whisper to the database when whisper logging is enabled.
 *
 * @param text The whisper text.
 * @param receiver The recipient player GUID.
 */
void Player::LogWhisper(const std::string& text, ObjectGuid receiver)
{
    WhisperLoggingLevels loggingLevel = WhisperLoggingLevels(sWorld.getConfig(CONFIG_UINT32_LOG_WHISPERS));

    if (loggingLevel == WHISPER_LOGGING_NONE)
    {
        return;
    }

    //Try to find ticket by either this player or the receiver
    GMTicket* ticket = sTicketMgr.GetGMTicket(GetObjectGuid());
    if (!ticket)
    {
        ticket = sTicketMgr.GetGMTicket(receiver);
    }

    uint32 ticketId = 0;
    if (ticket)
    {
        ticketId = ticket->GetId();
    }

    bool isSomeoneGM = false;

    //Find out if at least one of them is a GM for ticket logging
    if (GetSession()->GetSecurity() >= SEC_GAMEMASTER)
    {
        isSomeoneGM = true;
    }
    else
    {
        Player* pRecvPlayer = sObjectMgr.GetPlayer(receiver);
        if (pRecvPlayer && pRecvPlayer->GetSession()->GetSecurity() >= SEC_GAMEMASTER)
        {
            isSomeoneGM = true;
        }
    }

    if ((loggingLevel == WHISPER_LOGGING_TICKETS && ticket && isSomeoneGM)
        || loggingLevel == WHISPER_LOGGING_EVERYTHING)
    {
        static SqlStatementID wlog;
        SqlStatement stmt = CharacterDatabase.CreateStatement(wlog, "INSERT INTO `character_whispers` (`to_guid`, `from_guid`, `message`, `regarding_ticket_id`) VALUES (?, ?, ?, ?)");
        stmt.addUInt32(receiver.GetCounter());          // to_guid
        stmt.addUInt32(GetObjectGuid().GetCounter());   // from_guid
        stmt.addString(text.c_str());                   // message
        stmt.addUInt32(ticketId);                       // regarding_ticket_id
        stmt.Execute();
    }
}

/**
 * @brief Sends a whisper to another player and handles local response messages.
 *
 * @param text The whisper text.
 * @param language The requested chat language.
 * @param receiver The recipient player GUID.
 */
void Player::Whisper(const std::string& text, uint32 language, ObjectGuid receiver)
{
    Player* rPlayer = sObjectMgr.GetPlayer(receiver);

    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, text.c_str(), Language(language), GetChatTag(), GetObjectGuid(), GetName());
    rPlayer->GetSession()->SendPacket(&data);

    // do not send confirmations, afk, dnd or system notifications for addon messages
    if (language == LANG_ADDON)
    {
        data.clear();
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, text.c_str(), Language(language), CHAT_TAG_NONE, rPlayer->GetObjectGuid());
        LogWhisper(text, receiver);
        GetSession()->SendPacket(&data);
    }

    data.clear();
    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER_INFORM, text.c_str(), Language(language), CHAT_TAG_NONE, rPlayer->GetObjectGuid());
    GetSession()->SendPacket(&data);

    if (!isAcceptWhispers())
    {
        SetAcceptWhispers(true);
        ChatHandler(this).SendSysMessage(LANG_COMMAND_WHISPERON);
    }

    // announce afk or dnd message
    if (rPlayer->isAFK() || rPlayer->isDND())
    {
        const ChatMsg msgtype = rPlayer->isAFK() ? CHAT_MSG_AFK : CHAT_MSG_DND;
        data.clear();
        ChatHandler::BuildChatPacket(data, msgtype, rPlayer->autoReplyMsg.c_str(), LANG_UNIVERSAL, CHAT_TAG_NONE, rPlayer->GetObjectGuid());
        GetSession()->SendPacket(&data);
    }
}

/**
 * @brief Sends the controlled pet's spell bar and cooldown state to the client.
 */
void Player::PetSpellInitialize()
{
    Pet* pet = GetPet();

    if (!pet)
    {
        return;
    }

    DEBUG_LOG("Pet Spells Groups");

    CharmInfo* charmInfo = pet->GetCharmInfo();

    WorldPacket data(SMSG_PET_SPELLS, 8 + 2 + 4 + 4 + 4 * MAX_UNIT_ACTION_BAR_INDEX + 1 + 1);
    data << pet->GetObjectGuid();
    data << uint16(pet->GetCreatureInfo()->Family);         // creature family (required for pet talents)
    data << uint32(0);
    data << uint8(charmInfo->GetReactState()) << uint8(charmInfo->GetCommandState()) << uint16(0);

    // action bar loop
    charmInfo->BuildActionBar(&data);

    size_t spellsCountPos = data.wpos();

    // spells count
    uint8 addlist = 0;
    data << uint8(addlist);                                 // placeholder

    if (pet->isControlled())
    {
        // spells loop
        for (PetSpellMap::const_iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
        {
            if (itr->second.state == PETSPELL_REMOVED)
            {
                continue;
            }

            data << uint32(MAKE_UNIT_ACTION_BUTTON(itr->first, itr->second.active));
            ++addlist;
        }
    }

    data.put<uint8>(spellsCountPos, addlist);

    uint8 cooldownsCount = pet->m_CreatureSpellCooldowns.size() + pet->m_CreatureCategoryCooldowns.size();
    data << uint8(cooldownsCount);

    time_t curTime = time(NULL);

    for (CreatureSpellCooldowns::const_iterator itr = pet->m_CreatureSpellCooldowns.begin(); itr != pet->m_CreatureSpellCooldowns.end(); ++itr)
    {
        time_t cooldown = (itr->second > curTime) ? (itr->second - curTime) * IN_MILLISECONDS : 0;

        data << uint32(itr->first);                         // spellid
        data << uint16(0);                                  // spell category?
        data << uint32(cooldown);                           // cooldown
        data << uint32(0);                                  // category cooldown
    }

    for (CreatureSpellCooldowns::const_iterator itr = pet->m_CreatureCategoryCooldowns.begin(); itr != pet->m_CreatureCategoryCooldowns.end(); ++itr)
    {
        time_t cooldown = (itr->second > curTime) ? (itr->second - curTime) * IN_MILLISECONDS : 0;

        data << uint32(itr->first);                         // spellid
        data << uint16(0);                                  // spell category?
        data << uint32(0);                                  // cooldown
        data << uint32(cooldown);                           // category cooldown
    }

    GetSession()->SendPacket(&data);
}

void Player::SendPetGUIDs()
{
    if (!GetPetGuid())
    {
        return;
    }

    // Later this function might get modified for multiple guids
    WorldPacket data(SMSG_PET_GUIDS, 12);
    data << uint32(1);                      // count
    data << ObjectGuid(GetPetGuid());
    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends the possessed unit's action bar state to the client.
 */
void Player::PossessSpellInitialize()
{
    Unit* charm = GetCharm();

    if (!charm)
    {
        return;
    }

    CharmInfo* charmInfo = charm->GetCharmInfo();

    if (!charmInfo)
    {
        sLog.outError("Player::PossessSpellInitialize(): charm (GUID: %u TypeId: %u) has no charminfo!", charm->GetGUIDLow(), charm->GetTypeId());
        return;
    }

    WorldPacket data(SMSG_PET_SPELLS, 8 + 2 + 4 + 4 + 4 * MAX_UNIT_ACTION_BAR_INDEX + 1 + 1);
    data << charm->GetObjectGuid();
    data << uint16(0);
    data << uint32(0);
    data << uint32(0);

    charmInfo->BuildActionBar(&data);

    data << uint8(0);                                       // spells count
    data << uint8(0);                                       // cooldowns count

    GetSession()->SendPacket(&data);
}

/**
 * @brief Sends the charmed unit's available actions and spells to the client.
 */
void Player::CharmSpellInitialize()
{
    Unit* charm = GetCharm();

    if (!charm)
    {
        return;
    }

    CharmInfo* charmInfo = charm->GetCharmInfo();
    if (!charmInfo)
    {
        sLog.outError("Player::CharmSpellInitialize(): the player's charm (GUID: %u TypeId: %u) has no charminfo!", charm->GetGUIDLow(), charm->GetTypeId());
        return;
    }

    uint8 addlist = 0;

    if (charm->GetTypeId() != TYPEID_PLAYER)
    {
        CreatureInfo const* cinfo = ((Creature*)charm)->GetCreatureInfo();

        if (cinfo && cinfo->CreatureType == CREATURE_TYPE_DEMON && getClass() == CLASS_WARLOCK)
        {
            for (uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
            {
                if (charmInfo->GetCharmSpell(i)->GetAction())
                {
                    ++addlist;
                }
            }
        }
    }

    WorldPacket data(SMSG_PET_SPELLS, 8 + 2 + 4 + 4 + 4 * MAX_UNIT_ACTION_BAR_INDEX + 1 + 4 * addlist + 1);
    data << charm->GetObjectGuid();
    data << uint16(0);
    data << uint32(0);

    if (charm->GetTypeId() != TYPEID_PLAYER)
    {
        data << uint8(charmInfo->GetReactState()) << uint8(charmInfo->GetCommandState()) << uint16(0);
    }
    else
    {
        data << uint8(0) << uint8(0) << uint16(0);
    }

    charmInfo->BuildActionBar(&data);

    data << uint8(addlist);

    if (addlist)
    {
        for (uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
        {
            CharmSpellEntry* cspell = charmInfo->GetCharmSpell(i);
            if (cspell->GetAction())
            {
                data << uint32(cspell->packedData);
            }
        }
    }

    data << uint8(0);                                       // cooldowns count

    GetSession()->SendPacket(&data);
}

/**
 * @brief Applies or removes a spell modifier and notifies the client.
 *
 * @param aura The aura whose modifier should be added or removed.
 * @param apply True to apply the modifier; false to remove it.
 */
void Player::AddSpellMod(Aura* aura, bool apply)
{
    Modifier const* mod = aura->GetModifier();
    OpcodesList opcode = (mod->m_auraname == SPELL_AURA_ADD_FLAT_MODIFIER) ? SMSG_SET_FLAT_SPELL_MODIFIER : SMSG_SET_PCT_SPELL_MODIFIER;

    uint32 modTypeCount = 0;    // count of mods per one mod->op
    WorldPacket data(opcode, 4 + 4 + 1 + 1 + 4);
    data << uint32(1);          // count of different mod->op's in packet
    size_t writePos = data.wpos();
    data << uint32(modTypeCount);
    data << uint8(mod->m_miscvalue);
    for (int eff = 0; eff < 96; ++eff)
    {
        uint64 _mask = 0;
        uint32 _mask2 = 0;

        if (eff < 64)
        {
            _mask = uint64(1) << (eff - 0);
        }
        else
        {
            _mask2 = uint32(1) << (eff - 64);
        }

        if (aura->GetAuraSpellClassMask().IsFitToFamilyMask(_mask, _mask2))
        {
            int32 val = 0;
            for (AuraList::const_iterator itr = m_spellMods[mod->m_miscvalue].begin(); itr != m_spellMods[mod->m_miscvalue].end(); ++itr)
            {
                if ((*itr)->GetModifier()->m_auraname == mod->m_auraname && ((*itr)->GetAuraSpellClassMask().IsFitToFamilyMask(_mask, _mask2)))
                {
                    val += (*itr)->GetModifier()->m_amount;
                }
            }
            val += apply ? mod->m_amount : -(mod->m_amount);
            data << uint8(eff);
            data << float(val);
            ++modTypeCount;
        }
    }
    data.put<uint32>(writePos, modTypeCount);
    SendDirectMessage(&data);

    if (apply)
    {
        m_spellMods[mod->m_miscvalue].push_back(aura);
    }
    else
    {
        m_spellMods[mod->m_miscvalue].remove(aura);
    }
}

template <class T> T Player::ApplySpellMod(uint32 spellId, SpellModOp op, T& basevalue, Spell const* /*spell*/)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        return 0;
    }

    int32 totalpct = 0;
    int32 totalflat = 0;
    for (AuraList::iterator itr = m_spellMods[op].begin(); itr != m_spellMods[op].end(); ++itr)
    {
        Aura* aura = *itr;

        Modifier const* mod = aura->GetModifier();

        if (!aura->isAffectedOnSpell(spellInfo))
        {
            continue;
        }

        if (mod->m_auraname == SPELL_AURA_ADD_FLAT_MODIFIER)
        {
            totalflat += mod->m_amount;
        }
        else
        {
            // skip percent mods for null basevalue (most important for spell mods with charges )
            if (basevalue == T(0))
            {
                continue;
            }

            // special case (skip >10sec spell casts for instant cast setting)
            if (mod->m_miscvalue == SPELLMOD_CASTING_TIME
                    && basevalue >= T(10 * IN_MILLISECONDS) && mod->m_amount <= -100)
                continue;

            totalpct += mod->m_amount;
        }
    }

    float diff = (float)basevalue * (float)totalpct / 100.0f + (float)totalflat;
    basevalue = T((float)basevalue + diff);
    return T(diff);
}

template int32 Player::ApplySpellMod<int32>(uint32 spellId, SpellModOp op, int32& basevalue, Spell const* spell);
template uint32 Player::ApplySpellMod<uint32>(uint32 spellId, SpellModOp op, uint32& basevalue, Spell const* spell);
template float Player::ApplySpellMod<float>(uint32 spellId, SpellModOp op, float& basevalue, Spell const* spell);

// send Proficiency
void Player::SendProficiency(ItemClass itemClass, uint32 itemSubclassMask)
{
    WorldPacket data(SMSG_SET_PROFICIENCY, 1 + 4);
    data << uint8(itemClass) << uint32(itemSubclassMask);
    GetSession()->SendPacket(&data);
}

/**
 * @brief Removes petition ownership and signatures associated with a player.
 *
 * @param guid The player GUID whose petition data should be removed.
 */
void Player::RemovePetitionsAndSigns(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();

    QueryResult* result = NULL;
    result = CharacterDatabase.PQuery("SELECT `ownerguid`,`petitionguid` FROM `petition_sign` WHERE `playerguid` = '%u'", lowguid);
    if (result)
    {
        do                                                  // this part effectively does nothing, since the deletion / modification only takes place _after_ the PetitionQuery. Though I don't know if the result remains intact if I execute the delete query beforehand.
        {
            // and SendPetitionQueryOpcode reads data from the DB
            Field* fields = result->Fetch();
            ObjectGuid ownerguid   = ObjectGuid(HIGHGUID_PLAYER, fields[0].GetUInt32());
            ObjectGuid petitionguid = ObjectGuid(HIGHGUID_ITEM, fields[1].GetUInt32());

            // send update if charter owner in game
            Player* owner = sObjectMgr.GetPlayer(ownerguid);
            if (owner)
            {
                owner->GetSession()->SendPetitionQueryOpcode(petitionguid);
            }
        }
        while (result->NextRow());

        delete result;

        CharacterDatabase.PExecute("DELETE FROM `petition_sign` WHERE `playerguid` = '%u'", lowguid);
    }

    CharacterDatabase.BeginTransaction();
    CharacterDatabase.PExecute("DELETE FROM `petition` WHERE `ownerguid` = '%u'", lowguid);
    CharacterDatabase.PExecute("DELETE FROM `petition_sign` WHERE `ownerguid` = '%u'", lowguid);
    CharacterDatabase.CommitTransaction();
}

void Player::LeaveAllArenaTeams(ObjectGuid guid)
{
    uint32 lowguid = guid.GetCounter();
    QueryResult* result = CharacterDatabase.PQuery("SELECT `arena_team_member`.`arenateamid` FROM `arena_team_member` JOIN `arena_team` ON `arena_team_member`.`arenateamid` = `arena_team`.`arenateamid` WHERE `guid`='%u'", lowguid);
    if (!result)
    {
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        if (uint32 at_id = fields[0].GetUInt32())
            if (ArenaTeam* at = sObjectMgr.GetArenaTeamById(at_id))
            {
                at->DelMember(guid);
            }
    }
    while (result->NextRow());

    delete result;
}

/**
 * @brief Sets the player's accumulated rested experience bonus.
 *
 * @param rest_bonus_new The new rested bonus amount.
 */
void Player::SetRestBonus(float rest_bonus_new)
{
    // Prevent resting on max level
    if (getLevel() >= sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
    {
        rest_bonus_new = 0;
    }

    if (rest_bonus_new < 0)
    {
        rest_bonus_new = 0;
    }

    float rest_bonus_max = (float)GetUInt32Value(PLAYER_NEXT_LEVEL_XP) * 1.5f / 2.0f;

    if (rest_bonus_new > rest_bonus_max)
    {
        m_rest_bonus = rest_bonus_max;
    }
    else
    {
        m_rest_bonus = rest_bonus_new;
    }

    // update data for client
    if (m_rest_bonus > 10)
    {
        SetByteValue(PLAYER_BYTES_2, 3, REST_STATE_RESTED);
    }
    else if (m_rest_bonus <= 1)
    {
        SetByteValue(PLAYER_BYTES_2, 3, REST_STATE_NORMAL);
    }

    // RestTickUpdate
    SetUInt32Value(PLAYER_REST_STATE_EXPERIENCE, uint32(m_rest_bonus));
}

/**
 * @brief Updates visibility of nearby stealthed units based on detection checks.
 */
void Player::HandleStealthedUnitsDetection()
{
    std::list<Unit*> stealthedUnits;

    MaNGOS::AnyStealthedCheck u_check(this);
    MaNGOS::UnitListSearcher<MaNGOS::AnyStealthedCheck > searcher(stealthedUnits, u_check);
    Cell::VisitAllObjects(this, searcher, MAX_PLAYER_STEALTH_DETECT_RANGE);

    WorldObject const* viewPoint = GetCamera().GetBody();

    for (std::list<Unit*>::const_iterator i = stealthedUnits.begin(); i != stealthedUnits.end(); ++i)
    {
        if ((*i) == this)
        {
            continue;
        }

        bool hasAtClient = HaveAtClient((*i));
        bool hasDetected = (*i)->IsVisibleForOrDetect(this, viewPoint, true);

        if (hasDetected)
        {
            if (!hasAtClient)
            {
                ObjectGuid i_guid = (*i)->GetObjectGuid();
                (*i)->SendCreateUpdateToPlayer(this);
                m_clientGUIDs.insert(i_guid);

                DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "%s is detected in stealth by player %u. Distance = %f", i_guid.GetString().c_str(), GetGUIDLow(), GetDistance(*i));

                // target aura duration for caster show only if target exist at caster client
                // send data at target visibility change (adding to client)
                if ((*i) != this && (*i)->isType(TYPEMASK_UNIT))
                {
                    SendAurasForTarget(*i);
                }
            }
        }
        else
        {
            if (hasAtClient)
            {
                (*i)->DestroyForPlayer(this);
                m_clientGUIDs.erase((*i)->GetObjectGuid());
            }
        }
    }
}

/**
 * @brief Starts a taxi flight across a sequence of taxi nodes.
 *
 * @param nodes The ordered taxi node path to travel.
 * @param npc The taxi master providing the route, or NULL for spell/scripted travel.
 * @param spellid The spell initiating the taxi flight, if any.
 * @return True if the flight started successfully; otherwise, false.
 */
bool Player::ActivateTaxiPathTo(std::vector<uint32> const& nodes, Creature* npc /*= NULL*/, uint32 spellid /*= 0*/)
{
    if (nodes.size() < 2)
    {
        return false;
    }

    // not let cheating with start flight in time of logout process || if casting not finished || while in combat || if not use Spell's with EffectSendTaxi
    if (GetSession()->isLogingOut() || IsInCombat())
    {
        GetSession()->SendActivateTaxiReply(ERR_TAXIPLAYERBUSY);
        return false;
    }

    if (HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
    {
        return false;
    }

    // taximaster case
    if (npc)
    {
        // not let cheating with start flight mounted
        if (IsMounted())
        {
            GetSession()->SendActivateTaxiReply(ERR_TAXIPLAYERALREADYMOUNTED);
            return false;
        }

        if (IsInDisallowedMountForm())
        {
            GetSession()->SendActivateTaxiReply(ERR_TAXIPLAYERSHAPESHIFTED);
            return false;
        }

        // not let cheating with start flight in time of logout process || if casting not finished || while in combat || if not use Spell's with EffectSendTaxi
        if (IsNonMeleeSpellCasted(false))
        {
            GetSession()->SendActivateTaxiReply(ERR_TAXIPLAYERBUSY);
            return false;
        }
    }
    // cast case or scripted call case
    else
    {
        RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);

        if (IsInDisallowedMountForm())
        {
            RemoveSpellsCausingAura(SPELL_AURA_MOD_SHAPESHIFT);
        }

        if (Spell* spell = GetCurrentSpell(CURRENT_GENERIC_SPELL))
            if (spell->m_spellInfo->Id != spellid)
            {
                InterruptSpell(CURRENT_GENERIC_SPELL, false);
            }

        InterruptSpell(CURRENT_AUTOREPEAT_SPELL, false);

        if (Spell* spell = GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            if (spell->m_spellInfo->Id != spellid)
            {
                InterruptSpell(CURRENT_CHANNELED_SPELL, true);
            }
    }

    uint32 sourcenode = nodes[0];

    // starting node too far away (cheat?)
    TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(sourcenode);
    if (!node)
    {
        GetSession()->SendActivateTaxiReply(ERR_TAXINOSUCHPATH);
        return false;
    }

    // check node starting pos data set case if provided
    if (node->x != 0.0f || node->y != 0.0f || node->z != 0.0f)
    {
        if (node->map_id != GetMapId() ||
                (node->x - GetPositionX()) * (node->x - GetPositionX()) +
                (node->y - GetPositionY()) * (node->y - GetPositionY()) +
                (node->z - GetPositionZ()) * (node->z - GetPositionZ()) >
                (2 * INTERACTION_DISTANCE) * (2 * INTERACTION_DISTANCE) * (2 * INTERACTION_DISTANCE))
        {
            GetSession()->SendActivateTaxiReply(ERR_TAXITOOFARAWAY);
            return false;
        }
    }
    // node must have pos if taxi master case (npc != NULL)
    else if (npc)
    {
        GetSession()->SendActivateTaxiReply(ERR_TAXIUNSPECIFIEDSERVERERROR);
        return false;
    }

    // Prepare to flight start now

    // stop combat at start taxi flight if any
    CombatStop();

    // stop trade (client cancel trade at taxi map open but cheating tools can be used for reopen it)
    TradeCancel(true);

    // clean not finished taxi path if any
    m_taxi.ClearTaxiDestinations();

    // 0 element current node
    m_taxi.AddTaxiDestination(sourcenode);

    // fill destinations path tail
    uint32 sourcepath = 0;
    uint32 totalcost = 0;
    uint32 firstcost = 0;

    uint32 prevnode = sourcenode;
    uint32 lastnode = 0;

    for (uint32 i = 1; i < nodes.size(); ++i)
    {
        uint32 path, cost;

        lastnode = nodes[i];
        sObjectMgr.GetTaxiPath(prevnode, lastnode, path, cost);

        if (!path)
        {
            m_taxi.ClearTaxiDestinations();
            return false;
        }

        totalcost += cost;

        if (i == 1)
        {
            firstcost = cost;
        }

        if (prevnode == sourcenode)
        {
            sourcepath = path;
        }

        m_taxi.AddTaxiDestination(lastnode);

        prevnode = lastnode;
    }

    // get mount model (in case non taximaster (npc==NULL) allow more wide lookup)
    uint32 mount_display_id = sObjectMgr.GetTaxiMountDisplayId(sourcenode, GetTeam(), npc == NULL);

    // in spell case allow 0 model
    if ((mount_display_id == 0 && spellid == 0) || sourcepath == 0)
    {
        GetSession()->SendActivateTaxiReply(ERR_TAXIUNSPECIFIEDSERVERERROR);

        m_taxi.ClearTaxiDestinations();
        return false;
    }

    uint64 money = GetMoney();

    if (npc)
    {
        float discount = GetReputationPriceDiscount(npc);

        totalcost = uint32(ceil(totalcost * discount));
        firstcost = uint32(ceil(firstcost * discount));

        m_taxi.SetFlightMasterFactionTemplateId(npc->getFaction());
    }
    else
    {
        m_taxi.SetFlightMasterFactionTemplateId(0);
    }

    if (money < totalcost)
    {
        GetSession()->SendActivateTaxiReply(ERR_TAXINOTENOUGHMONEY);

        m_taxi.ClearTaxiDestinations();
        return false;
    }

    // Checks and preparations done, DO FLIGHT
    ModifyMoney(-(int64)totalcost);
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING, totalcost);
    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN, 1);

    // prevent stealth flight
    RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);

    GetSession()->SendActivateTaxiReply(ERR_TAXIOK);
    GetSession()->SendDoFlight(mount_display_id, sourcepath);

    return true;
}

/**
 * @brief Starts a taxi flight using a direct taxi path identifier.
 *
 * @param taxi_path_id The taxi path identifier to use.
 * @param spellid The spell initiating the taxi flight, if any.
 * @return True if the flight started successfully; otherwise, false.
 */
bool Player::ActivateTaxiPathTo(uint32 taxi_path_id, uint32 spellid /*= 0*/)
{
    TaxiPathEntry const* entry = sTaxiPathStore.LookupEntry(taxi_path_id);
    if (!entry)
    {
        return false;
    }

    std::vector<uint32> nodes;

    nodes.resize(2);
    nodes[0] = entry->from;
    nodes[1] = entry->to;

    return ActivateTaxiPathTo(nodes, NULL, spellid);
}

/**
 * @brief Resumes an interrupted taxi flight from the nearest path node.
 */
void Player::ContinueTaxiFlight()
{
    uint32 sourceNode = m_taxi.GetTaxiSource();
    if (!sourceNode)
    {
        return;
    }

    DEBUG_LOG("WORLD: Restart character %u taxi flight", GetGUIDLow());

    uint32 mountDisplayId = sObjectMgr.GetTaxiMountDisplayId(sourceNode, GetTeam(), true);
    uint32 path = m_taxi.GetCurrentTaxiPath();

    // search appropriate start path node
    uint32 startNode = 0;

    TaxiPathNodeList const& nodeList = sTaxiPathNodesByPath[path];

    float distPrev = MAP_SIZE * MAP_SIZE;
    float distNext =
        (nodeList[0].x - GetPositionX()) * (nodeList[0].x - GetPositionX()) +
        (nodeList[0].y - GetPositionY()) * (nodeList[0].y - GetPositionY()) +
        (nodeList[0].z - GetPositionZ()) * (nodeList[0].z - GetPositionZ());

    for (uint32 i = 1; i < nodeList.size(); ++i)
    {
        TaxiPathNodeEntry const& node = nodeList[i];
        TaxiPathNodeEntry const& prevNode = nodeList[i - 1];

        // skip nodes at another map
        if (node.mapid != GetMapId())
        {
            continue;
        }

        distPrev = distNext;

        distNext =
            (node.x - GetPositionX()) * (node.x - GetPositionX()) +
            (node.y - GetPositionY()) * (node.y - GetPositionY()) +
            (node.z - GetPositionZ()) * (node.z - GetPositionZ());

        float distNodes =
            (node.x - prevNode.x) * (node.x - prevNode.x) +
            (node.y - prevNode.y) * (node.y - prevNode.y) +
            (node.z - prevNode.z) * (node.z - prevNode.z);

        if (distNext + distPrev < distNodes)
        {
            startNode = i;
            break;
        }
    }

    GetSession()->SendDoFlight(mountDisplayId, path, startNode);
}

/**
 * @brief Applies cooldown lockouts to spells in the specified school mask.
 *
 * @param idSchoolMask The spell school mask to prohibit.
 * @param unTimeMs The prohibition duration in milliseconds.
 */
void Player::ProhibitSpellSchool(SpellSchoolMask idSchoolMask, uint32 unTimeMs)
{
    // last check 4.3.4
    WorldPacket data(SMSG_SPELL_COOLDOWN, 8 + 1 + m_spells.size() * 8);
    data << GetObjectGuid();
    data << uint8(0x0);                                     // flags (0x1, 0x2)
    time_t curTime = time(NULL);
    for (PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if (itr->second.state == PLAYERSPELL_REMOVED)
        {
            continue;
        }
        uint32 unSpellId = itr->first;
        SpellEntry const* spellInfo = sSpellStore.LookupEntry(unSpellId);
        MANGOS_ASSERT(spellInfo);

        // Not send cooldown for this spells
        if (spellInfo->HasAttribute(SPELL_ATTR_DISABLED_WHILE_ACTIVE))
        {
            continue;
        }

        if ((idSchoolMask & GetSpellSchoolMask(spellInfo)) && GetSpellCooldownDelay(unSpellId) < unTimeMs)
        {
            data << uint32(unSpellId);
            data << uint32(unTimeMs);                       // in m.secs
            AddSpellCooldown(unSpellId, 0, curTime + unTimeMs / IN_MILLISECONDS);
        }
    }
    GetSession()->SendPacket(&data);
}

/**
 * @brief Reinitializes player combat data for the current shapeshift form.
 *
 * @param reapplyMods True when reapplying modifiers without a real form change.
 */
void Player::InitDataForForm(bool reapplyMods)
{
    ShapeshiftForm form = GetShapeshiftForm();

    SpellShapeshiftFormEntry const* ssEntry = sSpellShapeshiftFormStore.LookupEntry(form);
    if (ssEntry && ssEntry->attackSpeed)
    {
        SetAttackTime(BASE_ATTACK, ssEntry->attackSpeed);
        SetAttackTime(OFF_ATTACK, ssEntry->attackSpeed);
        SetAttackTime(RANGED_ATTACK, BASE_ATTACK_TIME);
    }
    else
    {
        SetRegularAttackTime();
    }

    switch (form)
    {
        case FORM_CAT:
        {
            if (GetPowerType() != POWER_ENERGY)
            {
                SetPowerType(POWER_ENERGY);
            }
            break;
        }
        case FORM_BEAR:
        {
            if (GetPowerType() != POWER_RAGE)
            {
                SetPowerType(POWER_RAGE);
            }
            break;
        }
        default:                                            // 0, for example
        {
            ChrClassesEntry const* cEntry = sChrClassesStore.LookupEntry(getClass());
            if (cEntry && cEntry->powerType < MAX_POWERS && uint32(GetPowerType()) != cEntry->powerType)
            {
                SetPowerType(Powers(cEntry->powerType));
            }
            break;
        }
    }

    // update auras at form change, ignore this at mods reapply (.reset stats/etc) when form not change.
    if (!reapplyMods)
    {
        UpdateEquipSpellsAtFormChange();
    }

    UpdateAttackPowerAndDamage();
    UpdateAttackPowerAndDamage(true);
}

/**
 * @brief Initializes the player's native and current display identifiers.
 */
void Player::InitDisplayIds()
{
    PlayerInfo const* info = sObjectMgr.GetPlayerInfo(getRace(), getClass());
    if (!info)
    {
        sLog.outError("Player %u has incorrect race/class pair. Can't init display ids.", GetGUIDLow());
        return;
    }

    // reset scale before reapply auras
    SetObjectScale(DEFAULT_OBJECT_SCALE);

    uint8 gender = getGender();
    switch (gender)
    {
        case GENDER_FEMALE:
            SetDisplayId(info->displayId_f);
            SetNativeDisplayId(info->displayId_f);
            break;
        case GENDER_MALE:
            SetDisplayId(info->displayId_m);
            SetNativeDisplayId(info->displayId_m);
            break;
        default:
            sLog.outError("Invalid gender %u for player", gender);
            return;
    }
}


/**
 * @brief Updates the invalid-instance homebind timer and teleports when it expires.
 *
 * @param time The elapsed update time in milliseconds.
 */
void Player::UpdateHomebindTime(uint32 time)
{
    // GMs never get homebind timer online
    if (m_InstanceValid || isGameMaster())
    {
        if (m_HomebindTimer)                                // instance valid, but timer not reset
        {
            // hide reminder
            WorldPacket data(SMSG_RAID_GROUP_ONLY, 4 + 4);
            data << uint32(0);
            data << uint32(ERR_RAID_GROUP_NONE);            // error used only when timer = 0
            GetSession()->SendPacket(&data);
        }
        // instance is valid, reset homebind timer
        m_HomebindTimer = 0;
    }
    else if (m_HomebindTimer > 0)
    {
        if (time >= m_HomebindTimer)
        {
            // teleport to nearest graveyard
            RepopAtGraveyard();
        }
        else
        {
            m_HomebindTimer -= time;
        }
    }
    else
    {
        // instance is invalid, start homebind timer
        m_HomebindTimer = 60000;
        // send message to player
        WorldPacket data(SMSG_RAID_GROUP_ONLY, 4 + 4);
        data << uint32(m_HomebindTimer);
        data << uint32(ERR_RAID_GROUP_NONE);                // error used only when timer = 0
        GetSession()->SendPacket(&data);
        DEBUG_LOG("PLAYER: Player '%s' (GUID: %u) will be teleported to homebind in 60 seconds", GetName(), GetGUIDLow());
    }
}

/**
 * @brief Sets or clears the player's PvP state with timeout-aware handling.
 *
 * @param state True to enable PvP; false to disable it.
 * @param ovrride True to bypass the delayed PvP timeout behavior.
 */
void Player::UpdatePvP(bool state, bool ovrride)
{
    if (!state || ovrride)
    {
        SetPvP(state);
        pvpInfo.endTimer = 0;
    }
    else
    {
        if (pvpInfo.endTimer != 0)
        {
            pvpInfo.endTimer = time(NULL);
        }
        else
        {
            SetPvP(state);
        }
    }
}

// slot to be excluded while counting
bool Player::EnchantmentFitsRequirements(uint32 enchantmentcondition, int8 slot)
{
    if (!enchantmentcondition)
    {
        return true;
    }

    SpellItemEnchantmentConditionEntry const* Condition = sSpellItemEnchantmentConditionStore.LookupEntry(enchantmentcondition);

    if (!Condition)
    {
        return true;
    }

    uint8 curcount[4] = {0, 0, 0, 0};

    // counting current equipped gem colors
    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (i == slot)
        {
            continue;
        }
        Item* pItem2 = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (pItem2 && !pItem2->IsBroken() && pItem2->GetProto()->Socket[0].Color)
        {
            for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + 3; ++enchant_slot)
            {
                uint32 enchant_id = pItem2->GetEnchantmentId(EnchantmentSlot(enchant_slot));
                if (!enchant_id)
                {
                    continue;
                }

                SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                if (!enchantEntry)
                {
                    continue;
                }

                uint32 gemid = enchantEntry->GemID;
                if (!gemid)
                {
                    continue;
                }

                ItemPrototype const* gemProto = sItemStorage.LookupEntry<ItemPrototype>(gemid);
                if (!gemProto)
                {
                    continue;
                }

                GemPropertiesEntry const* gemProperty = sGemPropertiesStore.LookupEntry(gemProto->GemProperties);
                if (!gemProperty)
                {
                    continue;
                }

                uint8 GemColor = gemProperty->color;

                for (uint8 b = 0, tmpcolormask = 1; b < 4; ++b, tmpcolormask <<= 1)
                {
                    if (tmpcolormask & GemColor)
                    {
                        ++curcount[b];
                    }
                }
            }
        }
    }

    bool activate = true;

    for (int i = 0; i < 5; ++i)
    {
        if (!Condition->Color[i])
        {
            continue;
        }

        uint32 _cur_gem = curcount[Condition->Color[i] - 1];

        // if have <CompareColor> use them as count, else use <value> from Condition
        uint32 _cmp_gem = Condition->CompareColor[i] ? curcount[Condition->CompareColor[i] - 1] : Condition->Value[i];

        switch (Condition->Comparator[i])
        {
            case 2:                                         // requires less <color> than (<value> || <comparecolor>) gems
                activate &= (_cur_gem < _cmp_gem) ? true : false;
                break;
            case 3:                                         // requires more <color> than (<value> || <comparecolor>) gems
                activate &= (_cur_gem > _cmp_gem) ? true : false;
                break;
            case 5:                                         // requires at least <color> than (<value> || <comparecolor>) gems
                activate &= (_cur_gem >= _cmp_gem) ? true : false;
                break;
        }
    }

    DEBUG_LOG("Checking Condition %u, there are %u Meta Gems, %u Red Gems, %u Yellow Gems and %u Blue Gems, Activate:%s", enchantmentcondition, curcount[0], curcount[1], curcount[2], curcount[3], activate ? "yes" : "no");

    return activate;
}

void Player::CorrectMetaGemEnchants(uint8 exceptslot, bool apply)
{
    // cycle all equipped items
    for (uint32 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        // enchants for the slot being socketed are handled by Player::ApplyItemMods
        if (slot == exceptslot)
        {
            continue;
        }

        Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

        if (!pItem || !pItem->GetProto()->Socket[0].Color)
        {
            continue;
        }

        for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + 3; ++enchant_slot)
        {
            uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
            if (!enchant_id)
            {
                continue;
            }

            SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
            if (!enchantEntry)
            {
                continue;
            }

            uint32 condition = enchantEntry->EnchantmentCondition;
            if (condition)
            {
                // was enchant active with/without item?
                bool wasactive = EnchantmentFitsRequirements(condition, apply ? exceptslot : -1);
                // should it now be?
                if (wasactive != EnchantmentFitsRequirements(condition, apply ? -1 : exceptslot))
                {
                    // ignore item gem conditions
                    // if state changed, (dis)apply enchant
                    ApplyEnchantment(pItem, EnchantmentSlot(enchant_slot), !wasactive, true, true);
                }
            }
        }
    }
}

// if false -> then toggled off if was on| if true -> toggled on if was off AND meets requirements
void Player::ToggleMetaGemsActive(uint8 exceptslot, bool apply)
{
    // cycle all equipped items
    for (int slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        // enchants for the slot being socketed are handled by WorldSession::HandleSocketOpcode(WorldPacket& recv_data)
        if (slot == exceptslot)
        {
            continue;
        }

        Item* pItem = GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

        if (!pItem || !pItem->GetProto()->Socket[0].Color)  // if item has no sockets or no item is equipped go to next item
        {
            continue;
        }

        // cycle all (gem)enchants
        for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + 3; ++enchant_slot)
        {
            uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
            if (!enchant_id)                                // if no enchant go to next enchant(slot)
            {
                continue;
            }

            SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
            if (!enchantEntry)
            {
                continue;
            }

            // only metagems to be (de)activated, so only enchants with condition
            uint32 condition = enchantEntry->EnchantmentCondition;
            if (condition)
            {
                ApplyEnchantment(pItem, EnchantmentSlot(enchant_slot), apply);
            }
        }
    }
}

/**
 * @brief Stores the location used to return the player after leaving a battleground.
 */
void Player::SetBattleGroundEntryPoint()
{
    // Taxi path store
    if (!m_taxi.empty())
    {
        m_bgData.mountSpell  = 0;
        m_bgData.taxiPath[0] = m_taxi.GetTaxiSource();
        m_bgData.taxiPath[1] = m_taxi.GetTaxiDestination();

        // On taxi we don't need check for dungeon
        m_bgData.joinPos = WorldLocation(GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
        m_bgData.m_needSave = true;
        return;
    }
    else
    {
        m_bgData.ClearTaxiPath();

        // Mount spell id storing
        if (IsMounted())
        {
            AuraList const& auras = GetAurasByType(SPELL_AURA_MOUNTED);
            if (!auras.empty())
            {
                m_bgData.mountSpell = (*auras.begin())->GetId();
            }
        }
        else
        {
            m_bgData.mountSpell = 0;
        }

        // If map is dungeon find linked graveyard
        if (GetMap()->IsDungeon())
        {
            if (const WorldSafeLocsEntry* entry = sObjectMgr.GetClosestGraveYard(GetPositionX(), GetPositionY(), GetPositionZ(), GetMapId(), GetTeam()))
            {
                m_bgData.joinPos = WorldLocation(entry->map_id, entry->x, entry->y, entry->z, 0.0f);
                m_bgData.m_needSave = true;
                return;
            }
            else
            {
                sLog.outError("SetBattleGroundEntryPoint: Dungeon map %u has no linked graveyard, setting home location as entry point.", GetMapId());
            }
        }
        // If new entry point is not BG or arena set it
        else if (!GetMap()->IsBattleGroundOrArena())
        {
            m_bgData.joinPos = WorldLocation(GetMapId(), GetPositionX(), GetPositionY(), GetPositionZ(), GetOrientation());
            m_bgData.m_needSave = true;
            return;
        }
    }

    // In error cases use homebind position
    m_bgData.joinPos = WorldLocation(m_homebindMapId, m_homebindX, m_homebindY, m_homebindZ, 0.0f);
    m_bgData.m_needSave = true;
}

/**
 * @brief Removes the player from the battleground and handles deserter penalties.
 *
 * @param teleportToEntryPoint True to teleport the player back to the saved entry point.
 */
void Player::LeaveBattleground(bool teleportToEntryPoint)
{
    if (BattleGround* bg = GetBattleGround())
    {
        bg->RemovePlayerAtLeave(GetObjectGuid(), teleportToEntryPoint, true);

        // call after remove to be sure that player resurrected for correct cast
        if (bg->isBattleGround() && !isGameMaster() && sWorld.getConfig(CONFIG_BOOL_BATTLEGROUND_CAST_DESERTER))
        {
            if (bg->GetStatus() == STATUS_IN_PROGRESS || bg->GetStatus() == STATUS_WAIT_JOIN)
            {
                // lets check if player was teleported from BG and schedule delayed Deserter spell cast
                if (IsBeingTeleportedFar())
                {
                    ScheduleDelayedOperation(DELAYED_SPELL_CAST_DESERTER);
                    return;
                }

                CastSpell(this, 26013, true);               // Deserter
            }
        }
    }
}

/**
 * @brief Checks whether the player may currently join a battleground.
 *
 * @return True if the player can join; otherwise, false.
 */
bool Player::CanJoinToBattleground() const
{
    // check Deserter debuff
    if (GetDummyAura(26013))
    {
        return false;
    }

    return true;
}

bool Player::CanReportAfkDueToLimit()
{
    // a player can complain about 15 people per 5 minutes
    if (m_bgData.bgAfkReportedCount++ >= 15)
    {
        return false;
    }

    return true;
}

/// This player has been blamed to be inactive in a battleground
void Player::ReportedAfkBy(Player* reporter)
{
    BattleGround* bg = GetBattleGround();
    if (!bg || bg != reporter->GetBattleGround() || GetTeam() != reporter->GetTeam())
    {
        return;
    }

    // check if player has 'Idle' or 'Inactive' debuff
    if (m_bgData.bgAfkReporter.find(reporter->GetGUIDLow()) == m_bgData.bgAfkReporter.end() && !HasAura(43680, EFFECT_INDEX_0) && !HasAura(43681, EFFECT_INDEX_0) && reporter->CanReportAfkDueToLimit())
    {
        m_bgData.bgAfkReporter.insert(reporter->GetGUIDLow());
        // 3 players have to complain to apply debuff
        if (m_bgData.bgAfkReporter.size() >= 3)
        {
            // cast 'Idle' spell
            CastSpell(this, 43680, true);
            m_bgData.bgAfkReporter.clear();
        }
    }
}

/**
 * @brief Checks whether this player should be visible to another player in grid range.
 *
 * @param pl The observing player.
 * @return True if this player should be visible; otherwise, false.
 */
bool Player::IsVisibleInGridForPlayer(Player* pl) const
{
    // gamemaster in GM mode see all, including ghosts
    if (pl->isGameMaster() && GetSession()->GetSecurity() <= pl->GetSession()->GetSecurity())
    {
        return true;
    }

    // player see dead player/ghost from own group/raid
    if (IsInSameRaidWith(pl))
    {
        return true;
    }

    // Live player see live player or dead player with not realized corpse
    if (pl->IsAlive() || pl->m_deathTimer > 0)
    {
        return IsAlive() || m_deathTimer > 0;
    }

    // Ghost see other friendly ghosts, that's for sure
    if (!(IsAlive() || m_deathTimer > 0) && IsFriendlyTo(pl))
    {
        return true;
    }

    // Dead player see live players near own corpse
    if (IsAlive())
    {
        if (Corpse* corpse = pl->GetCorpse())
        {
            // 20 - aggro distance for same level, 25 - max additional distance if player level less that creature level
            if (corpse->IsWithinDistInMap(this, (20 + 25) * sWorld.getConfig(CONFIG_FLOAT_RATE_CREATURE_AGGRO)))
            {
                return true;
            }
        }
    }

    // and not see any other
    return false;
}

/**
 * @brief Checks whether this player should appear in global player visibility contexts.
 *
 * @param u The player attempting to see this player.
 * @return True if this player is globally visible; otherwise, false.
 */
bool Player::IsVisibleGloballyFor(Player* u) const
{
    if (!u)
    {
        return false;
    }

    // Always can see self
    if (u == this)
    {
        return true;
    }

    // Visible units, always are visible for all players
    if (GetVisibility() == VISIBILITY_ON)
    {
        return true;
    }

    // GMs are visible for higher gms (or players are visible for gms)
    if (u->GetSession()->GetSecurity() > SEC_PLAYER)
    {
        return GetSession()->GetSecurity() <= u->GetSession()->GetSecurity();
    }

    // non faction visibility non-breakable for non-GMs
    if (GetVisibility() == VISIBILITY_OFF)
    {
        return false;
    }

    // non-gm stealth/invisibility not hide from global player lists
    return true;
}

template<class T>
inline void BeforeVisibilityDestroy(T* /*t*/, Player* /*p*/)
{
}

template<>
inline void BeforeVisibilityDestroy<Creature>(Creature* t, Player* p)
{
    if (p->GetPetGuid() == t->GetObjectGuid() && ((Creature*)t)->IsPet())
    {
        ((Pet*)t)->Unsummon(PET_SAVE_REAGENTS);
    }
}

/**
 * @brief Updates visibility of a single world object for the player.
 *
 * @param viewPoint The viewpoint used for visibility checks.
 * @param target The target object whose visibility is being updated.
 */
void Player::UpdateVisibilityOf(WorldObject const* viewPoint, WorldObject* target)
{
    if (HaveAtClient(target))
    {
        if (!target->IsVisibleForInState(this, viewPoint, true))
        {
            ObjectGuid t_guid = target->GetObjectGuid();

            if (target->GetTypeId() == TYPEID_UNIT)
            {
                BeforeVisibilityDestroy<Creature>((Creature*)target, this);

                // at remove from map (destroy) show kill animation (in different out of range/stealth case)
                target->DestroyForPlayer(this, !target->IsInWorld() && ((Creature*)target)->IsDead());
            }
            else
            {
                target->DestroyForPlayer(this);
            }

            m_clientGUIDs.erase(t_guid);

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "UpdateVisibilityOf: %s out of range for player %u. Distance = %f", t_guid.GetString().c_str(), GetGUIDLow(), GetDistance(target));
        }
    }
    else
    {
        if (target->IsVisibleForInState(this, viewPoint, false))
        {
            target->SendCreateUpdateToPlayer(this);
            if (target->GetTypeId() != TYPEID_GAMEOBJECT || !((GameObject*)target)->IsTransport())
            {
                m_clientGUIDs.insert(target->GetObjectGuid());
            }

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "UpdateVisibilityOf: %s is visible now for player %u. Distance = %f", target->GetGuidStr().c_str(), GetGUIDLow(), GetDistance(target));

            // target aura duration for caster show only if target exist at caster client
            // send data at target visibility change (adding to client)
            if (target != this && target->isType(TYPEMASK_UNIT))
            {
                SendAurasForTarget((Unit*)target);
            }
        }
    }
}

template<class T>
inline void UpdateVisibilityOf_helper(GuidSet& s64, T* target)
{
    s64.insert(target->GetObjectGuid());
}

template<>
inline void UpdateVisibilityOf_helper(GuidSet& s64, GameObject* target)
{
    if (!target->IsTransport())
    {
        s64.insert(target->GetObjectGuid());
    }
}

template<class T>
void Player::UpdateVisibilityOf(WorldObject const* viewPoint, T* target, UpdateData& data, std::set<WorldObject*>& visibleNow)
{
    if (HaveAtClient(target))
    {
        if (!target->IsVisibleForInState(this, viewPoint, true))
        {
            BeforeVisibilityDestroy<T>(target, this);

            ObjectGuid t_guid = target->GetObjectGuid();

            target->BuildOutOfRangeUpdateBlock(&data);
            m_clientGUIDs.erase(t_guid);

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "UpdateVisibilityOf(TemplateV): %s is out of range for %s. Distance = %f", t_guid.GetString().c_str(), GetGuidStr().c_str(), GetDistance(target));
        }
    }
    else
    {
        if (target->IsVisibleForInState(this, viewPoint, false))
        {
            visibleNow.insert(target);
            target->BuildCreateUpdateBlockForPlayer(&data, this);
            UpdateVisibilityOf_helper(m_clientGUIDs, target);

            DEBUG_FILTER_LOG(LOG_FILTER_VISIBILITY_CHANGES, "UpdateVisibilityOf(TemplateV): %s is visible now for %s. Distance = %f", target->GetGuidStr().c_str(), GetGuidStr().c_str(), GetDistance(target));
        }
    }
}

template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, Player*        target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, Creature*      target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, Corpse*        target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, GameObject*    target, UpdateData& data, std::set<WorldObject*>& visibleNow);
template void Player::UpdateVisibilityOf(WorldObject const* viewPoint, DynamicObject* target, UpdateData& data, std::set<WorldObject*>& visibleNow);

/**
 * @brief Initializes the number of primary professions the player may learn.
 */
void Player::InitPrimaryProfessions()
{
    SetFreePrimaryProfessions(sWorld.getConfig(CONFIG_UINT32_MAX_PRIMARY_TRADE_SKILL));
}

/**
 * @brief Sends the current combo point target and value to the client fields.
 */
void Player::SendComboPoints()
{
    Unit* combotarget = sObjectAccessor.GetUnit(*this, m_comboTargetGuid);
    if (combotarget)
    {
        WorldPacket data(SMSG_UPDATE_COMBO_POINTS, combotarget->GetPackGUID().size() + 1);
        data << combotarget->GetPackGUID();
        data << uint8(m_comboPoints);
        GetSession()->SendPacket(&data);
    }
    /*else
    {
        // can be NULL, and then points=0. Use unknown; to reset points of some sort?
        data << PackedGuid();
        data << uint8(0);
        GetSession()->SendPacket(&data);
    }*/
}

/**
 * @brief Adds combo points on a target and updates combo point ownership.
 *
 * @param target The unit receiving combo points.
 * @param count The number of combo points to add.
 */
void Player::AddComboPoints(Unit* target, int8 count)
{
    if (!count)
    {
        return;
    }

    // without combo points lost (duration checked in aura)
    RemoveSpellsCausingAura(SPELL_AURA_RETAIN_COMBO_POINTS);

    if (target->GetObjectGuid() == m_comboTargetGuid)
    {
        m_comboPoints += count;
    }
    else
    {
        if (m_comboTargetGuid)
            if (Unit* target2 = sObjectAccessor.GetUnit(*this, m_comboTargetGuid))
            {
                target2->RemoveComboPointHolder(GetGUIDLow());
            }

        m_comboTargetGuid = target->GetObjectGuid();
        m_comboPoints = count;

        target->AddComboPointHolder(GetGUIDLow());
    }

    if (m_comboPoints > 5)
    {
        m_comboPoints = 5;
    }
    if (m_comboPoints < 0)
    {
        m_comboPoints = 0;
    }

    SendComboPoints();
}

/**
 * @brief Clears the player's combo points and current combo target.
 */
void Player::ClearComboPoints()
{
    if (!m_comboTargetGuid)
    {
        return;
    }

    // without combopoints lost (duration checked in aura)
    RemoveSpellsCausingAura(SPELL_AURA_RETAIN_COMBO_POINTS);

    m_comboPoints = 0;

    SendComboPoints();

    if (Unit* target = sObjectAccessor.GetUnit(*this, m_comboTargetGuid))
    {
        target->RemoveComboPointHolder(GetGUIDLow());
    }

    m_comboTargetGuid.Clear();
}

/**
 * @brief Assigns the player to a group and subgroup.
 *
 * @param group The group to join, or NULL to clear membership.
 * @param subgroup The subgroup index when joining a group.
 */
void Player::SetGroup(Group* group, int8 subgroup)
{
    if (group == NULL)
    {
        m_group.unlink();
    }
    else
    {
        // never use SetGroup without a subgroup unless you specify NULL for group
        MANGOS_ASSERT(subgroup >= 0);
        m_group.link(group, this);
        m_group.setSubGroup((uint8)subgroup);
    }
}

void Player::SendInitialPacketsBeforeAddToMap()
{
    GetSocial()->SendSocialList();

    // Homebind
    WorldPacket data(SMSG_BINDPOINTUPDATE, 5 * 4);
    data << m_homebindX << m_homebindY << m_homebindZ;
    data << (uint32) m_homebindMapId;
    data << (uint32) m_homebindAreaId;
    GetSession()->SendPacket(&data);

    // SMSG_SET_PROFICIENCY
    // SMSG_SET_PCT_SPELL_MODIFIER
    // SMSG_SET_FLAT_SPELL_MODIFIER

    SendTalentsInfoData(false);

    data.Initialize(SMSG_WORLD_SERVER_INFO, 1 + 1 + 4 + 4);
    data.WriteBit(0);                                               // HasRestrictedLevel
    data.WriteBit(0);                                               // HasRestrictedMoney
    data.WriteBit(0);                                               // IneligibleForLoot

    //if (IneligibleForLoot)
    //    data << uint32(0);                                        // EncounterMask

    data << uint8(0);                                               // IsOnTournamentRealm

    //if (HasRestrictedMoney)
    //    data << uint32(100000);                                   // RestrictedMoney (starter accounts)
    //if (HasRestrictedLevel)
    //    data << uint32(20);                                       // RestrictedLevel (starter accounts)

    data << uint32(sWorld.GetNextWeeklyQuestsResetTime() - WEEK);   // LastWeeklyReset (not instance reset)
    data << uint32(GetMap()->GetDifficulty());
    GetSession()->SendPacket(&data);

    SendInitialSpells();

    data.Initialize(SMSG_SEND_UNLEARN_SPELLS, 4);
    data << uint32(0);                                      // count, for(count) uint32;
    GetSession()->SendPacket(&data);

    SendInitialActionButtons();
    m_reputationMgr.SendInitialReputations();

    if (!IsAlive())
    {
        SendCorpseReclaimDelay(true);
    }

    SendInitWorldStates(GetZoneId(), GetAreaId());

    SendEquipmentSetList();

    m_achievementMgr.SendAllAchievementData();

    data.Initialize(SMSG_LOGIN_SETTIMESPEED, 4 + 4 + 4);
    data << uint32(secsToTimeBitFields(sWorld.GetGameTime()));
    data << (float)0.01666667f;                             // game speed
    data << uint32(0);                                      // added in 3.1.2
    GetSession()->SendPacket(&data);

    // SMSG_TALENTS_INFO x 2 for pet (unspent points and talents in separate packets...)
    // SMSG_PET_GUIDS
    // SMSG_POWER_UPDATE

    // set fly flag if in fly form or taxi flight to prevent visually drop at ground in showup moment
    if (IsFreeFlying() || IsTaxiFlying())
    {
        m_movementInfo.AddMovementFlag(MOVEFLAG_FLYING);
    }

    SendCurrencies();

    SetMover(this);
}

/**
 * @brief Sends map-dependent initialization packets after the player is added to the world.
 */
void Player::SendInitialPacketsAfterAddToMap()
{
    // update zone
    uint32 newzone, newarea;
    GetZoneAndAreaId(newzone, newarea);
    UpdateZone(newzone, newarea);                           // also call SendInitWorldStates();

    ResetTimeSync();
    SendTimeSync();

    CastSpell(this, 836, true);                             // LOGINEFFECT

    // set some aura effects that send packet to player client after add player to map
    // SendMessageToSet not send it to player not it map, only for aura that not changed anything at re-apply
    // same auras state lost at far teleport, send it one more time in this case also
    static const AuraType auratypes[] =
    {
        SPELL_AURA_MOD_FEAR,     SPELL_AURA_TRANSFORM,                 SPELL_AURA_WATER_WALK,
        SPELL_AURA_FEATHER_FALL, SPELL_AURA_HOVER,                     SPELL_AURA_SAFE_FALL,
        SPELL_AURA_FLY,          SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED,  SPELL_AURA_NONE
    };
    for (AuraType const* itr = &auratypes[0]; itr && itr[0] != SPELL_AURA_NONE; ++itr)
    {
        Unit::AuraList const& auraList = GetAurasByType(*itr);
        if (!auraList.empty())
        {
            auraList.front()->ApplyModifier(true, true);
        }
    }

    if (HasAuraType(SPELL_AURA_MOD_STUN) || HasAuraType(SPELL_AURA_MOD_ROOT))
    {
        SetRoot(true);
    }

    SendAurasForTarget(this);
    SendEnchantmentDurations();                             // must be after add to map
    SendItemDurations();                                    // must be after add to map

    UpdateSpeed(MOVE_RUN, true, 1.0f, true);
    UpdateSpeed(MOVE_SWIM, true, 1.0f, true);
    UpdateSpeed(MOVE_FLIGHT, true, 1.0f, true);
}

/**
 * @brief Flushes pending group update data to out-of-range group members.
 */
void Player::SendUpdateToOutOfRangeGroupMembers()
{
    if (m_groupUpdateMask == GROUP_UPDATE_FLAG_NONE)
    {
        return;
    }
    if (Group* group = GetGroup())
    {
        group->UpdatePlayerOutOfRange(this);
    }

    m_groupUpdateMask = GROUP_UPDATE_FLAG_NONE;
    m_auraUpdateMask = 0;
    if (Pet* pet = GetPet())
    {
        pet->ResetAuraUpdateMask();
    }
}

/**
 * @brief Sends the appropriate transfer-aborted feedback for an area lock failure.
 *
 * @param mapEntry The destination map entry.
 * @param lockStatus The evaluated area lock status.
 * @param miscRequirement Extra requirement data used by some messages.
 */
void Player::SendTransferAbortedByLockStatus(MapEntry const* mapEntry, AreaLockStatus lockStatus, uint32 miscRequirement)
{
    MANGOS_ASSERT(mapEntry);

    DEBUG_LOG("SendTransferAbortedByLockStatus: Called for %s on map %u, LockAreaStatus %u, miscRequirement %u)", GetGuidStr().c_str(), mapEntry->MapID, lockStatus, miscRequirement);

    switch (lockStatus)
    {
        case AREA_LOCKSTATUS_TOO_LOW_LEVEL:
            GetSession()->SendAreaTriggerMessage(GetSession()->GetMangosString(LANG_LEVEL_MINREQUIRED), miscRequirement);
            break;
        case AREA_LOCKSTATUS_ZONE_IN_COMBAT:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_ZONE_IN_COMBAT);
            break;
        case AREA_LOCKSTATUS_INSTANCE_IS_FULL:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_MAX_PLAYERS);
            break;
        case AREA_LOCKSTATUS_QUEST_NOT_COMPLETED:
            if (mapEntry->MapID == 269)                     // Exception for Black Morass
            {
                GetSession()->SendAreaTriggerMessage("%s", GetSession()->GetMangosString(LANG_TELEREQ_QUEST_BLACK_MORASS));
                break;
            }
            else if (mapEntry->IsContinent())               // do not report anything for quest areatrigge
            {
                DEBUG_LOG("SendTransferAbortedByLockStatus: LockAreaStatus %u, do not teleport, no message sent (mapId %u)", lockStatus, mapEntry->MapID);
                break;
            }
            // No break here!
            [[fallthrough]];
        case AREA_LOCKSTATUS_MISSING_ITEM:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_DIFFICULTY, GetDifficulty(mapEntry->IsRaid()));
            break;
        case AREA_LOCKSTATUS_MISSING_DIFFICULTY:
        {
            Difficulty difficulty = GetDifficulty(mapEntry->IsRaid());
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_DIFFICULTY, difficulty > RAID_DIFFICULTY_10MAN_HEROIC ? RAID_DIFFICULTY_10MAN_HEROIC : difficulty);
            break;
        }
        case AREA_LOCKSTATUS_INSUFFICIENT_EXPANSION:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_INSUF_EXPAN_LVL, miscRequirement);
            break;
        case AREA_LOCKSTATUS_NOT_ALLOWED:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_MAP_NOT_ALLOWED);
            break;
        case AREA_LOCKSTATUS_RAID_LOCKED:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_NEED_GROUP);
            break;
        case AREA_LOCKSTATUS_UNKNOWN_ERROR:
            GetSession()->SendTransferAborted(mapEntry->MapID, TRANSFER_ABORT_ERROR);
            break;
        case AREA_LOCKSTATUS_OK:
            sLog.outError("SendTransferAbortedByLockStatus: LockAreaStatus AREA_LOCKSTATUS_OK received for %s (mapId %u)", GetGuidStr().c_str(), mapEntry->MapID);
            MANGOS_ASSERT(false);
            break;
        default:
            sLog.outError("SendTransfertAbortedByLockstatus: unhandled LockAreaStatus %u, when %s attempts to enter in map %u", lockStatus, GetGuidStr().c_str(), mapEntry->MapID);
            break;
    }
}

/**
 * @brief Sends an instance reset warning message to the client.
 *
 * @param mapid The map identifier being reset.
 * @param time The remaining time until reset in seconds.
 */
void Player::SendInstanceResetWarning(uint32 mapid, Difficulty difficulty, uint32 time)
{
    // type of warning, based on the time remaining until reset
    uint32 type;
    if (time > 3600)
    {
        type = RAID_INSTANCE_WELCOME;
    }
    else if (time > 900 && time <= 3600)
    {
        type = RAID_INSTANCE_WARNING_HOURS;
    }
    else if (time > 300 && time <= 900)
    {
        type = RAID_INSTANCE_WARNING_MIN;
    }
    else
    {
        type = RAID_INSTANCE_WARNING_MIN_SOON;
    }

    WorldPacket data(SMSG_RAID_INSTANCE_MESSAGE, 4 + 4 + 4 + 4);
    data << uint32(type);
    data << uint32(mapid);
    data << uint32(difficulty);                             // difficulty
    data << uint32(time);
    if (type == RAID_INSTANCE_WELCOME)
    {
        data << uint8(0);                                   // is your (1)
        data << uint8(0);                                   // is extended (1), ignored if prev field is 0
    }
    GetSession()->SendPacket(&data);
}

/**
 * @brief Applies the default equip cooldown for item use spells.
 *
 * @param pItem The item whose on-use spells should receive equip cooldowns.
 */
void Player::ApplyEquipCooldown(Item* pItem)
{
    if (pItem->GetProto()->Flags & ITEM_FLAG_NO_EQUIP_COOLDOWN)
    {
        return;
    }

    for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        _Spell const& spellData = pItem->GetProto()->Spells[i];

        // no spell
        if (!spellData.SpellId)
        {
            continue;
        }

        // wrong triggering type (note: ITEM_SPELLTRIGGER_ON_NO_DELAY_USE not have cooldown)
        if (spellData.SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
        {
            continue;
        }

        AddSpellCooldown(spellData.SpellId, pItem->GetEntry(), time(NULL) + 30);

        WorldPacket data(SMSG_ITEM_COOLDOWN, 12);
        data << pItem->GetObjectGuid();
        data << uint32(spellData.SpellId);
        GetSession()->SendPacket(&data);
    }
}

/**
 * @brief Resets the player's learned spells and restores default and quest rewards.
 */
void Player::resetSpells()
{
    // not need after this call
    if (HasAtLoginFlag(AT_LOGIN_RESET_SPELLS))
    {
        RemoveAtLoginFlag(AT_LOGIN_RESET_SPELLS, true);
    }

    // make full copy of map (spells removed and marked as deleted at another spell remove
    // and we can't use original map for safe iterative with visit each spell at loop end
    PlayerSpellMap smap = GetSpellMap();

    for (PlayerSpellMap::const_iterator iter = smap.begin(); iter != smap.end(); ++iter)
    {
        removeSpell(iter->first, false, false);              // only iter->first can be accessed, object by iter->second can be deleted already
    }

    learnDefaultSpells();
    learnQuestRewardedSpells();
}

/**
 * @brief Teaches the player's default race and class spells.
 */
void Player::learnDefaultSpells()
{
    // learn default race/class spells
    PlayerInfo const* info = sObjectMgr.GetPlayerInfo(getRace(), getClass());
    for (PlayerCreateInfoSpells::const_iterator itr = info->spell.begin(); itr != info->spell.end(); ++itr)
    {
        uint32 tspell = *itr;
        DEBUG_LOG("PLAYER (Class: %u Race: %u): Adding initial spell, id = %u", uint32(getClass()), uint32(getRace()), tspell);
        if (!IsInWorld())                                   // will send in INITIAL_SPELLS in list anyway at map add
        {
            addSpell(tspell, true, true, true, false);
        }
        else                                                // but send in normal spell in game learn case
        {
            learnSpell(tspell, true);
        }
    }
}

/**
 * @brief Teaches a quest reward spell if the quest grants a learnable spell.
 *
 * @param quest The rewarded quest to inspect.
 */
void Player::learnQuestRewardedSpells(Quest const* quest)
{
    uint32 spell_id = quest->GetRewSpellCast();

    // skip quests without rewarded spell
    if (!spell_id)
    {
        return;
    }

    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);
    if (!spellInfo)
    {
        return;
    }

    // check learned spells state
    bool found = false;
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(SpellEffectIndex(i)))
        {
            if (spellEffect->Effect == SPELL_EFFECT_LEARN_SPELL && !HasSpell(spellEffect->EffectTriggerSpell))
            {
                found = true;
                break;
            }
        }
    }

    // skip quests with not teaching spell or already known spell
    if (!found)
    {
        return;
    }

    // prevent learn non first rank unknown profession and second specialization for same profession)
    SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(EFFECT_INDEX_0);
    uint32 learned_0 = spellEffect ? spellEffect->EffectTriggerSpell : 0;

    if (sSpellMgr.GetSpellRank(learned_0) > 1 && !HasSpell(learned_0))
    {
        // not have first rank learned (unlearned prof?)
        uint32 first_spell = sSpellMgr.GetFirstSpellInChain(learned_0);
        if (!HasSpell(first_spell))
        {
            return;
        }

        SpellEntry const* learnedInfo = sSpellStore.LookupEntry(learned_0);
        if (!learnedInfo)
        {
            return;
        }

        // specialization
        SpellEffectEntry const* learnedSpellEffect0 = learnedInfo->GetSpellEffect(EFFECT_INDEX_0);
        SpellEffectEntry const* learnedSpellEffect1 = learnedInfo->GetSpellEffect(EFFECT_INDEX_1);
        if (learnedSpellEffect0 && learnedSpellEffect0->Effect == SPELL_EFFECT_TRADE_SKILL && learnedSpellEffect1 && learnedSpellEffect1->Effect == 0)
        {
            // search other specialization for same prof
            for (PlayerSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
            {
                if (itr->second.state == PLAYERSPELL_REMOVED || itr->first == learned_0)
                {
                    continue;
                }

                SpellEntry const* itrInfo = sSpellStore.LookupEntry(itr->first);
                if (!itrInfo)
                {
                    return;
                }

                // compare only specializations
                SpellEffectEntry const* itrSpellEffect0 = learnedInfo->GetSpellEffect(EFFECT_INDEX_0);
                SpellEffectEntry const* itrSpellEffect1 = learnedInfo->GetSpellEffect(EFFECT_INDEX_1);
                if ((itrSpellEffect0 && itrSpellEffect0->Effect != SPELL_EFFECT_TRADE_SKILL) || (itrSpellEffect1 && itrSpellEffect1->Effect != 0))
                {
                    continue;
                }

                // compare same chain spells
                if (sSpellMgr.GetFirstSpellInChain(itr->first) != first_spell)
                {
                    continue;
                }

                // now we have 2 specialization, learn possible only if found is lesser specialization rank
                if (!sSpellMgr.IsHighRankOfSpell(learned_0, itr->first))
                {
                    return;
                }
            }
        }
    }

    CastSpell(this, spell_id, true);
}

/**
 * @brief Reapplies all quest reward spells from previously rewarded quests.
 */
void Player::learnQuestRewardedSpells()
{
    // learn spells received from quest completing
    for (QuestStatusMap::const_iterator itr = mQuestStatus.begin(); itr != mQuestStatus.end(); ++itr)
    {
        // skip no rewarded quests
        if (!itr->second.m_rewarded)
        {
            continue;
        }

        Quest const* quest = sObjectMgr.GetQuestTemplate(itr->first);
        if (!quest)
        {
            continue;
        }

        learnQuestRewardedSpells(quest);
    }
}

/**
 * @brief Learns or removes spells unlocked by a profession or skill value.
 *
 * @param skill_id The skill line identifier.
 * @param skill_value The current skill value.
 */
void Player::learnSkillRewardedSpells(uint32 skill_id, uint32 skill_value)
{
    uint32 raceMask  = getRaceMask();
    uint32 classMask = getClassMask();
    for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
    {
        SkillLineAbilityEntry const* pAbility = sSkillLineAbilityStore.LookupEntry(j);
        if (!pAbility || pAbility->skillId != skill_id || pAbility->learnOnGetSkill != ABILITY_LEARNED_ON_GET_PROFESSION_SKILL)
        {
            continue;
        }
        // Check race if set
        if (pAbility->racemask && !(pAbility->racemask & raceMask))
        {
            continue;
        }
        // Check class if set
        if (pAbility->classmask && !(pAbility->classmask & classMask))
        {
            continue;
        }

        if (sSpellStore.LookupEntry(pAbility->spellId))
        {
            // need unlearn spell
            if (skill_value < pAbility->req_skill_value)
            {
                removeSpell(pAbility->spellId);
            }
            // need learn
            else if (!IsInWorld())
            {
                addSpell(pAbility->spellId, true, true, true, false);
            }
            else
            {
                learnSpell(pAbility->spellId, true);
            }
        }
    }
}

/**
 * @brief Sends visible aura duration updates for a target to the player.
 *
 * @param target The unit whose aura durations should be sent.
 */
void Player::SendAurasForTarget(Unit* target)
{
    Unit::VisibleAuraMap const& visibleAuras = target->GetVisibleAuras();
    if (visibleAuras.empty())
    {
        return;
    }

    WorldPacket data(SMSG_AURA_UPDATE_ALL);
    data << target->GetPackGUID();

    for (Unit::VisibleAuraMap::const_iterator itr = visibleAuras.begin(); itr != visibleAuras.end(); ++itr)
    {
        itr->second->BuildUpdatePacket(data);
    }

    GetSession()->SendPacket(&data);
}

void Player::SetDailyQuestStatus(uint32 quest_id)
{
    for (uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
    {
        if (!GetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1 + quest_daily_idx))
        {
            SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1 + quest_daily_idx, quest_id);
            m_DailyQuestChanged = true;
            break;
        }
    }
}

void Player::SetWeeklyQuestStatus(uint32 quest_id)
{
    m_weeklyquests.insert(quest_id);
    m_WeeklyQuestChanged = true;
}

void Player::SetMonthlyQuestStatus(uint32 quest_id)
{
    m_monthlyquests.insert(quest_id);
    m_MonthlyQuestChanged = true;
}

void Player::ResetDailyQuestStatus()
{
    for (uint32 quest_daily_idx = 0; quest_daily_idx < PLAYER_MAX_DAILY_QUESTS; ++quest_daily_idx)
    {
        SetUInt32Value(PLAYER_FIELD_DAILY_QUESTS_1 + quest_daily_idx, 0);
    }

    // DB data deleted in caller
    m_DailyQuestChanged = false;
}

void Player::ResetWeeklyQuestStatus()
{
    if (m_weeklyquests.empty())
    {
        return;
    }

    m_weeklyquests.clear();
    // DB data deleted in caller
    m_WeeklyQuestChanged = false;
}

void Player::ResetMonthlyQuestStatus()
{
    if (m_monthlyquests.empty())
    {
        return;
    }

    m_monthlyquests.clear();
    // DB data deleted in caller
    m_MonthlyQuestChanged = false;
}

/**
 * @brief Gets the battleground instance the player is currently associated with.
 *
 * @return The active battleground instance, or NULL if none exists.
 */
BattleGround* Player::GetBattleGround() const
{
    if (GetBattleGroundId() == 0)
    {
        return NULL;
    }

    return sBattleGroundMgr.GetBattleGround(GetBattleGroundId(), m_bgData.bgTypeID);
}

bool Player::InArena() const
{
    BattleGround* bg = GetBattleGround();
    if (!bg || !bg->isArena())
    {
        return false;
    }

    return true;
}

/**
 * @brief Checks whether the player's level fits a battleground's allowed range.
 *
 * @param bgTypeId The battleground type to evaluate.
 * @return True if the player's level is within range; otherwise, false.
 */
bool Player::GetBGAccessByLevel(BattleGroundTypeId bgTypeId) const
{
    // get a template bg instead of running one
    BattleGround* bg = sBattleGroundMgr.GetBattleGroundTemplate(bgTypeId);
    if (!bg)
    {
        return false;
    }

    // limit check leel to dbc compatible level range
    uint32 level = getLevel();
    if (level > DEFAULT_MAX_LEVEL)
    {
        level = DEFAULT_MAX_LEVEL;
    }

    if (level < bg->GetMinLevel() || level > bg->GetMaxLevel())
    {
        return false;
    }

    return true;
}

/**
 * @brief Calculates the vendor price discount earned from reputation and rank.
 *
 * @param pCreature The vendor creature.
 * @return The price multiplier applied to vendor costs.
 */
float Player::GetReputationPriceDiscount(Creature const* pCreature) const
{
    return GetReputationPriceDiscount(pCreature->getFactionTemplateEntry());
}

float Player::GetReputationPriceDiscount(FactionTemplateEntry const* factionTemplate) const
{
    if (!factionTemplate || !factionTemplate->faction)
    {
        return 1.0f;
    }

    ReputationRank rank = GetReputationRank(factionTemplate->faction);
    if (rank <= REP_NEUTRAL)
    {
        return 1.0f;
    }

    return 1.0f - 0.05f * (rank - REP_NEUTRAL);
}

/*
 * Check spell availability for training base at SkillLineAbility/SkillRaceClassInfo data.
 * Checked allowed race/class and dependent from race/class allowed min level
 *
 * @param spell_id  checked spell id
 * @param pReqlevel if arg provided then function work in view mode (level check not applied but detected minlevel returned to var by arg pointer.
                    if arg not provided then considered train action mode and level checked
 * @return          true if spell available for show in trainer list (with skip level check) or training.
 */
bool Player::IsSpellFitByClassAndRace(uint32 spell_id, uint32* pReqlevel /*= NULL*/) const
{
    uint32 racemask  = getRaceMask();
    uint32 classmask = getClassMask();

    SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spell_id);
    if (bounds.first == bounds.second)
    {
        return true;
    }

    for (SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
    {
        SkillLineAbilityEntry const* abilityEntry = _spell_idx->second;
        // skip wrong race skills
        if (abilityEntry->racemask && (abilityEntry->racemask & racemask) == 0)
        {
            continue;
        }

        // skip wrong class skills
        if (abilityEntry->classmask && (abilityEntry->classmask & classmask) == 0)
        {
            continue;
        }

        SkillRaceClassInfoMapBounds raceBounds = sSpellMgr.GetSkillRaceClassInfoMapBounds(abilityEntry->skillId);
        for (SkillRaceClassInfoMap::const_iterator itr = raceBounds.first; itr != raceBounds.second; ++itr)
        {
            SkillRaceClassInfoEntry const* skillRCEntry = itr->second;
            if ((skillRCEntry->raceMask & racemask) && (skillRCEntry->classMask & classmask))
            {
                if (skillRCEntry->flags & ABILITY_SKILL_NONTRAINABLE)
                {
                    return false;
                }

                if (pReqlevel)                              // show trainers list case
                {
                    if (skillRCEntry->reqLevel)
                    {
                        *pReqlevel = skillRCEntry->reqLevel;
                        return true;
                    }
                }
                else                                        // check availble case at train
                {
                    if (skillRCEntry->reqLevel && getLevel() < skillRCEntry->reqLevel)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    return false;
}

/**
 * @brief Accepts or declines a pending summon and teleports when valid.
 *
 * @param agree True to accept the summon; false to decline it.
 */
void Player::SummonIfPossible(bool agree)
{
    if (!agree)
    {
        m_summon_expire = 0;
        return;
    }

    // expire and auto declined
    if (m_summon_expire < time(NULL))
    {
        return;
    }

    // stop taxi flight at summon
    if (IsTaxiFlying())
    {
        GetMotionMaster()->MovementExpired();
        m_taxi.ClearTaxiDestinations();
    }

    // drop flag at summon
    // this code can be reached only when GM is summoning player who carries flag, because player should be immune to summoning spells when he carries flag
    if (BattleGround* bg = GetBattleGround())
    {
        bg->EventPlayerDroppedFlag(this);
    }

    m_summon_expire = 0;

    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS, 1);

    TeleportTo(m_summon_mapid, m_summon_x, m_summon_y, m_summon_z, GetOrientation());
}

/**
 * @brief Removes an item from the list of items with active duration tracking.
 *
 * @param item The item to stop tracking.
 */
void Player::RemoveItemDurations(Item* item)
{
    for (ItemDurationList::iterator itr = m_itemDuration.begin(); itr != m_itemDuration.end(); ++itr)
    {
        if (*itr == item)
        {
            m_itemDuration.erase(itr);
            break;
        }
    }
}

/**
 * @brief Adds an item to the list of items with active duration tracking.
 *
 * @param item The item to track.
 */
void Player::AddItemDurations(Item* item)
{
    if (item->GetUInt32Value(ITEM_FIELD_DURATION))
    {
        m_itemDuration.push_back(item);
        item->SendTimeUpdate(this);
    }
}

/**
 * @brief Automatically unequips the offhand item when a two-handed weapon requires it.
 */
void Player::AutoUnequipOffhandIfNeed()
{
    Item* offItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
    if (!offItem)
    {
        return;
    }

    // need unequip offhand for 2h-weapon without TitanGrip (in any from hands)
    if ((CanDualWield() || offItem->GetProto()->InventoryType == INVTYPE_SHIELD || offItem->GetProto()->InventoryType == INVTYPE_HOLDABLE) &&
            (CanTitanGrip() || (offItem->GetProto()->InventoryType != INVTYPE_2HWEAPON && !IsTwoHandUsed())))
    {
        return;
    }

    ItemPosCountVec off_dest;
    uint8 off_msg = CanStoreItem(NULL_BAG, NULL_SLOT, off_dest, offItem, false);
    if (off_msg == EQUIP_ERR_OK)
    {
        RemoveItem(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND, true);
        StoreItem(off_dest, offItem, true);
    }
    else
    {
        MoveItemFromInventory(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND, true);
        CharacterDatabase.BeginTransaction();
        offItem->DeleteFromInventoryDB();                   // deletes item from character's inventory
        offItem->SaveToDB();                                // recursive and not have transaction guard into self, item not in inventory and can be save standalone
        CharacterDatabase.CommitTransaction();

        std::string subject = GetSession()->GetMangosString(LANG_NOT_EQUIPPED_ITEM);
        MailDraft(subject, "There's were problems with equipping this item.").AddItem(offItem).SendMailTo(this, MailSender(this, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED);
    }
}

/**
 * @brief Checks whether the player has an equipped item that satisfies a spell requirement.
 *
 * @param spellInfo The spell entry defining the equipment requirement.
 * @param ignoreItem An equipped item to ignore during the search.
 * @return True if a valid item is equipped; otherwise, false.
 */
bool Player::HasItemFitToSpellReqirements(SpellEntry const* spellInfo, Item const* ignoreItem)
{
    int32 itemClass = spellInfo->GetEquippedItemClass();
    if (itemClass < 0)
    {
        return true;
    }

    // scan other equipped items for same requirements (mostly 2 daggers/etc)
    // for optimize check 2 used cases only
    switch(itemClass)
    {
        case ITEM_CLASS_WEAPON:
        {
            for (int i = EQUIPMENT_SLOT_MAINHAND; i < EQUIPMENT_SLOT_TABARD; ++i)
                if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    if (item != ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                    {
                        return true;
                    }
            break;
        }
        case ITEM_CLASS_ARMOR:
        {
            // tabard not have dependent spells
            for (int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_MAINHAND; ++i)
                if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    if (item != ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                    {
                        return true;
                    }

            // shields can be equipped to offhand slot
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                if (item != ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                {
                    return true;
                }

            // ranged slot can have some armor subclasses
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED))
                if (item != ignoreItem && item->IsFitToSpellRequirements(spellInfo))
                {
                    return true;
                }

            break;
        }
        default:
            sLog.outError("HasItemFitToSpellReqirements: Not handled spell requirement for item class %u", itemClass);
            break;
    }

    return false;
}

/**
 * @brief Checks whether the player may cast a spell without consuming reagents.
 *
 * @param spellInfo The spell being cast.
 * @return True if reagents can be ignored; otherwise, false.
 */
bool Player::CanNoReagentCast(SpellEntry const* spellInfo) const
{
    // don't take reagents for spells with SPELL_ATTR_EX5_NO_REAGENT_WHILE_PREP
    if (spellInfo->HasAttribute(SPELL_ATTR_EX5_NO_REAGENT_WHILE_PREP) && HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION))
    {
        return true;
    }

    // Check no reagent use mask
    uint64 noReagentMask_0_1 = GetUInt64Value(PLAYER_NO_REAGENT_COST_1);
    uint32 noReagentMask_2   = GetUInt32Value(PLAYER_NO_REAGENT_COST_1 + 2);
    if (spellInfo->IsFitToFamilyMask(noReagentMask_0_1, noReagentMask_2))
    {
        return true;
    }

    return false;
}

/**
 * @brief Removes auras and interrupts casts that depend on a removed item.
 *
 * @param pItem The item being removed or invalidated.
 */
void Player::RemoveItemDependentAurasAndCasts(Item* pItem)
{
    SpellAuraHolderMap& auras = GetSpellAuraHolderMap();
    for (SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end();)
    {
        SpellAuraHolder* holder = itr->second;

        // skip passive (passive item dependent spells work in another way) and not self applied auras
        SpellEntry const* spellInfo = holder->GetSpellProto();
        if (holder->IsPassive() ||  holder->GetCasterGuid() != GetObjectGuid())
        {
            ++itr;
            continue;
        }

        // skip if not item dependent or have alternative item
        if (HasItemFitToSpellReqirements(spellInfo, pItem))
        {
            ++itr;
            continue;
        }

        // no alt item, remove aura, restart check
        RemoveAurasDueToSpell(holder->GetId());
        itr = auras.begin();
    }

    // currently casted spells can be dependent from item
    for (uint32 i = 0; i < CURRENT_MAX_SPELL; ++i)
        if (Spell* spell = GetCurrentSpell(CurrentSpellTypes(i)))
            if (spell->getState() != SPELL_STATE_DELAYED && !HasItemFitToSpellReqirements(spell->m_spellInfo, pItem))
            {
                InterruptSpell(CurrentSpellTypes(i));
            }
}

/**
 * @brief Chooses the resurrection spell currently available to the player.
 *
 * @return The resurrection spell identifier, or 0 if none is available.
 */
uint32 Player::GetResurrectionSpellId()
{
    // search priceless resurrection possibilities
    uint32 prio = 0;
    uint32 spell_id = 0;
    AuraList const& dummyAuras = GetAurasByType(SPELL_AURA_DUMMY);
    for (AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
    {
        // Soulstone Resurrection                           // prio: 3 (max, non death persistent)
        if (prio < 2 && (*itr)->GetSpellProto()->SpellVisual[0] == 99 && (*itr)->GetSpellProto()->SpellIconID == 92)
        {
            switch ((*itr)->GetId())
            {
                case 20707: spell_id =  3026; break;        // rank 1
                case 20762: spell_id = 20758; break;        // rank 2
                case 20763: spell_id = 20759; break;        // rank 3
                case 20764: spell_id = 20760; break;        // rank 4
                case 20765: spell_id = 20761; break;        // rank 5
                case 27239: spell_id = 27240; break;        // rank 6
                case 47883: spell_id = 47882; break;        // rank 7
                default:
                    sLog.outError("Unhandled spell %u: S.Resurrection", (*itr)->GetId());
                    continue;
            }

            prio = 3;
        }
        // Twisting Nether                                  // prio: 2 (max)
        else if ((*itr)->GetId() == 23701 && roll_chance_i(10))
        {
            prio = 2;
            spell_id = 23700;
        }
    }

    // Reincarnation (passive spell)                        // prio: 1
    // Glyph of Renewed Life remove reagent requiremnnt
    if (prio < 1 && HasSpell(20608) && !HasSpellCooldown(21169) && (HasItemCount(17030, 1) || HasAura(58059, EFFECT_INDEX_0)))
    {
        spell_id = 21169;
    }

    return spell_id;
}

// Used in triggers for check "Only to targets that grant experience or honor" req
bool Player::isHonorOrXPTarget(Unit* pVictim) const
{
    uint32 v_level = pVictim->getLevel();
    uint32 k_grey  = MaNGOS::XP::GetGrayLevel(getLevel());

    // Victim level less gray level
    if (v_level <= k_grey)
    {
        return false;
    }

    if (pVictim->GetTypeId() == TYPEID_UNIT)
    {
        Creature* pVictimAsCreature = reinterpret_cast<Creature*>(pVictim);
        if (pVictimAsCreature->IsTotem() ||
            pVictimAsCreature->IsPet() ||
            pVictimAsCreature->GetCreatureInfo()->ExtraFlags & CREATURE_FLAG_EXTRA_NO_XP_AT_KILL)
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief Rewards the player for killing a unit outside group reward distribution.
 *
 * @param pVictim The killed unit.
 */
void Player::RewardSinglePlayerAtKill(Unit* pVictim)
{
    bool PvP = pVictim->isCharmedOwnedByPlayerOrPlayer();
    uint32 xp = PvP ? 0 : MaNGOS::XP::Gain(this, pVictim);

    // honor can be in PvP and !PvP (racial leader) cases
    RewardHonor(pVictim, 1);

    // xp and reputation only in !PvP case
    if (!PvP)
    {
        RewardReputation(pVictim, 1);
        GiveXP(xp, pVictim);

        if (Pet* pet = GetPet())
        {
            pet->GivePetXP(xp);
        }

        // normal creature (not pet/etc) can be only in !PvP case
        if (pVictim->GetTypeId() == TYPEID_UNIT)
            if (CreatureInfo const* normalInfo = ObjectMgr::GetCreatureTemplate(pVictim->GetEntry()))
            {
                KilledMonster(normalInfo, pVictim->GetObjectGuid());
            }
    }
}

/**
 * @brief Grants event kill credit to the player or nearby group members.
 *
 * @param creature_id The credited creature entry identifier.
 * @param pRewardSource The world object used for distance checks.
 */
void Player::RewardPlayerAndGroupAtEvent(uint32 creature_id, WorldObject* pRewardSource)
{
    MANGOS_ASSERT((!GetGroup() || pRewardSource) && "Player::RewardPlayerAndGroupAtEvent called for Group-Case but no source for range searching provided");

    ObjectGuid creature_guid = pRewardSource && pRewardSource->GetTypeId() == TYPEID_UNIT ? pRewardSource->GetObjectGuid() : ObjectGuid();

    // prepare data for near group iteration
    if (Group* pGroup = GetGroup())
    {
        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupGuy = itr->getSource();
            if (!pGroupGuy)
            {
                continue;
            }

            if (!pGroupGuy->IsAtGroupRewardDistance(pRewardSource))
            {
                continue;                                    // member (alive or dead) or his corpse at req. distance
            }

            // quest objectives updated only for alive group member or dead but with not released body
            if (pGroupGuy->IsAlive() || !pGroupGuy->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
            {
                pGroupGuy->KilledMonsterCredit(creature_id, creature_guid);
            }
        }
    }
    else                                                    // if (!pGroup)
    {
        KilledMonsterCredit(creature_id, creature_guid);
    }
}

/**
 * @brief Grants quest cast credit to the player or nearby group members.
 *
 * @param pRewardSource The credited creature or gameobject.
 * @param spellid The spell that granted the credit.
 */
void Player::RewardPlayerAndGroupAtCast(WorldObject* pRewardSource, uint32 spellid)
{
    // prepare data for near group iteration
    if (Group* pGroup = GetGroup())
    {
        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* pGroupGuy = itr->getSource();
            if (!pGroupGuy)
            {
                continue;
            }

            if (!pGroupGuy->IsAtGroupRewardDistance(pRewardSource))
            {
                continue;                                // member (alive or dead) or his corpse at req. distance
            }

            // quest objectives updated only for alive group member or dead but with not released body
            if (pGroupGuy->IsAlive() || !pGroupGuy->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
            {
                pGroupGuy->CastedCreatureOrGO(pRewardSource->GetEntry(), pRewardSource->GetObjectGuid(), spellid, pGroupGuy == this);
            }
        }
    }
    else                                                    // if (!pGroup)
    {
        CastedCreatureOrGO(pRewardSource->GetEntry(), pRewardSource->GetObjectGuid(), spellid);
    }
}

/**
 * @brief Checks whether the player or corpse is close enough for shared rewards.
 *
 * @param pRewardSource The source object used for the distance check.
 * @return True if the player qualifies for group reward range; otherwise, false.
 */
bool Player::IsAtGroupRewardDistance(WorldObject const* pRewardSource) const
{
    if (pRewardSource->IsWithinDistInMap(this, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE)))
    {
        return true;
    }

    if (IsAlive())
    {
        return false;
    }

    Corpse* corpse = GetCorpse();
    if (!corpse)
    {
        return false;
    }

    return pRewardSource->IsWithinDistInMap(corpse, sWorld.getConfig(CONFIG_FLOAT_GROUP_XP_DISTANCE));
}

/**
 * @brief Resurrects the player using the pending resurrection request data.
 */
void Player::ResurectUsingRequestData()
{
    /// Teleport before resurrecting by player, otherwise the player might get attacked from creatures near his corpse
    if (m_resurrectGuid.IsPlayer())
    {
        TeleportTo(m_resurrectMap, m_resurrectX, m_resurrectY, m_resurrectZ, GetOrientation());
    }

    // we cannot resurrect player when we triggered far teleport
    // player will be resurrected upon teleportation
    if (IsBeingTeleportedFar())
    {
        ScheduleDelayedOperation(DELAYED_RESURRECT_PLAYER);
        return;
    }

    ResurrectPlayer(0.0f, false);

    if (GetMaxHealth() > m_resurrectHealth)
    {
        SetHealth(m_resurrectHealth);
    }
    else
    {
        SetHealth(GetMaxHealth());
    }

    if (GetMaxPower(POWER_MANA) > m_resurrectMana)
    {
        SetPower(POWER_MANA, m_resurrectMana);
    }
    else
    {
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }

    SetPower(POWER_RAGE, 0);

    SetPower(POWER_ENERGY, GetMaxPower(POWER_ENERGY));

    SpawnCorpseBones();
}

/**
 * @brief Sends a client-control state update for a unit.
 *
 * @param target The unit whose control state is being updated.
 * @param allowMove Nonzero to allow movement; zero to disable it.
 */
void Player::SetClientControl(Unit* target, uint8 allowMove)
{
    WorldPacket data(SMSG_CLIENT_CONTROL_UPDATE, target->GetPackGUID().size() + 1);
    data << target->GetPackGUID();
    data << uint8(allowMove);
    GetSession()->SendPacket(&data);
}

void Player::Uncharm()
{
    if (Unit* charm = GetCharm())
    {
        charm->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM);
        charm->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS);
        charm->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS_PET);
        if (charm == GetMover())
        {
            SetMover(NULL);
            GetCamera().ResetView();
            RemoveSpellsCausingAura(SPELL_AURA_MOD_INVISIBILITY);
            SetCharm(NULL);
            SetClientControl(this, 1);
        }
    }
}

/**
 * @brief Applies or removes auras that depend on the player's current zone.
 */
void Player::UpdateZoneDependentAuras()
{
    // Some spells applied at enter into zone (with subzones), aura removed in UpdateAreaDependentAuras that called always at zone->area update
    SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAreaMapBounds(m_zoneUpdateId);
    for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
    {
        itr->second->ApplyOrRemoveSpellIfCan(this, m_zoneUpdateId, 0, true);
    }
}

/**
 * @brief Applies or removes auras that depend on the player's current subzone.
 */
void Player::UpdateAreaDependentAuras()
{
    // remove auras from spells with area limitations
    for (SpellAuraHolderMap::iterator iter = m_spellAuraHolders.begin(); iter != m_spellAuraHolders.end();)
    {
        // use m_zoneUpdateId for speed: UpdateArea called from UpdateZone or instead UpdateZone in both cases m_zoneUpdateId up-to-date
        if (sSpellMgr.GetSpellAllowedInLocationError(iter->second->GetSpellProto(), GetMapId(), m_zoneUpdateId, m_areaUpdateId, this) != SPELL_CAST_OK)
        {
            RemoveSpellAuraHolder(iter->second);
            iter = m_spellAuraHolders.begin();
        }
        else
        {
            ++iter;
        }
    }

    // some auras applied at subzone enter
    SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAreaMapBounds(m_areaUpdateId);
    for (SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
    {
        itr->second->ApplyOrRemoveSpellIfCan(this, m_zoneUpdateId, m_areaUpdateId, true);
    }
}

struct UpdateZoneDependentPetsHelper
{
    explicit UpdateZoneDependentPetsHelper(Player* _owner, uint32 zone, uint32 area) : owner(_owner), zone_id(zone), area_id(area) {}
    void operator()(Unit* unit) const
    {
        if (unit->GetTypeId() == TYPEID_UNIT && ((Creature*)unit)->IsPet() && !((Pet*)unit)->isControlled())
            if (uint32 spell_id = unit->GetUInt32Value(UNIT_CREATED_BY_SPELL))
                if (SpellEntry const* spellEntry = sSpellStore.LookupEntry(spell_id))
                    if (sSpellMgr.GetSpellAllowedInLocationError(spellEntry, owner->GetMapId(), zone_id, area_id, owner) != SPELL_CAST_OK)
                    {
                        ((Pet*)unit)->Unsummon(PET_SAVE_AS_DELETED, owner);
                    }
    }
    Player* owner;
    uint32 zone_id;
    uint32 area_id;
};

void Player::UpdateZoneDependentPets()
{
    // check pet (permanent pets ignored), minipet, guardians (including protector)
    CallForAllControlledUnits(UpdateZoneDependentPetsHelper(this, m_zoneUpdateId, m_areaUpdateId), CONTROLLED_PET | CONTROLLED_GUARDIANS | CONTROLLED_MINIPET);
}

/**
 * @brief Gets the current corpse reclaim delay for PvE or PvP death.
 *
 * @param pvp True for PvP death rules; false for PvE death rules.
 * @return The reclaim delay in seconds.
 */
uint32 Player::GetCorpseReclaimDelay(bool pvp) const
{
    if ((pvp && !sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVP)) ||
            (!pvp && !sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVE)))
    {
        return corpseReclaimDelay[0];
    }

    time_t now = time(NULL);
    // 0..2 full period
    uint32 count = (now < m_deathExpireTime) ? uint32((m_deathExpireTime - now) / DEATH_EXPIRE_STEP) : 0;
    return corpseReclaimDelay[count];
}

/**
 * @brief Advances the corpse reclaim delay escalation after death.
 */
void Player::UpdateCorpseReclaimDelay()
{
    bool pvp = m_ExtraFlags & PLAYER_EXTRA_PVP_DEATH;

    if ((pvp && !sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVP)) ||
            (!pvp && !sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVE)))
    {
        return;
    }

    time_t now = time(NULL);
    if (now < m_deathExpireTime)
    {
        // full and partly periods 1..3
        uint32 count = uint32((m_deathExpireTime - now) / DEATH_EXPIRE_STEP + 1);
        if (count < MAX_DEATH_COUNT)
        {
            m_deathExpireTime = now + (count + 1) * DEATH_EXPIRE_STEP;
        }
        else
        {
            m_deathExpireTime = now + MAX_DEATH_COUNT * DEATH_EXPIRE_STEP;
        }
    }
    else
    {
        m_deathExpireTime = now + DEATH_EXPIRE_STEP;
    }
}

/**
 * @brief Sends the current corpse reclaim delay to the client.
 *
 * @param load True when restoring the delay from saved corpse state.
 */
void Player::SendCorpseReclaimDelay(bool load)
{
    Corpse* corpse = GetCorpse();
    if (!corpse)
    {
        return;
    }

    uint32 delay;
    if (load)
    {
        if (corpse->GetGhostTime() > m_deathExpireTime)
        {
            return;
        }

        bool pvp = corpse->GetType() == CORPSE_RESURRECTABLE_PVP;

        uint32 count;
        if ((pvp && sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVP)) ||
                (!pvp && sWorld.getConfig(CONFIG_BOOL_DEATH_CORPSE_RECLAIM_DELAY_PVE)))
        {
            count = uint32(m_deathExpireTime - corpse->GetGhostTime()) / DEATH_EXPIRE_STEP;
            if (count >= MAX_DEATH_COUNT)
            {
                count = MAX_DEATH_COUNT - 1;
            }
        }
        else
        {
            count = 0;
        }

        time_t expected_time = corpse->GetGhostTime() + corpseReclaimDelay[count];

        time_t now = time(NULL);
        if (now >= expected_time)
        {
            return;
        }

        delay = uint32(expected_time - now);
    }
    else
    {
        delay = GetCorpseReclaimDelay(corpse->GetType() == CORPSE_RESURRECTABLE_PVP);
    }

    //! corpse reclaim delay 30 * 1000ms or longer at often deaths
    WorldPacket data(SMSG_CORPSE_RECLAIM_DELAY, 4);
    data << uint32(delay * IN_MILLISECONDS);
    GetSession()->SendPacket(&data);
}

uint32 Player::GetNextResetTalentsCost()    const
{
    // The first time reset costs 1 gold
    if (GetTalentResetCost() < 1 * GOLD)
    {
        return 1 * GOLD;
    }
    // then 5 gold
    else if (GetTalentResetCost() < 5 * GOLD)
    {
        return 5 * GOLD;
    }
    // After that it increases in increments of 5 gold
    else if (GetTalentResetCost() < 10 * GOLD)
    {
        return 10 * GOLD;
    }
    else
    {
        uint64 months = (sWorld.GetGameTime() - GetTalentResetTime()) / MONTH;
        if (months > 0)
        {
            // This cost will be reduced by a rate of 5 gold per month
            int32 new_cost = int32(GetTalentResetCost() - 5 * GOLD*months);
            // to a minimum of 10 gold.
            return (new_cost < 10 * GOLD ? 10 * GOLD : new_cost);
        }
        else
        {
            // After that it increases in increments of 5 gold
            int32 new_cost = GetTalentResetCost() + 5 * GOLD;
            // until it hits a cap of 50 gold.
            if (new_cost > 50 * GOLD)
            {
                new_cost = 50 * GOLD;
            }
            return new_cost;
        }
    }
}

/**
 * @brief Selects a random nearby raid member within a radius.
 *
 * @param radius The maximum search radius.
 * @return A random eligible raid member, or NULL if none are found.
 */
Player* Player::GetNextRandomRaidMember(float radius)
{
    Group* pGroup = GetGroup();
    if (!pGroup)
    {
        return NULL;
    }

    std::vector<Player*> nearMembers;
    nearMembers.reserve(pGroup->GetMembersCount());

    for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* Target = itr->getSource();

        // IsHostileTo check duel and controlled by enemy
        if (Target && Target != this && IsWithinDistInMap(Target, radius) &&
                !Target->HasInvisibilityAura() && !IsHostileTo(Target))
                {
                    nearMembers.push_back(Target);
                }
    }

    if (nearMembers.empty())
    {
        return NULL;
    }

    uint32 randTarget = urand(0, nearMembers.size() - 1);
    return nearMembers[randTarget];
}

/**
 * @brief Checks whether the player is allowed to uninvite someone from the group.
 *
 * @return The party result code describing whether uninvite is allowed.
 */
PartyResult Player::CanUninviteFromGroup() const
{
    const Group* grp = GetGroup();
    if (!grp)
    {
        return ERR_NOT_IN_GROUP;
    }

    if (!grp->IsLeader(GetObjectGuid()) && !grp->IsAssistant(GetObjectGuid()))
    {
        return ERR_NOT_LEADER;
    }

    if (InBattleGround())
    {
        return ERR_INVITE_RESTRICTED;
    }

    return ERR_PARTY_RESULT_OK;
}

void Player::UpdateGroupLeaderFlag(const bool remove /*= false*/)
{
    const Group* group = GetGroup();
    if (HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GROUP_LEADER))
    {
        if (remove || !group || group->GetLeaderGuid() != GetObjectGuid())
        {
            RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GROUP_LEADER);
        }
    }
    else if (!remove && group && group->GetLeaderGuid() == GetObjectGuid())
    {
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GROUP_LEADER);
    }
}

/**
 * @brief Moves the player into a battleground raid group.
 *
 * @param group The battleground raid group.
 * @param subgroup The battleground subgroup index.
 */
void Player::SetBattleGroundRaid(Group* group, int8 subgroup)
{
    // we must move references from m_group to m_originalGroup
    SetOriginalGroup(GetGroup(), GetSubGroup());

    m_group.unlink();
    m_group.link(group, this);
    m_group.setSubGroup((uint8)subgroup);
}

/**
 * @brief Restores the player's original group after leaving a battleground raid.
 */
void Player::RemoveFromBattleGroundRaid()
{
    // remove existing reference
    m_group.unlink();
    if (Group* group = GetOriginalGroup())
    {
        m_group.link(group, this);
        m_group.setSubGroup(GetOriginalSubGroup());
    }
    SetOriginalGroup(NULL);
}

/**
 * @brief Stores the player's original non-battleground group reference.
 *
 * @param group The original group, or NULL to clear it.
 * @param subgroup The original subgroup index.
 */
void Player::SetOriginalGroup(Group* group, int8 subgroup)
{
    if (group == NULL)
    {
        m_originalGroup.unlink();
    }
    else
    {
        // never use SetOriginalGroup without a subgroup unless you specify NULL for group
        MANGOS_ASSERT(subgroup >= 0);
        m_originalGroup.link(group, this);
        m_originalGroup.setSubGroup((uint8)subgroup);
    }
}

/**
 * @brief Updates liquid auras and mirror timers based on the player's position.
 *
 * @param m The current map.
 * @param x The X coordinate.
 * @param y The Y coordinate.
 * @param z The Z coordinate.
 */
void Player::UpdateUnderwaterState(Map* m, float x, float y, float z)
{
    GridMapLiquidData liquid_status;
    GridMapLiquidStatus res = m->GetTerrain()->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &liquid_status);
    if (!res)
    {
        m_MirrorTimerFlags &= ~(UNDERWATER_INWATER | UNDERWATER_INLAVA | UNDERWATER_INSLIME | UNDERWATER_INDARKWATER);
        if (m_lastLiquid && m_lastLiquid->SpellId)
        {
            RemoveAurasDueToSpell(m_lastLiquid->SpellId == 37025 ? 37284 : m_lastLiquid->SpellId);
        }
        m_lastLiquid = NULL;
        return;
    }

    if (uint32 liqEntry = liquid_status.entry)
    {
        LiquidTypeEntry const* liquid = sLiquidTypeStore.LookupEntry(liqEntry);
        if (m_lastLiquid && m_lastLiquid->SpellId && m_lastLiquid->Id != liqEntry)
        {
            RemoveAurasDueToSpell(m_lastLiquid->SpellId);
        }

        if (liquid && liquid->SpellId)
        {
            // Exception for SSC water
            uint32 liquidSpellId = liquid->SpellId == 37025 ? 37284 : liquid->SpellId;

            if (res & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER))
            {
                if (!HasAura(liquidSpellId))
                {
                    // Handle exception for SSC water
                    if (liquid->SpellId == 37025)
                    {
                        if (InstanceData* pInst = GetInstanceData())
                        {
                            if (pInst->CheckConditionCriteriaMeet(this, INSTANCE_CONDITION_ID_LURKER, NULL, CONDITION_FROM_HARDCODED))
                            {
                                if (pInst->CheckConditionCriteriaMeet(this, INSTANCE_CONDITION_ID_SCALDING_WATER, NULL, CONDITION_FROM_HARDCODED))
                                {
                                    CastSpell(this, liquidSpellId, true);
                                }
                                else
                                {
                                    SummonCreature(21508, 0, 0, 0, 0, TEMPSPAWN_TIMED_OOC_DESPAWN, 2000);
                                    // Special update timer for the SSC water
                                    m_positionStatusUpdateTimer = 2000;
                                }
                            }
                        }
                    }
                    else
                    {
                        CastSpell(this, liquidSpellId, true);
                    }
                }
            }
            else
            {
                RemoveAurasDueToSpell(liquidSpellId);
            }
        }

        m_lastLiquid = liquid;
    }
    else if (m_lastLiquid && m_lastLiquid->SpellId)
    {
        RemoveAurasDueToSpell(m_lastLiquid->SpellId == 37025 ? 37284 : m_lastLiquid->SpellId);
        m_lastLiquid = NULL;
    }

    // All liquids type - check under water position
    if (liquid_status.CreatureTypeFlags & (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_SLIME))
    {
        if (res & LIQUID_MAP_UNDER_WATER)
        {
            m_MirrorTimerFlags |= UNDERWATER_INWATER;
        }
        else
        {
            m_MirrorTimerFlags &= ~UNDERWATER_INWATER;
        }
    }

    // Allow travel in dark water on taxi or transport
    if ((liquid_status.CreatureTypeFlags & MAP_LIQUID_TYPE_DARK_WATER) && !IsTaxiFlying() && !GetTransport())
    {
        m_MirrorTimerFlags |= UNDERWATER_INDARKWATER;
    }
    else
    {
        m_MirrorTimerFlags &= ~UNDERWATER_INDARKWATER;
    }

    // in lava check, anywhere in lava level
    if (liquid_status.CreatureTypeFlags & MAP_LIQUID_TYPE_MAGMA)
    {
        if (res & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER | LIQUID_MAP_WATER_WALK))
        {
            m_MirrorTimerFlags |= UNDERWATER_INLAVA;
        }
        else
        {
            m_MirrorTimerFlags &= ~UNDERWATER_INLAVA;
        }
    }
    // in slime check, anywhere in slime level
    if (liquid_status.CreatureTypeFlags & MAP_LIQUID_TYPE_SLIME)
    {
        if (res & (LIQUID_MAP_UNDER_WATER | LIQUID_MAP_IN_WATER | LIQUID_MAP_WATER_WALK))
        {
            m_MirrorTimerFlags |= UNDERWATER_INSLIME;
        }
        else
        {
            m_MirrorTimerFlags &= ~UNDERWATER_INSLIME;
        }
    }
}

/**
 * @brief Enables or disables the player's ability to parry.
 *
 * @param value True to allow parry; false to disable it.
 */
void Player::SetCanParry(bool value)
{
    if (m_canParry == value)
    {
        return;
    }

    m_canParry = value;
    UpdateParryPercentage();
}

/**
 * @brief Enables or disables the player's ability to block.
 *
 * @param value True to allow block; false to disable it.
 */
void Player::SetCanBlock(bool value)
{
    if (m_canBlock == value)
    {
        return;
    }

    m_canBlock = value;
    UpdateBlockPercentage();
    UpdateShieldBlockDamageValue();
}

/**
 * @brief Checks whether this item position entry exists in a vector of positions.
 *
 * @param vec The vector of item positions to search.
 * @return True if an entry with the same position exists; otherwise, false.
 */
bool ItemPosCount::isContainedIn(ItemPosCountVec const& vec) const
{
    for (ItemPosCountVec::const_iterator itr = vec.begin(); itr != vec.end(); ++itr)
        if (itr->pos == pos)
        {
            return true;
        }

    return false;
}

/**
 * @brief Checks whether the player can interact with a battleground object.
 *
 * @return True if the player can use the object; otherwise, false.
 */
bool Player::CanUseBattleGroundObject()
{
    // TODO : some spells gives player ForceReaction to one faction (ReputationMgr::ApplyForceReaction)
    // maybe gameobject code should handle that ForceReaction usage
    // BUG: sometimes when player clicks on flag in AB - client won't send gameobject_use, only gameobject_report_use packet
    return (IsAlive() &&                                    // living
            // the following two are incorrect, because invisible/stealthed players should get visible when they click on flag
            !HasStealthAura() &&                            // not stealthed
            !HasInvisibilityAura() &&                       // visible
            !isTotalImmune() &&                             // vulnerable (not immune)
            !HasAura(SPELL_RECENTLY_DROPPED_FLAG, EFFECT_INDEX_0));
}

uint32 Player::GetBarberShopCost(uint8 newhairstyle, uint8 newhaircolor, uint8 newfacialhair, uint32 newskintone)
{
    uint32 level = getLevel();

    if (level > GT_MAX_LEVEL)
    {
        level = GT_MAX_LEVEL;                               // max level in this dbc
    }

    uint8 hairstyle = GetByteValue(PLAYER_BYTES, 2);
    uint8 haircolor = GetByteValue(PLAYER_BYTES, 3);
    uint8 facialhair = GetByteValue(PLAYER_BYTES_2, 0);
    uint8 skintone = GetByteValue(PLAYER_BYTES, 0);

    if (hairstyle == newhairstyle && haircolor == newhaircolor && facialhair == newfacialhair &&
            (skintone == newskintone || newskintone == -1))
        return 0;

    GtBarberShopCostBaseEntry const* bsc = sGtBarberShopCostBaseStore.LookupEntry(level - 1);

    if (!bsc)                                               // shouldn't happen
    {
        return 0xFFFFFFFF;
    }

    float cost = 0;

    if (hairstyle != newhairstyle)
    {
        cost += bsc->cost;                                  // full price
    }

    if (haircolor != newhaircolor && hairstyle == newhairstyle)
    {
        cost += bsc->cost * 0.5f;                           // +1/2 of price
    }

    if (facialhair != newfacialhair)
    {
        cost += bsc->cost * 0.75f;                          // +3/4 of price
    }

    if (skintone != newskintone && newskintone != -1)
    {
        cost += bsc->cost * 0.5f;                           // +1/2 of price
    }

    return uint32(cost);
}

// Player::InitGlyphsForLevel, ApplyGlyph, ApplyGlyphs moved to GlyphMgr (2026-05-12);
// thin delegating wrappers live inline in Player.h.

/**
 * @brief Checks whether the player is immune to all spell schools.
 *
 * @return True if total immunity is active; otherwise, false.
 */
bool Player::isTotalImmune()
{
    AuraList const& immune = GetAurasByType(SPELL_AURA_SCHOOL_IMMUNITY);

    uint32 immuneMask = 0;
    for (AuraList::const_iterator itr = immune.begin(); itr != immune.end(); ++itr)
    {
        immuneMask |= (*itr)->GetModifier()->m_miscvalue;
        if (immuneMask & SPELL_SCHOOL_MASK_ALL)             // total immunity
        {
            return true;
        }
    }
    return false;
}

bool Player::HasTitle(uint32 bitIndex) const
{
    if (bitIndex > MAX_TITLE_INDEX)
    {
        return false;
    }

    uint32 fieldIndexOffset = bitIndex / 32;
    uint32 flag = 1 << (bitIndex % 32);
    return HasFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag);
}

void Player::SetTitle(CharTitlesEntry const* title, bool lost)
{
    uint32 fieldIndexOffset = title->bit_index / 32;
    uint32 flag = 1 << (title->bit_index % 32);

    if (lost)
    {
        if (!HasFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag))
        {
            return;
        }

        RemoveFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag);
    }
    else
    {
        if (HasFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag))
        {
            return;
        }

        SetFlag(PLAYER__FIELD_KNOWN_TITLES + fieldIndexOffset, flag);
    }

    WorldPacket data(SMSG_TITLE_EARNED, 4 + 4);
    data << uint32(title->bit_index);
    data << uint32(lost ? 0 : 1);                           // 1 - earned, 0 - lost
    GetSession()->SendPacket(&data);
}

void Player::UpdateRuneRegen(RuneType rune)
{
    if (rune >= RUNE_DEATH)
    {
        return;
    }

    RuneType actualRune = rune;
    float cooldown = RUNE_BASE_COOLDOWN;
    for (uint8 i = 0; i < MAX_RUNES; i += 2)
    {
        if (GetBaseRune(i) != rune)
        {
            continue;
        }

        uint32 cd = GetRuneCooldown(i);
        uint32 secondRuneCd = GetRuneCooldown(i + 1);
        if (!cd && !secondRuneCd)
        {
            actualRune = GetCurrentRune(i);
        }
        else if (secondRuneCd && (cd > secondRuneCd || !cd))
        {
            cooldown = GetBaseRuneCooldown(i + 1);
            actualRune = GetCurrentRune(i + 1);
        }
        else
        {
            cooldown = GetBaseRuneCooldown(i);
            actualRune = GetCurrentRune(i);
        }

        break;
    }

    float auraMod = 1.0f;
    Unit::AuraList const& regenAuras = GetAurasByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
    for (Unit::AuraList::const_iterator i = regenAuras.begin(); i != regenAuras.end(); ++i)
        if ((*i)->GetMiscValue() == POWER_RUNE && (*i)->GetSpellEffect()->EffectMiscValueB == rune)
        {
            auraMod *= (100.0f + (*i)->GetModifier()->m_amount) / 100.0f;
        }

    // Unholy Presence
    if (Aura* aura = GetAura(48265, EFFECT_INDEX_0))
    {
        auraMod *= (100.0f + aura->GetModifier()->m_amount) / 100.0f;
    }

    // Runic Corruption
    if (Aura* aura = GetAura(51460, EFFECT_INDEX_0))
    {
        auraMod *= (100.0f + aura->GetModifier()->m_amount) / 100.0f;
    }

    float hastePct = (100.0f - GetRatingBonusValue(CR_HASTE_MELEE)) / 100.0f;
    if (hastePct < 0)
    {
        hastePct = 1.0f;
    }

    cooldown *= hastePct / auraMod;

    float value = float(1 * IN_MILLISECONDS) / cooldown;
    SetFloatValue(PLAYER_RUNE_REGEN_1 + uint8(actualRune), value);
}

void Player::UpdateRuneRegen()
{
    for (uint8 i = 0; i < NUM_RUNE_TYPES; ++i)
    {
        UpdateRuneRegen(RuneType(i));
    }
}

uint8 Player::GetRuneCooldownFraction(uint8 index) const
{
    uint16 baseCd = GetBaseRuneCooldown(index);
    if (!baseCd || !GetRuneCooldown(index))
    {
        return 255;
    }
    else if (baseCd == GetRuneCooldown(index))
    {
        return 0;
    }

    return uint8(float(baseCd - GetRuneCooldown(index)) / baseCd * 255);
}

void Player::AddRuneByAuraEffect(uint8 index, RuneType newType, Aura const* aura)
{
    // Item - Death Knight T11 DPS 4P Bonus
    if (newType == RUNE_DEATH && HasAura(90459))
    {
        CastSpell(this, 90507, true);   // Death Eater
    }

    SetRuneConvertAura(index, aura); ConvertRune(index, newType);
}

void Player::RemoveRunesByAuraEffect(Aura const* aura)
{
    for (uint8 i = 0; i < MAX_RUNES; ++i)
    {
        if (m_runes->runes[i].ConvertAura == aura)
        {
            ConvertRune(i, GetBaseRune(i));
            SetRuneConvertAura(i, NULL);
        }
    }
}

void Player::RestoreBaseRune(uint8 index)
{
    Aura const* aura = m_runes->runes[index].ConvertAura;
    // If rune was converted by a non-pasive aura that still active we should keep it converted
    if (aura && !IsPassiveSpell(aura->GetSpellProto()))
    {
        return;
    }

    // Blood of the North
    if (aura->GetId() == 54637 && HasAura(54637))
    {
        return;
    }

    ConvertRune(index, GetBaseRune(index));
    SetRuneConvertAura(index, NULL);
    // Don't drop passive talents providing rune convertion
    if (!aura || aura->GetModifier()->m_auraname != SPELL_AURA_CONVERT_RUNE)
    {
        return;
    }

    for (uint8 i = 0; i < MAX_RUNES; ++i)
        if (aura == m_runes->runes[i].ConvertAura)
        {
            return;
        }

    if (Unit* target = aura->GetTarget())
    {
        target->RemoveSpellAuraHolder(const_cast<Aura*>(aura)->GetHolder());
    }
}

void Player::ConvertRune(uint8 index, RuneType newType)
{
    SetCurrentRune(index, newType);

    WorldPacket data(SMSG_CONVERT_RUNE, 2);
    data << uint8(index);
    data << uint8(newType);
    GetSession()->SendPacket(&data);
}

bool Player::ActivateRunes(RuneType type, uint32 count)
{
    bool modify = false;
    for (uint32 j = 0; count > 0 && j < MAX_RUNES; ++j)
    {
        if (GetRuneCooldown(j) && GetCurrentRune(j) == type)
        {
            SetRuneCooldown(j, 0);
            --count;
            modify = true;
        }
    }

    return modify;
}

void Player::ResyncRunes()
{
    WorldPacket data(SMSG_RESYNC_RUNES, 4 + MAX_RUNES * 2);
    data << uint32(MAX_RUNES);
    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        data << uint8(GetCurrentRune(i));                   // rune type
        data << uint8(GetRuneCooldownFraction(i));
    }
    GetSession()->SendPacket(&data);
}

void Player::AddRunePower(uint8 index)
{
    WorldPacket data(SMSG_ADD_RUNE_POWER, 4);
    data << uint32(1 << index);                             // mask (0x00-0x3F probably)
    GetSession()->SendPacket(&data);
}

void Player::InitRunes()
{
    if (getClass() != CLASS_DEATH_KNIGHT)
    {
        return;
    }

    m_runes = new Runes;

    m_runes->runeState = 0;

    for (uint32 i = 0; i < MAX_RUNES; ++i)
    {
        SetBaseRune(i, runeSlotTypes[i]);                   // init base types
        SetCurrentRune(i, runeSlotTypes[i]);                // init current types
        SetRuneCooldown(i, 0);                              // reset cooldowns
        m_runes->SetRuneState(i);
    }

    for (uint32 i = 0; i < NUM_RUNE_TYPES; ++i)
    {
        SetFloatValue(PLAYER_RUNE_REGEN_1 + i, 0.1f);
    }
}

bool Player::IsBaseRuneSlotsOnCooldown(RuneType runeType) const
{
    for (uint32 i = 0; i < MAX_RUNES; ++i)
        if (GetBaseRune(i) == runeType && GetRuneCooldown(i) == 0)
        {
            return false;
        }

    return true;
}

/**
 * @brief Builds temporary loot data and stores all eligible loot automatically.
 *
 * @param lootTarget The object owning the loot.
 * @param loot_id The loot template identifier.
 * @param store The loot store to use.
 * @param broadcast True to broadcast item gains.
 * @param bag The preferred destination bag.
 * @param slot The preferred destination slot.
 */
void Player::AutoStoreLoot(WorldObject const* lootTarget, uint32 loot_id, LootStore const& store, bool broadcast, uint8 bag, uint8 slot)
{
    Loot loot(lootTarget);
    loot.FillLoot(loot_id, store, this, true);

    AutoStoreLoot(loot, broadcast, bag, slot);
}

/**
 * @brief Stores all eligible loot entries directly into the player's inventory.
 *
 * @param loot The loot container to process.
 * @param broadcast True to broadcast item gains.
 * @param bag The preferred destination bag.
 * @param slot The preferred destination slot.
 */
void Player::AutoStoreLoot(Loot& loot, bool broadcast, uint8 bag, uint8 slot)
{
    uint32 max_slot = loot.GetMaxSlotInLootFor(this);
    for (uint32 i = 0; i < max_slot; ++i)
    {
        LootItem* lootItem = loot.LootItemInSlot(i, this);

        ItemPosCountVec dest;
        InventoryResult msg = CanStoreNewItem(bag, slot, dest, lootItem->itemid, lootItem->count);
        if (msg != EQUIP_ERR_OK && slot != NULL_SLOT)
        {
            msg = CanStoreNewItem(bag, NULL_SLOT, dest, lootItem->itemid, lootItem->count);
        }
        if (msg != EQUIP_ERR_OK && bag != NULL_BAG)
        {
            msg = CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, lootItem->itemid, lootItem->count);
        }
        if (msg != EQUIP_ERR_OK)
        {
            SendEquipError(msg, NULL, NULL, lootItem->itemid);
            continue;
        }

        Item* pItem = StoreNewItem(dest, lootItem->itemid, true, lootItem->randomPropertyId);
        SendNewItem(pItem, lootItem->count, false, false, broadcast);
    }
}

/**
 * @brief Replaces an item with another item while preserving transferable state.
 *
 * @param item The original item.
 * @param newItemId The new item entry identifier.
 * @return The converted item, or NULL if conversion failed.
 */
Item* Player::ConvertItem(Item* item, uint32 newItemId)
{
    uint16 pos = item->GetPos();

    Item* pNewItem = Item::CreateItem(newItemId, 1, this);
    if (!pNewItem)
    {
        return NULL;
    }

    // copy enchantments
    for (uint8 j = PERM_ENCHANTMENT_SLOT; j <= TEMP_ENCHANTMENT_SLOT; ++j)
    {
        if (item->GetEnchantmentId(EnchantmentSlot(j)))
            pNewItem->SetEnchantment(EnchantmentSlot(j), item->GetEnchantmentId(EnchantmentSlot(j)),
                                     item->GetEnchantmentDuration(EnchantmentSlot(j)), item->GetEnchantmentCharges(EnchantmentSlot(j)));
    }

    // copy durability
    if (item->GetUInt32Value(ITEM_FIELD_DURABILITY) < item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY))
    {
        double loosePercent = 1 - item->GetUInt32Value(ITEM_FIELD_DURABILITY) / double(item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY));
        DurabilityLoss(pNewItem, loosePercent);
    }

    if (IsInventoryPos(pos))
    {
        ItemPosCountVec dest;
        InventoryResult msg = CanStoreItem(item->GetBagSlot(), item->GetSlot(), dest, pNewItem, true);
        // ignore cast/combat time restriction
        if (msg == EQUIP_ERR_OK)
        {
            DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            return StoreItem(dest, pNewItem, true);
        }
    }
    else if (IsBankPos(pos))
    {
        ItemPosCountVec dest;
        InventoryResult msg = CanBankItem(item->GetBagSlot(), item->GetSlot(), dest, pNewItem, true);
        // ignore cast/combat time restriction
        if (msg == EQUIP_ERR_OK)
        {
            DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            return BankItem(dest, pNewItem, true);
        }
    }
    else if (IsEquipmentPos(pos))
    {
        uint16 dest;
        InventoryResult msg = CanEquipItem(item->GetSlot(), dest, pNewItem, true, false);
        // ignore cast/combat time restriction
        if (msg == EQUIP_ERR_OK)
        {
            DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            pNewItem = EquipItem(dest, pNewItem, true);
            AutoUnequipOffhandIfNeed();
            return pNewItem;
        }
    }

    // fail
    delete pNewItem;
    return NULL;
}

/**
 * @brief Calculates the total talent points available for the player's level.
 *
 * @return The number of talent points granted by level and rate settings.
 */
uint32 Player::CalculateTalentsPoints() const
{
    // this dbc file has entries only up to level 100
    NumTalentsAtLevelEntry const* count = sNumTalentsAtLevelStore.LookupEntry(std::min<uint32>(getLevel(), 100));
    if (!count)
    {
        return 0;
    }

    float baseForLevel = count->Talents;

    if (getClass() != CLASS_DEATH_KNIGHT)
    {
        return uint32(baseForLevel * sWorld.getConfig(CONFIG_FLOAT_RATE_TALENT));
    }

    // Death Knight starting level
    // hardcoded here - number of quest awarded talents is equal to number of talents any other class would have at level 55
    if (getLevel() < 55)
    {
        return 0;
    }

    NumTalentsAtLevelEntry const* dkBase = sNumTalentsAtLevelStore.LookupEntry(55);
    if (!dkBase)
    {
        return 0;
    }

    float talentPointsForLevel = count->Talents - dkBase->Talents;
    talentPointsForLevel += float(m_questRewardTalentCount);

    if (talentPointsForLevel > baseForLevel)
    {
        talentPointsForLevel = baseForLevel;
    }

    return uint32(talentPointsForLevel * sWorld.getConfig(CONFIG_FLOAT_RATE_TALENT));
}

bool Player::CanStartFlyInArea(uint32 mapid, uint32 zone, uint32 area) const
{
    if (isGameMaster())
    {
        return true;
    }

    // continent checked in SpellMgr::GetSpellAllowedInLocationError at cast and area update
    uint32 v_map = GetVirtualMapForMapAndZone(mapid, zone);

    // switch all known flying maps
    switch (v_map)
    {
        case 0:         // Eastern Kingdoms
        case 1:         // Kalimdor
        case 646:       // Deepholm
            return HasSpell(90267);
        case 530:       // Outland
            return true;
        case 571:       // Northrend
            // Check Cold Weather Flying
            // Disallow mounting in wintergrasp when battle is in progress
            return HasSpell(54197); /* && (!inWintergrasp || wg->GetState() != BF_IN_PROGRESS);*/
    }

    return false;
}

struct DoPlayerLearnSpell
{
    DoPlayerLearnSpell(Player& _player) : player(_player) {}
    void operator()(uint32 spell_id) { player.learnSpell(spell_id, false); }
    Player& player;
};

/**
 * @brief Learns a spell and all higher ranks linked in its rank chain.
 *
 * @param spellid The base spell identifier.
 */
void Player::learnSpellHighRank(uint32 spellid)
{
    learnSpell(spellid, false);

    DoPlayerLearnSpell worker(*this);
    sSpellMgr.doForHighRanks(spellid, worker);
}

/**
 * @brief Loads skill values from the database and initializes related rewards.
 *
 * @param result The query result containing saved skill rows.
 */
void Player::_LoadSkills(QueryResult* result)
{
    //                                                           0      1      2
    // SetPQuery(PLAYER_LOGIN_QUERY_LOADSKILLS,          "SELECT `skill`, `value`, `max` FROM `character_skills` WHERE `guid` = '%u'", GUID_LOPART(m_guid));

    uint32 count = 0;
    uint8 professionCount = 0;
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            uint16 skill    = fields[0].GetUInt16();
            uint16 value    = fields[1].GetUInt16();
            uint16 max      = fields[2].GetUInt16();

            SkillLineEntry const* pSkill = sSkillLineStore.LookupEntry(skill);
            if (!pSkill)
            {
                sLog.outError("Character %u has skill %u that does not exist.", GetGUIDLow(), skill);
                continue;
            }

            // set fixed skill ranges
            switch (GetSkillRangeType(pSkill, false))
            {
                case SKILL_RANGE_LANGUAGE:                  // 300..300
                    value = max = 300;
                    break;
                case SKILL_RANGE_MONO:                      // 1..1, grey monolite bar
                    value = max = 1;
                    break;
                default:
                    break;
            }

            if (value == 0)
            {
                sLog.outError("Character %u has skill %u with value 0. Will be deleted.", GetGUIDLow(), skill);
                CharacterDatabase.PExecute("DELETE FROM `character_skills` WHERE `guid` = '%u' AND `skill` = '%u' ", GetGUIDLow(), skill);
                continue;
            }

            uint16 field = count / 2;
            uint8 offset = count & 1;

            SetUInt16Value(PLAYER_SKILL_LINEID_0 + field, offset, skill);
            uint16 step = 0;

            if (pSkill->categoryId == SKILL_CATEGORY_SECONDARY)
            {
                step = max / 75;
            }

            if (pSkill->categoryId == SKILL_CATEGORY_PROFESSION)
            {
                step = max / 75;

                if (professionCount < 2)
                {
                    SetUInt32Value(PLAYER_PROFESSION_SKILL_LINE_1 + professionCount++, skill);
                }
            }

            SetUInt16Value(PLAYER_SKILL_STEP_0 + field, offset, step);
            SetUInt16Value(PLAYER_SKILL_RANK_0 + field, offset, value);
            SetUInt16Value(PLAYER_SKILL_MAX_RANK_0 + field, offset, max);
            SetUInt16Value(PLAYER_SKILL_MODIFIER_0 + field, offset, 0);
            SetUInt16Value(PLAYER_SKILL_TALENT_0 + field, offset, 0);

            mSkillStatus.insert(SkillStatusMap::value_type(skill, SkillStatusData(count, SKILL_UNCHANGED)));

            learnSkillRewardedSpells(skill, value);

            ++count;

            if (count >= PLAYER_MAX_SKILLS)                 // client limit
            {
                sLog.outError("Character %u has more than %u skills.", GetGUIDLow(), PLAYER_MAX_SKILLS);
                break;
            }
        }
        while (result->NextRow());
        delete result;
    }

    for (; count < PLAYER_MAX_SKILLS; ++count)
    {
        uint16 field = count / 2;
        uint8 offset = count & 1;

        SetUInt16Value(PLAYER_SKILL_LINEID_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_STEP_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_RANK_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_MAX_RANK_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_MODIFIER_0 + field, offset, 0);
        SetUInt16Value(PLAYER_SKILL_TALENT_0 + field, offset, 0);
    }

    // special settings
    if (getClass() == CLASS_DEATH_KNIGHT)
    {
        uint32 base_level = std::min(getLevel(), sWorld.getConfig(CONFIG_UINT32_START_HEROIC_PLAYER_LEVEL));
        if (base_level < 1)
        {
            base_level = 1;
        }
        uint32 base_skill = (base_level - 1) * 5;           // 270 at starting level 55
        if (base_skill < 1)
        {
            base_skill = 1;                                 // skill mast be known and then > 0 in any case
        }

        if (GetPureSkillValue(SKILL_FIRST_AID) < base_skill)
        {
            SetSkill(SKILL_FIRST_AID, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_AXES) < base_skill)
        {
            SetSkill(SKILL_AXES, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_DEFENSE) < base_skill)
        {
            SetSkill(SKILL_DEFENSE, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_POLEARMS) < base_skill)
        {
            SetSkill(SKILL_POLEARMS, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_SWORDS) < base_skill)
        {
            SetSkill(SKILL_SWORDS, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_2H_AXES) < base_skill)
        {
            SetSkill(SKILL_2H_AXES, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_2H_SWORDS) < base_skill)
        {
            SetSkill(SKILL_2H_SWORDS, base_skill, base_skill);
        }
        if (GetPureSkillValue(SKILL_UNARMED) < base_skill)
        {
            SetSkill(SKILL_UNARMED, base_skill, base_skill);
        }
    }
}

uint32 Player::GetPhaseMaskForSpawn() const
{
    uint32 phase = PHASEMASK_NORMAL;
    if (!isGameMaster())
    {
        phase = GetPhaseMask();
    }
    else
    {
        AuraList const& phases = GetAurasByType(SPELL_AURA_PHASE);
        if (!phases.empty())
        {
            phase = phases.front()->GetMiscValue();
        }
    }

    // some aura phases include 1 normal map in addition to phase itself
    if (uint32 n_phase = phase & ~PHASEMASK_NORMAL)
    {
        return n_phase;
    }

    return PHASEMASK_NORMAL;
}

/**
 * @brief Checks whether a concrete item can be equipped under unique-equip rules.
 *
 * @param pItem The item to test.
 * @param eslot The equipment slot being considered.
 * @return The inventory result describing whether equipping is allowed.
 */
InventoryResult Player::CanEquipUniqueItem(Item* pItem, uint8 eslot, uint32 limit_count) const
{
    ItemPrototype const* pProto = pItem->GetProto();

    // proto based limitations
    if (InventoryResult res = CanEquipUniqueItem(pProto, eslot, limit_count))
    {
        return res;
    }

    // check unique-equipped on gems
    for (uint32 enchant_slot = SOCK_ENCHANTMENT_SLOT; enchant_slot < SOCK_ENCHANTMENT_SLOT + 3; ++enchant_slot)
    {
        uint32 enchant_id = pItem->GetEnchantmentId(EnchantmentSlot(enchant_slot));
        if (!enchant_id)
        {
            continue;
        }
        SpellItemEnchantmentEntry const* enchantEntry = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!enchantEntry)
        {
            continue;
        }

        ItemPrototype const* pGem = ObjectMgr::GetItemPrototype(enchantEntry->GemID);
        if (!pGem)
        {
            continue;
        }

        // include for check equip another gems with same limit category for not equipped item (and then not counted)
        uint32 gem_limit_count = !pItem->IsEquipped() && pGem->ItemLimitCategory
                                 ? pItem->GetGemCountWithLimitCategory(pGem->ItemLimitCategory) : 1;

        if (InventoryResult res = CanEquipUniqueItem(pGem, eslot, gem_limit_count))
        {
            return res;
        }
    }

    return EQUIP_ERR_OK;
}

/**
 * @brief Checks whether an item prototype can be equipped under unique-equip rules.
 *
 * @param itemProto The item prototype to test.
 * @param except_slot An equipment slot to ignore during the check.
 * @return The inventory result describing whether equipping is allowed.
 */
InventoryResult Player::CanEquipUniqueItem(ItemPrototype const* itemProto, uint8 except_slot, uint32 limit_count) const
{
    // check unique-equipped on item
    if (itemProto->Flags & ITEM_FLAG_UNIQUE_EQUIPPED)
    {
        // there is an equip limit on this item
        if (HasItemOrGemWithIdEquipped(itemProto->ItemId, 1, except_slot))
        {
            return EQUIP_ERR_ITEM_UNIQUE_EQUIPABLE;
        }
    }

    // check unique-equipped limit
    if (itemProto->ItemLimitCategory)
    {
        ItemLimitCategoryEntry const* limitEntry = sItemLimitCategoryStore.LookupEntry(itemProto->ItemLimitCategory);
        if (!limitEntry)
        {
            return EQUIP_ERR_ITEM_CANT_BE_EQUIPPED;
        }

        // NOTE: limitEntry->mode not checked because if item have have-limit then it applied and to equip case

        if (limit_count > limitEntry->maxCount)
        {
            return EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS;
        }

        // there is an equip limit on this item
        if (HasItemOrGemWithLimitCategoryEquipped(itemProto->ItemLimitCategory, limitEntry->maxCount - limit_count + 1, except_slot))
        {
            return EQUIP_ERR_ITEM_MAX_LIMIT_CATEGORY_EQUIPPED_EXCEEDED_IS;
        }
    }

    return EQUIP_ERR_OK;
}

/**
 * @brief Calculates and applies fall damage from movement updates.
 *
 * @param movementInfo The movement packet information containing fall data.
 */
void Player::HandleFall(MovementInfo const& movementInfo)
{
    // calculate total z distance of the fall
    float z_diff = m_lastFallZ - movementInfo.GetPos()->z;
    DEBUG_LOG("zDiff = %f", z_diff);

    // Players with low fall distance, Feather Fall or physical immunity (charges used) are ignored
    // 14.57 can be calculated by resolving damageperc formula below to 0
    if (z_diff >= 14.57f && !IsDead() && !isGameMaster() && /*!HasMovementFlag(MOVEFLAG_ONTRANSPORT) &&*/
            !HasAuraType(SPELL_AURA_HOVER) && !HasAuraType(SPELL_AURA_FEATHER_FALL) &&
            !HasAuraType(SPELL_AURA_FLY) && !IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL))
    {
        // Safe fall, fall height reduction
        int32 safe_fall = GetTotalAuraModifier(SPELL_AURA_SAFE_FALL);

        float damageperc = 0.018f * (z_diff - safe_fall) - 0.2426f;

        if (damageperc > 0)
        {
            uint32 damage = (uint32)(damageperc * GetMaxHealth() * sWorld.getConfig(CONFIG_FLOAT_RATE_DAMAGE_FALL));

            float height = movementInfo.GetPos()->z;
            UpdateAllowedPositionZ(movementInfo.GetPos()->x, movementInfo.GetPos()->y, height);

            if (damage > 0)
            {
                // Prevent fall damage from being more than the player maximum health
                if (damage > GetMaxHealth())
                {
                    damage = GetMaxHealth();
                }

                // Gust of Wind
                if (GetDummyAura(43621))
                {
                    damage = GetMaxHealth() / 2;
                }

                uint32 original_health = GetHealth();
                uint32 final_damage = EnvironmentalDamage(DAMAGE_FALL, damage);

                // recheck alive, might have died of EnvironmentalDamage, avoid cases when player die in fact like Spirit of Redemption case
                if (IsAlive() && final_damage < original_health)
                {
                    GetAchievementMgr().UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING, uint32(z_diff * 100));
                }
            }

            // Z given by moveinfo, LastZ, FallTime, WaterZ, MapZ, Damage, Safefall reduction
            DEBUG_LOG("FALLDAMAGE z=%f sz=%f pZ=%f FallTime=%d mZ=%f damage=%d SF=%d" , movementInfo.GetPos()->z, height, GetPositionZ(), movementInfo.GetFallTime(), height, damage, safe_fall);
        }
    }
}

void Player::UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscvalue1/*=0*/, uint32 miscvalue2/*=0*/, Unit* unit/*=NULL*/, uint32 time/*=0*/)
{
    GetAchievementMgr().UpdateAchievementCriteria(type, miscvalue1, miscvalue2, unit, time);
}

void Player::StartTimedAchievementCriteria(AchievementCriteriaTypes type, uint32 timedRequirementId, time_t startTime /*= 0*/)
{
    GetAchievementMgr().StartTimedAchievementCriteria(type, timedRequirementId, startTime);
}

PlayerTalent const* Player::GetKnownTalentById(int32 talentId) const
{
    PlayerTalentMap::const_iterator itr = m_talents[m_activeSpec].find(talentId);
    if (itr != m_talents[m_activeSpec].end() && itr->second.state != PLAYERSPELL_REMOVED)
    {
        return &itr->second;
    }
    else
    {
        return NULL;
    }
}

SpellEntry const* Player::GetKnownTalentRankById(int32 talentId) const
{
    if (PlayerTalent const* talent = GetKnownTalentById(talentId))
    {
        return sSpellStore.LookupEntry(talent->talentEntry->RankID[talent->currentRank]);
    }
    else
    {
        return NULL;
    }
}


/**
 * @brief Clears an at-login flag from the player and optionally from the database.
 *
 * @param f The flag to remove.
 * @param in_db_also True to persist the removal to the database immediately.
 */
void Player::RemoveAtLoginFlag(AtLoginFlags f, bool in_db_also /*= false*/)
{
    m_atLoginFlags &= ~f;

    if (in_db_also)
    {
        CharacterDatabase.PExecute("UPDATE `characters` set `at_login` = `at_login` & ~ %u WHERE `guid` ='%u'", uint32(f), GetGUIDLow());
    }
}

/**
 * @brief Sends a packet that clears a spell cooldown on the client.
 *
 * @param spell_id The spell whose cooldown is being cleared.
 * @param target The target unit associated with the cooldown clear.
 */
void Player::SendClearCooldown(uint32 spell_id, Unit* target)
{
    ObjectGuid guid = target->GetObjectGuid();

    WorldPacket data(SMSG_CLEAR_COOLDOWNS, 1 + 8 + 4);
    data.WriteGuidMask<1, 3, 6>(guid);
    data.WriteBits(1, 24);      // cooldown count
    data.WriteGuidMask<7, 5, 2, 4, 0>(guid);

    data.WriteGuidBytes<7, 2, 4, 5, 1, 3>(guid);
    data << uint32(spell_id);
    data.WriteGuidBytes<0, 6>(guid);

    SendDirectMessage(&data);
}

/**
 * @brief Checks whether the player's movement info currently contains a flag.
 *
 * @param f The movement flag to test.
 * @return True if the flag is present; otherwise, false.
 */
bool Player::HasMovementFlag(MovementFlags f) const
{
    return m_movementInfo.HasMovementFlag(f);
}

void Player::ResetTimeSync()
{
    m_timeSyncCounter = 0;
    m_timeSyncTimer = 0;
    m_timeSyncClient = 0;
    m_timeSyncServer = GameTime::GetGameTimeMS();
}

void Player::SendTimeSync()
{
    WorldPacket data(SMSG_TIME_SYNC_REQ, 4);
    data << uint32(m_timeSyncCounter++);
    GetSession()->SendPacket(&data);

    // Schedule next sync in 10 sec
    m_timeSyncTimer = 10000;
    m_timeSyncServer = GameTime::GetGameTimeMS();
}

void Player::SendDuelCountdown(uint32 counter)
{
    WorldPacket data(SMSG_DUEL_COUNTDOWN, 4);
    data << uint32(counter);                                // seconds
    GetSession()->SendPacket(&data);
}

bool Player::IsImmuneToSpellEffect(SpellEntry const* spellInfo, SpellEffectIndex index, bool castOnSelf) const
{
    SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(index);
    if (spellEffect)
    {
        switch(spellEffect->Effect)
        {
            case SPELL_EFFECT_ATTACK_ME:
                return true;
            default:
                break;
        }
        switch(spellEffect->EffectApplyAuraName)
        {
            case SPELL_AURA_MOD_TAUNT:
                return true;
            default:
                break;
        }
    }

    return Unit::IsImmuneToSpellEffect(spellInfo, index, castOnSelf);
}

/**
 * @brief Sets the player's home bind location and persists it to the database.
 *
 * @param loc The new home bind world location.
 * @param area_id The associated area identifier.
 */
void Player::SetHomebindToLocation(WorldLocation const& loc, uint32 area_id)
{
    m_homebindMapId = loc.mapid;
    m_homebindAreaId = area_id;
    m_homebindX = loc.coord_x;
    m_homebindY = loc.coord_y;
    m_homebindZ = loc.coord_z;

    // update sql homebind
    CharacterDatabase.PExecute("UPDATE `character_homebind` SET `map` = '%u', `zone` = '%u', `position_x` = '%f', `position_y` = '%f', `position_z` = '%f' WHERE `guid` = '%u'",
                               m_homebindMapId, m_homebindAreaId, m_homebindX, m_homebindY, m_homebindZ, GetGUIDLow());
}

/**
 * @brief Resolves an object GUID to a world object constrained by a type mask.
 *
 * @param guid The object GUID to resolve.
 * @param typemask The allowed object type mask.
 * @return The matching object, or NULL if not found or disallowed.
 */
Object* Player::GetObjectByTypeMask(ObjectGuid guid, TypeMask typemask)
{
    switch (guid.GetHigh())
    {
        case HIGHGUID_ITEM:
            if (typemask & TYPEMASK_ITEM)
            {
                return GetItemByGuid(guid);
            }
            break;
        case HIGHGUID_PLAYER:
            if (GetObjectGuid() == guid)
            {
                return this;
            }
            if ((typemask & TYPEMASK_PLAYER) && IsInWorld())
            {
                return sObjectAccessor.FindPlayer(guid);
            }
            break;
        case HIGHGUID_GAMEOBJECT:
            if ((typemask & TYPEMASK_GAMEOBJECT) && IsInWorld())
            {
                return GetMap()->GetGameObject(guid);
            }
            break;
        case HIGHGUID_UNIT:
        case HIGHGUID_VEHICLE:
            if ((typemask & TYPEMASK_UNIT) && IsInWorld())
            {
                return GetMap()->GetCreature(guid);
            }
            break;
        case HIGHGUID_PET:
            if ((typemask & TYPEMASK_UNIT) && IsInWorld())
            {
                return GetMap()->GetPet(guid);
            }
            break;
        case HIGHGUID_DYNAMICOBJECT:
            if ((typemask & TYPEMASK_DYNAMICOBJECT) && IsInWorld())
            {
                return GetMap()->GetDynamicObject(guid);
            }
            break;
        case HIGHGUID_TRANSPORT:
        case HIGHGUID_CORPSE:
        case HIGHGUID_MO_TRANSPORT:
        case HIGHGUID_INSTANCE:
        case HIGHGUID_GROUP:
        default:
            break;
    }

    return NULL;
}

/**
 * @brief Updates the player's current resting state.
 *
 * @param n_r_type The new rest type.
 * @param areaTriggerId The inn or rest area trigger identifier, if applicable.
 */
void Player::SetRestType(RestType n_r_type, uint32 areaTriggerId /*= 0*/)
{
    rest_type = n_r_type;

    if (rest_type == REST_TYPE_NO)
    {
        RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);

        // Set player to FFA PVP when not in rested environment.
        if (sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(true);
        }
    }
    else
    {
        SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING);

        inn_trigger_id = areaTriggerId;
        time_inn_enter = time(NULL);

        if (sWorld.IsFFAPvPRealm())
        {
            SetFFAPvP(false);
        }
    }
}

uint32 Player::GetEquipGearScore(bool withBags, bool withBank)
{
    if (withBags && withBank && m_cachedGS > 0)
    {
        return m_cachedGS;
    }

    GearScoreVec gearScore(EQUIPMENT_SLOT_END);
    uint32 twoHandScore = 0;

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            _fillGearScoreData(item, &gearScore, twoHandScore);
        }
    }

    if (withBags)
    {
        // check inventory
        for (int i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        {
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                _fillGearScoreData(item, &gearScore, twoHandScore);
            }
        }

        // check bags
        for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        {
            if (Bag* pBag = (Bag*)GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
                {
                    if (Item* item2 = pBag->GetItemByPos(j))
                    {
                        _fillGearScoreData(item2, &gearScore, twoHandScore);
                    }
                }
            }
        }
    }

    if (withBank)
    {
        for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; ++i)
        {
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                _fillGearScoreData(item, &gearScore, twoHandScore);
            }
        }

        for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        {
            if (Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            {
                if (item->IsBag())
                {
                    Bag* bag = (Bag*)item;
                    for (uint8 j = 0; j < bag->GetBagSize(); ++j)
                    {
                        if (Item* item2 = bag->GetItemByPos(j))
                        {
                            _fillGearScoreData(item2, &gearScore, twoHandScore);
                        }
                    }
                }
            }
        }
    }

    uint8 count = EQUIPMENT_SLOT_END - 2;   // ignore body and tabard slots
    uint32 sum = 0;

    // check if 2h hand is higher level than main hand + off hand
    if (gearScore[EQUIPMENT_SLOT_MAINHAND] + gearScore[EQUIPMENT_SLOT_OFFHAND] < twoHandScore * 2)
    {
        gearScore[EQUIPMENT_SLOT_OFFHAND] = 0;  // off hand is ignored in calculations if 2h weapon has higher score
        --count;
        gearScore[EQUIPMENT_SLOT_MAINHAND] = twoHandScore;
    }

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        sum += gearScore[i];
    }

    if (count)
    {
        uint32 res = uint32(sum / count);
        DEBUG_LOG("Player: calculating gear score for %u. Result is %u", GetObjectGuid().GetCounter(), res);

        if (withBags && withBank)
        {
            m_cachedGS = res;
        }

        return res;
    }
    else
    {
        return 0;
    }
}

void Player::_fillGearScoreData(Item* item, GearScoreVec* gearScore, uint32& twoHandScore)
{
    if (!item)
    {
        return;
    }

    if (CanUseItem(item->GetProto()) != EQUIP_ERR_OK)
    {
        return;
    }

    uint8 type   = item->GetProto()->InventoryType;
    uint32 level = item->GetProto()->ItemLevel;

    switch (type)
    {
        case INVTYPE_2HWEAPON:
            twoHandScore = std::max(twoHandScore, level);
            break;
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
            (*gearScore)[EQUIPMENT_SLOT_MAINHAND] = std::max((*gearScore)[EQUIPMENT_SLOT_MAINHAND], level);
            break;
        case INVTYPE_SHIELD:
        case INVTYPE_WEAPONOFFHAND:
            (*gearScore)[EQUIPMENT_SLOT_OFFHAND] = std::max((*gearScore)[EQUIPMENT_SLOT_OFFHAND], level);
            break;
        case INVTYPE_THROWN:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_RANGED:
        case INVTYPE_QUIVER:
        case INVTYPE_RELIC:
            (*gearScore)[EQUIPMENT_SLOT_RANGED] = std::max((*gearScore)[EQUIPMENT_SLOT_RANGED], level);
            break;
        case INVTYPE_HEAD:
            (*gearScore)[EQUIPMENT_SLOT_HEAD] = std::max((*gearScore)[EQUIPMENT_SLOT_HEAD], level);
            break;
        case INVTYPE_NECK:
            (*gearScore)[EQUIPMENT_SLOT_NECK] = std::max((*gearScore)[EQUIPMENT_SLOT_NECK], level);
            break;
        case INVTYPE_SHOULDERS:
            (*gearScore)[EQUIPMENT_SLOT_SHOULDERS] = std::max((*gearScore)[EQUIPMENT_SLOT_SHOULDERS], level);
            break;
        case INVTYPE_BODY:
            (*gearScore)[EQUIPMENT_SLOT_BODY] = std::max((*gearScore)[EQUIPMENT_SLOT_BODY], level);
            break;
        case INVTYPE_CHEST:
            (*gearScore)[EQUIPMENT_SLOT_CHEST] = std::max((*gearScore)[EQUIPMENT_SLOT_CHEST], level);
            break;
        case INVTYPE_WAIST:
            (*gearScore)[EQUIPMENT_SLOT_WAIST] = std::max((*gearScore)[EQUIPMENT_SLOT_WAIST], level);
            break;
        case INVTYPE_LEGS:
            (*gearScore)[EQUIPMENT_SLOT_LEGS] = std::max((*gearScore)[EQUIPMENT_SLOT_LEGS], level);
            break;
        case INVTYPE_FEET:
            (*gearScore)[EQUIPMENT_SLOT_FEET] = std::max((*gearScore)[EQUIPMENT_SLOT_FEET], level);
            break;
        case INVTYPE_WRISTS:
            (*gearScore)[EQUIPMENT_SLOT_WRISTS] = std::max((*gearScore)[EQUIPMENT_SLOT_WRISTS], level);
            break;
        case INVTYPE_HANDS:
            (*gearScore)[EQUIPMENT_SLOT_HEAD] = std::max((*gearScore)[EQUIPMENT_SLOT_HEAD], level);
            break;
            // equipped gear score check uses both rings and trinkets for calculation, assume that for bags/banks it is the same
            // with keeping second highest score at second slot
        case INVTYPE_FINGER:
        {
            if ((*gearScore)[EQUIPMENT_SLOT_FINGER1] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_FINGER2] = (*gearScore)[EQUIPMENT_SLOT_FINGER1];
                (*gearScore)[EQUIPMENT_SLOT_FINGER1] = level;
            }
            else if ((*gearScore)[EQUIPMENT_SLOT_FINGER2] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_FINGER2] = level;
            }
            break;
        }
        case INVTYPE_TRINKET:
        {
            if ((*gearScore)[EQUIPMENT_SLOT_TRINKET1] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_TRINKET2] = (*gearScore)[EQUIPMENT_SLOT_TRINKET1];
                (*gearScore)[EQUIPMENT_SLOT_TRINKET1] = level;
            }
            else if ((*gearScore)[EQUIPMENT_SLOT_TRINKET2] < level)
            {
                (*gearScore)[EQUIPMENT_SLOT_TRINKET2] = level;
            }
            break;
        }
        case INVTYPE_CLOAK:
            (*gearScore)[EQUIPMENT_SLOT_BACK] = std::max((*gearScore)[EQUIPMENT_SLOT_BACK], level);
            break;
        default:
            break;
    }
}

// Player::SendCurrencies / Get/SendCurrencyWeekCap / GetCurrencyTotalCap / Get*Count moved
// to CurrencyMgr (2026-05-12); thin delegating wrappers live inline in Player.h.

// Player::ModifyCurrencyCount / SetCurrencyCount / _LoadCurrencies / _SaveCurrencies /
// SetCurrencyFlags / ResetCurrencyWeekCounts moved to CurrencyMgr (2026-05-12);
// thin delegating wrappers live inline in Player.h.

void Player::SendPvPRewards()
{
    // Placeholder

    WorldPacket data(SMSG_PVP_REWARDS, 6 * 4);
    data << uint32(1650);   // rbg conquest cap
    data << uint32(0);      // total conquest earned
    data << uint32(1350);   // arena conquest cap
    data << uint32(0);      // rbg conquest earned
    data << uint32(0);      // arena conquest earned
    data << uint32(1650);   // total conquest cap

    SendDirectMessage(&data);
}

void Player::SendRatedBGStats()
{
    // Placeholder

    WorldPacket data(SMSG_RATED_BG_STATS, 18 * 4);
    for (int i = 0; i < 18; ++i)
    {
        data << uint32(0);
    }

    SendDirectMessage(&data);
}

/**
 * @brief Evaluates whether the player can use an area trigger into another map.
 *
 * @param at The area trigger being used.
 * @param miscRequirement Output requirement data for failure messaging.
 * @return The evaluated area lock status.
 */
AreaLockStatus Player::GetAreaTriggerLockStatus(AreaTrigger const* at, Difficulty difficulty, uint32& miscRequirement)
{
    miscRequirement = 0;

    if (!at)
    {
        return AREA_LOCKSTATUS_UNKNOWN_ERROR;
    }

    MapEntry const* mapEntry = sMapStore.LookupEntry(at->target_mapId);
    if (!mapEntry)
    {
        return AREA_LOCKSTATUS_UNKNOWN_ERROR;
    }

    bool isRegularTargetMap = !mapEntry->IsDungeon() || GetDifficulty(mapEntry->IsRaid()) == REGULAR_DIFFICULTY;

    MapDifficultyEntry const* mapDiff = GetMapDifficultyData(at->target_mapId, difficulty);
    if (mapEntry->IsDungeon() && !mapDiff)
    {
        return AREA_LOCKSTATUS_MISSING_DIFFICULTY;
    }

    // Expansion requirement
    if (GetSession()->Expansion() < mapEntry->Expansion())
    {
        miscRequirement = mapEntry->Expansion();
        return AREA_LOCKSTATUS_INSUFFICIENT_EXPANSION;
    }

    // Gamemaster can always enter
    if (isGameMaster())
    {
        return AREA_LOCKSTATUS_OK;
    }

    // Level Requirements
    if (getLevel() < at->requiredLevel && !sWorld.getConfig(CONFIG_BOOL_INSTANCE_IGNORE_LEVEL))
    {
        miscRequirement = at->requiredLevel;
        return AREA_LOCKSTATUS_TOO_LOW_LEVEL;
    }
    if (!isRegularTargetMap && !sWorld.getConfig(CONFIG_BOOL_INSTANCE_IGNORE_LEVEL) && getLevel() < uint32(maxLevelForExpansion[mapEntry->Expansion()]))
    {
        miscRequirement = maxLevelForExpansion[mapEntry->Expansion()];
        return AREA_LOCKSTATUS_TOO_LOW_LEVEL;
    }

    // Raid Requirements
    if (mapEntry->IsRaid() && !sWorld.getConfig(CONFIG_BOOL_INSTANCE_IGNORE_RAID))
        if (!GetGroup() || !GetGroup()->isRaidGroup())
        {
            return AREA_LOCKSTATUS_RAID_LOCKED;
        }

    // Item Requirements: must have requiredItem OR requiredItem2, report the first one that's missing
    if (at->requiredItem)
    {
        if (!HasItemCount(at->requiredItem, 1) &&
                (!at->requiredItem2 || !HasItemCount(at->requiredItem2, 1)))
        {
            miscRequirement = at->requiredItem;
            return AREA_LOCKSTATUS_MISSING_ITEM;
        }
    }
    else if (at->requiredItem2 && !HasItemCount(at->requiredItem2, 1))
    {
        miscRequirement = at->requiredItem2;
        return AREA_LOCKSTATUS_MISSING_ITEM;
    }
    // Heroic item requirements
    if (!isRegularTargetMap && at->heroicKey)
    {
        if (!HasItemCount(at->heroicKey, 1) && (!at->heroicKey2 || !HasItemCount(at->heroicKey2, 1)))
        {
            miscRequirement = at->heroicKey;
            return AREA_LOCKSTATUS_MISSING_ITEM;
        }
    }
    else if (!isRegularTargetMap && at->heroicKey2 && !HasItemCount(at->heroicKey2, 1))
    {
        miscRequirement = at->heroicKey2;
        return AREA_LOCKSTATUS_MISSING_ITEM;
    }

    // Quest Requirements
    if (isRegularTargetMap && at->requiredQuest && !GetQuestRewardStatus(at->requiredQuest))
    {
        miscRequirement = at->requiredQuest;
        return AREA_LOCKSTATUS_QUEST_NOT_COMPLETED;
    }
    if (!isRegularTargetMap && at->requiredQuestHeroic && !GetQuestRewardStatus(at->requiredQuestHeroic))
    {
        miscRequirement = at->requiredQuestHeroic;
        return AREA_LOCKSTATUS_QUEST_NOT_COMPLETED;
    }

    // If the map is not created, assume it is possible to enter it.
    DungeonPersistentState* state = GetBoundInstanceSaveForSelfOrGroup(at->target_mapId);
    Map* map = sMapMgr.FindMap(at->target_mapId, state ? state->GetInstanceId() : 0);

    // ToDo add achievement check

    // Map's state check
    if (map && map->IsDungeon())
    {
        // cannot enter if the instance is full (player cap), GMs don't count
        if (((DungeonMap*)map)->GetPlayersCountExceptGMs() >= ((DungeonMap*)map)->GetMaxPlayers())
        {
            return AREA_LOCKSTATUS_INSTANCE_IS_FULL;
        }

        // In Combat check
        if (map && map->GetInstanceData() && map->GetInstanceData()->IsEncounterInProgress())
        {
            return AREA_LOCKSTATUS_ZONE_IN_COMBAT;
        }

        // Bind Checks
        InstancePlayerBind* pBind = GetBoundInstance(at->target_mapId, GetDifficulty(mapEntry->IsRaid()));
        if (pBind && pBind->perm && pBind->state != state)
        {
            return AREA_LOCKSTATUS_HAS_BIND;
        }
        if (pBind && pBind->perm && pBind->state != map->GetPersistentState())
        {
            return AREA_LOCKSTATUS_HAS_BIND;
        }
    }

    return AREA_LOCKSTATUS_OK;
}

const uint32 armorSpecToClass[MAX_CLASSES] =
{
    0,
    86526,  // CLASS_WARRIOR
    86525,  // CLASS_PALADIN
    86528,  // CLASS_HUNTER
    86531,  // CLASS_ROGUE
    0,      // CLASS_PRIEST
    86524,  // CLASS_DEATH_KNIGHT
    86529,  // CLASS_SHAMAN
    0,      // CLASS_MAGE
    0,      // CLASS_WARLOCK
    0,      // CLASS_UNK2
    86530,  // CLASS_DRUID
};

#define MAX_ARMOR_SPECIALIZATION_SPELLS 18
struct armorSpecToTabInfo
{
    uint32 spellId;
    uint8 Class;
    uint16 tab;
};

const armorSpecToTabInfo armorSpecToTab[MAX_ARMOR_SPECIALIZATION_SPELLS] =
{
    { 86537, CLASS_DEATH_KNIGHT, 398 }, // blood
    { 86113, CLASS_DEATH_KNIGHT, 399 }, // frost
    { 86536, CLASS_DEATH_KNIGHT, 400 }, // unholy
    { 86093, CLASS_DRUID, 752 },        // balance
    { 86096, CLASS_DRUID, 750 },        // feral
    { 86097, CLASS_DRUID, 750 },        // feral
    { 86104, CLASS_DRUID, 748 },        // resto
    { 86538, CLASS_HUNTER, 0 },         //
    { 86103, CLASS_PALADIN, 831 },      // holy
    { 86102, CLASS_PALADIN, 839 },      // prot
    { 86539, CLASS_PALADIN, 855 },      // retro
    { 86092, CLASS_ROGUE, 0 },          //
    { 86100, CLASS_SHAMAN, 261 },       // elem
    { 86099, CLASS_SHAMAN, 263 },       // ench
    { 86108, CLASS_SHAMAN, 262 },       // restor
    { 86101, CLASS_WARRIOR, 746 },      // arms
    { 86110, CLASS_WARRIOR, 815 },      // fury
    { 86535, CLASS_WARRIOR, 845 },      // prot
};

void Player::UpdateArmorSpecializations()
{
    uint32 specPassive = armorSpecToClass[getClass()];
    // return class has no armor specialization
    if (!specPassive)
    {
        return;
    }

    for (int i = 0; i < MAX_ARMOR_SPECIALIZATION_SPELLS; ++i)
    {
        if (armorSpecToTab[i].Class != getClass())
        {
            continue;
        }

        SpellEntry const * spellProto = sSpellStore.LookupEntry(armorSpecToTab[i].spellId);
        if (!spellProto || !spellProto->HasAttribute(SPELL_ATTR_EX8_ARMOR_SPECIALIZATION))
        {
            sLog.outError("Player::UpdateArmorSpecializations: unexistent or wrong spell %u for class %u",
                armorSpecToTab[i].spellId, armorSpecToTab[i].Class);
            continue;
        }

        // remove if base passive is unlearned
        if (!HasSpell(specPassive))
        {
            RemoveAurasDueToSpell(spellProto->Id);
            continue;
        }

        SpellAuraHolder* holder = GetSpellAuraHolder(spellProto->Id);
        if (!holder)
        {
            // cast absent spells that may be missing due to shapeshift form dependency
            CastSpell(this, spellProto->Id, true);
            continue;
        }

        Aura* aura = holder->GetAuraByEffectIndex(EFFECT_INDEX_0);
        if (!aura)
        {
            continue;
        }

        // recalculate modifier depending on current tree
        aura->ApplyModifier(false, false);
        aura->GetModifier()->m_amount = CalculateSpellDamage(this, spellProto, EFFECT_INDEX_0);
        aura->ApplyModifier(true, false);
    }
}

bool Player::FitArmorSpecializationRules(SpellEntry const * spellProto) const
{
    if (!spellProto || !spellProto->HasAttribute(SPELL_ATTR_EX8_ARMOR_SPECIALIZATION))
    {
        return true;
    }

    int i = 0;
    for (; i < MAX_ARMOR_SPECIALIZATION_SPELLS; ++i)
    {
        if (spellProto->Id == armorSpecToTab[i].spellId)
        {
            if (!armorSpecToTab[i].tab && m_talentsPrimaryTree[m_activeSpec] == 0 ||
                armorSpecToTab[i].tab && armorSpecToTab[i].tab != m_talentsPrimaryTree[m_activeSpec])
                return false;

            break;
        }
    }

    if (i == MAX_ARMOR_SPECIALIZATION_SPELLS)
    {
        return false;
    }

    if (!HasSpell(armorSpecToClass[getClass()]))
    {
        return false;
    }

    if (SpellEquippedItemsEntry const * itemsEntry = spellProto->GetSpellEquippedItems())
    {
        // there spells check items with inventory types which are in EquippedItemInventoryTypeMask
        uint32 inventoryTypeMask = itemsEntry->EquippedItemInventoryTypeMask;
        // get slots that should be check for item presence and SpellEquippedItemsEntry match
        uint32 slotMask = 0;
        uint8 slots[4];
        for (int i = 0; i < MAX_INVTYPE; ++i)
        {
            if (inventoryTypeMask & (1 << i))
            {
                if (!GetSlotsForInventoryType(i, slots))
                {
                    continue;
                }
                for (int j = 0; j < 4; ++j)
                    if (slots[j] != NULL_SLOT)
                    {
                        slotMask |= 1 << slots[j];
                    }
            }
        }

        for (int i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
        {
            if (slotMask & (1 << i))
            {
                Item* item = GetItemByPos(INVENTORY_SLOT_BAG_0, i);
                // item must be present for specialization to work
                if (!item)
                {
                    return false;
                }

                if (item->GetProto()->Class != itemsEntry->EquippedItemClass)
                {
                    return false;
                }

                if (((1 << item->GetProto()->SubClass) & itemsEntry->EquippedItemSubClassMask) == 0)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 * @brief Computes rested experience gained over a period of time.
 *
 * @param timePassed The elapsed time in seconds.
 * @param offline True when the gain is being computed for offline time.
 * @param inRestPlace True when the player is in a rest area while offline.
 * @return The amount of rested experience bonus gained.
 */
float Player::ComputeRest(time_t timePassed, bool offline /*= false*/, bool inRestPlace /*= false*/)
{
    // Every 8h in resting zone we gain a bubble
    // A bubble is 5% of the total xp so there are 20 bubbles
    // So we gain (total XP/20 every 8h) (8h = 288800 sec)
    // (TotalXP/20)/28800; simplified to (TotalXP/576000) per second
    // Client automatically double the value sent so we have to divide it by 2
    // So final formula (TotalXP/1152000)
    float bonus = timePassed * (GetUInt32Value(PLAYER_NEXT_LEVEL_XP) / 1152000.0f); // Get the gained rest xp for given second
    if (!offline)
    {
        bonus *= sWorld.getConfig(CONFIG_FLOAT_RATE_REST_INGAME);                   // Apply the custom setting
    }
    else
    {
        if (inRestPlace)
        {
            bonus *= sWorld.getConfig(CONFIG_FLOAT_RATE_REST_OFFLINE_IN_TAVERN_OR_CITY);
        }
        else
        {
            bonus *= sWorld.getConfig(CONFIG_FLOAT_RATE_REST_OFFLINE_IN_WILDERNESS) / 4.0f; // bonus is reduced by 4 when not in rest place
        }
    }
    return bonus;
}

float Player::GetCollisionHeight(bool mounted) const
{
    if (mounted)
    {
        // mounted case
        CreatureDisplayInfoEntry const* mountDisplayInfo = sCreatureDisplayInfoStore.LookupEntry(GetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID));
        if (!mountDisplayInfo)
        {
            return GetCollisionHeight(false);
        }

        CreatureModelDataEntry const* mountModelData = sCreatureModelDataStore.LookupEntry(mountDisplayInfo->ModelId);
        if (!mountModelData)
        {
            return GetCollisionHeight(false);
        }

        CreatureDisplayInfoEntry const* displayInfo = sCreatureDisplayInfoStore.LookupEntry(GetNativeDisplayId());
        if (!displayInfo)
        {
            sLog.outError("GetCollisionHeight::Unable to find CreatureDisplayInfoEntry for %u", GetNativeDisplayId());
            return 0;
        }

        CreatureModelDataEntry const* modelData = sCreatureModelDataStore.LookupEntry(displayInfo->ModelId);
        if (!modelData)
        {
            sLog.outError("GetCollisionHeight::Unable to find CreatureModelDataEntry for %u", displayInfo->ModelId);
            return 0;
        }

        float scaleMod = GetObjectScale(); // 99% sure about this

        return scaleMod * mountModelData->MountHeight + modelData->CollisionHeight * 0.5f;
    }
    else
    {
        // use native model collision height in dismounted case
        CreatureDisplayInfoEntry const* displayInfo = sCreatureDisplayInfoStore.LookupEntry(GetNativeDisplayId());
        if (!displayInfo)
        {
            sLog.outError("GetCollisionHeight::Unable to find CreatureDisplayInfoEntry for %u", GetNativeDisplayId());
            return 0;
        }

        CreatureModelDataEntry const* modelData = sCreatureModelDataStore.LookupEntry(displayInfo->ModelId);
        if (!modelData)
        {
            sLog.outError("GetCollisionHeight::Unable to find CreatureModelDataEntry for %u", displayInfo->ModelId);
            return 0;
        }

        return modelData->CollisionHeight;
    }
}

// set data to accept next resurrect response and process it with required data
void Player::setResurrectRequestData(Unit* caster, uint32 health, uint32 mana)
{
    m_resurrectGuid = caster->GetObjectGuid();
    m_resurrectMap = caster->GetMapId();
    caster->GetPosition(m_resurrectX, m_resurrectY, m_resurrectZ);
    m_resurrectHealth = health;
    m_resurrectMana = mana;
    m_resurrectToGhoul = false;
}

// we can use this to prepare data in case we have to resurrect player in ghoul form
void Player::setResurrectRequestDataToGhoul(Unit* caster)
{
    setResurrectRequestData(caster, 0, 0);
    m_resurrectToGhoul = true;
}

// player is interacting so we have to remove non authorized aura
void Player::DoInteraction(ObjectGuid const& interactObjGuid)
{
    if (interactObjGuid.IsUnit())
    {
        // remove some aura like stealth aura
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_TALK);
    }
    else if (interactObjGuid.IsGameObject())
    {
        // remove some aura like stealth aura
        RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_USE);
    }
    SendForcedObjectUpdate();
}


void Player::SendPetitionSignResult(ObjectGuid petitionGuid, Player* player, uint32 result)
{
    WorldPacket data(SMSG_PETITION_SIGN_RESULTS, 8 + 8 + 4);
    data << petitionGuid;
    data << player->GetObjectGuid();
    data << uint32(result);
    GetSession()->SendPacket(&data);
}

void Player::SendPetitionTurnInResult(uint32 result)
{
    WorldPacket data(SMSG_TURN_IN_PETITION_RESULTS, 4);
    data << uint32(result);
    GetSession()->SendPacket(&data);
}
