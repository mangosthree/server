#pragma once

// One 3.3.5a .adt tile, parsed into the grids the height engine wants. The V9/V8
// layout and the tile-local indexing match the reference map-extractor exactly, so
// TerrainTile::TerrainHeight indexes them correctly: the FIRST index is derived from
// the MCNK IndexY field and is what a world-X query resolves against.
//
// Liquid is reported as raw LiquidType.dbc row ids. Classifying a row into
// water/ocean/magma/slime needs the DBC and is deliberately not done here.

#include "ChunkReaders.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace world::terrain
{
    constexpr int ADT_CHUNKS = 16;
    constexpr int ADT_CELLS_PER_CHUNK = 8;
    constexpr int ADT_GRID = ADT_CHUNKS * ADT_CELLS_PER_CHUNK;  // 128
    constexpr int ADT_V9 = ADT_GRID + 1;                        // 129

    struct AdtData
    {
        bool hasTerrain = false;
        std::vector<float> v9;
        std::vector<float> v8;
        std::array<uint16_t, ADT_CHUNKS * ADT_CHUNKS> holes{};
        std::array<uint16_t, ADT_CHUNKS * ADT_CHUNKS> areaIds{};

        bool hasLiquid = false;
        bool hasMh2o = false;
        std::vector<float> liquidHeight;
        std::vector<uint8_t> liquidShow;
        std::vector<uint16_t> liquidEntry;
        std::vector<uint8_t> liquidDark;     ///< MCLQ per-cell dark-water bit
        std::vector<uint8_t> liquidDeepAttr; ///< MH2O chunk deep attribute, cell-broadcast

        std::vector<std::string> wmoNames;
        std::vector<std::string> m2Names;
        std::vector<Placement> wmoPlacements;
        std::vector<Placement> m2Placements;
    };

    /**
     * @brief Which half of a tile a file is expected to supply.
     *
     * Up to 3.3.5a a tile is one file and the answer is Both. Cataclysm splits it: the
     * root keeps MH2O and the terrain MCNKs, while _obj0.adt carries the name lists and
     * the MDDF/MODF placements -- AND its own 256 MCNKs, which hold per-chunk doodad
     * references and report area id 0 with no height data. Reading those as terrain
     * wipes what the root supplied, so each file is told what it is for.
     */
    enum class AdtParts
    {
        Both,       ///< one monolithic tile (up to 3.3.5a)
        Terrain,    ///< the root file of a split tile: MCNK heights, holes, areas, MH2O
        Objects,    ///< _obj0.adt: MMDX/MMID/MWMO/MWID and the MDDF/MODF placements
    };

    // Returns false only on a structurally broken file; a valid-but-empty tile yields
    // hasTerrain = false with no placements. Accumulates into `out`, so a split tile is
    // two calls on one AdtData.
    bool ParseAdt(const uint8_t* data, size_t size, AdtData& out,
                  AdtParts parts = AdtParts::Both);

    inline bool ParseAdt(const std::vector<uint8_t>& bytes, AdtData& out,
                         AdtParts parts = AdtParts::Both)
    {
        return ParseAdt(bytes.data(), bytes.size(), out, parts);
    }
}
