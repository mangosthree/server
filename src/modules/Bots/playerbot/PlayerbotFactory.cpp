#include "../botpch.h"
#include "playerbot.h"
#include "PlayerbotFactory.h"
#include "SQLStorages.h"
#include "ItemPrototype.h"
#include "PlayerbotAIConfig.h"
#include "AccountMgr.h"
#include "DBCStore.h"
#include "SharedDefines.h"
#include "ahbot/AhBot.h"
#include "RandomPlayerbotFactory.h"
#include "AiFactory.h"

using namespace ai;
using namespace std;

// List of trade skills available for player bots
uint32 PlayerbotFactory::tradeSkills[] =
{
    SKILL_ALCHEMY,
    SKILL_ENCHANTING,
    SKILL_SKINNING,
    SKILL_TAILORING,
#if !defined(CLASSIC)
    SKILL_JEWELCRAFTING,
#endif
    SKILL_LEATHERWORKING,
    SKILL_ENGINEERING,
    SKILL_HERBALISM,
    SKILL_MINING,
    SKILL_BLACKSMITHING,
    SKILL_COOKING,
    SKILL_FIRST_AID,
    SKILL_FISHING
};

// List of class-specific quest IDs
list<uint32> PlayerbotFactory::classQuestIds;

// Curated gear cache: class -> spec -> tier -> slot -> itemId.
map<uint8, map<string, map<string, map<string, uint32> > > > PlayerbotFactory::curatedGearSets;

// Phase B/C/D: curated enchant/gem/glyph caches, mirroring curatedGearSets.
bool PlayerbotFactory::curatedEnhancementsLoaded = false;
map<uint8, map<string, map<string, uint32> > > PlayerbotFactory::curatedGearEnchants;
map<uint8, map<string, map<string, pair<uint32, uint32> > > > PlayerbotFactory::curatedGearGems;
map<uint8, map<string, vector<CuratedGlyphRow> > > PlayerbotFactory::curatedGearGlyphs;

/**
 * Randomizes the player bot's attributes and equipment.
 */
void PlayerbotFactory::Randomize()
{
    Randomize(true);
}

/**
 * Refreshes the player bot's attributes and equipment.
 */
void PlayerbotFactory::Refresh()
{
    Prepare();
    InitEquipment(true);
    InitFood();
    InitPotions();
    InitPet();

    uint32 money = urand(level * 1000, level * 5 * 1000);
    if (bot->GetMoney() < money)
    {
        bot->SetMoney(money);
    }
    bot->SaveToDB();
}

/**
 * Randomizes the player bot's attributes and equipment without incremental changes.
 */
void PlayerbotFactory::CleanRandomize()
{
    Randomize(false);
}

/**
 * Prepares the player bot for randomization by setting initial attributes and flags.
 */
void PlayerbotFactory::Prepare()
{
    if (!itemQuality)
    {
        if (level <= 10)
        {
            itemQuality = urand(ITEM_QUALITY_NORMAL, ITEM_QUALITY_UNCOMMON);
        }
        else if (level <= 20)
        {
            itemQuality = urand(ITEM_QUALITY_UNCOMMON, ITEM_QUALITY_RARE);
        }
        else if (level <= 40)
        {
            itemQuality = urand(ITEM_QUALITY_UNCOMMON, ITEM_QUALITY_EPIC);
        }
        else if (level < 60)
        {
            itemQuality = urand(ITEM_QUALITY_UNCOMMON, ITEM_QUALITY_EPIC);
        }
        else
        {
            itemQuality = urand(ITEM_QUALITY_RARE, ITEM_QUALITY_EPIC);
        }
    }

    if (bot->IsDead())
    {
        bot->ResurrectPlayer(1.0f, false);
    }

    // Clamp to the realm's level cap: on a Cata 4.3.4 realm a master can be above
    // 85 (imported/MoP-level char), but gear only exists up to reqLevel 85, so an
    // over-cap bot finds nothing to equip and ends up naked.
    uint32 maxLevel = sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL);
    if (level > maxLevel)
    {
        level = maxLevel;
    }

    bot->CombatStop(true);
    bot->SetLevel(level);
    if (!sPlayerbotAIConfig.randomBotShowHelmet)
    {
        bot->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM);
    }

    if (!sPlayerbotAIConfig.randomBotShowCloak)
    {
        bot->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK);
    }
}

/**
 * Randomizes the player bot's attributes and equipment.
 * @param incremental Whether to apply incremental changes.
 */
void PlayerbotFactory::Randomize(bool incremental)
{
    sLog.outDetail("Preparing to randomize...");
    Prepare();

    sLog.outDetail("Resetting player...");
    bot->resetTalents(true);
    ClearSpells();
    ClearInventory();
    bot->SaveToDB();

    sLog.outDetail("Initializing quests...");
    InitQuests();

    // A quest reward spell completed above can teleport the bot off its map, and a
    // bot whose login never finished Map::Add is unmapped from the outset. Every
    // remaining step (equip, enchant, gems, spell learning) resolves the bot and its
    // items through the world/map and would crash on a mapless bot (GetOwner()==NULL,
    // GetTerrain() on a NULL map, ...). Abort here rather than run that gauntlet:
    // RandomizeFirst/IncreaseLevel re-anchor the bot via RandomTeleportForLevel right
    // after, so it re-randomizes cleanly once it is mapped again.
    if (!bot->GetMap())
    {
        sLog.outError("PlayerbotFactory::Randomize aborted for bot %u: unmapped after InitQuests",
                      bot->GetGUIDLow());
        return;
    }

    // quest rewards boost bot level, so reduce back
    bot->SetLevel(level);
    ClearInventory();
    bot->SetUInt32Value(PLAYER_XP, 0);
    CancelAuras();
    bot->SaveToDB();

    sLog.outDetail("Initializing spells (step 1)...");
    InitAvailableSpells();

    sLog.outDetail("Initializing skills (step 1)...");
    InitSkills();
    InitTradeSkills();

    sLog.outDetail("Initializing talents...");
    InitTalents();

    sLog.outDetail("Initializing glyphs...");
    InitGlyphs();

    sLog.outDetail("Initializing spells (step 2)...");
    InitAvailableSpells();
    InitSpecialSpells();

    sLog.outDetail("Initializing Quest Spells...");
    InitQuestSpells();

    sLog.outDetail("Initializing mounts...");
    InitMounts();

    sLog.outDetail("Initializing skills (step 2)...");
    UpdateTradeSkills();
    bot->SaveToDB();

    sLog.outDetail("Initializing equipment...");
    InitEquipment(incremental);

    sLog.outDetail("Initializing bags...");
    InitBags();

    sLog.outDetail("Initializing food...");
    InitFood();

    sLog.outDetail("Initializing potions...");
    InitPotions();

    sLog.outDetail("Initializing second equipment set...");
    InitSecondEquipmentSet();

    sLog.outDetail("Initializing inventory...");
    InitInventory();

    sLog.outDetail("Initializing guilds...");
    InitGuild();

    sLog.outDetail("Initializing pet...");
    InitPet();

    sLog.outDetail("Saving to DB...");
    bot->SetMoney(urand(level * 1000, level * 5 * 1000));
    bot->SetHealth(bot->GetMaxHealth());
    bot->SaveToDB();
    sLog.outDetail("Done.");
}

/**
 * Initializes the player bot's pet.
 */
void PlayerbotFactory::InitPet()
{
    Pet* pet = bot->GetPet();
    if (!pet)
    {
        if (bot->getClass() != CLASS_HUNTER)
        {
            return;
        }

        Map* map = bot->GetMap();
        if (!map)
        {
            return;
        }

        vector<uint32> ids;
        for (uint32 id = 0; id < sCreatureStorage.GetMaxEntry(); ++id)
        {
            CreatureInfo const* co = sCreatureStorage.LookupEntry<CreatureInfo>(id);
            if (!co)
            {
                continue;
            }

            if (!co->isTameable(false))
            {
                continue;
            }

            if ((int)co->MinLevel > (int)bot->getLevel())
            {
                continue;
            }

            ids.push_back(id);
        }

        if (ids.empty())
        {
            sLog.outError("No pets available for bot %s (%d level)", bot->GetName(), bot->getLevel());
            return;
        }

        for (int i = 0; i < 100; i++)
        {
            int index = urand(0, ids.size() - 1);
            CreatureInfo const* co = sCreatureStorage.LookupEntry<CreatureInfo>(ids[index]);
            if (!co)
            {
                continue;
            }

            uint32 guid = map->GenerateLocalLowGuid(HIGHGUID_PET);
            uint32 pet_number = sObjectMgr.GeneratePetNumber();
            CreatureCreatePos pos(map, bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetOrientation(), bot->GetPhaseMask());
            pet = new Pet(HUNTER_PET);

            if (!pet->Create(guid, pos, co, pet_number))
            {
                delete pet;
                pet = NULL;
                continue;
            }
            pet->GetCharmInfo()->SetPetNumber(pet_number, true);
            pet->SetOwnerGuid(bot->GetObjectGuid());
            pet->SetCreatorGuid(bot->GetObjectGuid());
            pet->setFaction(bot->getFaction());
            pet->SetLevel(bot->getLevel());
            pet->setPetType(HUNTER_PET);
            pet->SetCanModifyStats(true);
            pet->InitStatsForLevel(bot->getLevel());
            pet->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
            pet->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
            pet->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_RENAME); // Allow renaming
            pet->SetPowerType(POWER_FOCUS);
            pet->GetCharmInfo()->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);
            map->Add((Creature*)pet);
            pet->AIM_Initialize();
            pet->InitPetCreateSpells();
            pet->LearnPetPassives();
            pet->CastPetAuras(true);
            pet->SetHealth(pet->GetMaxHealth());
            pet->SetPower(POWER_FOCUS, pet->GetMaxPower(POWER_FOCUS));
            bot->SetPet(pet);
            bot->SetPetGuid(pet->GetObjectGuid());

            sLog.outDebug("Bot %s: assign pet %d (%d level)", bot->GetName(), co->Entry, bot->getLevel());
            pet->SavePetToDB(PET_SAVE_AS_CURRENT);
            bot->PetSpellInitialize();
            break;
        }
    }

    pet = bot->GetPet();
    if (pet)
    {
        pet->InitStatsForLevel(bot->getLevel());
        pet->SetLevel(bot->getLevel());
        pet->SetHealth(pet->GetMaxHealth());
    }
    else
    {
        sLog.outError("Cannot create pet for bot %s", bot->GetName());
        return;
    }

    for (PetSpellMap::const_iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
    {
        if (itr->second.state == PETSPELL_REMOVED)
        {
            continue;
        }

        uint32 spellId = itr->first;
        if (IsPassiveSpell(spellId))
        {
            continue;
        }

        pet->ToggleAutocast(spellId, true);
    }
}

/**
 * Clears the player bot's spells.
 */
void PlayerbotFactory::ClearSpells()
{
    list<uint32> spells;
    for (PlayerSpellMap::iterator itr = bot->GetSpellMap().begin(); itr != bot->GetSpellMap().end(); ++itr)
    {
        uint32 spellId = itr->first;
        if (itr->second.state == PLAYERSPELL_REMOVED || itr->second.disabled || IsPassiveSpell(spellId))
        {
            continue;
        }

        spells.push_back(spellId);
    }

    for (list<uint32>::iterator i = spells.begin(); i != spells.end(); ++i)
    {
        bot->removeSpell(*i, false, false);
    }
}

/**
 * Initializes the player bot's spells.
 */
void PlayerbotFactory::InitSpells()
{
    for (int i = 0; i < 15; i++)
    {
        InitAvailableSpells();
    }
}

/**
 * Initializes the player bot's talents.
 */
void PlayerbotFactory::InitTalents()
{
    uint32 point = urand(0, 100);
    uint8 cls = bot->getClass();
    uint32 p1 = sPlayerbotAIConfig.specProbability[cls][0];
    uint32 p2 = p1 + sPlayerbotAIConfig.specProbability[cls][1];

    uint32 specNo = (point < p1 ? 0 : (point < p2 ? 1 : 2));

    // start from a clean slate so the primary tree (and its mastery) is
    // chosen by Player::LearnTalent, healing bots built by the old
    // raw-learnSpell path that never set a tree
    bot->resetTalents(true);

    InitTalents(specNo);

    // Cata: off-tree rows unlock only after 31 points in the primary tree
    if (bot->GetFreeTalentPoints())
    {
        InitTalents((specNo + 1) % 3);
    }
    if (bot->GetFreeTalentPoints())
    {
        InitTalents((specNo + 2) % 3);
    }
}

/**
 * Visitor class for destroying items in the player bot's inventory.
 */
class DestroyItemsVisitor : public IterateItemsVisitor
{
public:
    DestroyItemsVisitor(Player* bot) : IterateItemsVisitor(), bot(bot) {}

    virtual bool Visit(Item* item)
    {
        uint32 id = item->GetProto()->ItemId;
        if (CanKeep(id))
        {
            keep.insert(id);
            return true;
        }

        bot->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
        return true;
    }

private:
    bool CanKeep(uint32 id)
    {
        if (keep.find(id) != keep.end())
        {
            return false;
        }

        if (sPlayerbotAIConfig.IsInRandomQuestItemList(id))
        {
            return true;
        }

        ItemPrototype const* proto = sItemStorage.LookupEntry<ItemPrototype>(id);
        if (proto->Class == ITEM_CLASS_MISC && proto->SubClass == ITEM_SUBCLASS_JUNK)
        {
            return true;
        }

        return false;
    }

private:
    Player* bot;
    set<uint32> keep;
};

/**
 * Checks if the player bot can equip the specified armor item.
 * @param proto The item prototype.
 * @return True if the player bot can equip the item, false otherwise.
 */
bool PlayerbotFactory::CanEquipArmor(ItemPrototype const* proto)
{
    if (bot->HasSkill(SKILL_SHIELD) && proto->SubClass == ITEM_SUBCLASS_ARMOR_SHIELD)
    {
        return true;
    }

    if (bot->HasSkill(SKILL_PLATE_MAIL))
    {
        if (proto->SubClass != ITEM_SUBCLASS_ARMOR_PLATE)
        {
            return false;
        }
    }
    else if (bot->HasSkill(SKILL_MAIL))
    {
        if (proto->SubClass != ITEM_SUBCLASS_ARMOR_MAIL)
        {
            return false;
        }
    }
    else if (bot->HasSkill(SKILL_LEATHER))
    {
        if (proto->SubClass != ITEM_SUBCLASS_ARMOR_LEATHER)
        {
            return false;
        }
    }

    if (proto->Quality <= ITEM_QUALITY_NORMAL)
    {
        return true;
    }

    uint8 sp = 0, ap = 0, tank = 0;
    for (int j = 0; j < MAX_ITEM_PROTO_STATS; ++j)
    {
        // for ItemStatValue != 0
        if (!proto->ItemStat[j].ItemStatValue)
        {
            continue;
        }

        AddItemStats(proto->ItemStat[j].ItemStatType, sp, ap, tank);
    }

    return CheckItemStats(sp, ap, tank);
}

/**
 * Checks if the player bot's item stats are valid.
 * @param sp The spell power stat.
 * @param ap The attack power stat.
 * @param tank The tank stat.
 * @return True if the item stats are valid, false otherwise.
 */
bool PlayerbotFactory::CheckItemStats(uint8 sp, uint8 ap, uint8 tank)
{
    switch (bot->getClass())
    {
        case CLASS_PRIEST:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            if (!sp || ap > sp || tank > sp)
            {
                return false;
            }
            break;
        case CLASS_PALADIN:
        case CLASS_WARRIOR:
            if ((!ap && !tank) || sp > ap || sp > tank)
            {
                return false;
            }
            break;
        case CLASS_HUNTER:
        case CLASS_ROGUE:
            if (!ap || sp > ap || sp > tank)
            {
                return false;
            }
            break;
    }

    return sp || ap || tank;
}

/**
 * Adds item stats to the player bot.
 * @param mod The stat modifier.
 * @param sp The spell power stat.
 * @param ap The attack power stat.
 * @param tank The tank stat.
 */
void PlayerbotFactory::AddItemStats(uint32 mod, uint8 &sp, uint8 &ap, uint8 &tank)
{
    switch (mod)
    {
        case ITEM_MOD_HEALTH:
        case ITEM_MOD_STAMINA:
        case ITEM_MOD_INTELLECT:
        case ITEM_MOD_SPIRIT:
            sp++;
            break;
    }

    switch (mod)
    {
        case ITEM_MOD_AGILITY:
        case ITEM_MOD_STRENGTH:
        case ITEM_MOD_HEALTH:
        case ITEM_MOD_STAMINA:
            tank++;
            break;
    }

    switch (mod)
    {
        case ITEM_MOD_HEALTH:
        case ITEM_MOD_STAMINA:
        case ITEM_MOD_AGILITY:
        case ITEM_MOD_STRENGTH:
            ap++;
            break;
    }
}

/**
 * Checks if the player bot can equip the specified weapon item.
 * @param proto The item prototype.
 * @return True if the player bot can equip the item, false otherwise.
 */
bool PlayerbotFactory::CanEquipWeapon(ItemPrototype const* proto)
{
    switch (bot->getClass())
    {
        case CLASS_PRIEST:
            if (proto->SubClass != ITEM_SUBCLASS_WEAPON_STAFF &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_WAND &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE)
                return false;
            break;
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            if (proto->SubClass != ITEM_SUBCLASS_WEAPON_STAFF &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_WAND &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_DAGGER &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_SWORD)
                return false;
            break;
        case CLASS_WARRIOR:
            if (proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_AXE &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_POLEARM &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_SWORD2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_SWORD &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_GUN &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_CROSSBOW &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_BOW &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_THROWN)
                return false;
            break;
        case CLASS_PALADIN:
            if (proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_SWORD2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_AXE2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_SWORD)
                return false;
            break;
        case CLASS_SHAMAN:
            if (proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_AXE &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_FIST &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_AXE2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_DAGGER &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_FIST &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_STAFF)
                return false;
            break;
        case CLASS_DRUID:
            if (proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_DAGGER &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_STAFF)
                return false;
            break;
        case CLASS_HUNTER:
            if (proto->SubClass != ITEM_SUBCLASS_WEAPON_AXE2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_AXE &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_SWORD2 &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_POLEARM &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_FIST &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_GUN &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_CROSSBOW &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_STAFF &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_BOW)
                return false;
            break;
        case CLASS_ROGUE:
            if (proto->SubClass != ITEM_SUBCLASS_WEAPON_DAGGER &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_SWORD &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_FIST &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_GUN &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_CROSSBOW &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_BOW &&
                proto->SubClass != ITEM_SUBCLASS_WEAPON_THROWN)
                return false;
            break;
    }

    return true;
}

/**
 * Checks if the player bot can equip the specified item.
 * @param proto The item prototype.
 * @param desiredQuality The desired quality of the item.
 * @return True if the player bot can equip the item, false otherwise.
 */
bool PlayerbotFactory::CanEquipItem(ItemPrototype const* proto, uint32 desiredQuality)
{
    if (proto->Duration & 0x80000000)
    {
        return false;
    }

    if (proto->Quality != desiredQuality)
    {
        return false;
    }

    if (proto->Bonding == BIND_QUEST_ITEM || proto->Bonding == BIND_WHEN_USE)
    {
        return false;
    }

    if (proto->Class == ITEM_CLASS_CONTAINER)
    {
        return true;
    }

    uint32 requiredLevel = proto->RequiredLevel;
    if (!requiredLevel)
    {
        return false;
    }

    uint32 level = bot->getLevel();
    uint32 delta = 2;
    if (level < 15)
    {
        delta = urand(7, 15);
    }
    else if (proto->Class == ITEM_CLASS_WEAPON || proto->SubClass == ITEM_SUBCLASS_ARMOR_SHIELD)
    {
        delta = urand(2, 3);
    }
    else if (!(level % 10) || (level % 10) == 9)
    {
        delta = 2;
    }
    else if (level < 40)
    {
        delta = urand(5, 10);
    }
    else if (level < 60)
    {
        delta = urand(3, 7);
    }
    else if (level < 70)
    {
        delta = urand(2, 5);
    }
    else if (level < 80)
    {
        delta = urand(2, 4);
    }

    if (desiredQuality > ITEM_QUALITY_NORMAL &&
        (requiredLevel > level || requiredLevel < level - delta))
        return false;

    for (uint32 gap = 60; gap <= 80; gap += 10)
    {
        if (level > gap && requiredLevel <= gap)
        {
            return false;
        }
    }

    return true;
}

/**
 * Resolves the ai_playerbot_gear dataset spec key for the bot's class and
 * primary talent tab.
 * @param bot The player bot.
 * @param tab The primary talent tab (0/1/2), or -1 if unresolved.
 * @return The spec key, or an empty string if unmapped.
 */
string PlayerbotFactory::GetCuratedSpecKey(Player* bot, int tab)
{
    if (tab < 0 || tab > 2)
    {
        return "";
    }

    switch (bot->getClass())
    {
        case CLASS_WARRIOR:
            switch (tab)
            {
                case 0: return "arms";
                case 1: return "fury";
                case 2: return "protection";
            }
            break;
        case CLASS_PALADIN:
            switch (tab)
            {
                case 0: return "holy";
                case 1: return "protection";
                case 2: return "retribution";
            }
            break;
        case CLASS_HUNTER:
            switch (tab)
            {
                case 0: return "beast_mastery";
                case 1: return "marksmanship";
                case 2: return "survival";
            }
            break;
        case CLASS_ROGUE:
            switch (tab)
            {
                case 0: return "assassination";
                case 1: return "combat";
                case 2: return "subtlety";
            }
            break;
        case CLASS_PRIEST:
            switch (tab)
            {
                case 0: return "discipline";
                case 1: return "holy";
                case 2: return "shadow";
            }
            break;
        case CLASS_DEATH_KNIGHT:
            switch (tab)
            {
                case 0: return "blood";
                case 1: return "frost";
                case 2: return "unholy";
            }
            break;
        case CLASS_SHAMAN:
            switch (tab)
            {
                case 0: return "elemental";
                case 1: return "enhancement";
                case 2: return "restoration";
            }
            break;
        case CLASS_MAGE:
            switch (tab)
            {
                case 0: return "arcane";
                case 1: return "fire";
                case 2: return "frost";
            }
            break;
        case CLASS_WARLOCK:
            switch (tab)
            {
                case 0: return "affliction";
                case 1: return "demonology";
                case 2: return "destruction";
            }
            break;
        case CLASS_DRUID:
            switch (tab)
            {
                case 0: return "balance";
                // Feral (tab 1) covers both the cat-DPS and bear-tank builds
                // in Cata's talent layout. Default to the DPS (cat) set --
                // cheaply detecting a bear-tank build isn't worth the
                // complexity for Phase A (gear only); revisit alongside role
                // detection if/when that lands.
                case 1: return "feral_cat";
                case 2: return "restoration";
            }
            break;
    }

    return "";
}

/**
 * Resolves the curated gear tier for the bot: honours the
 * AiPlayerbot.GearTier override when set, otherwise a balanced
 * normal/heroic/raider split keyed off the bot's GUID.
 * @param bot The player bot.
 * @return "normal", "heroic" or "raider".
 */
string PlayerbotFactory::GetCuratedTier(Player* bot)
{
    if (!sPlayerbotAIConfig.gearTier.empty())
    {
        return sPlayerbotAIConfig.gearTier;
    }

    static const char* tiers[3] = { "normal", "heroic", "raider" };
    return tiers[bot->GetGUIDLow() % 3];
}

/**
 * Lazily loads the curated enchant/gem/glyph caches (Phase B/C/D) from
 * ai_playerbot_gear_enchant, ai_playerbot_gem and ai_playerbot_glyph.
 * Mirrors the curatedGearSets loader in InitCuratedGear(). No-op after the
 * first call (curatedEnhancementsLoaded), even if the tables are empty.
 */
void PlayerbotFactory::LoadCuratedGearEnhancements()
{
    if (curatedEnhancementsLoaded)
    {
        return;
    }
    curatedEnhancementsLoaded = true;

    QueryResult* enchantResults = CharacterDatabase.Query(
        "SELECT `class`, `spec`, `slot`, `enchant` FROM `ai_playerbot_gear_enchant`");
    if (enchantResults)
    {
        do
        {
            Field* fields = enchantResults->Fetch();
            uint8 cls = fields[0].GetUInt8();
            string spec = fields[1].GetCppString();
            string slot = fields[2].GetCppString();
            uint32 enchant = fields[3].GetUInt32();
            curatedGearEnchants[cls][spec][slot] = enchant;
        }
        while (enchantResults->NextRow());
        delete enchantResults;
    }

    QueryResult* gemResults = CharacterDatabase.Query(
        "SELECT `class`, `spec`, `color`, `gem_item`, `gem_enchant` FROM `ai_playerbot_gem`");
    if (gemResults)
    {
        do
        {
            Field* fields = gemResults->Fetch();
            uint8 cls = fields[0].GetUInt8();
            string spec = fields[1].GetCppString();
            string color = fields[2].GetCppString();
            uint32 gemItem = fields[3].GetUInt32();
            uint32 gemEnchant = fields[4].GetUInt32();
            curatedGearGems[cls][spec][color] = make_pair(gemItem, gemEnchant);
        }
        while (gemResults->NextRow());
        delete gemResults;
    }

    QueryResult* glyphResults = CharacterDatabase.Query(
        "SELECT `class`, `spec`, `glyph_type`, `glyph_spell`, `slot_idx` FROM `ai_playerbot_glyph`");
    if (glyphResults)
    {
        do
        {
            Field* fields = glyphResults->Fetch();
            uint8 cls = fields[0].GetUInt8();
            string spec = fields[1].GetCppString();

            CuratedGlyphRow row;
            row.type = fields[2].GetCppString();
            row.glyphSpell = fields[3].GetUInt32();
            row.slotIdx = fields[4].GetUInt8();

            curatedGearGlyphs[cls][spec].push_back(row);
        }
        while (glyphResults->NextRow());
        delete glyphResults;
    }

    sLog.outDetail("AI Playerbot: loaded curated gear enhancements (%u enchant classes, %u gem classes, %u glyph classes)",
        (uint32)curatedGearEnchants.size(), (uint32)curatedGearGems.size(), (uint32)curatedGearGlyphs.size());
}

/**
 * Applies the curated permanent enchant (if any) for (bot's class, specKey,
 * gearSlotKey) to a just-equipped item.
 * @param specKey The curated spec key (e.g. "arms").
 * @param gearSlotKey The ai_playerbot_gear slot key; "finger1"/"finger2" are
 * folded to the dataset's single "finger" enchant key.
 * @param item The just-equipped item, or NULL (no-op).
 */
void PlayerbotFactory::ApplyCuratedEnchant(const string& specKey, const string& gearSlotKey, Item* item)
{
    if (!item)
    {
        return;
    }

    map<uint8, map<string, map<string, uint32> > >::const_iterator clsItr =
        curatedGearEnchants.find((uint8)bot->getClass());
    if (clsItr == curatedGearEnchants.end())
    {
        return;
    }

    map<string, map<string, uint32> >::const_iterator specItr = clsItr->second.find(specKey);
    if (specItr == clsItr->second.end())
    {
        return;
    }

    string enchantSlotKey = (gearSlotKey == "finger1" || gearSlotKey == "finger2") ? "finger" : gearSlotKey;

    map<string, uint32>::const_iterator slotItr = specItr->second.find(enchantSlotKey);
    if (slotItr == specItr->second.end())
    {
        return;
    }

    uint32 enchantId = slotItr->second;
    if (!enchantId || !sSpellItemEnchantmentStore.LookupEntry(enchantId))
    {
        sLog.outDebug("%s: curated enchant %u for slot %s not found in SpellItemEnchantment.dbc, skipping",
            bot->GetName(), enchantId, enchantSlotKey.c_str());
        return;
    }

    bot->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, false);
    item->SetEnchantment(PERM_ENCHANTMENT_SLOT, enchantId, 0, 0);
    bot->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, true);
}

/**
 * Sockets curated, color-matched gems into every real socket on a
 * just-equipped item. Meta sockets only ever take the spec's "meta" gem
 * (never a prismatic fallback, matching the real socketing rules in
 * WorldSession::HandleSocketOpcode); red/yellow/blue sockets fall back to
 * "prismatic" when the spec has no gem of that exact color. Socket-bonus
 * (BONUS_ENCHANTMENT_SLOT) activation is intentionally skipped.
 * @param specKey The curated spec key (e.g. "arms").
 * @param item The just-equipped item, or NULL (no-op).
 */
void PlayerbotFactory::ApplyCuratedGems(const string& specKey, Item* item)
{
    if (!item)
    {
        return;
    }

    map<uint8, map<string, map<string, pair<uint32, uint32> > > >::const_iterator clsItr =
        curatedGearGems.find((uint8)bot->getClass());
    if (clsItr == curatedGearGems.end())
    {
        return;
    }

    map<string, map<string, pair<uint32, uint32> > >::const_iterator specItr = clsItr->second.find(specKey);
    if (specItr == clsItr->second.end())
    {
        return;
    }

    const map<string, pair<uint32, uint32> >& gemsByColor = specItr->second;

    ItemPrototype const* proto = item->GetProto();
    if (!proto)
    {
        return;
    }

    for (int i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
    {
        uint32 socketColor = proto->Socket[i].Color;
        if (!socketColor)
        {
            continue; // no real socket here
        }

        string colorKey;
        if (socketColor & SOCKET_COLOR_META)
        {
            colorKey = "meta";
        }
        else if (socketColor == SOCKET_COLOR_RED)
        {
            colorKey = "red";
        }
        else if (socketColor == SOCKET_COLOR_YELLOW)
        {
            colorKey = "yellow";
        }
        else if (socketColor == SOCKET_COLOR_BLUE)
        {
            colorKey = "blue";
        }
        else
        {
            colorKey = "prismatic";
        }

        map<string, pair<uint32, uint32> >::const_iterator gemItr = gemsByColor.find(colorKey);
        if (gemItr == gemsByColor.end() && colorKey != "meta")
        {
            // No exact-color gem curated for this spec; a prismatic gem
            // fits any non-meta socket (loses the socket bonus only).
            gemItr = gemsByColor.find("prismatic");
        }
        if (gemItr == gemsByColor.end())
        {
            sLog.outDebug("%s: no curated %s gem for item %u socket %d, skipping",
                bot->GetName(), colorKey.c_str(), proto->ItemId, i);
            continue;
        }

        uint32 gemEnchant = gemItr->second.second;
        if (!gemEnchant || !sSpellItemEnchantmentStore.LookupEntry(gemEnchant))
        {
            sLog.outDebug("%s: curated gem enchant %u not found in SpellItemEnchantment.dbc, skipping",
                bot->GetName(), gemEnchant);
            continue;
        }

        EnchantmentSlot sockSlot = EnchantmentSlot(SOCK_ENCHANTMENT_SLOT + i);
        bot->ApplyEnchantment(item, sockSlot, false);
        item->SetEnchantment(sockSlot, gemEnchant, 0, 0);
        bot->ApplyEnchantment(item, sockSlot, true);
    }
}

/**
 * Applies the curated glyph set for (bot's class, specKey), overriding
 * InitGlyphs()'s random selection. Resolves each row's glyph_spell to its
 * GlyphProperties entry the same way InitGlyphs() builds its candidate
 * pools, then places it positionally (slot_idx) among the bot's unlocked
 * glyph slots that share that glyph's DBC TypeFlags. No-op if no rows exist
 * for the class/spec.
 * @param specKey The curated spec key (e.g. "arms").
 */
void PlayerbotFactory::ApplyCuratedGlyphs(const string& specKey)
{
    map<uint8, map<string, vector<CuratedGlyphRow> > >::const_iterator clsItr =
        curatedGearGlyphs.find((uint8)bot->getClass());
    if (clsItr == curatedGearGlyphs.end())
    {
        return;
    }

    map<string, vector<CuratedGlyphRow> >::const_iterator specItr = clsItr->second.find(specKey);
    if (specItr == clsItr->second.end())
    {
        return;
    }

    // Group the bot's unlocked physical glyph slots (0..MAX_GLYPH_SLOT_INDEX-1)
    // by their DBC TypeFlags (0/1/2), in ascending slot order, so a curated
    // row's slot_idx can be resolved positionally within its own type.
    map<uint32, vector<uint8> > slotsByType;
    for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot)
    {
        GlyphSlotEntry const* gs = sGlyphSlotStore.LookupEntry(bot->GetGlyphSlot(slot));
        if (!gs || gs->Type >= 3)
        {
            continue;
        }

        slotsByType[gs->Type].push_back(slot);
    }

    bool changed = false;
    for (vector<CuratedGlyphRow>::const_iterator rowItr = specItr->second.begin();
        rowItr != specItr->second.end(); ++rowItr)
    {
        const CuratedGlyphRow& row = *rowItr;

        SpellEntry const* spellInfo = sSpellStore.LookupEntry(row.glyphSpell);
        if (!spellInfo)
        {
            sLog.outDebug("%s: curated glyph spell %u not found, skipping", bot->GetName(), row.glyphSpell);
            continue;
        }

        uint32 glyphId = 0;
        for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            if (spellInfo->GetSpellEffectIdByIndex(SpellEffectIndex(j)) == SPELL_EFFECT_APPLY_GLYPH)
            {
                glyphId = spellInfo->GetEffectMiscValue(SpellEffectIndex(j));
                break;
            }
        }

        GlyphPropertiesEntry const* gp = glyphId ? sGlyphPropertiesStore.LookupEntry(glyphId) : NULL;
        if (!gp || gp->GlyphSlotFlags >= 3)
        {
            sLog.outDebug("%s: curated glyph spell %u (type %s) does not resolve to a usable glyph, skipping",
                bot->GetName(), row.glyphSpell, row.type.c_str());
            continue;
        }

        map<uint32, vector<uint8> >::const_iterator typeItr = slotsByType.find(gp->GlyphSlotFlags);
        if (typeItr == slotsByType.end() || (size_t)row.slotIdx >= typeItr->second.size())
        {
            sLog.outDebug("%s: no unlocked %s glyph slot %u for curated glyph %u, skipping",
                bot->GetName(), row.type.c_str(), (uint32)row.slotIdx, glyphId);
            continue;
        }

        uint8 physicalSlot = typeItr->second[row.slotIdx];

        bot->ApplyGlyph(physicalSlot, false);
        bot->SetGlyph(physicalSlot, glyphId);
        bot->ApplyGlyph(physicalSlot, true);
        changed = true;
    }

    if (changed)
    {
        bot->SendTalentsInfoData(false);
    }
}

/**
 * Equips the bot's curated (class/spec/tier) best-in-slot gear set.
 * @param filledSlots Out-param, receives every slot equipped from curated data.
 * @return True if at least one slot was equipped from a curated set (the
 * caller should skip the legacy gear-score loop); false to fall back.
 */
bool PlayerbotFactory::InitCuratedGear(set<uint8>& filledSlots)
{
    if (!sPlayerbotAIConfig.useCuratedGear)
    {
        return false;
    }

    // Lazily load the curated gear cache once, mirroring the classQuestIds
    // pattern used by InitQuests() below.
    if (curatedGearSets.empty())
    {
        QueryResult* results = CharacterDatabase.Query(
            "SELECT `class`, `spec`, `tier`, `slot`, `item` FROM `ai_playerbot_gear`");
        if (results)
        {
            do
            {
                Field* fields = results->Fetch();
                uint8 cls = fields[0].GetUInt8();
                string spec = fields[1].GetCppString();
                string tier = fields[2].GetCppString();
                string slot = fields[3].GetCppString();
                uint32 item = fields[4].GetUInt32();
                curatedGearSets[cls][spec][tier][slot] = item;
            }
            while (results->NextRow());
            delete results;
        }

        if (curatedGearSets.empty())
        {
            sLog.outDetail("AI Playerbot: ai_playerbot_gear is empty, curated gear unavailable");
            return false;
        }
    }

    map<uint8, map<string, map<string, map<string, uint32> > > >::const_iterator clsItr =
        curatedGearSets.find((uint8)bot->getClass());
    if (clsItr == curatedGearSets.end())
    {
        return false;
    }

    int tab = AiFactory::GetPlayerSpecTab(bot);
    string specKey = GetCuratedSpecKey(bot, tab);
    if (specKey.empty())
    {
        return false;
    }

    // Phase B/C/D: curated glyphs are per (class, spec) only -- no tier
    // dependency -- so apply them as soon as specKey resolves, independent
    // of whether curated gear itself has data for this tier.
    if (sPlayerbotAIConfig.useCuratedGearEnhancements)
    {
        LoadCuratedGearEnhancements();
        ApplyCuratedGlyphs(specKey);
    }

    map<string, map<string, map<string, uint32> > >::const_iterator specItr = clsItr->second.find(specKey);
    if (specItr == clsItr->second.end())
    {
        return false;
    }

    string tier = GetCuratedTier(bot);
    map<string, map<string, uint32> >::const_iterator tierItr = specItr->second.find(tier);
    if (tierItr == specItr->second.end())
    {
        return false;
    }

    static const struct { const char* key; uint8 slot; } slotMap[] =
    {
        { "head",     EQUIPMENT_SLOT_HEAD },
        { "neck",     EQUIPMENT_SLOT_NECK },
        { "shoulder", EQUIPMENT_SLOT_SHOULDERS },
        { "back",     EQUIPMENT_SLOT_BACK },
        { "chest",    EQUIPMENT_SLOT_CHEST },
        { "wrist",    EQUIPMENT_SLOT_WRISTS },
        { "hands",    EQUIPMENT_SLOT_HANDS },
        { "waist",    EQUIPMENT_SLOT_WAIST },
        { "legs",     EQUIPMENT_SLOT_LEGS },
        { "feet",     EQUIPMENT_SLOT_FEET },
        { "finger1",  EQUIPMENT_SLOT_FINGER1 },
        { "finger2",  EQUIPMENT_SLOT_FINGER2 },
        { "trinket1", EQUIPMENT_SLOT_TRINKET1 },
        { "trinket2", EQUIPMENT_SLOT_TRINKET2 },
        { "mainhand", EQUIPMENT_SLOT_MAINHAND },
        { "offhand",  EQUIPMENT_SLOT_OFFHAND },
        { "ranged",   EQUIPMENT_SLOT_RANGED },
        // Cata: paladin/shaman/druid/death knight relics occupy the ranged
        // slot. A spec's set carries either "ranged" or "relic", never both.
        { "relic",    EQUIPMENT_SLOT_RANGED },
        // Priest wands (INVTYPE_RANGEDRIGHT) also occupy the ranged slot.
        { "wand",     EQUIPMENT_SLOT_RANGED },
    };

    uint32 equipped = 0;
    for (size_t i = 0; i < sizeof(slotMap) / sizeof(slotMap[0]); ++i)
    {
        map<string, uint32>::const_iterator slotItr = tierItr->second.find(slotMap[i].key);
        if (slotItr == tierItr->second.end())
        {
            continue;
        }

        uint32 itemId = slotItr->second;
        uint8 slot = slotMap[i].slot;

        // Safety net: the dataset is class-correct, but still verify the
        // bot can actually equip this item before touching its gear.
        uint16 dest = 0;
        if (!CanEquipUnseenItem(slot, dest, itemId))
        {
            sLog.outDebug("%s: curated gear item %u cannot be equipped in slot %u, skipping",
                bot->GetName(), itemId, (uint32)slot);
            continue;
        }

        Item* oldItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (oldItem)
        {
            bot->RemoveItem(INVENTORY_SLOT_BAG_0, slot, true);
            oldItem->DestroyForPlayer(bot);
        }

        Item* newItem = bot->EquipNewItem(dest, itemId, true);
        if (newItem)
        {
            newItem->AddToWorld();
            newItem->AddToUpdateQueueOf(bot);
            bot->AutoUnequipOffhandIfNeed();
            ++equipped;
            filledSlots.insert(slot);

            // Phase B/C: enchant + socket this curated-equipped item only.
            if (sPlayerbotAIConfig.useCuratedGearEnhancements)
            {
                ApplyCuratedEnchant(specKey, slotMap[i].key, newItem);
                ApplyCuratedGems(specKey, newItem);
            }
        }
        else
        {
            sLog.outDebug("%s: failed to equip curated gear item %u in slot %u",
                bot->GetName(), itemId, (uint32)slot);
        }
    }

    if (equipped)
    {
        sLog.outDetail("%s: curated gear applied (class %u, spec %s, tier %s), %u slots equipped",
            bot->GetName(), (uint32)bot->getClass(), specKey.c_str(), tier.c_str(), equipped);
    }

    return equipped > 0;
}

/**
 * Initializes the player bot's equipment.
 * @param incremental Whether to apply incremental changes.
 */
void PlayerbotFactory::InitEquipment(bool incremental)
{
    DestroyItemsVisitor visitor(bot);
    IterateItems(&visitor, ITERATE_ALL_ITEMS);

    // Phase A: data-driven best-in-slot gear for the bot's class/spec/tier.
    // On success, the legacy loop below still runs, but restricted to
    // weapon slots the curated set left empty (e.g. hunter melee
    // main-hand when the curated set only carries a ranged weapon).
    // Falls back to the full legacy gear-score loop when no curated set
    // exists (or AiPlayerbot.UseCuratedGear is off).
    set<uint8> filledSlots;
    bool curatedApplied = InitCuratedGear(filledSlots);

    static const uint8 weaponSlots[] =
    {
        EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND, EQUIPMENT_SLOT_RANGED
    };

    vector<uint8> slotsToFill;
    if (curatedApplied)
    {
        for (size_t i = 0; i < sizeof(weaponSlots) / sizeof(weaponSlots[0]); ++i)
        {
            uint8 slot = weaponSlots[i];
            if (filledSlots.find(slot) == filledSlots.end() &&
                !bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            {
                slotsToFill.push_back(slot);
            }
        }

        if (slotsToFill.empty())
        {
            return;
        }
    }
    else
    {
        for (uint8 slot = 0; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (slot == EQUIPMENT_SLOT_TABARD || slot == EQUIPMENT_SLOT_BODY)
            {
                continue;
            }

            slotsToFill.push_back(slot);
        }
    }

    map<uint8, vector<uint32> > items;
    for (size_t slotIdx = 0; slotIdx < slotsToFill.size(); ++slotIdx)
    {
        uint8 slot = slotsToFill[slotIdx];

        uint32 desiredQuality = itemQuality;
        if (urand(0, 100) < 100 * sPlayerbotAIConfig.randomGearLoweringChance && desiredQuality > ITEM_QUALITY_NORMAL)
        {
            desiredQuality--;
        }

        do
        {
            for (uint32 itemId = 0; itemId < sItemStorage.GetMaxEntry(); ++itemId)
            {
                ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
                if (!proto)
                {
                    continue;
                }

                if (proto->Class != ITEM_CLASS_WEAPON &&
                    proto->Class != ITEM_CLASS_ARMOR &&
                    proto->Class != ITEM_CLASS_CONTAINER &&
                    proto->Class != ITEM_CLASS_PROJECTILE)
                    continue;

                if (!CanEquipItem(proto, desiredQuality))
                {
                    continue;
                }

                if (proto->Class == ITEM_CLASS_ARMOR && (
                    slot == EQUIPMENT_SLOT_HEAD ||
                    slot == EQUIPMENT_SLOT_SHOULDERS ||
                    slot == EQUIPMENT_SLOT_CHEST ||
                    slot == EQUIPMENT_SLOT_WAIST ||
                    slot == EQUIPMENT_SLOT_LEGS ||
                    slot == EQUIPMENT_SLOT_FEET ||
                    slot == EQUIPMENT_SLOT_WRISTS ||
                    slot == EQUIPMENT_SLOT_HANDS) && !CanEquipArmor(proto))
                {
                    continue;
                }

                if (proto->Class == ITEM_CLASS_WEAPON && !CanEquipWeapon(proto))
                {
                    continue;
                }

                if (slot == EQUIPMENT_SLOT_OFFHAND && bot->getClass() == CLASS_ROGUE && proto->Class != ITEM_CLASS_WEAPON)
                {
                    continue;
                }
                if (slot == EQUIPMENT_SLOT_OFFHAND && bot->getClass() == CLASS_PALADIN && proto->SubClass != ITEM_SUBCLASS_ARMOR_SHIELD)
                {
                    continue;
                }

                uint16 dest = 0;
                if (CanEquipUnseenItem(slot, dest, itemId))
                {
                    items[slot].push_back(itemId);
                }
            }
        } while (items[slot].empty() && desiredQuality-- > ITEM_QUALITY_NORMAL);
    }

    for (size_t slotIdx = 0; slotIdx < slotsToFill.size(); ++slotIdx)
    {
        uint8 slot = slotsToFill[slotIdx];

        vector<uint32>& ids = items[slot];
        if (ids.empty())
        {
            sLog.outDebug("%s: no items to equip for slot %d", bot->GetName(), slot);
            continue;
        }

        for (int attempts = 0; attempts < 15; attempts++)
        {
            uint32 index = urand(0, ids.size() - 1);
            uint32 newItemId = ids[index];
            Item* oldItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);

            if (incremental && !IsDesiredReplacement(oldItem))
            {
                continue;
            }

            uint16 dest;
            if (!CanEquipUnseenItem(slot, dest, newItemId))
            {
                continue;
            }

            if (oldItem)
            {
                bot->RemoveItem(INVENTORY_SLOT_BAG_0, slot, true);
                oldItem->DestroyForPlayer(bot);
            }

            Item* newItem = bot->EquipNewItem(dest, newItemId, true);
            if (newItem)
            {
                newItem->AddToWorld();
                newItem->AddToUpdateQueueOf(bot);
                bot->AutoUnequipOffhandIfNeed();
                EnchantItem(newItem);
                break;
            }
        }
    }
}
/**
 * Checks if the given item is a desired replacement for the current item.
 * @param item The current item.
 * @return True if the item is a desired replacement, false otherwise.
 */
bool PlayerbotFactory::IsDesiredReplacement(Item* item)
{
    if (!item)
    {
        return true;
    }

    ItemPrototype const* proto = item->GetProto();
    int delta = 1 + (80 - bot->getLevel()) / 10;
    return (int)bot->getLevel() - (int)proto->RequiredLevel > delta;
}

/**
 * Initializes the second equipment set for the player bot.
 */
void PlayerbotFactory::InitSecondEquipmentSet()
{
    // Skip for classes that do not need a second equipment set
    if (bot->getClass() == CLASS_MAGE || bot->getClass() == CLASS_WARLOCK || bot->getClass() == CLASS_PRIEST)
    {
        return;
    }

    map<uint32, vector<uint32> > items;

    uint32 desiredQuality = itemQuality;
    while (urand(0, 100) < 100 * sPlayerbotAIConfig.randomGearLoweringChance && desiredQuality > ITEM_QUALITY_NORMAL) {
        desiredQuality--;
    }

    do
    {
        for (uint32 itemId = 0; itemId < sItemStorage.GetMaxEntry(); ++itemId)
        {
            ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
            if (!proto)
            {
                continue;
            }

            if (!CanEquipItem(proto, desiredQuality))
            {
                continue;
            }

            if (proto->Class == ITEM_CLASS_WEAPON)
            {
                if (!CanEquipWeapon(proto))
                {
                    continue;
                }

                Item* existingItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
                if (existingItem)
                {
                    switch (existingItem->GetProto()->SubClass)
                    {
                        case ITEM_SUBCLASS_WEAPON_AXE:
                        case ITEM_SUBCLASS_WEAPON_DAGGER:
                        case ITEM_SUBCLASS_WEAPON_FIST:
                        case ITEM_SUBCLASS_WEAPON_MACE:
                        case ITEM_SUBCLASS_WEAPON_SWORD:
                            if (proto->SubClass == ITEM_SUBCLASS_WEAPON_AXE || proto->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER ||
                                proto->SubClass == ITEM_SUBCLASS_WEAPON_FIST || proto->SubClass == ITEM_SUBCLASS_WEAPON_MACE ||
                                proto->SubClass == ITEM_SUBCLASS_WEAPON_SWORD)
                                continue;
                            break;
                        default:
                            if (proto->SubClass != ITEM_SUBCLASS_WEAPON_AXE && proto->SubClass != ITEM_SUBCLASS_WEAPON_DAGGER &&
                                proto->SubClass != ITEM_SUBCLASS_WEAPON_FIST && proto->SubClass != ITEM_SUBCLASS_WEAPON_MACE &&
                                proto->SubClass != ITEM_SUBCLASS_WEAPON_SWORD)
                                continue;
                            break;
                    }
                }
            }
            else if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == ITEM_SUBCLASS_ARMOR_SHIELD)
            {
                if (!CanEquipArmor(proto))
                {
                    continue;
                }

                Item* existingItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
                if (existingItem && existingItem->GetProto()->SubClass == ITEM_SUBCLASS_ARMOR_SHIELD)
                {
                    continue;
                }
            }
            else
            {
                continue;
            }

            items[proto->Class].push_back(itemId);
        }
    } while (items[ITEM_CLASS_ARMOR].empty() && items[ITEM_CLASS_WEAPON].empty() && desiredQuality-- > ITEM_QUALITY_NORMAL);

    for (map<uint32, vector<uint32> >::iterator i = items.begin(); i != items.end(); ++i)
    {
        vector<uint32>& ids = i->second;
        if (ids.empty())
        {
            sLog.outDebug(  "%s: no items to make second equipment set for slot %d", bot->GetName(), i->first);
            continue;
        }

        for (int attempts = 0; attempts < 15; attempts++)
        {
            uint32 index = urand(0, ids.size() - 1);
            uint32 newItemId = ids[index];

            Item* newItem = bot->StoreNewItemInInventorySlot(newItemId, 1);
            if (newItem)
            {
                EnchantItem(newItem);
                newItem->AddToWorld();
                newItem->AddToUpdateQueueOf(bot);
                break;
            }
        }
    }
}

/**
 * Initializes the bags for the player bot.
 */
void PlayerbotFactory::InitBags()
{
    vector<uint32> ids;

    for (uint32 itemId = 0; itemId < sItemStorage.GetMaxEntry(); ++itemId)
    {
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        if (!proto || proto->Class != ITEM_CLASS_CONTAINER)
        {
            continue;
        }

        if (!CanEquipItem(proto, ITEM_QUALITY_NORMAL))
        {
            continue;
        }

        ids.push_back(itemId);
    }

    if (ids.empty())
    {
        sLog.outError("%s: no bags found", bot->GetName());
        return;
    }

    for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
    {
        for (int attempts = 0; attempts < 15; attempts++)
        {
            uint32 index = urand(0, ids.size() - 1);
            uint32 newItemId = ids[index];

            uint16 dest;
            if (!CanEquipUnseenItem(slot, dest, newItemId))
            {
                continue;
            }

            Item* newItem = bot->EquipNewItem(dest, newItemId, true);
            if (newItem)
            {
                newItem->AddToWorld();
                newItem->AddToUpdateQueueOf(bot);
                break;
            }
        }
    }
}

/**
 * Enchants the given item for the player bot.
 * @param item The item to enchant.
 */
void PlayerbotFactory::EnchantItem(Item* item)
{
    if (urand(0, 100) < 100 * sPlayerbotAIConfig.randomGearLoweringChance)
    {
        return;
    }

    if (bot->getLevel() < urand(40, 50))
    {
        return;
    }

    ItemPrototype const* proto = item->GetProto();
    int32 itemLevel = proto->ItemLevel;

    vector<uint32> ids;
    for (uint32 id = 0; id < sSpellStore.GetNumRows(); ++id)
    {
        SpellEntry const *entry = sSpellStore.LookupEntry(id);
        if (!entry)
        {
            continue;
        }

        int32 requiredLevel = (int32)entry->GetBaseLevel();
        if (requiredLevel && (requiredLevel > itemLevel || requiredLevel < itemLevel - 35))
        {
            continue;
        }

        if (entry->GetMaxLevel() && level > entry->GetMaxLevel())
        {
            continue;
        }

        uint32 spellLevel = entry->GetSpellLevel();
        if (spellLevel && (spellLevel > level || spellLevel < level - 10))
        {
            continue;
        }

        for (int j = 0; j < 3; ++j)
        {
            if (entry->GetSpellEffectIdByIndex(SpellEffectIndex(j)) != SPELL_EFFECT_ENCHANT_ITEM)
            {
                continue;
            }

            uint32 enchant_id = entry->GetEffectMiscValue(SpellEffectIndex(j));
            if (!enchant_id)
            {
                continue;
            }

            SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
            if (!enchant || enchant->slot != PERM_ENCHANTMENT_SLOT)
            {
                continue;
            }

            const SpellEntry *enchantSpell = sSpellStore.LookupEntry(enchant->EffectArg[0]);
            if (!enchantSpell || (enchantSpell->GetSpellLevel() && enchantSpell->GetSpellLevel() > level))
            {
                continue;
            }

            uint8 sp = 0, ap = 0, tank = 0;
            for (int i = 0; i < 3; ++i)
            {
                if (enchant->Effect[i] != ITEM_ENCHANTMENT_TYPE_STAT)
                {
                    continue;
                }

                AddItemStats(enchant->EffectArg[i], sp, ap, tank);
            }

            if (!CheckItemStats(sp, ap, tank))
            {
                continue;
            }

            if (!item->IsFitToSpellRequirements(entry))
            {
                continue;
            }

            ids.push_back(enchant_id);
        }
    }

    if (ids.empty())
    {
        sLog.outDebug(  "%s: no enchantments found for item %d", bot->GetName(), item->GetProto()->ItemId);
        return;
    }

    int index = urand(0, ids.size() - 1);
    uint32 id = ids[index];

    SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(id);
    if (!enchant)
    {
        return;
    }

    bot->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, false);
    item->SetEnchantment(PERM_ENCHANTMENT_SLOT, id, 0, 0);
    bot->ApplyEnchantment(item, PERM_ENCHANTMENT_SLOT, true);
}

/**
 * Checks if the player bot can equip the specified unseen item.
 * @param slot The equipment slot.
 * @param dest The destination slot.
 * @param item The item ID.
 * @return True if the player bot can equip the item, false otherwise.
 */
bool PlayerbotFactory::CanEquipUnseenItem(uint8 slot, uint16 &dest, uint32 item)
{
    dest = 0;
    Item *pItem = Item::CreateItem(item, 1, bot);
    if (pItem)
    {
        InventoryResult result = bot->CanEquipItem(slot, dest, pItem, true, false);
        pItem->RemoveFromUpdateQueueOf(bot);
        delete pItem;
        return result == EQUIP_ERR_OK;
    }

    return false;
}

/**
 * Initializes the trade skills for the player bot.
 */
#define PLAYER_SKILL_INDEX(x)       (PLAYER_SKILL_LINEID_0 + ((x)*3))
void PlayerbotFactory::InitTradeSkills()
{
    for (int i = 0; i < sizeof(tradeSkills) / sizeof(uint32); ++i)
    {
        bot->SetSkill(tradeSkills[i], 0, 0, 0);
    }

    bot->SetUInt32Value(PLAYER_SKILL_INDEX(0), 0);
    bot->SetUInt32Value(PLAYER_SKILL_INDEX(1), 0);

    vector<uint32> firstSkills;
    vector<uint32> secondSkills;
    switch (bot->getClass())
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
            firstSkills.push_back(SKILL_MINING);
            secondSkills.push_back(SKILL_BLACKSMITHING);
            secondSkills.push_back(SKILL_ENGINEERING);
            break;
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        case CLASS_HUNTER:
        case CLASS_ROGUE:
            firstSkills.push_back(SKILL_SKINNING);
            secondSkills.push_back(SKILL_LEATHERWORKING);
            break;
        default:
            firstSkills.push_back(SKILL_TAILORING);
            secondSkills.push_back(SKILL_ENCHANTING);
    }

    SetRandomSkill(SKILL_FIRST_AID);
    SetRandomSkill(SKILL_FISHING);
    SetRandomSkill(SKILL_COOKING);

    switch (urand(0, 3))
    {
        case 0:
            SetRandomSkill(SKILL_HERBALISM);
            SetRandomSkill(SKILL_ALCHEMY);
            break;
        case 1:
            SetRandomSkill(SKILL_HERBALISM);
            break;
        case 2:
            SetRandomSkill(SKILL_MINING);
#if !defined(CLASSIC)
            SetRandomSkill(SKILL_JEWELCRAFTING);
#endif
            break;
        case 3:
            SetRandomSkill(firstSkills[urand(0, firstSkills.size() - 1)]);
            SetRandomSkill(secondSkills[urand(0, secondSkills.size() - 1)]);
            break;
    }
}

/**
 * Updates the trade skills for the player bot.
 */
void PlayerbotFactory::UpdateTradeSkills()
{
    for (int i = 0; i < sizeof(tradeSkills) / sizeof(uint32); ++i)
    {
        if (bot->GetSkillValue(tradeSkills[i]) == 1)
        {
            bot->SetSkill(tradeSkills[i], 0, 0, 0);
        }
    }
}

/**
 * Initializes the skills for the player bot based on its class and level.
 */
void PlayerbotFactory::InitSkills() //Cosine
{
    uint32 maxValue = level * 5;
    if (bot->getLevel() >= 70)
    {
        bot->SetSkill(SKILL_RIDING, 300, 300);
    }
    else if (bot->getLevel() >= 60)
    {
        bot->SetSkill(SKILL_RIDING, 225, 225);
    }
    else if (bot->getLevel() >= 40)
    {
        bot->SetSkill(SKILL_RIDING, 150, 150);
    }
    else if (bot->getLevel() >= 20)
    {
        bot->SetSkill(SKILL_RIDING, 75, 75);
    }
    else
    {
        bot->SetSkill(SKILL_RIDING, 0, 0);
    }

    uint32 skillLevel = bot->getLevel() < 40 ? 0 : 1;
    switch (bot->getClass())
    {
        case CLASS_DRUID:
            SetRandomSkill(SKILL_MACES);
            SetRandomSkill(SKILL_STAVES);
            SetRandomSkill(SKILL_2H_MACES);
            SetRandomSkill(SKILL_DAGGERS);
            SetRandomSkill(SKILL_POLEARMS);
            SetRandomSkill(SKILL_FIST_WEAPONS);
            break;
        case CLASS_WARRIOR:
            SetRandomSkill(SKILL_SWORDS);
            SetRandomSkill(SKILL_AXES);
            SetRandomSkill(SKILL_BOWS);
            SetRandomSkill(SKILL_GUNS);
            SetRandomSkill(SKILL_MACES);
            SetRandomSkill(SKILL_2H_SWORDS);
            SetRandomSkill(SKILL_STAVES);
            SetRandomSkill(SKILL_2H_MACES);
            SetRandomSkill(SKILL_2H_AXES);
            SetRandomSkill(SKILL_DAGGERS);
            SetRandomSkill(SKILL_CROSSBOWS);
            SetRandomSkill(SKILL_POLEARMS);
            SetRandomSkill(SKILL_FIST_WEAPONS);
            SetRandomSkill(SKILL_THROWN);
            break;
        case CLASS_PALADIN:
            bot->SetSkill(SKILL_PLATE_MAIL, 0, skillLevel, skillLevel);
            SetRandomSkill(SKILL_SWORDS);
            SetRandomSkill(SKILL_AXES);
            SetRandomSkill(SKILL_MACES);
            SetRandomSkill(SKILL_2H_SWORDS);
            SetRandomSkill(SKILL_2H_MACES);
            SetRandomSkill(SKILL_2H_AXES);
            SetRandomSkill(SKILL_POLEARMS);
            break;
        case CLASS_PRIEST:
            SetRandomSkill(SKILL_MACES);
            SetRandomSkill(SKILL_STAVES);
            SetRandomSkill(SKILL_DAGGERS);
            SetRandomSkill(SKILL_WANDS);
            break;
        case CLASS_SHAMAN:
            SetRandomSkill(SKILL_AXES);
            SetRandomSkill(SKILL_MACES);
            SetRandomSkill(SKILL_STAVES);
            SetRandomSkill(SKILL_2H_MACES);
            SetRandomSkill(SKILL_2H_AXES);
            SetRandomSkill(SKILL_DAGGERS);
            SetRandomSkill(SKILL_FIST_WEAPONS);
            break;
        case CLASS_MAGE:
             SetRandomSkill(SKILL_SWORDS);
             SetRandomSkill(SKILL_STAVES);
             SetRandomSkill(SKILL_DAGGERS);
             SetRandomSkill(SKILL_WANDS);
             break;
        case CLASS_WARLOCK:
             SetRandomSkill(SKILL_SWORDS);
             SetRandomSkill(SKILL_STAVES);
             SetRandomSkill(SKILL_DAGGERS);
             SetRandomSkill(SKILL_WANDS);
             break;
        case CLASS_HUNTER:
             SetRandomSkill(SKILL_SWORDS);
             SetRandomSkill(SKILL_AXES);
             SetRandomSkill(SKILL_BOWS);
             SetRandomSkill(SKILL_GUNS);
             SetRandomSkill(SKILL_2H_SWORDS);
             SetRandomSkill(SKILL_STAVES);
             SetRandomSkill(SKILL_2H_AXES);
             SetRandomSkill(SKILL_DAGGERS);
             SetRandomSkill(SKILL_CROSSBOWS);
             SetRandomSkill(SKILL_POLEARMS);
             SetRandomSkill(SKILL_FIST_WEAPONS);
             SetRandomSkill(SKILL_THROWN);
             bot->SetSkill(SKILL_MAIL, 0, skillLevel, skillLevel);
             break;
       case CLASS_ROGUE:
             SetRandomSkill(SKILL_SWORDS);
             SetRandomSkill(SKILL_AXES);
             SetRandomSkill(SKILL_BOWS);
             SetRandomSkill(SKILL_GUNS);
             SetRandomSkill(SKILL_MACES);
             SetRandomSkill(SKILL_DAGGERS);
             SetRandomSkill(SKILL_CROSSBOWS);
             SetRandomSkill(SKILL_FIST_WEAPONS);
             SetRandomSkill(SKILL_THROWN);
    }
}

/**
 * Sets a random skill value for the player bot.
 * @param id The skill ID.
 */
void PlayerbotFactory::SetRandomSkill(uint16 id)
{
    uint32 maxValue = level * 5;
    uint32 curValue = urand(maxValue - level, maxValue);
    bot->SetSkill(id, curValue, maxValue);
}

/**
 * Initializes the available spells for the player bot.
 */
void PlayerbotFactory::InitAvailableSpells()
{
    // Learn default spells for the bot
    bot->learnDefaultSpells();

    // Iterate through all creature entries
    for (uint32 id = 0; id < sCreatureStorage.GetMaxEntry(); ++id)
    {
        CreatureInfo const* co = sCreatureStorage.LookupEntry<CreatureInfo>(id);
        if (!co)
        {
            continue;
        }

        // Check if the creature is a trainer for tradeskills or class
        if (co->TrainerType != TRAINER_TYPE_TRADESKILLS && co->TrainerType != TRAINER_TYPE_CLASS)
        {
            continue;
        }

        // Check if the trainer's class matches the bot's class
        if (co->TrainerType == TRAINER_TYPE_CLASS && co->TrainerClass != bot->getClass())
        {
            continue;
        }

        uint32 trainerId = co->TrainerTemplateId;
        if (!trainerId)
        {
            trainerId = co->Entry;
        }

        // Get the trainer's spells
        TrainerSpellData const* trainer_spells = sObjectMgr.GetNpcTrainerTemplateSpells(trainerId);
        if (!trainer_spells)
        {
            trainer_spells = sObjectMgr.GetNpcTrainerSpells(trainerId);
        }

        if (!trainer_spells)
        {
            continue;
        }

        // Iterate through the trainer's spells and learn them if the bot meets the requirements
        for (TrainerSpellMap::const_iterator itr = trainer_spells->spellList.begin(); itr != trainer_spells->spellList.end(); ++itr)
        {
            TrainerSpell const* tSpell = &itr->second;

            if (!tSpell)
            {
                continue;
            }

            uint32 reqLevel = 0;

            reqLevel = tSpell->isProvidedReqLevel ? tSpell->reqLevel : std::max(reqLevel, tSpell->reqLevel);
            TrainerSpellState state = bot->GetTrainerSpellState(tSpell, reqLevel);
            if (state != TRAINER_SPELL_GREEN)
            {
                continue;
            }

            bot->learnSpell(tSpell->spell, false);
        }
    }
}

/**
 * Initializes special spells for the player bot.
 */
void PlayerbotFactory::InitSpecialSpells()
{
    // Iterate through the list of random bot spell IDs and learn each spell
    for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotSpellIds.begin(); i != sPlayerbotAIConfig.randomBotSpellIds.end(); ++i)
    {
        uint32 spellId = *i;
        bot->learnSpell(spellId, false);
    }
}

/**
 * Initializes quest-reward and auto-learned spells for the player bot.
 * These are class-specific spells that aren't learned from trainers.
 */
void PlayerbotFactory::InitQuestSpells()
{
    uint8 cls = bot->getClass();
    uint32 level = bot->getLevel();

    switch (cls)
    {
        case CLASS_HUNTER:
            if (level >= 10)
            {
                if (!bot->HasSpell(883))   bot->learnSpell(883, false);   // Call Pet
                if (!bot->HasSpell(982))   bot->learnSpell(982, false);   // Revive Pet
                if (!bot->HasSpell(1515))  bot->learnSpell(1515, false);  // Tame Beast
                if (!bot->HasSpell(6991))  bot->learnSpell(6991, false);  // Feed Pet
                if (!bot->HasSpell(5149))  bot->learnSpell(5149, false);  // Beast Training
            }
            if (level >= 12)
            {
                if (!bot->HasSpell(136))   bot->learnSpell(136, false);   // Mend Pet
            }
            break;

        case CLASS_WARLOCK:
            if (level >= 1)
            {
                if (!bot->HasSpell(688))   bot->learnSpell(688, false);   // Summon Imp
            }
            if (level >= 10)
            {
                if (!bot->HasSpell(697))   bot->learnSpell(697, false);   // Summon Voidwalker
            }
            if (level >= 20)
            {
                if (!bot->HasSpell(712))   bot->learnSpell(712, false);   // Summon Succubus
            }
            if (level >= 30)
            {
                if (!bot->HasSpell(691))   bot->learnSpell(691, false);   // Summon Felhunter
            }
            break;

        case CLASS_WARRIOR:
            if (level >= 10)
            {
                if (!bot->HasSpell(71))    bot->learnSpell(71, false);    // Defensive Stance
            }
            if (level >= 30)
            {
                if (!bot->HasSpell(2458))  bot->learnSpell(2458, false);  // Berserker Stance
            }
            break;

        case CLASS_DRUID:
            if (level >= 10)
            {
                if (!bot->HasSpell(5487))  bot->learnSpell(5487, false);  // Bear Form
            }
            if (level >= 16)
            {
                if (!bot->HasSpell(1066))  bot->learnSpell(1066, false);  // Aquatic Form
            }
            if (level >= 20)
            {
                if (!bot->HasSpell(768))   bot->learnSpell(768, false);   // Cat Form
            }
            if (level >= 30)
            {
                if (!bot->HasSpell(783))   bot->learnSpell(783, false);   // Travel Form
            }
            if (level >= 40)
            {
                if (!bot->HasSpell(9634))  bot->learnSpell(9634, false);  // Dire Bear Form
            }
            break;

        case CLASS_PALADIN:
            if (level >= 12)
            {
                if (!bot->HasSpell(7328))  bot->learnSpell(7328, false);  // Redemption
            }
            if (level >= 40)
            {
                if (!bot->HasSpell(13819)) bot->learnSpell(13819, false); // Summon Warhorse
            }
            if (level >= 60)
            {
                if (!bot->HasSpell(23214)) bot->learnSpell(23214, false); // Summon Charger
            }
            // Horde-specific
            if (bot->GetTeam() == HORDE && level >= 50)
            {
                if (!bot->HasSpell(31892)) bot->learnSpell(31892, false); // Seal of Blood
            }
            break;

        case CLASS_SHAMAN:
            // Totem quests - these vary by race/level but add common ones
            if (level >= 10)
            {
                if (!bot->HasSpell(8012))  bot->learnSpell(8012, false);  // Purge (often quest)
            }
            break;
        case CLASS_PRIEST:
            break;
        case CLASS_ROGUE:
            break;
        case CLASS_MAGE:
            break;
        default:
            break;
    }
}

/**
 * Initializes the talents for the player bot based on the specified specialization number.
 * @param specNo The specialization number.
 */
void PlayerbotFactory::InitTalents(uint32 specNo)
{
    uint32 classMask = bot->getClassMask();

    // Map to store spells by talent row
    map<uint32, vector<TalentEntry const*> > spells;
    for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
    {
        TalentEntry const *talentInfo = sTalentStore.LookupEntry(i);
        if (!talentInfo)
        {
            continue;
        }

        TalentTabEntry const *talentTabInfo = sTalentTabStore.LookupEntry( talentInfo->TabID );
        if (!talentTabInfo || talentTabInfo->OrderIndex != specNo)
        {
            continue;
        }

        if ( (classMask & talentTabInfo->ClassMask) == 0 )
        {
            continue;
        }

        spells[talentInfo->TierID].push_back(talentInfo);
    }

    // Spend points row by row through Player::LearnTalent so every Cata rule
    // applies (5-point row gates, 31-point primary tree, prerequisites) and
    // the first learned talent sets the primary tree + mastery.
    bool progress = true;
    while (bot->GetFreeTalentPoints() && progress)
    {
        progress = false;
        for (map<uint32, vector<TalentEntry const*> >::iterator i = spells.begin(); i != spells.end() && bot->GetFreeTalentPoints(); ++i)
        {
            vector<TalentEntry const*> &rowTalents = i->second;
            vector<TalentEntry const*> shuffled = rowTalents;
            for (size_t j = shuffled.size(); j > 1; --j)
            {
                std::swap(shuffled[j - 1], shuffled[urand(0, j - 1)]);
            }

            for (vector<TalentEntry const*>::iterator t = shuffled.begin(); t != shuffled.end() && bot->GetFreeTalentPoints(); ++t)
            {
                TalentEntry const* talentInfo = *t;
                for (int rank = 0; rank < MAX_TALENT_RANK && bot->GetFreeTalentPoints(); ++rank)
                {
                    if (!talentInfo->SpellRank[rank])
                    {
                        continue;
                    }
                    if (bot->LearnTalent(talentInfo->ID, rank))
                    {
                        progress = true;
                    }
                }
            }
        }
    }
}

/**
 * Initializes glyphs for the player bot: fills every unlocked glyph slot with a
 * random class-appropriate glyph of the matching type.
 */
void PlayerbotFactory::InitGlyphs()
{
    bot->InitGlyphsForLevel();

    uint32 classMask = bot->getClassMask();

    // Build glyph candidates per slot type (0/1/2) by scanning the class's
    // glyph items and resolving each item's use-spell APPLY_GLYPH effect.
    vector<uint32> candidates[3];
    for (uint32 itemId = 0; itemId < sItemStorage.GetMaxEntry(); ++itemId)
    {
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        if (!proto || proto->Class != ITEM_CLASS_GLYPH)
        {
            continue;
        }

        if (proto->AllowableClass && !(proto->AllowableClass & classMask))
        {
            continue;
        }

        // The glyph-apply spell is the one carried in the LEARN_SPELL_ID slot
        // (spell_2); spell_1 is the generic SPELL_GENERIC_LEARN wrapper.
        uint32 useSpell = 0;
        for (int s = 0; s < MAX_ITEM_PROTO_SPELLS; ++s)
        {
            if (proto->Spells[s].SpellTrigger == ITEM_SPELLTRIGGER_LEARN_SPELL_ID && proto->Spells[s].SpellId)
            {
                useSpell = proto->Spells[s].SpellId;
                break;
            }
        }

        SpellEntry const* spellInfo = sSpellStore.LookupEntry(useSpell);
        if (!spellInfo)
        {
            continue;
        }

        for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            if (spellInfo->GetSpellEffectIdByIndex(SpellEffectIndex(j)) != SPELL_EFFECT_APPLY_GLYPH)
            {
                continue;
            }

            uint32 glyphId = spellInfo->GetEffectMiscValue(SpellEffectIndex(j));
            GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyphId);
            if (gp && gp->GlyphSlotFlags < 3)
            {
                candidates[gp->GlyphSlotFlags].push_back(glyphId);
            }
        }
    }

    for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot)
    {
        GlyphSlotEntry const* gs = sGlyphSlotStore.LookupEntry(bot->GetGlyphSlot(slot));
        if (!gs || gs->Type >= 3)
        {
            continue;
        }

        vector<uint32>& pool = candidates[gs->Type];
        if (pool.empty())
        {
            continue;
        }

        uint32 glyphId = pool[urand(0, pool.size() - 1)];
        bot->ApplyGlyph(slot, false);
        bot->SetGlyph(slot, glyphId);
        bot->ApplyGlyph(slot, true);
    }

    bot->SendTalentsInfoData(false);
}

/**
 * Retrieves a random bot from the list of random bot accounts.
 * @return The GUID of the random bot.
 */
ObjectGuid PlayerbotFactory::GetRandomBot()
{
    vector<ObjectGuid> guids;
    // Iterate through the list of random bot accounts
    for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotAccounts.begin(); i != sPlayerbotAIConfig.randomBotAccounts.end(); i++)
    {
        uint32 accountId = *i;
        // Check if the account has any characters
        if (!sAccountMgr.GetCharactersCount(accountId))
        {
            continue;
        }

        // Query the database for character GUIDs associated with the account
        QueryResult *result = CharacterDatabase.PQuery("SELECT `guid` FROM `characters` WHERE `account` = '%u'", accountId);
        if (!result)
        {
            continue;
        }

        // Add the character GUIDs to the list if they are not already in use
        do
        {
            Field* fields = result->Fetch();
            ObjectGuid guid = ObjectGuid(fields[0].GetUInt64());
            if (!sObjectMgr.GetPlayer(guid))
            {
                guids.push_back(guid);
            }
        } while (result->NextRow());

        delete result;
    }

    // Return a random GUID from the list
    if (guids.empty())
    {
        return ObjectGuid();
    }

    int index = urand(0, guids.size() - 1);
    return guids[index];
}

/**
 * Adds previous quests to the list of quest IDs.
 * @param questId The quest ID.
 * @param questIds The list of quest IDs.
 */
void AddPrevQuests(uint32 questId, list<uint32>& questIds, std::set<uint32>& visited)
{
    // Guard against cyclic / self-referential quest prerequisite chains in the
    // world DB, which would otherwise recurse forever and hang the world thread.
    if (!visited.insert(questId).second)
    {
        return;
    }

    Quest const *quest = sObjectMgr.GetQuestTemplate(questId);
    if (!quest)
    {
        return;
    }

    // Recursively add previous quests to the list
    for (Quest::PrevQuests::const_iterator iter = quest->prevQuests.begin(); iter != quest->prevQuests.end(); ++iter)
    {
        uint32 prevId = abs(*iter);
        AddPrevQuests(prevId, questIds, visited);
        questIds.remove(prevId);
        questIds.push_back(prevId);
    }
}

/**
 * Initializes the quests for the player bot.
 */
void PlayerbotFactory::InitQuests()
{
    // RewardQuest -> GetZoneAndAreaId -> GetTerrain dereferences m_currMap. A bot
    // whose login never completed a Map::Add (bad saved position: an unloaded
    // instance, Vashjir, a broken WMO) is still ticked by Player::Update but has no
    // map, so this would null-deref and crash the world thread. Skip the quest pass
    // for such a bot; every other randomize step (talents, spells, gear, glyphs) is
    // map-independent, so the bot is still fully geared.
    if (!bot->GetMap())
    {
        return;
    }

    // Initialize the list of class-specific quest IDs if it is empty
    if (classQuestIds.empty())
    {
        ObjectMgr::QuestMap const& questTemplates = sObjectMgr.GetQuestTemplates();
        for (ObjectMgr::QuestMap::const_iterator i = questTemplates.begin(); i != questTemplates.end(); ++i)
        {
            uint32 questId = i->first;
            Quest const *quest = i->second;

            if (!quest->GetRequiredClasses() || quest->IsRepeatable())
            {
                continue;
            }

            std::set<uint32> visitedQuests;
            AddPrevQuests(questId, classQuestIds, visitedQuests);
            classQuestIds.remove(questId);
            classQuestIds.push_back(questId);
        }
    }

    int count = 0;
    // Iterate through the list of class-specific quest IDs and complete the quests for the bot
    for (list<uint32>::iterator i = classQuestIds.begin(); i != classQuestIds.end(); ++i)
    {
        uint32 questId = *i;
        Quest const *quest = sObjectMgr.GetQuestTemplate(questId);

        if (!bot->SatisfyQuestClass(quest, false) ||
                quest->GetMinLevel() > bot->getLevel() ||
                !bot->SatisfyQuestRace(quest, false))
            continue;

        bot->SetQuestStatus(questId, QUEST_STATUS_COMPLETE);
        bot->RewardQuest(quest, 0, bot, false);
        // A quest reward spell can teleport the bot off its current map, leaving it
        // unmapped. Stop force-completing quests once that happens: the remaining
        // randomize steps (gear/talents/spells) are map-independent and the caller's
        // RandomTeleportForLevel re-anchors the bot immediately afterwards.
        if (!bot->GetMap())
        {
            break;
        }
        if (!(count++ % 10))
        {
            ClearInventory();
        }
    }
    ClearInventory();
}

/**
 * Clears the inventory of the player bot.
 */
void PlayerbotFactory::ClearInventory()
{
    DestroyItemsVisitor visitor(bot);
    IterateItems(&visitor);
}

/**
 * Initializes the mounts for the player bot.
 */
void PlayerbotFactory::InitMounts()
{
    map<int32, vector<uint32> > spells;

    // Iterate through all spell entries and find mount spells
    for (uint32 spellId = 0; spellId < sSpellStore.GetNumRows(); ++spellId)
    {
        SpellEntry const *spellInfo = sSpellStore.LookupEntry(spellId);
        if (!spellInfo || spellInfo->GetEffectApplyAuraNameByIndex(SpellEffectIndex(0)) != SPELL_AURA_MOUNTED)
        {
            continue;
        }

        if (GetSpellCastTime(spellInfo) < 500 || GetSpellDuration(spellInfo) != -1)
        {
            continue;
        }

        int32 effect = max(spellInfo->CalculateSimpleValue(SpellEffectIndex(1)), spellInfo->CalculateSimpleValue(SpellEffectIndex(2)));
        if (effect < 50)
        {
            continue;
        }

        spells[effect].push_back(spellId);
    }

    // Learn a random mount spell for each type of mount
    for (uint32 type = 0; type < 2; ++type)
    {
        for (map<int32, vector<uint32> >::iterator i = spells.begin(); i != spells.end(); ++i)
        {
            int32 effect = i->first;
            vector<uint32>& ids = i->second;
            uint32 index = urand(0, ids.size() - 1);
            if (index >= ids.size())
            {
                continue;
            }

            bot->learnSpell(ids[index], false);
        }
    }
}

/**
 * Initializes the potions for the player bot.
 */
void PlayerbotFactory::InitPotions()
{
    map<uint32, vector<uint32> > items;
    // Iterate through all item entries and find potions that the bot can use
    for (uint32 itemId = 0; itemId < sItemStorage.GetMaxEntry(); ++itemId)
    {
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        if (!proto)
        {
            continue;
        }

        if (proto->Class != ITEM_CLASS_CONSUMABLE ||
            proto->SubClass != ITEM_SUBCLASS_POTION ||
            proto->Spells[0].SpellCategory != 4 ||
            proto->Bonding != NO_BIND)
        {
            continue;
        }

        if (proto->RequiredLevel > bot->getLevel() || proto->RequiredLevel < bot->getLevel() - 10)
        {
            continue;
        }

        if (proto->RequiredSkill && !bot->HasSkill(proto->RequiredSkill))
        {
            continue;
        }

        if (proto->Area || proto->Map || proto->RequiredCityRank || proto->RequiredHonorRank)
        {
            continue;
        }

        // Add the potion to the list of items
        for (int j = 0; j < MAX_ITEM_PROTO_SPELLS; j++)
        {
            const SpellEntry* const spellInfo = sSpellStore.LookupEntry(proto->Spells[j].SpellId);
            if (!spellInfo)
            {
                continue;
            }

            for (int i = 0 ; i < 3; i++)
            {
                if (spellInfo->GetSpellEffectIdByIndex(SpellEffectIndex(i)) == SPELL_EFFECT_HEAL || spellInfo->GetSpellEffectIdByIndex(SpellEffectIndex(i)) == SPELL_EFFECT_ENERGIZE)
                {
                    items[spellInfo->GetSpellEffectIdByIndex(SpellEffectIndex(i))].push_back(itemId);
                    break;
                }
            }
        }
    }

    // Add a random potion to the bot's inventory
    uint32 effects[] = { SPELL_EFFECT_HEAL, SPELL_EFFECT_ENERGIZE };
    for (int i = 0; i < sizeof(effects) / sizeof(uint32); ++i)
    {
        uint32 effect = effects[i];
        vector<uint32>& ids = items[effect];
        uint32 index = urand(0, ids.size() - 1);
        if (index >= ids.size())
        {
            continue;
        }

        uint32 itemId = ids[index];
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        Item* newItem = bot->StoreNewItemInInventorySlot(itemId, urand(1, proto->GetMaxStackSize()));
        if (newItem)
        {
            newItem->AddToUpdateQueueOf(bot);
        }
   }
}

/**
 * Initializes the food for the player bot.
 */
void PlayerbotFactory::InitFood()
{
    map<uint32, vector<uint32> > items;
    // Iterate through all item entries and find food that the bot can use
    for (uint32 itemId = 0; itemId < sItemStorage.GetMaxEntry(); ++itemId)
    {
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        if (!proto)
        {
            continue;
        }

        if (proto->Class != ITEM_CLASS_CONSUMABLE ||
            proto->SubClass != ITEM_SUBCLASS_FOOD ||
            (proto->Spells[0].SpellCategory != 11 && proto->Spells[0].SpellCategory != 59) ||
            proto->Bonding != NO_BIND)
        {
            continue;
        }

        if (proto->RequiredLevel > bot->getLevel() || proto->RequiredLevel < bot->getLevel() - 10)
        {
            continue;
        }

        if (proto->RequiredSkill && !bot->HasSkill(proto->RequiredSkill))
        {
            continue;
        }

        if (proto->Area || proto->Map || proto->RequiredCityRank || proto->RequiredHonorRank)
        {
            continue;
        }

        items[proto->Spells[0].SpellCategory].push_back(itemId);
    }

    // Add a random food item to the bot's inventory
    uint32 categories[] = { 11, 59 };
    for (int i = 0; i < sizeof(categories) / sizeof(uint32); ++i)
    {
        uint32 category = categories[i];
        vector<uint32>& ids = items[category];
        uint32 index = urand(0, ids.size() - 1);
        if (index >= ids.size())
        {
            continue;
        }

        uint32 itemId = ids[index];
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        Item* newItem = bot->StoreNewItemInInventorySlot(itemId, urand(1, proto->GetMaxStackSize()));
        if (newItem)
        {
            newItem->AddToUpdateQueueOf(bot);
        }
   }
}

/**
 * Cancels all auras on the player bot.
 */
void PlayerbotFactory::CancelAuras()
{
    bot->RemoveAllAuras();
}

/**
 * Initializes the inventory for the player bot.
 */
void PlayerbotFactory::InitInventory()
{
    InitInventoryTrade();
    InitInventoryEquip();
    InitInventorySkill();
}

/**
 * Initializes the skill-related items in the player bot's inventory.
 */
void PlayerbotFactory::InitInventorySkill()
{
    if (bot->HasSkill(SKILL_MINING))
    {
        StoreItem(2901, 1); // Mining Pick
    }
#if !defined(CLASSIC)
    if (bot->HasSkill(SKILL_JEWELCRAFTING))
    {
        StoreItem(20815, 1); // Jeweler's Kit
        StoreItem(20824, 1); // Simple Grinder
    }
#endif
    if (bot->HasSkill(SKILL_BLACKSMITHING) || bot->HasSkill(SKILL_ENGINEERING))
    {
        StoreItem(5956, 1); // Blacksmith Hammer
    }
    if (bot->HasSkill(SKILL_ENGINEERING))
    {
        StoreItem(6219, 1); // Arclight Spanner
    }
    if (bot->HasSkill(SKILL_ENCHANTING))
    {
        StoreItem(16207, 1); // Runed Arcanite Rod
    }
    /*if (bot->HasSkill(SKILL_INSCRIPTION)) {
        StoreItem(39505, 1); // Virtuoso Inking Set
    }*/
    if (bot->HasSkill(SKILL_SKINNING))
    {
        StoreItem(7005, 1); // Skinning Knife
    }
}

/**
 * Stores an item in the player bot's inventory.
 * @param itemId The item ID.
 * @param count The quantity of the item.
 * @return The stored item.
 */
Item* PlayerbotFactory::StoreItem(uint32 itemId, uint32 count)
{
    ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
    ItemPosCountVec sDest;
    InventoryResult msg = bot->CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, sDest, itemId, count);
    if (msg != EQUIP_ERR_OK)
    {
        return NULL;
    }

    return bot->StoreNewItem(sDest, itemId, true, Item::GenerateItemRandomPropertyId(itemId));
}

/**
 * Initializes the trade-related items in the player bot's inventory.
 */
void PlayerbotFactory::InitInventoryTrade()
{
    vector<uint32> ids;
    // Iterate through all item entries and find trade goods that the bot can use
    for (uint32 itemId = 0; itemId < sItemStorage.GetMaxEntry(); ++itemId)
    {
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        if (!proto)
        {
            continue;
        }

        if (proto->Class != ITEM_CLASS_TRADE_GOODS || proto->Bonding != NO_BIND)
        {
            continue;
        }

        if (proto->ItemLevel < bot->getLevel())
        {
            continue;
        }

        if (proto->RequiredLevel > bot->getLevel() || proto->RequiredLevel < bot->getLevel() - 10)
        {
            continue;
        }

        if (proto->RequiredSkill && !bot->HasSkill(proto->RequiredSkill))
        {
            continue;
        }

        ids.push_back(itemId);
    }

    if (ids.empty())
    {
        sLog.outError("No trade items available for bot %s (%d level)", bot->GetName(), bot->getLevel());
        return;
    }

    // Add a random trade good item to the bot's inventory
    uint32 index = urand(0, ids.size() - 1);
    if (index >= ids.size())
    {
        return;
    }

    uint32 itemId = ids[index];
    ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
    if (!proto)
    {
        return;
    }

    uint32 count = 1, stacks = 1;
    switch (proto->Quality)
    {
        case ITEM_QUALITY_NORMAL:
            count = proto->GetMaxStackSize();
            stacks = urand(1, 7) / auctionbot.GetRarityPriceMultiplier(proto);
            break;
        case ITEM_QUALITY_UNCOMMON:
            stacks = 1;
            count = urand(1, proto->GetMaxStackSize());
            break;
        case ITEM_QUALITY_RARE:
            stacks = 1;
            count = urand(1, min(uint32(3), proto->GetMaxStackSize()));
            break;
    }

    for (uint32 i = 0; i < stacks; i++)
    {
        StoreItem(itemId, count);
    }
}

/**
 * Initializes the equipment for the player bot.
 */
void PlayerbotFactory::InitInventoryEquip()
{
    vector<uint32> ids;

    // Determine the desired quality of items
    uint32 desiredQuality = itemQuality;
    if (urand(0, 100) < 100 * sPlayerbotAIConfig.randomGearLoweringChance && desiredQuality > ITEM_QUALITY_NORMAL)
    {
        desiredQuality--;
    }

    // Iterate through all item entries and find items that the bot can equip
    for (uint32 itemId = 0; itemId < sItemStorage.GetMaxEntry(); ++itemId)
    {
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        if (!proto)
        {
            continue;
        }

        // Check if the item is armor or weapon and if it can be equipped by the bot
        if (proto->Class != ITEM_CLASS_ARMOR && proto->Class != ITEM_CLASS_WEAPON || (proto->Bonding == BIND_WHEN_PICKED_UP ||
                proto->Bonding == BIND_WHEN_USE))
        {
            continue;
        }

        if (proto->Class == ITEM_CLASS_ARMOR && !CanEquipArmor(proto))
        {
            continue;
        }

        if (proto->Class == ITEM_CLASS_WEAPON && !CanEquipWeapon(proto))
        {
            continue;
        }

        if (!CanEquipItem(proto, desiredQuality))
        {
            continue;
        }

        ids.push_back(itemId);
    }

    // Add a random number of items to the bot's inventory
    int maxCount = urand(0, 3);
    int count = 0;
    for (int attempts = 0; attempts < 15; attempts++)
    {
        uint32 index = urand(0, ids.size() - 1);
        if (index >= ids.size())
        {
            continue;
        }

        uint32 itemId = ids[index];
        if (StoreItem(itemId, 1) && count++ >= maxCount)
        {
            break;
        }
   }
}

/**
 * Initializes the guild for the player bot.
 */
void PlayerbotFactory::InitGuild()
{
    // Check if the bot is already in a guild
    if (bot->GetGuildId())
    {
        return;
    }

    // Create random guilds if the list is empty
    if (sPlayerbotAIConfig.randomBotGuilds.empty())
    {
        RandomPlayerbotFactory::CreateRandomGuilds();
    }

    vector<uint32> guilds;
    for(list<uint32>::iterator i = sPlayerbotAIConfig.randomBotGuilds.begin(); i != sPlayerbotAIConfig.randomBotGuilds.end(); ++i)
    {
        guilds.push_back(*i);
    }

    if (guilds.empty())
    {
        sLog.outError("No random guilds available");
        return;
    }

    // Add the bot to a random guild
    int index = urand(0, guilds.size() - 1);
    uint32 guildId = guilds[index];
    Guild* guild = sGuildMgr.GetGuildById(guildId);
    if (!guild)
    {
        sLog.outError("Invalid guild %u", guildId);
        return;
    }

    if (guild->GetMemberSize() < 10)
    {
        guild->AddMember(bot->GetObjectGuid(), urand(GR_OFFICER, GR_INITIATE));
    }
}
