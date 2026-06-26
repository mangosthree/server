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

#include "Pet.h"
#include "Database/DatabaseEnv.h"
#include "Log.h"
#include "WorldPacket.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Formulas.h"
#include "SpellAuras.h"
#include "CreatureAI.h"
#include "Unit.h"
#include "Util.h"

/**
 * @brief Creates a pet instance of the specified type.
 *
 * @param type The pet type to initialize.
 */
Pet::Pet(PetType type) :
    Creature(CREATURE_SUBTYPE_PET),
    m_resetTalentsCost(0), m_resetTalentsTime(0), m_usedTalentCount(0),
    m_removed(false),  m_petType(type), m_duration(0),
    m_bonusdamage(0), m_petSlot(-1), m_auraUpdateMask(0), m_loading(false),
    m_declinedname(NULL), m_petModeFlags(PET_MODE_DEFAULT), m_retreating(false),
    m_stayPosSet(false), m_stayPosX(0), m_stayPosY(0), m_stayPosZ(0), m_stayPosO(0),
    m_opener(0), m_openerMinRange(0), m_openerMaxRange(0)
{
    m_name = "Pet";
    m_regenTimer = 4000;
    m_holyPowerRegenTimer = REGEN_TIME_HOLY_POWER;

    // pets always have a charminfo, even if they are not actually charmed
    CharmInfo* charmInfo = InitCharmInfo(this);

    if (type == MINI_PET)                                   // always passive
    {
        charmInfo->SetReactState(REACT_PASSIVE);
    }
}

/**
 * @brief Destroys the pet instance.
 */
Pet::~Pet()
{
    delete m_declinedname;
}

/**
 * @brief Adds the pet to the world and object store.
 */
void Pet::AddToWorld()
{
    ///- Register the pet for guid lookup
    if (!IsInWorld())
    {
        GetMap()->GetObjectsStore().insert<Pet>(GetObjectGuid(), (Pet*)this);
    }

    Unit::AddToWorld();
}

/**
 * @brief Removes the pet from the world and object store.
 */
void Pet::RemoveFromWorld()
{
    ///- Remove the pet from the accessor
    if (IsInWorld())
    {
        GetMap()->GetObjectsStore().erase<Pet>(GetObjectGuid(), (Pet*)NULL);
    }

    ///- Don't call the function for Creature, normal mobs + totems go in a different storage
    Unit::RemoveFromWorld();
}


/**
 * @brief Updates the pet death state and related pet-specific behavior.
 *
 * @param s The new death state.
 */
void Pet::SetDeathState(DeathState s)                       // overwrite virtual Creature::SetDeathState and Unit::SetDeathState
{
    Creature::SetDeathState(s);
    if (GetDeathState() == CORPSE)
    {
        // remove summoned pet (no corpse)
        if (getPetType() == SUMMON_PET)
        {
            Unsummon(PET_SAVE_NOT_IN_SLOT);
        }
        // other will despawn at corpse desppawning (Pet::Update code)
        else
        {
            // pet corpse non lootable and non skinnable
            SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);
            RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

        }
    }
    else if (GetDeathState() == ALIVE)
    {
        RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        CastPetAuras(true);
    }
    CastOwnerTalentAuras();
}

/**
 * @brief Updates the pet each server tick.
 *
 * @param update_diff The elapsed time since the last update in milliseconds.
 * @param diff The world update time forwarded to base update logic.
 */
void Pet::Update(uint32 update_diff, uint32 diff)
{
    if (m_removed)                                          // pet already removed, just wait in remove queue, no updates
    {
        return;
    }

    switch (m_deathState)
    {
        case CORPSE:
        {
            if (m_corpseDecayTimer <= update_diff)
            {
                MANGOS_ASSERT(getPetType() != SUMMON_PET && "Must be already removed.");
                Unsummon(PET_SAVE_NOT_IN_SLOT);             // hunters' pets never get removed because of death, NEVER!
                return;
            }
            break;
        }
        case ALIVE:
        {
            // unsummon pet that lost owner
            Unit* owner = GetOwner();
            if (!owner ||
                (!IsWithinDistInMap(owner, GetMap()->GetVisibilityDistance()) && (owner->GetCharmGuid() && (owner->GetCharmGuid() != GetObjectGuid()))) ||
                (isControlled() && !owner->GetPetGuid()))
            {
                Unsummon(PET_SAVE_REAGENTS);
                return;
            }

            if (isControlled())
            {
                if (owner->GetPetGuid() != GetObjectGuid())
                {
                    Unsummon(getPetType() == HUNTER_PET ? PET_SAVE_AS_DELETED : PET_SAVE_NOT_IN_SLOT, owner);
                    return;
                }
            }

            if (m_duration > 0)
            {
                if (m_duration > (int32)update_diff)
                {
                    m_duration -= (int32)update_diff;
                }
                else
                {
                    Unsummon(getPetType() != SUMMON_PET ? PET_SAVE_AS_DELETED : PET_SAVE_NOT_IN_SLOT, owner);
                    return;
                }
            }
            break;
        }
        default:
            break;
    }

    Creature::Update(update_diff, diff);
}

/**
 * @brief Regenerates pet health, power, happiness, and loyalty timers.
 *
 * @param update_diff The elapsed time since the last update in milliseconds.
 */
void Pet::RegenerateAll(uint32 update_diff)
{
    // regenerate focus for hunter pets or energy for deathknight's ghoul
    if (m_regenTimer <= update_diff)
    {
        if (!IsInCombat() || IsPolymorphed())
        {
            RegenerateHealth();
        }

        RegeneratePower();

        m_regenTimer = 4000;
    }
    else
    {
        m_regenTimer -= update_diff;
    }
}

/**
 * @brief Checks whether the pet can learn another active spell family.
 *
 * @param spellid The spell being evaluated.
 * @return true if another active spell can be learned; otherwise, false.
 */
bool Pet::CanTakeMoreActiveSpells(uint32 spellid)
{
    uint8  activecount = 1;
    uint32 chainstartstore[ACTIVE_SPELLS_MAX];

    if (IsPassiveSpell(spellid))
    {
        return true;
    }

    chainstartstore[0] = sSpellMgr.GetFirstSpellInChain(spellid);

    for (PetSpellMap::const_iterator itr = m_spells.begin(); itr != m_spells.end(); ++itr)
    {
        if (itr->second.state == PETSPELL_REMOVED)
        {
            continue;
        }

        if (IsPassiveSpell(itr->first))
        {
            continue;
        }

        uint32 chainstart = sSpellMgr.GetFirstSpellInChain(itr->first);

        uint8 x;

        for (x = 0; x < activecount; ++x)
        {
            if (chainstart == chainstartstore[x])
            {
                break;
            }
        }

        if (x == activecount)                               // spellchain not yet saved -> add active count
        {
            ++activecount;
            if (activecount > ACTIVE_SPELLS_MAX)
            {
                return false;
            }
            chainstartstore[x] = chainstart;
        }
    }
    return true;
}

/**
 * @brief Unsummons the pet and optionally saves it.
 *
 * @param mode The pet save mode to use.
 * @param owner Optional owner override.
 */
void Pet::Unsummon(PetSaveMode mode, Unit* owner /*= NULL*/)
{
    if (!owner)
    {
        owner = GetOwner();
    }

    CombatStop();

    if (owner)
    {
        if (GetOwnerGuid() != owner->GetObjectGuid())
        {
            return;
        }

        Player* p_owner = owner->GetTypeId() == TYPEID_PLAYER ? (Player*)owner : NULL;

        if (p_owner)
        {
            // not save secondary permanent pet as current
            if (mode == PET_SAVE_AS_CURRENT && p_owner->GetTemporaryUnsummonedPetNumber() &&
                p_owner->GetTemporaryUnsummonedPetNumber() != GetCharmInfo()->GetPetNumber())
                {
                    mode = PET_SAVE_NOT_IN_SLOT;
                }

            if (mode == PET_SAVE_REAGENTS)
            {
                // returning of reagents only for players, so best done here
                uint32 spellId = GetUInt32Value(UNIT_CREATED_BY_SPELL);
                SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
                SpellReagentsEntry const* spellReagents = spellInfo ? spellInfo->GetSpellReagents() : NULL;

                if (spellReagents)
                {
                    for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
                    {
                        if (spellReagents->Reagent[i] > 0)
                        {
                            ItemPosCountVec dest;           // for succubus, voidwalker, felhunter and felguard credit soulshard when despawn reason other than death (out of range, logout)
                            uint8 msg = p_owner->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, spellReagents->Reagent[i], spellReagents->ReagentCount[i]);
                            if (msg == EQUIP_ERR_OK)
                            {
                                Item* item = p_owner->StoreNewItem(dest, spellReagents->Reagent[i], true);
                                if (p_owner->IsInWorld())
                                {
                                    p_owner->SendNewItem(item, spellReagents->ReagentCount[i], true, false);
                                }
                            }
                        }
                    }
                }
            }

            if (isControlled())
            {
                p_owner->RemovePetActionBar();

                if (p_owner->GetGroup())
                {
                    p_owner->SetGroupUpdateFlag(GROUP_UPDATE_PET);
                }
            }
        }

        // only if current pet in slot
        switch (getPetType())
        {
            case MINI_PET:
                if (p_owner)
                {
                    p_owner->SetMiniPet(NULL);
                }
                break;
            case PROTECTOR_PET:
            case GUARDIAN_PET:
                owner->RemoveGuardian(this);
                break;
            default:
                if (owner->GetPetGuid() == GetObjectGuid())
                {
                    owner->SetPet(NULL);
                }
                break;
        }
    }

    SavePetToDB(mode);
    AddObjectToRemoveList();
    m_removed = true;
}

/**
 * @brief Grants pet experience and handles leveling.
 *
 * @param xp The raw experience amount.
 */
void Pet::GivePetXP(uint32 xp)
{
    if (getPetType() != HUNTER_PET)
    {
        return;
    }

    if (xp < 1)
    {
        return;
    }

    if (!IsAlive())
    {
        return;
    }

    uint32 level = getLevel();
    uint32 maxlevel = std::min(sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL), GetOwner()->getLevel());

    // pet not receive xp for level equal to owner level
    if (level >= maxlevel)
    {
        return;
    }

    xp *= sWorld.getConfig(CONFIG_FLOAT_RATE_PET_XP_KILL);

    uint32 nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    uint32 curXP = GetUInt32Value(UNIT_FIELD_PETEXPERIENCE);
    uint32 newXP = curXP + xp;

    while (newXP >= nextLvlXP && level < maxlevel)
    {
        newXP -= nextLvlXP;
        ++level;

        GivePetLevel(level);                              // also update UNIT_FIELD_PETNEXTLEVELEXP and UNIT_FIELD_PETEXPERIENCE to level start

        nextLvlXP = GetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP);
    }

    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, level < maxlevel ? newXP : 0);
}

/**
 * @brief Sets the pet to a new level and refreshes level-dependent stats.
 *
 * @param level The new level.
 */
void Pet::GivePetLevel(uint32 level)
{
    if (!level || level == getLevel())
    {
        return;
    }

    if (getPetType() == HUNTER_PET)
    {
        SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
        SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(level));
    }

    InitStatsForLevel(level);
    InitLevelupSpellsForLevel();
    InitTalentForLevel();
}

/**
 * @brief Initializes base pet data from an existing creature.
 *
 * @param creature The source creature.
 * @return true if initialization succeeded; otherwise, false.
 */
bool Pet::CreateBaseAtCreature(Creature* creature)
{
    if (!creature)
    {
        sLog.outError("CRITICAL: NULL pointer passed into CreateBaseAtCreature()");
        return false;
    }

    CreatureCreatePos pos(creature, creature->GetOrientation());

    uint32 guid = creature->GetMap()->GenerateLocalLowGuid(HIGHGUID_PET);

    BASIC_LOG("Create pet");
    uint32 pet_number = sObjectMgr.GeneratePetNumber();
    if (!Create(guid, pos, creature->GetCreatureInfo(), pet_number))
    {
        return false;
    }

    CreatureInfo const* cInfo = GetCreatureInfo();
    if (!cInfo)
    {
        sLog.outError("CreateBaseAtCreature() failed, creatureInfo is missing!");
        return false;
    }

    SetDisplayId(creature->GetDisplayId());
    SetNativeDisplayId(creature->GetNativeDisplayId());
    SetPowerType(POWER_FOCUS);
    SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
    SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
    SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(creature->getLevel()));
    SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

    // Cata 4.3.4 tame-name rules:
    //  - Rare and rare-elite creatures (e.g. King Mosh, Loque'nahak)
    //    keep their NPC name when tamed, instead of being replaced by
    //    the family name. Players use Pet Rename (or a Certificate of
    //    Ownership for second+ renames) if they want a custom name.
    //  - Everything else inherits the pet-family name ("Wolf", "Cat",
    //    "Devilsaur"), so identical-species pets share a default.
    //  - Either path can hand back an empty string when the source DBC
    //    row is missing the localized name (some Cata-added families
    //    have empty Name[locale] entries on certain client builds);
    //    when that happens the WoW client falls back to rendering the
    //    pet as "Unknown's Pet" in unit frames. Fall through to the
    //    creature name so the pet always carries a usable label.
    std::string petName;
    bool const isRare = cInfo->Rank == CREATURE_ELITE_RARE
                     || cInfo->Rank == CREATURE_ELITE_RAREELITE;
    if (!isRare)
    {
        if (CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->Family))
        {
            petName = cFamily->Name[sWorld.GetDefaultDbcLocale()];
        }
    }
    if (petName.empty())
    {
        petName = creature->GetNameForLocaleIdx(sObjectMgr.GetDBCLocaleIndex());
    }
    SetName(petName);

    SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_WARRIOR);
    SetByteValue(UNIT_FIELD_BYTES_0, 2, GENDER_NONE);
    SetByteValue(UNIT_FIELD_BYTES_0, 3, POWER_FOCUS);
    SetSheath(SHEATH_STATE_MELEE);

    SetByteValue(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SUPPORTABLE | UNIT_BYTE2_FLAG_AURAS);
    SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED | UNIT_CAN_BE_ABANDONED);

    SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE | UNIT_FLAG_RENAME);

    SetUInt32Value(UNIT_MOD_CAST_SPEED, creature->GetUInt32Value(UNIT_MOD_CAST_SPEED));

    return true;
}

/**
 * @brief Initializes pet stats for a given level.
 *
 * @param petlevel The target pet level.
 * @return true if initialization succeeded; otherwise, false.
 */
void Pet::InitStatsForLevel(uint32 petlevel)
{
    Unit* owner = GetOwner();
    CreatureInfo const* cInfo = GetCreatureInfo();
    MANGOS_ASSERT(cInfo);

    SetLevel(petlevel);

    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0);

    int32 createResistance[MAX_SPELL_SCHOOL] = {0, 0, 0, 0, 0, 0, 0};

    if (getPetType() == HUNTER_PET)
    {
        SetMeleeDamageSchool(SpellSchools(SPELL_SCHOOL_NORMAL));
        SetAttackTime(BASE_ATTACK, BASE_ATTACK_TIME);
        SetAttackTime(OFF_ATTACK, BASE_ATTACK_TIME);
        SetAttackTime(RANGED_ATTACK, BASE_ATTACK_TIME);
    }
    else
    {
        SetMeleeDamageSchool(SpellSchools(cInfo->DamageSchool));
        SetAttackTime(BASE_ATTACK, cInfo->MeleeBaseAttackTime);
        SetAttackTime(OFF_ATTACK, cInfo->MeleeBaseAttackTime);
        SetAttackTime(RANGED_ATTACK, cInfo->RangedBaseAttackTime);

        createResistance[SPELL_SCHOOL_HOLY]   = cInfo->ResistanceHoly;
        createResistance[SPELL_SCHOOL_FIRE]   = cInfo->ResistanceFire;
        createResistance[SPELL_SCHOOL_NATURE] = cInfo->ResistanceNature;
        createResistance[SPELL_SCHOOL_FROST]  = cInfo->ResistanceFrost;
        createResistance[SPELL_SCHOOL_SHADOW] = cInfo->ResistanceShadow;
        createResistance[SPELL_SCHOOL_ARCANE] = cInfo->ResistanceArcane;
    }

    for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
    {
        SetModifierValue(UnitMods(UNIT_MOD_RESISTANCE_START + i), BASE_VALUE, float(createResistance[i]));
    }

    float health, mana, armor, minDmg;

    switch (getPetType())
    {
        case HUNTER_PET:
        {
            CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->Family);

            if (cFamily && cFamily->minScale > 0.0f)
            {
                float scale;
                if (getLevel() >= cFamily->maxScaleLevel)
                {
                    scale = cFamily->maxScale;
                }
                else if (getLevel() <= cFamily->minScaleLevel)
                {
                    scale = cFamily->minScale;
                }
                else
                {
                    scale = cFamily->minScale + float(getLevel() - cFamily->minScaleLevel) / cFamily->maxScaleLevel * (cFamily->maxScale - cFamily->minScale);
                }

                SetObjectScale(scale);
                UpdateModelData();
            }

            // Max level
            if (petlevel < sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL))
            {
                SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, sObjectMgr.GetXPForPetLevel(petlevel));
            }
            else
            {
                SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
                SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);
            }

            // Info found in pet_levelstats
            if (PetLevelInfo const* pInfo = sObjectMgr.GetPetLevelInfo(1, petlevel))
            {
                for (int i = STAT_STRENGTH; i < MAX_STATS;++i)
                {
                    SetCreateStat(Stats(i), float(pInfo->stats[i]));
                }

                health = pInfo->health;
                mana = 0;
                armor = pInfo->armor;

                // First we divide attack time by standard attack time, and then multipy by level and damage mod.
                uint32 mDmg = (GetAttackTime(BASE_ATTACK) * petlevel) / 2000;

                // Set damage
                SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(mDmg - mDmg / 4));
                SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float((mDmg - mDmg / 4) * 1.5));
            }
            else
            {
                sLog.outErrorDb("HUNTER PET levelstats missing in DB! 'Weakifying' pet");

                for (int i = STAT_STRENGTH; i < MAX_STATS;++i)
                {
                    SetCreateStat(Stats(i), 1.0f);
                }

                health = 1;
                mana = 0;
                armor = 0;

                // Set damage
                SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 1);
                SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 1);
            }

            break;
        }
        case SUMMON_PET:
        {
            if (owner)
            {
                switch (owner->getClass())
                {
                    case CLASS_WARLOCK:
                    {
                        // the damage bonus used for pets is either fire or shadow damage, whatever is higher
                        uint32 fire  = owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_FIRE);
                        uint32 shadow = owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_SHADOW);
                        uint32 val  = (fire > shadow) ? fire : shadow;

                        SetBonusDamage(int32(val * 0.15f));
                        // bonusAP += val * 0.57;
                        break;
                    }
                    case CLASS_MAGE:
                    {
                        // 40% damage bonus of mage's frost damage
                        float val = owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + SPELL_SCHOOL_FROST) * 0.4f;
                        if (val < 0)
                        {
                            val = 0;
                        }
                        SetBonusDamage(int32(val));
                        break;
                    }
                    default:
                        break;
                }
            }
            else
                sLog.outError("Pet::InitStatsForLevel> No owner for creature pet %s !", GetGuidStr().c_str());

            SetUInt32Value(UNIT_FIELD_PETEXPERIENCE, 0);
            SetUInt32Value(UNIT_FIELD_PETNEXTLEVELEXP, 1000);

            // Info found in pet_levelstats
            if (PetLevelInfo const* pInfo = sObjectMgr.GetPetLevelInfo(cInfo->Entry, petlevel))
            {
                for (int i = STAT_STRENGTH; i < MAX_STATS;++i)
                {
                    SetCreateStat(Stats(i), float(pInfo->stats[i]));
                }

                health = pInfo->health;
                mana = pInfo->mana;
                armor = pInfo->armor;

                // Info found in ClassLevelStats
                if (CreatureClassLvlStats const* cCLS = sObjectMgr.GetCreatureClassLvlStats(petlevel, cInfo->UnitClass, cInfo->Expansion))
                {
                    minDmg = (cCLS->BaseDamage * cInfo->DamageVariance + (cCLS->BaseMeleeAttackPower / 14) * (cInfo->MeleeBaseAttackTime/1000)) * cInfo->DamageMultiplier;

                    // Apply custom damage setting (from config)
                    minDmg *= _GetDamageMod(cInfo->Rank);

                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(minDmg));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(minDmg * 1.5));
                }
                else
                {
                    sLog.outErrorDb("SUMMON_PET creature_template not finished (expansion field = -1) on creature %s! (entry: %u)", GetGuidStr().c_str(), cInfo->Entry);

                    float dMinLevel = cInfo->MinMeleeDmg / cInfo->MinLevel;
                    float dMaxLevel = cInfo->MaxMeleeDmg / cInfo->MaxLevel;
                    float mDmg = (dMaxLevel - ((dMaxLevel - dMinLevel) / 2)) * petlevel;

                    // Set damage
                    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(mDmg - mDmg / 4));
                    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float((mDmg - mDmg / 4) * 1.5));
                }
            }
            else
            {
                sLog.outErrorDb("SUMMON_PET levelstats missing in DB! 'Weakifying' pet and giving it mana to make it obvious");

                for (int i = STAT_STRENGTH; i < MAX_STATS;++i)
                {
                    SetCreateStat(Stats(i), 1.0f);
                }

                health = 1;
                mana = 1;
                armor = 1;

                // Set damage
                SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 1);
                SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 1);
            }

            break;
        }
        case PROTECTOR_PET:
        case GUARDIAN_PET:
        {
            if (CreatureClassLvlStats const* cCLS = sObjectMgr.GetCreatureClassLvlStats(petlevel, cInfo->UnitClass, cInfo->Expansion))
            {
                health = cCLS->BaseHealth;
                mana = cCLS->BaseMana;
                armor = cCLS->BaseArmor;

                // Melee
                minDmg = (cCLS->BaseDamage * cInfo->DamageVariance + (cCLS->BaseMeleeAttackPower / 14) * (cInfo->MeleeBaseAttackTime/1000)) * cInfo->DamageMultiplier;

                // Get custom setting
                minDmg *= _GetDamageMod(cInfo->Rank);

                // If the damage value is not passed on as float it will result in damage = 1; but only for guardian type pets, though...
                SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(minDmg));
                SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(minDmg * 1.5));

                // Ranged
                minDmg = (cCLS->BaseDamage * cInfo->DamageVariance + (cCLS->BaseRangedAttackPower / 14) * (cInfo->RangedBaseAttackTime/1000)) * cInfo->DamageMultiplier;

                // Get custom setting
                minDmg *= _GetDamageMod(cInfo->Rank);

                SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, float(minDmg));
                SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, float(minDmg * 1.5));
            }
            else // TODO: Remove fallback to creature_template data when DB is ready
            {
                if (petlevel >= cInfo->MaxLevel)
                {
                    health = cInfo->MaxLevelHealth;
                    mana = cInfo->MaxLevelMana;
                }
                else if (petlevel <= cInfo->MinLevel)
                {
                    health = cInfo->MinLevelHealth;
                    mana = cInfo->MinLevelMana;
                }
                else
                {
                    float hMinLevel = cInfo->MinLevelHealth / cInfo->MinLevel;
                    float hMaxLevel = cInfo->MaxLevelHealth / cInfo->MaxLevel;
                    float mMinLevel = cInfo->MinLevelMana / cInfo->MinLevel;
                    float mMaxLevel = cInfo->MaxLevelMana / cInfo->MaxLevel;

                    health = (hMaxLevel - ((hMaxLevel - hMinLevel) / 2)) * petlevel;
                    mana = (mMaxLevel - ((mMaxLevel - mMinLevel) / 2)) * petlevel;
                }

                armor = cInfo->Armor;

                sLog.outErrorDb("Pet::InitStatsForLevel> Error trying to set stats for creature %s (entry: %u) using ClassLevelStats; not enough data to do it!", GetGuidStr().c_str(), cInfo->Entry);

                SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, float(cInfo->MinMeleeDmg));
                SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, float(cInfo->MaxMeleeDmg));

                SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, float(cInfo->MinRangedDmg));
                SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, float(cInfo->MaxRangedDmg));
            }

            break;
        }
        default:
            sLog.outError("Pet have incorrect type (%u) for level handling.", getPetType());
    }

    // Hunter's pets' should NOT use creature's original modifiers/multipliers
    if (getPetType() != HUNTER_PET)
    {
        health *= cInfo->HealthMultiplier;

        if (mana > 0)
        {
            mana *= cInfo->PowerMultiplier;
        }

        armor *= cInfo->ArmorMultiplier;
    }

    // Apply custom health setting (from config)
    health *= _GetHealthMod(cInfo->Rank);

    // Need to update stats before setting health and power or it will bug out in-game displaying it as the mob missing about 2/3
    UpdateAllStats();

    // A pet cannot not have health
    if (health < 1)
    {
        health = 1;
    }

    // Set health
    SetCreateHealth(health);
    SetMaxHealth(health);
    SetHealth(health);
    SetModifierValue(UNIT_MOD_HEALTH, BASE_VALUE, health);

    // Set mana
    SetCreateMana(mana);
    SetMaxPower(POWER_MANA, mana);
    SetPower(POWER_MANA, mana);
    SetModifierValue(UNIT_MOD_MANA, BASE_VALUE, mana);

    // Remove rage bar from pets (By setting rage = 0, and ensuring it stays that way by setting max rage = 0 as well)
    SetMaxPower(POWER_RAGE, 0);
    SetPower(POWER_RAGE, 0);
    SetModifierValue(UNIT_MOD_RAGE, BASE_VALUE, 0);

    // Set armor
    SetModifierValue(UNIT_MOD_ARMOR, BASE_VALUE, armor);

    return;
}

/**
 * @brief Checks whether the pet can eat a specific food item.
 *
 * @param item The food item prototype.
 * @return true if the food is valid for this pet; otherwise, false.
 */
bool Pet::HaveInDiet(ItemPrototype const* item) const
{
    if (!item->FoodType)
    {
        return false;
    }

    CreatureInfo const* cInfo = GetCreatureInfo();
    if (!cInfo)
    {
        return false;
    }

    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->Family);
    if (!cFamily)
    {
        return false;
    }

    uint32 diet = cFamily->petFoodMask;
    uint32 FoodMask = 1 << (item->FoodType - 1);
    return diet & FoodMask;
}

/**
 * @brief Computes the happiness benefit gained from a food level.
 *
 * @param itemlevel The level of the consumed food item.
 * @return The happiness benefit value.
 */
uint32 Pet::GetCurrentFoodBenefitLevel(uint32 itemlevel)
{
    // -5 or greater food level
    if (getLevel() <= itemlevel + 5)                        // possible to feed level 60 pet with level 55 level food for full effect
    {
        return 35000;
    }
    // -10..-6
    else if (getLevel() <= itemlevel + 10)                  // pure guess, but sounds good
    {
        return 17000;
    }
    // -14..-11
    else if (getLevel() <= itemlevel + 14)                  // level 55 food gets green on 70, makes sense to me
    {
        return 8000;
    }
    // -15 or less
    else
    {
        return 0;                                            // food too low level
    }
}


/**
 * @brief Creates the pet world object from creature data.
 *
 * @param guidlow The low GUID to use.
 * @param cPos The creation position.
 * @param cinfo The creature template.
 * @param pet_number The pet number identifier.
 * @return true if creation succeeded; otherwise, false.
 */
bool Pet::Create(uint32 guidlow, CreatureCreatePos& cPos, CreatureInfo const* cinfo, uint32 pet_number)
{
    SetMap(cPos.GetMap());
    SetPhaseMask(cPos.GetPhaseMask(), false);

    Object::_Create(guidlow, pet_number, HIGHGUID_PET);

    m_originalEntry = cinfo->Entry;

    if (!InitEntry(cinfo->Entry))
    {
        return false;
    }

    cPos.SelectFinalPoint(this);

    if (!cPos.Relocate(this))
    {
        return false;
    }

    SetSheath(SHEATH_STATE_MELEE);

    if (getPetType() == MINI_PET)                           // always non-attackable
    {
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    }

    return true;
}

/**
 * @brief Checks whether the pet currently knows a spell.
 *
 * @param spell The spell identifier.
 * @return true if the spell is present and not removed; otherwise, false.
 */
bool Pet::HasSpell(uint32 spell) const
{
    PetSpellMap::const_iterator itr = m_spells.find(spell);
    return (itr != m_spells.end() && itr->second.state != PETSPELL_REMOVED);
}

// Get all passive spells in our skill line
/**
 * @brief Learns passive family spells for the pet.
 */
void Pet::LearnPetPassives()
{
    CreatureInfo const* cInfo = GetCreatureInfo();
    if (!cInfo)
    {
        return;
    }

    CreatureFamilyEntry const* cFamily = sCreatureFamilyStore.LookupEntry(cInfo->Family);
    if (!cFamily)
    {
        return;
    }

    PetFamilySpellsStore::const_iterator petStore = sPetFamilySpellsStore.find(cFamily->ID);
    if (petStore != sPetFamilySpellsStore.end())
    {
        for (PetFamilySpellsSet::const_iterator petSet = petStore->second.begin(); petSet != petStore->second.end(); ++petSet)
        {
            addSpell(*petSet, ACT_DECIDE, PETSPELL_NEW, PETSPELL_FAMILY);
        }
    }
}

/**
 * @brief Applies owner pet auras to the pet.
 *
 * @param current true if this is the currently summoned permanent pet.
 */
void Pet::CastPetAuras(bool current)
{
    if (!isControlled())
    {
        return;
    }

    Unit* owner = GetOwner();

    for (PetAuraSet::const_iterator itr = owner->m_petAuras.begin(); itr != owner->m_petAuras.end();)
    {
        PetAura const* pa = *itr;
        ++itr;

        if (!current && pa->IsRemovedOnChangePet())
        {
            owner->RemovePetAura(pa);
        }
        else
        {
            CastPetAura(pa);
        }
    }
}

/**
 * @brief Applies owner talent auras that should affect the pet.
 */
void Pet::CastOwnerTalentAuras()
{
    if (!GetOwner() || GetOwner()->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* pOwner = static_cast<Player *>(GetOwner());

    // Handle Ferocious Inspiration Talent
    if (pOwner && pOwner->getClass() == CLASS_HUNTER)
    {
        // clear any existing Ferocious Inspiration auras
        if (HasAura(75593))
        {
            RemoveAurasDueToSpell(75593);
        }
        if (HasAura(75446))
        {
            RemoveAurasDueToSpell(75446);
        }
        if (HasAura(75447))
        {
            RemoveAurasDueToSpell(75447);
        }

        if (IsAlive())
        {
            const SpellEntry* seTalent = pOwner->GetKnownTalentRankById(1800); // Ferocious Inspiration

            if (seTalent)
            {
                switch (seTalent->Id)
                {
                    case 34455: // Ferocious Inspiration Rank 1
                        CastSpell(this, 75593, true); // Ferocious Inspiration 1%
                        break;
                    case 34459: // Ferocious Inspiration Rank 2
                        CastSpell(this, 75446, true); // Ferocious Inspiration 2%
                        break;
                    case 34460: // Ferocious Inspiration Rank 3
                        CastSpell(this, 75447, true); // Ferocious Inspiration 3%
                        break;
                }
            }
        }
    } // End Ferocious Inspiration Talent
}

/**
 * @brief Casts a specific pet aura effect.
 *
 * @param aura The pet aura definition to apply.
 */
void Pet::CastPetAura(PetAura const* aura)
{
    uint32 auraId = aura->GetAura(GetEntry());
    if (!auraId)
    {
        return;
    }

    if (auraId == 35696)                                    // Demonic Knowledge
    {
        int32 basePoints = int32(aura->GetDamage() * (GetStat(STAT_STAMINA) + GetStat(STAT_INTELLECT)) / 100);
        CastCustomSpell(this, auraId, &basePoints, NULL, NULL, true);
    }
    else
    {
        CastSpell(this, auraId, true);
    }
}

struct DoPetLearnSpell
{
    DoPetLearnSpell(Pet& _pet) : pet(_pet) {}
    void operator()(uint32 spell_id) { pet.learnSpell(spell_id); }
    Pet& pet;
};

void Pet::learnSpellHighRank(uint32 spellid)
{
    learnSpell(spellid);

    DoPetLearnSpell worker(*this);
    sSpellMgr.doForHighRanks(spellid, worker);
}

/**
 * @brief Synchronizes pet level rules with the owner.
 */
void Pet::SynchronizeLevelWithOwner()
{
    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    switch (getPetType())
    {
            // always same level
        case SUMMON_PET:
            GivePetLevel(owner->getLevel());
            break;
            // can't be greater owner level
        case HUNTER_PET:
            if (getLevel() > owner->getLevel())
            {
                GivePetLevel(owner->getLevel());
            }
            else if (getLevel() + 5 < owner->getLevel())
            {
                GivePetLevel(owner->getLevel() - 5);
            }
            break;
        default:
            break;
    }
}

void Pet::SetModeFlags(PetModeFlags mode)
{
    m_petModeFlags = mode;

    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    WorldPacket data(SMSG_PET_MODE, 12);
    data << GetObjectGuid();
    data << uint32(m_petModeFlags);
    ((Player*)owner)->GetSession()->SendPacket(&data);
}

void Pet::SetStayPosition(bool stay)
{
    if (stay)
    {
        m_stayPosX = GetPositionX();
        m_stayPosY = GetPositionY();
        m_stayPosZ = GetPositionZ();
        m_stayPosO = GetOrientation();
    }
    else
    {
        m_stayPosX = 0;
        m_stayPosY = 0;
        m_stayPosZ = 0;
        m_stayPosO = 0;
    }

    m_stayPosSet = stay;
}

/**
 * @brief Applies or removes pet mode flags and updates the owner client.
 *
 * @param mode The mode flag to modify.
 * @param apply true to set the flag; false to clear it.
 */
void Pet::ApplyModeFlags(PetModeFlags mode, bool apply)
{
    if (apply)
    {
        m_petModeFlags = PetModeFlags(m_petModeFlags | mode);
    }
    else
    {
        m_petModeFlags = PetModeFlags(m_petModeFlags & ~mode);
    }

    Unit* owner = GetOwner();
    if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    WorldPacket data(SMSG_PET_MODE, 12);
    data << GetObjectGuid();
    data << uint32(m_petModeFlags);
    ((Player*)owner)->GetSession()->SendPacket(&data);
}
