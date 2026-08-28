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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include "TestHarness.h"

#include "WaypointSmoothing.h"

#include "Geometry/Vector3.h"

#include <vector>

using Geometry::Vector3;

TEST(WaypointSmoothing_rejects_a_packed_tail_that_collapses_onto_the_destination)
{
    // Spawn 177930 emitted a closed-loop spline whose final packed offset was
    // zero. Model a source tail longer than the existing 0.1-yard filter whose
    // components still truncate to zero at the packet's 0.25-yard resolution.
    const Vector3 destination(-9011.26f, -80.4445f, 86.8345f);
    std::vector<Vector3> path{
        destination,
        Vector3(-9014.26f, -82.9445f, 86.8345f),
        Vector3(-9015.26f, -83.6945f, 86.8345f),
        Vector3(-9014.51f, -83.1945f, 86.8345f),
        Vector3(destination.x + 0.12f, destination.y, destination.z),
        destination
    };

    CHECK((path.back() - path[path.size() - 2]).length() >=
          WAYPOINT_SMOOTHING_MIN_SEGMENT_LENGTH);
    CHECK(!IsWaypointSmoothingWireSafe(path, destination));

    // Moving that intermediate point beyond one packed unit preserves a real
    // final segment after client reconstruction and keeps the same loop valid.
    path[path.size() - 2].x = destination.x + 0.30f;
    CHECK(IsWaypointSmoothingWireSafe(path, destination));
}

TEST(WaypointSmoothing_uses_the_actual_launch_position_for_the_packing_grid)
{
    // PathFinder may project its first point away from the mover. Launch replaces
    // that point before PacketBuilder computes the midpoint, so validation must
    // use the same launch position or it can put the tail in a different cell.
    std::vector<Vector3> path{
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3(1.22f, 0.0f, 0.0f),
        Vector3(1.10f, 0.0f, 0.0f)
    };
    const Vector3 launchPosition(0.10f, 0.0f, 0.0f);

    CHECK((path.back() - path[path.size() - 2]).length() >=
          WAYPOINT_SMOOTHING_MIN_SEGMENT_LENGTH);
    CHECK(!IsWaypointSmoothingWireSafe(path, launchPosition));
}
