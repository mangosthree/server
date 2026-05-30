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
