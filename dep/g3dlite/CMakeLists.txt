# This code is part of MaNGOS. Contributor & Copyright details are in AUTHORS/THANKS.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

add_library(g3dlite STATIC
    G3D/AABox.cpp
    G3D/Any.cpp
    G3D/BinaryFormat.cpp
    G3D/BinaryInput.cpp
    G3D/BinaryOutput.cpp
    G3D/Box.cpp
    G3D/Capsule.cpp
    G3D/CollisionDetection.cpp
    G3D/CoordinateFrame.cpp
    G3D/Crypto.cpp
    G3D/Cylinder.cpp
    G3D/FileSystem.cpp
    G3D/Line.cpp
    G3D/LineSegment.cpp
    G3D/Log.cpp
    G3D/Matrix3.cpp
    G3D/Matrix4.cpp
    G3D/MemoryManager.cpp
    G3D/PhysicsFrame.cpp
    G3D/Plane.cpp
    G3D/Quat.cpp
    G3D/Random.cpp
    G3D/Ray.cpp
    G3D/ReferenceCount.cpp
    G3D/RegistryUtil.cpp
    G3D/Sphere.cpp
    G3D/System.cpp
    G3D/TextInput.cpp
    G3D/TextOutput.cpp
    G3D/Triangle.cpp
    G3D/UprightFrame.cpp
    G3D/Vector2.cpp
    G3D/Vector3.cpp
    G3D/Vector4.cpp
    G3D/debugAssert.cpp
    G3D/fileutils.cpp
    G3D/format.cpp
    G3D/g3dfnmatch.cpp
    G3D/g3dmath.cpp
    G3D/prompt.cpp
    G3D/stringutils.cpp
    G3D/uint128.cpp
)

target_link_libraries(g3dlite
    PRIVATE
        ZLIB::ZLIB
)

target_include_directories(g3dlite
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
