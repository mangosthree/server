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

#include "PathMovementGenerator.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "MotionFrame.h"
#include "movement/MoveSpline.h"

void PathMovementGenerator::Initialize(Unit& owner)
{
    if (owner.hasUnitState(UNIT_STAT_CAN_NOT_REACT | UNIT_STAT_NOT_MOVE))
    {
        return;
    }

    owner.StopMoving();
    owner.addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    // Resume is position-based: whatever happened while we were away, the
    // route continues at the nearest authored point still worth walking to.
    SkipPassedPoints(owner);
    ResetLeg();
}

void PathMovementGenerator::Reset(Unit& owner)
{
    Initialize(owner);
}

void PathMovementGenerator::Interrupt(Unit& owner)
{
    owner.InterruptMoving();
    owner.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
    ResetLeg();
}

void PathMovementGenerator::Finalize(Unit& owner)
{
    owner.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    // Only a route ridden to its end fires the inform.
    if (m_arrived && owner.GetTypeId() == TYPEID_UNIT)
    {
        Creature& creature = static_cast<Creature&>(owner);

        if (creature.AI())
        {
            creature.AI()->MovementInform(PATH_MOTION_TYPE, m_id);
        }
    }
}

void PathMovementGenerator::SkipPassedPoints(Unit& owner)
{
    if (m_points.empty())
    {
        m_next = 0;
        return;
    }

    // Nearest authored point from where the unit actually stands; walking
    // back to a point already behind us reads as a glitch.
    const Motion::Vector3 here = Motion::FrameFor(owner).MoverPosition(owner);

    uint32 best = 0;
    float bestSq = (m_points[0] - here).squaredLength();

    for (uint32 i = 1; i < m_points.size(); ++i)
    {
        const float dSq = (m_points[i] - here).squaredLength();

        if (dSq < bestSq)
        {
            best = i;
            bestSq = dSq;
        }
    }

    // Within two yards of the nearest point: it counts as passed.
    m_next = (bestSq < 4.0f) ? best + 1 : best;
}

Motion::MoveIntent PathMovementGenerator::Intent(Unit& owner,
                                                Motion::MoveStatus const& status,
                                                uint32 /*diff*/)
{
    if (owner.hasUnitState(UNIT_STAT_CAN_NOT_MOVE))
    {
        owner.clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return Motion::MoveIntent::Hold();
    }

    // The whole remaining route rides in one leg: its arrival ends the route.
    if (status.arrived)
    {
        m_arrived = true;
        return Motion::MoveIntent::Done();
    }

    if (status.traveling)
    {
        return Motion::MoveIntent::Hold();
    }

    // Nothing left to lay: an empty route, or a resume that found us at the end.
    if (m_next >= m_points.size())
    {
        m_arrived = true;
        return Motion::MoveIntent::Done();
    }

    owner.addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    // One spline: current position first (the spline contract), then every
    // remaining authored point verbatim.
    m_legPath.clear();
    m_legPath.reserve(m_points.size() - m_next + 1);
    m_legPath.push_back(Motion::FrameFor(owner).MoverPosition(owner));

    for (uint32 i = m_next; i < m_points.size(); ++i)
    {
        m_legPath.push_back(m_points[i]);
    }

    Motion::MoveIntent intent = Motion::MoveIntent::Move(
        m_legPath.back(), m_walk ? Motion::MOVE_WALK : Motion::MOVE_NONE);
    intent.path = &m_legPath;

    return intent;
}
