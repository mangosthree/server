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

#ifndef MANGOS_H_MODELINSTANCE
#define MANGOS_H_MODELINSTANCE

#include <G3D/Matrix3.h>
#include <G3D/Vector3.h>
#include <G3D/AABox.h>
#include <G3D/Ray.h>

#include "Platform/Define.h"

namespace VMAP
{
    class WorldModel;
    struct AreaInfo;
    struct LocationInfo;

    /**
     * @brief Enumeration for model flags.
     */
    enum ModelFlags
    {
        MOD_M2 = 1,
        MOD_WORLDSPAWN = 1 << 1,
        MOD_HAS_BOUND = 1 << 2
    };

    /**
     * @brief Class representing a model spawn.
     */
    class ModelSpawn
    {
    public:
        // mapID, tileX, tileY, Flags, ID, Pos, Rot, Scale, Bound_lo, Bound_hi, name
        uint32 flags; /**< Model flags. */
        uint16 adtId; /**< ADT ID. */
        uint32 ID; /**< Model ID. */
        G3D::Vector3 iPos; /**< Position of the model. */
        G3D::Vector3 iRot; /**< Rotation of the model. */
        float iScale; /**< Scale of the model. */
        G3D::AABox iBound; /**< Bounding box of the model. */
        std::string name; /**< Name of the model. */

        /**
         * @brief Equality operator for ModelSpawn.
         *
         * @param other The other ModelSpawn to compare with.
         * @return bool True if the ModelSpawns are equal, false otherwise.
         */
        bool operator==(const ModelSpawn& other) const { return ID == other.ID; }

        /**
         * @brief Gets the bounding box of the model.
         *
         * @return const G3D::AABox& The bounding box of the model.
         */
        const G3D::AABox& getBounds() const { return iBound; }

        /**
         * @brief Reads a ModelSpawn from a file.
         *
         * @param rf The file to read from.
         * @param spawn The ModelSpawn to read into.
         * @return bool True if the read was successful, false otherwise.
         */
        static bool ReadFromFile(FILE* rf, ModelSpawn& spawn);

        /**
         * @brief Writes a ModelSpawn to a file.
         *
         * @param rw The file to write to.
         * @param spawn The ModelSpawn to write.
         * @return bool True if the write was successful, false otherwise.
         */
        static bool WriteToFile(FILE* rw, const ModelSpawn& spawn);
    };

    /**
     * @brief Class representing a model instance.
     */
    class ModelInstance : public ModelSpawn
    {
        public:
            /**
             * @brief
             *
             */
            ModelInstance(): iModel(0) {}
            /**
             * @brief
             *
             * @param spawn
             * @param model
             */
            ModelInstance(const ModelSpawn& spawn, WorldModel* model);
            /**
             * @brief
             *
             */
            void setUnloaded() { iModel = 0; }
            /**
             * @brief
             *
             * @param pRay
             * @param pMaxDist
             * @param pStopAtFirstHit
             * @return bool
             */
            bool IntersectRay(const G3D::Ray& pRay, float& pMaxDist, bool pStopAtFirstHit) const;
            /**
             * @brief
             *
             * @param p
             * @param info
             */
            void intersectPoint(const G3D::Vector3& p, AreaInfo& info) const;
            /**
             * @brief
             *
             * @param p
             * @param info
             * @return bool
             */
            bool GetLocationInfo(const G3D::Vector3& p, LocationInfo& info) const;
            /**
             * @brief
             *
             * @param p
             * @param info
             * @param liqHeight
             * @return bool
             */
            bool GetLiquidLevel(const G3D::Vector3& p, LocationInfo& info, float& liqHeight) const;
        protected:
            G3D::Matrix3 iInvRot; /**< TODO */
            float iInvScale; /**< TODO */
            WorldModel* iModel; /**< TODO */

#ifdef MMAP_GENERATOR
    public:
        /**
         * @brief Gets the world model.
         *
         * @return WorldModel* Pointer to the world model.
         */
        WorldModel* const getWorldModel();
#endif
    };
} // namespace VMAP

#endif // MANGOS_H_MODELINSTANCE
