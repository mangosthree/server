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
 * @brief Applies personal and category cooldowns for a spell cast.
 *
 * @param spellInfo The spell entry that triggered the cooldown.
 * @param itemId The casting item entry, if any.
 * @param spell The active spell instance.
 * @param infinityCooldown True to apply a long-lived cooldown marker.
 */
void Player::AddSpellAndCategoryCooldowns(SpellEntry const* spellInfo, uint32 itemId, Spell* spell, bool infinityCooldown)
{
    // init cooldown values
    uint32 cat   = 0;
    int32 rec    = -1;
    int32 catrec = -1;

    // some special item spells without correct cooldown in SpellInfo
    // cooldown information stored in item prototype
    // This used in same way in WorldSession::HandleItemQuerySingleOpcode data sending to client.

    if (itemId)
    {
        if (ItemPrototype const* proto = ObjectMgr::GetItemPrototype(itemId))
        {
            for (int idx = 0; idx < MAX_ITEM_PROTO_SPELLS; ++idx)
            {
                if (proto->Spells[idx].SpellId == spellInfo->Id)
                {
                    cat    = proto->Spells[idx].SpellCategory;
                    rec    = proto->Spells[idx].SpellCooldown;
                    catrec = proto->Spells[idx].SpellCategoryCooldown;
                    break;
                }
            }
        }
    }

    // if no cooldown found above then base at DBC data
    if (rec < 0 && catrec < 0)
    {
        cat = spellInfo->GetCategory();
        rec = spellInfo->GetRecoveryTime();
        catrec = spellInfo->GetCategoryRecoveryTime();
    }

    time_t curTime = time(NULL);

    time_t catrecTime;
    time_t recTime;

    // overwrite time for selected category
    if (infinityCooldown)
    {
        // use +MONTH as infinity mark for spell cooldown (will checked as MONTH/2 at save ans skipped)
        // but not allow ignore until reset or re-login
        catrecTime = catrec > 0 ? curTime + infinityCooldownDelay : 0;
        recTime    = rec    > 0 ? curTime + infinityCooldownDelay : catrecTime;
    }
    else
    {
        // shoot spells used equipped item cooldown values already assigned in GetAttackTime(RANGED_ATTACK)
        // prevent 0 cooldowns set by another way
        if (rec <= 0 && catrec <= 0 && (cat == 76 || (IsAutoRepeatRangedSpell(spellInfo) && spellInfo->Id != SPELL_ID_AUTOSHOT)))
        {
            rec = GetAttackTime(RANGED_ATTACK);
        }

        // Now we have cooldown data (if found any), time to apply mods
        if (rec > 0)
        {
            ApplySpellMod(spellInfo->Id, SPELLMOD_COOLDOWN, rec, spell);
        }

        if (catrec > 0)
        {
            ApplySpellMod(spellInfo->Id, SPELLMOD_COOLDOWN, catrec, spell);
        }

        // replace negative cooldowns by 0
        if (rec < 0) rec = 0;
        {
            if (catrec < 0) catrec = 0;
        }

        // no cooldown after applying spell mods
        if (rec == 0 && catrec == 0)
        {
            return;
        }

        catrecTime = catrec ? curTime + catrec / IN_MILLISECONDS : 0;
        recTime    = rec ? curTime + rec / IN_MILLISECONDS : catrecTime;
    }

    // self spell cooldown
    if (recTime > 0)
    {
        AddSpellCooldown(spellInfo->Id, itemId, recTime);
    }

    // category spells
    if (cat && catrec > 0)
    {
        SpellCategoryStore::const_iterator i_scstore = sSpellCategoryStore.find(cat);
        if (i_scstore != sSpellCategoryStore.end())
        {
            for (SpellCategorySet::const_iterator i_scset = i_scstore->second.begin(); i_scset != i_scstore->second.end(); ++i_scset)
            {
                if (*i_scset == spellInfo->Id)              // skip main spell, already handled above
                {
                    continue;
                }

                AddSpellCooldown(*i_scset, itemId, catrecTime);
            }
        }
    }
}

/**
 * @brief Stores a cooldown entry for a spell.
 *
 * @param spellid The spell identifier.
 * @param itemid The associated item identifier, if any.
 * @param end_time The server time when the cooldown ends.
 */
void Player::AddSpellCooldown(uint32 spellid, uint32 itemid, time_t end_time)
{
    SpellCooldown sc;
    sc.end = end_time;
    sc.itemid = itemid;
    m_spellCooldowns[spellid] = sc;
}

/**
 * @brief Applies cooldowns and notifies the client about a spell cooldown event.
 *
 * @param spellInfo The spell entry that triggered the cooldown.
 * @param itemId The associated item entry, if any.
 * @param spell The active spell instance.
 */
void Player::SendCooldownEvent(SpellEntry const* spellInfo, uint32 itemId, Spell* spell)
{
    // start cooldowns at server side, if any
    AddSpellAndCategoryCooldowns(spellInfo, itemId, spell);

    // Send activate cooldown timer (possible 0) at client side
    WorldPacket data(SMSG_COOLDOWN_EVENT, (4 + 8));
    data << uint32(spellInfo->Id);
    data << GetObjectGuid();
    SendDirectMessage(&data);
}

void Player::UpdatePotionCooldown(Spell* spell)
{
    // no potion used in combat or still in combat
    if (!m_lastPotionId || IsInCombat())
    {
        return;
    }

    // Call not from spell cast, send cooldown event for item spells if no in combat
    if (!spell)
    {
        // spell/item pair let set proper cooldown (except nonexistent charged spell cooldown spellmods for potions)
        if (ItemPrototype const* proto = ObjectMgr::GetItemPrototype(m_lastPotionId))
            for (int idx = 0; idx < 5; ++idx)
                if (proto->Spells[idx].SpellId && proto->Spells[idx].SpellTrigger == ITEM_SPELLTRIGGER_ON_USE)
                    if (SpellEntry const* spellInfo = sSpellStore.LookupEntry(proto->Spells[idx].SpellId))
                    {
                        SendCooldownEvent(spellInfo, m_lastPotionId);
                    }
    }
    // from spell cases (m_lastPotionId set in Spell::SendSpellCooldown)
    else
    {
        SendCooldownEvent(spell->m_spellInfo, m_lastPotionId, spell);
    }

    m_lastPotionId = 0;
}
