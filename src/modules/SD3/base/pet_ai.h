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

#ifndef SC_PET_H
#define SC_PET_H

// Using CreatureAI for now. Might change later and use PetAI (need to export for dll first)

/**
 * @class ScriptedPetAI
 * @brief Custom AI for scripted pets, derived from CreatureAI.
 */
class ScriptedPetAI : public CreatureAI
{
    public:
        /**
         * @brief Constructor for ScriptedPetAI.
         * @param pCreature Pointer to the creature this AI is associated with.
         */
        explicit ScriptedPetAI(Creature* pCreature);

        /**
         * @brief Destructor for ScriptedPetAI.
         */
        ~ScriptedPetAI() {}

        /**
         * @brief Called when a unit comes into the pet's line of sight.
         * @param pWho Pointer to the unit that came into sight.
         */
        void MoveInLineOfSight(Unit* /*pWho*/) override;

        /**
         * @brief Called when the pet starts attacking a unit.
         * @param pWho Pointer to the unit being attacked.
         */
        void AttackStart(Unit* /*pWho*/) override;

        /**
         * @brief Called when the pet is attacked by a unit.
         * @param pAttacker Pointer to the attacking unit.
         */
        void AttackedBy(Unit* /*pAttacker*/) override;

        /**
         * @brief Checks if a unit is visible to the pet.
         * @param pWho Pointer to the unit being checked.
         * @return True if the unit is visible, false otherwise.
         */
        bool IsVisible(Unit* /*pWho*/) const override;

        /**
         * @brief Called when the pet kills a unit.
         * @param pVictim Pointer to the killed unit.
         */
        void KilledUnit(Unit* /*pVictim*/) override {}

        /**
         * @brief Called when the pet's owner kills a unit.
         * @param pVictim Pointer to the killed unit.
         */
        void OwnerKilledUnit(Unit* /*pVictim*/) override {}

        /**
         * @brief Updates the pet's AI.
         * @param uiDiff Time since the last update.
         */
        void UpdateAI(const uint32 uiDiff) override;

        /**
         * @brief Resets the pet's state.
         */
        virtual void Reset() {}

        /**
         * @brief Updates the pet's AI while in combat.
         * @param uiDiff Time since the last update.
         */
        virtual void UpdatePetAI(const uint32 uiDiff);      // while in combat

        /**
         * @brief Updates the pet's AI while out of combat.
         * @param uiDiff Time since the last update.
         */
        virtual void UpdatePetOOCAI(const uint32 /*uiDiff*/) {} // when not in combat

    protected:
        /**
         * @brief Resets the pet's combat state.
         */
        void ResetPetCombat();
};

#endif
