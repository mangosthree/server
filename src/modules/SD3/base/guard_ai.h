/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013 ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2025 MaNGOS <https://www.getmangos.eu>
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

#ifndef SC_GUARDAI_H
#define SC_GUARDAI_H

// Enumeration for generic creature cooldown and guard sayings
enum
{
    GENERIC_CREATURE_COOLDOWN       = 5000,  // Cooldown time for generic creature actions

    SAY_GUARD_SIL_AGGRO1            = -1000198,  // Guard aggro sayings
    SAY_GUARD_SIL_AGGRO2            = -1000199,
    SAY_GUARD_SIL_AGGRO3            = -1000200,

    NPC_CENARION_INFANTRY           = 15184  // NPC ID for Cenarion Infantry
};

// Enumeration for Shattrath guard spells
enum eShattrathGuard
{
    SPELL_BANISHED_SHATTRATH_A      = 36642,  // Spell ID for banished Shattrath (Aldor)
    SPELL_BANISHED_SHATTRATH_S      = 36671,  // Spell ID for banished Shattrath (Scryer)
    SPELL_BANISH_TELEPORT           = 36643,  // Spell ID for banish teleport
    SPELL_EXILE                     = 39533   // Spell ID for exile
};

/**
 * @class guardAI
 * @brief AI for generic guards.
 */
struct guardAI : public ScriptedAI
{
    public:
        /**
         * @brief Constructor for guardAI.
         * @param pCreature Pointer to the creature this AI is associated with.
         */
        explicit guardAI(Creature* pCreature);
        ~guardAI() {}

        uint32 m_uiGlobalCooldown;  // Global cooldown timer for guard actions
        uint32 m_uiBuffTimer;       // Timer for guard buffs

        /**
         * @brief Resets the guard's state.
         */
        void Reset() override;

        /**
         * @brief Called when the guard aggroes on a unit.
         * @param pWho Pointer to the unit being aggroed.
         */
        void Aggro(Unit* pWho) override;

        /**
         * @brief Called when the guard dies.
         * @param pKiller Pointer to the unit that killed the guard.
         */
        void JustDied(Unit* /*pKiller*/) override;

        /**
         * @brief Updates the guard's AI.
         * @param uiDiff Time since the last update.
         */
        void UpdateAI(const uint32 uiDiff) override;

        /**
         * @brief Responds to text emotes directed at the guard.
         * @param uiTextEmote The text emote ID.
         */
        void DoReplyToTextEmote(uint32 uiTextEmote);
};

/**
 * @class guardAI_orgrimmar
 * @brief AI for Orgrimmar guards.
 */
struct guardAI_orgrimmar : public guardAI
{
    /**
     * @brief Constructor for guardAI_orgrimmar.
     * @param pCreature Pointer to the creature this AI is associated with.
     */
    guardAI_orgrimmar(Creature* pCreature) : guardAI(pCreature) {}

    /**
     * @brief Called when the guard receives an emote from a player.
     * @param pPlayer Pointer to the player sending the emote.
     * @param uiTextEmote The text emote ID.
     */
    void ReceiveEmote(Player* pPlayer, uint32 uiTextEmote) override;
};

/**
 * @class guardAI_stormwind
 * @brief AI for Stormwind guards.
 */
struct guardAI_stormwind : public guardAI
{
    /**
     * @brief Constructor for guardAI_stormwind.
     * @param pCreature Pointer to the creature this AI is associated with.
     */
    guardAI_stormwind(Creature* pCreature) : guardAI(pCreature) {}

    /**
     * @brief Called when the guard receives an emote from a player.
     * @param pPlayer Pointer to the player sending the emote.
     * @param uiTextEmote The text emote ID.
     */
    void ReceiveEmote(Player* pPlayer, uint32 uiTextEmote) override;
};

#endif

