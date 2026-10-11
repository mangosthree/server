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

#ifndef MANGOS_PATHMOVEMENTGENERATOR_H
#define MANGOS_PATHMOVEMENTGENERATOR_H

#include "IntentMovementGenerator.h"

/**
 * @brief Authored-path playback: one multi-point spline over an explicit
 * point list, no routing.
 *
 * This is how the retail server itself runs scripted movement - its
 * MONSTER_MOVE packets carry whole waypoint lists digested from designer
 * paths - and it is what TrinityCore rebuilt as SplineChain "to allow
 * accurate replication of sniffed waypoints in static sequences". The
 * geometry is the authority: points come from a capture or a surveyed
 * route, already on walkable ground, and the unit rides them as ONE
 * spline instead of chaining MovePoint legs (no per-leg stutter, no
 * navmesh reinterpretation of a path that is already correct).
 *
 * On arrival the AI receives MovementInform(PATH_MOTION_TYPE, id). A leg
 * interrupted mid-flight (combat push, stun) resumes at the nearest
 * authored point still ahead, from the unit's actual position.
 */
class PathMovementGenerator : public IntentMovementGenerator
{
    public:
        PathMovementGenerator(uint32 id, Movement::PointsArray const& points, bool walk)
            : m_id(id), m_points(points), m_walk(walk), m_next(0), m_arrived(false) {}

        void Initialize(Unit& owner) override;
        void Finalize(Unit& owner) override;
        void Interrupt(Unit& owner) override;
        void Reset(Unit& owner) override;

        MovementGeneratorType GetMovementGeneratorType() const override { return PATH_MOTION_TYPE; }

    protected:
        Motion::MoveIntent Intent(Unit& owner, Motion::MoveStatus const& status,
                                  uint32 diff) override;

    private:
        /// Advance m_next past every authored point already behind the unit,
        /// so a resume continues the route instead of walking it backwards.
        void SkipPassedPoints(Unit& owner);

        uint32 m_id;                       ///< Echoed to the AI on arrival.
        Movement::PointsArray m_points;    ///< The authored route (world coords).
        bool m_walk;                       ///< Walk pace, else run.
        uint32 m_next;                     ///< First authored point not yet laid.
        bool m_arrived;                    ///< The route was ridden to its end.
        Movement::PointsArray m_legPath;   ///< [current pos + remaining points]; the laid leg.
};

#endif // MANGOS_PATHMOVEMENTGENERATOR_H
