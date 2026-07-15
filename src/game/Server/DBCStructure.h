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

#ifndef MANGOS_DBCSTRUCTURE_H
#define MANGOS_DBCSTRUCTURE_H

#include "Common.h"
#include "DBCEnums.h"
#include "Path.h"
#include "Platform/Define.h"
#include "SharedDefines.h"

#include <map>
#include <set>
#include <vector>

// Structures using to access raw DBC data and required packing to portability

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push,N), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

typedef char const* const* DBCString;                       //char* DBCStrings[MAX_LOCALE];

struct AchievementEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Faction;                                  // 1        m_faction -1=all, 0=horde, 1=alliance
    uint32    Instance_ID;                                        // 2        m_instance_id -1=none
    uint32      Supercedes;                          // 3        m_supercedes its Achievement parent (can`t start while parent uncomplete, use its Criteria if don`t have own, use its progress on begin)
    DBCString   Title_lang;                                       // 4        m_title_lang
    DBCString   Description_lang;                                // 5        m_description_lang
    uint32      Category;                                 // 6        m_category
    uint32      Points;                                     // 7        m_points
    uint32      Ui_order;                            // 8        m_ui_order
    uint32      Flags;                                      // 9        m_flags
    uint32      IconID;                                       // 10       m_iconID
    DBCString   Reward_lang;                                // 11       m_reward_lang
    uint32      Minimum_criteria;                                      // 12       m_minimum_criteria - need this count of completed criterias (own or referenced achievement criterias)
    uint32      Shares_criteria;                             // 13       m_shares_criteria - referenced achievement (counting of all completed criterias)
};

struct AchievementCategoryEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    parentCategory;                               // 1        m_parent -1 for main category
    DBCString name;                                         // 2        m_name_lang
    uint32    sortOrder;                                    // 3        m_ui_order
};

struct AchievementCriteriaEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  referredAchievement;                            // 1        m_achievement_id
    uint32  requiredType;                                   // 2        m_type
    union
    {
        // ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE          = 0
        // TODO: also used for player deaths..
        struct
        {
            uint32  creatureID;                             // 3
            uint32  creatureCount;                          // 4
        } kill_creature;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_BG                 = 1
        struct
        {
            uint32  bgMapID;                                // 3
            uint32  winCount;                               // 4
            uint32  additionalRequirement1_type;            // 5
            uint32  additionalRequirement1_value;           // 6
            uint32  additionalRequirement2_type;            // 7
            uint32  additionalRequirement2_value;           // 8
        } win_bg;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL            = 5
        struct
        {
            uint32  unused;                                 // 3
            uint32  level;                                  // 4
        } reach_level;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL      = 7
        struct
        {
            uint32  skillID;                                // 3
            uint32  skillLevel;                             // 4
        } reach_skill_level;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT   = 8
        struct
        {
            uint32  linkedAchievement;                      // 3
        } complete_achievement;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT   = 9
        struct
        {
            uint32  unused;                                 // 3
            uint32  totalQuestCount;                        // 4
        } complete_quest_count;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY = 10
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfDays;                           // 4
        } complete_daily_quest_daily;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE = 11
        struct
        {
            uint32  zoneID;                                 // 3
            uint32  questCount;                             // 4
        } complete_quests_in_zone;

        // ACHIEVEMENT_CRITERIA_TYPE_CURRENCY_EARNED         = 12
        struct
        {
            uint32 currencyId;                              // 3
            uint32 count;                                   // 4
        } currencyEarned;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST   = 14
        struct
        {
            uint32  unused;                                 // 3
            uint32  questCount;                             // 4
        } complete_daily_quest;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND  = 15
        struct
        {
            uint32  mapID;                                  // 3
        } complete_battleground;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP           = 16
        struct
        {
            uint32  mapID;                                  // 3
        } death_at_map;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON       = 18
        struct
        {
            uint32  manLimit;                               // 3
        } death_in_dungeon;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID          = 19
        struct
        {
            uint32  groupSize;                              // 3 can be 5, 10 or 25
        } complete_raid;

        // ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE     = 20
        struct
        {
            uint32  creatureEntry;                          // 3
        } killed_by_creature;

        // ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING     = 24
        struct
        {
            uint32  unused;                                 // 3
            uint32  fallHeight;                             // 4
        } fall_without_dying;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM            = 26
        struct
        {
            uint32 type;                                    // 3, see enum EnviromentalDamage
        } death_from;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST         = 27
        struct
        {
            uint32  questID;                                // 3
            uint32  questCount;                             // 4
        } complete_quest;

        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET        = 28
        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2       = 69
        struct
        {
            uint32  spellID;                                // 3
            uint32  spellCount;                             // 4
        } be_spell_target;

        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL             = 29
        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2            = 110
        struct
        {
            uint32  spellID;                                // 3
            uint32  castCount;                              // 4
        } cast_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA = 31
        struct
        {
            uint32  areaID;                                 // 3 Reference to AreaTable.dbc
            uint32  killCount;                              // 4
        } honorable_kill_at_area;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA              = 32
        struct
        {
            uint32  mapID;                                  // 3 Reference to Map.dbc
        } win_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA             = 33
        struct
        {
            uint32  mapID;                                  // 3 Reference to Map.dbc
        } play_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL            = 34
        struct
        {
            uint32  spellID;                                // 3 Reference to Map.dbc
        } learn_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM               = 36
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } own_item;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA        = 37
        struct
        {
            uint32  unused;                                 // 3
            uint32  count;                                  // 4
            uint32  flag;                                   // 5 4=in a row
        } win_rated_arena;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING    = 38
        struct
        {
            uint32  teamtype;                               // 3 {2,3,5}
        } highest_team_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING= 39
        struct
        {
            uint32  teamtype;                               // 3 {2,3,5}
            uint32  teamrating;                             // 4
        } highest_personal_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL      = 40
        struct
        {
            uint32  skillID;                                // 3
            uint32  skillLevel;                             // 4 apprentice=1, journeyman=2, expert=3, artisan=4, master=5, grand master=6
        } learn_skill_level;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM               = 41
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } use_item;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM              = 42
        struct
        {
            uint32  itemID;                                 // 3
            uint32  itemCount;                              // 4
        } loot_item;

        // ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA           = 43
        struct
        {
            // TODO: This rank is _NOT_ the index from AreaTable.dbc
            uint32  areaReference;                          // 3
        } explore_area;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK               = 44
        struct
        {
            // TODO: This rank is _NOT_ the index from CharTitles.dbc
            uint32  rank;                                   // 3
        } own_rank;

        // ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT          = 45
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfSlots;                          // 4
        } buy_bank_slot;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION        = 46
        struct
        {
            uint32  factionID;                              // 3
            uint32  reputationAmount;                       // 4 Total reputation amount, so 42000 = exalted
        } gain_reputation;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION= 47
        struct
        {
            uint32  unused;                                 // 3
            uint32  numberOfExaltedFactions;                // 4
        } gain_exalted_reputation;

        // ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP      = 48
        struct
        {
            uint32 unused;                                  // 3
            uint32 numberOfVisits;                          // 4
        } visit_barber;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM        = 49
        // TODO: where is the required itemlevel stored?
        struct
        {
            uint32  itemSlot;                               // 3
            uint32  count;                                  // 4
        } equip_epic_item;

        // ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT      = 50
        struct
        {
            uint32  rollValue;                              // 3
            uint32  count;                                  // 4
        } roll_need_on_loot;
        // ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT      = 51
        struct
        {
            uint32  rollValue;                              // 3
            uint32  count;                                  // 4
        } roll_greed_on_loot;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS               = 52
        struct
        {
            uint32  classID;                                // 3
            uint32  count;                                  // 4
        } hk_class;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_RACE                = 53
        struct
        {
            uint32  raceID;                                 // 3
            uint32  count;                                  // 4
        } hk_race;

        // ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE               = 54
        // TODO: where is the information about the target stored?
        struct
        {
            uint32  emoteID;                                // 3 enum TextEmotes
            uint32  count;                                  // 4 count of emotes, always required special target or requirements
        } do_emote;
        // ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE            = 13
        // ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE           = 55
        // ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS      = 56
        struct
        {
            uint32  unused;                                 // 3
            uint32  count;                                  // 4
            uint32  flag;                                   // 5 =3 for battleground healing
            uint32  mapid;                                  // 6
        } healing_done;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM             = 57
        struct
        {
            uint32  itemID;                                 // 3
            uint32  count;                                  // 4
        } equip_item;

        // ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD= 62
        struct
        {
            uint32  unused;                                 // 3
            uint32  goldInCopper;                           // 4
        } quest_reward_money;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY             = 67
        struct
        {
            uint32  unused;                                 // 3
            uint32  goldInCopper;                           // 4
        } loot_money;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT         = 68
        struct
        {
            uint32  goEntry;                                // 3
            uint32  useCount;                               // 4
        } use_gameobject;

        // ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL       = 70
        // TODO: are those special criteria stored in the dbc or do we have to add another sql table?
        struct
        {
            uint32  unused;                                 // 3
            uint32  killCount;                              // 4
        } special_pvp_kill;

        // ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT     = 72
        struct
        {
            uint32  goEntry;                                // 3
            uint32  lootCount;                              // 4
        } fish_in_gameobject;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS = 75
        struct
        {
            uint32  skillLine;                              // 3
            uint32  spellCount;                             // 4
        } learn_skillline_spell;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL               = 76
        struct
        {
            uint32  unused;                                 // 3
            uint32  duelCount;                              // 4
        } win_duel;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_POWER          = 96
        struct
        {
            uint32  powerType;                              // 3 mana=0, 1=rage, 3=energy, 6=runic power
        } highest_power;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_STAT           = 97
        struct
        {
            uint32  statType;                               // 3 4=spirit, 3=int, 2=stamina, 1=agi, 0=strength
        } highest_stat;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELLPOWER     = 98
        struct
        {
            uint32  spellSchool;                            // 3
        } highest_spellpower;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_RATING         = 100
        struct
        {
            uint32  ratingType;                             // 3
        } highest_rating;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE              = 109
        struct
        {
            uint32  lootType;                               // 3 3=fishing, 2=pickpocket, 4=disentchant
            uint32  lootTypeCount;                          // 4
        } loot_type;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE       = 112
        struct
        {
            uint32  skillLine;                              // 3
            uint32  spellCount;                             // 4
        } learn_skill_line;

        // ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL    = 113
        struct
        {
            uint32  unused;                                 // 3
            uint32  killCount;                              // 4
        } honorable_kill;

        struct
        {
            uint32  value;                                  // 3        m_asset_id
            uint32  count;                                  // 4        m_quantity
            uint32  additionalRequirement1_type;            // 5        m_start_event
            uint32  additionalRequirement1_value;           // 6        m_start_asset
            uint32  additionalRequirement2_type;            // 7        m_fail_event
            uint32  additionalRequirement2_value;           // 8        m_fail_asset
        } raw;
    };
    //uint32  unk1;                                         // 9
    DBCString name;                                         // 10       m_description_lang
    uint32  completionFlag;                                 // 11       m_flags
    uint32  timedCriteriaStartType;                         // 12       m_timer_start_event Only appears with timed achievements, seems to be the type of starting a timed Achievement, only type 1 and some of type 6 need manual starting
                                                            //              1: ByEventId(?) (serverside IDs),    2: ByQuestId,   5: ByCastSpellId(?)
                                                            //              6: BySpellIdTarget(some of these are unknown spells, some not, some maybe spells)
                                                            //              7: ByKillNpcId,  9: ByUseItemId
    uint32  timedCriteriaMiscId;                            // 13       m_timer_asset_id Alway appears with timed events, used internally to start the achievement, store
    uint32  timeLimit;                                      // 14       m_timer_time time limit in seconds
    uint32  showOrder;                                      // 15       m_ui_order  also used in achievement shift-links as index in state bitmask
    //uint32 unk2;                                          // 16
    //uint32 moreRequirement[3];                            // 17-19
    //uint32 moreRequirementValue[3];                       // 20-22

    // helpers
    bool IsExplicitlyStartedTimedCriteria() const
    {
        if (!timeLimit)
        {
            return false;
        }

        // in case raw.value == timedCriteriaMiscId in timedCriteriaMiscId stored spellid/itemids for cast/use, so repeating aura start at first cast/use until fails
        return requiredType == ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST || raw.value != timedCriteriaMiscId;
    }
};

struct AreaTableEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  ContinentID;                                          // 1        m_ContinentID
    uint32  ParentAreaID;                                           // 2        m_ParentAreaID
    uint32  AreaBit;                                    // 3        m_AreaBit
    uint32  Flags;                                          // 4        m_flags
                                                            // 5        m_SoundProviderPref
                                                            // 6        m_SoundProviderPrefUnderwater
                                                            // 7        m_AmbienceID
                                                            // 8        m_ZoneMusic
                                                            // 9        m_IntroSound
    int32   ExplorationLevel;                                     // 10       m_ExplorationLevel
    DBCString AreaName_lang;                                    // 11       m_AreaName_lang
    uint32  FactionGroupMask;                                           // 12       m_factionGroupMask
    uint32  LiquidTypeID[4];                          // 13-16    m_liquidTypeID[4]
                                                            // 17       m_minElevation
                                                            // 18       m_ambient_multiplier
                                                            // 19       m_lightid
    //uint32 unk20;                                         // 20 4.0.0
    //uint32 unk21;                                         // 21 4.0.0
    //uint32 unk22;                                         // 22 4.0.0
    //uint32 unk23;                                         // 23 4.0.0
    //uint32 unk24;                                         // 24 4.0.1, may be worldStateId
};

struct AreaGroupEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  AreaID[6];                                      // 1-6      m_areaID
    uint32  NextAreaID;                                     // 7        m_nextAreaID
};

struct AreaTriggerEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  MapId;                                          // 1        m_ContinentID
    float   PosX;                                              // 2        m_x
    float   PosY;                                              // 3        m_y
    float   PosZ;                                              // 4        m_z
    //uint32                                                // 5
    //uint32                                                // 6
    //uint32                                                // 7
    float   Radius;                                         // 8        m_radius
    float   Box_length;                                          // 9        m_box_length
    float   Box_width;                                          // 10       m_box_width
    float   Box_height;                                          // 11       m_box_heigh
    float   Box_yaw;                                // 12       m_box_yaw
};

struct ArmorLocationEntry
{
  uint32    InventoryType;                                  // 0
  float     Value[5];                                       // 1-5 multiplier for armor types (cloth...plate, no armor?)
};

struct AuctionHouseEntry
{
    uint32    ID;                                      // 0        m_ID
    uint32    FactionID;                                      // 1        m_factionID
    uint32    DepositRate;                               // 2        m_depositRate
    uint32    ConsignmentRate;                                   // 3        m_consignmentRate
    //char*     name;                                       // 4        m_name_lang
};

struct BankBagSlotPricesEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  Cost;                                           // 1        m_Cost
};

struct BarberShopStyleEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  Type;                                           // 1        m_type
    //char*   name;                                         // 2        m_DisplayName_lang
    //uint32  unk_name;                                     // 3        m_Description_lang
    //float   CostMultiplier;                               // 4        m_Cost_Modifier
    uint32  Race;                                           // 5        m_race
    uint32  Sex;                                         // 6        m_sex
    uint32  Data;                                        // 7        m_data (real ID to hair/facial hair)
};

struct BattlemasterListEntry
{
    uint32  ID;                                             // 0        m_ID
    int32   MapID[8];                                       // 1-8      m_mapID[8]
    uint32  InstanceType;                                           // 9        m_instanceType
    //uint32 canJoinAsGroup;                                // 10       m_groupsAllowed
    DBCString Name_lang;                                         // 11       m_name_lang
    uint32 MaxGroupSize;                                    // 12       m_maxGroupSize
    uint32 HolidayWorldState;                             // 13       m_holidayWorldState
    uint32 MinLevel;                                        // 14,      m_minlevel (sync with PvPDifficulty.dbc content)
    uint32 MaxLevel;                                        // 15,      m_maxlevel (sync with PvPDifficulty.dbc content)
    uint32 RatedPlayers;                               // 16       4.0.1
    uint32 MinPlayers;                                      // 17       4.0.6.13596
    uint32 MaxPlayers;                                      // 18       4.0.1
    uint32 Flags;                                           // 19       4.0.3, value 2 for Rated Battlegrounds
};

/*struct Cfg_CategoriesEntry
{
    uint32 Index;                                           //          m_ID categoryId (sent in RealmList packet)
    uint32 Unk1;                                            //          m_localeMask
    uint32 Unk2;                                            //          m_charsetMask
    uint32 IsTournamentRealm;                               //          m_flags
    char *categoryName;                                     //          m_name_lang
}*/

/*struct Cfg_ConfigsEntry
{
    uint32 Id;                                              //          m_ID
    uint32 Type;                                            //          m_realmType (sent in RealmList packet)
    uint32 IsPvp;                                           //          m_playerKillingAllowed
    uint32 IsRp;                                            //          m_roleplaying
};*/

#define MAX_OUTFIT_ITEMS 24

struct CharStartOutfitEntry
{
    // uint32 Id;                                           // 0        m_ID
    uint32 RaceClassGender;                                 // 1        m_raceID m_classID m_sexID m_outfitID (UNIT_FIELD_BYTES_0 & 0x00FFFFFF) comparable (0 byte = race, 1 byte = class, 2 byte = gender)
    int32 ItemId[MAX_OUTFIT_ITEMS];                         // 2-25     m_ItemID
    // int32 ItemDisplayId[MAX_OUTFIT_ITEMS];               // 26-29    m_DisplayItemID not required at server side
    // int32 ItemInventorySlot[MAX_OUTFIT_ITEMS];           // 50-73    m_InventoryType not required at server side
    // uint32 Unknown1;                                     // 74 unique values (index-like with gaps ordered in other way as ids)
    // uint32 Unknown2;                                     // 75
    // uint32 Unknown3;                                     // 76
    //uint32 Unknown4;                                      // 77
    //uint32 Unknown5;                                      // 78
};

struct CharTitlesEntry
{
    uint32  ID;                                             // 0,       m_ID
    // uint32      unk1;                                    // 1        m_Condition_ID
    DBCString Name_lang;                                         // 2        m_name_lang
    //char*       name2;                                    // 3        m_name1_lang
    uint32  Mask_ID;                                      // 4        m_mask_ID used in PLAYER_CHOSEN_TITLE and 1<<index in PLAYER__FIELD_KNOWN_TITLES
    //uint32                                                // 5
};

struct ChatChannelsEntry
{
    uint32  ID;                                      // 0        m_ID
    uint32  Flags;                                          // 1        m_flags
    //uint32                                                // 2        m_factionGroup
    DBCString   Name_lang;                                    // 3        m_name_lang
//    char*   Name_lang[16];                                    // 3        m_name_lang
    //char*       name;                                     // 4        m_shortcut_lang
};

struct ChrClassesEntry
{
    uint32  ID;                                        // 0        m_ID
    uint32  DisplayPower;                                      // 1        m_DisplayPower
                                                            // 2        m_petNameToken
    DBCString Name_lang;                                         // 3        m_name_lang
    //char*       nameFemale;                               // 4        m_name_female_lang
    //char*       nameNeutralGender;                        // 5        m_name_male_lang
    //char*       capitalizedName                           // 6,       m_filename
    uint32  SpellClassSet;                                    // 7        m_spellClassSet
    //uint32 flags2;                                        // 8        m_flags (0x08 HasRelicSlot)
    uint32  CinematicSequenceID;                              // 9        m_cinematicSequenceID
    uint32  Required_expansion;                                      // 10       m_required_expansion
    uint32  AttackPowerPerStrength;                                       // 11       attack power per strength
    uint32  AttackPowerPerAgility;                                       // 12       attack power per agility
    uint32  RangedAttackPowerPerAgility;                                      // 13       ranged attack power per agility
};

struct ChrRacesEntry
{
    uint32      ID;                                     // 0        m_ID
    // 1        m_flags
    uint32      FactionID;                                  // 2        m_factionID
    // 3        m_ExplorationSoundID
    uint32      MaleDisplayID;                                    // 4        m_MaleDisplayId
    uint32      FemaleDisplayID;                                    // 5        m_FemaleDisplayId
    // 6        m_ClientPrefix
    uint32      BaseLanguage;                                     // 7        m_BaseLanguage (7-Alliance 1-Horde)
    // 8        m_creatureType
    // 9        m_ResSicknessSpellID
    // 10       m_SplashSoundID
    // 11       m_clientFileString
    uint32      CinematicSequenceID;                          // 12       m_cinematicSequenceID
    // uint32    unk_322;                                   // 13       m_alliance (0 alliance, 1 horde, 2 not available?)
    DBCString Name_lang;                                         // 14       m_name_lang used for DBC language detection/selection
    //char*       nameFemale;                               // 15       m_name_female_lang
    //char*       nameNeutralGender;                        // 16       m_name_male_lang
                                                            // 17-18    m_facialHairCustomization[2]
                                                            // 19       m_hairCustomization
    uint32      Race_related;                                  // 20       m_required_expansion
    //uint32                                                // 21 (23 for worgens)
    //uint32                                                // 22 4.0.0
    //uint32                                                // 23 4.0.0
};

struct ChrPowerTypesEntry
{
   uint32 ID;                                                  // 0
   uint32 ClassID;                                             // 1
   uint32 PowerType;                                           // 2
};

/*struct CinematicCameraEntry
{
    uint32      id;                                         // 0        m_ID
    char*       filename;                                   // 1        m_model
    uint32      soundid;                                    // 2        m_soundID
    float       start_x;                                    // 3        m_originX
    float       start_y;                                    // 4        m_originY
    float       start_z;                                    // 5        m_originZ
    float       unk6;                                       // 6        m_originFacing
};*/

struct CinematicSequencesEntry
{
    uint32      ID;                                         // 0        m_ID
    // uint32      unk1;                                    // 1        m_soundID
    // uint32      cinematicCamera;                         // 2        m_camera[8]
};

struct CreatureDisplayInfoEntry
{
    uint32      ID;                                  // 0        m_ID
    uint32      ModelID;                                    // 1        m_modelID
                                                            // 2        m_soundID
    uint32      ExtendedDisplayInfoID;                      // 3        m_extendedDisplayInfoID -> CreatureDisplayInfoExtraEntry::ID
    float       CreatureModelScale;                                      // 4        m_creatureModelScale
                                                            // 5        m_creatureModelAlpha
                                                            // 6-8      m_textureVariation[3]
                                                            // 9        m_portraitTextureName
                                                            // 10       m_sizeClass
                                                            // 11       m_bloodID
                                                            // 12       m_NPCSoundID
                                                            // 13       m_particleColorID
                                                            // 14       m_creatureGeosetData
                                                            // 15       m_objectEffectPackageID
                                                            // 16       all 0
};

struct CreatureDisplayInfoExtraEntry
{
    uint32      ID;                                         // 0        m_ID CreatureDisplayInfoEntry::m_extendedDisplayInfoID
    uint32      DisplayRaceID;                              // 1        m_DisplayRaceID
    // uint32    Gender;                                    // 2        m_DisplaySexID
    // uint32    SkinColor;                                 // 3        m_SkinID
    // uint32    FaceType;                                  // 4        m_FaceID
    // uint32    HairType;                                  // 5        m_HairStyleID
    // uint32    HairStyle;                                 // 6        m_HairColorID
    // uint32    BeardStyle;                                // 7        m_FacialHairID
    // uint32    Equipment[11];                             // 8-18     m_NPCItemDisplay equipped static items EQUIPMENT_SLOT_HEAD..EQUIPMENT_SLOT_HANDS, client show its by self
    // uint32    CanEquip;                                  // 19       m_flags 0..1 Can equip additional things when used for players
    // char*                                                // 20       m_BakeName CreatureDisplayExtra-*.blp
};

struct CreatureFamilyEntry
{
    uint32  ID;                                             // 0        m_ID
    float   MinScale;                                       // 1        m_minScale
    uint32  MinScaleLevel;                                  // 2        m_minScaleLevel
    float   MaxScale;                                       // 3        m_maxScale
    uint32  MaxScaleLevel;                                  // 4        m_maxScaleLevel
    uint32  SkillLine[2];                                   // 5-6      m_skillLine
    uint32  PetFoodMask;                                    // 7        m_petFoodMask
    int32   PetTalentType;                                  // 8        m_petTalentType
                                                            // 9        m_categoryEnumID
    DBCString Name_lang;                                         // 10       m_name_lang
                                                            // 11       m_iconFile
};

struct CreatureModelDataEntry
{
    uint32 ID;                                              // 0
    //uint32 Flags;                                         // 1
    //char* ModelPath                                       // 2
    //uint32 InhabitType;                                   // 3 model inhabit type
    //float Scale;                                          // 4 Used in calculation of unit collision data
    //int32 Unk2                                            // 5
    //int32 Unk3                                            // 6
    //uint32 Unk4                                           // 7
    //uint32 Unk5                                           // 8
    //float Unk6                                            // 9
    //uint32 Unk7                                           // 10
    //float Unk8                                            // 11
    //uint32 Unk9                                           // 12
    //uint32 Unk10                                          // 13
    //float CollisionWidth;                                 // 14
    float CollisionHeight;                                  // 15
    float MountHeight;                                      // 16 Used in calculation of unit collision data when mounted
    //float Unks[14]                                        // 17-30
};

#define MAX_CREATURE_SPELL_DATA_SLOT 4

struct CreatureSpellDataEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Spells[MAX_CREATURE_SPELL_DATA_SLOT];        // 1-4      m_spells[4]
    // uint32    availability[MAX_CREATURE_SPELL_DATA_SLOT];// 4-7      m_availability[4]
};

struct CreatureTypeEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*   Name;                                         // 1        m_name_lang
    //uint32    no_expirience;                              // 2        m_flags no exp? critters, non-combat pets, gas cloud.
};

/*struct CurrencyCategoryEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Unk1;                                         // 1        m_flags 0 for known categories and 3 for unknown one (3.0.9)
    char*   Name;                                           // 2        m_name_lang
};*/

struct CurrencyTypesEntry
{
    uint32  ID;                                             // 0
    uint32 CategoryID;                                        // 1
    DBCString Name_lang;                                         // 2
    //char* iconName;                                       // 3
    //char* iconName2;                                      // 4
    //uint32 unk5;                                          // 5
    //uint32 unk6;                                          // 6
    uint32 MaxQty;                                        // 7
    uint32 MaxEarnablePerWeek;                                         // 8
    uint32 Flags;                                           // 9
    //DBCString description;                                // 10

    bool HasPrecision() const   { return Flags & CURRENCY_FLAG_HAS_PRECISION; }
    bool HasSeasonCount() const { return Flags & CURRENCY_FLAG_HAS_SEASON_COUNT; }
    float GetPrecision() const  { return HasPrecision() ? CURRENCY_PRECISION : 1.0f; }
};

struct DestructibleModelDataEntry
{
    uint32 ID;                                              // 0        m_ID
    uint32 State0WMO;                                       // 1
    // uint32 unk2;                                         // 2
    // uint32 unk3;                                         // 3
    // uint32 unk4;                                         // 4
    uint32 State1WMO;                                       // 5
    // uint32 unk6;                                         // 6
    // uint32 unk7;                                         // 7
    // uint32 unk8;                                         // 8
    // uint32 unk9;                                         // 9
    uint32 State2WMO;                                       // 10
    // uint32 unk11;                                        // 11
    // uint32 unk12;                                        // 12
    // uint32 unk13;                                        // 13
    // uint32 unk14;                                        // 14
    uint32 State3WMO;                                       // 15
    // uint32 unk16;                                        // 16
    // uint32 unk17;                                        // 17
    // uint32 unk18;                                        // 18
    // uint32 unk19;                                        // 19
    uint32 RepairGroundFx;                                  // 20
    // uint32 unk21;                                        // 21
    // uint32 unk22;                                        // 22
    // uint32 unk23;                                        // 23
};

struct DungeonEncounterEntry
{
    uint32 ID;                                              // 0        m_ID
    uint32 MapID;                                           // 1        m_mapID
    uint32 Difficulty;                                      // 2        m_difficulty
    uint32 OrderIndex;                                   // 3        m_orderIndex
    uint32 Bit;                                  // 4        m_Bit
    DBCString Name_lang;                                // 5 - encounter name
    //uint32 nameLangFlags;                                 // 6        m_name_lang_flags
    //uint32 spellIconID;                                   // 7        m_spellIconID
};

struct DurabilityCostsEntry
{
    uint32    Itemlvl;                                      // 0        m_ID
    uint32    multiplier[29];                               // 1-29     m_weaponSubClassCost m_armorSubClassCost
};

struct DurabilityQualityEntry
{
    uint32    ID;                                           // 0        m_ID
    float     quality_mod;                                  // 1        m_data
};

struct EmotesEntry
{
    uint32  ID;                                             // 0        m_ID
    //char*   Name;                                         // 1        m_EmoteSlashCommand
    //uint32  AnimationId;                                  // 2        m_AnimID
    uint32  EmoteFlags;                                          // 3        m_EmoteFlags
    uint32  EmoteSpecProc;                                      // 4        m_EmoteSpecProc (determine how emote are shown)
    uint32  EmoteSpecProcParam;                                 // 5        m_EmoteSpecProcParam
    //uint32  SoundId;                                      // 6        m_EventSoundID
    //uint32 unk;                                           // 7 - 4.2.0
};

struct EmotesTextEntry
{
    uint32  ID;                                             //          m_ID
                                                            //          m_name
    uint32  EmoteID;                                         //          m_emoteID
                                                            //          m_emoteText
};

struct FactionEntry
{
    uint32      ID;                                         // 0        m_ID
    int32       ReputationIndex;                           // 1        m_reputationIndex
    uint32      ReputationRaceMask[4];                         // 2-5      m_reputationRaceMask
    uint32      ReputationClassMask[4];                        // 6-9      m_reputationClassMask
    int32       ReputationBase[4];                            // 10-13    m_reputationBase
    uint32      ReputationFlags[4];                         // 14-17    m_reputationFlags
    uint32      ParentFactionID;                                       // 18       m_parentFactionID
    float       ParentFactionMod_0;                            // 19       m_parentFactionMod[2] Faction gains incoming rep * ParentFactionMod_0
    float       ParentFactionMod_1;                           // 20       Faction outputs rep * ParentFactionMod_1 as spillover reputation
    uint32      ParentFactionCap_0;                         // 21       m_parentFactionCap[2] The highest rank the faction will profit from incoming spillover
    //uint32    spilloverRank_unk;                          // 22       It does not seem to be the max standing at which a faction outputs spillover ...so no idea
    DBCString Name_lang;                                         // 23       m_name_lang
    //char*     description;                                // 24       m_description_lang
    //uint32                                                // 25

    // helpers

    int GetIndexFitTo(uint32 raceMask, uint32 classMask) const
    {
        for (int i = 0; i < 4; ++i)
        {
            if ((ReputationRaceMask[i] == 0 || (ReputationRaceMask[i] & raceMask)) &&
                    (ReputationClassMask[i] == 0 || (ReputationClassMask[i] & classMask)))
                return i;
        }

        return -1;
    }
};

struct FactionTemplateEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      Faction;                                    // 1        m_faction
    uint32      Flags;                               // 2        m_flags
    uint32      FactionGroup;                                    // 3        m_factionGroup
    uint32      FriendGroup;                               // 4        m_friendGroup
    uint32      EnemyGroup;                                // 5        m_enemyGroup
    uint32      Enemies[4];                            // 6        m_enemies[4]
    uint32      Friend[4];                           // 10       m_friend[4]
    //-------------------------------------------------------  end structure

    // helpers
    bool IsFriendlyTo(FactionTemplateEntry const& entry) const
    {
        if (entry.Faction)
        {
            for (int i = 0; i < 4; ++i)
                if (Enemies[i]  == entry.Faction)
                {
                    return false;
                }
            for (int i = 0; i < 4; ++i)
                if (Friend[i] == entry.Faction)
                {
                    return true;
                }
        }
        return (FriendGroup & entry.FactionGroup) || (FactionGroup & entry.FriendGroup);
    }
    bool IsHostileTo(FactionTemplateEntry const& entry) const
    {
        if (entry.Faction)
        {
            for (int i = 0; i < 4; ++i)
                if (Enemies[i]  == entry.Faction)
                {
                    return true;
                }
            for (int i = 0; i < 4; ++i)
                if (Friend[i] == entry.Faction)
                {
                    return false;
                }
        }
        return (EnemyGroup & entry.FactionGroup) != 0;
    }
    bool IsHostileToPlayers() const { return (EnemyGroup & FACTION_MASK_PLAYER) != 0; }
    bool IsNeutralToAll() const
    {
        for (int i = 0; i < 4; ++i)
            if (Enemies[i] != 0)
            {
                return false;
            }
        return EnemyGroup == 0 && FriendGroup == 0;
    }
    bool IsContestedGuardFaction() const { return (Flags & FACTION_TEMPLATE_FLAG_CONTESTED_GUARD) != 0; }
};

struct GameObjectDisplayInfoEntry
{
    uint32 ID;                                              // 0 m_ID
    char* ModelName;                                        // 1 m_modelName
    // uint32 unknown2[10];                                 // 2-11 m_Sound
    float GeoBoxMin_0;                                      // 12 m_geoBoxMinX (use first value as interact dist, mostly in hacks way)
    float GeoBoxMin_1;                                      // 13 m_geoBoxMinY
    float GeoBoxMin_2;                                      // 14 m_geoBoxMinZ
    float GeoBoxMax_0;                                      // 15 m_geoBoxMaxX
    float GeoBoxMax_1;                                      // 16 m_geoBoxMaxY
    float GeoBoxMax_2;                                      // 17 m_geoBoxMaxZ
    //uint32 transport;                                     // 18
    //uint32 unk;                                           // 19
    //uint32 unk1;                                          // 20
};

struct GemPropertiesEntry
{
    uint32      ID;                                         // 0        m_id
    uint32      Enchant_ID;                                 // 1        m_enchant_id
                                                            // 2        m_maxcount_inv
                                                            // 3        m_maxcount_item
    uint32      Type;                                       // 4        m_type
    //uint32                                                // 5
};

struct GlyphPropertiesEntry
{
    uint32  ID;                                             //          m_id
    uint32  SpellID;                                        //          m_spellID
    uint32  GlyphSlotFlags;                                      //          m_glyphSlotFlags
    uint32  SpellIconID;                                           //          m_spellIconID
};

struct GlyphSlotEntry
{
    uint32  ID;                                             //          m_id
    uint32  Type;                                      //          m_type
    uint32  Tooltip;                                          //          m_tooltip
};

// All Gt* DBC store data for 100 levels, some by 100 per class/race
#define GT_MAX_LEVEL    100
// gtOCTClassCombatRatingScalar.dbc stores data for 32 ratings, look at MAX_COMBAT_RATING for real used amount
#define GT_MAX_RATING   32

struct GtBarberShopCostBaseEntry
{
    //uint32 level;
    float   Cost;
};

struct GtCombatRatingsEntry
{
    //uint32 level;
    float    ratio;
};

struct GtChanceToMeleeCritBaseEntry
{
    //uint32 level;
    float    base;
};

struct GtChanceToMeleeCritEntry
{
    //uint32 level;
    float    ratio;
};

struct GtChanceToSpellCritBaseEntry
{
    //uint32 level;
    float    base;
};

struct GtChanceToSpellCritEntry
{
    //uint32 level;
    float    ratio;
};

struct GtOCTClassCombatRatingScalarEntry
{
    float    Data;
};

struct GtOCTHpPerStaminaEntry
{
    //uint32 level;
    float    ratio;
};

struct GtRegenMPPerSptEntry
{
    //uint32 level;
    float    ratio;
};

struct GtSpellScalingEntry
{
    //uint32 id;
    float value;
};

struct GtOCTBaseHPByClassEntry
{
    float ratio;
};

struct GtOCTBaseMPByClassEntry
{
    float    ratio;
};

/*struct HolidayDescriptionsEntry
{
    uint32 ID;                                              // 0        m_ID this is NOT holiday id
    //char*     name;                                       // 1        m_name_lang
};*/

/*struct HolidayNamesEntry
{
    uint32 ID;                                              // 0,       m_ID this is NOT holiday id
    //char*     name;                                       // 1        m_name_lang
};*/

struct HolidaysEntry
{
    uint32 ID;                                              // 0        m_ID
    // uint32 duration[10];                                 // 1-10     m_duration
    // uint32 date[26];                                     // 11-36    m_date (dates in unix time starting at January, 1, 2000)
    // uint32 region;                                       // 37       m_region (wow region)
    // uint32 looping;                                      // 38       m_looping
    // uint32 calendarFlags[10];                            // 39-48    m_calendarFlags
    // uint32 holidayNameId;                                // 49       m_holidayNameID (HolidayNames.dbc)
    // uint32 holidayDescriptionId;                         // 50       m_holidayDescriptionID (HolidayDescriptions.dbc)
    // char *textureFilename;                               // 51       m_textureFilename
    // uint32 priority;                                     // 52       m_priority
    // uint32 calendarFilterType;                           // 53       m_calendarFilterType (-1,0,1 or 2)
    // uint32 flags;                                        // 54       m_flags
};

struct ItemArmorQualityEntry
{
  uint32    ID;                                             // 0 item level
  float     Qualitymod[7];                                       // 1-7 multiplier for item quality
  uint32    ItemLevel;                                            // 8 item level
};

struct ItemArmorShieldEntry
{
  uint32    ID;                                             // 0 item level
  uint32    ItemLevel;                                            // 1 item level
  float     Quality[7];                                       // 2-8 multiplier for item quality
};

struct ItemArmorTotalEntry
{
  uint32    Id;                                             // 0 item level
  uint32    Id2;                                            // 1 item level
  float     Value[4];                                       // 2-5 multiplier for armor types (cloth...plate)
};

struct ItemBagFamilyEntry
{
    uint32   ID;                                            // 0        m_ID
    //char*     name;                                       // 1        m_name_lang
};

struct ItemClassEntry
{
    uint32   ID;                                            // 0        m_ID
    uint32 ClassID;                                           // 1
    //uint32 unk2;                                          // 2 looks like second class
    //uint32 unk3;                                          // 3 1 for weapons
    float PriceModifier;                                       // 4
    DBCString ClassName_lang;                                         // 5        m_name_lang
};

struct ItemDisplayInfoEntry
{
    uint32      ID;                                         // 0        m_ID
                                                            // 1        m_modelName[2]
                                                            // 2        m_modelTexture[2]
                                                            // 3        m_inventoryIcon
                                                            // 4        m_geosetGroup[3]
                                                            // 5        m_flags
                                                            // 6        m_spellVisualID
                                                            // 7        m_groupSoundIndex
                                                            // 8        m_helmetGeosetVis[2]
                                                            // 9        m_texture[2]
                                                            // 10       m_itemVisual[8]
                                                            // 11       m_particleColorID
};

// common struct for:
// ItemDamageAmmo.dbc
// ItemDamageOneHand.dbc
// ItemDamageOneHandCaster.dbc
// ItemDamageRanged.dbc
// ItemDamageThrown.dbc
// ItemDamageTwoHand.dbc
// ItemDamageTwoHandCaster.dbc
// ItemDamageWand.dbc
struct ItemDamageEntry
{
  uint32    ID;                                             // 0 item level
  float     Quality[7];                                       // 1-7 multiplier for item quality
  uint32    ItemLevel;                                            // 8 item level
};

struct ItemLimitCategoryEntry
{
    uint32      ID;                                         // 0 Id     m_ID
    //char*     name;                                       // 1        m_name_lang
    uint32      Quantity;                                   // 2,       m_quantity max allowed equipped as item or in gem slot
    uint32      Flags;                                      // 3,       m_flags 0 = have, 1 = equip (enum ItemLimitCategoryMode)
};

struct ItemRandomPropertiesEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*     internalName                                // 1        m_Name
    uint32    Enchantment[5];                                // 2-6      m_Enchantment
    char*     Name_lang;                                   // 7        m_name_lang
};

struct ItemRandomSuffixEntry
{
    uint32    ID;                                           // 0        m_ID
    char*     Name_lang;                                   // 1        m_name_lang
                                                            // 2        m_internalName
    uint32    Enchantment[5];                                // 3-7      m_enchantment
    uint32    AllocationPct[5];                                    // 8-12     m_allocationPct
};

struct ItemReforgeEntry
{
    uint32 ID;                                              // 0
    uint32 Source_stat;                                      // 1
    float Source_multiplier;                                 // 2
    uint32 FinalStat;                                       // 3
    float FinalMultiplier;                                  // 4
};

struct ItemSetEntry
{
    //uint32    id                                          // 0        m_ID
    DBCString Name_lang;                                         // 1        m_name_lang
    //uint32    itemId[17];                                 // 2-18     m_itemID
    uint32    SetSpellID[8];                                    // 19-26    m_setSpellID
    uint32    SetThreshold[8];                     // 27-34    m_setThreshold
    uint32    RequiredSkill;                            // 35       m_requiredSkill
    uint32    RequiredSkillRank;                         // 36       m_requiredSkillRank
};

struct LfgDungeonsEntry
{
    uint32    ID;
    DBCString    Name;
    uint32    minLevel;
    uint32    maxLevel;
    uint32    target_level;
    uint32    target_level_min;
    uint32    target_level_max;
    float    mapID;
    uint32    difficulty;
    uint32    flags;
    uint32    typeID;
    float    faction;
    DBCString    textureFilename;
    uint32    expansionLevel;
    DBCString    order_index;
    uint32    group_id;
    DBCString    description_lang;
    uint32    col17;
    uint32    col18;
    uint32    col19;
    uint32    col20;

    uint32 Entry() const { return ID + ((uint8)typeID << 24); }
};

/*struct LfgDungeonGroupEntry
{
    m_ID
    m_name_lang
    m_order_index
    m_parent_group_id
    m_typeid
};*/

/*struct LfgDungeonExpansionEntry
{
    m_ID
    m_lfg_id
    m_expansion_level
    m_random_id
    m_hard_level_min
    m_hard_level_max
    m_target_level_min
    m_target_level_max
};*/

struct LiquidTypeEntry
{
    uint32 ID;                                              // 0
    //char* Name;                                           // 1
    //uint32 Flags;                                         // 2 Water: 1|2|4|8, Magma: 8|16|32|64, Slime: 2|64|256, WMO Ocean: 1|2|4|8|512
    uint32 SoundBank;                                            // 3 0: Water, 1: Ocean, 2: Magma, 3: Slime
    //uint32 SoundId;                                       // 4 Reference to SoundEntries.dbc
    uint32 SpellID;                                         // 5 Reference to Spell.dbc
    //float MaxDarkenDepth;                                 // 6 Only oceans got values here!
    //float FogDarkenIntensity;                             // 7 Only oceans got values here!
    //float AmbDarkenIntensity;                             // 8 Only oceans got values here!
    //float DirDarkenIntensity;                             // 9 Only oceans got values here!
    //uint32 LightID;                                       // 10 Only Slime (6) and Magma (7)
    //float ParticleScale;                                  // 11 0: Slime, 1: Water/Ocean, 4: Magma
    //uint32 ParticleMovement;                              // 12
    //uint32 ParticleTexSlots;                              // 13
    //uint32 LiquidMaterialID;                              // 14
    //char* Texture[6];                                     // 15-20
    //uint32 Color[2];                                      // 21-22
    //float Unk1[18];                                       // 23-40 Most likely these are attributes for the shaders. Water: (23, TextureTilesPerBlock),(24, Rotation) Magma: (23, AnimationX),(24, AnimationY)
    //uint32 Unk2[4];                                       // 41-44
};

#define MAX_LOCK_CASE 8

struct LockEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      Type[MAX_LOCK_CASE];                        // 1-8      m_Type
    uint32      Index[MAX_LOCK_CASE];                       // 9-16     m_Index
    uint32      Skill[MAX_LOCK_CASE];                       // 17-24    m_Skill
    //uint32      Action[MAX_LOCK_CASE];                    // 25-32    m_Action
};

struct MailTemplateEntry
{
    uint32      ID;                                         // 0        m_ID
    //char*       subject;                                  // 1        m_subject_lang
    DBCString Body_lang;                                    // 2        m_body_lang
};

struct MapEntry
{
    uint32  ID;                                          // 0        m_ID
    DBCString   Directory;                                   // 1        m_Directory
    uint32  InstanceType;                                       // 2        m_InstanceType
    uint32      Flags;                                       // 3        m_Flags (0x100 - CAN_CHANGE_PLAYER_DIFFICULTY)
    uint32      MapType;                                           // 4 4.0.1
    uint32      PVP;                                          // 5        m_PVP 0 or 1 for battlegrounds (not arenas)
    DBCString   MapName_lang;                                           // 6        m_MapName_lang
    uint32      AreaTableID;                                    // 7        m_areaTableID
    DBCString   MapDescription0_lang;                                     // 8        m_MapDescription0_lang
    DBCString   MapDescription1_lang;                                  // 9        m_MapDescription1_lang
    uint32      LoadingScreenID;                                    // 10       m_LoadingScreenID (LoadingScreens.dbc)
    float       MinimapIconScale;                        // 11       m_minimapIconScale
    int32       CorpseMapID;                             // 12       m_corpseMapID map_id of entrance map in ghost mode (continent always and in most cases = normal entrance)
    float       Corpse_0;                               // 13       m_corpseX entrance x coordinate in ghost mode  (in most cases = normal entrance)
    float       Corpse_1;                               // 14       m_corpseY entrance y coordinate in ghost mode  (in most cases = normal entrance)
    uint32      TimeOfDayOverride;                              // 15       m_timeOfDayOverride
    uint32      ExpansionID;                                          // 16       m_expansionID
    uint32      RaidOffset;                                        // 17       m_raidOffset
    uint32      MaxPlayers;                                     // 18       m_maxPlayers
    int32       ParentMapID;                                   // 19       map with base phasing

    // Helpers
    uint32 Expansion() const { return ExpansionID; }

    bool IsDungeon() const { return InstanceType == MAP_INSTANCE || InstanceType == MAP_RAID; }
    bool IsNonRaidDungeon() const { return InstanceType == MAP_INSTANCE; }
    bool Instanceable() const { return InstanceType == MAP_INSTANCE || InstanceType == MAP_RAID || InstanceType == MAP_BATTLEGROUND || InstanceType == MAP_ARENA; }
    bool IsRaid() const { return InstanceType == MAP_RAID; }
    bool IsBattleGround() const { return InstanceType == MAP_BATTLEGROUND; }
    bool IsBattleArena() const { return InstanceType == MAP_ARENA; }
    bool IsBattleGroundOrArena() const { return InstanceType == MAP_BATTLEGROUND || InstanceType == MAP_ARENA; }

    bool IsMountAllowed() const
    {
        return !IsDungeon() ||
               ID == 209 || ID == 269 || ID == 309 || // TanarisInstance, CavernsOfTime, Zul'gurub
               ID == 509 || ID == 534 || ID == 560 || // AhnQiraj, HyjalPast, HillsbradPast
               ID == 568 || ID == 580 || ID == 595 || // ZulAman, Sunwell Plateau, Culling of Stratholme
               ID == 603 || ID == 615 || ID == 616 || // Ulduar, The Obsidian Sanctum, The Eye Of Eternity
               ID == 631 ||                                 // Icecrown Citadel,
               ID == 654 || ID == 655 || ID == 656 || // Gilneas, Gilneas Phase 1, Gilneas Phase 2
               ID == 658 || ID == 720 || ID == 724 || // Pit of Saron, Firelands, Ruby Sanctum
               ID == 644 || ID == 721 || ID == 734 || // Halls of Origination, Firelands, ?????????
               ID == 754 || ID == 755 || ID == 859 || // Throne of Four Winds, Lost City of Tol'Vir, Zul'Gurub
               ID == 861 || ID == 938 || ID == 939 || // Firelands Dailies, End Time, Well of Eternity
               ID == 940 || ID == 962 || ID == 967;   // Hour of Twilight, Gate of Setting Sun, Dragon Soul
    }

    bool IsContinent() const
    {
        return ID == 0 || ID == 1 || ID == 530 || ID == 571;
    }
};

struct MapDifficultyEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      MapID;                                      // 1        m_mapID
    uint32      Difficulty;                                 // 2        m_difficulty (for arenas: arena slot)
    DBCString   Message_lang;                            // 3        m_message_lang (text showed when transfer to map failed)
    uint32      RaidDuration;                                  // 4,       m_raidDuration in secs, 0 if no fixed reset time
    uint32      MaxPlayers;                                 // 5,       m_maxPlayers some heroic versions have 0 when expected same amount as in normal version
    DBCString   Difficultystring;                           // 6        m_difficultystring
};

struct MovieEntry
{
    uint32      ID;                                         // 0        m_ID
    //char*       filename;                                 // 1        m_filename
    //uint32      unk1;                                     // 2        m_volume
    //uint32      unk2;                                     // 3 4.0.0
};

struct MountCapabilityEntry
{
    uint32 ID;
    uint32 Flags;
    uint32 RequiredRidingSkill;
    uint32 RequiredArea;
    uint32 RequiredAura;
    uint32 RequiredSpell;
    uint32 SpeedModSpell;
    int32  RequiredMap;
};

#define MAX_MOUNT_CAPABILITIES 24

struct MountTypeEntry
{
    uint32 ID;
    uint32 Capability[MAX_MOUNT_CAPABILITIES];
};

struct NumTalentsAtLevelEntry
{
    //uint32 Level;                                         // 0 index
    float NumberOfTalents;                                  // 1 talent count
};

#define MAX_OVERRIDE_SPELLS     10

struct OverrideSpellDataEntry
{
    uint32      ID;                                         // 0        m_ID
    uint32      Spells[MAX_OVERRIDE_SPELLS];                // 1-10     m_spells
    // uint32      unk2;                                    // 11       m_flags
    //uint32      unk3;                                     // 12 possibly flag
};

struct PhaseEntry
{
    uint32 Id;                                              // 0
    uint32 PhaseShift;                                      // 1
    uint32 Flags;                                           // 2 - 0x0, 0x4, 0x8
};

struct PowerDisplayEntry
{
    uint32      id;                                         // 0        m_ID
    uint32      power;                                      // 1        m_power
    // uint32   unk1                                        // 2
    // float    unk2                                        // 3
    // float    unk3                                        // 4
    // float    unk4                                        // 5
};

struct PvPDifficultyEntry
{
    //uint32      id;                                       // 0        m_ID
    uint32      MapID;                                      // 1        m_mapID
    uint32      RangeIndex;                                  // 2        m_rangeIndex
    uint32      MinLevel;                                   // 3        m_minLevel
    uint32      MaxLevel;                                   // 4        m_maxLevel
    uint32      Difficulty;                                 // 5        m_difficulty

    // helpers
    BattleGroundBracketId GetBracketId() const { return BattleGroundBracketId(RangeIndex); }
};

struct QuestFactionRewardEntry
{
    uint32      ID;                                         // 0        m_ID
    int32       Difficulty[10];                            // 1-10     m_Difficulty
};

struct QuestSortEntry
{
    uint32      ID;                                         // 0        m_ID
    //char*       name;                                     // 1        m_SortName_lang
};

struct QuestXPLevel
{
    uint32      ID;                                         // 0        m_ID
    uint32      Difficulty[10];                             // 1-10     m_difficulty[10]
};

struct RandomPropertiesPointsEntry
{
    uint32    ID;                                           // 0        m_ItemLevel
    uint32    Epic[5];                                      // 1-5      m_Epic
    uint32    Superior[5];                                  // 6-10     m_Superior
    uint32    Good[5];                                      // 11-15    m_Good
};

struct ScalingStatDistributionEntry
{
    uint32  ID;                                             // 0        m_ID
    int32   StatID[10];                                    // 1-10     m_statID
    uint32  Bonus[10];                                   // 11-20    m_bonus
    //uint32 unk1;                                          // 21
    uint32  Maxlevel;                                       // 22       m_maxlevel
};

struct ScalingStatValuesEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  Charlevel;                                          // 1        m_charlevel
    uint32  dpsMod[6];                                      // 2-7 DPS mod for level
    uint32  SpellPower;                                     // 8 spell power for level
    uint32  StatMultiplier[5];                               // 9-13 Multiplier for ScalingStatDistribution
    uint32  ArmorShoulder[4];                                    // 14-17 Armor for level
    uint32  ArmorChest[4];                                   // 18-21 Armor for level
    //uint32 trash[24];                                     // 22-45
    //uint32 unk2;                                          // 46 unk, probably also Armor for level (flag 0x80000?)

    /*struct ScalingStatValuesEntry
    {
        m_ID
        m_charlevel
        m_shoulderBudget
        m_trinketBudget
        m_weaponBudget1H
        m_rangedBudget
        m_clothShoulderArmor
        m_leatherShoulderArmor
        m_mailShoulderArmor
        m_plateShoulderArmor
        m_weaponDPS1H
        m_weaponDPS2H
        m_spellcasterDPS1H
        m_spellcasterDPS2H
        m_rangedDPS
        m_wandDPS
        m_spellPower
        m_primaryBudget
        m_tertiaryBudget
        m_clothCloakArmor
        m_clothChestArmor
        m_leatherChestArmor
        m_mailChestArmor
        m_plateChestArmor
    };*/
    uint32  getssdMultiplier(uint32 mask) const
    {
        if (mask & 0x4001F)
        {
            if (mask & 0x00000001)
            {
                return StatMultiplier[1];
            }
            if (mask & 0x00000002)
            {
                return StatMultiplier[2]; // 0 and 1 were duplicated
            }
            if (mask & 0x00000004)
            {
                return StatMultiplier[3];
            }
            if (mask & 0x00000008)
            {
                return StatMultiplier[0];
            }
            if (mask & 0x00000010)
            {
                return StatMultiplier[4];
            }
            if (mask & 0x00040000)
            {
                return StatMultiplier[2]; // 4.0.0
            }
        }
        return 0;
    }

    uint32  getArmorMod(uint32 mask) const
    {
        if (mask & 0x00F001E0)
        {
            if (mask & 0x00000020)
            {
                return ArmorShoulder[0];
            }
            if (mask & 0x00000040)
            {
                return ArmorShoulder[1];
            }
            if (mask & 0x00000080)
            {
                return ArmorShoulder[2];
            }
            if (mask & 0x00000100)
            {
                return ArmorShoulder[3];
            }
            if (mask & 0x00100000)
            {
                return ArmorChest[0];     // cloth
            }
            if (mask & 0x00200000)
            {
                return ArmorChest[1];     // leather
            }
            if (mask & 0x00400000)
            {
                return ArmorChest[2];     // mail
            }
            if (mask & 0x00800000)
            {
                return ArmorChest[3];     // plate
            }
        }
        return 0;
    }

    uint32 getDPSMod(uint32 mask) const
    {
        if (mask & 0x7E00)
        {
            if (mask & 0x00000200)
            {
                return dpsMod[0];
            }
            if (mask & 0x00000400)
            {
                return dpsMod[1];
            }
            if (mask & 0x00000800)
            {
                return dpsMod[2];
            }
            if (mask & 0x00001000)
            {
                return dpsMod[3];
            }
            if (mask & 0x00002000)
            {
                return dpsMod[4];
            }
            if (mask & 0x00004000)
            {
                return dpsMod[5];        // not used?
            }
        }
        return 0;
    }

    uint32 getSpellBonus(uint32 mask) const
    {
        if (mask & 0x00008000)
        {
            return SpellPower;
        }
        return 0;
    }
};

/*struct SkillLineCategoryEntry{
    uint32    id;                                         // 0      m_ID
    char*     name;                                       // 1      m_name_lang
    uint32    displayOrder;                               // 2      m_sortIndex
};*/

struct SkillRaceClassInfoEntry
{
    //uint32    id;                                         // 0        m_ID
    uint32    SkillID;                                      // 1        m_skillID
    uint32    RaceMask;                                     // 2        m_raceMask
    uint32    ClassMask;                                    // 3        m_classMask
    uint32    Flags;                                        // 4        m_flags
    uint32    Availability;                                     // 5        m_minLevel
    //uint32    skillTierId;                                // 6        m_skillTierID
    //uint32    skillCostID;                                // 7        m_skillCostIndex
    //uint32 Unk;                                           // 8
};

/*struct SkillTiersEntry{
    uint32    id;                                           // 0        m_ID
    uint32    skillValue[16];                               // 1-17     m_cost
    uint32    maxSkillValue[16];                            // 18-3     m_valueMax
};*/

struct SkillLineEntry
{
    uint32    ID;                                           // 0        m_ID
    int32     CategoryID;                                   // 1        m_categoryID
    DBCString DisplayName_lang;                                         // 2        m_displayName_lang
    //DBCString description;                                // 3        m_description_lang
    uint32    SpellIconID;                                    // 4        m_spellIconID
    //DBCString alternateVerb;                              // 5        m_alternateVerb_lang
    uint32    CanLink;                                      // 6        m_canLink (prof. with recipes)
};

struct SkillLineAbilityEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    SkillLine;                                      // 1        m_skillLine
    uint32    Spell;                                      // 2        m_spell
    uint32    RaceMask;                                     // 3        m_raceMask
    uint32    ClassMask;                                    // 4        m_classMask
    //uint32    racemaskNot;                                // 5        m_excludeRace
    //uint32    classmaskNot;                               // 6        m_excludeClass
    uint32    MinSkillLineRank;                              // 7        m_minSkillLineRank
    uint32    SupercededBySpell;                              // 8        m_supercededBySpell
    uint32    AcquireMethod;                              // 9        m_acquireMethod
    uint32    TrivialSkillLineRankHigh;                                    // 10       m_trivialSkillLineRankHigh
    uint32    TrivialSkillLineRankLow;                                    // 11       m_trivialSkillLineRankLow
    uint32    NumSkillUps;                              // 12       4.0.0
    //uint32                                                // 13       4.0.0
};

struct SoundEntriesEntry
{
    uint32          Id;                                     // 0        m_ID
    uint32          Type;                                   // 1        m_soundType
    DBCString       InternalName;                           // 2        m_name
    DBCString       FileName[10];                           // 3-12     m_File[10]
    DBCString       Unk13[10];                              // 13-22    m_Freq[10]
    DBCString       Path;                                   // 23       m_DirectoryBase
                                                            // 24       m_volumeFloat
                                                            // 25       m_flags
                                                            // 26       m_minDistance
                                                            // 27       m_distanceCutoff
                                                            // 28       m_EAXDef
                                                            // 29       m_soundEntriesAdvancedID, new in 3.1
    //unk                                                   // 30       4.0.0
    //unk                                                   // 31       4.0.0
    //unk                                                   // 32       4.0.0
    //unk                                                   // 33       4.0.0
    //unk                                                   // 34       4.3.0
};

struct ClassFamilyMask
{
    uint64 Flags;
    uint32 Flags2;

    ClassFamilyMask() : Flags(0), Flags2(0) {}
    explicit ClassFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) : Flags(familyFlags), Flags2(familyFlags2) {}

    bool Empty() const { return Flags == 0 && Flags2 == 0; }
    bool operator! () const { return Empty(); }
    operator void const* () const { return Empty() ? NULL : this; }// for allow normal use in if (mask)

    bool IsFitToFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        return (Flags & familyFlags) || (Flags2 & familyFlags2);
    }

    bool IsFitToFamilyMask(ClassFamilyMask const& mask) const
    {
        return (Flags & mask.Flags) || (Flags2 & mask.Flags2);
    }

    uint64 operator& (uint64 mask) const                     // possible will removed at finish convertion code use IsFitToFamilyMask
    {
        return Flags & mask;
    }

    ClassFamilyMask& operator|= (ClassFamilyMask const& mask)
    {
        Flags |= mask.Flags;
        Flags2 |= mask.Flags2;
        return *this;
    }
};

#define MAX_SPELL_REAGENTS 8
#define MAX_SPELL_TOTEMS 2
#define MAX_SPELL_TOTEM_CATEGORIES 2

// SpellAuraOptions.dbc
struct SpellAuraOptionsEntry
{
    //uint32    Id;                                         // 0       m_ID
    uint32    CumulativeAura;                                  // 1       m_cumulativeAura
    uint32    ProcChance;                                   // 2       m_procChance
    uint32    ProcCharges;                                  // 3       m_procCharges
    uint32    ProcTypeMask;                                    // 4       m_procTypeMask
};

// SpellAuraRestrictions.dbc
struct SpellAuraRestrictionsEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    CasterAuraState;                              // 1        m_casterAuraState
    uint32    TargetAuraState;                              // 2        m_targetAuraState
    uint32    ExcludeCasterAuraState;                           // 3        m_excludeCasterAuraState
    uint32    ExcludeTargetAuraState;                           // 4        m_excludeTargetAuraState
    uint32    CasterAuraSpell;                              // 5        m_casterAuraSpell
    uint32    TargetAuraSpell;                              // 6        m_targetAuraSpell
    uint32    ExcludeCasterAuraSpell;                       // 7        m_excludeCasterAuraSpell
    uint32    ExcludeTargetAuraSpell;                       // 8        m_excludeTargetAuraSpell
};

// SpellCastingRequirements.dbc
struct SpellCastingRequirementsEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    FacingCasterFlags;                            // 1        m_facingCasterFlags
    //uint32    MinFactionId;                               // 2        m_minFactionID not used
    //uint32    MinReputation;                              // 3        m_minReputation not used
    int32     RequiredAreasID;                                  // 4        m_requiredAreaGroupId
    //uint32    RequiredAuraVision;                         // 5        m_requiredAuraVision not used
    uint32    RequiresSpellFocus;                           // 6        m_requiresSpellFocus
};

// SpellCastTimes.dbc
struct SpellCastTimesEntry
{
    uint32    ID;                                           // 0        m_ID
    int32     CastTime;                                     // 1        m_base
    float     CastTimePerLevel;                             // 2        m_perLevel
    int32     MinCastTime;                                  // 3        m_minimum
};

// SpellCategories.dbc
struct SpellCategoriesEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    Category;                                     // 1        m_category
    uint32    DefenseType;                                     // 2        m_defenseType
    uint32    DispelType;                                       // 3        m_dispelType
    uint32    Mechanic;                                     // 4        m_mechanic
    uint32    PreventionType;                               // 5        m_preventionType
    uint32    StartRecoveryCategory;                        // 6        m_startRecoveryCategory
};

// SpellClassOptions.dbc
struct SpellClassOptionsEntry
{
    //uint32    Id;                                         // 0        m_ID
    //uint32    modalNextSpell;                             // 1        m_modalNextSpell not used
    ClassFamilyMask SpellClassMask;                       // 2-4      m_spellClassMask NOTE: size is 12 bytes!!!
    uint32    SpellClassSet;                              // 5        m_spellClassSet
    //char*   Description;                                  // 6 4.0.0
    // helpers

    bool IsFitToFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        return SpellClassMask.IsFitToFamilyMask(familyFlags, familyFlags2);
    }

    bool IsFitToFamily(SpellFamily family, uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        return SpellFamily(SpellClassSet) == family && IsFitToFamilyMask(familyFlags, familyFlags2);
    }

    bool IsFitToFamilyMask(ClassFamilyMask const& mask) const
    {
        return SpellClassMask.IsFitToFamilyMask(mask);
    }

    bool IsFitToFamily(SpellFamily family, ClassFamilyMask const& mask) const
    {
        return SpellFamily(SpellClassSet) == family && IsFitToFamilyMask(mask);
    }

    private:
        // catch wrong uses
        template<typename T>
        bool IsFitToFamilyMask(SpellFamily family, T t) const;
};

// SpellCooldowns.dbc
struct SpellCooldownsEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    CategoryRecoveryTime;                         // 1        m_categoryRecoveryTime
    uint32    RecoveryTime;                                 // 2        m_recoveryTime
    uint32    StartRecoveryTime;                            // 3        m_startRecoveryTime
};

// SpellEffect.dbc
struct SpellEffectEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    Effect;                                       // 1        m_effect
    float     EffectAmplitude;                          // 2        m_effectAmplitude
    uint32    EffectAura;                          // 3        m_effectAura
    uint32    EffectAuraPeriod;                              // 4        m_effectAuraPeriod
    int32     EffectBasePoints;                             // 5        m_effectBasePoints (don't must be used in spell/auras explicitly, must be used cached Spell::m_currentBasePoints)
    float     EffectBonusCoefficient;                        // 6        m_effectBonus
    float     EffectChainAmplitude;                       // 7        m_effectChainAmplitude
    uint32    EffectChainTargets;                            // 8        m_effectChainTargets
    int32     EffectDieSides;                               // 9        m_effectDieSides
    uint32    EffectItemType;                               // 10       m_effectItemType
    uint32    EffectMechanic;                               // 11       m_effectMechanic
    int32     EffectMiscValue_0;                              // 12       m_effectMiscValue
    int32     EffectMiscValue_1;                             // 13       m_effectMiscValueB
    float     EffectPointsPerResource;                    // 14       m_effectPointsPerCombo
    uint32    EffectRadiusIndex_0;                            // 15       m_effectRadiusIndex - spellradius.dbc
    uint32    EffectRadiusIndex_1;                         // 16       4.0.0
    float     EffectRealPointsPerLevel;                     // 17       m_effectRealPointsPerLevel
    ClassFamilyMask EffectSpellClassMask;                   // 18 19 20 m_effectSpellClassMask
    uint32    EffectTriggerSpell;                           // 21       m_effectTriggerSpell
    uint32    ImplicitTarget_0;                        // 22       m_implicitTargetA
    uint32    ImplicitTarget_1;                        // 23       m_implicitTargetB
    uint32    SpellID;                                // 24       m_spellId - spell.dbc
    uint32    EffectIndex;                                  // 25       m_spellEffectIdx
    //uint32 unk;                                           // 26       4.2.0 only 0 or 1

    // helpers

    int32 CalculateSimpleValue() const { return EffectBasePoints; }

    uint32 GetRadiusIndex() const
    {
        if (EffectRadiusIndex_0 != 0)
        {
            return EffectRadiusIndex_0;
        }

        return EffectRadiusIndex_1;
    }
};

// SpellEquippedItems.dbc
struct SpellEquippedItemsEntry
{
    //uint32    Id;                                         // 0        m_ID
    int32     EquippedItemClass;                            // 1        m_equippedItemClass (value)
    int32     EquippedItemInvTypes;                // 2        m_equippedItemInvTypes (mask)
    int32     EquippedItemSubclass;                     // 3        m_equippedItemSubclass (mask)
};

// SpellFocusObject.dbc
struct SpellFocusObjectEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*     Name;                                       // 1        m_name_lang
};

// SpellInterrupts.dbc
struct SpellInterruptsEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    AuraInterruptFlags_0;                           // 1        m_auraInterruptFlags
    //uint32                                                // 2        4.0.0
    uint32    ChannelInterruptFlags_0;                        // 3        m_channelInterruptFlags
    //uint32                                                // 4        4.0.0
    uint32    InterruptFlags;                               // 5        m_interruptFlags
};

// SpellItemEnchantment.dbc
struct SpellItemEnchantmentEntry
{
    uint32      ID;                                         // 0        m_ID
    //uint32      charges;                                  // 1        m_charges
    uint32      Effect[3];                                    // 2-4      m_effect[3]
    uint32      EffectPointsMin[3];                                  // 5-7      m_effectPointsMin[3]
    //uint32      amount2[3]                                // 8-10     m_effectPointsMax[3]
    uint32      EffectArg[3];                                 // 11-13    m_effectArg[3]
    DBCString Name_lang;                                  // 14       m_name_lang
    uint32      ItemVisual;                                    // 15       m_itemVisual
    uint32      Flags;                                       // 16       m_flags
    uint32      Src_itemID;                                      // 17       m_src_itemID
    uint32      Condition_ID;                       // 18       m_condition_id
    uint32      RequiredSkillID;                              // 19       m_requiredSkillID
    uint32      RequiredSkillRank;                         // 20       m_requiredSkillRank
    uint32      MinLevel;                              // 21       m_requiredLevel - 3.1
                                                            // 22       new in 3.1
};

// SpellItemEnchantmentCondition.dbc
struct SpellItemEnchantmentConditionEntry
{
    uint32  ID;                                             // 0        m_ID
    uint8   Color[5];                                       // 1-5      m_lt_operandType[5]
    //uint32  LT_Operand[5];                                // 6-10     m_lt_operand[5]
    uint8   Comparator[5];                                  // 11-15    m_operator[5]
    uint8   CompareColor[5];                                // 15-20    m_rt_operandType[5]
    uint32  Value[5];                                       // 21-25    m_rt_operand[5]
    //uint8   Logic[5]                                      // 25-30    m_logic[5]
};

// SpellLevels.dbc
struct SpellLevelsEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    BaseLevel;                                    // 1        m_baseLevel
    uint32    MaxLevel;                                     // 2        m_maxLevel
    uint32    SpellLevel;                                   // 3        m_spellLevel
};

// SpellPower.dbc
struct SpellPowerEntry
{
    //uint32    Id;                                         // 0 - m_ID
    uint32    ManaCost;                                     // 1 - m_manaCost
    uint32    ManaCostPerLevel;                             // 2 - m_manaCostPerLevel
    uint32    PowerCost;                           // 3 - m_manaCostPct
    uint32    ManaPerSecond;                                // 4 - m_manaPerSecond
    uint32    PowerDisplayID;                        // 5   m_manaPerSecondPerLevel
    //uint32  PowerDisplayId;                               // 6 - m_powerDisplayID - id from PowerDisplay.dbc, new in 3.1
    float     PowerCostPct;                      // 7   4.3.0
};

// SpellRadius.dbc
struct SpellRadiusEntry
{
    uint32    ID;                                           // 0        m_ID
    float     Radius;                                       // 1        m_radius
    float     RadiusPerLevel;                               // 2        m_radiusPerLevel
    float     RadiusMax;                                    // 3        m_radiusMax
};

// SpellRange.dbc
struct SpellRangeEntry
{
    uint32    ID;                                           // 0        m_ID
    float     RangeMin_0;                                     // 1        m_rangeMin[2]
    float     RangeMin_1;                             // 2
    float     RangeMax_0;                                     // 3        m_rangeMax[2]
    float     RangeMax_1;                             // 4
    uint32    Flags;                                         // 5        m_flags
    //char*   Name;                                         // 6-21     m_displayName_lang
    //char*   ShortName;                                    // 23-38    m_displayNameShort_lang
};

// SpellReagents.dbc
struct SpellReagentsEntry
{
    //uint32    Id;                                         // 0        m_ID
    int32     Reagent[MAX_SPELL_REAGENTS];                  // 54-61    m_reagent
    uint32    ReagentCount[MAX_SPELL_REAGENTS];             // 62-69    m_reagentCount
};

// SpellRuneCost.dbc
struct SpellRuneCostEntry
{
    uint32  ID;                                             // 0        m_ID
    uint32  RuneCost[3];                                    // 1-3      m_blood m_unholy m_frost (0=blood, 1=frost, 2=unholy)
    uint32  runePowerGain;                                  // 4        m_runicPower

    bool NoRuneCost() const { return RuneCost[0] == 0 && RuneCost[1] == 0 && RuneCost[2] == 0; }
    bool NoRunicPowerGain() const { return runePowerGain == 0; }
};

// SpellScaling.dbc
struct SpellScalingEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    CastTimeMin;                                  // 1
    uint32    CastTimeMax;                                  // 2
    uint32    CastTimeMaxLevel;                          // 3
    uint32    Class;                                  // 4        (index * 100) + charLevel => gtSpellScaling.dbc
    float     Coefficient[3];                                    // 5-7
    float     Variance[3];                                    // 8-10
    float     ComboPointsCoefficient[3];                                    // 11-13
    float     NerfFactor;                                     // 14       some coefficient, mostly 1.0f
    uint32    NerfMaxLevel;                                // 15       some level

    bool IsScalableEffect(SpellEffectIndex i) const { return Coefficient[i] != 0.0f; };
};

// SpellShapeshift.dbc
struct SpellShapeshiftEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    ShapeshiftExclude_0;                                   // 1        m_shapeshiftMask
    // uint32 unk_320_2;                                    // 2        3.2.0
    uint32    ShapeshiftMask_0;                                      // 3        m_shapeshiftExclude
    // uint32 unk_320_3;                                    // 4        3.2.0
    // uint32    StanceBarOrder;                            // 5        m_stanceBarOrder not used
};

// SpellShapeshiftForm.dbc
struct SpellShapeshiftFormEntry
{
    uint32 ID;                                              // 0        m_ID
    //uint32 buttonPosition;                                // 1        m_bonusActionBar
    //char*  Name;                                          // 2        m_name_lang
    uint32 Flags;                                          // 3        m_flags
    int32  CreatureType;                                    // 4        m_creatureType <=0 humanoid, other normal creature types
    //uint32 unk1;                                          // 5        m_attackIconID
    uint32 CombatRoundTime;                                     // 6        m_combatRoundTime
    uint32 CreatureDisplayID_0;                                       // 7        m_creatureDisplayID[4]
    uint32 CreatureDisplayID_1;                                       // 8
    //uint32 unk3;                                          // 9 unused always 0
    //uint32 unk4;                                          // 10 unused always 0
    uint32 PresetSpellID[8];                                      // 11-18    m_presetSpellID[8]
    //uint32 unk5;                                          // 19 unused, !=0 for flight forms
    //uint32 unk6;                                          // 20
};

// SpellTargetRestrictions.dbc
struct SpellTargetRestrictionsEntry
{
    //uint32    Id;                                         // 0        m_ID
    float     MaxTargetRadius;                              // 1 - m_maxTargetRadius
    uint32    MaxAffectedTargets;                           // 1 - m_maxTargets
    uint32    MaxTargetLevel;                               // 2 - m_maxTargetLevel
    uint32    TargetCreatureType;                           // 3 - m_targetCreatureType
    uint32    Targets;                                      // 4 - m_targets
};

// SpellTotems.dbc
struct SpellTotemsEntry
{
    //uint32    Id;                                         // 0        m_ID
    uint32    RequiredTotemCategoryID[MAX_SPELL_TOTEM_CATEGORIES];    // 1 2      m_requiredTotemCategoryID
    uint32    Totem[MAX_SPELL_TOTEMS];                      // 3 4      m_totem
};

// Spell.dbc
struct  SpellEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Attributes;                                   // 1        m_attribute
    uint32    AttributesEx;                                 // 2        m_attributesEx
    uint32    AttributesExB;                                // 3        m_attributesExB
    uint32    AttributesExC;                                // 4        m_attributesExC
    uint32    AttributesExD;                                // 5        m_attributesExD
    uint32    AttributesExE;                                // 6        m_attributesExE
    uint32    AttributesExF;                                // 7        m_attributesExF
    uint32    AttributesExG;                                // 8        m_attributesExG (0x20 - totems, 0x4 - paladin auras, etc...)
    uint32    AttributesExH;                                // 9        m_attributesExH
    uint32    AttributesExI;                                // 10       m_attributesExI
    uint32    AttributesExJ;                               // 11       m_attributesExJ
    uint32    CastingTimeIndex;                             // 12       m_castingTimeIndex
    uint32    DurationIndex;                                // 13       m_durationIndex
    uint32    PowerType;                                    // 14       m_powerType
    uint32    RangeIndex;                                   // 15       m_rangeIndex
    float     Speed;                                        // 16       m_speed
    uint32    SpellVisualID[2];                               // 17-18    m_spellVisualID
    uint32    SpellIconID;                                  // 19       m_spellIconID
    uint32    ActiveIconID;                                 // 20       m_activeIconID
    DBCString Name_lang;                                    // 21       m_name_lang
    DBCString NameSubtext_lang;                                         // 22       m_nameSubtext_lang
    //DBCString Description;                                // 23       m_description_lang not used
    //DBCString ToolTip;                                    // 24       m_auraDescription_lang not used
    uint32    SchoolMask;                                   // 25       m_schoolMask
    uint32    RuneCostID;                                   // 26       m_runeCostID
    //uint32    spellMissileID;                             // 27       m_spellMissileID not used
    //uint32  spellDescriptionVariableID;                   // 28       m_spellDescriptionVariableID, 3.2.0
    uint32  Difficulty;                              // 29       m_spellDifficultyID - id from SpellDifficulty.dbc
    //float unk_f1;                                         // 30
    uint32 ScalingID;                                  // 31       SpellScaling.dbc
    uint32 AuraOptionsID;                              // 32       SpellAuraOptions.dbc
    uint32 AuraRestrictionsID;                         // 33       SpellAuraRestrictions.dbc
    uint32 CastingRequirementsID;                      // 34       SpellCastingRequirements.dbc
    uint32 CategoriesID;                               // 35       SpellCategories.dbc
    uint32 ClassOptionsID;                             // 36       SpellClassOptions.dbc
    uint32 CooldownsID;                                // 37       SpellCooldowns.dbc
    //uint32 unkIndex7;                                     // 38       all zeros...
    uint32 EquippedItemsID;                            // 39       SpellEquippedItems.dbc
    uint32 InterruptsID;                               // 40       SpellInterrupts.dbc
    uint32 LevelsID;                                   // 41       SpellLevels.dbc
    uint32 PowerDisplayID;                                    // 42       SpellPower.dbc
    uint32 ReagentsID;                                 // 43       SpellReagents.dbc
    uint32 ShapeshiftID;                               // 44       SpellShapeshift.dbc
    uint32 TargetRestrictionsID;                       // 45       SpellTargetRestrictions.dbc
    uint32 TotemsID;                                   // 46       SpellTotems.dbc
    //uint32 ResearchProject;                               // 47       ResearchProject.dbc

    // helpers
    int32 CalculateSimpleValue(SpellEffectIndex eff) const;
    ClassFamilyMask const& GetEffectSpellClassMask(SpellEffectIndex eff) const;

    // struct access functions
    SpellAuraOptionsEntry const* GetSpellAuraOptions() const;
    SpellAuraRestrictionsEntry const* GetSpellAuraRestrictions() const;
    SpellCastingRequirementsEntry const* GetSpellCastingRequirements() const;
    SpellCategoriesEntry const* GetSpellCategories() const;
    SpellClassOptionsEntry const* GetSpellClassOptions() const;
    SpellCooldownsEntry const* GetSpellCooldowns() const;
    SpellEffectEntry const* GetSpellEffect(SpellEffectIndex eff) const;
    SpellEquippedItemsEntry const* GetSpellEquippedItems() const;
    SpellInterruptsEntry const* GetSpellInterrupts() const;
    SpellLevelsEntry const* GetSpellLevels() const;
    SpellPowerEntry const* GetSpellPower() const;
    SpellReagentsEntry const* GetSpellReagents() const;
    SpellScalingEntry const* GetSpellScaling() const;
    SpellShapeshiftEntry const* GetSpellShapeshift() const;
    SpellTargetRestrictionsEntry const* GetSpellTargetRestrictions() const;
    SpellTotemsEntry const* GetSpellTotems() const;

    // single fields
    uint32 GetManaCost() const;
    uint32 GetPreventionType() const;
    uint32 GetCategory() const;
    uint32 GetStartRecoveryTime() const;
    uint32 GetMechanic() const;
    uint32 GetRecoveryTime() const;
    uint32 GetCategoryRecoveryTime() const;
    uint32 GetStartRecoveryCategory() const;
    uint32 GetSpellLevel() const;
    int32 GetEquippedItemClass() const;
    SpellFamily GetSpellFamilyName() const;
    uint32 GetDmgClass() const;
    uint32 GetDispel() const;
    uint32 GetMaxAffectedTargets() const;
    uint32 GetStackAmount() const;
    uint32 GetManaCostPercentage() const;
    uint32 GetProcCharges() const;
    uint32 GetProcChance() const;
    uint32 GetMaxLevel() const;
    uint32 GetTargetAuraState() const;
    uint32 GetManaPerSecond() const;
    uint32 GetRequiresSpellFocus() const;
    uint32 GetSpellEffectIdByIndex(SpellEffectIndex index) const;
    uint32 GetAuraInterruptFlags() const;
    uint32 GetEffectImplicitTargetAByIndex(SpellEffectIndex index) const;
    int32 GetAreaGroupId() const;
    uint32 GetFacingCasterFlags() const;
    uint32 GetBaseLevel() const;
    uint32 GetInterruptFlags() const;
    uint32 GetTargetCreatureType() const;
    int32 GetEffectMiscValue(SpellEffectIndex index) const;
    uint32 GetStances() const;
    uint32 GetStancesNot() const;
    uint32 GetProcFlags() const;
    uint32 GetChannelInterruptFlags() const;
    uint32 GetManaCostPerLevel() const;
    uint32 GetCasterAuraState() const;
    uint32 GetTargets() const;
    uint32 GetEffectApplyAuraNameByIndex(SpellEffectIndex index) const;

    bool IsFitToFamilyMask(uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        SpellClassOptionsEntry const* classOpt = GetSpellClassOptions();
        return classOpt && classOpt->IsFitToFamilyMask(familyFlags, familyFlags2);
    }

    bool IsFitToFamily(SpellFamily family, uint64 familyFlags, uint32 familyFlags2 = 0) const
    {
        SpellClassOptionsEntry const* classOpt = GetSpellClassOptions();
        return classOpt && classOpt->IsFitToFamily(family, familyFlags, familyFlags2);
    }

    bool IsFitToFamilyMask(ClassFamilyMask const& mask) const
    {
        SpellClassOptionsEntry const* classOpt = GetSpellClassOptions();
        return classOpt && classOpt->IsFitToFamilyMask(mask);
    }

    bool IsFitToFamily(SpellFamily family, ClassFamilyMask const& mask) const
    {
        SpellClassOptionsEntry const* classOpt = GetSpellClassOptions();
        return classOpt && classOpt->IsFitToFamily(family, mask);
    }

        inline bool HasAttribute(SpellAttributes attribute) const { return Attributes & attribute; }
        inline bool HasAttribute(SpellAttributesEx attribute) const { return AttributesEx & attribute; }
        inline bool HasAttribute(SpellAttributesEx2 attribute) const { return AttributesExB & attribute; }
        inline bool HasAttribute(SpellAttributesEx3 attribute) const { return AttributesExC & attribute; }
        inline bool HasAttribute(SpellAttributesEx4 attribute) const { return AttributesExD & attribute; }
        inline bool HasAttribute(SpellAttributesEx5 attribute) const { return AttributesExE & attribute; }
        inline bool HasAttribute(SpellAttributesEx6 attribute) const { return AttributesExF & attribute; }
        inline bool HasAttribute(SpellAttributesEx7 attribute) const { return AttributesExG & attribute; }
        inline bool HasAttribute(SpellAttributesEx8 attribute) const { return AttributesExH & attribute; }
        inline bool HasAttribute(SpellAttributesEx9 attribute) const { return AttributesExI & attribute; }
        inline bool HasAttribute(SpellAttributesEx10 attribute) const { return AttributesExJ & attribute; }

    private:
        // prevent creating custom entries (copy data from original in fact)
        SpellEntry(SpellEntry const&);                      // DON'T must have implementation

        // catch wrong uses
        template<typename T>
        bool IsFitToFamilyMask(SpellFamily family, T t) const;
};

// A few fields which are required for automated convertion
// NOTE that these fields are count by _skipping_ the fields that are unused!
#define LOADED_SPELLDBC_FIELD_POS_EQUIPPED_ITEM_CLASS  65   // Must be converted to -1
#define LOADED_SPELLDBC_FIELD_POS_SPELLNAME_0          132  // Links to "MaNGOS server-side spell"

struct SpellDifficultyEntry
{
    uint32 ID;                                              // 0        m_ID
    uint32 DifficultySpellID[MAX_DIFFICULTY];                         // 1-4      m_difficultySpellID[4]
};

struct SpellDurationEntry
{
    uint32    ID;                                           //          m_ID
    int32     Duration[3];                                  //          m_duration, m_durationPerLevel, m_maxDuration
};

struct SummonPropertiesEntry
{
    uint32  ID;                                             // 0        m_id
    uint32  Control;                                          // 1        m_control (enum SummonPropGroup)
    uint32  Faction;                                      // 2        m_faction
    uint32  Title;                                          // 3        m_title (enum UnitNameSummonTitle)
    uint32  Slot;                                           // 4        m_slot if title = UNITNAME_SUMMON_TITLE_TOTEM, its actual slot (0-6).
                                                            //      if title = UNITNAME_SUMMON_TITLE_COMPANION, slot=6 -> defensive guardian, in other cases criter/minipet
                                                            //      Slot may have other uses, selection of pet type in some cases?
    uint32  Flags;                                          // 5        m_flags (enum SummonPropFlags)
};

#define MAX_TALENT_RANK 5
#define MAX_PET_TALENT_RANK 3                               // use in calculations, expected <= MAX_TALENT_RANK
#define MAX_TALENT_TABS 3

struct TalentEntry
{
    uint32    ID;                                     // 0        m_ID
    uint32    TabID;                                    // 1        m_tabID (TalentTab.dbc)
    uint32    TierID;                                          // 2        m_tierID
    uint32    ColumnIndex;                                          // 3        m_columnIndex
    uint32    SpellRank[MAX_TALENT_RANK];                      // 4-6      m_spellRank
    uint32    PrereqTalent_0;                                    // 9        m_prereqTalent (Talent.dbc)
                                                            // 10-11 part of prev field
    uint32    PrereqRank_0;                                // 12       m_prereqRank
                                                            // 13-14 part of prev field
    //uint32  needAddInSpellBook;                           // 15       m_flags also need disable higest ranks on reset talent tree
    //uint32  unk1;                                         // 16       m_requiredSpellID
    //uint64  allowForPet;                                  // 17       m_categoryMask its a 64 bit mask for pet 1<<m_categoryEnumID in CreatureFamily.dbc
};

struct TalentTabEntry
{
    uint32  ID;                                             // 0        m_ID
    //char* name;                                           // 1        m_name_lang
    //unit32  spellicon;                                    // 2        m_spellIconID
    uint32  ClassMask;                                      // 3        m_classMask
    uint32  PetTalentMask;                                  // 4        m_petTalentMask
    uint32  OrderIndex;                                     // 5        m_orderIndex
    //char* internalname;                                   // 6        m_backgroundFile
    //char* description;                                    // 7
    uint32 RoleMask;                                        // 8        4.0.0
    uint32 MasterySpellID[MAX_MASTERY_SPELLS];              // 9-10     passive mastery bonus spells
};

struct TalentTreePrimarySpellsEntry
{
    //uint32 Id;                                            // 0 index
    uint32 TalentTabID;                                      // 1 entry from TalentTab.dbc
    uint32 SpellID;                                         // 2 spell id to learn
    //uint32 Flags;                                         // 3 some kind of flags
};

struct TaxiNodesEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    ContinentID;                                       // 1        m_ContinentID
    float     Pos_0;                                            // 2        m_x
    float     Pos_1;                                            // 3        m_y
    float     Pos_2;                                            // 4        m_z
    DBCString Name_lang;                                         // 5        m_Name_lang
    uint32    MountCreatureID[2];                           // 6-7      m_MountCreatureID[2]
    //uint32 unk;                                           // 8  - 4.2.0
    //float unk1;                                           // 9  - 4.2.0
    //float unk2;                                           // 10 - 4.2.0
};

struct TaxiPathEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    FromTaxiNode;                                         // 1        m_FromTaxiNode
    uint32    ToTaxiNode;                                           // 2        m_ToTaxiNode
    uint32    Cost;                                        // 3        m_Cost
};

struct TaxiPathNodeEntry
{
                                                            // 0        m_ID
    uint32    PathID;                                         // 1        m_PathID
    uint32    NodeIndex;                                        // 2        m_NodeIndex
    uint32    ContinentID;                                        // 3        m_ContinentID
    float     Loc_0;                                            // 4        m_LocX
    float     Loc_1;                                            // 5        m_LocY
    float     Loc_2;                                            // 6        m_LocZ
    uint32    Flags;                                   // 7        m_flags
    uint32    Delay;                                        // 8        m_delay
    uint32    ArrivalEventID;                               // 9        m_arrivalEventID
    uint32    DepartureEventID;                             // 10       m_departureEventID
};

struct TotemCategoryEntry
{
    uint32    ID;                                           // 0        m_ID
    //char*   name;                                         // 1        m_name_lang
    uint32    TotemCategoryType;                            // 2        m_totemCategoryType (one for specialization)
    uint32    TotemCategoryMask;                            // 3        m_totemCategoryMask (compatibility mask for same type: different for totems, compatible from high to low for rods)
};

struct TransportAnimationEntry
{
    //uint32    id;                                         // 0
    uint32    TransportID;                               // 1
    uint32    TimeIndex;                                    // 2
    //float     xOffs;                                      // 3
    //float     yOffs;                                      // 4
    //float     zOffs;                                      // 5
    //uint32    unk;                                        // 6
};

#define MAX_VEHICLE_SEAT 8

struct VehicleEntry
{
    uint32  ID;                                           // 0
    uint32  Flags;                                        // 1
    float   TurnSpeed;                                    // 2
    float   PitchSpeed;                                   // 3
    float   PitchMin;                                     // 4
    float   PitchMax;                                     // 5
    uint32  SeatID[MAX_VEHICLE_SEAT];                     // 6-13
    float   MouseLookOffsetPitch;                         // 14
    float   CameraFadeDistScalarMin;                      // 15
    float   CameraFadeDistScalarMax;                      // 16
    float   CameraPitchOffset;                            // 17
    float   FacingLimitRight;                             // 18
    float   FacingLimitLeft;                              // 19
    float   MsslTrgtTurnLingering;                        // 20
    float   MsslTrgtPitchLingering;                       // 21
    float   MsslTrgtMouseLingering;                       // 22
    float   MsslTrgtEndOpacity;                           // 23
    float   MsslTrgtArcSpeed;                             // 24
    float   MsslTrgtArcRepeat;                            // 25
    float   MsslTrgtArcWidth;                             // 26
    float   MsslTrgtImpactRadius[2];                      // 27-28
    DBCString MsslTrgtArcTexture;                         // 29
    DBCString MsslTrgtImpactTexture;                      // 30
    DBCString MsslTrgtImpactModel[2];                     // 31-32
    float   CameraYawOffset;                              // 33
    uint32  UiLocomotionType;                             // 34
    float   MsslTrgtImpactTexRadius;                      // 35
    uint32  VehicleUIIndicatorID;                          // 36       m_vehicleUIIndicatorID
    uint32  PowerDisplayID_0;                               // 37
                                                            // 38 new in 3.1
                                                            // 39 new in 3.1
};

struct VehicleSeatEntry
{
    uint32  ID;                                           // 0
    uint32  Flags;                                        // 1
    int32   AttachmentID;                                 // 2
    float   AttachmentOffset_0;                            // 3
    float   AttachmentOffset_1;                            // 4
    float   AttachmentOffset_2;                            // 5
    float   EnterPreDelay;                                // 6
    float   EnterSpeed;                                   // 7
    float   EnterGravity;                                 // 8
    float   EnterMinDuration;                             // 9
    float   EnterMaxDuration;                             // 10
    float   EnterMinArcHeight;                            // 11
    float   EnterMaxArcHeight;                            // 12
    int32   EnterAnimStart;                               // 13
    int32   EnterAnimLoop;                                // 14
    int32   RideAnimStart;                                // 15
    int32   RideAnimLoop;                                 // 16
    int32   RideUpperAnimStart;                           // 17
    int32   RideUpperAnimLoop;                            // 18
    float   ExitPreDelay;                                 // 19
    float   ExitSpeed;                                    // 20
    float   ExitGravity;                                  // 21
    float   ExitMinDuration;                              // 22
    float   ExitMaxDuration;                              // 23
    float   ExitMinArcHeight;                             // 24
    float   ExitMaxArcHeight;                             // 25
    int32   ExitAnimStart;                                // 26
    int32   ExitAnimLoop;                                 // 27
    int32   ExitAnimEnd;                                  // 28
    float   PassengerYaw;                                 // 29
    float   PassengerPitch;                               // 30
    float   PassengerRoll;                                // 31
    int32   PassengerAttachmentID;                        // 32
    int32   VehicleEnterAnim;                             // 33
    int32   VehicleExitAnim;                              // 34
    int32   VehicleRideAnimLoop;                          // 35
    int32   VehicleEnterAnimBone;                         // 36
    int32   VehicleExitAnimBone;                          // 37
    int32   VehicleRideAnimLoopBone;                      // 38
    float   VehicleEnterAnimDelay;                        // 39
    float   VehicleExitAnimDelay;                         // 40
    uint32  VehicleAbilityDisplay;                        // 41
    uint32  EnterUISoundID;                               // 42
    uint32  ExitUISoundID;                                // 43
    int32   UiSkin;                                       // 44
    uint32  FlagsB;                                       // 45
                                                            // 46       m_cameraEnteringDelay
                                                            // 47       m_cameraEnteringDuration
                                                            // 48       m_cameraExitingDelay
                                                            // 49       m_cameraExitingDuration
                                                            // 50       m_cameraOffsetX
                                                            // 51       m_cameraOffsetY
                                                            // 52       m_cameraOffsetZ
                                                            // 53       m_cameraPosChaseRate
                                                            // 54       m_cameraFacingChaseRate
                                                            // 55       m_cameraEnteringZoom"
                                                            // 56       m_cameraSeatZoomMin
                                                            // 57       m_cameraSeatZoomMax
    //uint32 unk[6];                                        // 58-63
    //uint32 unk2;                                          // 64 4.0.0
    //uint32 unk3;                                          // 65 4.0.1
};

struct WMOAreaTableEntry
{
    uint32 ID;                                              // 0        m_ID index
    int32 WMOID;                                           // 1        m_WMOID used in root WMO
    int32 NameSetID;                                            // 2        m_NameSetID used in adt file
    int32 WMOGroupID;                                          // 3        m_WMOGroupID used in group WMO
    //uint32 field4;                                        // 4        m_SoundProviderPref
    //uint32 field5;                                        // 5        m_SoundProviderPrefUnderwater
    //uint32 field6;                                        // 6        m_AmbienceID
    //uint32 field7;                                        // 7        m_ZoneMusic
    //uint32 field8;                                        // 8        m_IntroSound
    uint32 Flags;                                           // 9        m_flags (used for indoor/outdoor determination)
    uint32 AreaTableID;                                          // 10       m_AreaTableID (AreaTable.dbc)
    //char *Name;                                           // 11       m_AreaName_lang
    //uint32 field12;                                       // 12
    //uint32 field13;                                       // 13
    //uint32 field14;                                       // 14
};

struct WorldMapAreaEntry
{
    //uint32  ID;                                           // 0        m_ID
    uint32  MapID;                                         // 1        m_mapID
    uint32  AreaID;                                        // 2        m_areaID index (continent 0 areas ignored)
    //char* internal_name                                   // 3        m_areaName
    float   LocLeft;                                             // 4        m_locLeft
    float   LocRight;                                             // 5        m_locRight
    float   LocTop;                                             // 6        m_locTop
    float   LocBottom;                                             // 7        m_locBottom
    int32   DisplayMapID;                                 // 8        m_displayMapID -1 (map_id have correct map) other: virtual map where zone show (map_id - where zone in fact internally)
    // int32   dungeonMap_id;                               // 9        m_defaultDungeonFloor (DungeonMap.dbc)
    // uint32  someMapID;                                   // 10       m_parentWorldMapID
    //uint32   unk1;                                        // 11 4.0.0
    //uint32 unk2;                                          // 12 - 4.3.0
    //uint32 unk4;                                          // 13 - 4.3.0
};

#define MAX_WORLD_MAP_OVERLAY_AREA_IDX 4

struct WorldMapOverlayEntry
{
    uint32    ID;                                           // 0        m_ID
    //uint32    worldMapAreaId;                             // 1        m_mapAreaID (WorldMapArea.dbc)
    uint32    AreaID[MAX_WORLD_MAP_OVERLAY_AREA_IDX];       // 2-5      m_areaID
    //char* internal_name                                   // 6        m_textureName
                                                            // 7        m_textureWidth
                                                            // 8        m_textureHeight
                                                            // 9        m_offsetX
                                                            // 10       m_offsetY
                                                            // 11       m_hitRectTop
                                                            // 12       m_hitRectLeft
                                                            // 13       m_hitRectBottom
                                                            // 14       m_hitRectRight
};

struct WorldSafeLocsEntry
{
    uint32    ID;                                           // 0        m_ID
    uint32    Continent;                                       // 1        m_continent
    float     Loc_0;                                            // 2        m_locX
    float     Loc_1;                                            // 3        m_locY
    float     Loc_2;                                            // 4        m_locZ
    //char*   name;                                         // 5        m_AreaName_lang
};

struct WorldPvPAreaEntry
{
    uint32      Id;                                         // 0 m_battlefieldId
    uint32      ZoneId;                                     // 1 m_zoneId
    uint32      NoWarTimeState;                             // 2 m_noWarTimeState
    uint32      WarTimeState;                               // 3 m_warTimeState
    uint32      ukn1;                                       // 4 m_unk1 not known yet, both 900
    uint32      minLevel;                                   // 5 m_minLevel
    uint32      ukn2;                                       // 6 m_unk2
};

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

typedef std::set<uint32> SpellCategorySet;
typedef std::map<uint32, SpellCategorySet > SpellCategoryStore;
typedef std::set<uint32> PetFamilySpellsSet;
typedef std::map<uint32, PetFamilySpellsSet > PetFamilySpellsStore;

// Structures not used for casting to loaded DBC data and not required then packing
struct TalentSpellPos
{
    TalentSpellPos() : talent_id(0), rank(0) {}
    TalentSpellPos(uint16 _talent_id, uint8 _rank) : talent_id(_talent_id), rank(_rank) {}

    uint16 talent_id;
    uint8  rank;
};

typedef std::map<uint32, TalentSpellPos> TalentSpellPosMap;

struct SpellEffect
{
    SpellEffect()
    {
        effects[0] = NULL;
        effects[1] = NULL;
        effects[2] = NULL;
    }
    SpellEffectEntry const* effects[3];
};

typedef std::map<uint32, SpellEffect> SpellEffectMap;

struct TaxiPathBySourceAndDestination
{
    TaxiPathBySourceAndDestination() : ID(0), price(0) {}
    TaxiPathBySourceAndDestination(uint32 _id, uint32 _price) : ID(_id), price(_price) {}

    uint32    ID;
    uint32    price;
};
typedef std::map<uint32, TaxiPathBySourceAndDestination> TaxiPathSetForSource;
typedef std::map<uint32, TaxiPathSetForSource> TaxiPathSetBySource;

struct TaxiPathNodePtr
{
    TaxiPathNodePtr() : i_ptr(NULL) {}
    TaxiPathNodePtr(TaxiPathNodeEntry const* ptr) : i_ptr(ptr) {}

    TaxiPathNodeEntry const* i_ptr;

    operator TaxiPathNodeEntry const& () const { return *i_ptr; }
};

typedef Path<TaxiPathNodePtr, TaxiPathNodeEntry const> TaxiPathNodeList;
typedef std::vector<TaxiPathNodeList> TaxiPathNodesByPath;

typedef std::unordered_map<uint32 /*frame*/, TransportAnimationEntry const*> TransportAnimationEntryMap;
typedef std::unordered_map<uint32, TransportAnimationEntryMap> TransportAnimationsByEntry;

#define TaxiMaskSize 114
typedef uint8 TaxiMask[TaxiMaskSize];
#endif
