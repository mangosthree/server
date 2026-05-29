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
