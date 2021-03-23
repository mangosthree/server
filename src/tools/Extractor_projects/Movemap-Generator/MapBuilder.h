/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2021 MaNGOS <https://getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef MANGOS_H_MAP_BUILDER
#define MANGOS_H_MAP_BUILDER

#include <vector>
#include <set>
#include <map>

#include <Recast.h>
#include <DetourNavMesh.h>

#include "TerrainBuilder.h"
#include "IntermediateValues.h"

#include "IVMapManager.h"
#include "WorldModel.h"

#include "TileThreadPool.h"

using namespace std;
using namespace VMAP;
// G3D namespace typedefs conflicts with ACE typedefs

namespace MMAP
{
    /**
     * @brief
     *
     */
    typedef map<uint32, set<uint32>*> TileList;
    /**
     * @brief
     *
     */
    struct Tile
    {
        /**
         * @brief
         *
         */
        Tile() : chf(NULL), solid(NULL), cset(NULL), pmesh(NULL), dmesh(NULL) {}
        /**
         * @brief
         *
         */
        ~Tile()
        {
            rcFreeCompactHeightfield(chf);
            rcFreeContourSet(cset);
            rcFreeHeightField(solid);
            rcFreePolyMesh(pmesh);
            rcFreePolyMeshDetail(dmesh);
        }
        rcCompactHeightfield* chf; /**< TODO */
        rcHeightfield* solid; /**< TODO */
        rcContourSet* cset; /**< TODO */
        rcPolyMesh* pmesh; /**< TODO */
        rcPolyMeshDetail* dmesh; /**< TODO */
    };

    /**
     * @brief
     *
     */
    class MapBuilder
    {
        public:
            /**
             * @brief
             *
             * @param maxWalkableAngle
             * @param skipLiquid
             * @param skipContinents
             * @param skipJunkMaps
             * @param skipBattlegrounds
             * @param debugOutput
             * @param bigBaseUnit
             * @param offMeshFilePath
             */
            MapBuilder(const char* magic,
                       float maxWalkableAngle   = 60.f,
                       bool skipLiquid          = false,
                       bool skipContinents      = false,
                       bool skipJunkMaps        = true,
                       bool skipBattlegrounds   = false,
                       bool debugOutput         = false,
                       bool bigBaseUnit         = false,
                       const char* offMeshFilePath = NULL);

            /**
             * @brief
             *
             */
            ~MapBuilder();

            /**
             * @brief builds all mmap tiles for the specified map id (ignores skip settings)
             *
             * @param mapID
             */
            void buildMap(int mapID, bool standAlone = true);

            /**
             * @brief builds an mmap tile for the specified map and its mesh
             *
             * @param mapID
             * @param tileX
             * @param tileY
             */
            void buildSingleTile(int mapID, int tileX, int tileY);

            /**
             * @brief builds list of maps, then builds all of mmap tiles (based on the skip settings)
             *
             */
            void buildAllMaps();

            int activate(int num_threads);

            bool activated() const { return m_poolActivated; }

            /**
             * @brief
             *
             * @param mapID
             * @param tileX
             * @param tileY
             * @param navMesh
             */
            void buildTile(int mapID, int tileX, int tileY, dtNavMesh* navMesh);

        private:
            /**
             * @brief detect maps and tiles
             *
             */
            void discoverTiles();
            /**
             * @brief
             *
             * @param mapID
             * @return set<uint32>
             */
            set<uint32>* getTileList(int mapID);

            /**
             * @brief
             *
             * @param mapID
             * @param navMesh
             */
            void buildNavMesh(int mapID, dtNavMesh*& navMesh, dtNavMeshParams*& navMeshParams);


            /**
             * @brief move map building
             *
             * @param mapID
             * @param tileX
             * @param tileY
             * @param meshData
             * @param bmin[]
             * @param bmax[]
             * @param navMesh
             */
            void buildMoveMapTile(int mapID,
                                  int tileX,
                                  int tileY,
                                  MeshData& meshData,
                                  float bmin[3],
                                  float bmax[3],
                                  dtNavMesh* navMesh);

            /**
             * @brief
             *
             * @param tileX
             * @param tileY
             * @param verts
             * @param vertCount
             * @param bmin
             * @param bmax
             */
            void getTileBounds(int tileX, int tileY,
                               float* verts, int vertCount,
                               float* bmin, float* bmax);
            /**
             * @brief
             *
             * @param mapID
             * @param minX
             * @param minY
             * @param maxX
             * @param maxY
             */
            void getGridBounds(int mapID, uint32& minX, uint32& minY, uint32& maxX, uint32& maxY);

            /**
             * @brief
             *
             * @param mapID
             * @param tileX
             * @param tileY
             * @return bool
             */
            bool shouldSkipTile(int mapID, int tileX, int tileY);

            TerrainBuilder* m_terrainBuilder; /**< TODO */
            TileList m_tiles; /**< TODO */

            bool m_debugOutput; /**< TODO */

            const char* m_offMeshFilePath; /**< TODO */
            bool m_skipContinents; /**< TODO */
            bool m_skipJunkMaps; /**< TODO */
            bool m_skipBattlegrounds; /**< TODO */

            float m_maxWalkableAngle; /**< TODO */
            bool m_bigBaseUnit; /**< TODO */
            char const* m_magic;

            int             m_numThreads;
            TileThreadPool* m_threadPool;
            bool            m_poolActivated;

            rcContext* m_rcContext; /**< build performance - not really used for now */
    };
}

#endif
