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

/**
 * @file Spell.cpp
 * @brief Spell casting and effect implementation
 *
 * This file implements the Spell class which handles spell casting:
 * - Spell validation and casting requirements
 * - Spell effect execution (damage, healing, summon, etc.)
 * - Spell targeting and area effects
 * - Spell cooldowns and resource costs
 * - Spell interruption and pushback
 * - Spell aura application
 * - Spell hit/miss calculations
 *
 * Spells are the primary combat mechanic in WoW, encompassing
 * abilities, talents, and item effects.
 *
 * @see Spell for the spell class
 * @see SpellAura for spell auras
 * @see SpellMgr for spell management
 */

#include "Spell.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Pet.h"
#include "Unit.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "CellImpl.h"
#include "Policies/Singleton.h"
#include "SharedDefines.h"
#include "LootMgr.h"
#include "VMapFactory.h"
#include "BattleGround/BattleGround.h"
#include "Util.h"
#include "Chat.h"
#include "DB2Stores.h"
#include "SQLStorages.h"
#include "Vehicle.h"
#include "TemporarySummon.h"
#include "SQLStorages.h"
#include "DisableMgr.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

extern pEffect SpellEffects[TOTAL_SPELL_EFFECTS];

class PrioritizeManaUnitWraper
{
    public:
        explicit PrioritizeManaUnitWraper(Unit* unit) : i_unit(unit)
        {
            uint32 maxmana = unit->GetMaxPower(POWER_MANA);
            i_percent = maxmana ? unit->GetPower(POWER_MANA) * 100 / maxmana : 101;
        }
        Unit* getUnit() const { return i_unit; }
        uint32 getPercent() const { return i_percent; }
    private:
        Unit* i_unit;
        uint32 i_percent;
};

struct PrioritizeMana
{
    int operator()(PrioritizeManaUnitWraper const& x, PrioritizeManaUnitWraper const& y) const
    {
        return x.getPercent() > y.getPercent();
    }
};

typedef std::priority_queue<PrioritizeManaUnitWraper, std::vector<PrioritizeManaUnitWraper>, PrioritizeMana> PrioritizeManaUnitQueue;

class PrioritizeHealthUnitWraper
{
    public:
        explicit PrioritizeHealthUnitWraper(Unit* unit) : i_unit(unit)
        {
            uint32 maxhealth = unit->GetMaxHealth();
            i_percent = maxhealth ? unit->GetHealth() * 100 / maxhealth : 0;
        }
        Unit* getUnit() const { return i_unit; }
        uint32 getPercent() const { return i_percent; }
    private:
        Unit* i_unit;
        uint32 i_percent;
};

struct PrioritizeHealth
{
    int operator()(PrioritizeHealthUnitWraper const& x, PrioritizeHealthUnitWraper const& y) const
    {
        return x.getPercent() > y.getPercent();
    }
};

typedef std::priority_queue<PrioritizeHealthUnitWraper, std::vector<PrioritizeHealthUnitWraper>, PrioritizeHealth> PrioritizeHealthUnitQueue;

/**
 * @brief Checks whether a spell matches the quest tame spell pattern.
 *
 * @param spellId The spell identifier to test.
 * @return True if the spell is a quest tame spell; otherwise, false.
 */
bool IsQuestTameSpell(uint32 spellId)
{
    SpellEntry const* spellproto = sSpellStore.LookupEntry(spellId);
    if (!spellproto)
    {
        return false;
    }

    SpellEffectEntry const* spellEffect0 = spellproto->GetSpellEffect(EFFECT_INDEX_0);
    SpellEffectEntry const* spellEffect1 = spellproto->GetSpellEffect(EFFECT_INDEX_1);
    return spellEffect0 && spellEffect0->Effect == SPELL_EFFECT_THREAT &&
        spellEffect1 && spellEffect1->Effect == SPELL_EFFECT_APPLY_AURA && spellEffect1->EffectApplyAuraName == SPELL_AURA_DUMMY;
}

SpellCastTargets::SpellCastTargets()
{
    m_unitTarget = NULL;
    m_itemTarget = NULL;
    m_GOTarget   = NULL;

    m_itemTargetEntry  = 0;

    m_srcX = m_srcY = m_srcZ = m_destX = m_destY = m_destZ = 0.0f;
    m_strTarget.clear();
    m_targetMask = 0;

    m_elevation = 0.0f;
    m_speed = 0.0f;
}

SpellCastTargets::~SpellCastTargets()
{
}

/**
 * @brief Sets a unit target and copies its current position as the destination.
 *
 * @param target The unit target.
 */
void SpellCastTargets::setUnitTarget(Unit* target)
{
    if (!target)
    {
        return;
    }

    m_destX = target->GetPositionX();
    m_destY = target->GetPositionY();
    m_destZ = target->GetPositionZ();
    m_unitTarget = target;
    m_unitTargetGUID = target->GetObjectGuid();
    m_targetMask |= TARGET_FLAG_UNIT;
}

/**
 * @brief Sets the destination coordinates for the cast.
 *
 * @param x The destination X coordinate.
 * @param y The destination Y coordinate.
 * @param z The destination Z coordinate.
 */
void SpellCastTargets::setDestination(float x, float y, float z)
{
    m_destX = x;
    m_destY = y;
    m_destZ = z;
    m_targetMask |= TARGET_FLAG_DEST_LOCATION;
}

/**
 * @brief Sets the source coordinates for the cast.
 *
 * @param x The source X coordinate.
 * @param y The source Y coordinate.
 * @param z The source Z coordinate.
 */
void SpellCastTargets::setSource(float x, float y, float z)
{
    m_srcX = x;
    m_srcY = y;
    m_srcZ = z;
    m_targetMask |= TARGET_FLAG_SOURCE_LOCATION;
}

/**
 * @brief Sets the game object target for the cast.
 *
 * @param target The game object target.
 */
void SpellCastTargets::setGOTarget(GameObject* target)
{
    m_GOTarget = target;
    m_GOTargetGUID = target ? target->GetObjectGuid() : ObjectGuid();
    //    m_targetMask |= TARGET_FLAG_OBJECT;
}

/**
 * @brief Sets the item target for the cast.
 *
 * @param item The item target.
 */
void SpellCastTargets::setItemTarget(Item* item)
{
    if (!item)
    {
        m_itemTarget = NULL;
        return;
    }

    m_itemTarget = item;
    m_itemTargetGUID = item->GetObjectGuid();
    m_itemTargetEntry = item->GetEntry();
    m_targetMask |= TARGET_FLAG_ITEM;
}

/**
 * @brief Sets the current trade slot as the item target.
 *
 * @param caster The player performing the cast.
 */
void SpellCastTargets::setTradeItemTarget(Player* caster)
{
    m_itemTargetGUID = ObjectGuid(uint64(TRADE_SLOT_NONTRADED));
    m_itemTargetEntry = 0;
    m_targetMask |= TARGET_FLAG_TRADE_ITEM;

    Update(caster);
}

/**
 * @brief Sets the corpse target for the cast.
 *
 * @param corpse The corpse target.
 */
void SpellCastTargets::setCorpseTarget(Corpse* corpse)
{
    m_CorpseTargetGUID = corpse ? corpse->GetObjectGuid() : ObjectGuid();
}

/**
 * @brief Resolves stored target GUIDs into live object pointers.
 *
 * @param caster The casting unit used to resolve map-relative targets.
 */
void SpellCastTargets::Update(Unit* caster)
{
    m_GOTarget   = m_GOTargetGUID ? caster->GetMap()->GetGameObject(m_GOTargetGUID) : NULL;
    m_unitTarget = m_unitTargetGUID ?
                   (m_unitTargetGUID == caster->GetObjectGuid() ? caster : sObjectAccessor.GetUnit(*caster, m_unitTargetGUID)) :
                       NULL;

    m_itemTarget = NULL;
    if (caster->GetTypeId() == TYPEID_PLAYER)
{
        Player* player = ((Player*)caster);

        if (m_targetMask & TARGET_FLAG_ITEM)
        {
            m_itemTarget = player->GetItemByGuid(m_itemTargetGUID);
        }
        else if (m_targetMask & TARGET_FLAG_TRADE_ITEM)
        {
            if (TradeData* pTrade = player->GetTradeData())
                if (m_itemTargetGUID.GetRawValue() < TRADE_SLOT_COUNT)
                {
                    m_itemTarget = pTrade->GetTraderData()->GetItem(TradeSlots(m_itemTargetGUID.GetRawValue()));
                }
        }

        if (m_itemTarget)
        {
            m_itemTargetEntry = m_itemTarget->GetEntry();
        }
    }
}

/**
 * @brief Deserializes spell cast targets from a packet buffer.
 *
 * @param data The packet buffer to read.
 * @param caster The casting unit.
 */
void SpellCastTargets::read(ByteBuffer& data, Unit* caster)
{
    data >> m_targetMask;

    if (m_targetMask == TARGET_FLAG_SELF)
    {
        m_destX = caster->GetPositionX();
        m_destY = caster->GetPositionY();
        m_destZ = caster->GetPositionZ();
        m_unitTarget = caster;
        m_unitTargetGUID = caster->GetObjectGuid();
        return;
    }

    // TARGET_FLAG_UNK2 is used for non-combat pets, maybe other?
    if (m_targetMask & (TARGET_FLAG_UNIT | TARGET_FLAG_UNK2))
    {
        data >> m_unitTargetGUID.ReadAsPacked();
    }

    if (m_targetMask & (TARGET_FLAG_OBJECT | TARGET_FLAG_GAMEOBJECT_ITEM))
    {
        data >> m_GOTargetGUID.ReadAsPacked();
    }

    if ((m_targetMask & (TARGET_FLAG_ITEM | TARGET_FLAG_TRADE_ITEM)) && caster->GetTypeId() == TYPEID_PLAYER)
    {
        data >> m_itemTargetGUID.ReadAsPacked();
    }

    if (m_targetMask & (TARGET_FLAG_CORPSE | TARGET_FLAG_PVP_CORPSE))
    {
        data >> m_CorpseTargetGUID.ReadAsPacked();
    }

    if (m_targetMask & TARGET_FLAG_SOURCE_LOCATION)
    {
        data >> m_srcTransportGUID.ReadAsPacked();
        data >> m_srcX >> m_srcY >> m_srcZ;
        if (!MaNGOS::IsValidMapCoord(m_srcX, m_srcY, m_srcZ))
        {
            throw ByteBufferException(false, data.rpos(), 0, data.size());
        }
    }

    if (m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        data >> m_destTransportGUID.ReadAsPacked();
        data >> m_destX >> m_destY >> m_destZ;
        if (!MaNGOS::IsValidMapCoord(m_destX, m_destY, m_destZ))
        {
            throw ByteBufferException(false, data.rpos(), 0, data.size());
        }
    }

    if (m_targetMask & TARGET_FLAG_STRING)
    {
        data >> m_strTarget;
    }

    // find real units/GOs
    Update(caster);
}

/**
 * @brief Serializes spell cast targets into a packet buffer.
 *
 * @param data The packet buffer to write.
 */
void SpellCastTargets::write(ByteBuffer& data) const
{
    data << uint32(m_targetMask);

    if (m_targetMask & (TARGET_FLAG_UNIT | TARGET_FLAG_PVP_CORPSE | TARGET_FLAG_OBJECT | TARGET_FLAG_CORPSE | TARGET_FLAG_UNK2))
    {
        if (m_targetMask & TARGET_FLAG_UNIT)
        {
            if (m_unitTarget)
            {
                data << m_unitTarget->GetPackGUID();
            }
            else
            {
                data << uint8(0);
            }
        }
        else if (m_targetMask & TARGET_FLAG_OBJECT)
        {
            if (m_GOTarget)
            {
                data << m_GOTarget->GetPackGUID();
            }
            else
            {
                data << uint8(0);
            }
        }
        else if (m_targetMask & (TARGET_FLAG_CORPSE | TARGET_FLAG_PVP_CORPSE))
        {
            data << m_CorpseTargetGUID.WriteAsPacked();
        }
        else
        {
            data << uint8(0);
        }
    }

    if (m_targetMask & (TARGET_FLAG_ITEM | TARGET_FLAG_TRADE_ITEM))
    {
        if (m_itemTarget)
        {
            data << m_itemTarget->GetPackGUID();
        }
        else
        {
            data << uint8(0);
        }
    }

    if (m_targetMask & TARGET_FLAG_SOURCE_LOCATION)
    {
        data << m_srcTransportGUID.WriteAsPacked();
        data << m_srcX << m_srcY << m_srcZ;
    }

    if (m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        data << m_destTransportGUID.WriteAsPacked();
        data << m_destX << m_destY << m_destZ;
    }

    if (m_targetMask & TARGET_FLAG_STRING)
    {
        data << m_strTarget;
    }
}

void SpellCastTargets::ReadAdditionalData(WorldPacket& data, uint8& cast_flags)
{
    if (cast_flags & 0x02)              // has trajectory data
    {
        data >> m_elevation >> m_speed;

        bool hasMovementData;
        data >> hasMovementData;
        if (hasMovementData)
        {
            MovementInfo mi;
            data >> mi;
            setSource(mi.GetPos()->x, mi.GetPos()->y, mi.GetPos()->z);
        }
    }
    else if (cast_flags & 0x08)         // has archaeology weight
    {
        uint32 count;
        uint8 type;
        data >> count;
        for (uint32 i = 0; i < count; ++i)
        {
            data >> type;
            switch (type)
            {
                case 1:                         // Fragments
                    data >> Unused<uint32>();   // Currency entry
                    data >> Unused<uint32>();   // Currency count
                    break;
                case 2:                         // Keystones
                    data >> Unused<uint32>();   // Item entry
                    data >> Unused<uint32>();   // Item count
                    break;
            }
        }
    }
}

Spell::Spell(Unit* caster, SpellEntry const* info, bool triggered, ObjectGuid originalCasterGUID, SpellEntry const* triggeredBy)
{
    MANGOS_ASSERT(caster != NULL && info != NULL);
    MANGOS_ASSERT(info == sSpellStore.LookupEntry(info->Id) && "`info` must be pointer to sSpellStore element");

    if (info->SpellDifficultyId && caster->IsInWorld() && caster->GetMap()->IsDungeon())
    {
        if (SpellEntry const* spellEntry = GetSpellEntryByDifficulty(info->SpellDifficultyId, caster->GetMap()->GetDifficulty(), caster->GetMap()->IsRaid()))
        {
            m_spellInfo = spellEntry;
        }
        else
        {
            m_spellInfo = info;
        }
    }
    else
    {
        m_spellInfo = info;
    }

    m_triggeredBySpellInfo = triggeredBy;

    m_spellInterrupts = m_spellInfo->GetSpellInterrupts();

    m_caster = caster;
    m_selfContainer = NULL;
    m_referencedFromCurrentSpell = false;
    m_executedCurrently = false;
    m_delayStart = 0;
    m_delayAtDamageCount = 0;

    m_applyMultiplierMask = 0;

    // Get data for type of attack
    m_attackType = GetWeaponAttackType(m_spellInfo);

    m_spellSchoolMask = GetSpellSchoolMask(info);           // Can be override for some spell (wand shoot for example)

    if (m_attackType == RANGED_ATTACK)
    {
        // wand case
        if ((m_caster->getClassMask() & CLASSMASK_WAND_USERS) != 0 && m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            if (Item* pItem = ((Player*)m_caster)->GetWeaponForAttack(RANGED_ATTACK))
            {
                m_spellSchoolMask = SpellSchoolMask(1 << pItem->GetProto()->DamageType);
            }
        }
    }
    // Set health leech amount to zero
    m_healthLeech = 0;

    m_originalCasterGUID = originalCasterGUID ? originalCasterGUID : m_caster->GetObjectGuid();

    UpdateOriginalCasterPointer();

    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        m_currentBasePoints[i] = m_spellInfo->CalculateSimpleValue(SpellEffectIndex(i));
    }

    m_spellState = SPELL_STATE_CREATED;

    m_castPositionX = m_castPositionY = m_castPositionZ = 0;
    m_TriggerSpells.clear();
    m_preCastSpells.clear();
    m_IsTriggeredSpell = triggered;
    // m_AreaAura = false;
    m_CastItem = NULL;

    unitTarget = NULL;
    itemTarget = NULL;
    gameObjTarget = NULL;
    focusObject = NULL;
    m_cast_count = 0;
    m_glyphIndex = 0;
    m_triggeredByAuraSpell  = NULL;
    m_spellAuraHolder = NULL;

    // Auto Shot & Shoot (wand)
    m_autoRepeat = IsAutoRepeatRangedSpell(m_spellInfo);

    m_runesState = 0;
    m_powerCost = 0;                                        // setup to correct value in Spell::prepare, don't must be used before.
    m_usedHolyPower = 0;
    m_casttime = 0;                                         // setup to correct value in Spell::prepare, don't must be used before.
    m_timer = 0;                                            // will set to cast time in prepare
    m_duration = 0;

    m_needAliveTargetMask = 0;

    // determine reflection
    m_canReflect = false;

    m_spellFlags = SPELL_FLAG_NORMAL;

    if (m_spellInfo->GetDmgClass() == SPELL_DAMAGE_CLASS_MAGIC && !m_spellInfo->HasAttribute(SPELL_ATTR_EX_CANT_REFLECTED))
    {
        for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
        {
            SpellEffectEntry const* spellEffect = m_spellInfo->GetSpellEffect(SpellEffectIndex(j));
            if (!spellEffect)
            {
                continue;
            }
            if (spellEffect->Effect == 0)
            {
                continue;
            }

            if (!IsPositiveTarget(spellEffect->EffectImplicitTargetA, spellEffect->EffectImplicitTargetB))
            {
                m_canReflect = true;
            }
            else
            {
                m_canReflect = m_spellInfo->HasAttribute(SPELL_ATTR_EX_UNK7);
            }

            if (m_canReflect)
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }

    CleanupTargetList();
}

Spell::~Spell()
{
}

/**
 * @brief Builds the spell target lists for each active effect.
 */
void Spell::FillTargetMap()
{
    // TODO: ADD the correct target FILLS!!!!!!

    UnitList tmpUnitLists[MAX_EFFECT_INDEX];                // Stores the temporary Target Lists for each effect
    uint8 effToIndex[MAX_EFFECT_INDEX] = {0, 1, 2};         // Helper array, to link to another tmpUnitList, if the targets for both effects match
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        SpellEffectEntry const* spellEffect = m_spellInfo->GetSpellEffect(SpellEffectIndex(i));
        if (!spellEffect)
        {
            continue;
        }

        // not call for empty effect.
        // Also some spells use not used effect targets for store targets for dummy effect in triggered spells
        if (spellEffect->Effect == SPELL_EFFECT_NONE)
        {
            continue;
        }

        // targets for TARGET_SCRIPT_COORDINATES (A) and TARGET_SCRIPT
        // for TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT (A) all is checked in Spell::CheckCast and in Spell::CheckItem
        // filled in Spell::CheckCast call
        if (spellEffect->EffectImplicitTargetA == TARGET_SCRIPT_COORDINATES ||
            spellEffect->EffectImplicitTargetA == TARGET_FOCUS_OR_SCRIPTED_GAMEOBJECT ||
            (spellEffect->EffectImplicitTargetA == TARGET_SCRIPT && spellEffect->EffectImplicitTargetB != TARGET_SELF) ||
            (spellEffect->EffectImplicitTargetB == TARGET_SCRIPT && spellEffect->EffectImplicitTargetA != TARGET_SELF))
            continue;

        // TODO: find a way so this is not needed?
        // for area auras always add caster as target (needed for totems for example)
        if (IsAreaAuraEffect(spellEffect->Effect))
        {
            AddUnitTarget(m_caster, SpellEffectIndex(i));
        }

        // no double fill for same targets
        for (int j = 0; j < i; ++j)
        {
            SpellEffectEntry const* spellEffect1 = m_spellInfo->GetSpellEffect(SpellEffectIndex(j));
            if (!spellEffect1)
            {
                continue;
            }

            // Check if same target, but handle i.e. AreaAuras different
            if (spellEffect->EffectImplicitTargetA == spellEffect1->EffectImplicitTargetA && spellEffect->EffectImplicitTargetB == spellEffect1->EffectImplicitTargetB
                && spellEffect1->Effect != SPELL_EFFECT_NONE
                && !IsAreaAuraEffect(spellEffect->Effect) && !IsAreaAuraEffect(spellEffect1->Effect))
                // Add further conditions here if required
            {
                effToIndex[i] = j;                          // effect i has same targeting list as effect j
                break;
            }
        }

        if (effToIndex[i] == i)                             // New target combination
        {
            // TargetA/TargetB dependent from each other, we not switch to full support this dependences
            // but need it support in some know cases
            switch(spellEffect->EffectImplicitTargetA)
            {
                case TARGET_NONE:
                    switch(spellEffect->EffectImplicitTargetB)
                    {
                        case TARGET_NONE:
                            if (m_caster->GetObjectGuid().IsPet())
                            {
                                SetTargetMap(SpellEffectIndex(i), TARGET_SELF, tmpUnitLists[i /*==effToIndex[i]*/]);
                            }
                            else
                            {
                                SetTargetMap(SpellEffectIndex(i), TARGET_EFFECT_SELECT, tmpUnitLists[i /*==effToIndex[i]*/]);
                            }
                            break;
                        default:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                    }
                    break;
                case TARGET_SELF:
                    switch(spellEffect->EffectImplicitTargetB)
                    {
                        case TARGET_NONE:                   // Fill Target based on A only
                        case TARGET_EFFECT_SELECT:
                        case TARGET_SCRIPT:                 // B-target only used with CheckCast here
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        case TARGET_AREAEFFECT_INSTANT:         // use B case that not dependent from from A in fact
                            if ((m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION) == 0)
                            {
                                m_targets.setDestination(m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ());
                            }
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        case TARGET_BEHIND_VICTIM:              // use B case that not dependent from from A in fact
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        default:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                    }
                    break;
                case TARGET_EFFECT_SELECT:
                    switch(spellEffect->EffectImplicitTargetB)
                    {
                        case TARGET_NONE:
                        case TARGET_EFFECT_SELECT:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        // dest point setup required
                        case TARGET_AREAEFFECT_INSTANT:
                        case TARGET_AREAEFFECT_CUSTOM:
                        case TARGET_ALL_ENEMY_IN_AREA:
                        case TARGET_ALL_ENEMY_IN_AREA_INSTANT:
                        case TARGET_ALL_ENEMY_IN_AREA_CHANNELED:
                        case TARGET_ALL_FRIENDLY_UNITS_IN_AREA:
                        case TARGET_AREAEFFECT_GO_AROUND_DEST:
                        case TARGET_RANDOM_NEARBY_DEST:
                            // triggered spells get dest point from default target set, ignore it
                            if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION) || m_IsTriggeredSpell)
                                if (WorldObject* castObject = GetCastingObject())
                                {
                                    m_targets.setDestination(castObject->GetPositionX(), castObject->GetPositionY(), castObject->GetPositionZ());
                                }
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        // target pre-selection required
                        case TARGET_INNKEEPER_COORDINATES:
                        case TARGET_TABLE_X_Y_Z_COORDINATES:
                        case TARGET_CASTER_COORDINATES:
                        case TARGET_SCRIPT_COORDINATES:
                        case TARGET_CURRENT_ENEMY_COORDINATES:
                        case TARGET_DUELVSPLAYER_COORDINATES:
                        case TARGET_DYNAMIC_OBJECT_COORDINATES:
                        case TARGET_POINT_AT_NORTH:
                        case TARGET_POINT_AT_SOUTH:
                        case TARGET_POINT_AT_EAST:
                        case TARGET_POINT_AT_WEST:
                        case TARGET_POINT_AT_NE:
                        case TARGET_POINT_AT_NW:
                        case TARGET_POINT_AT_SE:
                        case TARGET_POINT_AT_SW:
                            // need some target for processing
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        default:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                    }
                    break;
                case TARGET_CASTER_COORDINATES:
                    switch(spellEffect->EffectImplicitTargetB)
                    {
                        case TARGET_ALL_ENEMY_IN_AREA:
                            // Note: this hack with search required until GO casting not implemented
                            // environment damage spells already have around enemies targeting but this not help in case nonexistent GO casting support
                            // currently each enemy selected explicitly and self cast damage
                            if (spellEffect->Effect == SPELL_EFFECT_ENVIRONMENTAL_DAMAGE)
                            {
                                if (m_targets.getUnitTarget())
                                {
                                    tmpUnitLists[i /*==effToIndex[i]*/].push_back(m_targets.getUnitTarget());
                                }
                            }
                            else
                            {
                                SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                                SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            }
                            break;
                        case 0:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            tmpUnitLists[i /*==effToIndex[i]*/].push_back(m_caster);
                            break;
                        default:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                    }
                    break;
                case TARGET_TABLE_X_Y_Z_COORDINATES:
                    switch(spellEffect->EffectImplicitTargetB)
                    {
                        case 0:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);


                        // need some target for processing
                            SetTargetMap(SpellEffectIndex(i), TARGET_EFFECT_SELECT, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        case TARGET_AREAEFFECT_INSTANT:         // All 17/7 pairs used for dest teleportation, A processed in effect code
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        default:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                    }
                    case TARGET_SELF2:
                    switch(spellEffect->EffectImplicitTargetB)
                    {
                        case TARGET_NONE:
                        case TARGET_EFFECT_SELECT:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        case TARGET_AREAEFFECT_CUSTOM:
                            // triggered spells get dest point from default target set, ignore it
                            if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION) || m_IsTriggeredSpell)
                                if (WorldObject* castObject = GetCastingObject())
                                {
                                    m_targets.setDestination(castObject->GetPositionX(), castObject->GetPositionY(), castObject->GetPositionZ());
                                }
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        // most A/B target pairs is self->negative and not expect adding caster to target list
                        default:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                    }
                    break;
                case TARGET_DUELVSPLAYER_COORDINATES:
                    switch(spellEffect->EffectImplicitTargetB)
                    {
                        case TARGET_NONE:
                        case TARGET_EFFECT_SELECT:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            if (Unit* currentTarget = m_targets.getUnitTarget())
                            {
                                tmpUnitLists[i /*==effToIndex[i]*/].push_back(currentTarget);
                            }
                            break;
                        default:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                    }
                    break;
                case TARGET_SCRIPT:
                    switch (spellEffect->EffectImplicitTargetB)
                    {
                        case TARGET_SELF:
                            // Fill target based on B only, A is only used with CheckCast here.
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    switch(spellEffect->EffectImplicitTargetB)
                    {
                        case 0:
                        case TARGET_EFFECT_SELECT:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                        case TARGET_SCRIPT_COORDINATES:         // B case filled in CheckCast but we need fill unit list base at A case
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                           break;
                        default:
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetA, tmpUnitLists[i /*==effToIndex[i]*/]);
                            SetTargetMap(SpellEffectIndex(i), spellEffect->EffectImplicitTargetB, tmpUnitLists[i /*==effToIndex[i]*/]);
                            break;
                    }
                    break;
            }
        }

        if (m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            Player* me = (Player*)m_caster;
            for (UnitList::const_iterator itr = tmpUnitLists[effToIndex[i]].begin(); itr != tmpUnitLists[effToIndex[i]].end(); ++itr)
            {
                Player* targetOwner = (*itr)->GetCharmerOrOwnerPlayerOrPlayerItself();
                if (targetOwner && targetOwner != me && targetOwner->IsPvP() && !me->IsInDuelWith(targetOwner))
                {
                    me->UpdatePvP(true);
                    me->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_ENTER_PVP_COMBAT);
                    break;
                }
            }
        }

        for (UnitList::iterator itr = tmpUnitLists[effToIndex[i]].begin(); itr != tmpUnitLists[effToIndex[i]].end();)
        {
            if (!CheckTarget(*itr, SpellEffectIndex(i)))
            {
                itr = tmpUnitLists[effToIndex[i]].erase(itr);
                continue;
            }
            else
            {
                ++itr;
            }
        }

        for (UnitList::const_iterator iunit = tmpUnitLists[effToIndex[i]].begin(); iunit != tmpUnitLists[effToIndex[i]].end(); ++iunit)
        {
            AddUnitTarget((*iunit), SpellEffectIndex(i));
        }
    }
}

/**
 * @brief Prepares proc-trigger metadata for the current spell cast.
 */
void Spell::prepareDataForTriggerSystem()
{
    //==========================================================================================
    // Now fill data for trigger system, need know:
    // an spell trigger another or not ( m_canTrigger )
    // Create base triggers flags for Attacker and Victim ( m_procAttacker and  m_procVictim)
    //==========================================================================================
    // Fill flag can spell trigger or not
    // TODO: possible exist spell attribute for this
    m_canTrigger = false;

    if (m_CastItem)
    {
        m_canTrigger = false;                               // Do not trigger from item cast spell
    }
    else if (!m_IsTriggeredSpell)
    {
        m_canTrigger = true;                                // Normal cast - can trigger
    }
    else if (!m_triggeredByAuraSpell)
    {
        m_canTrigger = true;                                // Triggered from SPELL_EFFECT_TRIGGER_SPELL - can trigger
    }

    if (!m_canTrigger)                                      // Exceptions (some periodic triggers)
    {
        switch (m_spellInfo->GetSpellFamilyName())
        {
            case SPELLFAMILY_MAGE:
                // Arcane Missles / Blizzard triggers need do it
                if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0000000000200080)))
                {
                    m_canTrigger = true;
                }
                // Clearcasting trigger need do it
                else if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0000000200000000), 0x00000008))
                {
                    m_canTrigger = true;
                }
                // Replenish Mana, item spell with triggered cases (Mana Agate, etc mana gems)
                else if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0000010000000000)))
                {
                    m_canTrigger = true;
                }
                break;
            case SPELLFAMILY_WARLOCK:
                // For Hellfire Effect / Rain of Fire / Seed of Corruption triggers need do it
                if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0000800000000060)))
                {
                    m_canTrigger = true;
                }
                break;
            case SPELLFAMILY_PRIEST:
                // For Penance,Mind Sear,Mind Flay heal/damage triggers need do it
                if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0001800000800000), 0x00000040))
                {
                    m_canTrigger = true;
                }
                break;
            case SPELLFAMILY_ROGUE:
                // For poisons need do it
                if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x000000101001E000)))
                {
                    m_canTrigger = true;
                }
                break;
            case SPELLFAMILY_HUNTER:
                // Hunter Rapid Killing/Explosive Trap Effect/Immolation Trap Effect/Frost Trap Aura/Snake Trap Effect/Explosive Shot
                if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0100200000000214), 0x200))
                {
                    m_canTrigger = true;
                }
                break;
            case SPELLFAMILY_PALADIN:
                // For Judgements (all) / Holy Shock triggers need do it
                if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0001000900B80400)))
                {
                    m_canTrigger = true;
                }
                break;
            default:
                break;
        }
    }

    // Get data for type of attack and fill base info for trigger
    switch (m_spellInfo->GetDmgClass())
    {
        case SPELL_DAMAGE_CLASS_MELEE:
            m_procAttacker = PROC_FLAG_SUCCESSFUL_MELEE_SPELL_HIT;
            if (m_attackType == OFF_ATTACK)
            {
                m_procAttacker |= PROC_FLAG_SUCCESSFUL_OFFHAND_HIT;
            }
            m_procVictim   = PROC_FLAG_TAKEN_MELEE_SPELL_HIT;
            break;
        case SPELL_DAMAGE_CLASS_RANGED:
            // Auto attack
            if (m_spellInfo->HasAttribute(SPELL_ATTR_EX2_AUTOREPEAT_FLAG))
            {
                m_procAttacker = PROC_FLAG_SUCCESSFUL_RANGED_HIT;
                m_procVictim   = PROC_FLAG_TAKEN_RANGED_HIT;
            }
            else // Ranged spell attack
            {
                m_procAttacker = PROC_FLAG_SUCCESSFUL_RANGED_SPELL_HIT;
                m_procVictim   = PROC_FLAG_TAKEN_RANGED_SPELL_HIT;
            }
            break;
        default:
            if (IsPositiveSpell(m_spellInfo->Id))           // Check for positive spell
            {
                m_procAttacker = PROC_FLAG_SUCCESSFUL_POSITIVE_SPELL;
                m_procVictim   = PROC_FLAG_TAKEN_POSITIVE_SPELL;
            }
            else if (m_spellInfo->HasAttribute(SPELL_ATTR_EX2_AUTOREPEAT_FLAG))   // Wands auto attack
            {
                m_procAttacker = PROC_FLAG_SUCCESSFUL_RANGED_HIT;
                m_procVictim   = PROC_FLAG_TAKEN_RANGED_HIT;
            }
            else                                           // Negative spell
            {
                m_procAttacker = PROC_FLAG_SUCCESSFUL_NEGATIVE_SPELL_HIT;
                m_procVictim   = PROC_FLAG_TAKEN_NEGATIVE_SPELL_HIT;
            }
            break;
    }

    // some negative spells have positive effects to another or same targets
    // avoid triggering negative hit for only positive targets
    m_negativeEffectMask = 0x0;
    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
        if (!IsPositiveEffect(m_spellInfo, SpellEffectIndex(i)))
        {
            m_negativeEffectMask |= (1 << i);
        }

    // Hunter traps spells (for Entrapment trigger)
    // Gives your Immolation Trap, Frost Trap, Explosive Trap, and Snake Trap ....
    if (m_spellInfo->IsFitToFamily(SPELLFAMILY_HUNTER, UI64LIT(0x000020000000001C)))
    {
        m_procAttacker |= PROC_FLAG_ON_TRAP_ACTIVATION;
    }
}

/**
 * @brief Clears all accumulated target lists and delay tracking.
 */
void Spell::CleanupTargetList()
{
    m_UniqueTargetInfo.clear();
    m_UniqueGOTargetInfo.clear();
    m_UniqueItemInfo.clear();
    m_delayMoment = 0;
}

/**
 * @brief Adds a unit target entry for a spell effect.
 *
 * @param pVictim The unit target.
 * @param effIndex The effect index being applied.
 */
void Spell::AddUnitTarget(Unit* pVictim, SpellEffectIndex effIndex)
{
    SpellEffectEntry const *spellEffect = m_spellInfo->GetSpellEffect(effIndex);
    if (!spellEffect || spellEffect->Effect == 0)
    {
        return;
    }

    // Check for effect immune skip if immuned
    bool immuned = pVictim->IsImmuneToSpellEffect(m_spellInfo, effIndex, pVictim == m_caster);

    if (pVictim->GetTypeId() == TYPEID_UNIT && ((Creature*)pVictim)->IsTotem() && (m_spellFlags & SPELL_FLAG_REDIRECTED))
    {
        immuned = false;
    }

    ObjectGuid targetGUID = pVictim->GetObjectGuid();

    // Lookup target in already in list
    for (TargetList::iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (targetGUID == ihit->targetGUID)                 // Found in list
        {
            if (!immuned)
            {
                ihit->effectMask |= 1 << effIndex;          // Add only effect mask if not immuned
            }
            return;
        }
    }

    // This is new target calculate data for him

    // Get spell hit result on target
    TargetInfo target;
    target.targetGUID = targetGUID;                         // Store target GUID
    target.effectMask = immuned ? 0 : (1 << effIndex);      // Store index of effect if not immuned
    target.processed  = false;                              // Effects not applied on target

    // Calculate hit result
    target.missCondition = m_caster->SpellHitResult(pVictim, m_spellInfo, m_canReflect);

    // spell fly from visual cast object
    WorldObject* affectiveObject = GetAffectiveCasterObject();

    // Spell have speed (possible inherited from triggering spell) - need calculate incoming time
    float speed = m_spellInfo->speed == 0.0f && m_triggeredBySpellInfo ? m_triggeredBySpellInfo->speed : m_spellInfo->speed;
    if (speed > 0.0f && affectiveObject && (pVictim != affectiveObject || (m_targets.m_targetMask & (TARGET_FLAG_SOURCE_LOCATION | TARGET_FLAG_DEST_LOCATION))))
    {
        // calculate spell incoming interval
        float dist = 0.0f;                                  // distance to impact
        if (pVictim == affectiveObject)                     // Calculate dist to destination target also for self-cast spells
        {
            if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
            {
                dist = affectiveObject->GetDistance(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ);
            }
            else                                            // Must have Source Target
            {
                dist = affectiveObject->GetDistance(m_targets.m_srcX, m_targets.m_srcY, m_targets.m_srcZ);
            }
        }
        else                                                // normal unit target, take distance
        {
            dist = affectiveObject->GetDistance(pVictim->GetPositionX(), pVictim->GetPositionY(), pVictim->GetPositionZ());
        }

        if (dist < 5.0f)
        {
            dist = 5.0f;
        }
        target.timeDelay = (uint64) floor(dist / speed * 1000.0f);

        // Calculate minimum incoming time
        if (m_delayMoment == 0 || m_delayMoment > target.timeDelay)
        {
            m_delayMoment = target.timeDelay;
        }
    }
    // Spell casted on self - mostly TRIGGER_MISSILE code
    else if (m_spellInfo->speed > 0.0f && affectiveObject && pVictim == affectiveObject)
    {
        float dist = 0.0f;
        if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
        {
            dist = affectiveObject->GetDistance(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ);
        }

        target.timeDelay = (uint64) floor(dist / m_spellInfo->speed * 1000.0f);
    }
    else
    {
        target.timeDelay = UI64LIT(0);
    }

    // If target reflect spell back to caster
    if (target.missCondition == SPELL_MISS_REFLECT)
    {
        // Calculate reflected spell result on caster
        target.reflectResult =  m_caster->SpellHitResult(m_caster, m_spellInfo, m_canReflect);

        if (target.reflectResult == SPELL_MISS_REFLECT)     // Impossible reflect again, so simply deflect spell
        {
            target.reflectResult = SPELL_MISS_PARRY;
        }

        // Increase time interval for reflected spells by 1.5
        target.timeDelay += target.timeDelay >> 1;

        m_spellFlags |= SPELL_FLAG_REFLECTED;
    }
    else
    {
        target.reflectResult = SPELL_MISS_NONE;
    }

    // Add target to list
    m_UniqueTargetInfo.push_back(target);
}

/**
 * @brief Resolves and adds a unit target by guid for a spell effect.
 *
 * @param unitGuid The unit guid to resolve.
 * @param effIndex The effect index being applied.
 */
void Spell::AddUnitTarget(ObjectGuid unitGuid, SpellEffectIndex effIndex)
{
    if (Unit* unit = m_caster->GetObjectGuid() == unitGuid ? m_caster : sObjectAccessor.GetUnit(*m_caster, unitGuid))
    {
        AddUnitTarget(unit, effIndex);
    }
}

/**
 * @brief Adds a game object target entry for a spell effect.
 *
 * @param pVictim The game object target.
 * @param effIndex The effect index being applied.
 */
void Spell::AddGOTarget(GameObject* pVictim, SpellEffectIndex effIndex)
{
    SpellEffectEntry const* spellEffect = m_spellInfo->GetSpellEffect(effIndex);
    if (!spellEffect || spellEffect->Effect == 0)
    {
        return;
    }

    ObjectGuid targetGUID = pVictim->GetObjectGuid();

    // Lookup target in already in list
    for (GOTargetList::iterator ihit = m_UniqueGOTargetInfo.begin(); ihit != m_UniqueGOTargetInfo.end(); ++ihit)
    {
        if (targetGUID == ihit->targetGUID)                 // Found in list
        {
            ihit->effectMask |= (1 << effIndex);            // Add only effect mask
            return;
        }
    }

    // This is new target calculate data for him

    GOTargetInfo target;
    target.targetGUID = targetGUID;
    target.effectMask = (1 << effIndex);
    target.processed  = false;                              // Effects not apply on target

    // spell fly from visual cast object
    WorldObject* affectiveObject = GetAffectiveCasterObject();

    // Spell can have speed - need calculate incoming time
    float speed = m_spellInfo->speed == 0.0f && m_triggeredBySpellInfo ? m_triggeredBySpellInfo->speed : m_spellInfo->speed;
    if (speed > 0.0f && affectiveObject && pVictim != affectiveObject)
    {
        // calculate spell incoming interval
        float dist = affectiveObject->GetDistance(pVictim->GetPositionX(), pVictim->GetPositionY(), pVictim->GetPositionZ());
        if (dist < 5.0f)
        {
            dist = 5.0f;
        }
        target.timeDelay = (uint64) floor(dist / speed * 1000.0f);
        if (m_delayMoment == 0 || m_delayMoment > target.timeDelay)
        {
            m_delayMoment = target.timeDelay;
        }
    }
    else
    {
        target.timeDelay = UI64LIT(0);
    }

    // Add target to list
    m_UniqueGOTargetInfo.push_back(target);
}

/**
 * @brief Resolves and adds a game object target by guid for a spell effect.
 *
 * @param goGuid The game object guid to resolve.
 * @param effIndex The effect index being applied.
 */
void Spell::AddGOTarget(ObjectGuid goGuid, SpellEffectIndex effIndex)
{
    if (GameObject* go = m_caster->GetMap()->GetGameObject(goGuid))
    {
        AddGOTarget(go, effIndex);
    }
}

/**
 * @brief Adds an item target entry for a spell effect.
 *
 * @param pitem The item target.
 * @param effIndex The effect index being applied.
 */
void Spell::AddItemTarget(Item* pitem, SpellEffectIndex effIndex)
{
    SpellEffectEntry const* spellEffect = m_spellInfo->GetSpellEffect(effIndex);
    if (!spellEffect || spellEffect->Effect == 0)
    {
        return;
    }

    // Lookup target in already in list
    for (ItemTargetList::iterator ihit = m_UniqueItemInfo.begin(); ihit != m_UniqueItemInfo.end(); ++ihit)
    {
        if (pitem == ihit->item)                            // Found in list
        {
            ihit->effectMask |= (1 << effIndex);            // Add only effect mask
            return;
        }
    }

    // This is new target add data

    ItemTargetInfo target;
    target.item       = pitem;
    target.effectMask = (1 << effIndex);
    m_UniqueItemInfo.push_back(target);
}

/**
 * @brief Checks whether required alive targets are present in the current target list.
 *
 * @return True if all required effects have a valid alive target; otherwise, false.
 */
bool Spell::IsAliveUnitPresentInTargetList()
{
    // Not need check return true
    if (m_needAliveTargetMask == 0)
    {
        return true;
    }

    uint8 needAliveTargetMask = m_needAliveTargetMask;

    for (TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if (ihit->missCondition == SPELL_MISS_NONE && (needAliveTargetMask & ihit->effectMask))
        {
            Unit* unit = m_caster->GetObjectGuid() == ihit->targetGUID ? m_caster : sObjectAccessor.GetUnit(*m_caster, ihit->targetGUID);

            // either unit is alive and normal spell, or unit dead and deathonly-spell
            if (unit && (unit->IsAlive() != IsDeathOnlySpell(m_spellInfo)))
            {
                needAliveTargetMask &= ~ihit->effectMask;   // remove from need alive mask effect that have alive target
            }
        }
    }

    // is all effects from m_needAliveTargetMask have alive targets
    return needAliveTargetMask == 0;
}

/**
 * @brief Dispatches one spell effect against the current resolved targets.
 *
 * @param pUnitTarget The unit target, if any.
 * @param pItemTarget The item target, if any.
 * @param pGOTarget The game object target, if any.
 * @param i The effect index to process.
 * @param DamageMultiplier The damage multiplier to apply for the effect.
 */
void Spell::HandleEffects(Unit* pUnitTarget, Item* pItemTarget, GameObject* pGOTarget, SpellEffectIndex i, float DamageMultiplier)
{
    unitTarget = pUnitTarget;
    itemTarget = pItemTarget;
    gameObjTarget = pGOTarget;

    SpellEffectEntry const* spellEffect = m_spellInfo->GetSpellEffect(SpellEffectIndex(i));

    damage = int32(CalculateDamage(i, unitTarget) * DamageMultiplier);

    if (spellEffect)
    {
        if (spellEffect->Effect < TOTAL_SPELL_EFFECTS)
        {
            DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell %u Effect%d : %u Targets: %s, %s, %s",
                m_spellInfo->Id, i, spellEffect->Effect,
                unitTarget ? unitTarget->GetGuidStr().c_str() : "-",
                itemTarget ? itemTarget->GetGuidStr().c_str() : "-",
                gameObjTarget ? gameObjTarget->GetGuidStr().c_str() : "-");

            (*this.*SpellEffects[spellEffect->Effect])(spellEffect);
        }
        else
        {
            sLog.outError("WORLD: Spell %u Effect%d : %u > TOTAL_SPELL_EFFECTS", m_spellInfo->Id, i, spellEffect->Effect);
        }
    }
    else
    {
        sLog.outError("WORLD: Spell %u has no effect at index %u", m_spellInfo->Id, i);
    }
}

/**
 * @brief Queues a spell to be triggered after successful completion.
 *
 * @param spellId The triggered spell identifier.
 */
void Spell::AddTriggeredSpell(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);

    if (!spellInfo)
    {
        sLog.outError("Spell::AddTriggeredSpell: unknown spell id %u used as triggred spell for spell %u)", spellId, m_spellInfo->Id);
        return;
    }

    m_TriggerSpells.push_back(spellInfo);
}

/**
 * @brief Queues a spell to be cast before applying effects to each target.
 *
 * @param spellId The precast spell identifier.
 */
void Spell::AddPrecastSpell(uint32 spellId)
{
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);

    if (!spellInfo)
    {
        sLog.outError("Spell::AddPrecastSpell: unknown spell id %u used as pre-cast spell for spell %u)", spellId, m_spellInfo->Id);
        return;
    }

    m_preCastSpells.push_back(spellInfo);
}

/**
 * @brief Casts spells queued to trigger after the main spell completes successfully.
 */
void Spell::CastTriggerSpells()
{
    for (SpellInfoList::const_iterator si = m_TriggerSpells.begin(); si != m_TriggerSpells.end(); ++si)
    {
        Spell* spell = new Spell(m_caster, (*si), true, m_originalCasterGUID);
        spell->SpellStart(&m_targets);                         // use original spell original targets
    }
}

/**
 * @brief Casts queued precast spells on the provided target.
 *
 * @param target The unit target for the precast spells.
 */
void Spell::CastPreCastSpells(Unit* target)
{
    for (SpellInfoList::const_iterator si = m_preCastSpells.begin(); si != m_preCastSpells.end(); ++si)
    {
        m_caster->CastSpell(target, (*si), true, m_CastItem);
    }
}

/**
 * @brief Gets the first queued unit target guid for an effect, falling back to the explicit target guid.
 *
 * @param effIndex The effect index to inspect.
 * @return The matching unit target guid, or the explicit unit target guid when none is queued.
 */
Unit* Spell::GetPrefilledUnitTargetOrUnitTarget(SpellEffectIndex effIndex) const
{
    for (TargetList::const_iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
        if (itr->effectMask & (1 << effIndex))
        {
            return m_caster->GetMap()->GetUnit(itr->targetGUID);
        }

    return m_targets.getUnitTarget();
}

/**
 * @brief Applies spell pushback delay to a currently casting player spell.
 */
void Spell::Delayed()
{
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (m_spellState == SPELL_STATE_DELAYED)
    {
        return;                                              // spell is active and can't be time-backed
    }

    if (isDelayableNoMore())                                // Spells may only be delayed twice
    {
        return;
    }

    // spells not losing casting time ( slam, dynamites, bombs.. )
    if (!(m_spellInfo->GetInterruptFlags() & SPELL_INTERRUPT_FLAG_DAMAGE))
    {
        return;
    }

    // check pushback reduce
    int32 delaytime = 500;                                  // spellcasting delay is normally 500ms
    int32 delayReduce = 100;                                // must be initialized to 100 for percent modifiers
    ((Player*)m_caster)->ApplySpellMod(m_spellInfo->Id, SPELLMOD_NOT_LOSE_CASTING_TIME, delayReduce, this);
    delayReduce += m_caster->GetTotalAuraModifier(SPELL_AURA_REDUCE_PUSHBACK) - 100;
    if (delayReduce >= 100)
    {
        return;
    }

    delaytime = delaytime * (100 - delayReduce) / 100;

    if (int32(m_timer) + delaytime > m_casttime)
    {
        delaytime = m_casttime - m_timer;
        m_timer = m_casttime;
    }
    else
    {
        m_timer += delaytime;
    }

    DETAIL_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell %u partially interrupted for (%d) ms at damage", m_spellInfo->Id, delaytime);

    WorldPacket data(SMSG_SPELL_DELAYED, 8 + 4);
    data << m_caster->GetPackGUID();
    data << uint32(delaytime);

    m_caster->SendMessageToSet(&data, true);
}

/**
 * @brief Applies pushback to an active channeled spell and linked aura durations.
 */
void Spell::DelayedChannel()
{
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER || getState() != SPELL_STATE_CASTING)
    {
        return;
    }

    if (isDelayableNoMore())                                // Spells may only be delayed twice
    {
        return;
    }

    // check pushback reduce
    int32 delaytime = GetSpellDuration(m_spellInfo) * 25 / 100;// channeling delay is normally 25% of its time per hit
    int32 delayReduce = 100;                                // must be initialized to 100 for percent modifiers
    ((Player*)m_caster)->ApplySpellMod(m_spellInfo->Id, SPELLMOD_NOT_LOSE_CASTING_TIME, delayReduce, this);
    delayReduce += m_caster->GetTotalAuraModifier(SPELL_AURA_REDUCE_PUSHBACK) - 100;
    if (delayReduce >= 100)
    {
        return;
    }

    delaytime = delaytime * (100 - delayReduce) / 100;

    if (int32(m_timer) < delaytime)
    {
        delaytime = m_timer;
        m_timer = 0;
    }
    else
    {
        m_timer -= delaytime;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell %u partially interrupted for %i ms, new duration: %u ms", m_spellInfo->Id, delaytime, m_timer);

    for (TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
    {
        if ((*ihit).missCondition == SPELL_MISS_NONE)
        {
            if (Unit* unit = m_caster->GetObjectGuid() == ihit->targetGUID ? m_caster : sObjectAccessor.GetUnit(*m_caster, ihit->targetGUID))
            {
                unit->DelaySpellAuraHolder(m_spellInfo->Id, delaytime, unit->GetObjectGuid());
            }
        }
    }

    for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
    {
        // partially interrupt persistent area auras
        if (DynamicObject* dynObj = m_caster->GetDynObject(m_spellInfo->Id, SpellEffectIndex(j)))
        {
            dynObj->Delay(delaytime);
        }
    }

    SendChannelUpdate(m_timer);
}

/**
 * @brief Refreshes the cached original caster pointer from the stored guid.
 */
void Spell::UpdateOriginalCasterPointer()
{
    if (m_originalCasterGUID == m_caster->GetObjectGuid())
    {
        m_originalCaster = m_caster;
    }
    else if (m_originalCasterGUID.IsGameObject())
    {
        GameObject* go = m_caster->IsInWorld() ? m_caster->GetMap()->GetGameObject(m_originalCasterGUID) : NULL;
        m_originalCaster = go ? go->GetOwner() : NULL;
    }
    else
    {
        Unit* unit = sObjectAccessor.GetUnit(*m_caster, m_originalCasterGUID);
        m_originalCaster = unit && unit->IsInWorld() ? unit : NULL;
    }
}

/**
 * @brief Refreshes cached caster and target pointers from stored guids.
 */
void Spell::UpdatePointers()
{
    UpdateOriginalCasterPointer();

    m_targets.Update(m_caster);

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        m_CastItem = ((Player *)m_caster)->GetItemByGuid(m_CastItemGuid);
    }
    else
    {
        m_CastItem = NULL;
    }
}

/**
 * @brief Checks whether a target matches the spell's creature type restrictions.
 *
 * @param target The target being validated.
 * @return True if the target type is allowed; otherwise, false.
 */
bool Spell::CheckTargetCreatureType(Unit* target) const
{
    uint32 spellCreatureTargetMask = m_spellInfo->GetTargetCreatureType();

    // Curse of Doom: not find another way to fix spell target check :/
    if (m_spellInfo->GetSpellFamilyName() == SPELLFAMILY_WARLOCK && m_spellInfo->GetCategory() == 1179)
    {
        // not allow cast at player
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            return false;
        }

        spellCreatureTargetMask = 0x7FF;
    }

    // Dismiss Pet, Taming Lesson and Control Robot skipped
    if (m_spellInfo->Id == 2641 || m_spellInfo->Id == 23356 || m_spellInfo->Id == 30009)
    {
        spellCreatureTargetMask =  0;
    }

    if (spellCreatureTargetMask)
    {
        uint32 TargetCreatureType = target->GetCreatureTypeMask();

        return !TargetCreatureType || (spellCreatureTargetMask & TargetCreatureType);
    }
    return true;
}

/**
 * @brief Gets the current spell container slot used by this spell.
 *
 * @return The current spell container type.
 */
CurrentSpellTypes Spell::GetCurrentContainer()
{
    if (IsNextMeleeSwingSpell())
    {
        return (CURRENT_MELEE_SPELL);
    }
    else if (IsAutoRepeat())
    {
        return (CURRENT_AUTOREPEAT_SPELL);
    }
    else if (IsChanneledSpell(m_spellInfo))
    {
        return (CURRENT_CHANNELED_SPELL);
    }
    else
    {
        return (CURRENT_GENERIC_SPELL);
    }
}

/**
 * @brief Validates whether a candidate target is acceptable for a specific effect.
 *
 * @param target The target being checked.
 * @param eff The effect index being validated.
 * @return True if the target is valid for the effect; otherwise, false.
 */
bool Spell::CheckTarget(Unit* target, SpellEffectIndex eff)
{
    SpellEffectEntry const* spellEffect = m_spellInfo->GetSpellEffect(eff);
    if (!spellEffect)
    {
        return false;
    }

    // Check targets for creature type mask and remove not appropriate (skip explicit self target case, maybe need other explicit targets)
    if (spellEffect->EffectImplicitTargetA != TARGET_SELF )
    {
        if (!CheckTargetCreatureType(target))
        {
            return false;
        }
    }

    // Check Aura spell req (need for AoE spells)
    SpellAuraRestrictionsEntry const* auraRestrictions = m_spellInfo->GetSpellAuraRestrictions();
    if (auraRestrictions)
    {
        if (auraRestrictions->targetAuraSpell && !target->HasAura(auraRestrictions->targetAuraSpell))
        {
            return false;
        }
        if (auraRestrictions->excludeTargetAuraSpell && target->HasAura(auraRestrictions->excludeTargetAuraSpell))
        {
            return false;
        }
    }

    // Check targets for not_selectable unit flag and remove
    // A player can cast spells on his pet (or other controlled unit) though in any state
    if (target != m_caster && target->GetCharmerOrOwnerGuid() != m_caster->GetObjectGuid())
    {
        // any unattackable target skipped
        if (target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
        {
            return false;
        }

        // unselectable targets skipped in all cases except TARGET_SCRIPT targeting or vehicle passengers
        // in case TARGET_SCRIPT target selected by server always and can't be cheated
        if ((!m_IsTriggeredSpell || target != m_targets.getUnitTarget()) &&
            target->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE) &&
            (!target->GetTransportInfo() || (target->GetTransportInfo() &&
            !((Unit*)target->GetTransportInfo()->GetTransport())->IsVehicle())) &&
            spellEffect->EffectImplicitTargetA != TARGET_SCRIPT &&
            spellEffect->EffectImplicitTargetB != TARGET_SCRIPT &&
            spellEffect->EffectImplicitTargetA != TARGET_AREAEFFECT_INSTANT &&
            spellEffect->EffectImplicitTargetB != TARGET_AREAEFFECT_INSTANT &&
            spellEffect->EffectImplicitTargetA != TARGET_AREAEFFECT_CUSTOM &&
            spellEffect->EffectImplicitTargetB != TARGET_AREAEFFECT_CUSTOM &&
            spellEffect->EffectImplicitTargetA != TARGET_NARROW_FRONTAL_CONE &&
            spellEffect->EffectImplicitTargetB != TARGET_NARROW_FRONTAL_CONE &&
            spellEffect->EffectImplicitTargetA != TARGET_NARROW_FRONTAL_CONE_2 &&
            spellEffect->EffectImplicitTargetB != TARGET_NARROW_FRONTAL_CONE_2)
            {
                return false;
            }
    }

    // Check player targets and remove if in GM mode or GM invisibility (for not self casting case)
    if (target != m_caster && target->GetTypeId() == TYPEID_PLAYER)
    {
        if (((Player*)target)->GetVisibility() == VISIBILITY_OFF)
        {
            return false;
        }

        if (((Player*)target)->isGameMaster() && !IsPositiveSpell(m_spellInfo->Id))
        {
            return false;
        }
    }

    // Check targets for LOS visibility (except spells without range limitations )
    if (!DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, m_spellInfo->Id, NULL, SPELL_ATTR_EX2_IGNORE_LOS))
    {
        switch(spellEffect->Effect)
        {
        case SPELL_EFFECT_SUMMON_PLAYER:                    // from anywhere
            break;
        case SPELL_EFFECT_DUMMY:
            if (m_spellInfo->Id != 20577)                   // Cannibalize
            {
                break;
            }
            // fall through
        case SPELL_EFFECT_RESURRECT_NEW:
            // player far away, maybe his corpse near?
            if (target != m_caster && !m_spellInfo->HasAttribute(SPELL_ATTR_EX2_IGNORE_LOS) && !target->IsWithinLOSInMap(m_caster))
            {
                if (!m_targets.getCorpseTargetGuid())
                {
                    return false;
                }

                Corpse* corpse = m_caster->GetMap()->GetCorpse(m_targets.getCorpseTargetGuid());
                if (!corpse)
                {
                    return false;
                }

                if (target->GetObjectGuid() != corpse->GetOwnerGuid())
                {
                    return false;
                }

                if (!m_spellInfo->HasAttribute(SPELL_ATTR_EX2_IGNORE_LOS) && !corpse->IsWithinLOSInMap(m_caster))
                {
                    return false;
                }
            }

            // all ok by some way or another, skip normal check
            break;
        default:                                            // normal case
            // Get GO cast coordinates if original caster -> GO
            if (target != m_caster)
            {
                if (WorldObject* caster = GetCastingObject())
                {
                    if (!m_spellInfo->HasAttribute(SPELL_ATTR_EX2_IGNORE_LOS) && !target->IsWithinLOSInMap(caster))
                    {
                        return false;
                    }
                }
            }
            break;
        }
    }

    if (target->GetTypeId() != TYPEID_PLAYER && m_spellInfo->HasAttribute(SPELL_ATTR_EX3_TARGET_ONLY_PLAYER)
        && spellEffect->EffectImplicitTargetA != TARGET_SCRIPT && spellEffect->EffectImplicitTargetA != TARGET_SELF)
    {
        return false;
    }

    switch (m_spellInfo->Id)
    {
        case 37433:                                         // Spout (The Lurker Below), only players affected if its not in water
            if (target->GetTypeId() != TYPEID_PLAYER || target->IsInWater())
            {
                return false;
            }
            break;
        case 68921:                                         // Soulstorm (FoS), only targets farer than 10 away
        case 69049:                                         // Soulstorm            - = -
            if (m_caster->IsWithinDist(target, 10.0f, false))
            {
                return false;
            }
            break;
        default:
            break;
    }

    return true;
}

/**
 * @brief Checks whether this spell cast should produce client-visible packets.
 *
 * @return True if packets should be sent to clients; otherwise, false.
 */
bool Spell::IsNeedSendToClient() const
{
    return m_spellInfo->SpellVisual[0] || m_spellInfo->SpellVisual[1] || IsChanneledSpell(m_spellInfo) ||
           m_spellInfo->speed > 0.0f || (!m_triggeredByAuraSpell && !m_IsTriggeredSpell);
}

/**
 * @brief Checks whether the triggered spell still requires redundant cast-time handling.
 *
 * @return True if redundant cast-time handling is needed; otherwise, false.
 */
bool Spell::IsTriggeredSpellWithRedundentCastTime() const
{
    if (m_IsTriggeredSpell && (m_spellInfo->GetManaCost() || m_spellInfo->GetManaCostPercentage()))
    {
        return true;
    }

    // Mastery / talent "overload" spells: zero-cost duplicates of caster
    // spells triggered by Cata's Elemental Overload mastery (77222) and
    // the WotLK Lightning Overload talent (SpellIconID 2018). Their DBC
    // entries carry the source spell's cast time (so the spell preview /
    // tooltip render correctly) but their mana cost is zero — the
    // triggering spell already paid. The mana-cost heuristic above misses
    // them, so the framework respects the source cast time and the
    // triggered cast lands seconds after the proc rather than instantly,
    // which contradicts Cata mastery design (verified in-game on
    // 2026-05-17: source Lightning Bolt landed at 19:55:15.009 and the
    // 45284 overload landed at 19:55:17.924 — a ~2.9s delay matching the
    // source LB cast time, while the Lava Burst overload 77451 landed
    // with only the expected ~0.4s projectile delay). Add an explicit
    // allowlist of the affected spell IDs. This also fixes the latent
    // WotLK Lightning Overload talent (preexisting issue that was never
    // reported, presumably because the talent was rarely used at high
    // gear levels where the delay would be most visible).
    if (m_IsTriggeredSpell)
    {
        switch (m_spellInfo->Id)
        {
            // Lightning Bolt overload (ranks 1-14)
            case 45284:
            case 45286:
            case 45287:
            case 45288:
            case 45289:
            case 45290:
            case 45291:
            case 45292:
            case 45293:
            case 45294:
            case 45295:
            case 45296:
            case 49239:
            case 49240:
            // Chain Lightning overload (ranks 1-8)
            case 45297:
            case 45298:
            case 45299:
            case 45300:
            case 45301:
            case 45302:
            case 49268:
            case 49269:
            // Lava Burst overload (Cata; no rank progression). Listed
            // defensively — currently lands instantly in-game, suggesting
            // its DBC already has a non-zero mana cost or zero cast time,
            // but the explicit entry documents the intent.
            case 77451:
                return true;
        }
    }

    return false;
}

/**
 * @brief Checks whether any queued target entry contains a given effect.
 *
 * @param effect The effect index to look for.
 * @return True if at least one target has the effect queued; otherwise, false.
 */
bool Spell::HaveTargetsForEffect(SpellEffectIndex effect) const
{
    for (TargetList::const_iterator itr = m_UniqueTargetInfo.begin(); itr != m_UniqueTargetInfo.end(); ++itr)
    {
        if (itr->effectMask & (1 << effect))
        {
            return true;
        }
    }

    for (GOTargetList::const_iterator itr = m_UniqueGOTargetInfo.begin(); itr != m_UniqueGOTargetInfo.end(); ++itr)
    {
        if (itr->effectMask & (1 << effect))
        {
            return true;
        }
    }

    for (ItemTargetList::const_iterator itr = m_UniqueItemInfo.begin(); itr != m_UniqueItemInfo.end(); ++itr)
    {
        if (itr->effectMask & (1 << effect))
        {
            return true;
        }
    }

    return false;
}

SpellEvent::SpellEvent(Spell* spell) : BasicEvent()
{
    m_Spell = spell;
}

SpellEvent::~SpellEvent()
{
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
    {
        m_Spell->cancel();
    }

    if (m_Spell->IsDeletable())
    {
        delete m_Spell;
    }
    else
    {
        sLog.outError("~SpellEvent: %s %u tried to delete non-deletable spell %u. Was not deleted, causes memory leak.",
                      (m_Spell->GetCaster()->GetTypeId() == TYPEID_PLAYER ? "Player" : "Creature"), m_Spell->GetCaster()->GetGUIDLow(), m_Spell->m_spellInfo->Id);
    }
}

/**
 * @brief Advances spell execution within the event queue.
 *
 * @param e_time The event execution time.
 * @param p_time The elapsed update time in milliseconds.
 * @return True when the event is complete and can be removed; otherwise, false.
 */
bool SpellEvent::Execute(uint64 e_time, uint32 p_time)
{
    // update spell if it is not finished
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
    {
        m_Spell->update(p_time);
    }

    // check spell state to process
    switch (m_Spell->getState())
    {
        case SPELL_STATE_FINISHED:
        {
            // spell was finished, check deletable state
            if (m_Spell->IsDeletable())
            {
                // check, if we do have unfinished triggered spells
                return true;                                // spell is deletable, finish event
            }
            // event will be re-added automatically at the end of routine)
        } break;

        case SPELL_STATE_CASTING:
        {
            // this spell is in channeled state, process it on the next update
            // event will be re-added automatically at the end of routine)
        } break;

        case SPELL_STATE_DELAYED:
        {
            // first, check, if we have just started
            if (m_Spell->GetDelayStart() != 0)
            {
                // no, we aren't, do the typical update
                // check, if we have channeled spell on our hands
                if (IsChanneledSpell(m_Spell->m_spellInfo))
                {
                    // evented channeled spell is processed separately, casted once after delay, and not destroyed till finish
                    // check, if we have casting anything else except this channeled spell and autorepeat
                    if (m_Spell->GetCaster()->IsNonMeleeSpellCasted(false, true, true))
                    {
                        // another non-melee non-delayed spell is casted now, abort
                        m_Spell->cancel();
                    }
                    else
                    {
                        // do the action (pass spell to channeling state)
                        m_Spell->handle_immediate();
                    }
                    // event will be re-added automatically at the end of routine)
                }
                else
                {
                    // run the spell handler and think about what we can do next
                    uint64 t_offset = e_time - m_Spell->GetDelayStart();
                    uint64 n_offset = m_Spell->handle_delayed(t_offset);
                    if (n_offset)
                    {
                        // re-add us to the queue
                        m_Spell->GetCaster()->m_Events.AddEvent(this, m_Spell->GetDelayStart() + n_offset, false);
                        return false;                       // event not complete
                    }
                    // event complete
                    // finish update event will be re-added automatically at the end of routine)
                }
            }
            else
            {
                // delaying had just started, record the moment
                m_Spell->SetDelayStart(e_time);
                // re-plan the event for the delay moment
                m_Spell->GetCaster()->m_Events.AddEvent(this, e_time + m_Spell->GetDelayMoment(), false);
                return false;                               // event not complete
            }
        } break;

        default:
        {
            // all other states
            // event will be re-added automatically at the end of routine)
        } break;
    }

    // spell processing not complete, plan event on the next update interval
    m_Spell->GetCaster()->m_Events.AddEvent(this, e_time + 1, false);
    return false;                                           // event not complete
}

/**
 * @brief Aborts the queued spell event and cancels the spell if needed.
 *
 * @param e_time Unused event time.
 */
void SpellEvent::Abort(uint64 /*e_time*/)
{
    // oops, the spell we try to do is aborted
    if (m_Spell->getState() != SPELL_STATE_FINISHED)
    {
        m_Spell->cancel();
    }
}

/**
 * @brief Checks whether the underlying spell can be deleted.
 *
 * @return True if the spell is deletable; otherwise, false.
 */
bool SpellEvent::IsDeletable() const
{
    return m_Spell->IsDeletable();
}

/**
 * @brief Validates whether the caster can open a lock with this spell effect.
 *
 * @param effIndex The effect index performing the open-lock action.
 * @param lockId The lock identifier.
 * @param skillId Receives the required skill type.
 * @param reqSkillValue Receives the required skill value.
 * @param skillValue Receives the caster's effective skill value.
 * @return The resulting cast status.
 */
SpellCastResult Spell::CanOpenLock(SpellEffectIndex effIndex, uint32 lockId, SkillType& skillId, int32& reqSkillValue, int32& skillValue)
{
    if (!lockId)                                            // possible case for GO and maybe for items.
    {
        return SPELL_CAST_OK;
    }

    // Get LockInfo
    LockEntry const* lockInfo = sLockStore.LookupEntry(lockId);

    if (!lockInfo)
    {
        return SPELL_FAILED_BAD_TARGETS;
    }

    bool reqKey = false;                                    // some locks not have reqs

    for (int j = 0; j < 8; ++j)
    {
        switch (lockInfo->Type[j])
        {
                // check key item (many fit cases can be)
            case LOCK_KEY_ITEM:
            {
                if (lockInfo->Index[j] && m_CastItem && m_CastItem->GetEntry() == lockInfo->Index[j])
                {
                    return SPELL_CAST_OK;
                }
                reqKey = true;
                break;
                // check key skill (only single first fit case can be)
            }
            case LOCK_KEY_SKILL:
            {
                reqKey = true;

                // wrong locktype, skip
                if (uint32(m_spellInfo->GetEffectMiscValue(effIndex)) != lockInfo->Index[j])
                {
                    continue;
                }

                skillId = SkillByLockType(LockType(lockInfo->Index[j]));

                if (skillId != SKILL_NONE || skillId == MAX_SKILL_TYPE)
                {
                    // skill bonus provided by casting spell (mostly item spells)
                    // add the damage modifier from the spell casted (cheat lock / skeleton key etc.) (use m_currentBasePoints, CalculateDamage returns wrong value)
                    uint32 spellSkillBonus = uint32(m_currentBasePoints[effIndex]);
                    reqSkillValue = lockInfo->Skill[j];

                    // castitem check: rogue using skeleton keys. the skill values should not be added in this case.
                    // MAX_SKILL_TYPE - skill value Scales with caster level
                    if (skillId == MAX_SKILL_TYPE)
                    {
                        skillValue = m_CastItem || m_caster->GetTypeId() != TYPEID_PLAYER ? 0 : m_caster->getLevel() * 5;
                    }
                    else
                    {
                        skillValue = m_CastItem || m_caster->GetTypeId() != TYPEID_PLAYER ? 0 : ((Player*)m_caster)->GetSkillValue(skillId);
                    }

                    skillValue += spellSkillBonus;

                    if (skillValue < reqSkillValue)
                    {
                        return SPELL_FAILED_LOW_CASTLEVEL;
                    }
                }

                return SPELL_CAST_OK;
            }
        }
    }

    if (reqKey)
    {
        return SPELL_FAILED_BAD_TARGETS;
    }

    return SPELL_CAST_OK;
}

/**
 * Fill target list by units around (x,y) points at radius distance

 * @param targetUnitMap        Reference to target list that filled by function
 * @param radius               Radius around (x,y) for target search
 * @param pushType             Additional rules for target area selection (in front, angle, etc)
 * @param spellTargets         Additional rules for target selection base at hostile/friendly state to original spell caster
 * @param originalCaster       If provided set alternative original caster, if =NULL then used Spell::GetAffectiveObject() return
 */
void Spell::FillAreaTargets(UnitList& targetUnitMap, float radius, SpellNotifyPushType pushType, SpellTargets spellTargets, WorldObject* originalCaster /*=NULL*/)
{
    MaNGOS::SpellNotifierCreatureAndPlayer notifier(*this, targetUnitMap, radius, pushType, spellTargets, originalCaster);
    Cell::VisitAllObjects(notifier.GetCenterX(), notifier.GetCenterY(), m_caster->GetMap(), notifier, radius);
}

/**
 * @brief Fills a target list with party or raid members around a reference unit.
 *
 * @param targetUnitMap The target list being populated.
 * @param member The reference member.
 * @param radius The search radius.
 * @param raid True to include the whole raid; false to limit to the subgroup.
 * @param withPets True to include pets.
 * @param withcaster True to include the caster when applicable.
 */
void Spell::FillRaidOrPartyTargets(UnitList& targetUnitMap, Unit* member, Unit* center, float radius, bool raid, bool withPets, bool withcaster)
{
    Player* pMember = member->GetCharmerOrOwnerPlayerOrPlayerItself();
    Group* pGroup = pMember ? pMember->GetGroup() : NULL;

    if (pGroup)
    {
        uint8 subgroup = pMember->GetSubGroup();

        for (GroupReference* itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
        {
            Player* Target = itr->getSource();

            // IsHostileTo check duel and controlled by enemy
            if (Target && (raid || subgroup == Target->GetSubGroup())
                && !m_caster->IsHostileTo(Target))
            {
                if ((Target == center || center->IsWithinDistInMap(Target, radius)) &&
                        (withcaster || Target != m_caster))
                {
                    targetUnitMap.push_back(Target);
                }

                if (withPets)
                    if (Pet* pet = Target->GetPet())
                        if ((pet == center || center->IsWithinDistInMap(pet, radius)) &&
                                (withcaster || pet != m_caster))
                        {
                            targetUnitMap.push_back(pet);
                        }
            }
        }
    }
    else
    {
        Unit* ownerOrSelf = pMember ? pMember : member->GetCharmerOrOwnerOrSelf();
        if ((ownerOrSelf == center || center->IsWithinDistInMap(ownerOrSelf, radius)) &&
                (withcaster || ownerOrSelf != m_caster))
        {
            targetUnitMap.push_back(ownerOrSelf);
        }

        if (withPets)
        {
            if (Pet* pet = ownerOrSelf->GetPet())
            {
                if ((pet == center || center->IsWithinDistInMap(pet, radius)) &&
                        (withcaster || pet != m_caster))
                {
                    targetUnitMap.push_back(pet);
                }
            }
        }
    }
}

void Spell::FillRaidOrPartyManaPriorityTargets(UnitList& targetUnitMap, Unit* member, Unit* center, float radius, uint32 count, bool raid, bool withPets, bool withCaster)
{
    FillRaidOrPartyTargets(targetUnitMap, member, center, radius, raid, withPets, withCaster);

    PrioritizeManaUnitQueue manaUsers;
    for (UnitList::const_iterator itr = targetUnitMap.begin(); itr != targetUnitMap.end(); ++itr)
        if ((*itr)->GetPowerType() == POWER_MANA && !(*itr)->IsDead())
        {
            manaUsers.push(PrioritizeManaUnitWraper(*itr));
        }

    targetUnitMap.clear();
    while (!manaUsers.empty() && targetUnitMap.size() < count)
    {
        targetUnitMap.push_back(manaUsers.top().getUnit());
        manaUsers.pop();
    }
}

void Spell::FillRaidOrPartyHealthPriorityTargets(UnitList& targetUnitMap, Unit* member, Unit* center, float radius, uint32 count, bool raid, bool withPets, bool withCaster)
{
    FillRaidOrPartyTargets(targetUnitMap, member, center, radius, raid, withPets, withCaster);

    PrioritizeHealthUnitQueue healthQueue;
    for (UnitList::const_iterator itr = targetUnitMap.begin(); itr != targetUnitMap.end(); ++itr)
        if (!(*itr)->IsDead())
        {
            healthQueue.push(PrioritizeHealthUnitWraper(*itr));
        }

    targetUnitMap.clear();
    while (!healthQueue.empty() && targetUnitMap.size() < count)
    {
        targetUnitMap.push_back(healthQueue.top().getUnit());
        healthQueue.pop();
    }
}

/**
 * @brief Gets the world object that should be used as the effective spell origin.
 *
 * @return The effective caster world object.
 */
WorldObject* Spell::GetAffectiveCasterObject() const
{
    if (!m_originalCasterGUID)
    {
        return m_caster;
    }

    if (m_originalCasterGUID.IsGameObject() && m_caster->IsInWorld())
    {
        return m_caster->GetMap()->GetGameObject(m_originalCasterGUID);
    }
    return m_originalCaster;
}

/**
 * @brief Gets the world object used for cast-position and line-of-sight calculations.
 *
 * @return The casting world object.
 */
WorldObject* Spell::GetCastingObject() const
{
    if (m_originalCasterGUID.IsGameObject())
    {
        return m_caster->IsInWorld() ? m_caster->GetMap()->GetGameObject(m_originalCasterGUID) : NULL;
    }
    else
    {
        return m_caster;
    }
}

/**
 * @brief Clears the accumulated effect damage and healing counters.
 */
void Spell::ResetEffectDamageAndHeal()
{
    m_damage = 0;
    m_healing = 0;
}

void Spell::SelectMountByAreaAndSkill(Unit* target, SpellEntry const* parentSpell, uint32 spellId75, uint32 spellId150, uint32 spellId225, uint32 spellId300, uint32 spellIdSpecial)
{
    if (!target || target->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Prevent stacking of mounts
    target->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
    uint16 skillval = ((Player*)target)->GetSkillValue(SKILL_RIDING);
    if (!skillval)
    {
        return;
    }

    if (skillval >= 225 && (spellId300 > 0 || spellId225 > 0))
    {
        uint32 spellid = skillval >= 300 ? spellId300 : spellId225;
        SpellEntry const* pSpell = sSpellStore.LookupEntry(spellid);
        if (!pSpell)
        {
            sLog.outError("SelectMountByAreaAndSkill: unknown spell id %i by caster: %s", spellid, target->GetGuidStr().c_str());
            return;
        }

        // zone check
        uint32 zone, area;
        target->GetZoneAndAreaId(zone, area);

        SpellCastResult locRes = sSpellMgr.GetSpellAllowedInLocationError(pSpell, target->GetMapId(), zone, area, target->GetCharmerOrOwnerPlayerOrPlayerItself());
        if (locRes != SPELL_CAST_OK || !((Player*)target)->CanStartFlyInArea(target->GetMapId(), zone, area))
        {
            target->CastSpell(target, spellId150, true, NULL, NULL, ObjectGuid(), parentSpell);
        }
        else if (spellIdSpecial > 0)
        {
            for (PlayerSpellMap::const_iterator iter = ((Player*)target)->GetSpellMap().begin(); iter != ((Player*)target)->GetSpellMap().end(); ++iter)
            {
                if (iter->second.state != PLAYERSPELL_REMOVED)
                {
                    SpellEntry const* spellInfo = sSpellStore.LookupEntry(iter->first);
                    for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
                    {
                        SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(SpellEffectIndex(i));
                        if (!spellEffect)
                        {
                            continue;
                        }

                        if (spellEffect->EffectApplyAuraName == SPELL_AURA_MOD_FLIGHT_SPEED_MOUNTED)
                        {
                            int32 mountSpeed = spellInfo->CalculateSimpleValue(SpellEffectIndex(i));

                            // speed higher than 280 replace it
                            if (mountSpeed > 280)
                            {
                                target->CastSpell(target, spellIdSpecial, true, NULL, NULL, ObjectGuid(), parentSpell);
                                return;
                            }
                        }
                    }
                }
            }
            target->CastSpell(target, pSpell, true, NULL, NULL, ObjectGuid(), parentSpell);
        }
        else
        {
            target->CastSpell(target, pSpell, true, NULL, NULL, ObjectGuid(), parentSpell);
        }
    }
    else if (skillval >= 150 && spellId150 > 0)
    {
        target->CastSpell(target, spellId150, true, NULL, NULL, ObjectGuid(), parentSpell);
    }
    else if (spellId75 > 0)
    {
        target->CastSpell(target, spellId75, true, NULL, NULL, ObjectGuid(), parentSpell);
    }

    return;
}

/**
 * @brief Clears the cached cast item and unlinks it from target data when necessary.
 */
void Spell::ClearCastItem()
{
    if (m_CastItem == m_targets.getItemTarget())
    {
        m_targets.setItemTarget(NULL);
    }

    m_CastItem = NULL;
    m_CastItemGuid.Clear();
}

/**
 * @brief Resolves effective radius, chain target count, and target cap modifiers for an effect.
 *
 * @param spellEffect The spell effect entry being evaluated.
 * @param radius Receives the effective radius.
 * @param EffectChainTarget Receives the effective chain target count.
 * @param unMaxTargets Receives the effective maximum affected target count.
 */
void Spell::GetSpellRangeAndRadius(SpellEffectEntry const* spellEffect, float& radius, uint32& EffectChainTarget, uint32& unMaxTargets) const
{
    if (uint32 radiusIndex = spellEffect->GetRadiusIndex())
    {
        radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(spellEffect->GetRadiusIndex()));
    }
    else
    {
        radius = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex));
    }

    if (Unit* realCaster = GetAffectiveCaster())
    {
        if (Player* modOwner = realCaster->GetSpellModOwner())
        {
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RADIUS, radius, this);
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_JUMP_TARGETS, EffectChainTarget, this);
        }
    }

    // custom target amount cases
    switch(m_spellInfo->GetSpellFamilyName())
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->Id)
            {
                case 802:                                   // Mutate Bug (AQ40, Emperor Vek'nilash)
                case 804:                                   // Explode Bug (AQ40, Emperor Vek'lor)
                case 23138:                                 // Gate of Shazzrah (MC, Shazzrah)
                case 28560:                                 // Summon Blizzard (Naxx, Sapphiron)
                case 30541:                                 // Blaze (Magtheridon)
                case 30572:                                 // Quake (Magtheridon)
                case 30769:                                 // Pick Red Riding Hood (Karazhan, Big Bad Wolf)
                case 30835:                                 // Infernal Relay (Karazhan, Prince Malchezaar)
                case 31347:                                 // Doom (Hyjal Summit, Azgalor)
                case 32312:                                 // Move 1 (Karazhan, Chess Event)
                case 33711:                                 // Murmur's Touch (Shadow Labyrinth, Murmur)
                case 37388:                                 // Move 2 (Karazhan, Chess Event)
                case 38794:                                 // Murmur's Touch (h) (Shadow Labyrinth, Murmur)
                case 39338:                                 // Karazhan - Chess, Medivh CHEAT: Hand of Medivh, Target Horde
                case 39342:                                 // Karazhan - Chess, Medivh CHEAT: Hand of Medivh, Target Alliance
                case 40834:                                 // Agonizing Flames (BT, Illidan Stormrage)
                case 41537:                                 // Summon Enslaved Soul (BT, Reliquary of Souls)
                case 44869:                                 // Spectral Blast (SWP, Kalecgos)
                case 45391:                                 // Summon Demonic Vapor (SWP, Felmyst)
                case 45785:                                 // Sinister Reflection Clone (SWP, Kil'jaeden)
                case 45863:                                 // Cosmetic - Incinerate to Random Target (Borean Tundra)
                case 45892:                                 // Sinister Reflection (SWP, Kil'jaeden)
                case 45976:                                 // Open Portal (SWP, M'uru)
                case 46372:                                 // Ice Spear Target Picker (Slave Pens, Ahune)
                case 47669:                                 // Awaken Subboss (Utgarde Pinnacle)
                case 48278:                                 // Paralyze (Utgarde Pinnacle)
                case 50742:                                 // Ooze Combine (Halls of Stone)
                case 50988:                                 // Glare of the Tribunal (Halls of Stone)
                case 51003:                                 // Summon Dark Matter Target (Halls of Stone)
                case 51146:                                 // Summon Searing Gaze Target (Halls Of Stone)
                case 52438:                                 // Summon Skittering Swarmer (Azjol Nerub,  Krik'thir the Gatewatcher)
                case 52449:                                 // Summon Skittering Infector (Azjol Nerub,  Krik'thir the Gatewatcher)
                case 53457:                                 // Impale (Azjol Nerub,  Anub'arak)
                case 54148:                                 // Ritual of the Sword (Utgarde Pinnacle, Svala)
                case 55479:                                 // Forced Obedience (Naxxramas, Razovius)
                case 56140:                                 // Summon Power Spark (Eye of Eternity, Malygos)
                case 57578:                                 // Lava Strike (Obsidian Sanctum, Sartharion)
                case 59870:                                 // Glare of the Tribunal (h) (Halls of Stone)
                case 62016:                                 // Charge Orb (Ulduar, Thorim)
                case 62042:                                 // Stormhammer (Ulduar, Thorim)
                case 62166:                                 // Stone Grip (Ulduar, Kologarn)
                case 62301:                                 // Cosmic Smash (Ulduar, Algalon)
                case 62374:                                 // Pursued (Ulduar, Flame Leviathan)
                case 62488:                                 // Activate Construct (Ulduar, Ignis)
                case 62577:                                 // Blizzard (Ulduar, Thorim)
                case 62603:                                 // Blizzard (h) (Ulduar, Thorim)
                case 62797:                                 // Storm Cloud (Ulduar, Hodir)
                case 62978:                                 // Summon Guardian (Ulduar, Yogg Saron)
                case 63018:                                 // Searing Light (Ulduar, XT-002)
                case 63024:                                 // Gravity Bomb (Ulduar, XT-002)
                case 63545:                                 // Icicle (Ulduar, Hodir)
                case 63744:                                 // Sara's Anger (Ulduar, Yogg-Saron)
                case 63745:                                 // Sara's Blessing (Ulduar, Yogg-Saron)
                case 63747:                                 // Sara's Fervor (Ulduar, Yogg-Saron)
                case 63795:                                 // Psychosis (Ulduar, Yogg-Saron)
                case 63820:                                 // Summon Scrap Bot Trigger (Ulduar, Mimiron) use for Scrap Bots, hits npc 33856
                case 63830:                                 // Malady of the Mind (Ulduar, Yogg-Saron)
                case 64218:                                 // Overcharge (VoA, Emalon)
                case 64234:                                 // Gravity Bomb (h) (Ulduar, XT-002)
                case 64402:                                 // Rocket Strike (Ulduar, Mimiron)
                case 64425:                                 // Summon Scrap Bot Trigger (Ulduar, Mimiron) use for Assault Bots, hits npc 33856
                case 64465:                                 // Shadow Beacon (Ulduar, Yogg-Saron)
                case 64543:                                 // Melt Ice (Ulduar, Hodir)
                case 64623:                                 // Frost Bomb (Ulduar, Mimiron)
                case 65121:                                 // Searing Light (h) (Ulduar, XT-002)
                case 65301:                                 // Psychosis (Ulduar, Yogg-Saron)
                case 65872:                                 // Pursuing Spikes (ToCrusader, Anub'arak)
                case 65950:                                 // Touch of Light (ToCrusader, Val'kyr Twins)
                case 66001:                                 // Touch of Darkness (ToCrusader, Val'kyr Twins)
                case 66152:                                 // Bullet Controller Summon Periodic Trigger Light (ToCrusader)
                case 66153:                                 // Bullet Controller Summon Periodic Trigger Dark (ToCrusader)
                case 66332:                                 // Nerubian Burrower (Mode 0) (ToCrusader, Anub'arak)
                case 66336:                                 // Mistress' Kiss (ToCrusader, Jaraxxus)
                case 66339:                                 // Summon Scarab (ToCrusader, Anub'arak)
                case 67077:                                 // Mistress' Kiss (Mode 2) (ToCrusader, Jaraxxus)
                case 67281:                                 // Touch of Darkness (Mode 1)
                case 67282:                                 // Touch of Darkness (Mode 2)
                case 67283:                                 // Touch of Darkness (Mode 3)
                case 67296:                                 // Touch of Light (Mode 1)
                case 67297:                                 // Touch of Light (Mode 2)
                case 67298:                                 // Touch of Light (Mode 3)
                case 68912:                                 // Wailing Souls (FoS)
                case 68950:                                 // Fear (FoS)
                case 68987:                                 // Pursuit (PoS)
                case 69048:                                 // Mirrored Soul (FoS)
                case 69057:                                 // Bone Spike Graveyard (Icecrown Citadel, Lord Marrowgar) 10 man
                case 72088:
                case 73142:
                case 73144:
                case 69140:                                 // Coldflame (ICC, Marrowgar)
                case 69674:                                 // Mutated Infection (ICC, Rotface)
                case 70450:                                 // Blood Mirror
                case 70837:                                 // Blood Mirror
                case 70882:                                 // Slime Spray Summon Trigger (ICC, Rotface)
                case 70920:                                 // Unbound Plague Search Effect (ICC, Putricide)
                case 71224:                                 // Mutated Infection (Mode 1)
                case 71445:                                 // Twilight Bloodbolt
                case 71471:                                 // Twilight Bloodbolt
                case 71837:                                 // Vampiric Bite
                case 71861:                                 // Swarming Shadows
                case 72091:                                 // Frozen Orb (Vault of Archavon, Toravon)
                case 72254:                                 // Mark of Fallen Champion (target selection) (ICC, Deathbringer Saurfang)
                case 73022:                                 // Mutated Infection (Mode 2)
                case 73023:                                 // Mutated Infection (Mode 3)
                    unMaxTargets = 1;
                    break;
                case 10258:                                 // Awaken Vault Warder (Uldaman)
                case 28542:                                 // Life Drain (Naxx, Sapphiron)
                case 62476:                                 // Icicle (Ulduar, Hodir)
                case 63802:                                 // Brain Link (Ulduar, Yogg-Saron)
                case 66013:                                 // Penetrating Cold (10 man) (ToCrusader, Anub'arak)
                case 67755:                                 // Nerubian Burrower (Mode 1) (ToCrusader, Anub'arak)
                case 67756:                                 // Nerubian Burrower (Mode 2) (ToCrusader, Anub'arak)
                case 68509:                                 // Penetrating Cold (10 man heroic)
                case 69055:                                 // Bone Slice (ICC, Lord Marrowgar)
                case 69278:                                 // Gas spore (ICC, Festergut)
                case 70341:                                 // Slime Puddle (ICC, Putricide)
                case 71336:                                 // Pact of the Darkfallen
                case 71390:                                 // Pact of the Darkfallen
                    unMaxTargets = 2;
                    break;
                case 28796:                                 // Poison Bolt Volley (Naxx, Faerlina)
                case 29213:                                 // Curse of the Plaguebringer (Naxx, Noth the Plaguebringer)
                case 30004:                                 // Flame Wreath (Karazhan, Shade of Aran)
                case 31298:                                 // Sleep (Hyjal Summit, Anetheron)
                case 39341:                                 // Karazhan - Chess, Medivh CHEAT: Fury of Medivh, Target Horde
                case 39344:                                 // Karazhan - Chess, Medivh CHEAT: Fury of Medivh, Target Alliance
                case 39992:                                 // Needle Spine Targeting (BT, Warlord Najentus)
                case 40869:                                 // Fatal Attraction (BT, Mother Shahraz)
                case 41303:                                 // Soul Drain (BT, Reliquary of Souls)
                case 41376:                                 // Spite (BT, Reliquary of Souls)
                case 51904:                                 // Summon Ghouls On Scarlet Crusade
                case 54522:                                 // Summon Ghouls On Scarlet Crusade
                case 60936:                                 // Surge of Power (h) (Malygos)
                case 61693:                                 // Arcane Storm (Malygos)
                case 62477:                                 // Icicle (h) (Ulduar, Hodir)
                case 63981:                                 // StoneGrip (h) (Ulduar, Kologarn)
                case 64598:                                 // Cosmic Smash (h) (Ulduar, Algalon)
                case 64620:                                 // Summon Fire Bot Trigger (Ulduar, Mimiron) hits npc 33856
                case 70814:                                 // Bone Slice (ICC, Lord Marrowgar, heroic)
                case 72095:                                 // Frozen Orb (h) (Vault of Archavon, Toravon)
                case 72089:                                 // Bone Spike Graveyard (Icecrown Citadel, Lord Marrowgar) 25 man
                case 70826:
                case 73143:
                case 73145:
                    unMaxTargets = 3;
                    break;
                case 37676:                                 // Insidious Whisper (SSC, Leotheras the Blind)
                case 38028:                                 // Watery Grave (SSC, Morogrim Tidewalker)
                case 46650:                                 // Open Brutallus Back Door (SWP, Felmyst)
                case 67757:                                 // Nerubian Burrower (Mode 3) (ToCrusader, Anub'arak)
                case 71221:                                 // Gas spore (Mode 1) (ICC, Festergut)
                    unMaxTargets = 4;
                    break;
                case 30843:                                 // Enfeeble (Karazhan, Prince Malchezaar)
                case 40243:                                 // Crushing Shadows (BT, Teron Gorefiend)
                case 42005:                                 // Bloodboil (BT, Gurtogg Bloodboil)
                case 45641:                                 // Fire Bloom (SWP, Kil'jaeden)
                case 55665:                                 // Life Drain (h) (Naxx, Sapphiron)
                case 58917:                                 // Consume Minions
                case 64604:                                 // Nature Bomb (Ulduar, Freya)
                case 67076:                                 // Mistress' Kiss (Mode 1) (ToCrusader, Jaraxxus)
                case 67078:                                 // Mistress' Kiss (Mode 3) (ToCrusader, Jaraxxus)
                case 67700:                                 // Penetrating Cold (25 man)
                case 68510:                                 // Penetrating Cold (25 man, heroic)
                    unMaxTargets = 5;
                    break;
                case 61694:                                 // Arcane Storm (h) (Malygos)
                    unMaxTargets = 7;
                    break;
                case 38054:                                 // Random Rocket Missile
                    unMaxTargets = 8;
                    break;
                case 54098:                                 // Poison Bolt Volley (h) (Naxx, Faerlina)
                case 54835:                                 // Curse of the Plaguebringer (h) (Naxx, Noth the Plaguebringer)
                    unMaxTargets = 10;
                    break;
                case 25991:                                 // Poison Bolt Volley (AQ40, Pincess Huhuran)
                    unMaxTargets = 15;
                    break;
                case 61916:                                 // Lightning Whirl (Ulduar, Stormcaller Brundir)
                    unMaxTargets = urand(2, 3);
                    break;
                case 46771:                                 // Flame Sear (SWP, Grand Warlock Alythess)
                    unMaxTargets = urand(3, 5);
                    break;
                case 63482:                                 // Lightning Whirl (h) (Ulduar, Stormcaller Brundir)
                    unMaxTargets = urand(3, 6);
                    break;
                case 74452:                                 // Conflagration (Saviana, Ruby Sanctum)
                {
                    if (m_caster)
                    {
                        switch (m_caster->GetMap()->GetDifficulty())
                        {
                            case RAID_DIFFICULTY_10MAN_NORMAL:
                            case RAID_DIFFICULTY_10MAN_HEROIC:
                                unMaxTargets = 2;
                                break;
                            case RAID_DIFFICULTY_25MAN_NORMAL:
                            case RAID_DIFFICULTY_25MAN_HEROIC:
                                unMaxTargets = 5;
                                break;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_MAGE:
        {
            if (m_spellInfo->Id == 38194)                   // Blink
            {
                unMaxTargets = 1;
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Sunder Armor (main spell)
            if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0000000000004000), 0x00000000) && m_spellInfo->SpellVisual[0] == 406)
            {
                if (m_caster->HasAura(58387))               // Glyph of Sunder Armor
                {
                    EffectChainTarget = 2;
                }
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Starfall
            if (m_spellInfo->IsFitToFamilyMask(UI64LIT(0x0000000000000000), 0x00000100))
            {
                unMaxTargets = 2;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            if (m_spellInfo->SpellIconID == 1737)           // Corpse Explosion // TODO - spell 50445?
            {
                unMaxTargets = 1;
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
            if (m_spellInfo->Id == 20424)                   // Seal of Command (2 more target for single targeted spell)
            {
                // overwrite EffectChainTarget for non single target spell
                if (Spell* currSpell = m_caster->GetCurrentSpell(CURRENT_GENERIC_SPELL))
               {
                    for(int i = 0; i < MAX_EFFECT_INDEX; ++i)
                    {
                        if (SpellEffectEntry const* spellEffect = currSpell->m_spellInfo->GetSpellEffect(SpellEffectIndex(i)))
                        {
                            if (spellEffect->EffectChainTarget > 0)
                            {
                                EffectChainTarget = 0;      // no chain targets
                            }
                        }
                    }
                    if (currSpell->m_spellInfo->GetMaxAffectedTargets() > 0)
                    {
                        EffectChainTarget = 0;              // no chain targets
                    }
                }
            }
            break;
        default:
            break;
    }

    // custom radius cases
    switch (m_spellInfo->GetSpellFamilyName())
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->Id)
            {
                case 24811:                                 // Draw Spirit (Lethon)
                {
                    if (spellEffect->EffectIndex == EFFECT_INDEX_0)         // Copy range from EFF_1 to 0
                    {
                        radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(spellEffect->GetRadiusIndex()));
                    }
                    break;
                }
                case 28241:                                 // Poison (Naxxramas, Grobbulus Cloud)
                case 54363:                                 // Poison (Naxxramas, Grobbulus Cloud) (H)
                {
                    uint32 auraId = (m_spellInfo->Id == 28241 ? 28158 : 54362);
                    if (SpellAuraHolder* auraHolder = m_caster->GetSpellAuraHolder(auraId))
                    {
                        radius = 0.5f * (60000 - auraHolder->GetAuraDuration()) * 0.001f;
                    }
                    break;
                }
                case 66881:                                 // Slime Pool (ToCrusader, Acidmaw & DreadScale)
                case 67638:                                 // Slime Pool (ToCrusader, Acidmaw & DreadScale) (Mode 1)
                case 67639:                                 // Slime Pool (ToCrusader, Acidmaw & DreadScale) (Mode 2)
                case 67640:                                 // Slime Pool (ToCrusader, Acidmaw & DreadScale) (Mode 3)
                    if (SpellAuraHolder* auraHolder = m_caster->GetSpellAuraHolder(66882))
                    {
                        radius = 0.5f * (60000 - auraHolder->GetAuraDuration()) * 0.001f;
                    }
                    break;
                case 56438:                                 // Arcane Overload
                    if (Unit* realCaster = GetAffectiveCaster())
                    {
                        radius = radius * realCaster->GetObjectScale();
                    }
                    break;
                case 69057:                                 // Bone Spike Graveyard (Icecrown Citadel, Lord Marrowgar encounter, 10N)
                case 70826:                                 // Bone Spike Graveyard (Icecrown Citadel, Lord Marrowgar encounter, 25N)
                case 72088:                                 // Bone Spike Graveyard (Icecrown Citadel, Lord Marrowgar encounter, 10H)
                case 72089:                                 // Bone Spike Graveyard (Icecrown Citadel, Lord Marrowgar encounter, 25H)
                case 73142:                                 // Bone Spike Graveyard (during Bone Storm) (Icecrown Citadel, Lord Marrowgar encounter, 10N)
                case 73143:                                 // Bone Spike Graveyard (during Bone Storm) (Icecrown Citadel, Lord Marrowgar encounter, 25N)
                case 73144:                                 // Bone Spike Graveyard (during Bone Storm) (Icecrown Citadel, Lord Marrowgar encounter, 10H)
                case 73145:                                 // Bone Spike Graveyard (during Bone Storm) (Icecrown Citadel, Lord Marrowgar encounter, 25H)
                case 72350:                                 // Fury of Frostmourne
                case 72351:                                 // Fury of Frostmourne
                case 72706:                                 // Achievement Check
                case 72830:                                 // Achievement Check
                    radius = DEFAULT_VISIBILITY_INSTANCE;
                    break;
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            switch (m_spellInfo->Id)
            {
                case 49376:                                 // Feral Charge - Cat
                    // No default radius for this spell, so we need to use the contact distance
                    radius = CONTACT_DISTANCE;
                    break;
            }
        }
        default:
            break;
    }
}
