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
 */

#include "TestFramework.h"

#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>

// ============================================================================
// Standalone reimplementation of movement patterns from
// src/game/MotionGenerators/ and src/game/movement/
// ============================================================================

namespace MovementTest
{

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define M_PI_F float(M_PI)

// Base movement speeds (yards/second)
enum UnitMoveType
{
    MOVE_WALK       = 0,
    MOVE_RUN        = 1,
    MOVE_RUN_BACK   = 2,
    MOVE_SWIM       = 3,
    MOVE_SWIM_BACK  = 4,
    MOVE_TURN_RATE  = 5,
    MOVE_FLIGHT     = 6,
    MOVE_FLIGHT_BACK = 7,
    MOVE_PITCH_RATE  = 8,
    MAX_MOVE_TYPE    = 9
};

const float baseSpeeds[MAX_MOVE_TYPE] = {
    2.5f,       // MOVE_WALK
    7.0f,       // MOVE_RUN
    4.5f,       // MOVE_RUN_BACK
    4.722222f,  // MOVE_SWIM
    2.5f,       // MOVE_SWIM_BACK
    3.141594f,  // MOVE_TURN_RATE (pi rad/s)
    7.0f,       // MOVE_FLIGHT
    4.5f,       // MOVE_FLIGHT_BACK
    3.141594f,  // MOVE_PITCH_RATE
};

struct Position
{
    float x, y, z;
    Position(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

float GetDistance2d(const Position& a, const Position& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

float GetDistance3d(const Position& a, const Position& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

float GetAngleTo(const Position& from, const Position& to)
{
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float angle = atan2f(dy, dx);
    if (angle < 0.0f) angle += 2.0f * M_PI_F;
    return angle;
}

// Movement speed with modifiers
float CalculateSpeed(float baseSpeed, float speedMod, float stackedMod)
{
    float speed = baseSpeed * speedMod;
    speed *= (1.0f + stackedMod / 100.0f);
    if (speed < 0.0f) speed = 0.0f;
    return speed;
}

// Time to travel between two points
uint32_t CalculateTravelTime(float distance, float speed)
{
    if (speed <= 0.0f) return 0;
    return uint32_t(distance / speed * 1000.0f); // in ms
}

// Interpolate position along a path
Position InterpolatePosition(const Position& start, const Position& end, float t)
{
    if (t <= 0.0f) return start;
    if (t >= 1.0f) return end;
    return Position(
        start.x + (end.x - start.x) * t,
        start.y + (end.y - start.y) * t,
        start.z + (end.z - start.z) * t
    );
}

// Waypoint path structure
struct WaypointPath
{
    std::vector<Position> points;

    float GetTotalLength() const
    {
        float length = 0.0f;
        for (size_t i = 1; i < points.size(); ++i)
            length += GetDistance3d(points[i-1], points[i]);
        return length;
    }

    size_t GetPointCount() const { return points.size(); }
};

// Fall damage calculation
uint32_t CalculateFallDamage(float fallDistance, uint32_t maxHealth)
{
    if (fallDistance < 14.57f) return 0; // safe fall distance
    float pct = (fallDistance - 14.57f) * 0.01f;
    if (pct > 1.0f) pct = 1.0f;
    return uint32_t(float(maxHealth) * pct);
}

// Swimming depth check
bool IsSubmerged(float waterLevel, float posZ, float collisionHeight)
{
    return posZ + collisionHeight < waterLevel;
}

bool IsInWater(float waterLevel, float posZ)
{
    return posZ < waterLevel;
}

// Mounting speed bonus
float GetMountSpeedBonus(uint8_t mountType)
{
    switch (mountType)
    {
        case 0: return 0.0f;     // no mount
        case 1: return 60.0f;    // slow ground
        case 2: return 100.0f;   // fast ground
        case 3: return 150.0f;   // slow flying
        case 4: return 280.0f;   // fast flying
        case 5: return 310.0f;   // 310% flying
        default: return 0.0f;
    }
}

} // namespace MovementTest

using namespace MovementTest;

// ============================================================================
// Movement Speed Tests
// ============================================================================

TEST(MovementSpeed, BaseRunSpeed)
{
    EXPECT_FLOAT_EQ(7.0f, baseSpeeds[MOVE_RUN]);
}

TEST(MovementSpeed, BaseWalkSpeed)
{
    EXPECT_FLOAT_EQ(2.5f, baseSpeeds[MOVE_WALK]);
}

TEST(MovementSpeed, WalkSlowerThanRun)
{
    EXPECT_LT(baseSpeeds[MOVE_WALK], baseSpeeds[MOVE_RUN]);
}

TEST(MovementSpeed, BackwardSlowerThanForward)
{
    EXPECT_LT(baseSpeeds[MOVE_RUN_BACK], baseSpeeds[MOVE_RUN]);
    EXPECT_LT(baseSpeeds[MOVE_SWIM_BACK], baseSpeeds[MOVE_SWIM]);
}

TEST(MovementSpeed, FlightEqualToRun)
{
    EXPECT_FLOAT_EQ(baseSpeeds[MOVE_RUN], baseSpeeds[MOVE_FLIGHT]);
}

TEST(MovementSpeed, CalculateWithModifier)
{
    float speed = CalculateSpeed(7.0f, 1.0f, 0.0f);
    EXPECT_FLOAT_EQ(7.0f, speed);
}

TEST(MovementSpeed, CalculateWithSpeedBuff)
{
    // 7.0 base, 1.0 mod, +30% stacked
    float speed = CalculateSpeed(7.0f, 1.0f, 30.0f);
    EXPECT_NEAR(9.1f, speed, 0.01f);
}

TEST(MovementSpeed, CalculateSlowed)
{
    // 7.0 base, 0.5 modifier (50% snare)
    float speed = CalculateSpeed(7.0f, 0.5f, 0.0f);
    EXPECT_FLOAT_EQ(3.5f, speed);
}

TEST(MovementSpeed, SpeedCantGoNegative)
{
    float speed = CalculateSpeed(7.0f, -1.0f, 0.0f);
    EXPECT_FLOAT_EQ(0.0f, speed);
}

// ============================================================================
// Distance Tests
// ============================================================================

TEST(MovementDistance, Distance2dSamePoint)
{
    Position p(1, 2, 3);
    EXPECT_FLOAT_EQ(0.0f, GetDistance2d(p, p));
}

TEST(MovementDistance, Distance2dSimple)
{
    Position a(0, 0, 0);
    Position b(3, 4, 0);
    EXPECT_FLOAT_EQ(5.0f, GetDistance2d(a, b));
}

TEST(MovementDistance, Distance3d)
{
    Position a(0, 0, 0);
    Position b(1, 2, 2);
    EXPECT_FLOAT_EQ(3.0f, GetDistance3d(a, b));
}

TEST(MovementDistance, Distance3dIgnoresZ)
{
    Position a(0, 0, 0);
    Position b(3, 4, 100);
    float d2d = GetDistance2d(a, b);
    float d3d = GetDistance3d(a, b);
    EXPECT_GT(d3d, d2d);
}

// ============================================================================
// Travel Time Tests
// ============================================================================

TEST(TravelTime, BasicTravel)
{
    // 70 yards at 7 yards/sec = 10 seconds = 10000ms
    EXPECT_EQ(uint32_t(10000), CalculateTravelTime(70.0f, 7.0f));
}

TEST(TravelTime, ZeroSpeed)
{
    EXPECT_EQ(uint32_t(0), CalculateTravelTime(100.0f, 0.0f));
}

TEST(TravelTime, ZeroDistance)
{
    EXPECT_EQ(uint32_t(0), CalculateTravelTime(0.0f, 7.0f));
}

TEST(TravelTime, FastMount)
{
    // 100 yards at 7 * 2.8 = 19.6 yards/sec = ~5102ms
    float speed = 7.0f * (1.0f + 180.0f / 100.0f); // 280% mount
    uint32_t time = CalculateTravelTime(100.0f, speed);
    EXPECT_GT(time, uint32_t(5000));
    EXPECT_LT(time, uint32_t(5200));
}

// ============================================================================
// Position Interpolation Tests
// ============================================================================

TEST(Interpolation, AtStart)
{
    Position start(0, 0, 0);
    Position end(10, 10, 10);
    Position p = InterpolatePosition(start, end, 0.0f);
    EXPECT_FLOAT_EQ(0.0f, p.x);
    EXPECT_FLOAT_EQ(0.0f, p.y);
    EXPECT_FLOAT_EQ(0.0f, p.z);
}

TEST(Interpolation, AtEnd)
{
    Position start(0, 0, 0);
    Position end(10, 10, 10);
    Position p = InterpolatePosition(start, end, 1.0f);
    EXPECT_FLOAT_EQ(10.0f, p.x);
    EXPECT_FLOAT_EQ(10.0f, p.y);
    EXPECT_FLOAT_EQ(10.0f, p.z);
}

TEST(Interpolation, Midpoint)
{
    Position start(0, 0, 0);
    Position end(10, 20, 30);
    Position p = InterpolatePosition(start, end, 0.5f);
    EXPECT_FLOAT_EQ(5.0f, p.x);
    EXPECT_FLOAT_EQ(10.0f, p.y);
    EXPECT_FLOAT_EQ(15.0f, p.z);
}

TEST(Interpolation, QuarterPoint)
{
    Position start(0, 0, 0);
    Position end(100, 0, 0);
    Position p = InterpolatePosition(start, end, 0.25f);
    EXPECT_FLOAT_EQ(25.0f, p.x);
}

TEST(Interpolation, BeyondEndClamped)
{
    Position start(0, 0, 0);
    Position end(10, 10, 10);
    Position p = InterpolatePosition(start, end, 2.0f);
    EXPECT_FLOAT_EQ(10.0f, p.x);
}

TEST(Interpolation, NegativeClamped)
{
    Position start(5, 5, 5);
    Position end(10, 10, 10);
    Position p = InterpolatePosition(start, end, -1.0f);
    EXPECT_FLOAT_EQ(5.0f, p.x);
}

// ============================================================================
// Waypoint Path Tests
// ============================================================================

TEST(WaypointPath, EmptyPath)
{
    WaypointPath path;
    EXPECT_FLOAT_EQ(0.0f, path.GetTotalLength());
    EXPECT_EQ(size_t(0), path.GetPointCount());
}

TEST(WaypointPath, SinglePoint)
{
    WaypointPath path;
    path.points.push_back(Position(0, 0, 0));
    EXPECT_FLOAT_EQ(0.0f, path.GetTotalLength());
    EXPECT_EQ(size_t(1), path.GetPointCount());
}

TEST(WaypointPath, TwoPoints)
{
    WaypointPath path;
    path.points.push_back(Position(0, 0, 0));
    path.points.push_back(Position(3, 4, 0));
    EXPECT_FLOAT_EQ(5.0f, path.GetTotalLength());
}

TEST(WaypointPath, MultipleSegments)
{
    WaypointPath path;
    path.points.push_back(Position(0, 0, 0));
    path.points.push_back(Position(3, 4, 0)); // 5
    path.points.push_back(Position(3, 14, 0)); // 10
    EXPECT_FLOAT_EQ(15.0f, path.GetTotalLength());
}

// ============================================================================
// Fall Damage Tests
// ============================================================================

TEST(FallDamage, SafeFall)
{
    EXPECT_EQ(uint32_t(0), CalculateFallDamage(10.0f, 10000));
}

TEST(FallDamage, JustAboveSafe)
{
    uint32_t dmg = CalculateFallDamage(15.0f, 10000);
    EXPECT_GT(dmg, uint32_t(0));
}

TEST(FallDamage, HighFall)
{
    uint32_t dmg = CalculateFallDamage(100.0f, 10000);
    EXPECT_GT(dmg, uint32_t(0));
    EXPECT_LE(dmg, uint32_t(10000));
}

TEST(FallDamage, LethalFall)
{
    uint32_t dmg = CalculateFallDamage(200.0f, 10000);
    EXPECT_EQ(uint32_t(10000), dmg); // capped at 100% health
}

TEST(FallDamage, ZeroFall)
{
    EXPECT_EQ(uint32_t(0), CalculateFallDamage(0.0f, 10000));
}

// ============================================================================
// Swimming Tests
// ============================================================================

TEST(Swimming, NotInWater)
{
    EXPECT_FALSE(IsInWater(100.0f, 110.0f));
}

TEST(Swimming, InWater)
{
    EXPECT_TRUE(IsInWater(100.0f, 90.0f));
}

TEST(Swimming, AtWaterLine)
{
    EXPECT_FALSE(IsInWater(100.0f, 100.0f));
}

TEST(Swimming, NotSubmerged)
{
    EXPECT_FALSE(IsSubmerged(100.0f, 95.0f, 6.0f)); // 95 + 6 = 101 > 100
}

TEST(Swimming, Submerged)
{
    EXPECT_TRUE(IsSubmerged(100.0f, 90.0f, 6.0f)); // 90 + 6 = 96 < 100
}

// ============================================================================
// Mount Speed Tests
// ============================================================================

TEST(MountSpeed, NoMount)
{
    EXPECT_FLOAT_EQ(0.0f, GetMountSpeedBonus(0));
}

TEST(MountSpeed, SlowGround)
{
    EXPECT_FLOAT_EQ(60.0f, GetMountSpeedBonus(1));
}

TEST(MountSpeed, FastGround)
{
    EXPECT_FLOAT_EQ(100.0f, GetMountSpeedBonus(2));
}

TEST(MountSpeed, FastFlying)
{
    EXPECT_FLOAT_EQ(280.0f, GetMountSpeedBonus(4));
}

TEST(MountSpeed, EpicFlying)
{
    EXPECT_FLOAT_EQ(310.0f, GetMountSpeedBonus(5));
}

TEST(MountSpeed, InvalidMount)
{
    EXPECT_FLOAT_EQ(0.0f, GetMountSpeedBonus(99));
}

// ============================================================================
// Angle Tests
// ============================================================================

TEST(Angle, East)
{
    EXPECT_NEAR(0.0f, GetAngleTo(Position(0, 0), Position(1, 0)), 0.001f);
}

TEST(Angle, North)
{
    EXPECT_NEAR(M_PI_F / 2.0f, GetAngleTo(Position(0, 0), Position(0, 1)), 0.001f);
}

TEST(Angle, West)
{
    EXPECT_NEAR(M_PI_F, GetAngleTo(Position(0, 0), Position(-1, 0)), 0.001f);
}

TEST(Angle, South)
{
    EXPECT_NEAR(3.0f * M_PI_F / 2.0f, GetAngleTo(Position(0, 0), Position(0, -1)), 0.001f);
}
