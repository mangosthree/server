/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file PetHandler.cpp
 * @brief Pet interaction opcode handlers
 *
 * This file handles pet-related opcodes including:
 * - CMSG_PET_ACTION: Pet action (attack, follow, stay, etc.)
 * - CMSG_PET_NAME_QUERY: Query pet name
 * - CMSG_PET_ABANDON: Abandon pet
 * - CMSG_PET_RENAME: Rename pet
 * - CMSG_PET_SPELL_AUTOCAST: Toggle autocast
 * - CMSG_PET_CANCEL_AURA: Cancel pet aura
 * - CMSG_PET_STOP_ATTACK: Stop pet attack
 * - CMSG_PET_SET_ACTION: Set pet action bar slot
 *
 * Pets include hunter pets, warlock minions, and temporary summons.
 * Pet actions are validated and synchronized with the owner.
 */

#include "Platform/Define.h"
#include <cstring>
#include <string>
#include <ctime>
#include "WorldPacket.h"
#include "WorldSession.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Spell.h"
#include "CreatureAI.h"
#include "Util.h"
#include "Pet.h"
#include "SpellAuras.h"
#include "TemporarySummon.h"
#include "movement/typedefs.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

/**
 * @brief Handles pet action bar commands, reactions, and spell casts.
 *
 * @param recv_data The incoming pet action packet.
 */
void WorldSession::HandlePetAction(WorldPacket& recv_data)
{
    ObjectGuid petGuid;
    uint32 data;
    ObjectGuid targetGuid;
    float x, y, z;
    recv_data >> petGuid;
    recv_data >> data;
    recv_data >> targetGuid;
    recv_data >> x >> y >> z;

    uint32 spellid = UNIT_ACTION_BUTTON_ACTION(data);
    uint8 flag = UNIT_ACTION_BUTTON_TYPE(data);             // delete = 0x07 CastSpell = C1

    DETAIL_LOG("HandlePetAction: %s flag is %u, spellid is %u, target %s.", petGuid.GetString().c_str(), uint32(flag), spellid, targetGuid.GetString().c_str());

    // used also for charmed creature/player
    Unit* pet = _player->GetMap()->GetUnit(petGuid);
    if (!pet)
    {
        sLog.outError("HandlePetAction: %s not exist.", petGuid.GetString().c_str());
        return;
    }

    if (_player->GetObjectGuid() != pet->GetCharmerOrOwnerGuid())
    {
        sLog.outError("HandlePetAction: %s isn't controlled by %s.", petGuid.GetString().c_str(), _player->GetGuidStr().c_str());
        return;
    }

    if (!pet->IsAlive())
    {
        return;
    }

    if (pet->GetTypeId() == TYPEID_PLAYER)
    {
        // controller player can only do melee attack
        if (!(flag == ACT_COMMAND && spellid == COMMAND_ATTACK))
        {
            return;
        }
    }
    else if (((Creature*)pet)->IsPet())
    {
        // pet can have action bar disabled
        if (((Pet*)pet)->GetModeFlags() & PET_MODE_DISABLE_ACTIONS)
        {
            return;
        }
    }

    CharmInfo* charmInfo = pet->GetCharmInfo();
    if (!charmInfo)
    {
        sLog.outError("WorldSession::HandlePetAction: object (GUID: %u TypeId: %u) is considered pet-like but doesn't have a charminfo!", pet->GetGUIDLow(), pet->GetTypeId());
        return;
    }

    switch (flag)
    {
        case ACT_COMMAND:                                   // 0x07
            switch (spellid)
            {
                case COMMAND_STAY:                          // flat=1792  // STAY
                {
                    pet->StopMoving();
                    pet->AttackStop(true);
                    pet->GetMotionMaster()->Clear();
                    ((Pet*)pet)->SetStayPosition(true);
                    ((Pet*)pet)->SetIsRetreating();
                    ((Pet*)pet)->SetSpellOpener();
                    charmInfo->SetCommandState(COMMAND_STAY);
                    break;
                }
                case COMMAND_FOLLOW:                        // spellid=1792  // FOLLOW
                {
                    pet->StopMoving();
                    pet->AttackStop(true);
                    pet->GetMotionMaster()->Clear();
                    ((Pet*)pet)->SetStayPosition();
                    ((Pet*)pet)->SetIsRetreating(true);
                    ((Pet*)pet)->SetSpellOpener();
                    charmInfo->SetCommandState(COMMAND_FOLLOW);
                    break;
                }
                case COMMAND_ATTACK:                        // spellid=1792  // ATTACK
                {
                    ((Pet*)pet)->SetIsRetreating();
                    ((Pet*)pet)->SetSpellOpener();

                    Unit* targetUnit = targetGuid ? _player->GetMap()->GetUnit(targetGuid) : NULL;

                    if (targetUnit && targetUnit != pet && targetUnit->IsTargetableForAttack() && targetUnit->isInAccessablePlaceFor((Creature*)pet))
                    {
                        _player->SetInCombatState(true, targetUnit);

                        // This is true if pet has no target or has target but targets differs.
                        if (pet->getVictim() != targetUnit)
                        {
                            pet->AttackStop();
                            pet->GetMotionMaster()->Clear();

                            if (((Creature*)pet)->AI())
                            {
                                ((Creature*)pet)->AI()->AttackStart(targetUnit);
                                 // 10% chance to play special warlock pet attack talk, else growl
                                if (((Creature*)pet)->IsPet() && ((Pet*)pet)->getPetType() == SUMMON_PET && roll_chance_i(10))
                                {
                                    pet->SendPetTalk((uint32)PET_TALK_ATTACK);
                                }

                                pet->SendPetAIReaction();
                            }
                            else
                            {
                                pet->Attack(targetUnit, true);
                            }
                        }
                    }

                    break;
                }
                case COMMAND_ABANDON:                       // abandon (hunter pet) or dismiss (summoned pet)
                {
                    Creature* petC = (Creature*)pet;
                    if (petC->IsPet())
                    {
                        Pet* p = (Pet*)petC;
                        if (p->getPetType() == HUNTER_PET)
                        {
                            p->Unsummon(PET_SAVE_AS_DELETED, _player);
                            return;
                        }
                        else
                        {
                            // dismissing a summoned pet is like killing them (this prevents returning a soulshard...)
                            p->SetDeathState(CORPSE);
                        }
                    }
                    else                                    // charmed
                    {
                        _player->Uncharm();
                    }

                    if (petC->IsTemporarySummon()) // special case when pet was temporary summon through DoSummonPossesed
                    {
                        petC->ForcedDespawn();
                        return;
                    }

                    ((Pet*)pet)->SetStayPosition();
                    break;
                }
                default:
                    sLog.outError("WORLD: unknown PET flag Action %i and spellid %i.", uint32(flag), spellid);
            }
            break;
        case ACT_REACTION:                                  // 0x6
            switch (spellid)
            {
                case REACT_PASSIVE:                         // passive
                {
                    pet->AttackStop(true);
                    ((Pet*)pet)->SetSpellOpener();
                }
                // fall through
                case REACT_DEFENSIVE:                       // recovery
                case REACT_AGGRESSIVE:                      // activete
                {
                    charmInfo->SetReactState(ReactStates(spellid));
                    break;
                }
            }
            break;
        case ACT_DISABLED:                                  // 0x81    spell (disabled), ignore
        case ACT_PASSIVE:                                   // 0x01
        case ACT_ENABLED:                                   // 0xC1    spell
        {
            ((Pet*)pet)->SetIsRetreating();
            ((Pet*)pet)->SetSpellOpener();

            Unit* unit_target = targetGuid ? _player->GetMap()->GetUnit(targetGuid) : NULL;

            // do not cast unknown spells
            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellid);
            if (!spellInfo)
            {
                sLog.outError("WORLD: unknown PET spell id %i", spellid);
                return;
            }

            if (pet->GetCharmInfo() && pet->GetCharmInfo()->GetGlobalCooldownMgr().HasGlobalCooldown(spellInfo))
            {
                return;
            }

            for (int i = 0; i < MAX_EFFECT_INDEX; ++i)
            {
                SpellEffectEntry const* spellEffect = spellInfo->GetSpellEffect(SpellEffectIndex(i));
                if (!spellEffect)
                {
                    continue;
                }

                if (spellEffect->ImplicitTarget_0 == TARGET_ALL_ENEMY_IN_AREA
                    || spellEffect->ImplicitTarget_0 == TARGET_ALL_ENEMY_IN_AREA_INSTANT
                    || spellEffect->ImplicitTarget_0 == TARGET_ALL_ENEMY_IN_AREA_CHANNELED)
                    return;
            }

            // do not cast not learned spells
            if (!pet->HasSpell(spellid) || IsPassiveSpell(spellInfo))
            {
                return;
            }

            _player->SetInCombatState(true, unit_target);

            pet->clearUnitState(UNIT_STAT_MOVING);

            Spell* spell = new Spell(pet, spellInfo, false);

            SpellCastResult result = spell->CheckPetCast(unit_target);

            const SpellRangeEntry* sRange = sSpellRangeStore.LookupEntry(spellInfo->RangeIndex);

            if (unit_target && !(InReach(*pet, *unit_target, sRange->RangeMax_0) && HasLineOfSight(*pet, *unit_target))
                && !(GetPlayer()->IsFriendlyTo(unit_target) || pet->HasAuraType(SPELL_AURA_MOD_POSSESS)))
            {
                ((Pet*)pet)->SetSpellOpener(spellid, sRange->RangeMin_0, sRange->RangeMax_0);
                spell->finish(false);
                delete spell;

                pet->AttackStop();
                pet->GetMotionMaster()->Clear();

                ((Creature*)pet)->AI()->AttackStart(unit_target);
                 // 10% chance to play special warlock pet attack talk, else growl
                if (((Creature*)pet)->IsPet() && ((Pet*)pet)->getPetType() == SUMMON_PET && pet != unit_target && roll_chance_i(10))
                {
                    pet->SendPetTalk((uint32)PET_TALK_ATTACK);
                }

                pet->SendPetAIReaction();

                return;
            }

            // auto turn to target unless possessed
            if (result == SPELL_FAILED_UNIT_NOT_INFRONT && !pet->HasAuraType(SPELL_AURA_MOD_POSSESS))
            {
                if (unit_target)
                {
                    pet->SetInFront(unit_target);
                    if (unit_target->GetTypeId() == TYPEID_PLAYER)
                    {
                        pet->SendCreateUpdateToPlayer((Player*)unit_target);
                    }
                }
                else if (Unit* unit_target2 = spell->m_targets.getUnitTarget())
                {
                    pet->SetInFront(unit_target2);
                    if (unit_target2->GetTypeId() == TYPEID_PLAYER)
                    {
                        pet->SendCreateUpdateToPlayer((Player*)unit_target2);
                    }
                }
                if (Unit* powner = pet->GetCharmerOrOwner())
                    if (powner->GetTypeId() == TYPEID_PLAYER)
                    {
                        pet->SendCreateUpdateToPlayer((Player*)powner);
                    }
                result = SPELL_CAST_OK;
            }

            if (result == SPELL_CAST_OK)
            {
                ((Creature*)pet)->AddCreatureSpellCooldown(spellid);

                unit_target = spell->m_targets.getUnitTarget();

                ((Pet*)pet)->SetSpellOpener();
                spell->SpellStart(&(spell->m_targets));
            }
            else
            {
                if (pet->HasAuraType(SPELL_AURA_MOD_POSSESS))
                {
                    Spell::SendCastResult(GetPlayer(), spellInfo, 0, result);
                }
                else
                {
                    Unit* owner = pet->GetCharmerOrOwner();
                    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
                    {
                        Spell::SendCastResult((Player*)owner, spellInfo, 0, result, true);
                    }
                }

                if (!((Creature*)pet)->HasSpellCooldown(spellid))
                {
                    GetPlayer()->SendClearCooldown(spellid, pet);
                }

                ((Pet*)pet)->SetSpellOpener();
                spell->finish(false);
                delete spell;
            }
            break;
        }
        default:
            sLog.outError("WORLD: unknown PET flag Action %i and spellid %i.", uint32(flag), spellid);
    }
}

/**
 * @brief Handles a request to stop the active pet attack.
 *
 * @param recv_data The incoming stop-attack packet.
 */
void WorldSession::HandlePetStopAttack(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_PET_STOP_ATTACK");

    ObjectGuid petGuid;
    recv_data >> petGuid;

    Unit* pet = GetPlayer()->GetMap()->GetUnit(petGuid);    // pet or controlled creature/player
    if (!pet)
    {
        sLog.outError("%s doesn't exist.", petGuid.GetString().c_str());
        return;
    }

    if (GetPlayer()->GetObjectGuid() != pet->GetCharmerOrOwnerGuid())
    {
        sLog.outError("HandlePetStopAttack: %s isn't charm/pet of %s.", petGuid.GetString().c_str(), GetPlayer()->GetGuidStr().c_str());
        return;
    }

    if (!pet->IsAlive())
    {
        return;
    }

    pet->AttackStop();
}

/**
 * @brief Handles a client request for a pet name query.
 *
 * @param recv_data The incoming pet name query packet.
 */
void WorldSession::HandlePetNameQueryOpcode(WorldPacket& recv_data)
{
    DETAIL_LOG("HandlePetNameQuery. CMSG_PET_NAME_QUERY");

    uint32 petnumber;
    ObjectGuid petguid;

    recv_data >> petnumber;
    recv_data >> petguid;

    SendPetNameQuery(petguid, petnumber);
}

/**
 * @brief Sends pet name data for a specific pet number and guid.
 *
 * @param petguid The pet guid.
 * @param petnumber The pet number.
 */
void WorldSession::SendPetNameQuery(ObjectGuid petguid, uint32 petnumber)
{
    Creature* pet = _player->GetMap()->GetAnyTypeCreature(petguid);
    if (!pet || !pet->GetCharmInfo() || pet->GetCharmInfo()->GetPetNumber() != petnumber)
    {
        WorldPacket data(SMSG_PET_NAME_QUERY_RESPONSE, (4 + 1 + 4 + 1));
        data << uint32(petnumber);
        data << uint8(0);
        data << uint32(0);
        data << uint8(0);
        _player->GetSession()->SendPacket(&data);
        return;
    }

    char const* name = pet->GetName();

    // creature pets have localization like other creatures
    if (!pet->GetOwnerGuid().IsPlayer())
    {
        int loc_idx = GetSessionDbLocaleIndex();
        sObjectMgr.GetCreatureLocaleStrings(pet->GetEntry(), loc_idx, &name);
    }

    WorldPacket data(SMSG_PET_NAME_QUERY_RESPONSE, (4 + 4 + strlen(name) + 1));
    data << uint32(petnumber);
    data << name;
    data << uint32(pet->GetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP));

    if (pet->IsPet() && ((Pet*)pet)->GetDeclinedNames())
    {
        data << uint8(1);
        for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        {
            data << ((Pet*)pet)->GetDeclinedNames()->name[i];
        }
    }
    else
    {
        data << uint8(0);
    }

    _player->GetSession()->SendPacket(&data);
}

/**
 * @brief Handles updates to the pet action bar layout and autocast states.
 *
 * @param recv_data The incoming pet set-action packet.
 */
void WorldSession::HandlePetSetAction(WorldPacket& recv_data)
{
    DETAIL_LOG("HandlePetSetAction. CMSG_PET_SET_ACTION");

    ObjectGuid petGuid;
    uint8  count;

    recv_data >> petGuid;

    Creature* pet = _player->GetMap()->GetAnyTypeCreature(petGuid);

    if (!pet || (pet != _player->GetPet() && pet != _player->GetCharm()))
    {
        sLog.outError("HandlePetSetAction: Unknown pet or pet owner.");
        return;
    }

    // pet can have action bar disabled
    if (pet->IsPet() && ((Pet*)pet)->GetModeFlags() & PET_MODE_DISABLE_ACTIONS)
    {
        return;
    }

    CharmInfo* charmInfo = pet->GetCharmInfo();
    if (!charmInfo)
    {
        sLog.outError("WorldSession::HandlePetSetAction: object (GUID: %u TypeId: %u) is considered pet-like but doesn't have a charminfo!", pet->GetGUIDLow(), pet->GetTypeId());
        return;
    }

    count = (recv_data.size() == 24) ? 2 : 1;

    uint32 position[2];
    uint32 data[2];
    bool move_command = false;

    for (uint8 i = 0; i < count; ++i)
    {
        recv_data >> position[i];
        recv_data >> data[i];

        uint8 act_state = UNIT_ACTION_BUTTON_TYPE(data[i]);

        // ignore invalid position
        if (position[i] >= MAX_UNIT_ACTION_BAR_INDEX)
        {
            return;
        }

        // in the normal case, command and reaction buttons can only be moved, not removed
        // at moving count ==2, at removing count == 1
        // ignore attempt to remove command|reaction buttons (not possible at normal case)
        if (act_state == ACT_COMMAND || act_state == ACT_REACTION)
        {
            if (count == 1)
            {
                return;
            }

            move_command = true;
        }
    }

    // check swap (at command->spell swap client remove spell first in another packet, so check only command move correctness)
    if (move_command)
    {
        uint8 act_state_0 = UNIT_ACTION_BUTTON_TYPE(data[0]);
        if (act_state_0 == ACT_COMMAND || act_state_0 == ACT_REACTION)
        {
            uint32 spell_id_0 = UNIT_ACTION_BUTTON_ACTION(data[0]);
            UnitActionBarEntry const* actionEntry_1 = charmInfo->GetActionBarEntry(position[1]);
            if (!actionEntry_1 || spell_id_0 != actionEntry_1->GetAction() ||
                    act_state_0 != actionEntry_1->GetType())
                return;
        }

        uint8 act_state_1 = UNIT_ACTION_BUTTON_TYPE(data[1]);
        if (act_state_1 == ACT_COMMAND || act_state_1 == ACT_REACTION)
        {
            uint32 spell_id_1 = UNIT_ACTION_BUTTON_ACTION(data[1]);
            UnitActionBarEntry const* actionEntry_0 = charmInfo->GetActionBarEntry(position[0]);
            if (!actionEntry_0 || spell_id_1 != actionEntry_0->GetAction() ||
                    act_state_1 != actionEntry_0->GetType())
                return;
        }
    }

    for (uint8 i = 0; i < count; ++i)
    {
        uint32 spell_id = UNIT_ACTION_BUTTON_ACTION(data[i]);
        uint8 act_state = UNIT_ACTION_BUTTON_TYPE(data[i]);

        DETAIL_LOG("Player %s has changed pet spell action. Position: %u, Spell: %u, State: 0x%X", _player->GetName(), position[i], spell_id, uint32(act_state));

        // if it's act for spell (en/disable/cast) and there is a spell given (0 = remove spell) which pet doesn't know, don't add
        if (!((act_state == ACT_ENABLED || act_state == ACT_DISABLED || act_state == ACT_PASSIVE) && spell_id && !pet->HasSpell(spell_id)))
        {
            // sign for autocast
            if (act_state == ACT_ENABLED && spell_id)
            {
                if (pet->IsCharmed())
                {
                    charmInfo->ToggleCreatureAutocast(spell_id, true);
                }
                else
                {
                    ((Pet*)pet)->ToggleAutocast(spell_id, true);
                }
            }
            // sign for no/turn off autocast
            else if (act_state == ACT_DISABLED && spell_id)
            {
                if (pet->IsCharmed())
                {
                    charmInfo->ToggleCreatureAutocast(spell_id, false);
                }
                else
                {
                    ((Pet*)pet)->ToggleAutocast(spell_id, false);
                }
            }

            charmInfo->SetActionBar(position[i], spell_id, ActiveStates(act_state));
        }
    }
}

/**
 * @brief Handles hunter pet renaming and persists the new name.
 *
 * @param recv_data The incoming pet rename packet.
 */
void WorldSession::HandlePetRename(WorldPacket& recv_data)
{
    DETAIL_LOG("HandlePetRename. CMSG_PET_RENAME");

    ObjectGuid petGuid;
    uint8 isdeclined;

    std::string name;
    DeclinedName declinedname;

    recv_data >> petGuid;
    recv_data >> name;
    recv_data >> isdeclined;

    Pet* pet = _player->GetMap()->GetPet(petGuid);
    // check it!
    if (!pet || pet->getPetType() != HUNTER_PET ||
            !pet->HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED) ||
            pet->GetOwnerGuid() != _player->GetObjectGuid() || !pet->GetCharmInfo())
        return;

    PetNameInvalidReason res = ObjectMgr::CheckPetName(name);
    if (res != PET_NAME_SUCCESS)
    {
        SendPetNameInvalid(res, name, NULL);
        return;
    }

    if (sObjectMgr.IsReservedName(name))
    {
        SendPetNameInvalid(PET_NAME_RESERVED, name, NULL);
        return;
    }

    pet->SetName(name);

    if (_player->GetGroup())
    {
        _player->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_NAME);
    }

    pet->RemoveByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);

    if (isdeclined)
    {
        for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        {
            recv_data >> declinedname.name[i];
        }

        std::wstring wname;
        Utf8toWStr(name, wname);
        if (!ObjectMgr::CheckDeclinedNames(GetMainPartOfName(wname, 0), declinedname))
        {
            SendPetNameInvalid(PET_NAME_DECLENSION_DOESNT_MATCH_BASE_NAME, name, &declinedname);
            return;
        }
    }

    CharacterDatabase.BeginTransaction();
    if (isdeclined)
    {
        for (int i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        {
            CharacterDatabase.escape_string(declinedname.name[i]);
        }
        CharacterDatabase.PExecute("DELETE FROM `character_pet_declinedname` WHERE `owner` = '%u' AND `id` = '%u'", _player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber());
        CharacterDatabase.PExecute("INSERT INTO `character_pet_declinedname` (`id`, `owner`, `genitive`, `dative`, `accusative`, `instrumental`, `prepositional`) VALUES ('%u','%u','%s','%s','%s','%s','%s')",
                                   pet->GetCharmInfo()->GetPetNumber(), _player->GetGUIDLow(), declinedname.name[0].c_str(), declinedname.name[1].c_str(), declinedname.name[2].c_str(), declinedname.name[3].c_str(), declinedname.name[4].c_str());
    }

    CharacterDatabase.escape_string(name);
    CharacterDatabase.PExecute("UPDATE `character_pet` SET `name` = '%s', `renamed` = '1' WHERE `owner` = '%u' AND `id` = '%u'", name.c_str(), _player->GetGUIDLow(), pet->GetCharmInfo()->GetPetNumber());
    CharacterDatabase.CommitTransaction();

    pet->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
}

/**
 * @brief Handles abandoning or dismissing a pet or charm.
 *
 * @param recv_data The incoming pet abandon packet.
 */
void WorldSession::HandlePetAbandon(WorldPacket& recv_data)
{
    ObjectGuid guid;
    recv_data >> guid;                                      // pet guid

    DETAIL_LOG("HandlePetAbandon. CMSG_PET_ABANDON pet guid is %s", guid.GetString().c_str());

    if (!_player->IsInWorld())
    {
        return;
    }

    // pet/charmed
    if (Creature* pet = _player->GetMap()->GetAnyTypeCreature(guid))
    {
        if (pet->IsPet())
        {
            ((Pet*)pet)->Unsummon(PET_SAVE_AS_DELETED, _player);
        }
        else if (pet->GetObjectGuid() == _player->GetCharmGuid())
        {
            _player->Uncharm();
        }
    }
}

/**
 * @brief Handles hunter pet talent reset and passive relearning.
 *
 * @param recvPacket The incoming pet unlearn packet.
 */
void WorldSession::HandlePetUnlearnOpcode(WorldPacket& recvPacket)
{
    DETAIL_LOG("CMSG_PET_UNLEARN");

    ObjectGuid guid;
    recvPacket >> guid;                 // Pet guid

    Pet* pet = _player->GetPet();

    if (!pet || guid != pet->GetObjectGuid())
    {
        sLog.outError("HandlePetUnlearnOpcode. %s isn't pet of %s .", guid.GetString().c_str(), GetPlayer()->GetGuidStr().c_str());
        return;
    }

    if (pet->getPetType() != HUNTER_PET || pet->m_usedTalentCount == 0)
    {
        return;
    }

    CharmInfo* charmInfo = pet->GetCharmInfo();
    if (!charmInfo)
    {
        sLog.outError("WorldSession::HandlePetUnlearnOpcode: %s is considered pet-like but doesn't have a charminfo!", pet->GetGuidStr().c_str());
        return;
    }
    pet->resetTalents();
    _player->SendTalentsInfoData(true);
}

/**
 * @brief Handles pet spell autocast toggles.
 *
 * @param recvPacket The incoming pet autocast packet.
 */
void WorldSession::HandlePetSpellAutocastOpcode(WorldPacket& recvPacket)
{
    DETAIL_LOG("CMSG_PET_SPELL_AUTOCAST");

    ObjectGuid guid;
    uint32 spellid;
    uint8  state;                                           // 1 for on, 0 for off
    recvPacket >> guid >> spellid >> state;

    Creature* pet = _player->GetMap()->GetAnyTypeCreature(guid);
    if (!pet || (guid != _player->GetPetGuid() && guid != _player->GetCharmGuid()))
    {
        sLog.outError("HandlePetSpellAutocastOpcode. %s isn't pet of %s .", guid.GetString().c_str(), GetPlayer()->GetGuidStr().c_str());
        return;
    }

    // do not add not learned spells/ passive spells
    if (!pet->HasSpell(spellid) || IsPassiveSpell(spellid))
    {
        return;
    }

    CharmInfo* charmInfo = pet->GetCharmInfo();
    if (!charmInfo)
    {
        sLog.outError("WorldSession::HandlePetSpellAutocastOpcod: %s is considered pet-like but doesn't have a charminfo!", guid.GetString().c_str());
        return;
    }

    if (pet->IsCharmed())
        // state can be used as boolean
        pet->GetCharmInfo()->ToggleCreatureAutocast(spellid, state);
    else
    {
        ((Pet*)pet)->ToggleAutocast(spellid, state);
    }

    charmInfo->SetSpellAutocast(spellid, state);
}

/**
 * @brief Handles an explicit pet spell cast request.
 *
 * @param recvPacket The incoming pet cast-spell packet.
 */
void WorldSession::HandlePetCastSpellOpcode(WorldPacket& recvPacket)
{
    DETAIL_LOG("WORLD: CMSG_PET_CAST_SPELL");

    ObjectGuid guid;
    uint32 spellid;
    uint8  cast_count;
    uint8  cast_flags;                                      // flags (if 0x02 - some additional data are received)

    recvPacket >> guid >> cast_count >> spellid >> cast_flags;

    DEBUG_LOG("WORLD: CMSG_PET_CAST_SPELL, %s, cast_count: %u, spellid %u, cast_flags %u", guid.GetString().c_str(), cast_count, spellid, cast_flags);

    Creature* pet = _player->GetMap()->GetAnyTypeCreature(guid);

    if (!pet || (guid != _player->GetPetGuid() && guid != _player->GetCharmGuid()))
    {
        sLog.outError("HandlePetCastSpellOpcode: %s isn't pet of %s .", guid.GetString().c_str(), GetPlayer()->GetGuidStr().c_str());
        return;
    }

    SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellid);
    if (!spellInfo)
    {
        sLog.outError("WORLD: unknown PET spell id %i", spellid);
        return;
    }

    Aura* triggeredByAura = pet->GetTriggeredByClientAura(spellid);

    // do not cast not learned spells - checked before the served quest
    // spells below, so none of them can be cast from a vehicle that does
    // not carry it
    if ((!triggeredByAura && !pet->HasSpell(spellid)) || IsPassiveSpell(spellInfo))
    {
        return;
    }

    // 67063 Throw Torch, the Sacrifices (14212) ride. The button sits on the
    // vehicle bar, but retail has the RIDER throw: every capture cast carries
    // the player as caster, in the player's own cast sequence, and the throw
    // animation belongs to him - cast by the horse it plays on nothing and the
    // client leaves the rider frozen mid-wind-up. The vehicle stays the proper
    // caster for everything else on a vehicle bar (the Rebel Cannon fires its
    // own shot on retail), so this stays a single-spell allowance.
    if (spellid == 67063 && pet->IsVehicle() && pet->GetEntry() == 35231 &&
        pet->GetCharmerGuid() == _player->GetObjectGuid())
    {
        SpellCastTargets riderTargets;
        recvPacket >> riderTargets.ReadForCaster(_player);
        riderTargets.ReadAdditionalData(recvPacket, cast_flags);

        Spell* riderSpell = new Spell(_player, spellInfo, true, _player->GetObjectGuid());
        riderSpell->m_cast_count = cast_count;
        riderSpell->SpellStart(&riderTargets);
        return;
    }

    // 68659 Launch Me, the Two By Sea (14382) catapult ride. The player aims
    // and the client supplies the destination; retail answers with a vehicle
    // exit, the 8 s Launch safe-fall (66251, catapult as caster) and a
    // knockback along the aim bearing - horizontal speed a fixed 57.159, the
    // vertical solved so the arc lands on the aim point (the capture's
    // 18.267 for a 96.7 yd shot reproduces from exactly that equation). The
    // spell's own effects cannot carry any of this here: its targets are
    // TARGET_DEST_TRAJ and PASSENGER_0 chained through two 4.2+ internal
    // force-casts this core does not resolve, so the launch is served whole
    // at the point the aim arrives.
    if (spellid == 68659 && pet->IsVehicle() && pet->GetEntry() == 36283 &&
        pet->GetCharmerGuid() == _player->GetObjectGuid())
    {
        SpellCastTargets aimTargets;
        recvPacket >> aimTargets.ReadForCaster(pet);
        aimTargets.ReadAdditionalData(recvPacket, cast_flags);

        if (!(aimTargets.m_targetMask & TARGET_FLAG_DEST_LOCATION))
        {
            return;
        }

        float fx, fy, fz;
        aimTargets.getDestination(fx, fy, fz);

        // The wind-up and the boulder arc are the client's, from the cast.
        Spell* launchSpell = new Spell(pet, spellInfo, true, pet->GetObjectGuid());
        launchSpell->m_cast_count = cast_count;
        launchSpell->SpellStart(&aimTargets);

        // Out of the bucket, softly, and away along the aim. The bearing is
        // taken from the catapult - the exit spot is a yard off its side and
        // the teleport there is still in flight when the knockback is sent,
        // exactly the packet order the capture shows.
        pet->RemoveAurasDueToSpell(69434);                  // the seat
        pet->CastSpell(_player, 66251, true);               // Launch: safe fall

        float dx = fx - pet->GetPositionX();
        float dy = fy - pet->GetPositionY();
        float dist = sqrt(dx * dx + dy * dy);

        if (dist > 3.0f)
        {
            if (dist > 150.0f)                              // sanity for a hand-crafted aim
            {
                dist = 150.0f;
            }

            float dz = fz - pet->GetPositionZ();
            float ft = dist / 57.159f;
            float vz = dz / ft + 0.5f * float(Movement::gravity) * ft;
            SendKnockBack(atan2(dy, dx), 57.159f, vz);
        }

        return;
    }

    // 68903 Rope, The Hungry Ettin (14416). Mounted on a Mountain Horse the
    // player lassos the loose ones: the capture runs 68903 through two
    // internal triggers (68908, 68916) before the credit (68917) - a chain
    // this core does not resolve, so the lasso is served at cast-receive:
    // the roped horse takes the rope-attach visual, falls in behind the
    // ridden one, and the credit lands at once.
    if (spellid == 68903 && pet->IsVehicle() && pet->GetEntry() == 36540 &&
        pet->GetCharmerGuid() == _player->GetObjectGuid())
    {
        SpellCastTargets ropeTargets;
        recvPacket >> ropeTargets.ReadForCaster(pet);
        ropeTargets.ReadAdditionalData(recvPacket, cast_flags);

        Unit* pRoped = _player->GetMap()->GetUnit(ropeTargets.getUnitTargetGuid());

        // The Rope button carries no implicit target, so the client happily
        // presses it bare: no unit on the wire, and the lasso must find the
        // nearest loose horse itself.
        if (!pRoped || pRoped->GetTypeId() != TYPEID_UNIT ||
            pRoped->GetEntry() != 36540 || pRoped == pet ||
            !pRoped->GetCharmerGuid().IsEmpty() ||
            !pRoped->GetChannelObjectGuid().IsEmpty())
        {
            // The nearest LOOSE horse. Eligibility is tested per candidate:
            // horses already on the string trail at 3-6 yd, so the plain
            // nearest horse is always one of them after the first rope.
            pRoped = NULL;
            float fBest = 0.0f;

            std::list<Creature*> lHorses;
            MaNGOS::AllCreaturesOfEntryInRangeCheck u_check(pet, 36540, 30.0f);
            MaNGOS::CreatureListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(lHorses, u_check);
            Cell::VisitGridObjects(pet, searcher, 30.0f);

            for (std::list<Creature*>::const_iterator itr = lHorses.begin();
                 itr != lHorses.end(); ++itr)
            {
                Creature* pHorse = *itr;
                if (pHorse == pet || !pHorse->IsAlive() ||
                    !pHorse->GetCharmerGuid().IsEmpty() ||
                    !pHorse->GetChannelObjectGuid().IsEmpty() ||
                    pHorse->IsTemporarySummon())
                {
                    continue;
                }

                float fDist = pet->GetDistance(pHorse);
                if (!pRoped || fDist < fBest)
                {
                    pRoped = pHorse;
                    fBest = fDist;
                }
            }
        }

        if (pRoped && pRoped->GetTypeId() == TYPEID_UNIT &&
            pRoped->GetEntry() == 36540 && pRoped->IsAlive() &&
            pRoped != pet && pRoped->GetCharmerGuid().IsEmpty() &&
            pRoped->GetChannelObjectGuid().IsEmpty() &&
            !((Creature*)pRoped)->IsTemporarySummon() &&
            pet->IsWithinDistInMap(pRoped, 40.0f))
        {
            // The RIDER throws - the same lesson as the Sacrifices torch:
            // a vehicle-bar action that is the rider's own deed must be cast
            // by the player or the animation plays on the wrong body. 68908
            // carries the lasso-arc visual (17938); its serverside summon is
            // suppressed in EffectSummonType.
            _player->CastSpell(pRoped, 68908, true);

            // The visible rope. Every rope-beam spell in the client keeps
            // its rope in the CHANNEL visual kit alone - no state kit, so an
            // aura draws nothing and the client shows rope only from a unit
            // whose channel fields point at a target (this is why the TBC
            // ancestor is named "Wrangling Rope Channel"). The lassoed horse
            // therefore CHANNELS the lead-rope at its wrangler: one rope per
            // horse, drawn horse-to-rider, for as long as the fields stand.
            // The rope is a pure RENDERING reference: a spell id in the
            // channel field casts nothing - no aura, no area check, no
            // script anywhere can fire from it. The quest kit's own rope
            // (68908/16866) anchors at a rig the plain horse model lacks
            // and floats overhead; 46674's generic rope kit anchors the
            // horse correctly (seen in the earlier test - only its CAST
            // baggage was wrong, and none of that exists here).
            pRoped->SetChannelObjectGuid(_player->GetObjectGuid());
            pRoped->SetUInt32Value(UNIT_CHANNEL_SPELL, 46674);

            // Each horse takes its own slot in a loose wedge behind the
            // rider - with one shared follow spot (and no unit collision)
            // the whole string nested perfectly inside one horse model.
            uint32 uiSlot = 0;
            {
                std::list<Creature*> lString;
                MaNGOS::AllCreaturesOfEntryInRangeCheck u_stringCheck(pet, 36540, 60.0f);
                MaNGOS::CreatureListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> stringSearcher(lString, u_stringCheck);
                Cell::VisitGridObjects(pet, stringSearcher, 60.0f);

                for (std::list<Creature*>::const_iterator itr = lString.begin();
                     itr != lString.end(); ++itr)
                {
                    if ((*itr)->GetChannelObjectGuid() == _player->GetObjectGuid())
                    {
                        ++uiSlot;
                    }
                }
            }

            static const float ROPE_SLOT_ANGLE[5] = { 2.36f, 3.93f, 3.14f, 2.09f, 4.19f };
            static const float ROPE_SLOT_DIST[5]  = { 3.0f, 3.0f, 5.5f, 6.0f, 6.0f };
            const uint8 uiIdx = uiSlot % 5;

            // No Clear() first: an idle horse's motion stack can be empty
            // and DirectClean asserts on that. MoveFollow replaces the top.
            pRoped->GetMotionMaster()->MoveFollow(pet,
                ROPE_SLOT_DIST[uiIdx], ROPE_SLOT_ANGLE[uiIdx]);

            // No credit yet: retail registers the horses at DELIVERY - Lorna
            // "accepts the horses in 2s or 3s" - handled by the ridden
            // horse's script when the string reaches her (npc_mountain_horse).

            // The pasture restocks itself for the next wrangler.
            ((Creature*)pRoped)->ForcedDespawn(300000);
        }

        return;
    }

    if (pet->GetCharmInfo() && pet->GetCharmInfo()->GetGlobalCooldownMgr().HasGlobalCooldown(spellInfo))
    {
        return;
    }

    SpellCastTargets targets;

    recvPacket >> targets.ReadForCaster(pet);

    targets.ReadAdditionalData(recvPacket, cast_flags);

    pet->clearUnitState(UNIT_STAT_MOVING);

    Spell* spell = new Spell(pet, spellInfo, triggeredByAura ? true : false, pet->GetObjectGuid(), triggeredByAura ? triggeredByAura->GetSpellProto() : NULL);
    spell->m_cast_count = cast_count;                       // probably pending spell cast
    spell->m_targets = targets;

    SpellCastResult result = triggeredByAura ? SPELL_CAST_OK : spell->CheckPetCast(NULL);
    if (result == SPELL_CAST_OK)
    {
        pet->AddCreatureSpellCooldown(spellid);
        spell->SpellStart(&(spell->m_targets), triggeredByAura);
    }
    else
    {
        Unit* owner = pet->GetCharmerOrOwner();
        if (owner && owner->GetTypeId() == TYPEID_PLAYER && !triggeredByAura)
        {
            Spell::SendCastResult((Player*)owner, spellInfo, 0, result, true);
        }

        if (!pet->HasSpellCooldown(spellid) && !triggeredByAura)
        {
            GetPlayer()->SendClearCooldown(spellid, pet);
        }

        spell->finish(false);
        delete spell;
    }
}

/**
 * @brief Sends a pet-name-invalid response to the client.
 *
 * @param error The invalid-name error code.
 * @param name The rejected name.
 */
void WorldSession::SendPetNameInvalid(uint32 error, const std::string& name, DeclinedName* declinedName)
{
    WorldPacket data(SMSG_PET_NAME_INVALID, 4 + name.size() + 1 + 1);
    data << uint32(error);
    data << name;
    if (declinedName)
    {
        data << uint8(1);
        for (uint32 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
        {
            data << declinedName->name[i];
        }
    }
    else
    {
        data << uint8(0);
    }
    SendPacket(&data);
}

void WorldSession::HandlePetLearnTalent(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: CMSG_PET_LEARN_TALENT");

    ObjectGuid guid;
    uint32 talent_id, requested_rank;
    recv_data >> guid >> talent_id >> requested_rank;

    _player->LearnPetTalent(guid, talent_id, requested_rank);
    _player->SendTalentsInfoData(true);
}

void WorldSession::HandleLearnPreviewTalentsPet(WorldPacket& recv_data)
{
    DEBUG_LOG("CMSG_LEARN_PREVIEW_TALENTS_PET");

    ObjectGuid guid;
    recv_data >> guid;

    uint32 talentsCount;
    recv_data >> talentsCount;

    uint32 talentId, talentRank;

    for (uint32 i = 0; i < talentsCount; ++i)
    {
        recv_data >> talentId >> talentRank;

        _player->LearnPetTalent(guid, talentId, talentRank);
    }

    _player->SendTalentsInfoData(true);
}

void WorldSession::HandleDismissCritter(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;

    DEBUG_LOG("WORLD: Received CMSG_DISMISS_CRITTER for %s", guid.GetString().c_str());
    Unit* pet = _player->GetMap()->GetAnyTypeCreature(guid);
    if (!pet)
    {
        DEBUG_LOG("Vanitypet %s does not exist - Player %s (guid %u, account: %u) attempted to dismiss it (possibly lagged out)",
        guid.GetString().c_str(), GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), GetAccountId());
        return;
    }

    if (_player->GetCritterGuid() == guid)
    {
        if (pet->GetTypeId() == TYPEID_UNIT && ((Creature*)pet)->IsTemporarySummon())
        {
            ((TemporarySummon*)pet)->UnSummon();
        }
    }
}
