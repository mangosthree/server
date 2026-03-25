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
#include <set>
#include <algorithm>

// ============================================================================
// Standalone reimplementation of Grid system patterns from
// src/shared/GameSystem/Grid.h, NGrid.h, and Map coordinate system
// ============================================================================

namespace GridTest
{

// Map coordinate system constants
const float MAP_SIZE = 533.33333f * 64.0f; // 34133.33
const float MAP_HALFSIZE = MAP_SIZE / 2.0f;
const uint32_t MAX_NUMBER_OF_GRIDS = 64;
const float SIZE_OF_GRIDS = MAP_SIZE / MAX_NUMBER_OF_GRIDS;
const uint32_t MAX_NUMBER_OF_CELLS = 8; // cells per grid
const float SIZE_OF_CELLS = SIZE_OF_GRIDS / MAX_NUMBER_OF_CELLS;

// Convert world position to grid coordinates
uint32_t ComputeGridCoord(float pos)
{
    float adjusted = MAP_HALFSIZE - pos;
    if (adjusted < 0.0f) adjusted = 0.0f;
    uint32_t gridCoord = uint32_t(adjusted / SIZE_OF_GRIDS);
    if (gridCoord >= MAX_NUMBER_OF_GRIDS)
        gridCoord = MAX_NUMBER_OF_GRIDS - 1;
    return gridCoord;
}

// Convert world position to cell coordinates (within a grid)
uint32_t ComputeCellCoord(float pos)
{
    float adjusted = MAP_HALFSIZE - pos;
    if (adjusted < 0.0f) adjusted = 0.0f;
    float gridOffset = fmodf(adjusted, SIZE_OF_GRIDS);
    uint32_t cellCoord = uint32_t(gridOffset / SIZE_OF_CELLS);
    if (cellCoord >= MAX_NUMBER_OF_CELLS)
        cellCoord = MAX_NUMBER_OF_CELLS - 1;
    return cellCoord;
}

struct GridCoord
{
    uint32_t x, y;
    GridCoord(uint32_t x = 0, uint32_t y = 0) : x(x), y(y) {}
    bool operator==(const GridCoord& o) const { return x == o.x && y == o.y; }
};

struct CellCoord
{
    uint32_t x, y;
    CellCoord(uint32_t x = 0, uint32_t y = 0) : x(x), y(y) {}
    bool operator==(const CellCoord& o) const { return x == o.x && y == o.y; }
};

// Visibility distance
const float DEFAULT_VISIBILITY_DISTANCE = 90.0f;
const float MAX_VISIBILITY_DISTANCE = 250.0f;

// Grid for tracking objects
template <typename T>
class SimpleGrid
{
public:
    void AddObject(T obj) { m_objects.push_back(obj); }
    void RemoveObject(T obj)
    {
        m_objects.erase(std::remove(m_objects.begin(), m_objects.end(), obj), m_objects.end());
    }
    bool HasObject(T obj) const
    {
        return std::find(m_objects.begin(), m_objects.end(), obj) != m_objects.end();
    }
    size_t GetObjectCount() const { return m_objects.size(); }
    void Clear() { m_objects.clear(); }

    std::vector<T> GetObjectsInRange(float x, float y, float range,
        std::function<float(T)> getX, std::function<float(T)> getY) const
    {
        std::vector<T> result;
        float rangeSq = range * range;
        for (const auto& obj : m_objects)
        {
            float dx = getX(obj) - x;
            float dy = getY(obj) - y;
            if (dx * dx + dy * dy <= rangeSq)
                result.push_back(obj);
        }
        return result;
    }

private:
    std::vector<T> m_objects;
};

} // namespace GridTest

using namespace GridTest;

// ============================================================================
// Grid Coordinate Conversion Tests
// ============================================================================

TEST(GridCoord, CenterOfMap)
{
    // Position (0,0) should map to center grid
    uint32_t grid = ComputeGridCoord(0.0f);
    EXPECT_EQ(uint32_t(32), grid);
}

TEST(GridCoord, PositiveEdge)
{
    uint32_t grid = ComputeGridCoord(MAP_HALFSIZE);
    EXPECT_EQ(uint32_t(0), grid);
}

TEST(GridCoord, NegativeEdge)
{
    uint32_t grid = ComputeGridCoord(-MAP_HALFSIZE);
    EXPECT_EQ(uint32_t(63), grid);
}

TEST(GridCoord, ValidRange)
{
    for (float pos = -MAP_HALFSIZE; pos <= MAP_HALFSIZE; pos += 500.0f)
    {
        uint32_t grid = ComputeGridCoord(pos);
        EXPECT_GE(grid, uint32_t(0));
        EXPECT_LT(grid, MAX_NUMBER_OF_GRIDS);
    }
}

TEST(GridCoord, BeyondBoundsClamps)
{
    uint32_t grid = ComputeGridCoord(MAP_HALFSIZE + 1000.0f);
    EXPECT_GE(grid, uint32_t(0));
    EXPECT_LT(grid, MAX_NUMBER_OF_GRIDS);
}

// ============================================================================
// Cell Coordinate Tests
// ============================================================================

TEST(CellCoord, ValidRange)
{
    for (float pos = -MAP_HALFSIZE; pos <= MAP_HALFSIZE; pos += 100.0f)
    {
        uint32_t cell = ComputeCellCoord(pos);
        EXPECT_GE(cell, uint32_t(0));
        EXPECT_LT(cell, MAX_NUMBER_OF_CELLS);
    }
}

TEST(CellCoord, CenterOfMap)
{
    uint32_t cell = ComputeCellCoord(0.0f);
    EXPECT_LT(cell, MAX_NUMBER_OF_CELLS);
}

// ============================================================================
// Grid Constants Tests
// ============================================================================

TEST(GridConstants, MapSize)
{
    EXPECT_NEAR(34133.33f, MAP_SIZE, 1.0f);
}

TEST(GridConstants, GridCount)
{
    EXPECT_EQ(uint32_t(64), MAX_NUMBER_OF_GRIDS);
}

TEST(GridConstants, CellsPerGrid)
{
    EXPECT_EQ(uint32_t(8), MAX_NUMBER_OF_CELLS);
}

TEST(GridConstants, GridSizeConsistency)
{
    EXPECT_NEAR(MAP_SIZE, float(MAX_NUMBER_OF_GRIDS) * SIZE_OF_GRIDS, 0.01f);
}

TEST(GridConstants, CellSizeConsistency)
{
    EXPECT_NEAR(SIZE_OF_GRIDS, float(MAX_NUMBER_OF_CELLS) * SIZE_OF_CELLS, 0.01f);
}

TEST(GridConstants, VisibilityDefaults)
{
    EXPECT_FLOAT_EQ(90.0f, DEFAULT_VISIBILITY_DISTANCE);
    EXPECT_FLOAT_EQ(250.0f, MAX_VISIBILITY_DISTANCE);
    EXPECT_LT(DEFAULT_VISIBILITY_DISTANCE, MAX_VISIBILITY_DISTANCE);
}

// ============================================================================
// SimpleGrid Tests
// ============================================================================

TEST(SimpleGrid, InitiallyEmpty)
{
    SimpleGrid<uint32_t> grid;
    EXPECT_EQ(size_t(0), grid.GetObjectCount());
}

TEST(SimpleGrid, AddObject)
{
    SimpleGrid<uint32_t> grid;
    grid.AddObject(42);
    EXPECT_EQ(size_t(1), grid.GetObjectCount());
    EXPECT_TRUE(grid.HasObject(42));
}

TEST(SimpleGrid, RemoveObject)
{
    SimpleGrid<uint32_t> grid;
    grid.AddObject(42);
    grid.RemoveObject(42);
    EXPECT_EQ(size_t(0), grid.GetObjectCount());
    EXPECT_FALSE(grid.HasObject(42));
}

TEST(SimpleGrid, RemoveNonexistent)
{
    SimpleGrid<uint32_t> grid;
    grid.AddObject(1);
    grid.RemoveObject(2); // doesn't exist
    EXPECT_EQ(size_t(1), grid.GetObjectCount());
}

TEST(SimpleGrid, MultipleObjects)
{
    SimpleGrid<uint32_t> grid;
    grid.AddObject(1);
    grid.AddObject(2);
    grid.AddObject(3);
    EXPECT_EQ(size_t(3), grid.GetObjectCount());
    EXPECT_TRUE(grid.HasObject(1));
    EXPECT_TRUE(grid.HasObject(2));
    EXPECT_TRUE(grid.HasObject(3));
    EXPECT_FALSE(grid.HasObject(4));
}

TEST(SimpleGrid, Clear)
{
    SimpleGrid<uint32_t> grid;
    grid.AddObject(1);
    grid.AddObject(2);
    grid.Clear();
    EXPECT_EQ(size_t(0), grid.GetObjectCount());
}

TEST(SimpleGrid, RangeSearch)
{
    struct Entity { uint32_t id; float x, y; };
    SimpleGrid<Entity> grid;
    grid.AddObject({1, 0.0f, 0.0f});
    grid.AddObject({2, 5.0f, 0.0f});
    grid.AddObject({3, 50.0f, 50.0f});

    auto results = grid.GetObjectsInRange(0.0f, 0.0f, 10.0f,
        [](const Entity& e) { return e.x; },
        [](const Entity& e) { return e.y; });

    EXPECT_EQ(size_t(2), results.size());
}

TEST(SimpleGrid, RangeSearchNoResults)
{
    struct Entity { uint32_t id; float x, y; };
    SimpleGrid<Entity> grid;
    grid.AddObject({1, 100.0f, 100.0f});

    auto results = grid.GetObjectsInRange(0.0f, 0.0f, 10.0f,
        [](const Entity& e) { return e.x; },
        [](const Entity& e) { return e.y; });

    EXPECT_EQ(size_t(0), results.size());
}

// ============================================================================
// GridCoord/CellCoord Struct Tests
// ============================================================================

TEST(GridCoordStruct, DefaultConstructor)
{
    GridCoord gc;
    EXPECT_EQ(uint32_t(0), gc.x);
    EXPECT_EQ(uint32_t(0), gc.y);
}

TEST(GridCoordStruct, Equality)
{
    GridCoord a(10, 20);
    GridCoord b(10, 20);
    EXPECT_TRUE(a == b);
}

TEST(GridCoordStruct, Inequality)
{
    GridCoord a(10, 20);
    GridCoord b(10, 21);
    EXPECT_FALSE(a == b);
}

TEST(CellCoordStruct, DefaultConstructor)
{
    CellCoord cc;
    EXPECT_EQ(uint32_t(0), cc.x);
    EXPECT_EQ(uint32_t(0), cc.y);
}

TEST(CellCoordStruct, Equality)
{
    CellCoord a(5, 3);
    CellCoord b(5, 3);
    EXPECT_TRUE(a == b);
}
