#include <algorithm>
#include "WaypointSmoothing.h"

namespace
{
    constexpr float PACKED_WAYPOINT_STEP = 0.25f;

    /**
     * @brief Applies appendPackXYZ's truncation and the client's signed decode.
     * @param offset Component offset from the linear-path midpoint.
     * @param bits Packed field width (11 for X/Y, 10 for Z).
     * @return The signed packed integer reconstructed by the client.
     */
    int32 RoundTripPackedOffset(float offset, uint32 bits)
    {
        const uint32 range = uint32(1) << bits;
        const uint32 mask = range - 1;
        const uint32 sign = range >> 1;

        // ByteBuffer::appendPackXYZ truncates toward zero before masking the
        // signed value into its fixed-width field.
        const int32 quantized = static_cast<int32>(offset / PACKED_WAYPOINT_STEP);
        const uint32 encoded = static_cast<uint32>(quantized) & mask;
        return encoded & sign ? static_cast<int32>(encoded) - static_cast<int32>(range)
                              : static_cast<int32>(encoded);
    }

    Geometry::Vector3 ReconstructPackedPoint(Geometry::Vector3 const& midpoint,
                                             Geometry::Vector3 const& point)
    {
        const Geometry::Vector3 offset = midpoint - point;
        return Geometry::Vector3(
            midpoint.x - float(RoundTripPackedOffset(offset.x, 11)) *
                         PACKED_WAYPOINT_STEP,
            midpoint.y - float(RoundTripPackedOffset(offset.y, 11)) *
                         PACKED_WAYPOINT_STEP,
            midpoint.z - float(RoundTripPackedOffset(offset.z, 10)) *
                         PACKED_WAYPOINT_STEP);
    }
}

/**
 * @brief Whether smoothing may pass through the given node without stopping.
 * @param node Per-node smoothing properties.
 * @return True if the node imposes no stop (no delay, script or behavior).
 */
bool IsWaypointSmoothingSafe(WaypointSmoothingNode const& node)
{
    // Movement splines face along their path; waypoint orientation is only
    // applied when a node stops.
    return !node.hasDelay &&
           !node.hasScript &&
           !node.hasBehavior;
}

/**
 * @brief Whether the spline has reached a tracked waypoint endpoint.
 * @param currentPathIdx Current spline point index (movespline->currentPathIdx()).
 * @param endpointPathIndex Path-point index recorded for the waypoint.
 * @return True once the spline index has reached or passed the endpoint.
 */
bool HasReachedWaypointEndpoint(int32 currentPathIdx, size_t endpointPathIndex)
{
    if (currentPathIdx <= 0)
    {
        return false;
    }

    return static_cast<size_t>(currentPathIdx) >= endpointPathIndex;
}

/**
 * @brief Classifies an in-progress segment by whether its spline has completed.
 * @param splineFinalized Whether the spline has completed.
 * @return The resulting WaypointSegmentUpdateState.
 */
WaypointSegmentUpdateState GetWaypointSegmentUpdateState(bool splineFinalized)
{
    // A stop from outside is not read here any more: the driver reports it as a cut
    // leg, and the waypoint generator pauses or resumes on its own terms.
    if (splineFinalized)
    {
        return WaypointSegmentUpdateState::Finalized;
    }

    return WaypointSegmentUpdateState::Moving;
}

/**
 * @brief Expands the bounding box to include the given point.
 * @param bounds Bounding box to grow.
 * @param x Point X-coordinate.
 * @param y Point Y-coordinate.
 * @param z Point Z-coordinate.
 */
void AddWaypointSmoothingPoint(WaypointSmoothingBounds& bounds, float x, float y, float z)
{
    if (!bounds.initialized)
    {
        bounds.minX = bounds.maxX = x;
        bounds.minY = bounds.maxY = y;
        bounds.minZ = bounds.maxZ = z;
        bounds.initialized = true;
        return;
    }

    bounds.minX = std::min(bounds.minX, x);
    bounds.maxX = std::max(bounds.maxX, x);
    bounds.minY = std::min(bounds.minY, y);
    bounds.maxY = std::max(bounds.maxY, y);
    bounds.minZ = std::min(bounds.minZ, z);
    bounds.maxZ = std::max(bounds.maxZ, z);
}

/**
 * @brief Whether the bounding box stays within the packable offset budget.
 * @param bounds Bounding box to test.
 * @return True while within budget (an empty box is within budget).
 */
bool IsWaypointSmoothingWithinBudget(WaypointSmoothingBounds const& bounds)
{
    if (!bounds.initialized)
    {
        return true;
    }

    return (bounds.maxX - bounds.minX) <= WAYPOINT_SMOOTHING_MAX_XY_SPAN &&
           (bounds.maxY - bounds.minY) <= WAYPOINT_SMOOTHING_MAX_XY_SPAN &&
           (bounds.maxZ - bounds.minZ) <= WAYPOINT_SMOOTHING_MAX_Z_SPAN;
}

/**
 * @brief Whether every linear-path segment survives Cata's packed wire encoding.
 * @param points Source spline points. The endpoints are uncompressed; every
 *        intermediate point is reconstructed from a 0.25-yard packed offset.
 * @param launchPosition Exact first point that MoveSplineInit will put on the wire.
 * @return False if two adjacent client-side points reconstruct identically.
 */
bool IsWaypointSmoothingWireSafe(std::vector<Geometry::Vector3> const& points,
                                 Geometry::Vector3 const& launchPosition)
{
    if (points.size() < 2)
    {
        return true;
    }

    // PacketBuilder::WriteLinearPath sends the destination directly and every
    // intermediate point as an offset from this midpoint. Compare the path the
    // client reconstructs, including both uncompressed endpoint boundaries.
    const Geometry::Vector3 midpoint = (launchPosition + points.back()) / 2.0f;
    Geometry::Vector3 previous = launchPosition;
    for (size_t i = 1; i + 1 < points.size(); ++i)
    {
        const Geometry::Vector3 current = ReconstructPackedPoint(midpoint, points[i]);
        if (current == previous)
        {
            return false;
        }
        previous = current;
    }

    return previous != points.back();
}
