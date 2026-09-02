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

#ifndef MANGOS_WAYPOINTSMOOTHING_H
#define MANGOS_WAYPOINTSMOOTHING_H

#include "Geometry/Vector3.h"
#include "Platform/Define.h"

#include <vector>

/**
 * @brief Safety ceiling on how many waypoints a single smoothed spline may span.
 *
 * The primary limiter is the distance budget below; this only guards against
 * pathological paths with huge numbers of tightly-packed nodes producing one
 * enormous spline/packet.
 */
constexpr size_t WAYPOINT_SMOOTHING_MAX_LOOKAHEAD = 32;

/**
 * @brief Maximum X/Y bounding-box span (in yards) of a single smoothed path.
 *
 * Smoothed splines are sent as a linear path in SMSG_MONSTER_MOVE, whose
 * intermediate points are encoded as offsets from the endpoint midpoint, packed at
 * 0.25yd granularity into signed 11-bit (X/Y) and 10-bit (Z) fields (see
 * ByteBuffer::appendPackXYZ / PacketBuilder::WriteLinearPath). That caps the
 * representable offset at roughly +/-256yd (X/Y) and +/-128yd (Z); beyond it
 * the value wraps and the client renders a wild jump. These spans stay well
 * inside those limits so no offset can wrap.
 */
constexpr float WAYPOINT_SMOOTHING_MAX_XY_SPAN = 200.0f;

/// Maximum Z bounding-box span (yards); see WAYPOINT_SMOOTHING_MAX_XY_SPAN.
constexpr float WAYPOINT_SMOOTHING_MAX_Z_SPAN = 100.0f;

/**
 * @brief Minimum length (in yards) of a segment in a smoothed multi-point spline.
 *
 * The 4.3.4 client cannot interpolate across a zero-length segment inside a
 * multi-point spline; a mover carrying one becomes selectable from anywhere.
 * Waypoint paths commonly duplicate their final node, so candidate points
 * closer than this to the previous path point are dropped during the merge.
 */
constexpr float WAYPOINT_SMOOTHING_MIN_SEGMENT_LENGTH = 0.1f;

/**
 * @brief Per-node properties that decide whether smoothing may pass through a waypoint.
 */
struct WaypointSmoothingNode
{
    bool hasDelay = false;    ///< Node has a non-zero wait time
    bool hasScript = false;   ///< Node triggers a movement script
    bool hasBehavior = false; ///< Node has emote/spell/model/text behavior
};

/**
 * @brief Decision for how an in-progress waypoint segment should be updated.
 */
enum class WaypointSegmentUpdateState
{
    Moving,   ///< Spline still running normally
    Finalized ///< Spline has completed
};

/**
 * @brief Running axis-aligned bounding box of a candidate smoothed path.
 *
 * Used to keep the path within the packable offset budget, see
 * WAYPOINT_SMOOTHING_MAX_XY_SPAN.
 */
struct WaypointSmoothingBounds
{
    float minX = 0.0f;        ///< Minimum X seen
    float maxX = 0.0f;        ///< Maximum X seen
    float minY = 0.0f;        ///< Minimum Y seen
    float maxY = 0.0f;        ///< Maximum Y seen
    float minZ = 0.0f;        ///< Minimum Z seen
    float maxZ = 0.0f;        ///< Maximum Z seen
    bool initialized = false; ///< False until the first point is added
};

/**
 * @brief Whether smoothing may pass through the given node without stopping.
 * @param node Per-node smoothing properties.
 * @return True if the node imposes no stop (no delay, script or behavior).
 */
bool IsWaypointSmoothingSafe(WaypointSmoothingNode const& node);

/**
 * @brief Whether the spline has reached a tracked waypoint endpoint.
 * @param currentPathIdx Current spline point index (movespline->currentPathIdx()).
 * @param endpointPathIndex Path-point index recorded for the waypoint.
 * @return True once the spline index has reached or passed the endpoint.
 */
bool HasReachedWaypointEndpoint(int32 currentPathIdx, size_t endpointPathIndex);

/**
 * @brief Classifies an in-progress segment by whether its spline has completed.
 * @param splineFinalized Whether the spline has completed.
 * @return The resulting WaypointSegmentUpdateState.
 */
WaypointSegmentUpdateState GetWaypointSegmentUpdateState(bool splineFinalized);

/**
 * @brief Expands the bounding box to include the given point.
 * @param bounds Bounding box to grow.
 * @param x Point X-coordinate.
 * @param y Point Y-coordinate.
 * @param z Point Z-coordinate.
 */
void AddWaypointSmoothingPoint(WaypointSmoothingBounds& bounds, float x, float y, float z);

/**
 * @brief Whether the bounding box stays within the packable offset budget.
 * @param bounds Bounding box to test.
 * @return True while within budget (an empty box is within budget).
 */
bool IsWaypointSmoothingWithinBudget(WaypointSmoothingBounds const& bounds);

/**
 * @brief Whether every linear-path segment survives Cata's packed wire encoding.
 * @param points Source spline points. The endpoints are uncompressed; every
 *        intermediate point is reconstructed from a 0.25-yard packed offset.
 * @param launchPosition Exact first point that MoveSplineInit will put on the wire.
 * @return False if two adjacent client-side points reconstruct identically.
 */
bool IsWaypointSmoothingWireSafe(std::vector<Geometry::Vector3> const& points,
                                 Geometry::Vector3 const& launchPosition);

/**
 * @brief Removes intermediate points that collapse after packed-path decoding.
 * @param points Routed spline points to sanitize in place.
 * @param launchPosition Exact first point that MoveSplineInit will put on the wire.
 *
 * The destination and every distinct decoded corner are retained, so a routed
 * path does not degrade into an obstacle-crossing straight-line shortcut.
 */
bool SanitizeWaypointSmoothingWirePath(std::vector<Geometry::Vector3>& points,
                                       Geometry::Vector3 const& launchPosition);

#endif
