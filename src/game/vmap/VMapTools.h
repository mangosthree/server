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

#ifndef MANGOS_H_VMAPTOOLS
#define MANGOS_H_VMAPTOOLS

#include <G3D/CollisionDetection.h>
#include <G3D/AABox.h>

#include "NodeValueAccess.h"

/**
The Class is mainly taken from G3D/AABSPTree.h but modified to be able to use our internal data structure.
This is an iterator that helps us analysing the BSP-Trees.
The collision detection is modified to return true, if we are inside an object.
*/

namespace VMAP
{
    template<class TValue>
    /**
     * @brief
     *
     */
    class IntersectionCallBack
    {
        public:
            TValue*      closestEntity; /**< TODO */
            G3D::Vector3 hitLocation; /**< TODO */
            G3D::Vector3 hitNormal; /**< TODO */

            /**
             * @brief
             *
             * @param ray
             * @param entity
             * @param pStopAtFirstHit
             * @param distance
             */
            void operator()(const G3D::Ray& ray, const TValue* entity, bool pStopAtFirstHit, float& distance)
            {
                entity->intersect(ray, distance, pStopAtFirstHit, hitLocation, hitNormal);
            }
    };

    //==============================================================
    //==============================================================
    //==============================================================

    /**
     * @brief
     *
     */
    class MyCollisionDetection
    {
        private:
        public:

            /**
             * @brief
             *
             * @param origin
             * @param dir
             * @param box
             * @param location
             * @param Inside
             * @return bool
             */
            static bool collisionLocationForMovingPointFixedAABox(
                const G3D::Vector3&     origin,
                const G3D::Vector3&     dir,
                const G3D::AABox&       box,
                G3D::Vector3&           location,
                bool&                   Inside)
            {
                // Integer representation of a floating-point value.
#define IR(x)   (reinterpret_cast<G3D::uint32 const&>(x))

                Inside = true;
                const G3D::Vector3& MinB = box.low();
                const G3D::Vector3& MaxB = box.high();
                G3D::Vector3 MaxT(-1.0f, -1.0f, -1.0f);

                // Find candidate planes.
                for (int i = 0; i < 3; ++i)
                {
                    if (origin[i] < MinB[i])
                    {
                        location[i] = MinB[i];
                        Inside      = false;

                        // Calculate T distances to candidate planes
                        if (IR(dir[i]))
                        {
                            MaxT[i] = (MinB[i] - origin[i]) / dir[i];
                        }
                    }
                    else if (origin[i] > MaxB[i])
                    {
                        location[i] = MaxB[i];
                        Inside      = false;

                        // Calculate T distances to candidate planes
                        if (IR(dir[i]))
                        {
                            MaxT[i] = (MaxB[i] - origin[i]) / dir[i];
                        }
                    }
                }

                if (Inside)
                {
                    // definite hit
                    location = origin;
                    return true;
                }

                // Get largest of the maxT's for final choice of intersection
                int WhichPlane = 0;
                if (MaxT[1] > MaxT[WhichPlane])
                {
                    WhichPlane = 1;
                }

                if (MaxT[2] > MaxT[WhichPlane])
                {
                    WhichPlane = 2;
                }

                // Check final candidate actually inside box
                if (IR(MaxT[WhichPlane]) & 0x80000000)
                {
                    // Miss the box
                    return false;
                }

                for (int i = 0; i < 3; ++i)
                {
                    if (i != WhichPlane)
                    {
                        location[i] = origin[i] + MaxT[WhichPlane] * dir[i];
                        if ((location[i] < MinB[i]) ||
                            (location[i] > MaxB[i]))
                        {
                            // On this plane we're outside the box extents, so
                            // we miss the box
                            return false;
                        }
                    }
                }
                /*
                // Choose the normal to be the plane normal facing into the ray
                normal = G3D::Vector3::zero();
                normal[WhichPlane] = (dir[WhichPlane] > 0) ? -1.0 : 1.0;
                */
                return true;

#undef IR
            }
    };
}
#endif
