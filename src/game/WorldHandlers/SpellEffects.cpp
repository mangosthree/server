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

pEffect SpellEffects[TOTAL_SPELL_EFFECTS] =
{
    &Spell::EffectNULL,                                     //  0
    &Spell::EffectInstaKill,                                //  1 SPELL_EFFECT_INSTAKILL
    &Spell::EffectSchoolDMG,                                //  2 SPELL_EFFECT_SCHOOL_DAMAGE
    &Spell::EffectDummy,                                    //  3 SPELL_EFFECT_DUMMY
    &Spell::EffectUnused,                                   //  4 SPELL_EFFECT_PORTAL_TELEPORT          unused from pre-1.2.1
    &Spell::EffectTeleportUnits,                            //  5 SPELL_EFFECT_TELEPORT_UNITS
    &Spell::EffectApplyAura,                                //  6 SPELL_EFFECT_APPLY_AURA
    &Spell::EffectEnvironmentalDMG,                         //  7 SPELL_EFFECT_ENVIRONMENTAL_DAMAGE
    &Spell::EffectPowerDrain,                               //  8 SPELL_EFFECT_POWER_DRAIN
    &Spell::EffectHealthLeech,                              //  9 SPELL_EFFECT_HEALTH_LEECH
    &Spell::EffectHeal,                                     // 10 SPELL_EFFECT_HEAL
    &Spell::EffectBind,                                     // 11 SPELL_EFFECT_BIND
    &Spell::EffectUnused,                                   // 12 SPELL_EFFECT_PORTAL                   unused from pre-1.2.1, exist 2 spell, but not exist any data about its real usage
    &Spell::EffectUnused,                                   // 13 SPELL_EFFECT_RITUAL_BASE              unused from pre-1.2.1
    &Spell::EffectUnused,                                   // 14 SPELL_EFFECT_RITUAL_SPECIALIZE        unused from pre-1.2.1
    &Spell::EffectUnused,                                   // 15 SPELL_EFFECT_RITUAL_ACTIVATE_PORTAL   unused from pre-1.2.1
    &Spell::EffectQuestComplete,                            // 16 SPELL_EFFECT_QUEST_COMPLETE
    &Spell::EffectWeaponDmg,                                // 17 SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL
    &Spell::EffectResurrect,                                // 18 SPELL_EFFECT_RESURRECT
    &Spell::EffectAddExtraAttacks,                          // 19 SPELL_EFFECT_ADD_EXTRA_ATTACKS
    &Spell::EffectEmpty,                                    // 20 SPELL_EFFECT_DODGE                    one spell: Dodge
    &Spell::EffectEmpty,                                    // 21 SPELL_EFFECT_EVADE                    one spell: Evade (DND)
    &Spell::EffectParry,                                    // 22 SPELL_EFFECT_PARRY
    &Spell::EffectBlock,                                    // 23 SPELL_EFFECT_BLOCK                    one spell: Block
    &Spell::EffectCreateItem,                               // 24 SPELL_EFFECT_CREATE_ITEM
    &Spell::EffectEmpty,                                    // 25 SPELL_EFFECT_WEAPON                   spell per weapon type, in ItemSubclassmask store mask that can be used for usability check at equip, but current way using skill also work.
    &Spell::EffectEmpty,                                    // 26 SPELL_EFFECT_DEFENSE                  one spell: Defense
    &Spell::EffectPersistentAA,                             // 27 SPELL_EFFECT_PERSISTENT_AREA_AURA
    &Spell::EffectSummonType,                               // 28 SPELL_EFFECT_SUMMON
    &Spell::EffectLeapForward,                              // 29 SPELL_EFFECT_LEAP
    &Spell::EffectEnergize,                                 // 30 SPELL_EFFECT_ENERGIZE
    &Spell::EffectWeaponDmg,                                // 31 SPELL_EFFECT_WEAPON_PERCENT_DAMAGE
    &Spell::EffectTriggerMissileSpell,                      // 32 SPELL_EFFECT_TRIGGER_MISSILE
    &Spell::EffectOpenLock,                                 // 33 SPELL_EFFECT_OPEN_LOCK
    &Spell::EffectSummonChangeItem,                         // 34 SPELL_EFFECT_SUMMON_CHANGE_ITEM
    &Spell::EffectApplyAreaAura,                            // 35 SPELL_EFFECT_APPLY_AREA_AURA_PARTY
    &Spell::EffectLearnSpell,                               // 36 SPELL_EFFECT_LEARN_SPELL
    &Spell::EffectEmpty,                                    // 37 SPELL_EFFECT_SPELL_DEFENSE            one spell: SPELLDEFENSE (DND)
    &Spell::EffectDispel,                                   // 38 SPELL_EFFECT_DISPEL
    &Spell::EffectEmpty,                                    // 39 SPELL_EFFECT_LANGUAGE                 misc store lang id
    &Spell::EffectDualWield,                                // 40 SPELL_EFFECT_DUAL_WIELD
    &Spell::EffectJump,                                     // 41 SPELL_EFFECT_JUMP
    &Spell::EffectJump,                                     // 42 SPELL_EFFECT_JUMP2
    &Spell::EffectTeleUnitsFaceCaster,                      // 43 SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER
    &Spell::EffectLearnSkill,                               // 44 SPELL_EFFECT_SKILL_STEP
    &Spell::EffectNULL,                                     // 45 SPELL_EFFECT_PLAY_MOVIE
    &Spell::EffectNULL,                                     // 46 SPELL_EFFECT_SPAWN                    spawn/login animation, expected by spawn unit cast, also base points store some dynflags
    &Spell::EffectTradeSkill,                               // 47 SPELL_EFFECT_TRADE_SKILL
    &Spell::EffectUnused,                                   // 48 SPELL_EFFECT_STEALTH                  one spell: Base Stealth
    &Spell::EffectUnused,                                   // 49 SPELL_EFFECT_DETECT                   one spell: Detect
    &Spell::EffectTransmitted,                              // 50 SPELL_EFFECT_TRANS_DOOR
    &Spell::EffectUnused,                                   // 51 SPELL_EFFECT_FORCE_CRITICAL_HIT       unused from pre-1.2.1
    &Spell::EffectUnused,                                   // 52 SPELL_EFFECT_GUARANTEE_HIT            unused from pre-1.2.1
    &Spell::EffectEnchantItemPerm,                          // 53 SPELL_EFFECT_ENCHANT_ITEM
    &Spell::EffectEnchantItemTmp,                           // 54 SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY
    &Spell::EffectTameCreature,                             // 55 SPELL_EFFECT_TAMECREATURE
    &Spell::EffectSummonPet,                                // 56 SPELL_EFFECT_SUMMON_PET
    &Spell::EffectLearnPetSpell,                            // 57 SPELL_EFFECT_LEARN_PET_SPELL
    &Spell::EffectWeaponDmg,                                // 58 SPELL_EFFECT_WEAPON_DAMAGE
    &Spell::EffectCreateRandomItem,                         // 59 SPELL_EFFECT_CREATE_RANDOM_ITEM       create item base at spell specific loot
    &Spell::EffectProficiency,                              // 60 SPELL_EFFECT_PROFICIENCY
    &Spell::EffectSendEvent,                                // 61 SPELL_EFFECT_SEND_EVENT
    &Spell::EffectPowerBurn,                                // 62 SPELL_EFFECT_POWER_BURN
    &Spell::EffectThreat,                                   // 63 SPELL_EFFECT_THREAT
    &Spell::EffectTriggerSpell,                             // 64 SPELL_EFFECT_TRIGGER_SPELL
    &Spell::EffectApplyAreaAura,                            // 65 SPELL_EFFECT_APPLY_AREA_AURA_RAID
    &Spell::EffectRestoreItemCharges,                       // 66 SPELL_EFFECT_RESTORE_ITEM_CHARGES     itemtype - is affected item ID
    &Spell::EffectHealMaxHealth,                            // 67 SPELL_EFFECT_HEAL_MAX_HEALTH
    &Spell::EffectInterruptCast,                            // 68 SPELL_EFFECT_INTERRUPT_CAST
    &Spell::EffectDistract,                                 // 69 SPELL_EFFECT_DISTRACT
    &Spell::EffectPull,                                     // 70 SPELL_EFFECT_PULL                     one spell: Distract Move
    &Spell::EffectPickPocket,                               // 71 SPELL_EFFECT_PICKPOCKET
    &Spell::EffectAddFarsight,                              // 72 SPELL_EFFECT_ADD_FARSIGHT
    &Spell::EffectNULL,                                     // 73 SPELL_EFFECT_UNTRAIN_TALENTS          one spell: Trainer: Untrain Talents
    &Spell::EffectApplyGlyph,                               // 74 SPELL_EFFECT_APPLY_GLYPH
    &Spell::EffectHealMechanical,                           // 75 SPELL_EFFECT_HEAL_MECHANICAL          one spell: Mechanical Patch Kit
    &Spell::EffectSummonObjectWild,                         // 76 SPELL_EFFECT_SUMMON_OBJECT_WILD
    &Spell::EffectScriptEffect,                             // 77 SPELL_EFFECT_SCRIPT_EFFECT
    &Spell::EffectUnused,                                   // 78 SPELL_EFFECT_ATTACK
    &Spell::EffectSanctuary,                                // 79 SPELL_EFFECT_SANCTUARY
    &Spell::EffectAddComboPoints,                           // 80 SPELL_EFFECT_ADD_COMBO_POINTS
    &Spell::EffectUnused,                                   // 81 SPELL_EFFECT_CREATE_HOUSE             one spell: Create House (TEST)
    &Spell::EffectNULL,                                     // 82 SPELL_EFFECT_BIND_SIGHT
    &Spell::EffectDuel,                                     // 83 SPELL_EFFECT_DUEL
    &Spell::EffectStuck,                                    // 84 SPELL_EFFECT_STUCK
    &Spell::EffectSummonPlayer,                             // 85 SPELL_EFFECT_SUMMON_PLAYER
    &Spell::EffectActivateObject,                           // 86 SPELL_EFFECT_ACTIVATE_OBJECT
    &Spell::EffectWMODamage,                                // 87 SPELL_EFFECT_WMO_DAMAGE (57 spells in 3.3.2)
    &Spell::EffectWMORepair,                                // 88 SPELL_EFFECT_WMO_REPAIR (2 spells in 3.3.2)
    &Spell::EffectWMOChange,                                // 89 SPELL_EFFECT_WMO_CHANGE (7 spells in 3.3.2)
    &Spell::EffectKillCreditPersonal,                       // 90 SPELL_EFFECT_KILL_CREDIT_PERSONAL     Kill credit but only for single person
    &Spell::EffectUnused,                                   // 91 SPELL_EFFECT_THREAT_ALL               one spell: zzOLDBrainwash
    &Spell::EffectEnchantHeldItem,                          // 92 SPELL_EFFECT_ENCHANT_HELD_ITEM
    &Spell::EffectBreakPlayerTargeting,                     // 93 SPELL_EFFECT_BREAK_PLAYER_TARGETING
    &Spell::EffectSelfResurrect,                            // 94 SPELL_EFFECT_SELF_RESURRECT
    &Spell::EffectSkinning,                                 // 95 SPELL_EFFECT_SKINNING
    &Spell::EffectCharge,                                   // 96 SPELL_EFFECT_CHARGE
    &Spell::EffectSummonAllTotems,                          // 97 SPELL_EFFECT_SUMMON_ALL_TOTEMS
    &Spell::EffectKnockBack,                                // 98 SPELL_EFFECT_KNOCK_BACK
    &Spell::EffectDisEnchant,                               // 99 SPELL_EFFECT_DISENCHANT
    &Spell::EffectInebriate,                                //100 SPELL_EFFECT_INEBRIATE
    &Spell::EffectFeedPet,                                  //101 SPELL_EFFECT_FEED_PET
    &Spell::EffectDismissPet,                               //102 SPELL_EFFECT_DISMISS_PET
    &Spell::EffectReputation,                               //103 SPELL_EFFECT_REPUTATION
    &Spell::EffectSummonObject,                             //104 SPELL_EFFECT_SUMMON_OBJECT_SLOT
    &Spell::EffectNULL,                                     //105 SPELL_EFFECT_SURVEY
    &Spell::EffectNULL,                                     //106 SPELL_EFFECT_SUMMON_RAID_MARKER
    &Spell::EffectNULL,                                     //107 SPELL_EFFECT_LOOT_CORPSE
    &Spell::EffectDispelMechanic,                           //108 SPELL_EFFECT_DISPEL_MECHANIC
    &Spell::EffectSummonDeadPet,                            //109 SPELL_EFFECT_SUMMON_DEAD_PET
    &Spell::EffectDestroyAllTotems,                         //110 SPELL_EFFECT_DESTROY_ALL_TOTEMS
    &Spell::EffectDurabilityDamage,                         //111 SPELL_EFFECT_DURABILITY_DAMAGE
    &Spell::EffectUnused,                                   //112 SPELL_EFFECT_112 (old SPELL_EFFECT_SUMMON_DEMON)
    &Spell::EffectResurrectNew,                             //113 SPELL_EFFECT_RESURRECT_NEW
    &Spell::EffectTaunt,                                    //114 SPELL_EFFECT_ATTACK_ME
    &Spell::EffectDurabilityDamagePCT,                      //115 SPELL_EFFECT_DURABILITY_DAMAGE_PCT
    &Spell::EffectSkinPlayerCorpse,                         //116 SPELL_EFFECT_SKIN_PLAYER_CORPSE       one spell: Remove Insignia, bg usage, required special corpse flags...
    &Spell::EffectSpiritHeal,                               //117 SPELL_EFFECT_SPIRIT_HEAL              one spell: Spirit Heal
    &Spell::EffectSkill,                                    //118 SPELL_EFFECT_SKILL                    professions and more
    &Spell::EffectApplyAreaAura,                            //119 SPELL_EFFECT_APPLY_AREA_AURA_PET
    &Spell::EffectUnused,                                   //120 SPELL_EFFECT_TELEPORT_GRAVEYARD       one spell: Graveyard Teleport Test
    &Spell::EffectWeaponDmg,                                //121 SPELL_EFFECT_NORMALIZED_WEAPON_DMG
    &Spell::EffectUnused,                                   //122 SPELL_EFFECT_122                      unused
    &Spell::EffectSendTaxi,                                 //123 SPELL_EFFECT_SEND_TAXI                taxi/flight related (misc value is taxi path id)
    &Spell::EffectPlayerPull,                               //124 SPELL_EFFECT_PLAYER_PULL              opposite of knockback effect (pulls player twoard caster)
    &Spell::EffectModifyThreatPercent,                      //125 SPELL_EFFECT_MODIFY_THREAT_PERCENT
    &Spell::EffectStealBeneficialBuff,                      //126 SPELL_EFFECT_STEAL_BENEFICIAL_BUFF    spell steal effect?
    &Spell::EffectProspecting,                              //127 SPELL_EFFECT_PROSPECTING              Prospecting spell
    &Spell::EffectApplyAreaAura,                            //128 SPELL_EFFECT_APPLY_AREA_AURA_FRIEND
    &Spell::EffectApplyAreaAura,                            //129 SPELL_EFFECT_APPLY_AREA_AURA_ENEMY
    &Spell::EffectRedirectThreat,                           //130 SPELL_EFFECT_REDIRECT_THREAT
    &Spell::EffectPlaySound,                                //131 SPELL_EFFECT_PLAY_SOUND               sound id in misc value (SoundEntries.dbc)
    &Spell::EffectPlayMusic,                                //132 SPELL_EFFECT_PLAY_MUSIC               sound id in misc value (SoundEntries.dbc)
    &Spell::EffectUnlearnSpecialization,                    //133 SPELL_EFFECT_UNLEARN_SPECIALIZATION   unlearn profession specialization
    &Spell::EffectKillCreditGroup,                          //134 SPELL_EFFECT_KILL_CREDIT_GROUP        misc value is creature entry
    &Spell::EffectNULL,                                     //135 SPELL_EFFECT_CALL_PET
    &Spell::EffectHealPct,                                  //136 SPELL_EFFECT_HEAL_PCT
    &Spell::EffectEnergisePct,                              //137 SPELL_EFFECT_ENERGIZE_PCT
    &Spell::EffectLeapBack,                                 //138 SPELL_EFFECT_LEAP_BACK                Leap back
    &Spell::EffectClearQuest,                               //139 SPELL_EFFECT_CLEAR_QUEST              (misc - is quest ID)
    &Spell::EffectForceCast,                                //140 SPELL_EFFECT_FORCE_CAST
    &Spell::EffectForceCast,                                //141 SPELL_EFFECT_FORCE_CAST_WITH_VALUE
    &Spell::EffectTriggerSpellWithValue,                    //142 SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE
    &Spell::EffectApplyAreaAura,                            //143 SPELL_EFFECT_APPLY_AREA_AURA_OWNER
    &Spell::EffectKnockBackFromPosition,                    //144 SPELL_EFFECT_KNOCKBACK_FROM_POSITION
    &Spell::EffectGravityPull,                              //145 SPELL_EFFECT_GRAVITY_PULL
    &Spell::EffectActivateRune,                             //146 SPELL_EFFECT_ACTIVATE_RUNE
    &Spell::EffectQuestFail,                                //147 SPELL_EFFECT_QUEST_FAIL               quest fail
    &Spell::EffectNULL,                                     //148 SPELL_EFFECT_148                      single spell: Inflicts Fire damage to an enemy.
    &Spell::EffectCharge2,                                  //149 SPELL_EFFECT_CHARGE2                  swoop
    &Spell::EffectQuestOffer,                               //150 SPELL_EFFECT_QUEST_OFFER
    &Spell::EffectTriggerRitualOfSummoning,                 //151 SPELL_EFFECT_TRIGGER_SPELL_2
    &Spell::EffectNULL,                                     //152 SPELL_EFFECT_152                      summon Refer-a-Friend
    &Spell::EffectCreateTamedPet,                           //153 SPELL_EFFECT_CREATE_PET               misc value is creature entry
    &Spell::EffectTeachTaxiNode,                            //154 SPELL_EFFECT_TEACH_TAXI_NODE          single spell: Teach River's Heart Taxi Path
    &Spell::EffectTitanGrip,                                //155 SPELL_EFFECT_TITAN_GRIP Allows you to equip two-handed axes, maces and swords in one hand, but you attack $49152s1% slower than normal.
    &Spell::EffectEnchantItemPrismatic,                     //156 SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC
    &Spell::EffectCreateItem2,                              //157 SPELL_EFFECT_CREATE_ITEM_2            create item or create item template and replace by some randon spell loot item
    &Spell::EffectMilling,                                  //158 SPELL_EFFECT_MILLING                  milling
    &Spell::EffectRenamePet,                                //159 SPELL_EFFECT_ALLOW_RENAME_PET         allow rename pet once again
    &Spell::EffectNULL,                                     //160 SPELL_EFFECT_160                      single spell: Nerub'ar Web Random Unit
    &Spell::EffectSpecCount,                                //161 SPELL_EFFECT_TALENT_SPEC_COUNT        second talent spec (learn/revert)
    &Spell::EffectActivateSpec,                             //162 SPELL_EFFECT_TALENT_SPEC_SELECT       activate primary/secondary spec
    &Spell::EffectUnused,                                   //163 unused in 3.3.5a
    &Spell::EffectCancelAura,                               //164 SPELL_EFFECT_CANCEL_AURA
    &Spell::EffectNULL,                                     //165 SPELL_EFFECT_DAMAGE_FROM_MAX_HEALTH_PCT 82 spells in 4.3.4
    &Spell::EffectNULL,                                     //166 SPELL_EFFECT_REWARD_CURRENCY          56 spells in 4.3.4
    &Spell::EffectNULL,                                     //167 SPELL_EFFECT_167                      42 spells in 4.3.4
    &Spell::EffectNULL,                                     //168 SPELL_EFFECT_168                      2 spells in 4.3.4 Allows give commands to controlled pet
    &Spell::EffectNULL,                                     //169 SPELL_EFFECT_DESTROY_ITEM             9 spells in 4.3.4
    &Spell::EffectNULL,                                     //170 SPELL_EFFECT_170                      70 spells in 4.3.4
    &Spell::EffectNULL,                                     //171 SPELL_EFFECT_171                      19 spells in 4.3.4 related to GO summon
    &Spell::EffectNULL,                                     //172 SPELL_EFFECT_MASS_RESSURECTION        Mass Ressurection (Guild Perk)
    &Spell::EffectNULL,                                     //173 SPELL_EFFECT_BUY_GUILD_BANKSLOT       4 spells in 4.3.4 basepoints - slot
    &Spell::EffectNULL,                                     //174 SPELL_EFFECT_174                      13 spells some sort of area aura apply effect
    &Spell::EffectUnused,                                   //175 SPELL_EFFECT_175                      unused in 4.3.4
    &Spell::EffectNULL,                                     //176 SPELL_EFFECT_SANCTUARY_2              4 spells in 4.3.4
    &Spell::EffectNULL,                                     //177 SPELL_EFFECT_177                      2 spells in 4.3.4 Deluge(100757) and test spell
    &Spell::EffectUnused,                                   //178 SPELL_EFFECT_178                      unused in 4.3.4
    &Spell::EffectNULL,                                     //179 SPELL_EFFECT_179                      15 spells in 4.3.4
    &Spell::EffectUnused,                                   //180 SPELL_EFFECT_180                      unused in 4.3.4
    &Spell::EffectUnused,                                   //181 SPELL_EFFECT_181                      unused in 4.3.4
    &Spell::EffectNULL,                                     //182 SPELL_EFFECT_182                      3 spells 4.3.4
};

/**
 * @brief Handles a no-op spell effect used only as a marker.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectEmpty(SpellEffectEntry const* /*effect*/)
{
    // NOT NEED ANY IMPLEMENTATION CODE, EFFECT POSISBLE USED AS MARKER OR CLIENT INFORM
}

/**
 * @brief Handles a legacy null spell effect placeholder.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectNULL(SpellEffectEntry const* /*effect*/)
{
    DEBUG_LOG("WORLD: Spell Effect DUMMY");
}

/**
 * @brief Handles an unused spell effect slot.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectUnused(SpellEffectEntry const* /*effect*/)
{
    // NOT USED BY ANY SPELL OR USELESS OR IMPLEMENTED IN DIFFERENT WAY IN MANGOS
}

/**
 * @brief Sends a resurrection request to a dead player target.
 *
 * @param effect The effect index providing resurrection values.
 */
void Spell::EffectResurrectNew(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->IsAlive())
    {
        return;
    }

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (!unitTarget->IsInWorld())
    {
        return;
    }

    Player* pTarget = ((Player*)unitTarget);

    if (pTarget->isRessurectRequested())      // already have one active request
    {
        return;
    }

    uint32 health = damage;
    uint32 mana = effect->EffectMiscValue;
    pTarget->setResurrectRequestData(m_caster->GetObjectGuid(), m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), health, mana);
    SendResurrectRequest(pTarget);
}

/**
 * @brief Instantly kills the unit target and handles spell-specific side effects.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectInstaKill(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || !unitTarget->IsAlive())
    {
        return;
    }

    if (m_caster == unitTarget)                             // prevent interrupt message
    {
        finish();
    }

    WorldObject* caster = GetCastingObject();               // we need the original casting object

    WorldPacket data(SMSG_SPELLINSTAKILLLOG, (8 + 8 + 4));
    data << (caster && caster->GetTypeId() != TYPEID_GAMEOBJECT ? m_caster->GetObjectGuid() : ObjectGuid()); // Caster GUID
    data << unitTarget->GetObjectGuid();                    // Victim GUID
    data << uint32(m_spellInfo->Id);
    m_caster->SendMessageToSet(&data, true);

    m_caster->DealDamage(unitTarget, unitTarget->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, m_spellInfo, false);
}

/**
 * @brief Applies environmental damage to the caster.
 *
 * @param effect The effect index used to calculate the base damage.
 */
void Spell::EffectEnvironmentalDMG(SpellEffectEntry const* effect)
{
    uint32 absorb = 0;
    uint32 resist = 0;

    // Note: this hack with damage replace required until GO casting not implemented
    // environment damage spells already have around enemies targeting but this not help in case nonexistent GO casting support
    // currently each enemy selected explicitly and self cast damage, we prevent apply self casted spell bonuses/etc
    damage = effect->CalculateSimpleValue();

    m_caster->CalculateDamageAbsorbAndResist(m_caster, GetSpellSchoolMask(m_spellInfo), SPELL_DIRECT_DAMAGE, damage, &absorb, &resist);

    m_caster->SendSpellNonMeleeDamageLog(m_caster, m_spellInfo->Id, damage, GetSpellSchoolMask(m_spellInfo), absorb, resist, false, 0, false);
    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)m_caster)->EnvironmentalDamage(DAMAGE_FIRE, damage);
    }
}

/**
 * @brief Computes school-damage special cases and accumulates resulting damage.
 *
 * @param effect The damage effect index.
 */
void Spell::EffectSchoolDMG(SpellEffectEntry const* effect)
{
    if (unitTarget && unitTarget->IsAlive())
    {
        SpellClassOptionsEntry const* classOptions = m_spellInfo->GetSpellClassOptions();

        switch(m_spellInfo->GetSpellFamilyName())
        {
            case SPELLFAMILY_GENERIC:
            {
                switch (m_spellInfo->Id)                    // better way to check unknown
                {
                    // Meteor like spells (divided damage to targets)
                    case 24340: case 26558: case 28884:     // Meteor
                    case 36837: case 38903: case 41276:     // Meteor
                    case 57467:                             // Meteor
                    case 26789:                             // Shard of the Fallen Star
                    case 31436:                             // Malevolent Cleave
                    case 35181:                             // Dive Bomb
                    case 40810: case 43267: case 43268:     // Saber Lash
                    case 42384:                             // Brutal Swipe
                    case 45150:                             // Meteor Slash
                    case 64422: case 64688:                 // Sonic Screech
                    case 70492: case 72505:                 // Ooze Eruption
                    case 71904:                             // Chaos Bane
                    case 72624: case 72625:                 // Ooze Eruption
                    case 77679: case 92968: case 92969: case 92970:     // Scorching Blast
                    case 82935: case 88915: case 88916: case 88917:     // Caustic Slime
                    case 86014: case 92863: case 92864: case 92865:     // Twilight Meteorite
                    case 86367: case 93135: case 93136: case 93137:     // Sleet Storm
                    case 86825: case 92879: case 92880: case 92881:     // Blackout
                    case 88942: case 95172:                 // Meteor Slash
                    case 89348: case 95178:                 // Demon Repellent Ray
                    case 98474: case 100212: case 100213: case 100214:  //Flame Scythe
                    case 103414: case 108571: case 109033: case 109034: //Stomp
                    case 105069: case 108094:               // Seething Hate
                    case 106375: case 109182: case 109183: case 109184: //Twilight Unstable
                    {
                        uint32 count = 0;
                        for(TargetList::const_iterator ihit = m_UniqueTargetInfo.begin(); ihit != m_UniqueTargetInfo.end(); ++ihit)
                            if (ihit->effectMask & (1<<effect->EffectIndex))
                            {
                                ++count;
                            }

                        damage /= count;                    // divide to all targets
                        break;
                    }
                    // percent from health with min
                    case 25599:                             // Thundercrash
                    {
                        damage = unitTarget->GetHealth() / 2;
                        if (damage < 200)
                        {
                            damage = 200;
                        }
                        break;
                    }
                    // Intercept (warrior spell trigger)
                    case 20253:
                    case 61491:
                    {
                        damage += uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.12f);
                        break;
                    }
                    // percent max target health
                    case 29142:                             // Eyesore Blaster
                    case 35139:                             // Throw Boom's Doom
                    case 49882:                             // Leviroth Self-Impale
                    case 55269:                             // Deathly Stare
                    {
                        damage = damage * unitTarget->GetMaxHealth() / 100;
                        break;
                    }
                    // Lightning Strike
                    case 37841:
                        if (unitTarget->GetTypeId() == TYPEID_PLAYER && unitTarget->HasAura(37830)) // Repolarized Magneto Sphere
                        {
                            ((Player*)unitTarget)->KilledMonsterCredit(21910);
                        }
                        break;
                    // Cataclysmic Bolt
                    // Cataclysmic Bolt
                    case 38441:
                    {
                        damage = unitTarget->GetMaxHealth() / 2;
                        break;
                    }
                    // Touch the Nightmare
                    case 50341:
                    {
                        if (SpellEffectIndex(effect->EffectIndex) == EFFECT_INDEX_2)
                        {
                            damage = int32(unitTarget->GetMaxHealth() * 0.3f);
                        }
                        break;
                    }
                    // Tympanic Tantrum
                    case 62775:
                    {
                        damage = unitTarget->GetMaxHealth() / 10;
                        break;
                    }
                    // Hand of Rekoning (name not have typos ;) )
                    case 67485:
                        damage += uint32(0.5f * m_caster->GetTotalAttackPowerValue(BASE_ATTACK));
                        break;
                    // Magic Bane normal (Forge of Souls - Bronjahm)
                    case 68793:
                    {
                        damage += uint32(unitTarget->GetMaxPower(POWER_MANA) / 2);
                        damage = std::min(damage, 10000);
                        break;
                    }
                    // Magic Bane heroic (Forge of Souls - Bronjahm)
                    case 69050:
                    {
                        damage += uint32(unitTarget->GetMaxPower(POWER_MANA) / 2);
                        damage = std::min(damage, 15000);
                        break;
                    }
                }
                break;
            }
            case SPELLFAMILY_MAGE:
                // remove Arcane Blast buffs at any non-Arcane Blast arcane damage spell.
                // NOTE: it removed at hit instead cast because currently spell done-damage calculated at hit instead cast
                if ((m_spellInfo->SchoolMask & SPELL_SCHOOL_MASK_ARCANE) && !(classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x20000000)))
                {
                    m_caster->RemoveAurasDueToSpell(36032); // Arcane Blast buff
                }
                break;
            case SPELLFAMILY_WARRIOR:
            {
                // Bloodthirst
                if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x40000000000))
                {
                    damage = uint32(damage * (m_caster->GetTotalAttackPowerValue(BASE_ATTACK)) / 100);
                }
                // Victory Rush
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x10000000000))
                {
                    damage = uint32(damage * m_caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                    m_caster->ModifyAuraState(AURA_STATE_WARRIOR_VICTORY_RUSH, false);
                }
                // Revenge ${$m1+$AP*0.310} to ${$M1+$AP*0.310}
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000000400))
                {
                    damage+= uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.310f);
                }
                // Heroic Throw ${$m1+$AP*.50}
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000100000000))
                {
                    damage+= uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.5f);
                }
                // Shattering Throw ${$m1+$AP*.50}
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0040000000000000))
                {
                    damage+= uint32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.5f);
                }
                // Shockwave ${$m3/100*$AP}
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000800000000000))
                {
                    int32 pct = m_caster->CalculateSpellDamage(unitTarget, m_spellInfo, EFFECT_INDEX_2);
                    if (pct > 0)
                    {
                        damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * pct / 100);
                    }
                    break;
                }
                // Thunder Clap
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000000080))
                {
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 12 / 100);
                }
                break;
            }
            case SPELLFAMILY_WARLOCK:
            {
                // Incinerate Rank 1 & 2
                if ((classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00004000000000)) && m_spellInfo->SpellIconID==2128)
                {
                    // Incinerate does more dmg (dmg*0.25) if the target have Immolate debuff.
                    // Check aura state for speed but aura state set not only for Immolate spell
                    if (unitTarget->HasAuraState(AURA_STATE_CONFLAGRATE))
                    {
                        Unit::AuraList const& RejorRegr = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                        for (Unit::AuraList::const_iterator i = RejorRegr.begin(); i != RejorRegr.end(); ++i)
                        {
                            // Immolate
                            SpellClassOptionsEntry const* immSpellClassOpt = (*i)->GetSpellProto()->GetSpellClassOptions();
                            if (!immSpellClassOpt)
                            {
                                continue;
                            }
                            if (immSpellClassOpt->SpellFamilyName == SPELLFAMILY_WARLOCK &&
                                (immSpellClassOpt->SpellFamilyFlags & UI64LIT(0x00000000000004)))
                            {
                                damage += damage / 4;
                                break;
                            }
                        }
                    }
                }
                // Shadowflame
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0001000000000000))
                {
                    // Apply DOT part
                    switch (m_spellInfo->Id)
                    {
                        case 47897: m_caster->CastSpell(unitTarget, 47960, true); break;
                        case 61290: m_caster->CastSpell(unitTarget, 61291, true); break;
                        default:
                            sLog.outError("Spell::EffectDummy: Unhandeled Shadowflame spell rank %u", m_spellInfo->Id);
                            break;
                    }
                }
                // Shadow Bite
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0040000000000000))
                {
                    Unit* owner = m_caster->GetOwner();
                    if (!owner)
                    {
                        break;
                    }

                    uint32 counter = 0;
                    Unit::AuraList const& dotAuras = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraList::const_iterator itr = dotAuras.begin(); itr != dotAuras.end(); ++itr)
                        if ((*itr)->GetCasterGuid() == owner->GetObjectGuid())
                        {
                            ++counter;
                        }

                    if (counter)
                    {
                        damage += (counter * owner->CalculateSpellDamage(unitTarget, m_spellInfo, EFFECT_INDEX_2) * damage) / 100.0f;
                    }
                }
                // Conflagrate - consumes Immolate or Shadowflame
                else if (m_spellInfo->GetTargetAuraState() == AURA_STATE_CONFLAGRATE)
                {
                    Aura const* aura = NULL;                // found req. aura for damage calculation

                    Unit::AuraList const& mPeriodic = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraList::const_iterator i = mPeriodic.begin(); i != mPeriodic.end(); ++i)
                    {
                        // for caster applied auras only
                        if ((*i)->GetSpellProto()->GetSpellFamilyName() != SPELLFAMILY_WARLOCK ||
                            (*i)->GetCasterGuid() != m_caster->GetObjectGuid())
                            continue;

                        // Immolate
                        if ((*i)->GetSpellProto()->IsFitToFamilyMask(UI64LIT(0x0000000000000004)))
                        {
                            aura = *i;                      // it selected always if exist
                            break;
                        }

                        // Shadowflame
                        if ((*i)->GetSpellProto()->IsFitToFamilyMask(UI64LIT(0x0000000000000000), 0x00000002))
                        {
                            aura = *i;                      // remember but wait possible Immolate as primary priority
                        }
                    }

                    // found Immolate or Shadowflame
                    if (aura)
                    {
                        int32 damagetick = aura->GetModifier()->m_amount;
                        damage += damagetick * 4;

                        // Glyph of Conflagrate
                        if (!m_caster->HasAura(56235))
                        {
                            unitTarget->RemoveAurasByCasterSpell(aura->GetId(), m_caster->GetObjectGuid());
                        }
                        break;
                    }
                }
                break;
            }
            case SPELLFAMILY_PRIEST:
            {
                // Shadow Word: Death - deals damage equal to damage done to caster
                if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000200000000))
                {
                    m_caster->CastCustomSpell(m_caster, 32409, &damage, 0, 0, true);
                }
                // Improved Mind Blast (Mind Blast in shadow form bonus)
                else if (m_caster->GetShapeshiftForm() == FORM_SHADOW && (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00002000)))
                {
                    Unit::AuraList const& ImprMindBlast = m_caster->GetAurasByType(SPELL_AURA_ADD_FLAT_MODIFIER);
                    for (Unit::AuraList::const_iterator i = ImprMindBlast.begin(); i != ImprMindBlast.end(); ++i)
                    {
                        if ((*i)->GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_PRIEST &&
                            ((*i)->GetSpellProto()->SpellIconID == 95))
                        {
                            int chance = (*i)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_1);
                            if (roll_chance_i(chance))
                                // Mind Trauma
                                m_caster->CastSpell(unitTarget, 48301, true);
                            break;
                        }
                    }
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                SpellEffectEntry const* rakeSpellEffect = m_spellInfo->GetSpellEffect(EFFECT_INDEX_2);
                // Ferocious Bite
                if (m_caster->GetTypeId()==TYPEID_PLAYER && (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x000800000)) && m_spellInfo->SpellVisual[0]==6587)
                {
                    // converts up to 30 points of energy into ($f1+$AP/410) additional damage
                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    float multiple = ap / 410 + effect->EffectDamageMultiplier;
                    damage += int32(((Player*)m_caster)->GetComboPoints() * ap * 7 / 100);
                    uint32 energy = m_caster->GetPower(POWER_ENERGY);
                    uint32 used_energy = energy > 30 ? 30 : energy;
                    damage += int32(used_energy * multiple);
                    m_caster->SetPower(POWER_ENERGY, energy - used_energy);
                }
                // Rake
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000001000) && rakeSpellEffect && rakeSpellEffect->Effect == SPELL_EFFECT_ADD_COMBO_POINTS)
                {
                    // $AP*0.01 bonus
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                }
                // Swipe
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0010000000000000))
                {
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.08f);
                }
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                // Envenom
                if (m_caster->GetTypeId()==TYPEID_PLAYER && (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x800000000)))
                {
                    // consume from stack dozes not more that have combo-points
                    if (uint32 combo = ((Player*)m_caster)->GetComboPoints())
                    {
                        Aura* poison = 0;
                        // Lookup for Deadly poison (only attacker applied)
                        Unit::AuraList const& auras = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                        for(Unit::AuraList::const_iterator itr = auras.begin(); itr!=auras.end(); ++itr)
                        {
                            SpellClassOptionsEntry const* poisonClassOptions = (*itr)->GetSpellProto()->GetSpellClassOptions();
                            if (!poisonClassOptions)
                            {
                                continue;
                            }
                            if ( poisonClassOptions->SpellFamilyName==SPELLFAMILY_ROGUE &&
                                (poisonClassOptions->SpellFamilyFlags & UI64LIT(0x10000)) &&
                                (*itr)->GetCasterGuid() == m_caster->GetObjectGuid())
                            {
                                poison = *itr;
                                break;
                            }
                        }
                        // count consumed deadly poison doses at target
                        if (poison)
                        {
                            bool needConsume = true;
                            uint32 spellId = poison->GetId();
                            uint32 doses = poison->GetStackAmount();
                            if (doses > combo)
                            {
                                doses = combo;
                            }

                            // Master Poisoner
                            Unit::AuraList const& auraList = ((Player*)m_caster)->GetAurasByType(SPELL_AURA_MOD_DURATION_OF_EFFECTS_BY_DISPEL);
                            for (Unit::AuraList::const_iterator iter = auraList.begin(); iter != auraList.end(); ++iter)
                            {
                                if ((*iter)->GetSpellProto()->GetSpellFamilyName() == SPELLFAMILY_ROGUE && (*iter)->GetSpellProto()->SpellIconID == 1960)
                                {
                                    if (int32 chance = (*iter)->GetSpellProto()->CalculateSimpleValue(EFFECT_INDEX_2))
                                        if (roll_chance_i(chance))
                                        {
                                            needConsume = false;
                                        }

                                    break;
                                }
                            }

                            if (needConsume)
                            {
                                unitTarget->RemoveAuraHolderFromStack(spellId, doses, m_caster->GetObjectGuid());
                            }

                            damage *= doses;
                            damage += int32(((Player*)m_caster)->GetTotalAttackPowerValue(BASE_ATTACK) * 0.09f * doses);
                        }
                        // Eviscerate and Envenom Bonus Damage (item set effect)
                        if (m_caster->GetDummyAura(37169))
                        {
                            damage += ((Player*)m_caster)->GetComboPoints() * 40;
                        }
                    }
                }
                // Eviscerate
                else if ((classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x00020000)) && m_caster->GetTypeId()==TYPEID_PLAYER)
                {
                    if (uint32 combo = ((Player*)m_caster)->GetComboPoints())
                    {
                        float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                        damage += irand(int32(ap * combo * 0.03f), int32(ap * combo * 0.07f));

                        // Eviscerate and Envenom Bonus Damage (item set effect)
                        if (m_caster->GetDummyAura(37169))
                        {
                            damage += combo * 40;
                        }
                    }
                }
                break;
            }
            case SPELLFAMILY_HUNTER:
            {
                // Gore
                if (m_spellInfo->SpellIconID == 1578)
                {
                    if (m_caster->HasAura(57627))           // Charge 6 sec post-affect
                    {
                        damage *= 2;
                    }
                }
                // Steady Shot
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x100000000))
                {
                    int32 base = irand((int32)m_caster->GetWeaponDamageRange(RANGED_ATTACK, MINDAMAGE), (int32)m_caster->GetWeaponDamageRange(RANGED_ATTACK, MAXDAMAGE));
                    uint32 attTime = m_caster->GetAttackTime(RANGED_ATTACK);
                    damage += attTime ? int32(float(base) / attTime * 2800 + m_caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 0.1f) : 0;
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                // Judgement of Righteousness - receive benefit from Spell Damage and Attack power
                if (m_spellInfo->Id == 20187)
                {
                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                    if (holy < 0)
                    {
                        holy = 0;
                    }
                    damage += int32(ap * 0.2f) + int32(holy * 32 / 100);
                }
                // Judgement of Vengeance/Corruption ${1+0.22*$SPH+0.14*$AP} + 10% for each application of Holy Vengeance/Blood Corruption on the target
                else if ((classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x800000000)) && m_spellInfo->SpellIconID==2292)
                {
                    uint32 debuf_id;
                    switch (m_spellInfo->Id)
                    {
                        case 53733: debuf_id = 53742; break;// Judgement of Corruption -> Blood Corruption
                        case 31804: debuf_id = 31803; break;// Judgement of Vengeance -> Holy Vengeance
                        default: return;
                    }

                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                    if (holy < 0)
                    {
                        holy = 0;
                    }
                    damage += int32(ap * 0.14f) + int32(holy * 22 / 100);
                    // Get stack of Holy Vengeance on the target added by caster
                    uint32 stacks = 0;
                    Unit::AuraList const& auras = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    {
                        if (((*itr)->GetId() == debuf_id) && (*itr)->GetCasterGuid() == m_caster->GetObjectGuid())
                        {
                            stacks = (*itr)->GetStackAmount();
                            break;
                        }
                    }
                    // + 10% for each application of Holy Vengeance on the target
                    if (stacks)
                    {
                        damage += damage * stacks * 10 / 100;
                    }
                }
                // Avenger's Shield ($m1+0.07*$SPH+0.07*$AP) - ranged sdb for future
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000000000004000))
                {
                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                    if (holy < 0)
                    {
                        holy = 0;
                    }
                    damage += int32(ap * 0.07f) + int32(holy * 7 / 100);
                }
                // Hammer of Wrath ($m1+0.15*$SPH+0.15*$AP) - ranged type sdb future fix
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0000008000000000))
                {
                    float ap = m_caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 holy = m_caster->SpellBaseDamageBonusDone(GetSpellSchoolMask(m_spellInfo));
                    if (holy < 0)
                    {
                        holy = 0;
                    }
                    damage += int32(ap * 0.15f) + int32(holy * 15 / 100);
                }
                // Hammer of the Righteous
                else if (classOptions && classOptions->SpellFamilyFlags & UI64LIT(0x0004000000000000))
                {
                    // Add main hand dps * effect[2] amount
                    float average = (m_caster->GetFloatValue(UNIT_FIELD_MINDAMAGE) + m_caster->GetFloatValue(UNIT_FIELD_MAXDAMAGE)) / 2;
                    int32 count = m_caster->CalculateSpellDamage(unitTarget, m_spellInfo, EFFECT_INDEX_2);
                    uint32 attTime = m_caster->GetAttackTime(BASE_ATTACK);
                    damage += attTime ? count * int32(average * IN_MILLISECONDS) / attTime : 0;
                }
                // Judgement
                else if (m_spellInfo->Id == 20271)
                {
                    // [1 + 0.25 * SPH + 0.16 * AP]
                    damage += int32(m_caster->GetTotalAttackPowerValue(BASE_ATTACK) * 0.16f);
                }
                break;
            }
        }

        if (damage >= 0)
        {
            m_damage += damage;
        }
    }
}

void Spell::EffectTriggerSpellWithValue(SpellEffectEntry const* effect)
{
    uint32 triggered_spell_id = effect->EffectTriggerSpell;

    // normal case
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        sLog.outError("EffectTriggerSpellWithValue of spell %u: triggering unknown spell id %i", m_spellInfo->Id, triggered_spell_id);
        return;
    }

    int32 bp = damage;
    m_caster->CastCustomSpell(unitTarget, triggered_spell_id, &bp, &bp, &bp, true, m_CastItem , NULL, m_originalCasterGUID, m_spellInfo);
}

void Spell::EffectTriggerRitualOfSummoning(SpellEffectEntry const* effect)
{
    uint32 triggered_spell_id = effect->EffectTriggerSpell;
    SpellEntry const *spellInfo = sSpellStore.LookupEntry( triggered_spell_id );

    if (!spellInfo)
    {
        sLog.outError("EffectTriggerRitualOfSummoning of spell %u: triggering unknown spell id %i", m_spellInfo->Id, triggered_spell_id);
        return;
    }

    finish();

    m_caster->CastSpell(unitTarget, spellInfo, false);
}

void Spell::EffectClearQuest(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)m_caster;

    uint32 quest_id = effect->EffectMiscValue;

    if (!sObjectMgr.GetQuestTemplate(quest_id))
    {
        sLog.outError("Spell::EffectClearQuest spell entry %u attempt clear quest entry %u but this quest does not exist.", m_spellInfo->Id, quest_id);
        return;
    }

    // remove quest possibly in quest log (is that expected?)
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 quest = player->GetQuestSlotQuestId(slot);

        if (quest == quest_id)
        {
            player->SetQuestSlot(slot, 0);
            // ignore unequippable quest items in this case, it will still be equipped
            player->TakeQuestSourceItem(quest_id, false);
        }
    }

    player->SetQuestStatus(quest_id, QUEST_STATUS_NONE);
    player->getQuestStatusMap()[quest_id].m_rewarded = false;
}

void Spell::EffectForceCast(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart spellid %u in EffectDummy", m_spellInfo->Id);
    m_caster->GetMap()->ScriptsStart(DBS_ON_SPELL, m_spellInfo->Id, m_caster, unitTarget);
}

/**
 * @brief Triggers another spell on the current unit target.
 *
 * @param effect The effect index providing the triggered spell id.
 */
void Spell::EffectTriggerSpell(SpellEffectEntry const* effect)
{
    // only unit case known
    if (!unitTarget)
    {
        if (gameObjTarget || itemTarget)
        {
            sLog.outError("Spell::EffectTriggerSpell (Spell: %u): Unsupported non-unit case!", m_spellInfo->Id);
        }
        return;
    }

    uint32 triggered_spell_id = effect->EffectTriggerSpell;

    // special cases
    switch (triggered_spell_id)
    {
        case 18461:                                         // Vanish (not exist)
        {
            unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
            unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_DECREASE_SPEED);
            unitTarget->RemoveSpellsCausingAura(SPELL_AURA_MOD_STALKED);

            // if this spell is given to NPC it must handle rest by it's own AI
            if (unitTarget->GetTypeId() != TYPEID_PLAYER)
            {
                return;
            }

            uint32 spellId = 1784;
            // reset cooldown on it if needed
            if (((Player*)unitTarget)->HasSpellCooldown(spellId))
            {
                ((Player*)unitTarget)->RemoveSpellCooldown(spellId);
            }

            m_caster->CastSpell(unitTarget, spellId, true);
            return;
        }
        case 29284:                                         // Brittle Armor - (need add max stack of 24575 Brittle Armor)
            m_caster->CastSpell(unitTarget, 24575, true, m_CastItem, NULL, m_originalCasterGUID);
            return;
        case 29286:                                         // Mercurial Shield - (need add max stack of 26464 Mercurial Shield)
            m_caster->CastSpell(unitTarget, 26464, true, m_CastItem, NULL, m_originalCasterGUID);
            return;
        case 31980:                                         // Righteous Defense
        {
            m_caster->CastSpell(unitTarget, 31790, true, m_CastItem, NULL, m_originalCasterGUID);
            return;
        }
        case 35729:                                         // Cloak of Shadows
        {
            Unit::SpellAuraHolderMap& Auras = unitTarget->GetSpellAuraHolderMap();
            for (Unit::SpellAuraHolderMap::iterator iter = Auras.begin(); iter != Auras.end(); ++iter)
            {
                // Remove all harmful spells on you except positive/passive/physical auras
                if (!iter->second->IsPositive() &&
                        !iter->second->IsPassive() &&
                        !iter->second->IsDeathPersistent() &&
                        (GetSpellSchoolMask(iter->second->GetSpellProto()) & SPELL_SCHOOL_MASK_NORMAL) == 0)
                {
                    m_caster->RemoveAurasDueToSpell(iter->second->GetSpellProto()->Id);
                    iter = Auras.begin();
                }
            }
            return;
        }
        case 41967:                                         // Priest Shadowfiend (34433) need apply mana gain trigger aura on pet
        {
            if (Unit* pet = unitTarget->GetPet())
            {
                pet->CastSpell(pet, 28305, true);
            }
            return;
        }
        case 58832:                                         // Mirror Image
        {
            // Glyph of Mirror Image
            if (m_caster->HasAura(63093))
            {
                m_caster->CastSpell(m_caster, 65047, true, m_CastItem, NULL, m_originalCasterGUID);
            }
            break;
        }
    }

    // normal case
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(triggered_spell_id);
    if (!spellInfo)
    {
        sLog.outError("EffectTriggerSpell of spell %u: triggering unknown spell id %i", m_spellInfo->Id, triggered_spell_id);
        return;
    }

    // select formal caster for triggered spell
    Unit* caster = m_caster;

    // some triggered spells require specific equipment
    if (spellInfo->GetEquippedItemClass() >=0 && m_caster->GetTypeId()==TYPEID_PLAYER)
    {
        // main hand weapon required
        if (spellInfo->HasAttribute(SPELL_ATTR_EX3_MAIN_HAND))
        {
            Item* item = ((Player*)m_caster)->GetWeaponForAttack(BASE_ATTACK, true, false);

            // skip spell if no weapon in slot or broken
            if (!item)
            {
                return;
            }

            // skip spell if weapon not fit to triggered spell
            if (!item->IsFitToSpellRequirements(spellInfo))
            {
                return;
            }
        }

        // offhand hand weapon required
        if (spellInfo->HasAttribute(SPELL_ATTR_EX3_REQ_OFFHAND))
        {
            Item* item = ((Player*)m_caster)->GetWeaponForAttack(OFF_ATTACK, true, false);

            // skip spell if no weapon in slot or broken
            if (!item)
            {
                return;
            }

            // skip spell if weapon not fit to triggered spell
            if (!item->IsFitToSpellRequirements(spellInfo))
            {
                return;
            }
        }
    }
    else
    {
        // Note: not exist spells with weapon req. and IsSpellHaveCasterSourceTargets == true
        // so this just for speedup places in else
        caster = IsSpellWithCasterSourceTargetsOnly(spellInfo) ? unitTarget : m_caster;
    }

    caster->CastSpell(unitTarget, spellInfo, true, m_CastItem, NULL, m_originalCasterGUID, m_spellInfo);
}

/**
 * @brief Triggers a missile spell at the stored destination coordinates.
 *
 * @param effect The effect index providing the triggered spell id.
 */
void Spell::EffectTriggerMissileSpell(SpellEffectEntry const* effect)
{
    uint32 triggered_spell_id = effect->EffectTriggerSpell;

    // normal case
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(triggered_spell_id);

    if (!spellInfo)
    {
        if (unitTarget)
        {
            DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart spellid %u in EffectTriggerMissileSpell", m_spellInfo->Id);
            m_caster->GetMap()->ScriptsStart(DBS_ON_SPELL, m_spellInfo->Id, m_caster, unitTarget);
        }
        else
        {
            sLog.outError("EffectTriggerMissileSpell of spell %u (eff: %u): triggering unknown spell id %u",
                      m_spellInfo->Id, effect->EffectIndex, triggered_spell_id);
        }
        return;
    }

    if (m_CastItem)
    {
        DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "WORLD: cast Item spellId - %i", spellInfo->Id);
    }

    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_caster->CastSpell(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, spellInfo, true, m_CastItem, NULL, m_originalCasterGUID, m_spellInfo);
    }
    else if (unitTarget)
    {
        m_caster->CastSpell(unitTarget, spellInfo, true, m_CastItem, NULL, m_originalCasterGUID, m_spellInfo);
    }
}

void Spell::EffectJump(SpellEffectEntry const* effect)
{
    if (m_caster->IsTaxiFlying())
    {
        return;
    }

    // Init dest coordinates
    float x, y, z, o;
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(x, y, z);

        if (effect->EffectImplicitTargetA == TARGET_BEHIND_VICTIM)
        {
            // explicit cast data from client or server-side cast
            // some spell at client send caster
            Unit* pTarget = NULL;
            if (m_targets.getUnitTarget() && m_targets.getUnitTarget() != m_caster)
            {
                pTarget = m_targets.getUnitTarget();
            }
            else if (unitTarget->getVictim())
            {
                pTarget = m_caster->getVictim();
            }
            else if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                pTarget = m_caster->GetMap()->GetUnit(((Player*)m_caster)->GetSelectionGuid());
            }

            o = pTarget ? pTarget->GetOrientation() : m_caster->GetOrientation();
        }
        else
        {
            o = m_caster->GetOrientation();
        }
    }
    else if (unitTarget)
    {
        unitTarget->GetContactPoint(m_caster, x, y, z, CONTACT_DISTANCE);
        o = m_caster->GetOrientation();
    }
    else if (gameObjTarget)
    {
        gameObjTarget->GetContactPoint(m_caster, x, y, z, CONTACT_DISTANCE);
        o = m_caster->GetOrientation();
    }
    else
    {
        sLog.outError("Spell::EffectJump - unsupported target mode for spell ID %u", m_spellInfo->Id);
        return;
    }

    // Try to normalize Z coord because GetContactPoint do nothing with Z axis
    m_caster->UpdateAllowedPositionZ(x, y, z);

    float speed = m_spellInfo->speed ? m_spellInfo->speed : 27.0f;
    m_caster->GetMotionMaster()->MoveDestination(x, y, z, o, speed, 2.5f);
}

void Spell::EffectTeleportUnits(SpellEffectEntry const* effect)   // TODO - Use target settings for this effect!
{
    if (!unitTarget || unitTarget->IsTaxiFlying())
    {
        return;
    }

        switch (m_spellInfo->Id)
        {
            case 48129:                                 // Scroll of Recall
            case 60320:                                 // Scroll of Recall II
            case 60321:                                 // Scroll of Recall III
            {
                uint32 failAtLevel = 0;
                switch (m_spellInfo->Id)
                {
                    case 48129: failAtLevel = 40; break;
                    case 60320: failAtLevel = 70; break;
                    case 60321: failAtLevel = 80; break;
                }

                if (unitTarget->getLevel() > failAtLevel && unitTarget->GetTypeId() == TYPEID_PLAYER)
                {
                    unitTarget->CastSpell(unitTarget, 60444, true);
                    // TODO: Unclear use of probably related spell 60322
                    uint32 spellId = (((Player*)unitTarget)->GetTeam() == ALLIANCE ? 60323 : 60328) + urand(0, 7);
                    unitTarget->CastSpell(unitTarget, spellId, true);
                    return;
                }
                break;
            }
        }

    // Target dependend on TargetB, if there is none provided, decide dependend on A
    uint32 targetType = effect->EffectImplicitTargetB;
    if (!targetType)
    {
        targetType = effect->EffectImplicitTargetA;
    }

    switch (targetType)
    {
        case TARGET_INNKEEPER_COORDINATES:
        {
            // Only players can teleport to innkeeper
            if (unitTarget->GetTypeId() != TYPEID_PLAYER)
            {
                return;
            }

            ((Player*)unitTarget)->TeleportToHomebind(unitTarget == m_caster ? TELE_TO_SPELL : 0);
            return;
        }
        case TARGET_AREAEFFECT_INSTANT:                     // in all cases first TARGET_TABLE_X_Y_Z_COORDINATES
        case TARGET_TABLE_X_Y_Z_COORDINATES:
        {
            SpellTargetPosition const* st = sSpellMgr.GetSpellTargetPosition(m_spellInfo->Id);
            if (!st)
            {
                sLog.outError("Spell::EffectTeleportUnits - unknown Teleport coordinates for spell ID %u", m_spellInfo->Id);
                return;
            }

            if (st->target_mapId == unitTarget->GetMapId())
            {
                unitTarget->NearTeleportTo(st->target_X, st->target_Y, st->target_Z, st->target_Orientation, unitTarget == m_caster);
            }
            else if (unitTarget->GetTypeId() == TYPEID_PLAYER)
            {
                ((Player*)unitTarget)->TeleportTo(st->target_mapId, st->target_X, st->target_Y, st->target_Z, st->target_Orientation, unitTarget == m_caster ? TELE_TO_SPELL : 0);
            }
            break;
        }
        case TARGET_EFFECT_SELECT:
        {
            // m_destN filled, but sometimes for wrong dest and does not have TARGET_FLAG_DEST_LOCATION

            float x = unitTarget->GetPositionX();
            float y = unitTarget->GetPositionY();
            float z = unitTarget->GetPositionZ();
            float orientation = m_caster->GetOrientation();

            m_caster->NearTeleportTo(x, y, z, orientation, unitTarget == m_caster);
            return;
        }
        case TARGET_BEHIND_VICTIM:
        {
            Unit* pTarget = NULL;

            // explicit cast data from client or server-side cast
            // some spell at client send caster
            if (m_targets.getUnitTarget() && m_targets.getUnitTarget() != unitTarget)
            {
                pTarget = m_targets.getUnitTarget();
            }
            else if (unitTarget->getVictim())
            {
                pTarget = unitTarget->getVictim();
            }
            else if (unitTarget->GetTypeId() == TYPEID_PLAYER)
            {
                pTarget = unitTarget->GetMap()->GetUnit(((Player*)unitTarget)->GetSelectionGuid());
            }

            // Init dest coordinates
            float x = m_targets.m_destX;
            float y = m_targets.m_destY;
            float z = m_targets.m_destZ;
            float orientation = pTarget ? pTarget->GetOrientation() : unitTarget->GetOrientation();
            unitTarget->NearTeleportTo(x, y, z, orientation, unitTarget == m_caster);
            return;
        }
        default:
        {
            // If not exist data for dest location - return
            if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
            {
                sLog.outError( "Spell::EffectTeleportUnits - unknown EffectImplicitTargetB[%u] = %u for spell ID %u", effect->EffectIndex, effect->EffectImplicitTargetB, m_spellInfo->Id );
                return;
            }
            // Init dest coordinates
            float x = m_targets.m_destX;
            float y = m_targets.m_destY;
            float z = m_targets.m_destZ;
            float orientation = unitTarget->GetOrientation();
            // Teleport
            unitTarget->NearTeleportTo(x, y, z, orientation, unitTarget == m_caster);
            return;
        }
    }

    // post effects for TARGET_TABLE_X_Y_Z_COORDINATES
    switch (m_spellInfo->Id)
    {
            // Dimensional Ripper - Everlook
        case 23442:
        {
            int32 r = irand(0, 119);
            if (r >= 70)                                    // 7/12 success
            {
                if (r < 100)                                // 4/12 evil twin
                {
                    m_caster->CastSpell(m_caster, 23445, true);
                }
                else                                        // 1/12 fire
                {
                    m_caster->CastSpell(m_caster, 23449, true);
                }
            }
            return;
        }
        // Ultrasafe Transporter: Toshley's Station
        case 36941:
        {
            if (roll_chance_i(50))                          // 50% success
            {
                int32 rand_eff = urand(1, 7);
                switch (rand_eff)
                {
                    case 1:
                        // soul split - evil
                        m_caster->CastSpell(m_caster, 36900, true);
                        break;
                    case 2:
                        // soul split - good
                        m_caster->CastSpell(m_caster, 36901, true);
                        break;
                    case 3:
                        // Increase the size
                        m_caster->CastSpell(m_caster, 36895, true);
                        break;
                    case 4:
                        // Decrease the size
                        m_caster->CastSpell(m_caster, 36893, true);
                        break;
                    case 5:
                        // Transform
                    {
                        if (((Player*)m_caster)->GetTeam() == ALLIANCE)
                        {
                            m_caster->CastSpell(m_caster, 36897, true);
                        }
                        else
                        {
                            m_caster->CastSpell(m_caster, 36899, true);
                        }
                        break;
                    }
                    case 6:
                        // chicken
                        m_caster->CastSpell(m_caster, 36940, true);
                        break;
                    case 7:
                        // evil twin
                        m_caster->CastSpell(m_caster, 23445, true);
                        break;
                }
            }
            return;
        }
        // Dimensional Ripper - Area 52
        case 36890:
        {
            if (roll_chance_i(50))                          // 50% success
            {
                int32 rand_eff = urand(1, 4);
                switch (rand_eff)
                {
                    case 1:
                        // soul split - evil
                        m_caster->CastSpell(m_caster, 36900, true);
                        break;
                    case 2:
                        // soul split - good
                        m_caster->CastSpell(m_caster, 36901, true);
                        break;
                    case 3:
                        // Increase the size
                        m_caster->CastSpell(m_caster, 36895, true);
                        break;
                    case 4:
                        // Transform
                    {
                        if (((Player*)m_caster)->GetTeam() == ALLIANCE)
                        {
                            m_caster->CastSpell(m_caster, 36897, true);
                        }
                        else
                        {
                            m_caster->CastSpell(m_caster, 36899, true);
                        }
                        break;
                    }
                }
            }
            return;
        }
    }
}

/**
 * @brief Creates and attaches an aura effect to the current unit target.
 *
 * @param effect The aura effect index.
 */
void Spell::EffectApplyAura(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    // ghost spell check, allow apply any auras at player loading in ghost mode (will be cleanup after load)
    if ((!unitTarget->IsAlive() && !(IsDeathOnlySpell(m_spellInfo) || IsDeathPersistentSpell(m_spellInfo))) &&
            (unitTarget->GetTypeId() != TYPEID_PLAYER || !((Player*)unitTarget)->GetSession()->PlayerLoading()))
        return;

    Unit* caster = GetAffectiveCaster();
    if (!caster)
    {
        // FIXME: currently we can't have auras applied explicitly by gameobjects
        // so for auras from wild gameobjects (no owner) target used
        if (m_originalCasterGUID.IsGameObject())
        {
            caster = unitTarget;
        }
        else
        {
            return;
        }
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell: Aura is: %u", effect->EffectApplyAuraName);

    Aura* aur = CreateAura(m_spellInfo, SpellEffectIndex(effect->EffectIndex), &m_currentBasePoints[effect->EffectIndex], m_spellAuraHolder, unitTarget, caster, m_CastItem);
    m_spellAuraHolder->AddAura(aur, SpellEffectIndex(effect->EffectIndex));
}

void Spell::EffectUnlearnSpecialization(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player *_player = (Player*)unitTarget;
    uint32 spellToUnlearn = effect->EffectTriggerSpell;

    _player->removeSpell(spellToUnlearn);

    if (WorldObject const* caster = GetCastingObject())
    {
        DEBUG_LOG("Spell: %s has unlearned spell %u at %s", _player->GetGuidStr().c_str(), spellToUnlearn, caster->GetGuidStr().c_str());
    }
}

/**
 * @brief Drains power from the unit target and optionally restores mana to the caster.
 *
 * @param effect The effect index defining the drained power type.
 */
void Spell::EffectPowerDrain(SpellEffectEntry const* effect)
{
    if (effect->EffectMiscValue < 0 || effect->EffectMiscValue >= MAX_POWERS)
    {
        return;
    }

    Powers drain_power = Powers(effect->EffectMiscValue);

    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }
    if (unitTarget->GetPowerType() != drain_power)
    {
        return;
    }
    if (damage < 0)
    {
        return;
    }

    uint32 curPower = unitTarget->GetPower(drain_power);

    // add spell damage bonus
    damage = m_caster->SpellDamageBonusDone(unitTarget, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);
    damage = unitTarget->SpellDamageBonusTaken(m_caster, m_spellInfo, uint32(damage), SPELL_DIRECT_DAMAGE);

    // Cata 4.0.1: TBC-era crit-resilience mana-drain reduction removed
    // (resilience no longer has a separate crit-specific path).
    uint32 power = damage;

    int32 new_damage;
    if (curPower < power)
    {
        new_damage = curPower;
    }
    else
    {
        new_damage = power;
    }

    unitTarget->ModifyPower(drain_power, -new_damage);

    // Don`t restore from self drain
    if (drain_power == POWER_MANA && m_caster != unitTarget)
    {
        float manaMultiplier = effect->EffectMultipleValue;
        if (manaMultiplier==0)
        {
            manaMultiplier = 1;
        }

        if (Player* modOwner = m_caster->GetSpellModOwner())
        {
            modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_MULTIPLE_VALUE, manaMultiplier);
        }

        int32 gain = int32(new_damage * manaMultiplier);

        m_caster->EnergizeBySpell(m_caster, m_spellInfo->Id, gain, POWER_MANA);
    }
}

/**
 * @brief Starts a scripted event defined by the spell effect.
 *
 * @param effect The effect index providing the event identifier.
 */
void Spell::EffectSendEvent(SpellEffectEntry const* effect)
{
    /*
    we do not handle a flag dropping or clicking on flag in battleground by sendevent system
    TODO: Actually, why not...
    */
    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "Spell ScriptStart %u for spellid %u in EffectSendEvent ", effect->EffectMiscValue, m_spellInfo->Id);
    StartEvents_Event(m_caster->GetMap(), effect->EffectMiscValue, m_caster, focusObject, true, m_caster);
}

/**
 * @brief Burns target power and converts it into spell damage.
 *
 * @param effect The effect index defining the burned power type.
 */
void Spell::EffectPowerBurn(SpellEffectEntry const* effect)
{
    if (effect->EffectMiscValue < 0 || effect->EffectMiscValue >= MAX_POWERS)
    {
        return;
    }

    Powers powertype = Powers(effect->EffectMiscValue);

    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }
    if (unitTarget->GetPowerType() != powertype)
    {
        return;
    }
    if (damage < 0)
    {
        return;
    }

    // burn x% of target's mana, up to maximum of 2x% of caster's mana (Mana Burn)
    if (m_spellInfo->GetManaCostPercentage())
    {
        int32 maxdamage = m_caster->GetMaxPower(powertype) * damage * 2 / 100;
        damage = unitTarget->GetMaxPower(powertype) * damage / 100;
        if (damage > maxdamage)
        {
            damage = maxdamage;
        }
    }

    int32 curPower = int32(unitTarget->GetPower(powertype));

    // Cata 4.0.1: TBC-era crit-resilience mana-drain reduction removed
    // (resilience no longer has a separate crit-specific path).
    int32 power = damage;

    int32 new_damage = (curPower < power) ? curPower : power;

    unitTarget->ModifyPower(powertype, -new_damage);
    float multiplier = effect->EffectMultipleValue;

    if (Player* modOwner = m_caster->GetSpellModOwner())
    {
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_MULTIPLE_VALUE, multiplier);
    }

    new_damage = int32(new_damage * multiplier);
    m_damage += new_damage;
}

/**
 * @brief Accumulates healing for the current unit target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectHeal(SpellEffectEntry const* /*effect*/)
{
    if (unitTarget && unitTarget->IsAlive() && damage >= 0)
    {
        // Try to get original caster
        Unit* caster = GetAffectiveCaster();
        if (!caster)
        {
            return;
        }

        int32 addhealth = damage;

        // Seal of Light proc
        if (m_spellInfo->Id == 20167)
        {
            float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
            int32 holy = caster->SpellBaseHealingBonusDone(GetSpellSchoolMask(m_spellInfo));
            if (holy < 0)
            {
                holy = 0;
            }
            addhealth += int32(ap * 0.15) + int32(holy * 15 / 100);
        }
        // Vessel of the Naaru (Vial of the Sunwell trinket)
        else if (m_spellInfo->Id == 45064)
        {
            // Amount of heal - depends from stacked Holy Energy
            int damageAmount = 0;
            Unit::AuraList const& mDummyAuras = m_caster->GetAurasByType(SPELL_AURA_DUMMY);
            for (Unit::AuraList::const_iterator i = mDummyAuras.begin(); i != mDummyAuras.end(); ++i)
                if ((*i)->GetId() == 45062)
                {
                    damageAmount += (*i)->GetModifier()->m_amount;
                }
            if (damageAmount)
            {
                m_caster->RemoveAurasDueToSpell(45062);
            }

            addhealth += damageAmount;
        }
        // Death Pact (percent heal)
        else if (m_spellInfo->Id == 48743)
        {
            addhealth = addhealth * unitTarget->GetMaxHealth() / 100;
        }
        // Swiftmend - consumes Regrowth or Rejuvenation
        else if (m_spellInfo->GetTargetAuraState() == AURA_STATE_SWIFTMEND && unitTarget->HasAuraState(AURA_STATE_SWIFTMEND))
        {
            Unit::AuraList const& RejorRegr = unitTarget->GetAurasByType(SPELL_AURA_PERIODIC_HEAL);
            // find most short by duration
            Aura* targetAura = NULL;
            for (Unit::AuraList::const_iterator i = RejorRegr.begin(); i != RejorRegr.end(); ++i)
            {
                SpellClassOptionsEntry const* smClassOptions = (*i)->GetSpellProto()->GetSpellClassOptions();
                if (smClassOptions && smClassOptions->SpellFamilyName == SPELLFAMILY_DRUID &&
                    // Regrowth or Rejuvenation 0x40 | 0x10
                    (smClassOptions->SpellFamilyFlags & UI64LIT(0x0000000000000050)))
                {
                    if (!targetAura || (*i)->GetAuraDuration() < targetAura->GetAuraDuration())
                    {
                        targetAura = *i;
                    }
                }
            }

            if (!targetAura)
            {
                sLog.outError("Target (GUID: %u TypeId: %u) has aurastate AURA_STATE_SWIFTMEND but no matching aura.", unitTarget->GetGUIDLow(), unitTarget->GetTypeId());
                return;
            }
            int idx = 0;
            SpellEffectEntry const* targetSpellEffect = NULL;
            while(idx < 3)
            {
                targetSpellEffect = targetAura->GetSpellProto()->GetSpellEffect(SpellEffectIndex(idx));
                if (targetSpellEffect && targetSpellEffect->EffectApplyAuraName == SPELL_AURA_PERIODIC_HEAL)
                {
                    break;
                }
                ++idx;
            }

            int32 tickheal = targetAura->GetModifier()->m_amount;
            int32 tickcount = GetSpellDuration(targetAura->GetSpellProto()) / (targetSpellEffect ? targetSpellEffect->EffectAmplitude : 1) - 1;

            // Glyph of Swiftmend
            if (!caster->HasAura(54824))
            {
                unitTarget->RemoveAurasDueToSpell(targetAura->GetId());
            }

            addhealth += tickheal * tickcount;
        }
        // Runic Healing Injector & Healing Potion Injector effect increase for engineers
        else if ((m_spellInfo->Id == 67486 || m_spellInfo->Id == 67489) && unitTarget->GetTypeId() == TYPEID_PLAYER)
        {
            Player* player = (Player*)unitTarget;
            if (player->HasSkill(SKILL_ENGINEERING))
            {
                addhealth += int32(addhealth * 0.25);
            }
        }

        // Chain Healing
        SpellClassOptionsEntry const* chClassOptions = m_spellInfo->GetSpellClassOptions();
        if (chClassOptions && chClassOptions->SpellFamilyName == SPELLFAMILY_SHAMAN && chClassOptions->SpellFamilyFlags & UI64LIT(0x0000000000000100))
        {
            if (unitTarget == m_targets.getUnitTarget())
            {
                // check for Riptide
                Aura* riptide = unitTarget->GetAura(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_SHAMAN, UI64LIT(0x0), 0x00000010, caster->GetObjectGuid());
                if (riptide)
                {
                    addhealth += addhealth / 4;
                    unitTarget->RemoveAurasDueToSpell(riptide->GetId());
                }
            }
        }

        addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);
        addhealth = unitTarget->SpellHealingBonusTaken(caster, m_spellInfo, addhealth, HEAL);

        m_healing += addhealth;
    }
}

void Spell::EffectHealPct(SpellEffectEntry const* /*effect*/)
{
    if (unitTarget && unitTarget->IsAlive() && damage >= 0)
    {
        // Try to get original caster
        Unit* caster = GetAffectiveCaster();
        if (!caster)
        {
            return;
        }

        uint32 addhealth = unitTarget->GetMaxHealth() * damage / 100;

        addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, addhealth, HEAL);
        addhealth = unitTarget->SpellHealingBonusTaken(caster, m_spellInfo, addhealth, HEAL);

        uint32 absorb = 0;
        unitTarget->CalculateHealAbsorb(addhealth, &absorb);

        int32 gain = caster->DealHeal(unitTarget, addhealth - absorb, m_spellInfo, false, absorb);
        unitTarget->GetHostileRefManager().threatAssist(caster, float(gain) * 0.5f * sSpellMgr.GetSpellThreatMultiplier(m_spellInfo), m_spellInfo);
    }
}

/**
 * @brief Heals a mechanical target immediately.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectHealMechanical(SpellEffectEntry const* /*effect*/)
{
    // Mechanic creature type should be correctly checked by targetCreatureType field
    if (unitTarget && unitTarget->IsAlive() && damage >= 0)
    {
        // Try to get original caster
        Unit* caster = GetAffectiveCaster();
        if (!caster)
        {
            return;
        }

        uint32 addhealth = caster->SpellHealingBonusDone(unitTarget, m_spellInfo, damage, HEAL);
        addhealth = unitTarget->SpellHealingBonusTaken(caster, m_spellInfo, addhealth, HEAL);

        uint32 absorb = 0;
        unitTarget->CalculateHealAbsorb(addhealth, &absorb);

        caster->DealHeal(unitTarget, addhealth - absorb, m_spellInfo, false, absorb);
    }
}

/**
 * @brief Damages the target and heals the caster for a portion of the damage dealt.
 *
 * @param effect The effect index providing the leech multiplier.
 */
void Spell::EffectHealthLeech(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    if (damage < 0)
    {
        return;
    }

    DEBUG_FILTER_LOG(LOG_FILTER_SPELL_CAST, "HealthLeech :%i", damage);

    uint32 curHealth = unitTarget->GetHealth();
    damage = m_caster->SpellNonMeleeDamageLog(unitTarget, m_spellInfo->Id, damage);
    if ((int32)curHealth < damage)
    {
        damage = curHealth;
    }

    float multiplier = effect->EffectMultipleValue;

    if (Player* modOwner = m_caster->GetSpellModOwner())
    {
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_MULTIPLE_VALUE, multiplier);
    }

    int32 heal = int32(damage * multiplier);
    if (m_caster->IsAlive())
    {
        heal = m_caster->SpellHealingBonusTaken(m_caster, m_spellInfo, heal, HEAL);

        uint32 absorb = 0;
        m_caster->CalculateHealAbsorb(heal, &absorb);

        m_caster->DealHeal(m_caster, heal - absorb, m_spellInfo, false, absorb);
    }
}

/**
 * @brief Creates an item and stores it in the player's inventory.
 *
 * @param effect The effect index creating the item.
 * @param itemtype The item entry to create.
 */
void Spell::DoCreateItem(SpellEffectEntry const* effect, uint32 itemtype)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)unitTarget;

    uint32 newitemid = itemtype;
    ItemPrototype const* pProto = ObjectMgr::GetItemPrototype(newitemid);
    if (!pProto)
    {
        player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    // bg reward have some special in code work
    bool bg_mark = false;
    switch (m_spellInfo->Id)
    {
        case SPELL_WG_MARK_VICTORY:
        case SPELL_WG_MARK_DEFEAT:
            bg_mark = true;
            break;
        default:
            break;
    }

    uint32 num_to_add = damage;

    if (num_to_add < 1)
    {
        num_to_add = 1;
    }
    if (num_to_add > pProto->GetMaxStackSize())
    {
        num_to_add = pProto->GetMaxStackSize();
    }

    // init items_count to 1, since 1 item will be created regardless of specialization
    int items_count = 1;
    // the chance to create additional items
    float additionalCreateChance = 0.0f;
    // the maximum number of created additional items
    uint8 additionalMaxNum = 0;
    // get the chance and maximum number for creating extra items
    if (canCreateExtraItems(player, m_spellInfo->Id, additionalCreateChance, additionalMaxNum))
    {
        // roll with this chance till we roll not to create or we create the max num
        while (roll_chance_f(additionalCreateChance) && items_count <= additionalMaxNum)
            ++items_count;
    }

    // really will be created more items
    num_to_add *= items_count;

    // can the player store the new item?
    ItemPosCountVec dest;
    uint32 no_space = 0;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, newitemid, num_to_add, &no_space);
    if (msg != EQUIP_ERR_OK)
    {
        // convert to possible store amount
        if (msg == EQUIP_ERR_INVENTORY_FULL || msg == EQUIP_ERR_CANT_CARRY_MORE_OF_THIS)
        {
            num_to_add -= no_space;
        }
        else
        {
            // ignore mana gem case (next effect will recharge existing example)
            if (effect->EffectIndex == EFFECT_INDEX_0 && m_spellInfo->GetSpellEffectIdByIndex(EFFECT_INDEX_1) == SPELL_EFFECT_DUMMY )
            {
                return;
            }

            // if not created by another reason from full inventory or unique items amount limitation
            player->SendEquipError(msg, NULL, NULL, newitemid);
            return;
        }
    }

    if (num_to_add)
    {
        // create the new item and store it
        Item* pItem = player->StoreNewItem(dest, newitemid, true, Item::GenerateItemRandomPropertyId(newitemid));

        // was it successful? return error if not
        if (!pItem)
        {
            player->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
            return;
        }

        // set the "Crafted by ..." property of the item
        if (pItem->GetProto()->Class != ITEM_CLASS_CONSUMABLE && pItem->GetProto()->Class != ITEM_CLASS_QUEST)
        {
            pItem->SetGuidValue(ITEM_FIELD_CREATOR, player->GetObjectGuid());
        }

        // send info to the client
        player->SendNewItem(pItem, num_to_add, true, !bg_mark);

        // we succeeded in creating at least one item, so a levelup is possible
        if (!bg_mark)
        {
            player->UpdateCraftSkill(m_spellInfo->Id);
        }
    }
}

/**
 * @brief Handles item-creation effects and related special target cleanup.
 *
 * @param effect The effect index creating the item.
 */
void Spell::EffectCreateItem(SpellEffectEntry const* effect)
{
    DoCreateItem(effect, effect->EffectItemType);
}

void Spell::EffectCreateItem2(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    Player* player = (Player*)m_caster;

    // explicit item (possible fake)
    uint32 item_id = effect->EffectItemType;

    if (item_id)
    {
        DoCreateItem(effect, item_id);
    }

    // not explicit loot (with fake item drop if need)
    if (IsLootCraftingSpell(m_spellInfo))
    {
        if (item_id)
        {
            if (!player->HasItemCount(item_id, 1))
            {
                return;
            }

            // remove reagent
            uint32 count = 1;
            player->DestroyItemCount(item_id, count, true);
        }

        // create some random items
        player->AutoStoreLoot(NULL, m_spellInfo->Id, LootTemplates_Spell);
    }
}

void Spell::EffectCreateRandomItem(SpellEffectEntry const* /*effect*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    Player* player = (Player*)m_caster;

    // create some random items
    player->AutoStoreLoot(NULL, m_spellInfo->Id, LootTemplates_Spell);
}

/**
 * @brief Creates a persistent area aura dynamic object.
 *
 * @param effect The persistent area aura effect index.
 */
void Spell::EffectPersistentAA(SpellEffectEntry const* effect)
{
    Unit* pCaster = GetAffectiveCaster();
    // FIXME: in case wild GO will used wrong affective caster (target in fact) as dynobject owner
    if (!pCaster)
    {
        pCaster = m_caster;
    }

    float radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(effect->GetRadiusIndex()));

    if (Player* modOwner = pCaster->GetSpellModOwner())
    {
        modOwner->ApplySpellMod(m_spellInfo->Id, SPELLMOD_RADIUS, radius);
    }

    DynamicObject* dynObj = new DynamicObject;
    if (!dynObj->Create(pCaster->GetMap()->GenerateLocalLowGuid(HIGHGUID_DYNAMICOBJECT), pCaster, m_spellInfo->Id, SpellEffectIndex(effect->EffectIndex), m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, m_duration, radius, DYNAMIC_OBJECT_AREA_SPELL))
    {
        delete dynObj;
        return;
    }

    pCaster->AddDynObject(dynObj);
    pCaster->GetMap()->Add(dynObj);
}

/**
 * @brief Restores power to the current unit target.
 *
 * @param effect The effect index defining the power type.
 */
void Spell::EffectEnergize(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    if (effect->EffectMiscValue < 0 || effect->EffectMiscValue >= MAX_POWERS)
    {
        return;
    }

    Powers power = Powers(effect->EffectMiscValue);

    // Some level depends spells
    int level_multiplier = 0;
    int level_diff = 0;
    switch (m_spellInfo->Id)
    {
        case 9512:                                          // Restore Energy
            level_diff = m_caster->getLevel() - 40;
            level_multiplier = 2;
            break;
        case 24571:                                         // Blood Fury
            level_diff = m_caster->getLevel() - 60;
            level_multiplier = 10;
            break;
        case 24532:                                         // Burst of Energy
            level_diff = m_caster->getLevel() - 60;
            level_multiplier = 4;
            break;
        case 31930:                                         // Judgements of the Wise
        case 48542:                                         // Revitalize (mana restore case)
        case 63375:                                         // Improved Stormstrike
        case 68082:                                         // Glyph of Seal of Command
            damage = damage * unitTarget->GetCreateMana() / 100;
            break;
        case 67487:                                         // Mana Potion Injector
        case 67490:                                         // Runic Mana Injector
        {
            if (unitTarget->GetTypeId() == TYPEID_PLAYER)
            {
                Player* player = (Player*)unitTarget;
                if (player->HasSkill(SKILL_ENGINEERING))
                {
                    damage += int32(damage * 0.25);
                }
            }
            break;
        }
        default:
            break;
    }

    if (level_diff > 0)
    {
        damage -= level_multiplier * level_diff;
    }

    if (damage < 0)
    {
        return;
    }

    if (unitTarget->GetMaxPower(power) == 0)
    {
        return;
    }

    m_caster->EnergizeBySpell(unitTarget, m_spellInfo->Id, damage, power);

    // Mad Alchemist's Potion
    if (m_spellInfo->Id == 45051)
    {
        // find elixirs on target
        uint32 elixir_mask = 0;
        Unit::SpellAuraHolderMap& Auras = unitTarget->GetSpellAuraHolderMap();
        for (Unit::SpellAuraHolderMap::iterator itr = Auras.begin(); itr != Auras.end(); ++itr)
        {
            uint32 spell_id = itr->second->GetId();
            if (uint32 mask = sSpellMgr.GetSpellElixirMask(spell_id))
            {
                elixir_mask |= mask;
            }
        }

        // get available elixir mask any not active type from battle/guardian (and flask if no any)
        elixir_mask = (elixir_mask & ELIXIR_FLASK_MASK) ^ ELIXIR_FLASK_MASK;

        // get all available elixirs by mask and spell level
        std::vector<uint32> elixirs;
        SpellElixirMap const& m_spellElixirs = sSpellMgr.GetSpellElixirMap();
        for (SpellElixirMap::const_iterator itr = m_spellElixirs.begin(); itr != m_spellElixirs.end(); ++itr)
        {
            if (itr->second & elixir_mask)
            {
                if (itr->second & (ELIXIR_UNSTABLE_MASK | ELIXIR_SHATTRATH_MASK))
                {
                    continue;
                }

                SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                if (spellInfo && (spellInfo->GetSpellLevel() < m_spellInfo->GetSpellLevel() || spellInfo->GetSpellLevel() > unitTarget->getLevel()))
                {
                    continue;
                }

                elixirs.push_back(itr->first);
            }
        }

        if (!elixirs.empty())
        {
            // cast random elixir on target
            uint32 rand_spell = urand(0, elixirs.size() - 1);
            m_caster->CastSpell(unitTarget, elixirs[rand_spell], true, m_CastItem);
        }
    }
}

void Spell::EffectEnergisePct(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    if (effect->EffectMiscValue < 0 || effect->EffectMiscValue >= MAX_POWERS)
    {
        return;
    }

    Powers power = Powers(effect->EffectMiscValue);

    uint32 maxPower = unitTarget->GetMaxPower(power);
    if (maxPower == 0)
    {
        return;
    }

    uint32 gain = damage * maxPower / 100;
    m_caster->EnergizeBySpell(unitTarget, m_spellInfo->Id, gain, power);
}

/**
 * @brief Opens or sends loot for the specified object guid.
 *
 * @param guid The loot source guid.
 * @param loottype The loot type to open.
 * @param lockType The lock interaction type.
 */
void Spell::SendLoot(ObjectGuid guid, LootType loottype, LockType lockType)
{
    if (gameObjTarget)
    {
        switch (gameObjTarget->GetGoType())
        {
            case GAMEOBJECT_TYPE_DOOR:
            case GAMEOBJECT_TYPE_BUTTON:
            case GAMEOBJECT_TYPE_QUESTGIVER:
            case GAMEOBJECT_TYPE_SPELL_FOCUS:
            case GAMEOBJECT_TYPE_GOOBER:
                gameObjTarget->Use(m_caster);
                return;

            case GAMEOBJECT_TYPE_CHEST:
                gameObjTarget->Use(m_caster);
                // Don't return, let loots been taken
                break;

            case GAMEOBJECT_TYPE_TRAP:
                if (lockType == LOCKTYPE_DISARM_TRAP)
                {
                    gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
                    return;
                }
                sLog.outError("Spell::SendLoot unhandled locktype %u for GameObject trap (entry %u) for spell %u.", lockType, gameObjTarget->GetEntry(), m_spellInfo->Id);
                return;
            default:
                sLog.outError("Spell::SendLoot unhandled GameObject type %u (entry %u) for spell %u.", gameObjTarget->GetGoType(), gameObjTarget->GetEntry(), m_spellInfo->Id);
                return;
        }
    }

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Send loot
    ((Player*)m_caster)->SendLoot(guid, loottype);
}

/**
 * @brief Opens a locked game object or item and awards related skill progress.
 *
 * @param effect The open-lock effect index.
 */
void Spell::EffectOpenLock(SpellEffectEntry const* effect)
{
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        DEBUG_LOG("WORLD: Open Lock - No Player Caster!");
        return;
    }

    Player* player = (Player*)m_caster;

    uint32 lockId = 0;
    ObjectGuid guid;

    // Get lockId
    if (gameObjTarget)
    {
        GameObjectInfo const* goInfo = gameObjTarget->GetGOInfo();
        // Arathi Basin banner opening !
        if ((goInfo->type == GAMEOBJECT_TYPE_BUTTON && goInfo->button.noDamageImmune) ||
                (goInfo->type == GAMEOBJECT_TYPE_GOOBER && goInfo->goober.losOK))
        {
            // CanUseBattleGroundObject() already called in CheckCast()
            // in battleground check
            if (BattleGround* bg = player->GetBattleGround())
            {
                // check if it's correct bg
                if (bg->GetTypeID() == BATTLEGROUND_AB || bg->GetTypeID() == BATTLEGROUND_AV)
                {
                    bg->EventPlayerClickedOnFlag(player, gameObjTarget);
                }
                return;
            }
        }
        else if (goInfo->type == GAMEOBJECT_TYPE_FLAGSTAND)
        {
            // CanUseBattleGroundObject() already called in CheckCast()
            // in battleground check
            if (BattleGround* bg = player->GetBattleGround())
            {
                if (bg->GetTypeID() == BATTLEGROUND_EY)
                {
                    bg->EventPlayerClickedOnFlag(player, gameObjTarget);
                }
                return;
            }
        }
        lockId = goInfo->GetLockId();
        guid = gameObjTarget->GetObjectGuid();
    }
    else if (itemTarget)
    {
        lockId = itemTarget->GetProto()->LockID;
        guid = itemTarget->GetObjectGuid();
    }
    else
    {
        DEBUG_LOG("WORLD: Open Lock - No GameObject/Item Target!");
        return;
    }

    SkillType skillId = SKILL_NONE;
    int32 reqSkillValue = 0;
    int32 skillValue;

    SpellCastResult res = CanOpenLock(SpellEffectIndex(effect->EffectIndex), lockId, skillId, reqSkillValue, skillValue);
    if (res != SPELL_CAST_OK)
    {
        SendCastResult(res);
        return;
    }

    // mark item as unlocked
    if (itemTarget)
    {
        itemTarget->SetFlag(ITEM_FIELD_FLAGS, ITEM_DYNFLAG_UNLOCKED);
    }

    SendLoot(guid, LOOT_SKINNING, LockType(effect->EffectMiscValue));

    // not allow use skill grow at item base open
    if (!m_CastItem && skillId != SKILL_NONE)
    {
        // update skill if really known
        if (uint32 pureSkillValue = player->GetPureSkillValue(skillId))
        {
            if (gameObjTarget)
            {
                // Allow one skill-up until respawned
                if (!gameObjTarget->IsInSkillupList(player) &&
                        player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue))
                    gameObjTarget->AddToSkillupList(player);
            }
            else if (itemTarget)
            {
                // Do one skill-up
                player->UpdateGatherSkill(skillId, pureSkillValue, reqSkillValue);
            }
        }
    }
}

/**
 * @brief Replaces the cast item with another item entry.
 *
 * @param effect The effect index defining the replacement item.
 */
void Spell::EffectSummonChangeItem(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)m_caster;

    // applied only to using item
    if (!m_CastItem)
    {
        return;
    }

    // ... only to item in own inventory/bank/equip_slot
    if (m_CastItem->GetOwnerGuid() != player->GetObjectGuid())
    {
        return;
    }

    uint32 newitemid = effect->EffectItemType;
    if (!newitemid)
    {
        return;
    }

    Item* oldItem = m_CastItem;

    // prevent crash at access and unexpected charges counting with item update queue corrupt
    ClearCastItem();

    player->ConvertItem(oldItem, newitemid);
}

/**
 * @brief Grants weapon or armor proficiency to a player target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectProficiency(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    Player *p_target = (Player*)unitTarget;

    SpellEquippedItemsEntry const* eqItems = m_spellInfo->GetSpellEquippedItems();

    if (!eqItems)
    {
        return;
    }

    if (eqItems->EquippedItemClass == ITEM_CLASS_WEAPON && !(p_target->GetWeaponProficiency() & eqItems->EquippedItemSubClassMask))
    {
        p_target->AddWeaponProficiency(eqItems->EquippedItemSubClassMask);
        p_target->SendProficiency(ITEM_CLASS_WEAPON, p_target->GetWeaponProficiency());
    }
    if (eqItems->EquippedItemClass == ITEM_CLASS_ARMOR && !(p_target->GetArmorProficiency() & eqItems->EquippedItemSubClassMask))
    {
        p_target->AddArmorProficiency(eqItems->EquippedItemSubClassMask);
        p_target->SendProficiency(ITEM_CLASS_ARMOR, p_target->GetArmorProficiency());
    }
}

/**
 * @brief Creates and attaches an area aura for the current unit target.
 *
 * @param effect The area aura effect index.
 */
void Spell::EffectApplyAreaAura(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    AreaAura* Aur = new AreaAura(m_spellInfo, SpellEffectIndex(effect->EffectIndex), &m_currentBasePoints[effect->EffectIndex], m_spellAuraHolder, unitTarget, m_caster, m_CastItem);
    m_spellAuraHolder->AddAura(Aur, SpellEffectIndex(effect->EffectIndex));
}

void Spell::EffectSummonType(SpellEffectEntry const* effect)
{
    // if this spell already have an aura applied cancel the summon
    if (m_caster->HasAura(m_spellInfo->Id))
    {
        return;
    }

    uint32 prop_id = effect->EffectMiscValueB;
    SummonPropertiesEntry const *summon_prop = sSummonPropertiesStore.LookupEntry(prop_id);
    if (!summon_prop)
    {
        sLog.outError("EffectSummonType: Unhandled summon type %u", prop_id);
        return;
    }

    // Pet's are atm handled differently
    if (summon_prop->Group == SUMMON_PROP_GROUP_PETS && prop_id != 1562)
    {
        DoSummonPet(effect);
    }

    // Expected Amount: TODO - there are quite some exceptions (like totems, engineering dragonlings..)
    uint32 amount = damage > 0 ? damage : 1;

    // basepoints of SUMMON_PROP_GROUP_VEHICLE is often a spellId, set amount to 1
    if ((summon_prop->Group == SUMMON_PROP_GROUP_VEHICLE || (prop_id == 1961)) || summon_prop->Group == SUMMON_PROP_GROUP_UNCONTROLLABLE_VEHICLE || summon_prop->Group == SUMMON_PROP_GROUP_CONTROLLABLE)
    {
        amount = 1;
    }

    // Get casting object
    WorldObject* realCaster = GetCastingObject();
    if (!realCaster)
    {
        sLog.outError("EffectSummonType: No Casting Object found for spell %u, (caster = %s)", m_spellInfo->Id, m_caster->GetGuidStr().c_str());
        return;
    }

    Unit* responsibleCaster = m_originalCaster;
    if (realCaster->GetTypeId() == TYPEID_GAMEOBJECT)
    {
        responsibleCaster = ((GameObject*)realCaster)->GetOwner();
    }

    // Expected Level                                       (Totem, Pet and Critter may not use this)
    uint32 level = responsibleCaster ? responsibleCaster->getLevel() : m_caster->getLevel();
    // level of creature summoned using engineering item based at engineering skill level
    if (m_caster->GetTypeId() == TYPEID_PLAYER && m_CastItem)
    {
        ItemPrototype const* proto = m_CastItem->GetProto();
        if (proto && proto->RequiredSkill == SKILL_ENGINEERING)
            if (uint16 engineeringSkill = ((Player*)m_caster)->GetSkillValue(SKILL_ENGINEERING))
            {
                level = engineeringSkill / 5;
                amount = 1;                                 // TODO HACK (needs a neat way of doing)
            }
    }

    CreatureSummonPositions summonPositions;
    summonPositions.resize(amount, CreaturePosition());

    // Set middle position
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z);
    }
    else
    {
        realCaster->GetPosition(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z);

        // TODO - Is this really an error?
        sLog.outDebug("Spell Effect EFFECT_SUMMON (%u) - summon without destination (spell id %u, effIndex %u)", effect->Effect, m_spellInfo->Id, SpellEffectIndex(effect->EffectIndex));
    }

    // Set summon positions
    float radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(effect->GetRadiusIndex()));
    CreatureSummonPositions::iterator itr = summonPositions.begin();
    for (++itr; itr != summonPositions.end(); ++itr)        // In case of multiple summons around position for not-fist positions
    {
        if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION || radius > 1.0f)
        {
            realCaster->GetRandomPoint(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z, radius, itr->x, itr->y, itr->z);
            if (realCaster->GetMap()->GetHitPosition(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z, itr->x, itr->y, itr->z, m_caster->GetPhaseMask(), -0.5f))
            {
                realCaster->UpdateAllowedPositionZ(itr->x, itr->y, itr->z);
            }
        }
        else                                                // Get a point near the caster
        {
            realCaster->GetRandomPoint(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z, radius, itr->x, itr->y, itr->z);
            if (realCaster->GetMap()->GetHitPosition(summonPositions[0].x, summonPositions[0].y, summonPositions[0].z, itr->x, itr->y, itr->z, m_caster->GetPhaseMask(), -0.5f))
            {
                realCaster->UpdateAllowedPositionZ(itr->x, itr->y, itr->z);
            }
        }
    }

    bool summonResult = false;
    switch (summon_prop->Group)
    {
            // faction handled later on, or loaded from template
        case SUMMON_PROP_GROUP_WILD:
        case SUMMON_PROP_GROUP_FRIENDLY:
        {
            switch (summon_prop->Title)                     // better from known way sorting summons by AI types
            {
                case UNITNAME_SUMMON_TITLE_NONE:
                {
                    // those are classical totems - effectbasepoints is their hp and not summon ammount!
                    // 121: 23035, battlestands
                    // 647: 52893, Anti-Magic Zone (npc used)
                    if (prop_id == 121 || prop_id == 647)
                    {
                        summonResult = DoSummonTotem(effect);
                    }
                    else
                    {
                        summonResult = DoSummonWild(summonPositions, summon_prop, effect, level);
                    }
                    break;
                }
                case UNITNAME_SUMMON_TITLE_PET:
                case UNITNAME_SUMMON_TITLE_MINION:
                case UNITNAME_SUMMON_TITLE_RUNEBLADE:
                    summonResult = DoSummonGuardian(summonPositions, summon_prop, effect, level);
                    break;
                case UNITNAME_SUMMON_TITLE_GUARDIAN:
                {
                    if (prop_id == 61)                      // mixed guardians, totems, statues
                    {
                        // * Stone Statue, etc  -- fits much better totem AI
                        if (m_spellInfo->SpellIconID == 2056)
                        {
                            summonResult = DoSummonTotem(effect);
                        }
                        else
                        {
                            // possible sort totems/guardians only by summon creature type
                            CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(effect->EffectMiscValue);

                            if (!cInfo)
                            {
                                return;
                            }

                            // FIXME: not all totems and similar cases selected by this check...
                            if (cInfo->CreatureType == CREATURE_TYPE_TOTEM)
                            {
                                summonResult = DoSummonTotem(effect);
                            }
                            else
                            {
                                summonResult = DoSummonGuardian(summonPositions, summon_prop, effect, level);
                            }
                        }
                    }
                    else
                    {
                        summonResult = DoSummonGuardian(summonPositions, summon_prop, effect, level);
                    }
                    break;
                }
                case UNITNAME_SUMMON_TITLE_CONSTRUCT:
                {
                    if (prop_id == 2913)                    // Scrapbot
                    {
                        summonResult = DoSummonWild(summonPositions, summon_prop, effect, level);
                    }
                    else
                    {
                        summonResult = DoSummonGuardian(summonPositions, summon_prop, effect, level);
                    }
                    break;
                }
                case UNITNAME_SUMMON_TITLE_TOTEM:
                    summonResult = DoSummonTotem(effect, summon_prop->Slot);
                    break;
                case UNITNAME_SUMMON_TITLE_COMPANION:
                    // slot 6 set for critters that can help to player in fighting
                    if (summon_prop->Slot == 6)
                    {
                        summonResult = DoSummonGuardian(summonPositions, summon_prop, effect, level);
                    }
                    else
                    {
                        summonResult = DoSummonCritter(summonPositions, summon_prop, effect, level);
                    }
                    break;
                case UNITNAME_SUMMON_TITLE_OPPONENT:
                case UNITNAME_SUMMON_TITLE_VEHICLE:
                case UNITNAME_SUMMON_TITLE_MOUNT:
                case UNITNAME_SUMMON_TITLE_LIGHTWELL:
                case UNITNAME_SUMMON_TITLE_BUTLER:
                    summonResult = DoSummonWild(summonPositions, summon_prop, effect, level);
                    break;
                default:
                    sLog.outError("EffectSummonType: Unhandled summon title %u", summon_prop->Title);
                    break;
            }
            break;
        }
        case SUMMON_PROP_GROUP_PETS:
        {
            // FIXME : multiple summons -  not yet supported as pet
            // 1562 - force of nature  - sid 33831
            // 1161 - feral spirit - sid 51533
            if (prop_id == 1562)                            // 3 uncontrolable instead of one controllable :/
            {
                summonResult = DoSummonGuardian(summonPositions, summon_prop, effect, level);
            }
            break;
        }
        case SUMMON_PROP_GROUP_CONTROLLABLE:
        {
            summonResult = DoSummonPossessed(summonPositions, summon_prop, effect, level);
            break;
        }
        case SUMMON_PROP_GROUP_VEHICLE:
        case SUMMON_PROP_GROUP_UNCONTROLLABLE_VEHICLE:
        {
            summonResult = DoSummonVehicle(summonPositions, summon_prop, effect, level);
            break;
        }
        default:
            sLog.outError("EffectSummonType: Unhandled summon group type %u", summon_prop->Group);
            break;
    }

    if (!summonResult)
    {
        return;                                             // No further handling required
    }

    for (itr = summonPositions.begin(); itr != summonPositions.end(); ++itr)
    {
        MANGOS_ASSERT(itr->creature || itr != summonPositions.begin());
        if (!itr->creature)
        {
            sLog.outError("EffectSummonType: Expected to have %u NPCs summoned, but some failed (Spell id %u)", amount, m_spellInfo->Id);
            continue;
        }

        if (summon_prop->FactionId)
        {
            itr->creature->setFaction(summon_prop->FactionId);
        }
        // Else set faction to summoner's faction for pet-like summoned
        else if ((summon_prop->Flags & SUMMON_PROP_FLAG_INHERIT_FACTION) || !itr->creature->IsTemporarySummon())
        {
            itr->creature->setFaction(responsibleCaster ? responsibleCaster->getFaction() : m_caster->getFaction());
        }

        if (!itr->creature->IsTemporarySummon())
        {
            itr->creature->AIM_Initialize();

            m_caster->GetMap()->Add(itr->creature);

            // Notify Summoner
            if (m_caster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_caster)->AI())
            {
                ((Creature*)m_caster)->AI()->JustSummoned(itr->creature);
            }
            if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
            {
                ((Creature*)m_originalCaster)->AI()->JustSummoned(itr->creature);
            }

            // used by eluna
#ifdef ENABLE_ELUNA
            if (Unit* summoner = m_caster->ToUnit())
            {
                if (Eluna* e = summoner->GetEluna())
                {
                    e->OnSummoned(itr->creature, summoner);
                }
            }
            else if (m_originalCaster)
            {
                if (Unit* summoner = m_originalCaster->ToUnit())
                {
                    if (Eluna* e = summoner->GetEluna())
                    {
                        e->OnSummoned(itr->creature, summoner);
                    }
                }
            }
#endif
        }
    }
}

bool Spell::DoSummonWild(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const* effect, uint32 level)
{
    MANGOS_ASSERT(!list.empty() && prop);

    uint32 creature_entry = effect->EffectMiscValue;
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(creature_entry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonWild: creature entry %u not found for spell %u.", creature_entry, m_spellInfo->Id);
        return false;
    }

    TempSpawnType summonType = (m_duration == 0) ? TEMPSPAWN_DEAD_DESPAWN : TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN;

    for (CreatureSummonPositions::iterator itr = list.begin(); itr != list.end(); ++itr)
        if (Creature* summon = m_caster->SummonCreature(creature_entry, itr->x, itr->y, itr->z, m_caster->GetOrientation(), summonType, m_duration))
        {
            itr->creature = summon;

            summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

            // UNIT_FIELD_CREATEDBY are not set for these kind of spells.
            // Does exceptions exist? If so, what are they?
            // summon->SetCreatorGuid(m_caster->GetObjectGuid());

            // Notify original caster if not done already
            if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
            {
                ((Creature*)m_originalCaster)->AI()->JustSummoned(summon);
#ifdef ENABLE_ELUNA
                if (Unit* summoner = m_originalCaster->ToUnit())
                {
                    if (Eluna* e = summoner->GetEluna())
                    {
                        e->OnSummoned(summon, summoner);
                    }
                }
#endif
            }
        }
        else
        {
            return false;
        }

    return true;
}

bool Spell::DoSummonCritter(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const* effect, uint32 /*level*/)
{
    MANGOS_ASSERT(!list.empty() && prop);

    // ATM only first position is supported for summoning
    uint32 pet_entry = effect->EffectMiscValue;
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(pet_entry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonCritter: creature entry %u not found for spell %u.", pet_entry, m_spellInfo->Id);
        return false;
    }

    Pet* old_critter = m_caster->GetMiniPet();

    // for same pet just despawn (player unsummon command)
    if (m_caster->GetTypeId() == TYPEID_PLAYER && old_critter && old_critter->GetEntry() == pet_entry)
    {
        m_caster->RemoveMiniPet();
        return false;
    }

    // despawn old pet before summon new
    if (old_critter)
    {
        m_caster->RemoveMiniPet();
    }

    // for (CreatureSummonPositions::iterator itr = list.begin(); itr != list.end(); ++itr)
    CreatureCreatePos pos(m_caster->GetMap(), list[0].x, list[0].y, list[0].z, m_caster->GetOrientation(), m_caster->GetPhaseMask());

    // summon new pet
    Pet* critter = new Pet(MINI_PET);

    uint32 pet_number = sObjectMgr.GeneratePetNumber();
    if (!critter->Create(m_caster->GetMap()->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
    {
        sLog.outError("Spell::EffectSummonCritter, spellid %u: no such creature entry %u", m_spellInfo->Id, pet_entry);
        delete critter;
        return false;
    }

    // itr!
    list[0].creature = critter;

    critter->SetRespawnCoord(pos);

    // critter->SetName("");                                // generated by client
    critter->SetOwnerGuid(m_caster->GetObjectGuid());
    critter->SetCreatorGuid(m_caster->GetObjectGuid());

    critter->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

    critter->InitPetCreateSpells();                         // e.g. disgusting oozeling has a create spell as critter...
    // critter->InitLevelupSpellsForLevel();                // none?
    critter->SelectLevel(critter->GetCreatureInfo());       // some summoned creaters have different from 1 DB data for level/hp
    critter->SetUInt32Value(UNIT_NPC_FLAGS, critter->GetCreatureInfo()->NpcFlags);
    // some mini-pets have quests
    // set timer for unsummon
    if (m_duration > 0)
    {
        critter->SetDuration(m_duration);
    }

    m_caster->SetMiniPet(critter);

#ifdef ENABLE_ELUNA
    if (Unit* summoner = m_caster->ToUnit())
    {
        if (Eluna* e = summoner->GetEluna())
        {
            e->OnSummoned(critter, summoner);
        }
    }
    if (m_originalCaster)
        if (Unit* summoner = m_originalCaster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(critter, summoner);
            }
        }
#endif

    return true;
}

bool Spell::DoSummonGuardian(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const* effect, uint32 level)
{
    MANGOS_ASSERT(!list.empty() && prop);

    uint32 pet_entry = effect->EffectMiscValue;
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(pet_entry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonGuardian: creature entry %u not found for spell %u.", pet_entry, m_spellInfo->Id);
        return false;
    }

    PetType petType = prop->Title == UNITNAME_SUMMON_TITLE_COMPANION ? PROTECTOR_PET : GUARDIAN_PET;

    // second direct cast unsummon guardian(s) (guardians without like functionality have cooldown > spawn time)
    if (!m_IsTriggeredSpell && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        bool found = false;
        // including protector
        while (Pet* old_summon = m_caster->FindGuardianWithEntry(pet_entry))
        {
            old_summon->Unsummon(PET_SAVE_AS_DELETED, m_caster);
            found = true;
        }

        if (found)
        {
            return false;
        }
    }

    // protectors allowed only in single amount
    if (petType == PROTECTOR_PET)
        if (Pet* old_protector = m_caster->GetProtectorPet())
        {
            old_protector->Unsummon(PET_SAVE_AS_DELETED, m_caster);
        }

    // in another case summon new
    for (CreatureSummonPositions::iterator itr = list.begin(); itr != list.end(); ++itr)
    {
        Pet* spawnCreature = new Pet(petType);

        CreatureCreatePos pos(m_caster->GetMap(), itr->x, itr->y, itr->z, -m_caster->GetOrientation(), m_caster->GetPhaseMask());

        uint32 pet_number = sObjectMgr.GeneratePetNumber();
        if (!spawnCreature->Create(m_caster->GetMap()->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
        {
            sLog.outError("Spell::DoSummonGuardian: can't create creature entry %u for spell %u.", pet_entry, m_spellInfo->Id);
            delete spawnCreature;
            return false;
        }

        itr->creature = spawnCreature;

        spawnCreature->SetRespawnCoord(pos);

        if (m_duration > 0)
        {
            spawnCreature->SetDuration(m_duration);
        }

        CreatureInfo const* cInfo = spawnCreature->GetCreatureInfo();

        // spawnCreature->SetName("");                      // generated by client
        spawnCreature->SetOwnerGuid(m_caster->GetObjectGuid());
        spawnCreature->SetUInt32Value(UNIT_FIELD_FLAGS, cInfo->UnitFlags);
        spawnCreature->SetUInt32Value(UNIT_NPC_FLAGS, cInfo->NpcFlags);
        spawnCreature->setFaction(m_caster->getFaction());
        spawnCreature->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
        spawnCreature->SetCreatorGuid(m_caster->GetObjectGuid());
        spawnCreature->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

        spawnCreature->InitStatsForLevel(level);

        if (CharmInfo* charmInfo = spawnCreature->GetCharmInfo())
        {
            charmInfo->SetPetNumber(pet_number, false);

            if (spawnCreature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE))
            {
                charmInfo->SetReactState(REACT_PASSIVE);
            }
            else if ((cInfo->ExtraFlags & CREATURE_FLAG_EXTRA_NO_MELEE) || petType == PROTECTOR_PET)
            {
                charmInfo->SetReactState(REACT_DEFENSIVE);
            }
            else
            {
                charmInfo->SetReactState(REACT_AGGRESSIVE);
            }
        }

        m_caster->AddGuardian(spawnCreature);

#ifdef ENABLE_ELUNA
        if (Unit* summoner = m_caster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(spawnCreature, summoner);
            }
        }
        if (m_originalCaster)
        {
            if (Unit* summoner = m_originalCaster->ToUnit())
            {
                if (Eluna* e = summoner->GetEluna())
                {
                    e->OnSummoned(spawnCreature, summoner);
                }
            }
        }
#endif
    }

    return true;
}

bool Spell::DoSummonTotem(SpellEffectEntry const* effect, uint8 slot_dbc)
{
    // DBC store slots starting from 1, with no slot 0 value)
    int slot = slot_dbc ? slot_dbc - 1 : TOTEM_SLOT_NONE;

    // unsummon old totem
    if (slot < MAX_TOTEM_SLOT)
        if (Totem* OldTotem = m_caster->GetTotem(TotemSlot(slot)))
        {
            OldTotem->UnSummon();
        }

    // FIXME: Setup near to finish point because GetObjectBoundingRadius set in Create but some Create calls can be dependent from proper position
    // if totem have creature_template_addon.auras with persistent point for example or script call
    float angle = slot < MAX_TOTEM_SLOT ? M_PI_F / MAX_TOTEM_SLOT - (slot * 2 * M_PI_F / MAX_TOTEM_SLOT) : 0;

    CreatureCreatePos pos(m_caster, m_caster->GetOrientation(), 2.0f, angle);

    CreatureInfo const* cinfo = ObjectMgr::GetCreatureTemplate(effect->EffectMiscValue);
    if (!cinfo)
    {
        sLog.outErrorDb("Creature entry %u does not exist but used in spell %u totem summon.", m_spellInfo->Id, effect->EffectMiscValue);
        return false;
    }

    Totem* pTotem = new Totem;

    if (!pTotem->Create(m_caster->GetMap()->GenerateLocalLowGuid(HIGHGUID_UNIT), pos, cinfo, m_caster))
    {
        delete pTotem;
        return false;
    }

    pTotem->SetRespawnCoord(pos);

    if (slot < MAX_TOTEM_SLOT)
    {
        m_caster->_AddTotem(TotemSlot(slot), pTotem);
    }

    // pTotem->SetName("");                                 // generated by client
    pTotem->SetOwner(m_caster);
    pTotem->SetTypeBySummonSpell(m_spellInfo);              // must be after Create call where m_spells initialized

    pTotem->SetDuration(m_duration);

    if (damage)                                             // if not spell info, DB values used
    {
        pTotem->SetMaxHealth(damage);
        pTotem->SetHealth(damage);
    }

    pTotem->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        pTotem->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
    }

    if (m_caster->IsPvP())
    {
        pTotem->SetPvP(true);
    }

    if (m_caster->IsFFAPvP())
    {
        pTotem->SetFFAPvP(true);
    }

    // sending SMSG_TOTEM_CREATED before add to map (done in Summon)
    if (slot < MAX_TOTEM_SLOT && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_TOTEM_CREATED, 1 + 8 + 4 + 4);
        data << uint8(slot);
        data << pTotem->GetObjectGuid();
        data << uint32(m_duration);
        data << uint32(m_spellInfo->Id);
        ((Player*)m_caster)->SendDirectMessage(&data);
    }

    pTotem->Summon(m_caster);

    return false;
}

bool Spell::DoSummonPossessed(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const* effect, uint32 level)
{
    MANGOS_ASSERT(!list.empty() && prop);

    uint32 const& creatureEntry = effect->EffectMiscValue;

    Unit* newUnit = m_caster->TakePossessOf(m_spellInfo, prop, effect, list[0].x, list[0].y, list[0].z, m_caster->GetOrientation());
    if (!newUnit)
    {
        sLog.outError("Spell::DoSummonPossessed: creature entry %d for spell %u could not be summoned.", creatureEntry, m_spellInfo->Id);
        return false;
    }

    list[0].creature = static_cast<Creature*>(newUnit);

    // Notify Summoner
    if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
    {
        ((Creature*)m_originalCaster)->AI()->JustSummoned(list[0].creature);

#ifdef ENABLE_ELUNA
        if (Unit* summoner = m_originalCaster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(list[0].creature, summoner);
            }
        }
#endif
    }

    return true;
}

bool Spell::DoSummonPet(SpellEffectEntry const* effect)
{
    if (m_caster->GetPetGuid())
    {
        return false;
    }

    if (!unitTarget)
    {
        return false;
    }

    uint32 pet_entry = effect->EffectMiscValue;
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(pet_entry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonPet: creature entry %u not found for spell %u.", pet_entry, m_spellInfo->Id);
        return false;
    }

    Pet* spawnCreature = new Pet();

    // set timer for unsummon
    if (m_duration > 0)
    {
        spawnCreature->SetDuration(m_duration);
    }

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (spawnCreature->LoadPetFromDB((Player*)m_caster, pet_entry))
        {
            // Summon in dest location
            if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
            {
                spawnCreature->Relocate(m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, -m_caster->GetOrientation());
            }

            return true;
        }

        spawnCreature->setPetType(SUMMON_PET);
    }
    else
    {
        spawnCreature->setPetType(GUARDIAN_PET);
    }

    // Summon in dest location
    CreatureCreatePos pos(m_caster->GetMap(), m_targets.m_destX, m_targets.m_destY, m_targets.m_destZ, -m_caster->GetOrientation(), m_caster->GetPhaseMask());

    if (!(m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
    {
        pos = CreatureCreatePos(m_caster, -m_caster->GetOrientation());
    }

    Map* map = m_caster->GetMap();
    uint32 pet_number = sObjectMgr.GeneratePetNumber();
    if (!spawnCreature->Create(map->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, pet_number))
    {
        sLog.outErrorDb("Spell::EffectSummon: can't create creature with entry %u for spell %u", cInfo->Entry, m_spellInfo->Id);
        delete spawnCreature;
        return false;
    }

    uint32 level = std::max(m_caster->getLevel() + effect->EffectMultipleValue, 1.0f);

    spawnCreature->SetRespawnCoord(pos);

    spawnCreature->SetOwnerGuid(m_caster->GetObjectGuid());
    spawnCreature->setFaction(m_caster->getFaction());
    spawnCreature->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, 0);
    spawnCreature->SetCreatorGuid(m_caster->GetObjectGuid());
    spawnCreature->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

    spawnCreature->InitStatsForLevel(level);

    spawnCreature->GetCharmInfo()->SetPetNumber(pet_number, false);

    spawnCreature->InitPetCreateSpells();
    spawnCreature->InitLevelupSpellsForLevel();

    // spawnCreature->SetName("");                          // generated by client

    map->Add((Creature*)spawnCreature);
    spawnCreature->AIM_Initialize();

    m_caster->SetPet(spawnCreature);

    if (m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        spawnCreature->GetCharmInfo()->SetReactState(REACT_DEFENSIVE);
        spawnCreature->SavePetToDB(PET_SAVE_AS_CURRENT);
        ((Player*)m_caster)->PetSpellInitialize();
    }
    else
    {
        // Notify Summoner
        if (m_originalCaster && (m_originalCaster != m_caster)
            && (m_originalCaster->GetTypeId() == TYPEID_UNIT) && ((Creature*)m_originalCaster)->AI())
        {
            ((Creature*)m_originalCaster)->AI()->JustSummoned(spawnCreature);
            if (m_originalCaster->IsInCombat() && !(spawnCreature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE)))
            {
                ((Creature*)spawnCreature)->AI()->AttackStart(m_originalCaster->getAttackerForHelper());
            }
        }
        else if ((m_caster->GetTypeId() == TYPEID_UNIT) && ((Creature*)m_caster)->AI())
        {
            ((Creature*)m_caster)->AI()->JustSummoned(spawnCreature);
            if (m_caster->IsInCombat() && !(spawnCreature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE)))
            {
                ((Creature*)spawnCreature)->AI()->AttackStart(m_caster->getAttackerForHelper());
            }
        }
    }

#ifdef ENABLE_ELUNA
    if (Unit* summoner = m_caster->ToUnit())
    {
        if (Eluna* e = summoner->GetEluna())
        {
            e->OnSummoned(spawnCreature, summoner);
        }
    }
    if (m_originalCaster)
    {
        if (Unit* summoner = m_originalCaster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(spawnCreature, summoner);
            }
        }
    }
#endif
    return true;
}

bool Spell::DoSummonVehicle(CreatureSummonPositions& list, SummonPropertiesEntry const* prop, SpellEffectEntry const * effect, uint32 /*level*/)
{
    MANGOS_ASSERT(!list.empty() && prop);

    uint32 creatureEntry = effect->EffectMiscValue;
    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(creatureEntry);
    if (!cInfo)
    {
        sLog.outErrorDb("Spell::DoSummonVehicle: creature entry %u not found for spell %u.", creatureEntry, m_spellInfo->Id);
        return false;
    }

    Creature* spawnCreature = m_caster->SummonCreature(creatureEntry, list[0].x, list[0].y, list[0].z, m_caster->GetOrientation(),
                              (m_duration == 0) ? TEMPSPAWN_CORPSE_DESPAWN : TEMPSPAWN_TIMED_OOC_OR_CORPSE_DESPAWN, m_duration);

    if (!spawnCreature)
    {
        sLog.outError("Spell::DoSummonVehicle: creature entry %u for spell %u could not be summoned.", creatureEntry, m_spellInfo->Id);
        return false;
    }

    list[0].creature = spawnCreature;

    // Changes to be sent
    spawnCreature->SetCreatorGuid(m_caster->GetObjectGuid());
    spawnCreature->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);
    //spawnCreature->SetLevel(level); // Do we need to set level for vehicles?

    // Board the caster right after summoning
    SpellEntry const* controlSpellEntry = sSpellStore.LookupEntry(effect->CalculateSimpleValue());
    if (controlSpellEntry && IsSpellHaveAura(controlSpellEntry, SPELL_AURA_CONTROL_VEHICLE))
    {
        m_caster->CastSpell(spawnCreature, controlSpellEntry, true);
    }
    else
    {
        m_caster->CastSpell(spawnCreature, SPELL_RIDE_VEHICLE_HARDCODED, true);
    }

    // If the boarding failed...
    if (!spawnCreature->HasAuraType(SPELL_AURA_CONTROL_VEHICLE))
    {
        spawnCreature->ForcedDespawn();
        return false;
    }

    // Notify Summoner
    if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
    {
        ((Creature*)m_originalCaster)->AI()->JustSummoned(spawnCreature);
    }
#ifdef ENABLE_ELUNA
    if (Unit* summoner = m_caster->ToUnit())
    {
        if (Eluna* e = summoner->GetEluna())
        {
            e->OnSummoned(spawnCreature, summoner);
        }
    }
    else if (m_originalCaster)
        if (Unit* summoner = m_originalCaster->ToUnit())
        {
            if (Eluna* e = summoner->GetEluna())
            {
                e->OnSummoned(spawnCreature, summoner);
            }
        }
#endif /* ENABLE_ELUNA */
    return true;
}

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

/**
 * @brief Adds flat threat from the caster to the unit target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectThreat(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || !unitTarget->IsAlive() || !m_caster->IsAlive())
    {
        return;
    }

    if (!unitTarget->CanHaveThreatList())
    {
        return;
    }

    unitTarget->AddThreat(m_caster, float(damage), false, GetSpellSchoolMask(m_spellInfo), m_spellInfo);
}

/**
 * @brief Heals the target for an amount equal to the caster's maximum health.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectHealMaxHealth(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    uint32 heal = m_caster->GetMaxHealth();

    m_healing += heal;
}

/**
 * @brief Interrupts interruptible non-melee spells on the unit target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectInterruptCast(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget)
    {
        return;
    }
    if (!unitTarget->IsAlive())
    {
        return;
    }

    // TODO: not all spells that used this effect apply cooldown at school spells
    // also exist case: apply cooldown to interrupted cast only and to all spells
    for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
    {
        if (Spell* spell = unitTarget->GetCurrentSpell(CurrentSpellTypes(i)))
        {
            SpellEntry const* curSpellInfo = spell->m_spellInfo;
            // check if we can interrupt spell
            if ((curSpellInfo->GetInterruptFlags() & SPELL_INTERRUPT_FLAG_INTERRUPT) && curSpellInfo->GetPreventionType() == SPELL_PREVENTION_TYPE_SILENCE )
            {
                unitTarget->ProhibitSpellSchool(GetSpellSchoolMask(curSpellInfo), GetSpellDuration(m_spellInfo));
                unitTarget->InterruptSpell(CurrentSpellTypes(i), false);
            }
        }
    }
}

/**
 * @brief Summons a wild game object at the destination or near the caster.
 *
 * @param effect The summon object effect index.
 */
void Spell::EffectSummonObjectWild(SpellEffectEntry const* effect)
{
    uint32 gameobject_id = effect->EffectMiscValue;

    GameObject* pGameObj = new GameObject;

    WorldObject* target = focusObject;
    if (!target)
    {
        target = m_caster;
    }

    float x, y, z;
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(x, y, z);
    }
    else
    {
        m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);
    }

    Map* map = target->GetMap();

    if (!pGameObj->Create(map->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), gameobject_id, map,
                          m_caster->GetPhaseMask(), x, y, z, target->GetOrientation()))
    {
        delete pGameObj;
        return;
    }

    pGameObj->SetRespawnTime(m_duration > 0 ? m_duration / IN_MILLISECONDS : 0);
    pGameObj->SetSpellId(m_spellInfo->Id);

    // Wild object not have owner and check clickable by players
    map->Add(pGameObj);

    // Store the GO to the caster
    m_caster->AddWildGameObject(pGameObj);

    if (pGameObj->GetGoType() == GAMEOBJECT_TYPE_FLAGDROP && m_caster->GetTypeId() == TYPEID_PLAYER)
    {
        Player* pl = (Player*)m_caster;
        BattleGround* bg = ((Player*)m_caster)->GetBattleGround();

        switch (pGameObj->GetMapId())
        {
            case 489:                                       // WS
            {
                if (bg && bg->GetTypeID() == BATTLEGROUND_WS && bg->GetStatus() == STATUS_IN_PROGRESS)
                {
                    Team team = pl->GetTeam() == ALLIANCE ? HORDE : ALLIANCE;

                    ((BattleGroundWS*)bg)->SetDroppedFlagGuid(pGameObj->GetObjectGuid(), team);
                }
                break;
            }
            case 566:                                       // EY
            {
                if (bg && bg->GetTypeID() == BATTLEGROUND_EY && bg->GetStatus() == STATUS_IN_PROGRESS)
                {
                    ((BattleGroundEY*)bg)->SetDroppedFlagGuid(pGameObj->GetObjectGuid());
                }
                break;
            }
        }
    }

    pGameObj->SummonLinkedTrapIfAny();

    if (m_caster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_caster)->AI())
    {
        ((Creature*)m_caster)->AI()->JustSummoned(pGameObj);
    }
    if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
    {
        ((Creature*)m_originalCaster)->AI()->JustSummoned(pGameObj);
    }
}

/**
 * @brief Clears combat and threat state for the target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectSanctuary(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget)
    {
        return;
    }
    // unitTarget->CombatStop();

    unitTarget->CombatStop();
    unitTarget->GetHostileRefManager().deleteReferences();  // stop all fighting

    // Vanish allows to remove all threat and cast regular stealth so other spells can be used
    if (m_spellInfo->IsFitToFamily(SPELLFAMILY_ROGUE, UI64LIT(0x0000000000000800)))
    {
        ((Player*)m_caster)->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
    }
}

/**
 * @brief Adds combo points to the caster for the unit target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectAddComboPoints(SpellEffectEntry const* effect /*effect*/)
{
    if (!unitTarget)
    {
        return;
    }

    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (damage <= 0)
    {
        return;
    }

    ((Player*)m_caster)->AddComboPoints(unitTarget, damage);
}

/**
 * @brief Creates a duel flag object and starts a duel request between two players.
 *
 * @param effect The effect index containing the duel flag game object id.
 */
void Spell::EffectDuel(SpellEffectEntry const* effect)
{
    if (!m_caster || !unitTarget || m_caster->GetTypeId() != TYPEID_PLAYER || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* caster = (Player*)m_caster;
    Player* target = (Player*)unitTarget;

    // caster or target already have requested duel
    if (caster->duel || target->duel || !target->GetSocial() || target->GetSocial()->HasIgnore(caster->GetObjectGuid()))
    {
        return;
    }

    // Players can only fight a duel with each other outside (=not inside dungeons and not in capital cities)
    AreaTableEntry const* casterAreaEntry = GetAreaEntryByAreaID(caster->GetAreaId());
    if (casterAreaEntry && !(casterAreaEntry->flags & AREA_FLAG_DUEL))
    {
        SendCastResult(SPELL_FAILED_NO_DUELING);            // Dueling isn't allowed here
        return;
    }

    AreaTableEntry const* targetAreaEntry = GetAreaEntryByAreaID(target->GetAreaId());
    if (targetAreaEntry && !(targetAreaEntry->flags & AREA_FLAG_DUEL))
    {
        SendCastResult(SPELL_FAILED_NO_DUELING);            // Dueling isn't allowed here
        return;
    }

    // CREATE DUEL FLAG OBJECT
    GameObject* pGameObj = new GameObject;

    uint32 gameobject_id = effect->EffectMiscValue;

    Map* map = m_caster->GetMap();
    float x = (m_caster->GetPositionX() + unitTarget->GetPositionX()) * 0.5f;
    float y = (m_caster->GetPositionY() + unitTarget->GetPositionY()) * 0.5f;
    float z = m_caster->GetPositionZ();
    m_caster->UpdateAllowedPositionZ(x, y, z);
    if (!pGameObj->Create(map->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), gameobject_id, map, m_caster->GetPhaseMask(), x, y, z, m_caster->GetOrientation()))
    {
        delete pGameObj;
        return;
    }

    pGameObj->SetUInt32Value(GAMEOBJECT_FACTION, m_caster->getFaction());
    pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel() + 1);

    pGameObj->SetRespawnTime(m_duration > 0 ? m_duration / IN_MILLISECONDS : 0);
    pGameObj->SetSpellId(m_spellInfo->Id);

    m_caster->AddGameObject(pGameObj);
    map->Add(pGameObj);
    // END

    // Send request
    WorldPacket data(SMSG_DUEL_REQUESTED, 8 + 8);
    data << pGameObj->GetObjectGuid();
    data << caster->GetObjectGuid();
    caster->GetSession()->SendPacket(&data);
    target->GetSession()->SendPacket(&data);

    // create duel-info
    DuelInfo* duel   = new DuelInfo;
    duel->initiator  = caster;
    duel->opponent   = target;
    duel->startTime  = 0;
    duel->startTimer = 0;
    caster->duel     = duel;

    DuelInfo* duel2   = new DuelInfo;
    duel2->initiator  = caster;
    duel2->opponent   = caster;
    duel2->startTime  = 0;
    duel2->startTimer = 0;
    target->duel      = duel2;

    caster->SetGuidValue(PLAYER_DUEL_ARBITER, pGameObj->GetObjectGuid());
    target->SetGuidValue(PLAYER_DUEL_ARBITER, pGameObj->GetObjectGuid());

    // Used by Eluna
#ifdef ENABLE_ELUNA
    if (Eluna* e = caster->GetEluna())
    {
        e->OnDuelRequest(target, caster);
    }
#endif /* ENABLE_ELUNA */
}

/**
 * @brief Teleports a player target to its homebind as an unstuck action.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectStuck(SpellEffectEntry const* effect /*effect*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (!sWorld.getConfig(CONFIG_BOOL_CAST_UNSTUCK))
    {
        return;
    }

    Player* pTarget = (Player*)unitTarget;

    DEBUG_LOG("Spell Effect: Stuck");
    DETAIL_LOG("Player %s (guid %u) used auto-unstuck future at map %u (%f, %f, %f)", pTarget->GetName(), pTarget->GetGUIDLow(), m_caster->GetMapId(), m_caster->GetPositionX(), pTarget->GetPositionY(), pTarget->GetPositionZ());

    if (pTarget->IsTaxiFlying())
    {
        return;
    }

    // homebind location is loaded always
    pTarget->TeleportToHomebind(unitTarget == m_caster ? TELE_TO_SPELL : 0);

    // Stuck spell trigger Hearthstone cooldown
    SpellEntry const* spellInfo = sSpellStore.LookupEntry(8690);
    if (!spellInfo)
    {
        return;
    }
    Spell spell(pTarget, spellInfo, true);
    spell.SendSpellCooldown();
}

/**
 * @brief Sends a summon request to a player target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectSummonPlayer(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // Evil Twin (ignore player summon, but hide this for summoner)
    if (unitTarget->GetDummyAura(23445))
    {
        return;
    }

    float x, y, z;
    m_caster->GetClosePoint(x, y, z, unitTarget->GetObjectBoundingRadius());

    ((Player*)unitTarget)->SetSummonPoint(m_caster->GetMapId(), x, y, z);

    WorldPacket data(SMSG_SUMMON_REQUEST, 8 + 4 + 4);
    data << m_caster->GetObjectGuid();                      // summoner guid
    data << uint32(m_caster->GetZoneId());                  // summoner zone
    data << uint32(MAX_PLAYER_SUMMON_DELAY * IN_MILLISECONDS); // auto decline after msecs
    ((Player*)unitTarget)->GetSession()->SendPacket(&data);
}

/**
 * @brief Builds the default script command used to activate a game object.
 *
 * @return ScriptInfo Preconfigured activation command data.
 */
static ScriptInfo generateActivateCommand()
{
    ScriptInfo si;
    si.command = SCRIPT_COMMAND_ACTIVATE_OBJECT;
    si.id = 0;
    si.buddyEntry = 0;
    si.searchRadiusOrGuid = 0;
    si.data_flags = 0x00;
    return si;
}

/**
 * @brief Activates or manipulates the targeted game object based on the effect misc value.
 *
 * @param effect The activation effect index.
 */
void Spell::EffectActivateObject(SpellEffectEntry const* effect)
{
    if (!gameObjTarget)
    {
        return;
    }

    uint32 misc_value = uint32(effect->EffectMiscValue);

    switch (misc_value)
    {
        case 1:                     // GO simple use
        case 2:                     // unk - 2 spells
        case 4:                     // unk - 1 spell
        case 5:                     // GO trap usage
        case 7:                     // unk - 2 spells
        case 8:                     // GO usage with TargetB = none or random
        case 10:                    // GO explosions
        case 11:                    // unk - 1 spell
        case 19:                    // unk - 1 spell
        case 20:                    // unk - 2 spells
        {
            static ScriptInfo activateCommand = generateActivateCommand();

            int32 delay_secs = effect->CalculateSimpleValue();
            gameObjTarget->GetMap()->ScriptCommandStart(activateCommand, delay_secs, m_caster, gameObjTarget);
            break;
        }
        case 3:                     // GO custom anim - found mostly in Lunar Fireworks spells
            gameObjTarget->SendGameObjectCustomAnim(gameObjTarget->GetObjectGuid());
            break;
        case 12:                    // GO state active alternative - found mostly in Simon Game spells
            gameObjTarget->UseDoorOrButton(0, true);
            break;
        case 13:                    // GO state ready - found only in Simon Game spells
            gameObjTarget->ResetDoorOrButton();
            break;
        case 15:                    // GO destroy
            gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
            break;
        case 16:                    // GO custom use - found mostly in Wind Stones spells, Simon Game spells and other GO target summoning spells
        {
            switch (m_spellInfo->Id)
            {
                case 24734:         // Summon Templar Random
                case 24744:         // Summon Templar (fire)
                case 24756:         // Summon Templar (air)
                case 24758:         // Summon Templar (earth)
                case 24760:         // Summon Templar (water)
                case 24763:         // Summon Duke Random
                case 24765:         // Summon Duke (fire)
                case 24768:         // Summon Duke (air)
                case 24770:         // Summon Duke (earth)
                case 24772:         // Summon Duke (water)
                case 24784:         // Summon Royal Random
                case 24786:         // Summon Royal (fire)
                case 24788:         // Summon Royal (air)
                case 24789:         // Summon Royal (earth)
                case 24790:         // Summon Royal (water)
                {
                    uint32 npcEntry = 0;
                    uint32 templars[] = {15209, 15211, 15212, 15307};
                    uint32 dukes[] = {15206, 15207, 15208, 15220};
                    uint32 royals[] = {15203, 15204, 15205, 15305};

                    switch (m_spellInfo->Id)
                    {
                        case 24734: npcEntry = templars[urand(0, 3)]; break;
                        case 24763: npcEntry = dukes[urand(0, 3)];    break;
                        case 24784: npcEntry = royals[urand(0, 3)];   break;
                        case 24744: npcEntry = 15209;                 break;
                        case 24756: npcEntry = 15212;                 break;
                        case 24758: npcEntry = 15307;                 break;
                        case 24760: npcEntry = 15211;                 break;
                        case 24765: npcEntry = 15206;                 break;
                        case 24768: npcEntry = 15220;                 break;
                        case 24770: npcEntry = 15208;                 break;
                        case 24772: npcEntry = 15207;                 break;
                        case 24786: npcEntry = 15203;                 break;
                        case 24788: npcEntry = 15204;                 break;
                        case 24789: npcEntry = 15205;                 break;
                        case 24790: npcEntry = 15305;                 break;
                    }

                    gameObjTarget->SummonCreature(npcEntry, gameObjTarget->GetPositionX(), gameObjTarget->GetPositionY(), gameObjTarget->GetPositionZ(), gameObjTarget->GetAngle(m_caster), TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, MINUTE * IN_MILLISECONDS);
                    gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
                    break;
                }
                case 40176:         // Simon Game pre-game Begin, blue
                case 40177:         // Simon Game pre-game Begin, green
                case 40178:         // Simon Game pre-game Begin, red
                case 40179:         // Simon Game pre-game Begin, yellow
                case 40283:         // Simon Game END, blue
                case 40284:         // Simon Game END, green
                case 40285:         // Simon Game END, red
                case 40286:         // Simon Game END, yellow
                case 40494:         // Simon Game, switched ON
                case 40495:         // Simon Game, switched OFF
                case 40512:         // Simon Game, switch...disable Off switch
                    gameObjTarget->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
                    break;
                case 40632:         // Summon Gezzarak the Huntress
                case 40640:         // Summon Karrog
                case 40642:         // Summon Darkscreecher Akkarai
                case 40644:         // Summon Vakkiz the Windrager
                case 41004:         // Summon Terokk
                    gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
                    break;
                case 46085:         // Place Fake Fur
                {
                    float x, y, z;
                    gameObjTarget->GetClosePoint(x, y, z, gameObjTarget->GetObjectBoundingRadius(), 2 * INTERACTION_DISTANCE, frand(0, M_PI_F * 2));

                    // Note: event script is implemented in script library
                    gameObjTarget->SummonCreature(25835, x, y, z, gameObjTarget->GetOrientation(), TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, 15000);
                    gameObjTarget->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE);
                    break;
                }
                case 46592:         // Summon Ahune Lieutenant
                {
                    uint32 npcEntry = 0;

                    switch (gameObjTarget->GetEntry())
                    {
                        case 188049: npcEntry = 26116; break;       // Frostwave Lieutenant (Ashenvale)
                        case 188137: npcEntry = 26178; break;       // Hailstone Lieutenant (Desolace)
                        case 188138: npcEntry = 26204; break;       // Chillwind Lieutenant (Stranglethorn)
                        case 188148: npcEntry = 26214; break;       // Frigid Lieutenant (Searing Gorge)
                        case 188149: npcEntry = 26215; break;       // Glacial Lieutenant (Silithus)
                        case 188150: npcEntry = 26216; break;       // Glacial Templar (Hellfire Peninsula)
                    }

                    gameObjTarget->SummonCreature(npcEntry, gameObjTarget->GetPositionX(), gameObjTarget->GetPositionY(), gameObjTarget->GetPositionZ(), gameObjTarget->GetAngle(m_caster), TEMPSPAWN_TIMED_OOC_OR_DEAD_DESPAWN, MINUTE * IN_MILLISECONDS);
                    gameObjTarget->SetLootState(GO_JUST_DEACTIVATED);
                    break;
                }
            }
            break;
        }
        case 17:                    // GO unlock - found mostly in Simon Game spells
            gameObjTarget->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NO_INTERACT);
            break;
        default:
            sLog.outError("Spell::EffectActivateObject called with unknown misc value. Spell Id %u", m_spellInfo->Id);
            break;
    }
}

void Spell::EffectApplyGlyph(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)m_caster;

    // glyph sockets level requirement
    uint8 minLevel = 0;
    switch (m_glyphIndex)
    {
        case 0:
        case 1:
        case 6: minLevel = 25; break;
        case 2:
        case 3:
        case 7: minLevel = 50; break;
        case 4:
        case 5:
        case 8: minLevel = 75; break;
    }

    if (minLevel && m_caster->getLevel() < minLevel)
    {
        SendCastResult(SPELL_FAILED_GLYPH_SOCKET_LOCKED);
        return;
    }

    // apply new one
    if (uint32 glyph = effect->EffectMiscValue)
    {
        if (GlyphPropertiesEntry const* gp = sGlyphPropertiesStore.LookupEntry(glyph))
        {
            if (GlyphSlotEntry const* gs = sGlyphSlotStore.LookupEntry(player->GetGlyphSlot(m_glyphIndex)))
            {
                if (gp->TypeFlags != gs->TypeFlags)
                {
                    SendCastResult(SPELL_FAILED_INVALID_GLYPH);
                    return;                                 // glyph slot mismatch
                }
            }

            // remove old glyph
            player->ApplyGlyph(m_glyphIndex, false);
            player->SetGlyph(m_glyphIndex, glyph);
            player->ApplyGlyph(m_glyphIndex, true);
            player->SendTalentsInfoData(false);
        }
    }
// TODO: ELUNAFIX NEEDED
//#ifdef ENABLE_ELUNA
//    if (Unit* summoner = m_originalCaster->ToUnit())
//    {
//        if (Eluna* e = player->GetEluna())
//        {
//            e->OnSummoned(spawnCreature, summoner);
//        }
//    }
//#endif /* ENABLE_ELUNA */

}

/**
 * @brief Applies a temporary enchantment to the main-hand item of the player target.
 *
 * @param effect The enchant effect index.
 */
void Spell::EffectEnchantHeldItem(SpellEffectEntry const* effect)
{
    // this is only item spell effect applied to main-hand weapon of target player (players in area)
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* item_owner = (Player*)unitTarget;
    Item* item = item_owner->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

    if (!item)
    {
        return;
    }

    // must be equipped
    if (!item ->IsEquipped())
    {
        return;
    }

    if (effect->EffectMiscValue)
    {
        uint32 enchant_id = effect->EffectMiscValue;
        int32 duration = m_duration;                        // Try duration index first...
        if (!duration)
        {
            duration = m_currentBasePoints[SpellEffectIndex(effect->EffectIndex)];        // Base points after...
        }
        if (!duration)
            duration = 10;                                  // 10 seconds for enchants which don't have listed duration

        SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
        if (!pEnchant)
        {
            return;
        }

        // Always go to temp enchantment slot
        EnchantmentSlot slot = TEMP_ENCHANTMENT_SLOT;

        // Enchantment will not be applied if a different one already exists
        if (item->GetEnchantmentId(slot) && item->GetEnchantmentId(slot) != enchant_id)
        {
            return;
        }

        // Apply the temporary enchantment
        item->SetEnchantment(slot, enchant_id, duration * IN_MILLISECONDS, 0, m_caster->GetObjectGuid());
        item_owner->ApplyEnchantment(item, slot, true);
    }
}

/**
 * @brief Starts disenchanting loot generation for the target item.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectDisEnchant(SpellEffectEntry const* /*effect*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* p_caster = (Player*)m_caster;
    if (!itemTarget || !itemTarget->GetProto()->DisenchantID)
    {
        return;
    }

    p_caster->UpdateCraftSkill(m_spellInfo->Id);

    ((Player*)m_caster)->SendLoot(itemTarget->GetObjectGuid(), LOOT_DISENCHANTING);

    // item will be removed at disenchanting end
}

/**
 * @brief Increases the drunk state of a player target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectInebriate(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)unitTarget;

    uint8 drunkValue = player->GetDrunkValue() + (uint8)damage;
    if (drunkValue > 100)
    {
        drunkValue = 100;
        if (roll_chance_i(25))
        {
            player->CastSpell(player, 67468, false);    // Drunken Vomit
        }
    }
    player->SetDrunkValue(drunkValue, m_CastItem ? m_CastItem->GetEntry() : 0);
}

/**
 * @brief Feeds the caster's pet and triggers the associated benefit spell.
 *
 * @param effect The effect index containing the triggered spell id.
 */
void Spell::EffectFeedPet(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* _player = (Player*)m_caster;

    Item* foodItem = m_targets.getItemTarget();
    if (!foodItem)
    {
        return;
    }

    Pet* pet = _player->GetPet();
    if (!pet)
    {
        return;
    }

    if (!pet->IsAlive())
    {
        return;
    }

    int32 benefit = pet->GetCurrentFoodBenefitLevel(foodItem->GetProto()->ItemLevel);
    if (benefit <= 0)
    {
        return;
    }

    uint32 count = 1;
    _player->DestroyItemCount(foodItem, count, true);
    // Clear item target to prevent dangling pointer if another effect targets the same item
    m_targets.setItemTarget(NULL);

    m_caster->CastCustomSpell(pet, effect->EffectTriggerSpell, &benefit, NULL, NULL, true);
}

/**
 * @brief Dismisses the caster's living pet.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectDismissPet(SpellEffectEntry const* /*effect*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Pet* pet = m_caster->GetPet();

    // not let dismiss dead pet
    if (!pet || !pet->IsAlive())
    {
        return;
    }

    pet->Unsummon(PET_SAVE_NOT_IN_SLOT, m_caster);
}

/**
 * @brief Summons a persistent object into one of the caster's object slots.
 *
 * @param effect The summon object effect index.
 */
void Spell::EffectSummonObject(SpellEffectEntry const* effect)
{
    uint32 go_id = effect->EffectMiscValue;
    uint8 slot = effect->EffectMiscValueB;
    if (slot >= MAX_OBJECT_SLOT)
    {
        return;
    }

    if (ObjectGuid guid = m_caster->m_ObjectSlotGuid[slot])
    {
        if (GameObject* obj = m_caster ? m_caster->GetMap()->GetGameObject(guid) : NULL)
        {
            obj->SetLootState(GO_JUST_DEACTIVATED);
        }
        m_caster->m_ObjectSlotGuid[slot].Clear();
    }

    GameObject* pGameObj = new GameObject;

    float x, y, z;
    // If dest location if present
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(x, y, z);
    }
    // Summon in random point all other units if location present
    else
    {
        m_caster->GetClosePoint(x, y, z, DEFAULT_WORLD_OBJECT_SIZE);
    }

    Map* map = m_caster->GetMap();
    if (!pGameObj->Create(map->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), go_id, map,
                          m_caster->GetPhaseMask(), x, y, z, m_caster->GetOrientation()))
    {
        delete pGameObj;
        return;
    }

    pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
    pGameObj->SetRespawnTime(m_duration > 0 ? m_duration / IN_MILLISECONDS : 0);
    pGameObj->SetSpellId(m_spellInfo->Id);
    m_caster->AddGameObject(pGameObj);

    map->Add(pGameObj);

    m_caster->m_ObjectSlotGuid[slot] = pGameObj->GetObjectGuid();

    pGameObj->SummonLinkedTrapIfAny();

    if (m_caster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_caster)->AI())
    {
        ((Creature*)m_caster)->AI()->JustSummoned(pGameObj);
    }
    if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
    {
        ((Creature*)m_originalCaster)->AI()->JustSummoned(pGameObj);
    }
}

/**
 * @brief Sends a resurrection request with percentage-based health and mana restoration.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectResurrect(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (unitTarget->IsAlive() || !unitTarget->IsInWorld())
    {
        return;
    }

    switch (m_spellInfo->Id)
    {
        case 8342:                                          // Defibrillate (Goblin Jumper Cables) has 33% chance on success
        case 22999:                                         // Defibrillate (Goblin Jumper Cables XL) has 50% chance on success
        case 54732:                                         // Defibrillate (Gnomish Army Knife) has 67% chance on success
        {
            uint32 failChance = 0;
            uint32 failSpellId = 0;
            switch (m_spellInfo->Id)
            {
                case 8342:  failChance = 67; failSpellId = 8338;  break;
                case 22999: failChance = 50; failSpellId = 23055; break;
                case 54732: failChance = 33; failSpellId = 0; break;
            }

            if (roll_chance_i(failChance))
            {
                if (failSpellId)
                {
                    m_caster->CastSpell(m_caster, failSpellId, true, m_CastItem);
                }
                return;
            }
            break;
        }
        default:
            break;
    }

    Player* pTarget = ((Player*)unitTarget);

    if (pTarget->isRessurectRequested())      // already have one active request
    {
        return;
    }

    uint32 health = pTarget->GetMaxHealth() * damage / 100;
    uint32 mana   = pTarget->GetMaxPower(POWER_MANA) * damage / 100;

    pTarget->setResurrectRequestData(m_caster->GetObjectGuid(), m_caster->GetMapId(), m_caster->GetPositionX(), m_caster->GetPositionY(), m_caster->GetPositionZ(), health, mana);
    SendResurrectRequest(pTarget);
}

/**
 * @brief Adds queued extra attacks to the target unit.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectAddExtraAttacks(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || !unitTarget->IsAlive())
    {
        return;
    }

    if (unitTarget->m_extraAttacks)
    {
        return;
    }

    unitTarget->m_extraAttacks = damage;
}

/**
 * @brief Grants the ability to parry to a player target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectParry(SpellEffectEntry const* /*effect*/)
{
    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)unitTarget)->SetCanParry(true);
    }
}

/**
 * @brief Grants the ability to block to a player target.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectBlock(SpellEffectEntry const* /*effect*/)
{
    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)unitTarget)->SetCanBlock(true);
    }
}

/**
 * @brief Teleports the target forward while avoiding steep terrain, water edges, and obstacles.
 *
 * @param effect The effect index providing the leap distance.
 */
void Spell::EffectLeapForward(SpellEffectEntry const* effect)
{
    float dist = GetSpellRadius(sSpellRadiusStore.LookupEntry(effect->GetRadiusIndex()));
    const float IN_OR_UNDER_LIQUID_RANGE = 0.8f;                // range to make player under liquid or on liquid surface from liquid level

    G3D::Vector3 prevPos, nextPos;
    float orientation = unitTarget->GetOrientation();

    prevPos.x = unitTarget->GetPositionX();
    prevPos.y = unitTarget->GetPositionY();
    prevPos.z = unitTarget->GetPositionZ();

    float groundZ = prevPos.z;

    // falling case
    if (!unitTarget->GetMap()->GetHeightInRange(unitTarget->GetPhaseMask(), prevPos.x, prevPos.y, groundZ, 3.0f) && unitTarget->m_movementInfo.HasMovementFlag(MOVEFLAG_FALLING))
    {
        nextPos.x = prevPos.x + dist * cos(orientation);
        nextPos.y = prevPos.y + dist * sin(orientation);
        nextPos.z = prevPos.z - 2.0f; // little hack to avoid the impression to go up when teleporting instead of continue to fall. This value may need some tweak

        //
        GridMapLiquidData liquidData;
        if (unitTarget->GetMap()->GetTerrain()->IsInWater(nextPos.x, nextPos.y, nextPos.z, &liquidData))
        {
            if (fabs(nextPos.z - liquidData.level) < 10.0f)
            {
                nextPos.z = liquidData.level - IN_OR_UNDER_LIQUID_RANGE;
            }
        }
        else
        {
            // fix z to ground if near of it
            unitTarget->GetMap()->GetHeightInRange(unitTarget->GetPhaseMask(), nextPos.x, nextPos.y, nextPos.z, 10.0f);
        }

        // check any obstacle and fix coords
        unitTarget->GetMap()->GetHitPosition(prevPos.x, prevPos.y, prevPos.z + 0.5f, nextPos.x, nextPos.y, nextPos.z, unitTarget->GetPhaseMask(), -0.5f);

        // teleport
        unitTarget->NearTeleportTo(nextPos.x, nextPos.y, nextPos.z, orientation, unitTarget == m_caster);

        //sLog.outString("Falling BLINK!");
        return;
    }

    // fix origin position if player was jumping and near of the ground but not in ground
    if (fabs(prevPos.z - groundZ) > 0.5f)
    {
        prevPos.z = groundZ;
    }

    //check if in liquid
    bool isPrevInLiquid = unitTarget->GetMap()->GetTerrain()->IsInWater(prevPos.x, prevPos.y, prevPos.z);

    const float step = 2.0f;                                    // step length before next check slope/edge/water
    const float maxSlope = 50.0f;                               // 50(degree) max seem best value for walkable slope
    const float MAX_SLOPE_IN_RADIAN = maxSlope / 180.0f * M_PI_F;
    float nextZPointEstimation = 1.0f;
    float destx = prevPos.x + dist * cos(orientation);
    float desty = prevPos.y + dist * sin(orientation);
    const uint32 numChecks = ceil(fabs(dist / step));
    const float DELTA_X = (destx - prevPos.x) / numChecks;
    const float DELTA_Y = (desty - prevPos.y) / numChecks;

    for (uint32 i = 1; i < numChecks + 1; ++i)
    {
        // compute next point average position
        nextPos.x = prevPos.x + DELTA_X;
        nextPos.y = prevPos.y + DELTA_Y;
        nextPos.z = prevPos.z + nextZPointEstimation;

        bool isInLiquid = false;
        bool isInLiquidTested = false;
        bool isOnGround = false;
        GridMapLiquidData liquidData;

        // try fix height for next position
        if (!unitTarget->GetMap()->GetHeightInRange(unitTarget->GetPhaseMask(), nextPos.x, nextPos.y, nextPos.z))
        {
            // we cant so test if we are on water
            if (!unitTarget->GetMap()->GetTerrain()->IsInWater(nextPos.x, nextPos.y, nextPos.z, &liquidData))
            {
                // not in water and cannot get correct height, maybe flying?
                //sLog.outString("Can't get height of point %u, point value %s", i, nextPos.toString().c_str());
                nextPos = prevPos;
                break;
            }
            else
            {
                isInLiquid = true;
                isInLiquidTested = true;
            }
        }
        else
        {
            isOnGround = true;                                  // player is on ground
        }

        if (isInLiquid || (!isInLiquidTested && unitTarget->GetMap()->GetTerrain()->IsInWater(nextPos.x, nextPos.y, nextPos.z, &liquidData)))
        {
            if (!isPrevInLiquid && fabs(liquidData.level - prevPos.z) > 2.0f)
            {
                // on edge of water with difference a bit to high to continue
                //sLog.outString("Ground vs liquid edge detected!");
                nextPos = prevPos;
                break;
            }

            if ((liquidData.level - IN_OR_UNDER_LIQUID_RANGE) > nextPos.z)
            {
                nextPos.z = prevPos.z;                                      // we are under water so next z equal prev z
            }
            else
            {
                nextPos.z = liquidData.level - IN_OR_UNDER_LIQUID_RANGE;    // we are on water surface, so next z equal liquid level
            }

            isInLiquid = true;

            float ground = nextPos.z;
            if (unitTarget->GetMap()->GetHeightInRange(unitTarget->GetPhaseMask(), nextPos.x, nextPos.y, ground))
            {
                if (nextPos.z < ground)
                {
                    nextPos.z = ground;
                    isOnGround = true;                          // player is on ground of the water
                }
            }
        }

        //unitTarget->SummonCreature(VISUAL_WAYPOINT, nextPos.x, nextPos.y, nextPos.z, 0, TEMPSPAWN_TIMED_DESPAWN, 15000);
        float hitZ = nextPos.z + 1.5f;
        if (unitTarget->GetMap()->GetHitPosition(prevPos.x, prevPos.y, prevPos.z + 1.5f, nextPos.x, nextPos.y, hitZ, unitTarget->GetPhaseMask(), -1.0f))
        {
            //sLog.outString("Blink collision detected!");
            nextPos = prevPos;
            break;
        }

        if (isOnGround)
        {
            // project vector to get only positive value
            float ac = fabs(prevPos.z - nextPos.z);

            // compute slope (in radian)
            float slope = atan(ac / step);

            // check slope value
            if (slope > MAX_SLOPE_IN_RADIAN)
            {
                //sLog.outString("bad slope detected! %4.2f max %4.2f, ac(%4.2f)", slope * 180 / M_PI_F, maxSlope, ac);
                nextPos = prevPos;
                break;
            }
            //sLog.outString("slope is ok! %4.2f max %4.2f, ac(%4.2f)", slope * 180 / M_PI_F, maxSlope, ac);
        }

        //sLog.outString("point %u is ok, coords %s", i, nextPos.toString().c_str());
        nextZPointEstimation = (nextPos.z - prevPos.z) / 2.0f;
        isPrevInLiquid = isInLiquid;
        prevPos = nextPos;
    }

    unitTarget->NearTeleportTo(nextPos.x, nextPos.y, nextPos.z, orientation, unitTarget == m_caster);
}

void Spell::EffectLeapBack(SpellEffectEntry const* effect)
{
    if (unitTarget->IsTaxiFlying())
    {
        return;
    }

    m_caster->KnockBackFrom(unitTarget, float(effect->EffectMiscValue) / 10, float(damage) / 10);
}

/**
 * @brief Modifies player reputation for the faction referenced by the effect.
 *
 * @param effect The effect index containing faction and reputation values.
 */
void Spell::EffectReputation(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* _player = (Player*)unitTarget;

    int32  rep_change = m_currentBasePoints[effect->EffectIndex];
    uint32 faction_id = effect->EffectMiscValue;

    FactionEntry const* factionEntry = sFactionStore.LookupEntry(faction_id);

    if (!factionEntry)
    {
        return;
    }

    rep_change = _player->CalculateReputationGain(REPUTATION_SOURCE_SPELL, rep_change, faction_id);

    _player->GetReputationMgr().ModifyReputation(factionEntry, rep_change);
}

/**
 * @brief Marks the referenced quest objective as completed for the player target.
 *
 * @param effect The effect index containing the quest id.
 */
void Spell::EffectQuestComplete(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    // A few spells has additional value from basepoints, check condition here.
    switch (m_spellInfo->Id)
    {
        case 43458:                                         // Secrets of Nifflevar
        {
            if (!unitTarget->HasAura(effect->CalculateSimpleValue()))
            {
                return;
            }

            break;
        }
        // TODO: implement these!
        // "this spell awards credit for the entire raid (all spell targets as this is area target) if just ONE member has both auras (yes, both effect's basepoints)"
        // case 72155:                                      // Harvest Blight Specimen
        // case 72162:                                      // Harvest Blight Specimen
        // break;
        default:
            break;
    }

    uint32 quest_id = effect->EffectMiscValue;
    ((Player*)unitTarget)->AreaExploredOrEventHappens(quest_id);
}

/**
 * @brief Resurrects the target player with flat or percentage-based health and mana.
 *
 * @param effect The effect index containing resurrection resource data.
 */
void Spell::EffectSelfResurrect(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->IsAlive())
    {
        return;
    }
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    if (!unitTarget->IsInWorld())
    {
        return;
    }

    uint32 health = 0;
    uint32 mana = 0;

    // flat case
    if (damage < 0)
    {
        health = uint32(-damage);
        mana = effect->EffectMiscValue;
    }
    // percent case
    else
    {
        health = uint32(damage / 100.0f * unitTarget->GetMaxHealth());
        if (unitTarget->GetMaxPower(POWER_MANA) > 0)
        {
            mana = uint32(damage / 100.0f * unitTarget->GetMaxPower(POWER_MANA));
        }
    }

    Player* plr = ((Player*)unitTarget);
    plr->ResurrectPlayer(0.0f);

    plr->SetHealth(health);
    plr->SetPower(POWER_MANA, mana);
    plr->SetPower(POWER_RAGE, 0);
    plr->SetPower(POWER_ENERGY, plr->GetMaxPower(POWER_ENERGY));

    plr->SpawnCorpseBones();
}

/**
 * @brief Opens skinning loot for a creature and updates the player's gathering skill.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectSkinning(SpellEffectEntry const* /*effect*/)
{
    if (unitTarget->GetTypeId() != TYPEID_UNIT)
    {
        return;
    }
    if (!m_caster || m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Creature* creature = (Creature*) unitTarget;
    int32 targetLevel = creature->getLevel();

    uint32 skill = creature->GetCreatureInfo()->GetRequiredLootSkill();

    ((Player*)m_caster)->SendLoot(creature->GetObjectGuid(), LOOT_SKINNING);
    creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

    int32 reqValue = targetLevel < 10 ? 0 : targetLevel < 20 ? (targetLevel - 10) * 10 : targetLevel * 5;

    int32 skillValue = ((Player*)m_caster)->GetPureSkillValue(skill);

    // Double chances for elites
    ((Player*)m_caster)->UpdateGatherSkill(skill, skillValue, reqValue, creature->IsElite() ? 2 : 1);
}

/**
 * @brief Moves the caster into melee contact with the target and starts attacking if appropriate.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectCharge(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget)
    {
        return;
    }

    // TODO: research more ContactPoint/attack distance.
    // 3.666666 instead of ATTACK_DISTANCE(5.0f) in below seem to give more accurate result.
    float x, y, z;
    unitTarget->GetContactPoint(m_caster, x, y, z, 3.666666f);

    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        ((Creature*)unitTarget)->StopMoving();
    }

    // Only send MOVEMENTFLAG_WALK_MODE, client has strange issues with other move flags
    m_caster->MonsterMoveWithSpeed(x, y, z, 24.f, true, true);

    // not all charge effects used in negative spells
    if (unitTarget != m_caster && !IsPositiveSpell(m_spellInfo->Id))
    {
        m_caster->Attack(unitTarget, true);
    }
}

void Spell::EffectCharge2(SpellEffectEntry const* /*effect*/)
{
    float x, y, z;
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(x, y, z);

        if (unitTarget->GetTypeId() != TYPEID_PLAYER)
        {
            ((Creature*)unitTarget)->StopMoving();
        }
    }
    else if (unitTarget && unitTarget != m_caster)
    {
        unitTarget->GetContactPoint(m_caster, x, y, z, 3.666666f);
    }
    else
    {
        return;
    }

    // Only send MOVEMENTFLAG_WALK_MODE, client has strange issues with other move flags
    m_caster->MonsterMoveWithSpeed(x, y, z, 24.f, true, true);

    // not all charge effects used in negative spells
    if (unitTarget && unitTarget != m_caster && !IsPositiveSpell(m_spellInfo->Id))
    {
        m_caster->Attack(unitTarget, true);
    }
}

/**
 * @brief Applies a knockback to a player target.
 *
 * @param effect The effect index containing horizontal speed data.
 */
void Spell::EffectKnockBack(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    unitTarget->KnockBackFrom(m_caster, float(effect->EffectMiscValue) / 10, float(damage) / 10);
}

/**
 * @brief Starts the taxi path referenced by the spell effect for a player target.
 *
 * @param effect The effect index containing the taxi path id.
 */
void Spell::EffectSendTaxi(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)unitTarget)->ActivateTaxiPathTo(effect->EffectMiscValue, m_spellInfo->Id);
}

/**
 * @brief Pulls a player target toward the caster using reverse knockback.
 *
 * @param effect The effect index containing vertical speed data.
 */
void Spell::EffectPlayerPull(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    float x, y, z;
    m_caster->GetPosition(x, y, z);

    // move back a bit
    x = x - (0.6 * cos(m_caster->GetOrientation() + M_PI_F));
    y = y - (0.6 * sin(m_caster->GetOrientation() + M_PI_F));

    // Try to normalize Z coord because GetContactPoint do nothing with Z axis
    unitTarget->UpdateAllowedPositionZ(x, y, z);

    float speed = m_spellInfo->speed ? m_spellInfo->speed : 27.0f;
    unitTarget->GetMotionMaster()->MoveJump(x, y, z, speed, 2.5f);
}

/**
 * @brief Removes auras from the target that match the specified mechanic.
 *
 * @param effect The effect index containing the mechanic id.
 */
void Spell::EffectDispelMechanic(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    uint32 mechanic = effect->EffectMiscValue;

    Unit::SpellAuraHolderMap& Auras = unitTarget->GetSpellAuraHolderMap();
    for (Unit::SpellAuraHolderMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
    {
        next = iter;
        ++next;
        SpellEntry const* spell = iter->second->GetSpellProto();
        if (iter->second->HasMechanic(mechanic))
        {
            unitTarget->RemoveAurasDueToSpell(spell->Id);
            if (Auras.empty())
            {
                break;
            }
            else
            {
                next = Auras.begin();
            }
        }
    }
}

/**
 * @brief Restores the caster's dead pet and revives it with percentage-based health.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectSummonDeadPet(SpellEffectEntry const* /*effect*/)
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
    if (pet->IsAlive())
    {
        return;
    }

    if (_player->GetDistance(pet) >= 2.0f)
    {
        float px, py, pz;
        m_caster->GetClosePoint(px, py, pz, pet->GetObjectBoundingRadius());
        ((Unit*)pet)->NearTeleportTo(px, py, pz, -m_caster->GetOrientation());
    }

    pet->SetUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_NONE);
    pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);
    pet->SetDeathState(ALIVE);
    pet->clearUnitState(UNIT_STAT_ALL_STATE);
    pet->SetHealth(uint32(pet->GetMaxHealth() * (float(damage) / 100)));

    pet->AIM_Initialize();

    // _player->PetSpellInitialize(); -- action bar not removed at death and not required send at revive
    pet->SavePetToDB(PET_SAVE_AS_CURRENT);
}

void Spell::EffectSummonAllTotems(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    int32 start_button = ACTION_BUTTON_SHAMAN_TOTEMS_BAR + effect->EffectMiscValue;
    int32 amount_buttons = effect->EffectMiscValueB;

    for (int32 slot = 0; slot < amount_buttons; ++slot)
        if (ActionButton const* actionButton = ((Player*)m_caster)->GetActionButton(start_button + slot))
            if (actionButton->GetType() == ACTION_BUTTON_SPELL)
                if (uint32 spell_id = actionButton->GetAction())
                {
                    m_caster->CastSpell(unitTarget, spell_id, true);
                }
}

/**
 * @brief Unsummons all totems currently owned by the caster.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectDestroyAllTotems(SpellEffectEntry const* /*effect*/)
{
    int32 mana = 0;
    for (int slot = 0;  slot < MAX_TOTEM_SLOT; ++slot)
    {
        if (Totem* totem = m_caster->GetTotem(TotemSlot(slot)))
        {
            if (damage)
            {
                uint32 spell_id = totem->GetUInt32Value(UNIT_CREATED_BY_SPELL);
                if (SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id))
                {
                    uint32 manacost = m_caster->GetCreateMana() * spellInfo->GetManaCostPercentage() / 100;
                    mana += manacost * damage / 100;
                }
            }
            totem->UnSummon();
        }
    }

    if (mana)
    {
        m_caster->CastCustomSpell(m_caster, 39104, &mana, NULL, NULL, true);
    }
}

void Spell::EffectBreakPlayerTargeting (SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget)
    {
        return;
    }

    WorldPacket data(SMSG_CLEAR_TARGET, 8);
    data << unitTarget->GetObjectGuid();
    unitTarget->SendMessageToSet(&data, false);
}

/**
 * @brief Removes a fixed number of durability points from one or more player items.
 *
 * @param effect The effect index containing the inventory slot selector.
 */
void Spell::EffectDurabilityDamage(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    int32 slot = effect->EffectMiscValue;

    // FIXME: some spells effects have value -1/-2
    // Possibly its mean -1 all player equipped items and -2 all items
    if (slot < 0)
    {
        ((Player*)unitTarget)->DurabilityPointsLossAll(damage, (slot < -1));
        return;
    }

    // invalid slot value
    if (slot >= INVENTORY_SLOT_BAG_END)
    {
        return;
    }

    if (Item* item = ((Player*)unitTarget)->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
    {
        ((Player*)unitTarget)->DurabilityPointsLoss(item, damage);
    }
}

/**
 * @brief Removes a percentage of durability from one or more player items.
 *
 * @param effect The effect index containing the inventory slot selector.
 */
void Spell::EffectDurabilityDamagePCT(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    int32 slot = effect->EffectMiscValue;

    // FIXME: some spells effects have value -1/-2
    // Possibly its mean -1 all player equipped items and -2 all items
    if (slot < 0)
    {
        ((Player*)unitTarget)->DurabilityLossAll(double(damage) / 100.0f, (slot < -1));
        return;
    }

    // invalid slot value
    if (slot >= INVENTORY_SLOT_BAG_END)
    {
        return;
    }

    if (damage <= 0)
    {
        return;
    }

    if (Item* item = ((Player*)unitTarget)->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
    {
        ((Player*)unitTarget)->DurabilityLoss(item, double(damage) / 100.0f);
    }
}

/**
 * @brief Modifies the caster's threat on the target by a percentage.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectModifyThreatPercent(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget)
    {
        return;
    }

    unitTarget->GetThreatManager().modifyThreatPercent(m_caster, damage);
}

/**
 * @brief Summons a transmitted game object such as fishing nodes, rituals, or spell casters.
 *
 * @param effect The effect index containing the game object entry.
 */
void Spell::EffectTransmitted(SpellEffectEntry const* effect)
{
    uint32 name_id = effect->EffectMiscValue;

    switch (m_spellInfo->Id)
    {
        case 29886: // Create Soulwell
            if (m_caster->HasAura(18692))
            {
                name_id = 183510;
            }
            else if (m_caster->HasAura(18693))
            {
                name_id = 183511;
            }
            break;
        default:
            break;
    }

    GameObjectInfo const* goinfo = ObjectMgr::GetGameObjectInfo(name_id);

    if (!goinfo)
    {
        sLog.outErrorDb("Gameobject (Entry: %u) not exist and not created at spell (ID: %u) cast", name_id, m_spellInfo->Id);
        return;
    }

    float fx, fy, fz;

    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(fx, fy, fz);
    }
    // FIXME: this can be better check for most objects but still hack
    else if (effect->GetRadiusIndex() && m_spellInfo->speed == 0)
    {
        float dis = GetSpellRadius(sSpellRadiusStore.LookupEntry(effect->GetRadiusIndex()));
        m_caster->GetClosePoint(fx, fy, fz, DEFAULT_WORLD_OBJECT_SIZE, dis);
    }
    else
    {
        float min_dis = GetSpellMinRange(sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex));
        float max_dis = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellInfo->rangeIndex));
        float dis = rand_norm_f() * (max_dis - min_dis) + min_dis;

        // special code for fishing bobber (TARGET_SELF_FISHING), should not try to avoid objects
        // nor try to find ground level, but randomly vary in angle
        if (goinfo->type == GAMEOBJECT_TYPE_FISHINGNODE)
        {
            // calculate angle variation for roughly equal dimensions of target area
            float max_angle = (max_dis - min_dis) / (max_dis + m_caster->GetObjectBoundingRadius());
            float angle_offset = max_angle * (rand_norm_f() - 0.5f);
            m_caster->GetNearPoint2D(fx, fy, dis + m_caster->GetObjectBoundingRadius(), m_caster->GetOrientation() + angle_offset);

            GridMapLiquidData liqData;
            if (!m_caster->GetTerrain()->IsInWater(fx, fy, m_caster->GetPositionZ() + 1.f, &liqData))
            {
                SendCastResult(SPELL_FAILED_NOT_FISHABLE);
                SendChannelUpdate(0);
                return;
            }

            fz = liqData.level;
            // finally, check LoS
            if (!m_caster->IsWithinLOS(fx, fy, fz))
            {
                SendCastResult(SPELL_FAILED_LINE_OF_SIGHT);
                SendChannelUpdate(0);
                return;
            }
        }
        else
        {
            m_caster->GetClosePoint(fx, fy, fz, DEFAULT_WORLD_OBJECT_SIZE, dis);
        }
    }

    Map* cMap = m_caster->GetMap();

    // if gameobject is summoning object, it should be spawned right on caster's position
    if (goinfo->type == GAMEOBJECT_TYPE_SUMMONING_RITUAL)
    {
        m_caster->GetPosition(fx, fy, fz);
    }

    GameObject* pGameObj = new GameObject;

    if (!pGameObj->Create(cMap->GenerateLocalLowGuid(HIGHGUID_GAMEOBJECT), name_id, cMap,
                          m_caster->GetPhaseMask(), fx, fy, fz, m_caster->GetOrientation()))
    {
        delete pGameObj;
        return;
    }

    int32 duration = m_duration;

    switch (goinfo->type)
    {
        case GAMEOBJECT_TYPE_FISHINGNODE:
        {
            m_caster->SetChannelObjectGuid(pGameObj->GetObjectGuid());
            m_caster->AddGameObject(pGameObj);              // will removed at spell cancel

            // end time of range when possible catch fish (FISHING_BOBBER_READY_TIME..GetDuration(m_spellInfo))
            // start time == fish-FISHING_BOBBER_READY_TIME (0..GetDuration(m_spellInfo)-FISHING_BOBBER_READY_TIME)
            int32 lastSec = 0;
            switch (urand(0, 3))
            {
                case 0: lastSec =  3; break;
                case 1: lastSec =  7; break;
                case 2: lastSec = 13; break;
                case 3: lastSec = 17; break;
            }

            duration = duration - lastSec * IN_MILLISECONDS + FISHING_BOBBER_READY_TIME * IN_MILLISECONDS;
            break;
        }
        case GAMEOBJECT_TYPE_SUMMONING_RITUAL:
        {
            if (m_caster->GetTypeId() == TYPEID_PLAYER)
            {
                pGameObj->AddUniqueUse((Player*)m_caster);
                m_caster->AddGameObject(pGameObj);          // will removed at spell cancel
            }
            break;
        }
        case GAMEOBJECT_TYPE_SPELLCASTER:
        {
            m_caster->AddGameObject(pGameObj);
            break;
        }
        case GAMEOBJECT_TYPE_FISHINGHOLE:
        case GAMEOBJECT_TYPE_CHEST:
        default:
            break;
    }

    pGameObj->SetRespawnTime(duration > 0 ? duration / IN_MILLISECONDS : 0);

    pGameObj->SetOwnerGuid(m_caster->GetObjectGuid());

    pGameObj->SetUInt32Value(GAMEOBJECT_LEVEL, m_caster->getLevel());
    pGameObj->SetSpellId(m_spellInfo->Id);

    DEBUG_LOG("AddObject at SpellEfects.cpp EffectTransmitted");
    // m_caster->AddGameObject(pGameObj);
    // m_ObjToDel.push_back(pGameObj);

    cMap->Add(pGameObj);

    pGameObj->SummonLinkedTrapIfAny();

    if (m_caster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_caster)->AI())
    {
        ((Creature*)m_caster)->AI()->JustSummoned(pGameObj);
    }
    if (m_originalCaster && m_originalCaster != m_caster && m_originalCaster->GetTypeId() == TYPEID_UNIT && ((Creature*)m_originalCaster)->AI())
    {
        ((Creature*)m_originalCaster)->AI()->JustSummoned(pGameObj);
    }
}

void Spell::EffectProspecting(SpellEffectEntry const* /*effect*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER || !itemTarget)
    {
        return;
    }

    Player* p_caster = (Player*)m_caster;

    if (sWorld.getConfig(CONFIG_BOOL_SKILL_PROSPECTING))
    {
        uint32 SkillValue = p_caster->GetPureSkillValue(SKILL_JEWELCRAFTING);
        uint32 reqSkillValue = itemTarget->GetProto()->RequiredSkillRank;
        p_caster->UpdateGatherSkill(SKILL_JEWELCRAFTING, SkillValue, reqSkillValue);
    }

    ((Player*)m_caster)->SendLoot(itemTarget->GetObjectGuid(), LOOT_PROSPECTING);
}

void Spell::EffectMilling(SpellEffectEntry const* /*effect*/)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER || !itemTarget)
    {
        return;
    }

    Player* p_caster = (Player*)m_caster;

    if (sWorld.getConfig(CONFIG_BOOL_SKILL_MILLING))
    {
        uint32 SkillValue = p_caster->GetPureSkillValue(SKILL_INSCRIPTION);
        uint32 reqSkillValue = itemTarget->GetProto()->RequiredSkillRank;
        p_caster->UpdateGatherSkill(SKILL_INSCRIPTION, SkillValue, reqSkillValue);
    }

    ((Player*)m_caster)->SendLoot(itemTarget->GetObjectGuid(), LOOT_MILLING);
}

void Spell::EffectSkill(SpellEffectEntry const* /*effect*/)
{
    DEBUG_LOG("WORLD: SkillEFFECT");
}

/**
 * @brief Fully resurrects a dead player target as part of a spirit heal effect.
 *
 * @param effect Unused effect index.
 */
void Spell::EffectSpiritHeal(SpellEffectEntry const* /*effect*/)
{
    // TODO player can't see the heal-animation - he should respawn some ticks later
    if (!unitTarget || unitTarget->IsAlive())
    {
        return;
    }
    if (unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }
    if (!unitTarget->IsInWorld())
    {
        return;
    }
    if (m_spellInfo->Id == 22012 && !unitTarget->HasAura(2584))
    {
        return;
    }

    ((Player*)unitTarget)->ResurrectPlayer(1.0f);
    ((Player*)unitTarget)->SpawnCorpseBones();
}

// remove insignia spell effect
void Spell::EffectSkinPlayerCorpse(SpellEffectEntry const* /*effect*/)
{
    DEBUG_LOG("Effect: SkinPlayerCorpse");
    if ((m_caster->GetTypeId() != TYPEID_PLAYER) || (unitTarget->GetTypeId() != TYPEID_PLAYER) || (unitTarget->IsAlive()))
    {
        return;
    }

    ((Player*)unitTarget)->RemovedInsignia((Player*)m_caster);
}

void Spell::EffectStealBeneficialBuff(SpellEffectEntry const* effect)
{
    DEBUG_LOG("Effect: StealBeneficialBuff");

    if (!unitTarget || unitTarget == m_caster)              // can't steal from self
    {
        return;
    }

    typedef std::vector<SpellAuraHolder*> StealList;
    StealList steal_list;
    // Create dispel mask by dispel type
    uint32 dispelMask  = GetDispellMask( DispelType(effect->EffectMiscValue) );
    Unit::SpellAuraHolderMap const& auras = unitTarget->GetSpellAuraHolderMap();
    for (Unit::SpellAuraHolderMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
    {
        SpellAuraHolder *holder = itr->second;
        if (holder && (1<<holder->GetSpellProto()->GetDispel()) & dispelMask)
        {
            // Need check for passive? this
            if (holder->IsPositive() && !holder->IsPassive() && !holder->GetSpellProto()->HasAttribute(SPELL_ATTR_EX4_NOT_STEALABLE))
            {
                steal_list.push_back(holder);
            }
        }
    }
    // Ok if exist some buffs for dispel try dispel it
    if (!steal_list.empty())
    {
        typedef std::list < std::pair<uint32, ObjectGuid> > SuccessList;
        SuccessList success_list;
        int32 list_size = steal_list.size();
        // Dispell N = damage buffs (or while exist buffs for dispel)
        for (int32 count = 0; count < damage && list_size > 0; ++count)
        {
            // Random select buff for dispel
            SpellAuraHolder* holder = steal_list[urand(0, list_size - 1)];
            // Not use chance for steal
            // TODO possible need do it
            success_list.push_back(SuccessList::value_type(holder->GetId(), holder->GetCasterGuid()));

            // Remove buff from list for prevent doubles
            for (StealList::iterator j = steal_list.begin(); j != steal_list.end();)
            {
                SpellAuraHolder* stealed = *j;
                if (stealed->GetId() == holder->GetId() && stealed->GetCasterGuid() == holder->GetCasterGuid())
                {
                    j = steal_list.erase(j);
                    --list_size;
                }
                else
                {
                    ++j;
                }
            }
        }
        // Really try steal and send log
        if (!success_list.empty())
        {
            int32 count = success_list.size();
            WorldPacket data(SMSG_SPELLSTEALLOG, 8 + 8 + 4 + 1 + 4 + count * 5);
            data << unitTarget->GetPackGUID();       // Victim GUID
            data << m_caster->GetPackGUID();         // Caster GUID
            data << uint32(m_spellInfo->Id);         // Dispell spell id
            data << uint8(0);                        // not used
            data << uint32(count);                   // count
            for (SuccessList::iterator j = success_list.begin(); j != success_list.end(); ++j)
            {
                SpellEntry const* spellInfo = sSpellStore.LookupEntry(j->first);
                data << uint32(spellInfo->Id);       // Spell Id
                data << uint8(0);                    // 0 - steals !=0 transfers
                unitTarget->RemoveAurasDueToSpellBySteal(spellInfo->Id, j->second, m_caster);
            }
            m_caster->SendMessageToSet(&data, true);
        }
    }
}

void Spell::EffectWMODamage(SpellEffectEntry const* effect)
{
    DEBUG_LOG("Effect: WMODamage");

    if (!gameObjTarget)
    {
        return;
    }

    if (gameObjTarget->GetGoType() != GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
    {
        sLog.outError("Spell::EffectWMODamage called without valid targets. Spell Id %u", m_spellInfo->Id);
        return;
    }

    if (!gameObjTarget->GetHealth())
    {
        return;
    }

    Unit* caster = GetAffectiveCaster();
    if (!caster)
    {
        return;
    }

    DEBUG_LOG("Spell::EffectWMODamage, spell Id %u, go entry %u, damage %u", m_spellInfo->Id, gameObjTarget->GetEntry(), uint32(damage));
    gameObjTarget->DealGameObjectDamage(uint32(damage), m_spellInfo->Id, caster);
}

void Spell::EffectWMORepair(SpellEffectEntry const* effect)
{
    DEBUG_LOG("Effect: WMORepair");

    if (!gameObjTarget)
    {
        return;
    }

    if (gameObjTarget->GetGoType() != GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
    {
        sLog.outError("Spell::EffectWMORepair called without valid targets. Spell Id %u", m_spellInfo->Id);
        return;
    }

    Unit* caster = GetAffectiveCaster();
    if (!caster)
    {
        return;
    }

    DEBUG_LOG("Spell::EffectWMORepair, spell Id %u, go entry %u", m_spellInfo->Id, gameObjTarget->GetEntry());
    gameObjTarget->RebuildGameObject(m_spellInfo->Id, caster);
}

void Spell::EffectWMOChange(SpellEffectEntry const* effect)
{
    DEBUG_LOG("Effect: WMOChange");

    if (!gameObjTarget)
    {
        return;
    }

    if (gameObjTarget->GetGoType() != GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING)
    {
        sLog.outError("Spell::EffectWMOChange called without valid targets. Spell Id %u", m_spellInfo->Id);
        return;
    }

    DEBUG_LOG("Spell::EffectWMOChange, spell Id %u, object %u, misc-value %u", m_spellInfo->Id, gameObjTarget->GetEntry(), effect->EffectMiscValue);

    Unit* caster = GetAffectiveCaster();
    if (!caster)
    {
        return;
    }

    switch (effect->EffectMiscValue)
    {
        case 0:                                             // Set to full health
            gameObjTarget->ForceGameObjectHealth(gameObjTarget->GetMaxHealth(), caster);
            break;
        case 1:                                             // Set to damaged
            gameObjTarget->ForceGameObjectHealth(gameObjTarget->GetGOInfo()->destructibleBuilding.damagedNumHits, caster);
            break;
        case 2:                                             // Set to destroyed
            gameObjTarget->ForceGameObjectHealth(-int32(gameObjTarget->GetHealth()), caster);
            break;
        case 3:                                             // Set to rebuilding
            gameObjTarget->ForceGameObjectHealth(0, caster);
            break;
        default:
            sLog.outError("Spell::EffectWMOChange, spell Id %u with undefined change value %u", m_spellInfo->Id, effect->EffectMiscValue);
            break;
    }
}

void Spell::EffectKillCreditPersonal(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)unitTarget)->KilledMonsterCredit(effect->EffectMiscValue);
}

void Spell::EffectKillCreditGroup(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)unitTarget)->RewardPlayerAndGroupAtEvent(effect->EffectMiscValue, unitTarget);
}

void Spell::EffectQuestFail(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)unitTarget)->FailQuest(effect->EffectMiscValue);
}

void Spell::EffectActivateRune(SpellEffectEntry const* effect)
{
    if (m_caster->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* plr = (Player*)m_caster;

    if (plr->getClass() != CLASS_DEATH_KNIGHT)
    {
        return;
    }

    int32 count = damage;                                   // max amount of reset runes

    plr->ResyncRunes();
}

void Spell::EffectTitanGrip(SpellEffectEntry const* effect)
{
    // Make sure "Titan's Grip" (49152) penalty spell does not silently change
    if (effect->EffectMiscValue != 49152)
    {
        sLog.outError("Spell::EffectTitanGrip: Spell %u has unexpected EffectMiscValue '%u'", m_spellInfo->Id, effect->EffectMiscValue);
    }
    if (unitTarget && unitTarget->GetTypeId() == TYPEID_PLAYER)
    {
        Player* plr = (Player*)m_caster;
        plr->SetCanTitanGrip(true);
        if (plr->HasTwoHandWeaponInOneHand() && !plr->HasAura(49152))
        {
            plr->CastSpell(plr, 49152, true);
        }
    }
}

void Spell::EffectRenamePet(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_UNIT ||
            !((Creature*)unitTarget)->IsPet() || ((Pet*)unitTarget)->getPetType() != HUNTER_PET)
        return;

    unitTarget->RemoveByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);
}

void Spell::EffectPlaySound(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    uint32 soundId = effect->EffectMiscValue;
    if (!sSoundEntriesStore.LookupEntry(soundId))
    {
        sLog.outError("EffectPlaySound: Sound (Id: %u) in spell %u does not exist.", soundId, m_spellInfo->Id);
        return;
    }

    unitTarget->PlayDirectSound(soundId, (Player*)unitTarget);
}

void Spell::EffectPlayMusic(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    uint32 soundId = effect->EffectMiscValue;
    if (!sSoundEntriesStore.LookupEntry(soundId))
    {
        sLog.outError("EffectPlayMusic: Sound (Id: %u) in spell %u does not exist.", soundId, m_spellInfo->Id);
        return;
    }

    m_caster->PlayMusic(soundId, (Player*)unitTarget);
}

void Spell::EffectSpecCount(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    ((Player*)unitTarget)->UpdateSpecCount(damage);
}

void Spell::EffectActivateSpec(SpellEffectEntry const* /*effect*/)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    uint32 spec = damage - 1;

    ((Player*)unitTarget)->ActivateSpec(spec);
}

/**
 * @brief Sets the player's homebind location to the current position.
 *
 * @param effect The bind effect index.
 */
void Spell::EffectBind(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)unitTarget;

    uint32 area_id = uint32(effect->EffectMiscValue);
    WorldLocation loc;
    if (effect->EffectImplicitTargetA == TARGET_TABLE_X_Y_Z_COORDINATES ||
        effect->EffectImplicitTargetB == TARGET_TABLE_X_Y_Z_COORDINATES)
    {
        SpellTargetPosition const* st = sSpellMgr.GetSpellTargetPosition(m_spellInfo->Id);
        if (!st)
        {
            sLog.outError("Spell::EffectBind - unknown Teleport coordinates for spell ID %u", m_spellInfo->Id);
            return;
        }

        loc.mapid       = st->target_mapId;
        loc.coord_x     = st->target_X;
        loc.coord_y     = st->target_Y;
        loc.coord_z     = st->target_Z;
        loc.orientation = st->target_Orientation;
        if (!area_id)
        {
            area_id = sTerrainMgr.GetAreaId(loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z);
        }
    }
    else
    {
        player->GetPosition(loc);
        if (!area_id)
        {
            area_id = player->GetAreaId();
        }
    }

    player->SetHomebindToLocation(loc, area_id);

    // binding
    WorldPacket data(SMSG_BINDPOINTUPDATE, (4 + 4 + 4 + 4 + 4));
    data << float(loc.coord_x);
    data << float(loc.coord_y);
    data << float(loc.coord_z);
    data << uint32(loc.mapid);
    data << uint32(area_id);
    player->SendDirectMessage(&data);

    DEBUG_LOG("New Home Position for %s: XYZ: %f %f %f on Map %u", player->GetGuidStr().c_str(), loc.coord_x, loc.coord_y, loc.coord_z, loc.mapid);

    // zone update
    data.Initialize(SMSG_PLAYERBOUND, 8 + 4);
    data << m_caster->GetObjectGuid();
    data << uint32(area_id);
    player->SendDirectMessage(&data);
}

void Spell::EffectRestoreItemCharges(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)unitTarget;

    ItemPrototype const* itemProto = ObjectMgr::GetItemPrototype(effect->EffectItemType);
    if (!itemProto)
    {
        return;
    }

    // In case item from limited category recharge any from category, is this valid checked early in spell checks
    Item* item;
    if (itemProto->ItemLimitCategory)
    {
        item = ((Player*)unitTarget)->GetItemByLimitedCategory(itemProto->ItemLimitCategory);
    }
    else
    {
        item = player->GetItemByEntry(effect->EffectItemType);
    }

    if (!item)
    {
        return;
    }

    item->RestoreCharges();
}

/**
 * @brief Sends a battleground player target to its graveyard.
 *
 * @param effect The teleport effect index.
 */
void Spell::EffectRedirectThreat(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    if (m_spellInfo->Id == 59665)                           // Vigilance
        if (Aura* glyph = unitTarget->GetDummyAura(63326))  // Glyph of Vigilance
        {
            damage += glyph->GetModifier()->m_amount;
        }

    m_caster->GetHostileRefManager().SetThreatRedirection(unitTarget->GetObjectGuid(), uint32(damage));
}

void Spell::EffectTeachTaxiNode(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    Player* player = (Player*)unitTarget;

    uint32 taxiNodeId = effect->EffectMiscValue;
    if (!sTaxiNodesStore.LookupEntry(taxiNodeId))
    {
        return;
    }

    if (player->m_taxi.SetTaximaskNode(taxiNodeId))
    {
        WorldPacket data(SMSG_NEW_TAXI_PATH, 0);
        player->SendDirectMessage(&data);

        data.Initialize(SMSG_TAXINODE_STATUS, 9);
        data << m_caster->GetObjectGuid();
        data << uint8(1);
        player->SendDirectMessage(&data);
    }
}

void Spell::EffectQuestOffer(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->GetTypeId() != TYPEID_PLAYER)
    {
        return;
    }

    if (Quest const* quest = sObjectMgr.GetQuestTemplate(effect->EffectMiscValue))
    {
        Player* player = (Player*)unitTarget;

        if (player->CanTakeQuest(quest, false))
        {
            player->PlayerTalkClass->SendQuestGiverQuestDetails(quest, player->GetObjectGuid(), true);
        }
    }
}

void Spell::EffectCancelAura(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    uint32 spellId = effect->EffectTriggerSpell;

    if (!sSpellStore.LookupEntry(spellId))
    {
        sLog.outError("Spell::EffectCancelAura: spell %u doesn't exist", spellId);
        return;
    }

    unitTarget->RemoveAurasDueToSpell(spellId);
}

void Spell::EffectKnockBackFromPosition(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    float x, y, z;
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(x, y, z);
    }
    else
    {
        m_caster->GetPosition(x, y, z);
    }

    float angle = unitTarget->GetAngle(x, y) + M_PI_F;
    float horizontalSpeed = effect->EffectMiscValue * 0.1f;
    float verticalSpeed = damage * 0.1f;
    unitTarget->KnockBackWithAngle(angle, horizontalSpeed, verticalSpeed);
}

void Spell::EffectGravityPull(SpellEffectEntry const* effect)
{
    if (!unitTarget)
    {
        return;
    }

    float x, y, z;
    if (m_targets.m_targetMask & TARGET_FLAG_DEST_LOCATION)
    {
        m_targets.getDestination(x, y, z);
    }
    else
    {
        m_caster->GetPosition(x, y, z);
    }

    float speed = float(effect->EffectMiscValue) * 0.15f;
    float height = float(unitTarget->GetDistance(x, y, z) * 0.2f);

    unitTarget->GetMotionMaster()->MoveJump(x, y, z, speed, height);
}

void Spell::EffectCreateTamedPet(SpellEffectEntry const* effect)
{
    if (!unitTarget || unitTarget->getClass() != CLASS_HUNTER)
    {
        return;
    }

    uint32 creatureEntry = effect->EffectMiscValue;

    CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(creatureEntry);
    if (creatureEntry && !cInfo)
    {
        sLog.outErrorDb("EffectCreateTamedPet: Creature entry %u not found for spell %u.", creatureEntry, m_spellInfo->Id);
        return;
    }

    Pet* newTamedPet = new Pet(HUNTER_PET);
    CreatureCreatePos pos(unitTarget, unitTarget->GetOrientation());

    Map* map = unitTarget->GetMap();
    uint32 petNumber = sObjectMgr.GeneratePetNumber();
    if (!newTamedPet->Create(map->GenerateLocalLowGuid(HIGHGUID_PET), pos, cInfo, petNumber))
    {
        delete newTamedPet;
        return;
    }

    newTamedPet->SetRespawnCoord(pos);

    newTamedPet->SetOwnerGuid(unitTarget->GetObjectGuid());
    newTamedPet->SetCreatorGuid(unitTarget->GetObjectGuid());
    newTamedPet->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
    newTamedPet->setFaction(unitTarget->getFaction());
    newTamedPet->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
    newTamedPet->SetUInt32Value(UNIT_CREATED_BY_SPELL, m_spellInfo->Id);

    newTamedPet->GetCharmInfo()->SetPetNumber(petNumber, true);

    if (unitTarget->IsPvP())
    {
        newTamedPet->SetPvP(true);
    }

    if (unitTarget->IsFFAPvP())
    {
        newTamedPet->SetFFAPvP(true);
    }

    newTamedPet->InitStatsForLevel(unitTarget->getLevel());
    newTamedPet->InitPetCreateSpells();
    newTamedPet->InitLevelupSpellsForLevel();
    newTamedPet->InitTalentForLevel();

    newTamedPet->SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);
    newTamedPet->SetByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_ABANDONED);

    newTamedPet->AIM_Initialize();

    float x, y, z;
    unitTarget->GetClosePoint(x, y, z, newTamedPet->GetObjectBoundingRadius());
    newTamedPet->Relocate(x, y, z, unitTarget->GetOrientation());

    map->Add((Creature*)newTamedPet);
    m_caster->SetPet(newTamedPet);

    newTamedPet->SavePetToDB(PET_SAVE_AS_CURRENT);
    ((Player*)unitTarget)->PetSpellInitialize();
}

