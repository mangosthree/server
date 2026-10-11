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

#ifndef MANGOS_FORMATIONMOVEMENTGENERATOR_H
#define MANGOS_FORMATIONMOVEMENTGENERATOR_H

#include "IntentMovementGenerator.h"
#include "ObjectGuid.h"
#include "Utilities/Timer.h"

/**
 * @brief A slot in a moving column: hold (angle, range) relative to a
 * leader's heading, re-deriving the spot on a fixed cadence and walking
 * each leg through the router.
 *
 * The distinction from Follow is the whole point. Follow chases the
 * leader's CURRENT position independently every tick, which in a crowd
 * knots the column and rubber-bands stragglers. A formation slot aims
 * where the leader is GOING - the final destination of his live spline -
 * so members swing wide, keep their spacing through turns, and arrive as
 * ranks. TrinityCore's FormationMovementGenerator works the same way, on
 * the same sniffed cadence (three batch update cycles).
 *
 * Every leg is routed with MOVE_REQUIRE_PATH: a member whose slot is
 * momentarily unroutable pauses a beat and retries, rather than walking
 * through a wall to keep station.
 */
class FormationMovementGenerator : public IntentMovementGenerator
{
    public:
        FormationMovementGenerator(Unit& leader, float range, float angle);

        void Initialize(Unit& owner) override;
        void Finalize(Unit& owner) override;
        void Interrupt(Unit& owner) override;
        void Reset(Unit& owner) override;

        MovementGeneratorType GetMovementGeneratorType() const override { return FORMATION_MOTION_TYPE; }

    protected:
        Motion::MoveIntent Intent(Unit& owner, Motion::MoveStatus const& status,
                                  uint32 diff) override;

    private:
        /// Where the slot sits right now: the leader's travel goal (or his
        /// position when standing), offset by (angle, range) from his heading.
        bool ComputeSlot(Unit& owner, Unit& leader, Motion::Vector3& out) const;

        /// Sniffed: the cadence retail updates formation members at.
        static const uint32 FORMATION_MOVE_INTERVAL = 1200;

        ObjectGuid m_leaderGuid;   ///< The column's spine.
        float m_range;             ///< Slot distance from the leader.
        float m_angle;             ///< Slot bearing relative to the leader's heading.
        TimeTracker m_nextMove{0}; ///< Cadence gate.
        Motion::Vector3 m_spot;    ///< Last slot laid.
        bool m_haveSpot;
};

#endif // MANGOS_FORMATIONMOVEMENTGENERATOR_H
