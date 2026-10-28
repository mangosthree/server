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

#include "FormationMovementGenerator.h"
#include "Creature.h"
#include "MotionFrame.h"
#include "ObjectLookup.h"
#include "movement/MoveSpline.h"

#include <cmath>

FormationMovementGenerator::FormationMovementGenerator(Unit& leader, float range, float angle)
    : m_leaderGuid(leader.GetObjectGuid()), m_range(range), m_angle(angle),
      m_haveSpot(false)
{
    m_nextMove.Reset(0);
}

void FormationMovementGenerator::Initialize(Unit& owner)
{
    owner.addUnitState(UNIT_STAT_ROAMING);

    // Guid-staggered first beat: fifty men whose cadence gates all opened
    // in the same tick re-laid legs in unison - the column moved like one
    // twitching organism.
    m_nextMove.Reset((owner.GetGUIDLow() % 13) * 90);
    m_haveSpot = false;
    ResetLeg();
}

void FormationMovementGenerator::Reset(Unit& owner)
{
    Initialize(owner);
}

void FormationMovementGenerator::Interrupt(Unit& owner)
{
    owner.InterruptMoving();
    owner.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
    ResetLeg();
}

void FormationMovementGenerator::Finalize(Unit& owner)
{
    owner.clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
}

bool FormationMovementGenerator::ComputeSlot(Unit& owner, Unit& leader,
                                             Motion::Vector3& out) const
{
    Motion::IMotionFrame const& frame = Motion::FrameFor(owner);
    const Motion::Vector3 leaderPos = frame.ObjectPosition(owner, leader);

    // Aim where the leader is going, not where he stands: the slot rides his
    // live spline's final destination, so the column swings through turns as
    // ranks instead of contracting into a knot behind him.
    Motion::Vector3 anchor = leaderPos;
    float heading = leader.Where().Facing();

    if (!leader.movespline->Finalized())
    {
        const Motion::Vector3 goal = leader.movespline->FinalDestination();

        if (goal.x != 0.0f || goal.y != 0.0f || goal.z != 0.0f)
        {
            const float dx = goal.x - leaderPos.x;
            const float dy = goal.y - leaderPos.y;
            const float dz = goal.z - leaderPos.z;
            const float dist = std::sqrt(dx * dx + dy * dy);

            if (dist > 0.5f)
            {
                heading = std::atan2(dy, dx);

                // A rolling point a few strides ahead, never the far end of a
                // long authored spline: slots must ride WITH the leader, not
                // route off on their own to where he will eventually arrive.
                const float lead = dist > 25.0f ? 25.0f / dist : 1.0f;
                anchor = Motion::Vector3(leaderPos.x + dx * lead,
                                         leaderPos.y + dy * lead,
                                         leaderPos.z + dz * lead);
            }
        }
    }

    out.x = anchor.x + std::cos(heading + m_angle) * m_range;
    out.y = anchor.y + std::sin(heading + m_angle) * m_range;
    out.z = anchor.z;

    return true;
}

Motion::MoveIntent FormationMovementGenerator::Intent(Unit& owner,
                                                      Motion::MoveStatus const& status,
                                                      uint32 diff)
{
    Unit* leader = ObjectLookup::GetUnit(owner, m_leaderGuid);

    // The column's spine is gone: this slot means nothing any more.
    if (!leader || !leader->IsInWorld() || !leader->IsAlive())
    {
        return Motion::MoveIntent::Done();
    }

    if (!owner.IsAlive() || owner.hasUnitState(UNIT_STAT_NOT_MOVE))
    {
        return Motion::MoveIntent::Hold();
    }

    // The cadence gate: slots re-derive on a fixed beat (the sniffed retail
    // interval), not per tick - and a blocked route simply waits for the
    // next beat instead of hammering the router.
    m_nextMove.Update(diff);

    if (!m_nextMove.Passed() && !status.arrived)
    {
        return Motion::MoveIntent::Hold();
    }

    m_nextMove.Reset(FORMATION_MOVE_INTERVAL);

    Motion::Vector3 spot;
    ComputeSlot(owner, *leader, spot);

    // Standing on a still-valid slot: rest until the leader moves the goalposts.
    const float drift = (spot - m_spot).squaredLength();

    // Re-lay only for a slot that moved MEANINGFULLY (>3 yd): sub-yard
    // corrections every beat read as a column of twitching bots.
    if (m_haveSpot && drift < 9.0f && status.traveling)
    {
        return Motion::MoveIntent::Hold();
    }

    if (m_haveSpot && !status.traveling && drift < 9.0f &&
        (Motion::FrameFor(owner).MoverPosition(owner) - spot).squaredLength() < 4.0f)
    {
        return Motion::MoveIntent::Hold();
    }

    m_spot = spot;
    m_haveSpot = true;

    owner.addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    uint32 flags = Motion::MOVE_REQUIRE_PATH;

    // The column matches its leader's gait.
    if (leader->IsWalking())
    {
        flags |= Motion::MOVE_WALK;
    }

    return Motion::MoveIntent::Move(spot, flags);
}
