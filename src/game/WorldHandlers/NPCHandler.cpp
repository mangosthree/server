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
 * @file NPCHandler.cpp
 * @brief NPC interaction opcode handlers
 *
 * This file handles NPC-related opcodes including:
 * - CMSG_NPC_TEXT_QUERY: Query NPC text/gossip
 * - CMSG_GOSSIP_HELLO: Open gossip menu
 * - CMSG_GOSSIP_SELECT_OPTION: Select gossip option
 * - CMSG_NPC_WELCOME: NPC welcome
 * - CMSG_TABARD_VENDOR_ACTIVATE: Activate tabard vendor
 * - CMSG_BANKER_ACTIVATE: Activate banker
 * - CMSG_BUY_BANK_SLOT: Buy bank slot
 * - CMSG_TRAINER_LIST: Query trainer list
 * - CMSG_TRAINER_BUY_SPELL: Buy spell from trainer
 * - CMSG_PETITION_SHOW_LIST: Show petition list
 * - CMSG_PETITION_BUY: Buy petition
 * - CMSG_PETITION_SIGN: Sign petition
 * - CMSG_PETITION_QUERY: Query petition
 * - CMSG_OFFER_PETITION: Offer petition
 * - CMSG_TURN_IN_PETITION: Turn in petition
 * - CMSG_STABLE_PET: Stable pet
 * - CMSG_UNSTABLE_PET: Unstable pet
 * - CMSG_BUY_STABLE_SLOT: Buy stable slot
 */

#include "Common.h"
#include "Language.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "GossipDef.h"
#include "UpdateMask.h"
#include "ScriptMgr.h"
#include "Creature.h"
#include "Pet.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Chat.h"
#ifdef ENABLE_ELUNA
#include "LuaEngine.h"
#endif /* ENABLE_ELUNA */

enum StableResultCode
{
    STABLE_ERR_MONEY        = 0x01,                         // "you don't have enough money"
    STABLE_INVALID_SLOT     = 0x03,
    STABLE_ERR_STABLE       = 0x06,                         // currently used in most fail cases
    STABLE_SUCCESS_STABLE   = 0x08,                         // stable success
    STABLE_SUCCESS_UNSTABLE = 0x09,                         // unstable/swap success
    STABLE_SUCCESS_BUY_SLOT = 0x0A,                         // buy slot success
    STABLE_ERR_EXOTIC       = 0x0B,                         // "you are unable to control exotic creatures"
    STABLE_ERR_INTERNAL     = 0x0C,
};

/**
 * @brief Opens the tabard vendor interface for the selected NPC.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleTabardVendorActivateOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;
    recv_data >> guid;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TABARDDESIGNER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleTabardVendorActivateOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    SendTabardVendorActivate(guid);
}

/**
 * @brief Sends the tabard vendor activation packet.
 *
 * @param guid The tabard vendor guid.
 */
void WorldSession::SendTabardVendorActivate(ObjectGuid guid)
{
    WorldPacket data(MSG_TABARDVENDOR_ACTIVATE, 8);
    data << ObjectGuid(guid);
    SendPacket(&data);
}

/**
 * @brief Opens the bank window for the selected banker.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleBankerActivateOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;

    DEBUG_LOG("WORLD: Received opcode CMSG_BANKER_ACTIVATE");

    recv_data >> guid;

    if (!CheckBanker(guid))
    {
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    SendShowBank(guid);
}

/**
 * @brief Sends the bank window packet.
 *
 * @param guid The banker guid.
 */
void WorldSession::SendShowBank(ObjectGuid guid)
{
    WorldPacket data(SMSG_SHOW_BANK, 8);
    data << ObjectGuid(guid);
    SendPacket(&data);
}

void WorldSession::SendShowMailBox(ObjectGuid guid)
{
    WorldPacket data(SMSG_SHOW_MAILBOX, 8);
    data << ObjectGuid(guid);
    SendPacket(&data);
}

/**
 * @brief Requests the trainer list for a selected trainer.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleTrainerListOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;

    recv_data >> guid;

    SendTrainerList(guid);
}

/**
 * @brief Sends a trainer list using the default localized title.
 *
 * @param guid The trainer guid.
 */
void WorldSession::SendTrainerList(ObjectGuid guid)
{
    std::string str = GetMangosString(LANG_NPC_TAINER_HELLO);
    SendTrainerList(guid, str);
}

/**
 * @brief Writes a single trainer spell entry into a trainer list packet.
 *
 * @param data The packet being built.
 * @param tSpell The trainer spell entry.
 * @param state The trainer spell state for the player.
 * @param fDiscountMod The reputation price discount multiplier.
 * @param can_learn_primary_prof Whether a primary profession can still be learned.
 * @param reqLevel The required player level.
 */
static void SendTrainerSpellHelper(WorldPacket& data, TrainerSpell const* tSpell, TrainerSpellState state, float fDiscountMod, bool can_learn_primary_prof, uint32 reqLevel)
{
    bool primary_prof_first_rank = sSpellMgr.IsPrimaryProfessionFirstRankSpell(tSpell->learnedSpell);

    SpellChainNode const* chain_node = sSpellMgr.GetSpellChainNode(tSpell->learnedSpell);

    data << uint32(tSpell->spell);                      // learned spell (or cast-spell in profession case)
    data << uint8(state == TRAINER_SPELL_GREEN_DISABLED ? TRAINER_SPELL_GREEN : state);
    data << uint32(floor(tSpell->spellCost * fDiscountMod));
    data << uint8(reqLevel);
    data << uint32(tSpell->reqSkill);
    data << uint32(tSpell->reqSkillValue);
    data << uint32(primary_prof_first_rank && can_learn_primary_prof ? 1 : 0);
    // primary prof. learn confirmation dialog
    data << uint32(primary_prof_first_rank ? 1 : 0);    // must be equal prev. field to have learn button in enabled state
    data << uint32(!tSpell->IsCastable() && chain_node ? (chain_node->prev ? chain_node->prev : chain_node->req) : 0);
    data << uint32(!tSpell->IsCastable() && chain_node && chain_node->prev ? chain_node->req : 0);
}

/**
 * @brief Sends the trainer list with a custom title string.
 *
 * @param guid The trainer guid.
 * @param strTitle The title displayed in the trainer window.
 */
void WorldSession::SendTrainerList(ObjectGuid guid, const std::string& strTitle)
{
    DEBUG_LOG("WORLD: SendTrainerList");

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: SendTrainerList - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    // trainer list loaded at check;
    if (!unit->IsTrainerOf(_player, true))
    {
        return;
    }

    CreatureInfo const* ci = unit->GetCreatureInfo();
    if (!ci)
    {
        return;
    }

    TrainerSpellData const* cSpells = unit->GetTrainerSpells();
    TrainerSpellData const* tSpells = unit->GetTrainerTemplateSpells();

    if (!cSpells && !tSpells)
    {
        DEBUG_LOG("WORLD: SendTrainerList - Training spells not found for %s", guid.GetString().c_str());
        return;
    }

    uint32 maxcount = (cSpells ? cSpells->spellList.size() : 0) + (tSpells ? tSpells->spellList.size() : 0);
    uint32 TrainerType = cSpells && cSpells->trainerType ? cSpells->trainerType : (tSpells ? tSpells->trainerType : 0);

    WorldPacket data(SMSG_TRAINER_LIST, 8 + 4 + 4 + maxcount * 38 + strTitle.size() + 1);
    data << ObjectGuid(guid);
    data << uint32(TrainerType);
    data << uint32(ci->TrainerTemplateId);

    size_t count_pos = data.wpos();
    data << uint32(maxcount);

    // reputation discount
    float fDiscountMod = _player->GetReputationPriceDiscount(unit);
    bool can_learn_primary_prof = GetPlayer()->GetFreePrimaryProfessionPoints() > 0;

    uint32 count = 0;

    if (cSpells)
    {
        for (TrainerSpellMap::const_iterator itr = cSpells->spellList.begin(); itr != cSpells->spellList.end(); ++itr)
        {
            TrainerSpell const* tSpell = &itr->second;

            uint32 reqLevel = 0;
            if (!_player->IsSpellFitByClassAndRace(tSpell->learnedSpell, &reqLevel))
            {
                continue;
            }

            reqLevel = tSpell->isProvidedReqLevel ? tSpell->reqLevel : std::max(reqLevel, tSpell->reqLevel);

            TrainerSpellState state = _player->GetTrainerSpellState(tSpell, reqLevel);

            SendTrainerSpellHelper(data, tSpell, state, fDiscountMod, can_learn_primary_prof, reqLevel);

            ++count;
        }
    }

    if (tSpells)
    {
        for (TrainerSpellMap::const_iterator itr = tSpells->spellList.begin(); itr != tSpells->spellList.end(); ++itr)
        {
            TrainerSpell const* tSpell = &itr->second;

            uint32 reqLevel = 0;
            if (!_player->IsSpellFitByClassAndRace(tSpell->learnedSpell, &reqLevel))
            {
                continue;
            }

            reqLevel = tSpell->isProvidedReqLevel ? tSpell->reqLevel : std::max(reqLevel, tSpell->reqLevel);

            TrainerSpellState state = _player->GetTrainerSpellState(tSpell, reqLevel);

            SendTrainerSpellHelper(data, tSpell, state, fDiscountMod, can_learn_primary_prof, reqLevel);

            ++count;
        }
    }

    data << strTitle;

    data.put<uint32>(count_pos, count);
    SendPacket(&data);
}

/**
 * @brief Purchases and casts a trainer spell for the player.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleTrainerBuySpellOpcode(WorldPacket& recv_data)
{
    ObjectGuid guid;
    uint32 spellId = 0, TrainerTemplateId = 0;

    recv_data >> guid >> TrainerTemplateId >> spellId;
    DEBUG_LOG("WORLD: Received opcode CMSG_TRAINER_BUY_SPELL Trainer: %s, learn spell id is: %u", guid.GetString().c_str(), spellId);

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_TRAINER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleTrainerBuySpellOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    WorldPacket sendData(SMSG_TRAINER_SERVICE, 16);

    uint32 trainState = 2;

    if (!unit->IsTrainerOf(_player, true))
    {
        trainState = 1;
    }

    // check present spell in trainer spell list
    TrainerSpellData const* cSpells = unit->GetTrainerSpells();
    TrainerSpellData const* tSpells = unit->GetTrainerTemplateSpells();

    if (!cSpells && !tSpells)
    {
        trainState = 1;
    }

    // Try find spell in npc_trainer
    TrainerSpell const* trainer_spell = cSpells ? cSpells->Find(spellId) : NULL;

    // Not found, try find in npc_trainer_template
    if (!trainer_spell && tSpells)
    {
        trainer_spell = tSpells->Find(spellId);
    }

    // Not found anywhere, cheating?
    if (!trainer_spell)
    {
        trainState = 1;
    }

    // can't be learn, cheat? Or double learn with lags...
    uint32 reqLevel = 0;
    if (!_player->IsSpellFitByClassAndRace(trainer_spell->learnedSpell, &reqLevel))
    {
        trainState = 1;
    }

    reqLevel = trainer_spell->isProvidedReqLevel ? trainer_spell->reqLevel : std::max(reqLevel, trainer_spell->reqLevel);
    if (_player->GetTrainerSpellState(trainer_spell, reqLevel) != TRAINER_SPELL_GREEN)
    {
        trainState = 1;
    }

    // apply reputation discount
    uint32 nSpellCost = uint32(floor(trainer_spell->spellCost * _player->GetReputationPriceDiscount(unit)));

    // check money requirement
    if (_player->GetMoney() < nSpellCost && trainState > 1)
    {
        trainState = 0;
    }

    if (trainState != 2)
    {
        sendData << ObjectGuid(guid);
        sendData << uint32(spellId);
        sendData << uint32(trainState);
        SendPacket(&sendData);
    }
    else
    {
        _player->ModifyMoney(-int64(nSpellCost));

        // visual effect on trainer
        WorldPacket data;
        unit->BuildSendPlayVisualPacket(&data, 0xB3, false);
        SendPacket(&data);

        // visual effect on player
        _player->BuildSendPlayVisualPacket(&data, 0x016A, true);
        SendPacket(&data);

    // learn explicitly or cast explicitly
    // TODO - Are these spells really cast correctly this way?
    if (trainer_spell->IsCastable())
    {
        _player->CastSpell(_player, trainer_spell->spell, true);
    }
    else
    {
        _player->learnSpell(spellId, false);
    }

        sendData << ObjectGuid(guid);
        sendData << uint32(spellId);                                // should be same as in packet from client
        sendData << uint32(trainState);
        SendPacket(&sendData);
    }
}

/**
 * @brief Starts a gossip conversation with a creature.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleGossipHelloOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Received opcode CMSG_GOSSIP_HELLO");

    ObjectGuid guid;
    recv_data >> guid;

    Creature* pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!pCreature)
    {
        DEBUG_LOG("WORLD: HandleGossipHelloOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    pCreature->StopMoving();

    if (pCreature->IsSpiritGuide())
    {
        pCreature->SendAreaSpiritHealerQueryOpcode(_player);
    }

    if (!sScriptMgr.OnGossipHello(_player, pCreature))
    {
        _player->PrepareGossipMenu(pCreature, pCreature->GetCreatureInfo()->GossipMenuId);
        _player->SendPreparedGossip(pCreature);
    }
}

/**
 * @brief Handles selection of a gossip menu option.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleGossipSelectOptionOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: CMSG_GOSSIP_SELECT_OPTION");

    uint32 gossipListId;
    uint32 menuId;
    ObjectGuid guid;
    std::string code;

    recv_data >> guid >> menuId >> gossipListId;

    if (_player->PlayerTalkClass->GossipOptionCoded(gossipListId))
    {
        recv_data >> code;
        DEBUG_LOG("Gossip code: %s", code.c_str());
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    uint32 sender = _player->PlayerTalkClass->GossipOptionSender(gossipListId);
    uint32 action = _player->PlayerTalkClass->GossipOptionAction(gossipListId);

    if (guid.IsAnyTypeCreature())
    {
        Creature* pCreature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);

        if (!pCreature)
        {
            DEBUG_LOG("WORLD: HandleGossipSelectOptionOpcode - %s not found or you can't interact with it.", guid.GetString().c_str());
            return;
        }

        if (!sScriptMgr.OnGossipSelect(_player, pCreature, sender, action, code.empty() ? NULL : code.c_str()))
        {
            _player->OnGossipSelect(pCreature, gossipListId, menuId);
        }
    }
    else if (guid.IsGameObject())
    {
        GameObject* pGo = GetPlayer()->GetGameObjectIfCanInteractWith(guid);

        if (!pGo)
        {
            DEBUG_LOG("WORLD: HandleGossipSelectOptionOpcode - %s not found or you can't interact with it.", guid.GetString().c_str());
            return;
        }

        if (!sScriptMgr.OnGossipSelect(_player, pGo, sender, action, code.empty() ? NULL : code.c_str()))
        {
            _player->OnGossipSelect(pGo, gossipListId, menuId);
        }
    }
}

/**
 * @brief Activates a spirit healer resurrection.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleSpiritHealerActivateOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: CMSG_SPIRIT_HEALER_ACTIVATE");

    ObjectGuid guid;

    recv_data >> guid;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_SPIRITHEALER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleSpiritHealerActivateOpcode - %s not found or you can't interact with him.", guid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    SendSpiritResurrect();
}

/**
 * @brief Resurrects the player through a spirit healer.
 */
void WorldSession::SendSpiritResurrect()
{
    _player->ResurrectPlayer(0.5f, true);

    _player->DurabilityLossAll(0.25f, true);

    // get corpse nearest graveyard
    WorldSafeLocsEntry const* corpseGrave = NULL;
    Corpse* corpse = _player->GetCorpse();
    if (corpse)
        corpseGrave = sObjectMgr.GetClosestGraveYard(
                          corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ(), corpse->GetMapId(), _player->GetTeam());

    // now can spawn bones
    _player->SpawnCorpseBones();

    // teleport to nearest from corpse graveyard, if different from nearest to player ghost
    if (corpseGrave)
    {
        WorldSafeLocsEntry const* ghostGrave = sObjectMgr.GetClosestGraveYard(
                _player->GetPositionX(), _player->GetPositionY(), _player->GetPositionZ(), _player->GetMapId(), _player->GetTeam());

        if (corpseGrave != ghostGrave)
        {
            _player->TeleportTo(corpseGrave->map_id, corpseGrave->x, corpseGrave->y, corpseGrave->z, _player->GetOrientation());
        }
        // or update at original position
        else
        {
            _player->GetCamera().UpdateVisibilityForOwner();
            _player->UpdateObjectVisibility();
        }
    }
    // or update at original position
    else
    {
        _player->GetCamera().UpdateVisibilityForOwner();
        _player->UpdateObjectVisibility();
    }
}

void WorldSession::HandleReturnToGraveyardOpcode(WorldPacket& recv_data)
{
    Corpse* corpse = _player->GetCorpse();
    if (!corpse)
    {
        return;
    }

    WorldSafeLocsEntry const* corpseGrave = sObjectMgr.GetClosestGraveYard(corpse->GetPositionX(), corpse->GetPositionY(),
            corpse->GetPositionZ(), corpse->GetMapId(), _player->GetTeam());
    if (!corpseGrave)
    {
        return;
    }

    _player->TeleportTo(corpseGrave->map_id, corpseGrave->x, corpseGrave->y, corpseGrave->z, _player->GetOrientation());
}

/**
 * @brief Handles interaction with an innkeeper bind point.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleBinderActivateOpcode(WorldPacket& recv_data)
{
    ObjectGuid npcGuid;
    recv_data >> npcGuid;

    if (!GetPlayer()->IsInWorld() || !GetPlayer()->IsAlive())
    {
        return;
    }

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_INNKEEPER);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleBinderActivateOpcode - %s not found or you can't interact with him.", npcGuid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    SendBindPoint(unit);
}

/**
 * @brief Binds the player's home location at an innkeeper.
 *
 * @param npc The innkeeper creature.
 */
void WorldSession::SendBindPoint(Creature* npc)
{
    // prevent set homebind to instances in any case
    if (GetPlayer()->GetMap()->Instanceable())
    {
        return;
    }

    // send spell for bind 3286 bind magic
    npc->CastSpell(_player, 3286, true);                    // Bind

    WorldPacket data(SMSG_TRAINER_SERVICE, 16);
    data << npc->GetObjectGuid();
    data << uint32(3286);                                   // Bind
    data << uint32(2);
    SendPacket(&data);

    _player->PlayerTalkClass->CloseGossip();
}

/**
 * @brief Requests the list of stabled pets.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleListStabledPetsOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Recv MSG_LIST_STABLED_PETS");
    ObjectGuid npcGUID;

    recv_data >> npcGUID;

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    SendStablePet(npcGUID);
}

/**
 * @brief Sends the player's active and stabled pets.
 *
 * @param guid The stable master guid.
 */
void WorldSession::SendStablePet(ObjectGuid guid)
{
    DEBUG_LOG("WORLD: Recv MSG_LIST_STABLED_PETS Send.");

    // Cata 4.3.4 MSG_LIST_STABLED_PETS payload layout (mirrors TC):
    //   uint64  stablemaster guid
    //   uint8   pet count (N)
    //   uint8   slot count exposed to client (PET_SLOT_LAST_ACTIVE_SLOT + 1)
    //   N times:
    //     int32  petSlot              -- Call Pet 1..5 == slots 0..4
    //     uint32 petNumber            -- character_pet.id
    //     uint32 creatureEntry        -- character_pet.entry
    //     uint32 petLevel             -- character_pet.level
    //     string name                 -- character_pet.name
    //     uint8  flags                -- PET_STABLE_ACTIVE [| PET_STABLE_INACTIVE]
    //
    // The pre-Cata layout (no slot field, magic status byte 1/2/3) is
    // what the WotLK client expects; the Cata client misreads it field-
    // for-field and renders the wrong pet name/level/entry in each pane.
    WorldPacket data(MSG_LIST_STABLED_PETS, 200);           // guess size
    data << guid;

    Pet* pet = _player->GetPet();

    size_t wpos = data.wpos();
    data << uint8(0);                                       // place holder for pet count
    // Tell the client how many stable-slot panes to draw. TC sends
    // PET_SLOT_LAST_STABLE_SLOT (=20) literally; mirror that so the
    // 4.3.4 client renders the full stable grid and the drag-and-drop
    // targets in slots 5..20 are valid drop zones.
    data << uint8(PET_SLOT_LAST_STABLE_SLOT);

    uint8 num = 0;                                          // counter for place holder

    // not let move dead pet in slot
    if (pet && pet->IsAlive() && pet->getPetType() == HUNTER_PET)
    {
        // Use the pet's actual stored slot, not a hardcoded slot 0.
        // Under the Cata multi-pet model the currently summoned pet
        // can live in any active slot 0..PET_SLOT_LAST_ACTIVE_SLOT --
        // emitting the wrong slot here would render the active pet in
        // the wrong stable pane. Falls back to PET_SLOT_FIRST when
        // m_petSlot is -1 (pet object that has never round-tripped
        // through LoadPetFromDB or SavePetToDB -- pre-Cata single-pet
        // legacy path).
        int32 activeSlot = pet->GetSlot();
        if (activeSlot < int32(PET_SLOT_FIRST) || activeSlot > int32(PET_SLOT_LAST_ACTIVE_SLOT))
        {
            activeSlot = int32(PET_SLOT_FIRST);
        }
        data << int32(activeSlot);
        data << uint32(pet->GetCharmInfo()->GetPetNumber());
        data << uint32(pet->GetEntry());
        data << uint32(pet->getLevel());
        data << pet->GetName();
        data << uint8(PET_STABLE_ACTIVE);
        ++num;
    }

    // Query covers the entire active-roster range (slot 0..MAX) instead
    // of starting at PET_SAVE_FIRST_STABLE_SLOT. Pre-Cata the slot-0
    // branch was exclusively the currently summoned pet (one active pet
    // per character) so the legacy query intentionally skipped it.
    // Under Cata multi-pet a hunter can have multiple pets in
    // 0..PET_SLOT_LAST_ACTIVE_SLOT and any of them can be dismissed --
    // the dismissed ones still belong in the stable panel, just not as
    // the active pet. Excluding the currently summoned pet's id
    // prevents emitting it twice (once from the active branch above,
    // once from this query). When no pet is summoned the WHERE id <> 0
    // matches every row because character_pet.id is never zero.
    uint32 activePetId = pet ? pet->GetCharmInfo()->GetPetNumber() : 0;

    // Query the full Cata slot range 0..PET_SLOT_LAST_STABLE_SLOT
    // (=20), not just the legacy PET_SAVE_LAST_STABLE_SLOT
    // (=MAX_PET_STABLES). Slots 5..20 are stable-only positions the
    // Cata stable UI displays and the player drags pets into via
    // CMSG_SET_PET_SLOT. Restricting the query to 0..4 like the
    // pre-Cata code would make any stable-only pet vanish from the
    // panel the moment the player dragged it past slot 4.
    //                                                      0        1     2        3        4       5
    QueryResult* result = CharacterDatabase.PQuery("SELECT `owner`, `id`, `entry`, `level`, `name`, `slot` FROM `character_pet` WHERE `owner` = '%u' AND `slot` >= '%u' AND `slot` <= '%u' AND `id` <> '%u' ORDER BY `slot`",
                          _player->GetGUIDLow(), uint32(PET_SLOT_FIRST), uint32(PET_SLOT_LAST_STABLE_SLOT), activePetId);

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();

            // character_pet.slot 1..MAX_PET_STABLES maps directly onto
            // Call Pet 2..N+1 in the Cata client. Pets stored beyond the
            // Call Pet range get PET_STABLE_INACTIVE for forward parity,
            // even though 4.3.4 has no such slots today.
            uint32 petSlot = fields[5].GetUInt32();
            uint8 flags = PET_STABLE_ACTIVE;
            if (petSlot > uint32(PET_SLOT_LAST_ACTIVE_SLOT))
            {
                flags |= PET_STABLE_INACTIVE;
            }

            data << int32(petSlot);
            data << uint32(fields[1].GetUInt32());          // petnumber
            data << uint32(fields[2].GetUInt32());          // creature entry
            data << uint32(fields[3].GetUInt32());          // level
            data << fields[4].GetString();                  // name
            data << uint8(flags);

            ++num;
        }
        while (result->NextRow());

        delete result;
    }

    data.put<uint8>(wpos, num);                             // set real data to placeholder
    SendPacket(&data);
}

/**
 * @brief Sends a stable operation result code.
 *
 * @param res The stable result code.
 */
void WorldSession::SendStableResult(uint8 res)
{
    WorldPacket data(SMSG_STABLE_RESULT, 1);
    data << uint8(res);
    SendPacket(&data);
}

/**
 * @brief Verifies that a guid can be used as a stable master interaction target.
 *
 * @param guid The stable master or player guid.
 * @return true if stable interaction is allowed; otherwise false.
 */
bool WorldSession::CheckStableMaster(ObjectGuid guid)
{
    // spell case or GM
    if (guid == GetPlayer()->GetObjectGuid())
    {
        // command case will return only if player have real access to command
        if (!GetPlayer()->HasAuraType(SPELL_AURA_OPEN_STABLE) && !ChatHandler(GetPlayer()).FindCommand("stable"))
        {
            DEBUG_LOG("%s attempt open stable in cheating way.", guid.GetString().c_str());
            return false;
        }
    }
    // stable master case
    else
    {
        if (!GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_STABLEMASTER))
        {
            DEBUG_LOG("Stablemaster %s not found or you can't interact with him.", guid.GetString().c_str());
            return false;
        }
    }

    return true;
}

/**
 * @brief Moves a hunter pet between Call Pet 1..N slots (Cata payload).
 *
 * @param recv_data The received opcode packet.
 *
 * Cata 4.3.4 reuses the CMSG_STABLE_PET opcode value but ships a
 * completely different payload: a pet number, a destination slot
 * index, and a bit-packed stable-master guid. The drag-and-drop
 * stable UI sends one of these per move. The legacy WotLK semantics
 * (`stable my current pet into the first free stable slot`) are
 * gone -- there is no "stable" vs "active" distinction in 4.3.4
 * since every slot is callable from anywhere.
 *
 * Payload layout (matches TC-Preservation 4.3.4
 * `Handlers/NPCHandler.cpp::HandleSetPetSlot`):
 *   uint32  petId
 *   uint8   new_slot
 *   bits    stable-master guid mask  (3,2,0,7,5,6,1,4)
 *   bytes   stable-master guid bytes (5,3,1,7,4,0,6,2)
 */
void WorldSession::HandleStablePet(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Recv CMSG_STABLE_PET (Cata payload)");

    uint32 petId;
    uint8 new_slot;
    ObjectGuid guid;

    recv_data >> petId >> new_slot;

    recv_data.ReadGuidMask<3, 2, 0, 7, 5, 6, 1, 4>(guid);
    recv_data.ReadGuidBytes<5, 3, 1, 7, 4, 0, 6, 2>(guid);

    if (!GetPlayer()->IsAlive())
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    if (!CheckStableMaster(guid))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    // Slot range goes up to PET_SLOT_LAST_STABLE_SLOT (=20), not just
    // PET_SLOT_LAST_ACTIVE_SLOT (=4). The Cata client uses slots 0..4
    // for the Call Pet 1..5 active roster and 5..20 for stable-only
    // pets that need a stable master interaction to move out. Drag-
    // and-drop in the stable UI sends new_slot values across the full
    // 0..20 range -- a wire-capture of drag operations against
    // mangosthree confirmed values up to 0x0F. Reject only the
    // genuinely out-of-range case so the storage layer doesn't end
    // up with nonsensical slot indices.
    if (new_slot > uint8(PET_SLOT_LAST_STABLE_SLOT))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // Look up the source pet by petId. Verify it belongs to this
    // player so a forged packet can't reassign another character's
    // pet. Capture the creature entry and the current slot for the
    // tameable-exotic check and the swap logic below.
    QueryResult* result = CharacterDatabase.PQuery(
        "SELECT `entry`, `slot` FROM `character_pet` WHERE `owner` = '%u' AND `id` = '%u'",
        _player->GetGUIDLow(), petId);
    if (!result)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    Field* fields = result->Fetch();
    uint32 creatureEntry = fields[0].GetUInt32();
    int32 old_slot = int32(fields[1].GetUInt32());
    delete result;

    CreatureInfo const* creatureInfo = ObjectMgr::GetCreatureTemplate(creatureEntry);
    if (!creatureInfo || !creatureInfo->isTameable(true))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }
    if (!creatureInfo->isTameable(_player->CanTameExoticPets()))
    {
        SendStableResult(STABLE_ERR_EXOTIC);
        return;
    }

    // Retail Cata workflow: the currently summoned pet cannot have
    // its slot changed -- the player must dismiss it first. Reject
    // any drag operation whose source pet is the active in-world
    // pet so the client surfaces the "That slot is locked" message
    // via STABLE_INVALID_SLOT and the user knows to dismiss.
    // The displaced-pet equivalent of this check fires below, after
    // we know which pet (if any) sits at new_slot.
    Pet* pet = _player->GetPet();
    if (pet && pet->GetCharmInfo()->GetPetNumber() == petId)
    {
        SendStableResult(STABLE_INVALID_SLOT);
        return;
    }

    // No-op when the source pet is already at new_slot. Acknowledge
    // success so the client UI settles.
    if (old_slot == int32(new_slot))
    {
        SendStableResult(STABLE_SUCCESS_STABLE);
        SendStablePet(guid);
        return;
    }

    // Find the pet (if any) currently occupying new_slot, so we can
    // swap their slots atomically. character_pet has no unique key
    // on (owner, slot) so we have to walk it explicitly.
    QueryResult* swapResult = CharacterDatabase.PQuery(
        "SELECT `id` FROM `character_pet` WHERE `owner` = '%u' AND `slot` = '%u' AND `id` <> '%u'",
        _player->GetGUIDLow(), uint32(new_slot), petId);
    uint32 displacedPetId = 0;
    if (swapResult)
    {
        displacedPetId = swapResult->Fetch()[0].GetUInt32();
        delete swapResult;
    }

    // Symmetric to the active-pet check above: if the swap would
    // shove the currently summoned pet into a different slot, refuse
    // until the player dismisses it. The active pet's slot may not
    // change while it is in world.
    if (pet && displacedPetId && pet->GetCharmInfo()->GetPetNumber() == displacedPetId)
    {
        SendStableResult(STABLE_INVALID_SLOT);
        return;
    }

    CharacterDatabase.BeginTransaction();
    if (displacedPetId)
    {
        CharacterDatabase.PExecute(
            "UPDATE `character_pet` SET `slot` = '%u' WHERE `owner` = '%u' AND `id` = '%u'",
            uint32(old_slot), _player->GetGUIDLow(), displacedPetId);
    }
    CharacterDatabase.PExecute(
        "UPDATE `character_pet` SET `slot` = '%u' WHERE `owner` = '%u' AND `id` = '%u'",
        uint32(new_slot), _player->GetGUIDLow(), petId);
    CharacterDatabase.CommitTransaction();

    // Keep the in-world Pet's m_petSlot in sync so any subsequent
    // SavePetToDB(PET_SAVE_AS_CURRENT) writes the new slot back via
    // the multi-pet intercept. Cover both the case where the moved
    // pet is the active pet AND the case where the displaced pet
    // was the active pet.
    if (pet)
    {
        if (pet->GetCharmInfo()->GetPetNumber() == petId)
        {
            pet->SetSlot(int32(new_slot));
        }
        else if (displacedPetId && pet->GetCharmInfo()->GetPetNumber() == displacedPetId)
        {
            pet->SetSlot(old_slot);
        }
    }

    SendStableResult(STABLE_SUCCESS_STABLE);
    // Refresh the stable panel so the client sees the new layout
    // without waiting for the next interaction.
    SendStablePet(guid);
}

/**
 * @brief Restores a pet from the stable.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleUnstablePet(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Recv CMSG_UNSTABLE_PET.");
    ObjectGuid npcGUID;
    uint32 petnumber;

    recv_data >> npcGUID >> petnumber;

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    uint32 creature_id = 0;

    {
        QueryResult* result = CharacterDatabase.PQuery("SELECT `entry` FROM `character_pet` WHERE `owner` = '%u' AND `id` = '%u' AND `slot` >='%u' AND `slot` <= '%u'",
                              _player->GetGUIDLow(), petnumber, PET_SAVE_FIRST_STABLE_SLOT, PET_SAVE_LAST_STABLE_SLOT);
        if (result)
        {
            Field* fields = result->Fetch();
            creature_id = fields[0].GetUInt32();
            delete result;
        }
    }

    if (!creature_id)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    CreatureInfo const* creatureInfo = ObjectMgr::GetCreatureTemplate(creature_id);
    if (!creatureInfo || !creatureInfo->isTameable(_player->CanTameExoticPets()))
    {
        // if problem in exotic pet
        if (creatureInfo && creatureInfo->isTameable(true))
        {
            SendStableResult(STABLE_ERR_EXOTIC);
        }
        else
        {
            SendStableResult(STABLE_ERR_STABLE);
        }
        return;
    }

    Pet* pet = _player->GetPet();
    if (pet && pet->IsAlive())
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // delete dead pet
    if (pet)
    {
        pet->Unsummon(PET_SAVE_AS_DELETED, _player);
    }

    Pet* newpet = new Pet(HUNTER_PET);
    if (!newpet->LoadPetFromDB(_player, creature_id, petnumber))
    {
        delete newpet;
        newpet = NULL;
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    SendStableResult(STABLE_SUCCESS_UNSTABLE);
}

/**
 * @brief Buys an additional stable slot.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleBuyStableSlot(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Recv CMSG_BUY_STABLE_SLOT.");
    ObjectGuid npcGUID;

    recv_data >> npcGUID;

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }
}

/**
 * @brief Placeholder for stable pet revival handling.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleStableRevivePet(WorldPacket& recv_data)
{
    DEBUG_LOG("HandleStableRevivePet: CMSG_STABLE_REVIVE_PET");

    ObjectGuid npcGUID;
    recv_data >> npcGUID;

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // Revive all dead pets in the stable by setting their health to full
    // The character_pet table stores pet health; setting it to non-zero revives it
    CharacterDatabase.PExecute(
        "UPDATE `character_pet` SET `curhealth` = `maxhealth` "
        "WHERE `owner` = '%u' AND `curhealth` = 0 AND `slot` >= %u AND `slot` <= %u",
        _player->GetGUIDLow(), PET_SAVE_FIRST_STABLE_SLOT, PET_SAVE_LAST_STABLE_SLOT);

    // Also revive current pet if dead
    CharacterDatabase.PExecute(
        "UPDATE `character_pet` SET `curhealth` = `maxhealth` "
        "WHERE `owner` = '%u' AND `curhealth` = 0 AND `slot` = %u",
        _player->GetGUIDLow(), PET_SAVE_AS_CURRENT);

    SendStableResult(STABLE_SUCCESS_UNSTABLE);
}

/**
 * @brief Swaps the current pet with a stabled pet.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleStableSwapPet(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: Recv CMSG_STABLE_SWAP_PET.");
    ObjectGuid npcGUID;
    uint32 pet_number;

    recv_data >> npcGUID >> pet_number;

    if (!CheckStableMaster(npcGUID))
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    Pet* pet = _player->GetPet();

    if (!pet || pet->getPetType() != HUNTER_PET)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    // find swapped pet slot in stable
    QueryResult* result = CharacterDatabase.PQuery("SELECT `slot`,`entry` FROM `character_pet` WHERE `owner` = '%u' AND `id` = '%u'",
                          _player->GetGUIDLow(), pet_number);
    if (!result)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    Field* fields = result->Fetch();

    uint32 slot        = fields[0].GetUInt32();
    uint32 creature_id = fields[1].GetUInt32();
    delete result;

    if (!creature_id)
    {
        SendStableResult(STABLE_ERR_STABLE);
        return;
    }

    CreatureInfo const* creatureInfo = ObjectMgr::GetCreatureTemplate(creature_id);
    if (!creatureInfo || !creatureInfo->isTameable(_player->CanTameExoticPets()))
    {
        // if problem in exotic pet
        if (creatureInfo && creatureInfo->isTameable(true))
        {
            SendStableResult(STABLE_ERR_EXOTIC);
        }
        else
        {
            SendStableResult(STABLE_ERR_STABLE);
        }
        return;
    }

    // move alive pet to slot or delete dead pet
    pet->Unsummon(pet->IsAlive() ? PetSaveMode(slot) : PET_SAVE_AS_DELETED, _player);

    // summon unstabled pet
    Pet* newpet = new Pet;
    if (!newpet->LoadPetFromDB(_player, creature_id, pet_number))
    {
        delete newpet;
        SendStableResult(STABLE_ERR_STABLE);
    }
    else
    {
        SendStableResult(STABLE_SUCCESS_UNSTABLE);
    }
}

/**
 * @brief Repairs one item or all equipped gear at a repair NPC.
 *
 * @param recv_data The received opcode packet.
 */
void WorldSession::HandleRepairItemOpcode(WorldPacket& recv_data)
{
    DEBUG_LOG("WORLD: CMSG_REPAIR_ITEM");

    ObjectGuid npcGuid;
    ObjectGuid itemGuid;
    uint8 guildBank;                                        // new in 2.3.2, bool that means from guild bank money

    recv_data >> npcGuid >> itemGuid >> guildBank;

    Creature* unit = GetPlayer()->GetNPCIfCanInteractWith(npcGuid, UNIT_NPC_FLAG_REPAIR);
    if (!unit)
    {
        DEBUG_LOG("WORLD: HandleRepairItemOpcode - %s not found or you can't interact with him.", npcGuid.GetString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->hasUnitState(UNIT_STAT_DIED))
    {
        GetPlayer()->RemoveSpellsCausingAura(SPELL_AURA_FEIGN_DEATH);
    }

    // reputation discount
    float discountMod = _player->GetReputationPriceDiscount(unit);

    uint32 TotalCost = 0;
    if (itemGuid)
    {
        DEBUG_LOG("ITEM: %s repair of %s", npcGuid.GetString().c_str(), itemGuid.GetString().c_str());

        Item* item = _player->GetItemByGuid(itemGuid);

        if (item)
        {
            TotalCost = _player->DurabilityRepair(item->GetPos(), true, discountMod, (guildBank > 0));
        }
    }
    else
    {
        DEBUG_LOG("ITEM: %s repair all items", npcGuid.GetString().c_str());

        TotalCost = _player->DurabilityRepairAll(true, discountMod, (guildBank > 0));
    }
    if (guildBank)
    {
        uint32 GuildId = _player->GetGuildId();
        if (!GuildId)
        {
            return;
        }
        Guild* pGuild = sGuildMgr.GetGuildById(GuildId);
        if (!pGuild)
        {
            return;
        }
        pGuild->LogBankEvent(GUILD_BANK_LOG_REPAIR_MONEY, 0, _player->GetGUIDLow(), TotalCost);
        pGuild->SendMoneyInfo(this, _player->GetGUIDLow());
    }
}
