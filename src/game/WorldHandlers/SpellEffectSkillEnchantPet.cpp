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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "SkillExtraItems.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "SpellAuras.h"
#include "Group.h"
#include "UpdateData.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "SharedDefines.h"
#include "Pet.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "Creature.h"
#include "Totem.h"
#include "CreatureAI.h"
#include "BattleGround/BattleGroundMgr.h"
#include "BattleGround/BattleGround.h"
#include "BattleGround/BattleGroundEY.h"
#include "BattleGround/BattleGroundWS.h"
#include "Language.h"
#include "SocialMgr.h"
#include "VMapFactory.h"
#include "Util.h"
#include "TemporarySummon.h"
#include "ScriptMgr.h"
#include "SkillDiscovery.h"
#include "Formulas.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Vehicle.h"
#include "G3D/Vector3.h"
#include "LootMgr.h"
#include <random>

#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

/**
 * @brief Teaches a spell to the target player or pet.
 *
 * @param effect The effect index containing the learned spell id.
 */
void Spell::EffectLearnSpell(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            EffectLearnPetSpell(effect);
        }

        return;
    }

    Player* player = (Player*)unitTarget;

    uint32 spellToLearn = ((m_spellInfo->Id==SPELL_ID_GENERIC_LEARN) || (m_spellInfo->Id==SPELL_ID_GENERIC_LEARN_PET)) ? damage : effect->EffectTriggerSpell;
    player->learnSpell(spellToLearn, false);

    if (WorldObject const* caster = GetCastingObject())
    {
        DEBUG_LOG("Spell: %s has learned spell %u from %s", player->GetGuidStr().c_str(), spellToLearn, caster->GetGuidStr().c_str());
    }
}

/**
 * @brief Dispels matching auras from the target and sends result logs.
 *
 * @param effect The dispel effect index.
 */
void Spell::EffectDispel(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    // Fill possible dispel list
    std::list <std::pair<SpellAuraHolder* , uint32> > dispel_list;

    // Create dispel mask by dispel type
    uint32 dispel_type = effect->EffectMiscValue;
    uint32 dispelMask  = GetDispellMask( DispelType(dispel_type) );
    Unit::SpellAuraHolderMap const& auras = unitTarget->GetSpellAuraHolderMap();
    for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        SpellAuraHolder *holder = itr->second;
        if ((1<<holder->GetSpellProto()->GetDispel()) & dispelMask)
        {
            if (holder->GetSpellProto()->GetDispel() == DISPEL_MAGIC)
            {
                bool positive = true;
                if (!holder->IsPositive())
                {
                    positive = false;
                }
                else
                {
                    positive = !holder->GetSpellProto()->HasAttribute(SPELL_ATTR_NEGATIVE);
                }

                // do not remove positive auras if friendly target
                //               negative auras if non-friendly target
                if (positive == unitTarget->IsFriendlyTo(m_caster))
                {
                    continue;
                }
            }
            // Unholy Blight prevents dispel of diseases from target
            else if (holder->GetSpellProto()->GetDispel() == DISPEL_DISEASE)
                if (unitTarget->HasAura(50536))
                {
                    continue;
                }

            dispel_list.push_back(std::pair<SpellAuraHolder* , uint32>(holder, holder->GetStackAmount()));
        }
    }
    // Ok if exist some buffs for dispel try dispel it
    if (!dispel_list.empty())
    {
        std::list<std::pair<SpellAuraHolder* , uint32> > success_list; // (spell_id,casterGuid)
        std::list < uint32 > fail_list;                     // spell_id

        // some spells have effect value = 0 and all from its by meaning expect 1
        if (!damage)
        {
            damage = 1;
        }

        // Dispel N = damage buffs (or while exist buffs for dispel)
        for (int32 count = 0; count < damage && !dispel_list.empty(); ++count)
        {
            // Random select buff for dispel
            std::list<std::pair<SpellAuraHolder* , uint32> >::iterator dispel_itr = dispel_list.begin();
            std::advance(dispel_itr, urand(0, dispel_list.size() - 1));

            SpellAuraHolder* holder = dispel_itr->first;

            dispel_itr->second -= 1;

            // remove entry from dispel_list if nothing left in stack
            if (dispel_itr->second == 0)
            {
                dispel_list.erase(dispel_itr);
            }

            SpellEntry const* spellInfo = holder->GetSpellProto();
            // Base dispel chance
            // TODO: possible chance depend from spell level??
            int32 miss_chance = 0;
            // Apply dispel mod from aura caster
            if (Unit* caster = holder->GetCaster())
            {
                if (Player* modOwner = caster->GetSpellModOwner())
                {
                    modOwner->ApplySpellMod(spellInfo->Id, SPELLMOD_RESIST_DISPEL_CHANCE, miss_chance, this);
                }
            }
            // Try dispel
            if (roll_chance_i(miss_chance))
            {
                fail_list.push_back(spellInfo->Id);
            }
            else
            {
                bool foundDispelled = false;
                for (std::list<std::pair<SpellAuraHolder* , uint32> >::iterator success_iter = success_list.begin(); success_iter != success_list.end(); ++success_iter)
                {
                    if (success_iter->first->GetId() == holder->GetId() && success_iter->first->GetCasterGuid() == holder->GetCasterGuid())
                    {
                        success_iter->second += 1;
                        foundDispelled = true;
                        break;
                    }
                }
                if (!foundDispelled)
                {
                    success_list.push_back(std::pair<SpellAuraHolder* , uint32>(holder, 1));
                }
            }
        }
        // Send success log and really remove auras
        if (!success_list.empty())
        {
            int32 count = success_list.size();
            WorldPacket data(SMSG_SPELLDISPELLOG, 8 + 8 + 4 + 1 + 4 + count * 5);
            data << unitTarget->GetPackGUID();              // Victim GUID
            data << m_caster->GetPackGUID();                // Caster GUID
            data << uint32(m_spellInfo->Id);                // Dispel spell id
            data << uint8(0);                               // not used
            data << uint32(count);                          // count
            for (std::list<std::pair<SpellAuraHolder* , uint32> >::iterator j = success_list.begin(); j != success_list.end(); ++j)
            {
                SpellAuraHolder* dispelledHolder = j->first;
                data << uint32(dispelledHolder->GetId());   // Spell Id
                data << uint8(0);                           // 0 - dispelled !=0 cleansed
                unitTarget->RemoveAuraHolderDueToSpellByDispel(dispelledHolder->GetId(), j->second, dispelledHolder->GetCasterGuid(), m_caster);
            }
            m_caster->SendMessageToSet(&data, true);

            // On success dispel
            // Devour Magic
            if (m_spellInfo->GetSpellFamilyName() == SPELLFAMILY_WARLOCK && m_spellInfo->GetCategory() == SPELLCATEGORY_DEVOUR_MAGIC)
            {
                int32 heal_amount = m_spellInfo->CalculateSimpleValue(EFFECT_INDEX_1);
                m_caster->CastCustomSpell(m_caster, 19658, &heal_amount, NULL, NULL, true);
            }
        }
        // Send fail log to client
        if (!fail_list.empty())
        {
            // Failed to dispel
            WorldPacket data(SMSG_DISPEL_FAILED, 8 + 8 + 4 + 4 * fail_list.size());
            data << m_caster->GetObjectGuid();              // Caster GUID
            data << unitTarget->GetObjectGuid();            // Victim GUID
            data << uint32(m_spellInfo->Id);                // Dispel spell id
            for (std::list< uint32 >::iterator j = fail_list.begin(); j != fail_list.end(); ++j)
            {
                data << uint32(*j);                         // Spell Id
            }
            m_caster->SendMessageToSet(&data, true);
        }
    }
}

/**
 * @brief Enables dual wielding for a player target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectDualWield(SpellEffectEntry const* /*effect*/)
{
    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)unitTarget)->SetCanDualWield(true);
    }
}

/**
 * @brief Placeholder for pull-style spell effects.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectPull(SpellEffectEntry const* /*effect*/)
{
    // TODO: create a proper pull towards distract spell center for distract
    DEBUG_LOG("WORLD: Spell Effect DUMMY");
}

/**
 * @brief Turns and distracts a non-combat unit target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectDistract(SpellEffectEntry const* /*effect*/)
{
    // Check for possible target
    if (!unitTarget || unitTarget->IsInCombat())
    {
        return;
    }

    // target must be OK to do this
    if (unitTarget->hasUnitState(UNIT_STAT_CAN_NOT_REACT))
    {
        return;
    }

    unitTarget->SetFacingTo(unitTarget->GetAngle(m_targets.m_destX, m_targets.m_destY));
    unitTarget->clearUnitState(UNIT_STAT_MOVING);

    if (unitTarget->GetTypeId() == TYPEID_UNIT)
    {
        unitTarget->GetMotionMaster()->MoveDistract(damage * IN_MILLISECONDS);
    }
}

/**
 * @brief Attempts to pickpocket a valid creature target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectPickPocket(SpellEffectEntry const* /*effect*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // victim must be creature and attackable
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT || m_caster->IsFriendlyTo(unitTarget))
    {
        return;
    }

    // victim have to be alive and humanoid or undead
    if (unitTarget->IsAlive() && (unitTarget->GetCreatureTypeMask() & CREATURE_TYPEMASK_HUMANOID_OR_UNDEAD) != 0)
    {
        int32 chance = 10 + int32(m_caster->getLevel()) - int32(unitTarget->getLevel());

        if (chance > irand(0, 19))
        {
            // Stealing successful
            // DEBUG_LOG("Sending loot from pickpocket");
            ((Player*)m_caster)->SendLoot(unitTarget->GetObjectGuid(), LOOT_PICKPOCKETING);
        }
        else
        {
            // Reveal action + get attack
            m_caster->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
            unitTarget->AttackedBy(m_caster);
        }
    }
}

/**
 * @brief Creates a farsight focus object and switches the player's camera to it.
 *
 * @param effect The farsight effect index.
 */
void Spell::EffectAddFarsight(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    int32 duration = GetSpellDuration(m_spellInfo);
    DynamicObject* dynObj = new DynamicObject;

    // set radius to 0: spell not expected to work as persistent aura
    if (!dynObj->Create(m_caster->GetMap()->GenerateLocalLowGuid(HIGHGUID_DYNAMICOBJECT), m_caster, m_spellInfo->Id, SpellEffectIndex(effect->EffectIndex), m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, duration, 0, DYNAMIC_OBJECT_FARSIGHT_FOCUS))
    {
        delete dynObj;
        return;
    }

    m_caster->AddDynObject(dynObj);
    m_caster->GetMap()->Add(dynObj);

    ((Player*)m_caster)->GetCamera().SetView(dynObj);
}

/**
 * @brief Teleports the target near the caster and turns it to face away from the caster.
 *
 * @param effect The teleport effect index.
 */
void Spell::EffectTeleUnitsFaceCaster(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    if (unitTarget->IsTaxiFlying())
    {
        return;
    }

    float fx, fy, fz;
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(fx, fy, fz);
    }
    else
    {
        float dis = GetSpellRadius(sSpellRadiusStore.LookupEntry(effect->GetRadiusIndex()));
        m_caster->GetClosePoint(fx, fy, fz, unitTarget->GetObjectBoundingRadius(), dis);
    }

    unitTarget->NearTeleportTo(fx, fy, fz, -m_caster->GetOrientation(), unitTarget == m_caster);
}

/**
 * @brief Teaches or updates a skill for a player target.
 *
 * @param effect The effect index containing the skill identifier.
 */
void Spell::EffectLearnSkill(SpellEffectEntry const* effect)
{
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (damage < 0)
    {
        return;
    }

    uint32 skillid =  effect->EffectMiscValue;
    uint16 skillval = ((Player*)unitTarget)->GetPureSkillValue(skillid);
    ((Player*)unitTarget)->SetSkill(skillid, skillval ? skillval : 1, damage * 75, damage);

    if (WorldObject const* caster = GetCastingObject())
    {
        DEBUG_LOG("Spell: %s has learned skill %u (to maxlevel %u) from %s", unitTarget->GetGuidStr().c_str(), skillid, damage * 75, caster->GetGuidStr().c_str());
    }
}

void Spell::EffectTradeSkill(SpellEffectEntry const* /*effect*/)
{
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    // uint32 skillid =  m_spellInfo->EffectMiscValue[i];
    // uint16 skillmax = ((Player*)unitTarget)->(skillid);
    // ((Player*)unitTarget)->SetSkill(skillid,skillval?skillval:1,skillmax+75);
}

/**
 * @brief Applies a permanent enchantment to the target item.
 *
 * @param effect The effect index providing the enchantment id.
 */
void Spell::EffectEnchantItemPerm(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    if (!itemTarget)
    {
        return;
    }

    uint32 enchant_id = effect->EffectMiscValue;
    if (!enchant_id)
    {
        return;
    }

    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if (!pEnchant)
    {
        return;
    }

    // item can be in trade slot and have owner diff. from caster
    Player* item_owner = itemTarget->GetOwner();
    if (!item_owner)
    {
        return;
    }

    Player* p_caster = (Player*)m_caster;

    // Enchanting a vellum requires special handling, as it creates a new item
    // instead of modifying an existing one.
    ItemPrototype const* targetProto = itemTarget->GetProto();
    if (targetProto->IsVellum() && effect->EffectItemType)
    {
        unitTarget = m_caster;
        DoCreateItem(effect, effect->EffectItemType);
        // Vellum target case: Target becomes additional reagent, new scroll item created instead in Spell::EffectEnchantItemPerm()
        // cannot already delete in TakeReagents() unfortunately
        p_caster->DestroyItemCount(targetProto->ItemId, 1, true);
        return;
    }

    // Using enchant stored on scroll does not increase enchanting skill! (Already granted on scroll creation)
    if (!(m_CastItem && m_CastItem->GetProto()->Flags & ITEM_FLAG_ENCHANT_SCROLL))
    {
        p_caster->UpdateCraftSkill(m_spellInfo->Id);
    }

    if (item_owner != p_caster && p_caster->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
    {
        sLog.outCommand(p_caster->GetSession()->GetAccountId(), "GM %s (Account: %u) enchanting(perm): %s (Entry: %d) for player: %s (Account: %u)",
                        p_caster->GetName(), p_caster->GetSession()->GetAccountId(),
                        itemTarget->GetProto()->Name1, itemTarget->GetEntry(),
                        item_owner->GetName(), item_owner->GetSession()->GetAccountId());
    }

    // remove old enchanting before applying new if equipped
    item_owner->ApplyEnchantment(itemTarget, PERM_ENCHANTMENT_SLOT, false);

    itemTarget->SetEnchantment(PERM_ENCHANTMENT_SLOT, enchant_id, 0, 0, m_caster->GetObjectGuid());

    // add new enchanting if equipped
    item_owner->ApplyEnchantment(itemTarget, PERM_ENCHANTMENT_SLOT, true);
}

/**
 * @brief Applies a temporary enchantment to the target item.
 *
 * @param effect The effect index providing the enchantment id.
 */
void Spell::EffectEnchantItemPrismatic(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    if (!itemTarget)
    {
        return;
    }

    Player* p_caster = (Player*)m_caster;

    uint32 enchant_id = effect->EffectMiscValue;
    if (!enchant_id)
    {
        return;
    }

    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if (!pEnchant)
    {
        return;
    }

    // support only enchantings with add socket in this slot
    {
        bool add_socket = false;
        for (int i = 0; i < 3; ++i)
        {
            if (pEnchant->type[i] == ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET)
            {
                add_socket = true;
                break;
            }
        }
        if (!add_socket)
        {
            sLog.outError("Spell::EffectEnchantItemPrismatic: attempt apply enchant spell %u with SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC (%u) but without ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET (%u), not suppoted yet.",
                          m_spellInfo->Id, SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC, ITEM_ENCHANTMENT_TYPE_PRISMATIC_SOCKET);
            return;
        }
    }

    // item can be in trade slot and have owner diff. from caster
    Player* item_owner = itemTarget->GetOwner();
    if (!item_owner)
    {
        return;
    }

    if (item_owner != p_caster && p_caster->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
    {
        sLog.outCommand(p_caster->GetSession()->GetAccountId(), "GM %s (Account: %u) enchanting(perm): %s (Entry: %d) for player: %s (Account: %u)",
                        p_caster->GetName(), p_caster->GetSession()->GetAccountId(),
                        itemTarget->GetProto()->Name1, itemTarget->GetEntry(),
                        item_owner->GetName(), item_owner->GetSession()->GetAccountId());
    }

    // remove old enchanting before applying new if equipped
    item_owner->ApplyEnchantment(itemTarget, PRISMATIC_ENCHANTMENT_SLOT, false);

    itemTarget->SetEnchantment(PRISMATIC_ENCHANTMENT_SLOT, enchant_id, 0, 0, m_caster->GetObjectGuid());

    // add new enchanting if equipped
    item_owner->ApplyEnchantment(itemTarget, PRISMATIC_ENCHANTMENT_SLOT, true);
}

void Spell::EffectEnchantItemTmp(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* p_caster = (Player*)m_caster;

    // Rockbiter Weapon
    SpellClassOptionsEntry const* classOptions = m_spellInfo->GetSpellClassOptions();
    if (classOptions && classOptions->SpellFamilyName == SPELLFAMILY_SHAMAN && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000400000))
    {
        uint32 spell_id = 0;

        // enchanting spell selected by calculated damage-per-sec stored in Effect[1] base value
        // Note: damage calculated (correctly) with rounding int32(float(v)) but
        // RW enchantments applied damage int32(float(v)+0.5), this create  0..1 difference sometime
        switch (damage)
        {
                // Rank 1
            case  2: spell_id = 36744; break;               //  0% [ 7% ==  2, 14% == 2, 20% == 2]
                // Rank 2
            case  4: spell_id = 36753; break;               //  0% [ 7% ==  4, 14% == 4]
            case  5: spell_id = 36751; break;               // 20%
                // Rank 3
            case  6: spell_id = 36754; break;               //  0% [ 7% ==  6, 14% == 6]
            case  7: spell_id = 36755; break;               // 20%
                // Rank 4
            case  9: spell_id = 36761; break;               //  0% [ 7% ==  6]
            case 10: spell_id = 36758; break;               // 14%
            case 11: spell_id = 36760; break;               // 20%
            default:
                sLog.outError("Spell::EffectEnchantItemTmp: Damage %u not handled in S'RW", damage);
                return;
        }

        SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);
        if (!spellInfo)
        {
            sLog.outError("Spell::EffectEnchantItemTmp: unknown spell id %i", spell_id);
            return;
        }

        Spell* spell = new Spell(m_caster, spellInfo, true);
        SpellCastTargets targets;
        targets.setItemTarget(itemTarget);
        spell->SpellStart(&targets);
        return;
    }

    if (!itemTarget)
    {
        return;
    }

    uint32 enchant_id = effect->EffectMiscValue;

    if (!enchant_id)
    {
        sLog.outError("Spell %u Effect %u (SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) have 0 as enchanting id",m_spellInfo->Id,effect->EffectIndex);
        return;
    }

    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
    if (!pEnchant)
    {
        sLog.outError("Spell %u Effect %u (SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) have nonexistent enchanting id %u ",m_spellInfo->Id,effect->EffectIndex,enchant_id);
        return;
    }

    // select enchantment duration
    uint32 duration;

    // rogue family enchantments exception by duration
    if (m_spellInfo->Id == 38615)
    {
        duration = 1800;                                    // 30 mins
    }
    // other rogue family enchantments always 1 hour (some have spell damage=0, but some have wrong data in EffBasePoints)
    else if (classOptions && classOptions->SpellFamilyName == SPELLFAMILY_ROGUE)
    {
        duration = 3600;                                    // 1 hour
    }
    // shaman family enchantments
    else if (classOptions && classOptions->SpellFamilyName == SPELLFAMILY_SHAMAN)
    {
        duration = 1800;                                    // 30 mins
    }
    // other cases with this SpellVisual already selected
    else if (m_spellInfo->SpellVisual[0] == 215)
    {
        duration = 1800;                                    // 30 mins
    }
    // some fishing pole bonuses
    else if (m_spellInfo->SpellVisual[0] == 563)
    {
        duration = 600;                                     // 10 mins
    }
    // shaman rockbiter enchantments
    else if (m_spellInfo->SpellVisual[0] == 0)
    {
        duration = 1800;                                    // 30 mins
    }
    else if (m_spellInfo->Id == 29702)
    {
        duration = 300;                                     // 5 mins
    }
    else if (m_spellInfo->Id == 37360)
    {
        duration = 300;                                     // 5 mins
    }
    // default case
    else
    {
        duration = 3600;                                    // 1 hour
    }

    // item can be in trade slot and have owner diff. from caster
    Player* item_owner = itemTarget->GetOwner();
    if (!item_owner)
    {
        return;
    }

    if (item_owner != p_caster && p_caster->GetSession()->GetSecurity() > SEC_PLAYER && sWorld.getConfig(CONFIG_BOOL_GM_LOG_TRADE))
    {
        sLog.outCommand(p_caster->GetSession()->GetAccountId(), "GM %s (Account: %u) enchanting(temp): %s (Entry: %d) for player: %s (Account: %u)",
                        p_caster->GetName(), p_caster->GetSession()->GetAccountId(),
                        itemTarget->GetProto()->Name1, itemTarget->GetEntry(),
                        item_owner->GetName(), item_owner->GetSession()->GetAccountId());
    }

    // remove old enchanting before applying new if equipped
    item_owner->ApplyEnchantment(itemTarget, TEMP_ENCHANTMENT_SLOT, false);

    itemTarget->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, duration * 1000, 0, m_caster->GetObjectGuid());

    // add new enchanting if equipped
    item_owner->ApplyEnchantment(itemTarget, TEMP_ENCHANTMENT_SLOT, true);
}

/**
 * @brief Converts the creature target into a hunter pet for the caster.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectTameCreature(SpellEffectEntry const* /*effect*/)
{
    // Caster must be player, checked in Spell::CheckCast
    // Spell can be triggered, we need to check original caster prior to caster
    Player* plr = (Player*)GetAffectiveCaster();

    Creature* creatureTarget = (Creature*)unitTarget;

    // cast finish successfully
    // SendChannelUpdate(0);
    finish();

    Pet* pet = new Pet(HUNTER_PET);

    if (!pet->CreateBaseAtCreature(creatureTarget))
    {
        delete pet;
        return;
    }

    pet->SetOwnerGuid(plr->GetObjectGuid());
    pet->SetCreatorGuid(plr->GetObjectGuid());
    pet->setFaction(plr->getFaction());
    pet->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

    if (plr->IsPvP())
    {
        pet->SetPvP(true);
    }

    if (plr->IsFFAPvP())
    {
        pet->SetFFAPvP(true);
    }

    pet->GetCharmInfo()->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);

    pet->GetCharmInfo()->SetReactState(REACT_DEFENSIVE);

    // level of hunter pet can't be less owner level at 5 levels
    uint32 cLevel = creatureTarget->getLevel();
    uint32 plLevel = plr->getLevel();
    uint32 level = (cLevel + 5) < plLevel ? (plLevel - 5) : cLevel;
    pet->InitStatsForLevel(level);
    pet->InitLevelupSpellsForLevel();
    pet->InitTalentForLevel();

    pet->SetHealthPercent(creatureTarget->GetHealthPercent());

    pet->GetCharmInfo()->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);

    // destroy creature object
    creatureTarget->ForcedDespawn();

    // prepare visual effect for levelup
    pet->SetUInt32Value(UNIT_FIELD_LEVEL, level - 1);

    // add pet object to the world
    pet->GetMap()->Add((Creature*)pet);
    pet->AIM_Initialize();

    // visual effect for levelup
    pet->SetUInt32Value(UNIT_FIELD_LEVEL, level);

    // this enables pet details window (Shift+P)
    pet->InitPetCreateSpells();

    // caster have pet now
    plr->SetPet(pet);

    plr->PetSpellInitialize();

    // Cata Call Pet 1..5 storage: allocate the next free active slot
    // instead of always overwriting slot 0. SavePetToDB(PET_SAVE_NEW_PET)
    // resolves into 0..PET_SLOT_LAST_ACTIVE_SLOT via the free-slot scan
    // added in step 6 of MANGOS/PET_SAVE_CALLSITE_AUDIT.md, and leaves
    // GetSlot() above PET_SLOT_LAST_ACTIVE_SLOT when the roster is full.
    // Roll the tame back when that happens: keep the player from owning
    // a phantom pet that has no character_pet row, and notify the client
    // with PETTAME_TOOMANY so the UI clears the cast.
    //
    // Audit row T1. This is the keystone commit that makes Call Pet 1..5
    // routing meaningful end-to-end: with this in place each tame writes
    // to its own slot, step 9's CheckCast slot-decoding retrieves the
    // right pet, step 5's auto-promote gate keeps it parked during temp
    // unsummon, and step 8's intercept preserves the slot across
    // dismiss / re-save / logout.
    pet->SavePetToDB(PET_SAVE_NEW_PET);
    if (pet->GetSlot() > PET_SLOT_LAST_ACTIVE_SLOT)
    {
        plr->SendPetTameFailure(PETTAME_TOOMANY);
        pet->Unsummon(PET_SAVE_AS_DELETED, plr);
        return;
    }
}

/**
 * @brief Summons, recalls, or replaces the caster's pet.
 *
 * @param effect The summon effect index.
 */
void Spell::EffectSummonPet(SpellEffectEntry const* effect)
{
    uint32 petentry = effect->EffectMiscValue;

    Pet* NewSummon = new Pet;

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        switch(m_caster->getClass())
        {
            case CLASS_HUNTER:
            {
                // Everything already taken care of, we are only here because we loaded pet from db successfully
                delete NewSummon;
                return;
            }
            default:
            {
                if (Pet* OldSummon = m_caster->GetPet())
                {
                    OldSummon->Unsummon(PET_SAVE_NOT_IN_SLOT, m_caster);
                }

                // Load pet from db; if any to load
                if (NewSummon->LoadPetFromDB((Player*)m_caster, petentry))
                {
                    NewSummon->SetHealth(NewSummon->GetMaxHealth());
                    NewSummon->SetPower(POWER_MANA, NewSummon->GetMaxPower(POWER_MANA));

                    NewSummon->SavePetToDB(PET_SAVE_AS_CURRENT);

                    return;
                }

                NewSummon->setPetType(SUMMON_PET);
            }
        }
    }
    else
    {
        NewSummon->setPetType(GUARDIAN_PET);
    }

    CreatureInfo const* cInfo = petentry ? ObjectMgr::GetCreatureTemplate(petentry) : NULL;
    if (!cInfo)
    {
        sLog.outErrorDb("EffectSummonPet: creature entry %u not found for spell %u.", petentry, m_spellInfo->Id);
        delete NewSummon;
        return;
    }

    CreatureCreatePos pos(m_caster, m_caster->GetOrientation());

    Map* map = m_caster->GetMap();
    uint32 pet_number = sObjectMgr.GeneratePetNumber();
    if (!NewSummon->Create(map->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
    {
        delete NewSummon;
        return;
    }

    NewSummon->SetRespawnCoord(pos);

    // Level of pet summoned
    uint32 level = std::max(m_caster->getLevel() + effect->EffectMultipleValue, 1.0f);

    NewSummon->GetCharmInfo()->SetReactState(REACT_DEFENSIVE);
    NewSummon->SetOwnerGuid(m_caster->GetObjectGuid());
    NewSummon->SetCreatorGuid(m_caster->GetObjectGuid());
    NewSummon->setFaction(m_caster->getFaction());
    NewSummon->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
    NewSummon->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

    NewSummon->InitStatsForLevel(level);
    NewSummon->InitPetCreateSpells();
    NewSummon->InitLevelupSpellsForLevel();
    NewSummon->InitTalentForLevel();

    map->Add((Creature*)NewSummon);
    NewSummon->AIM_Initialize();

    m_caster->SetPet(NewSummon);
    DEBUG_LOG("New Pet has guid %u", NewSummon->GetGUIDLow());

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        NewSummon->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

        NewSummon->SetByteValue(UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_SUPPORTABLE | UNIT_BYTE2_FLAG_AURAS);
        NewSummon->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);

        NewSummon->GetCharmInfo()->SetPetNumber(pet_number, true);

        // generate new name for summon pet
        NewSummon->SetName(sObjectMgr.GeneratePetName(petentry));

        if (m_caster->IsPvP())
        {
            NewSummon->SetPvP(true);
        }

        if (m_caster->IsFFAPvP())
        {
            NewSummon->SetFFAPvP(true);
        }

        NewSummon->SavePetToDB(PET_SAVE_AS_CURRENT);
        ((Player*)m_caster)->PetSpellInitialize();
    }
    else
    {
        // Notify Summoner
        if (m_originalCaster && (m_originalCaster != m_caster)
            && (m_originalCaster->GetTypeId() == TYPEID_UNIT) && ((Creature*)m_originalCaster)->AI())
        {
            ((Creature*)m_originalCaster)->AI()->JustSummoned(NewSummon);
            if (m_originalCaster->IsInCombat() && !(NewSummon->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE)))
            {
                ((Creature*)NewSummon)->AI()->AttackStart(m_originalCaster->getAttackerForHelper());
            }
        }
        else if ((m_caster->GetTypeId() == TYPEID_UNIT) && ((Creature*)m_caster)->AI())
        {
            ((Creature*)m_caster)->AI()->JustSummoned(NewSummon);
            if (m_caster->IsInCombat() && !(NewSummon->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE)))
            {
                ((Creature*)NewSummon)->AI()->AttackStart(m_caster->getAttackerForHelper());
            }
        }
    }
}

/**
 * @brief Teaches a new spell to the caster's pet.
 *
 * @param effect The effect index containing the learned spell id.
 */
void Spell::EffectLearnPetSpell(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* _player = (Player*)m_caster;

    Pet* pet = _player->GetPet();
    if (!pet)
    {
        return;
    }
    if (!pet->IsAlive())
    {
        return;
    }

    SpellEntry const *learn_spellproto = sSpellStore.LookupEntry(effect->EffectTriggerSpell);
    if (!learn_spellproto)
    {
        return;
    }

    pet->learnSpell(learn_spellproto->Id);

    pet->SavePetToDB(PET_SAVE_AS_CURRENT);
    _player->PetSpellInitialize();

    if (WorldObject const* caster = GetCastingObject())
    {
        DEBUG_LOG("Spell: %s has learned spell %u from %s", pet->GetGuidStr().c_str(), learn_spellproto->Id, caster->GetGuidStr().c_str());
    }
}

/**
 * @brief Prepares taunt threat adjustments before the taunt aura is applied.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectTaunt(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget)
    {
        return;
    }

    // this effect use before aura Taunt apply for prevent taunt already attacking target
    // for spell as marked "non effective at already attacking target"
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        if (unitTarget->getVictim() == m_caster)
        {
            SendCastResult(SPELL_FAILED_DONT_REPORT);
            return;
        }
    }

    // Also use this effect to set the taunter's threat to the taunted creature's highest value
    if (unitTarget->CanHaveThreatList() && unitTarget->GetThreatManager().getCurrentVictim())
    {
        unitTarget->GetThreatManager().addThreat(m_caster, unitTarget->GetThreatManager().getCurrentVictim()->getThreat());
    }
}

/**
 * @brief Computes weapon-based spell damage and accumulates it for application.
 *
 * @param effect The weapon damage effect index.
 */
void Spell::EffectWeaponDmg(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    // multiple weapon dmg effect workaround
    // execute only the last weapon damage
    // and handle all effects at once
    for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
    {
        switch(m_spellInfo->GetSpellEffectIdByIndex(SpellEffectIndex(j)))
        {
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                if (j < int(effect->EffectIndex))           // we must calculate only at last weapon effect
                {
                    return;
                }
                break;
        }
    }

    // some spell specific modifiers
    bool spellBonusNeedWeaponDamagePercentMod = false;      // if set applied weapon damage percent mode to spell bonus

    float weaponDamagePercentMod = 1.0f;                    // applied to weapon damage and to fixed effect damage bonus
    float totalDamagePercentMod  = 1.0f;                    // applied to final bonus+weapon damage
    bool normalized = false;

    int32 spell_bonus = 0;                                  // bonus specific for spell

    SpellClassOptionsEntry const* classOptions = m_spellInfo->GetSpellClassOptions();

    switch(m_spellInfo->GetSpellFamilyName())
    {
        case SPELLFAMILY_GENERIC:
        {
            switch (m_spellInfo->Id)
            {
                    // for spells with divided damage to targets
                case 66765: case 66809: case 67331:         // Meteor Fists
                case 67333:                                 // Meteor Fists
                case 69055:                                 // Bone Slice
                case 71021:                                 // Saber Lash
                {
                    uint32 count = 0;
                    for(TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                        if (ihit->effectMask & (1<<effect->EffectIndex))
                        {
                            ++count;
                        }

                    totalDamagePercentMod /= float(count);  // divide to all targets
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_WARRIOR:
        {
            // Devastate
            if (m_spellInfo->SpellVisual[0] == 12295 && m_spellInfo->SpellIconID == 1508)
            {
                // Sunder Armor
                Aura* sunder = unitTarget->GetAura(SPELL_AURA_MOD_RESISTANCE_PCT, SPELLFAMILY_WARRIOR, UI64LIT(0x0000000000004000), 0x00000000, m_caster->GetObjectGuid());

                // Devastate bonus and sunder armor refresh
                if (sunder)
                {
                    sunder->GetHolder()->RefreshHolder();
                    spell_bonus += sunder->GetStackAmount() * CalculateDamage(EFFECT_INDEX_2, unitTarget);
                }

                // Devastate causing Sunder Armor Effect
                // and no need to cast over max stack amount
                if (!sunder || sunder->GetStackAmount() < sunder->GetSpellProto()->GetStackAmount())
                {
                    m_caster->CastSpell(unitTarget, 58567, true);
                }
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            // Mutilate (for each hand)
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x600000000))
            {
                bool found = false;
                // fast check
                if (unitTarget->HasAuraState(AURA_STATE_DEADLY_POISON))
                {
                    found = true;
                }
                // full aura scan
                else
                {
                    Unit::SpellAuraHolderMap const& auras = unitTarget->GetSpellAuraHolderMap();
                    for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    {
                        if (itr->second->GetSpellProto()->GetDispel() == DISPEL_POISON)
                        {
                            found = true;
                            break;
                        }
                    }
                }

                if (found)
                {
                    totalDamagePercentMod *= 1.2f;          // 120% if poisoned
                }
            }
            // Fan of Knives
            else if (m_caster->GetTypeId()==TYPEID_PLAYER && classOptions && (classOptions->SpellFamilyFlags & UI64LIT(0x0004000000000000)))
            {
                Item* weapon = ((Player*)m_caster)->GetWeaponForAttack(m_attackType, true, true);
                if (weapon && weapon->GetProto()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER)
                {
                    totalDamagePercentMod *= 1.5f;          // 150% to daggers
                }
            }
            // Ghostly Strike
            else if (m_caster->GetTypeId() == TYPEID_PLAYER && m_spellInfo->Id == 14278)
            {
                Item* weapon = ((Player*)m_caster)->GetWeaponForAttack(m_attackType, true, true);
                if (weapon && weapon->GetProto()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER)
                {
                    totalDamagePercentMod *= 1.44f;         // 144% to daggers
                }
            }
            // Hemorrhage
            else if (m_caster->GetTypeId() == TYPEID_PLAYER && classOptions && (classOptions->SpellFamilyFlags & UI64LIT(0x2000000)))
            {
                Item* weapon = ((Player*)m_caster)->GetWeaponForAttack(m_attackType, true, true);
                if (weapon && weapon->GetProto()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER)
                {
                    totalDamagePercentMod *= 1.45f;         // 145% to daggers
                }
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            // Judgement of Command - receive benefit from Spell Damage and Attack Power
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00020000000000))
            {
                float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                int32 holy = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                if (holy < 0)
                {
                    holy = 0;
                }
                spell_bonus += int32(ap * 0.08f) + int32(holy * 13 / 100);
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // Kill Shot
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x80000000000000))
            {
                // 0.4*RAP added to damage (that is 0.2 if we apply PercentMod (200%) to spell_bonus, too)
                spellBonusNeedWeaponDamagePercentMod = true;
                spell_bonus += int32(0.2f * m_caster->GetTotalAttackPowerValue(RANGED_ATTACK));
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Skyshatter Harness item set bonus
            // Stormstrike
            if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x001000000000))
            {
                Unit::AuraList const& m_OverrideClassScript = m_caster->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for (Unit::AuraList::const_iterator citr = m_OverrideClassScript.begin(); citr != m_OverrideClassScript.end(); ++citr)
                {
                    // Stormstrike AP Buff
                    if ((*citr)->GetModifier()->m_miscvalue == 5634)
                    {
                        m_caster->CastSpell(m_caster, 38430, true, NULL, *citr);
                        break;
                    }
                }
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            // Blood Strike, Heart Strike, Obliterate
            // Blood-Caked Strike
            if (m_spellInfo->SpellIconID == 1736)
            {
                uint32 count = 0;
                Unit::SpellAuraHolderMap const& auras = unitTarget->GetSpellAuraHolderMap();
                for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                {
                    if (itr->second->GetSpellProto()->GetDispel() == DISPEL_DISEASE &&
                        itr->second->GetCasterGuid() == m_caster->GetObjectGuid())
                        ++count;
                }

                if (count)
                {
                    // Effect 1(for Blood-Caked Strike)/3(other) damage is bonus
                    float bonus = count * CalculateDamage(m_spellInfo->SpellIconID == 1736 ? EFFECT_INDEX_0 : EFFECT_INDEX_2, unitTarget) / 100.0f;
                    // Blood Strike, Blood-Caked Strike and Obliterate store bonus*2
                    if ((classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0002000000400000)) ||
                        m_spellInfo->SpellIconID == 1736)
                        bonus /= 2.0f;

                    totalDamagePercentMod *= 1.0f + bonus;
                }

                // Heart Strike secondary target
                if (m_spellInfo->SpellIconID == 3145)
                    if (m_targets.getUnitTarget() != unitTarget)
                    {
                        weaponDamagePercentMod /= 2.0f;
                    }
            }
            // Glyph of Blood Strike
            if ( classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000400000) &&
                m_caster->HasAura(59332) &&
                unitTarget->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED))
            {
                totalDamagePercentMod *= 1.2f;              // 120% if snared
            }
            // Glyph of Death Strike
            if ( classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000000010) &&
                m_caster->HasAura(59336))
            {
                int32 rp = m_caster->GetPower(POWER_RUNIC_POWER) / 10;
                if (rp > 25)
                {
                    rp = 25;
                }
                totalDamagePercentMod *= 1.0f + rp / 100.0f;
            }
            // Glyph of Plague Strike
            if ( classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000000001) &&
                m_caster->HasAura(58657) )
            {
                totalDamagePercentMod *= 1.2f;
            }
            break;
        }
    }

    int32 fixed_bonus = 0;
    for (int j = 0; j < MAX_EFFECT_INDEX; ++j)
    {
        SpellEffectEntry const* spellEffect = m_spellInfo->GetSpellEffect(SpellEffectIndex(j));
        if (!spellEffect)
        {
            continue;
        }

        switch(spellEffect->Effect)
        {
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
                fixed_bonus += CalculateDamage(SpellEffectIndex(j), unitTarget);
                break;
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                fixed_bonus += CalculateDamage(SpellEffectIndex(j), unitTarget);
                normalized = true;
                break;
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                weaponDamagePercentMod *= float(CalculateDamage(SpellEffectIndex(j), unitTarget)) / 100.0f;

                // applied only to prev.effects fixed damage
                fixed_bonus = int32(fixed_bonus * weaponDamagePercentMod);
                break;
            default:
                break;                                      // not weapon damage effect, just skip
        }
    }

    // apply weaponDamagePercentMod to spell bonus also
    if (spellBonusNeedWeaponDamagePercentMod)
    {
        spell_bonus = int32(spell_bonus * weaponDamagePercentMod);
    }

    // non-weapon damage
    int32 bonus = spell_bonus + fixed_bonus;

    // apply to non-weapon bonus weapon total pct effect, weapon total flat effect included in weapon damage
    if (bonus)
    {
        UnitMods unitMod;
        switch (m_attackType)
        {
            default:
            case BASE_ATTACK:   unitMod = UNIT_MOD_DAMAGE_MAINHAND; break;
            case OFF_ATTACK:    unitMod = UNIT_MOD_DAMAGE_OFFHAND;  break;
            case RANGED_ATTACK: unitMod = UNIT_MOD_DAMAGE_RANGED;   break;
        }

        float weapon_total_pct  = m_caster->GetModifierValue(unitMod, TOTAL_PCT);
        bonus = int32(bonus * weapon_total_pct);
    }

    // + weapon damage with applied weapon% dmg to base weapon damage in call
    bonus += int32(m_caster->CalculateDamage(m_attackType, normalized) * weaponDamagePercentMod);

    // total damage
    bonus = int32(bonus * totalDamagePercentMod);

    // prevent negative damage
    m_damage += uint32(bonus > 0 ? bonus : 0);

    // Hemorrhage
    if (m_spellInfo->IsFitToFamily(SPELLFAMILY_ROGUE, UI64LIT(0x0000000002000000)))
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)m_caster)->AddComboPoints(unitTarget, 1);
        }
    }
    // Mangle (Cat): CP
    else if (m_spellInfo->IsFitToFamily(SPELLFAMILY_DRUID, UI64LIT(0x0000040000000000)))
    {
        if (m_caster->GetTypeId() == TYPEID_PLAYER)
        {
            ((Player*)m_caster)->AddComboPoints(unitTarget, 1);
        }
    }
}

