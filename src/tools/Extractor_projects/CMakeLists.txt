# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
#
# Copyright (C) 2005-2021 MaNGOS <https://getmangos.eu>
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

# Used for install targets
set(TOOLS_DIR "tools")
set(EXTRACTOR_BINARIES_DIR "${CMAKE_SOURCE_DIR}/src/tools/Extractor_Binaries")

set(SHARED_SRCS
    shared/ExtractorCommon.cpp
    shared/ExtractorCommon.h
)

#=======================================================#
#map-extractor
#=======================================================#
add_executable(map-extractor
    map-extractor/System.cpp
    map-extractor/dbcfile.cpp
    ${SHARED_SRCS}
    $<$<BOOL:${WIN32}>:map-extractor/map-extractor.rc>
)

target_include_directories(map-extractor
    PUBLIC
        shared
        map-extractor
)

target_link_libraries(map-extractor
    PUBLIC
    loadlib
)

install(
    TARGETS map-extractor
    DESTINATION "${BIN_DIR}/${TOOLS_DIR}"
)

if(WIN32 AND MSVC)
    install(
        FILES $<TARGET_PDB_FILE:map-extractor>
        DESTINATION ${BIN_DIR}/${TOOLS_DIR}
        OPTIONAL)
endif()

#=======================================================#
#vmap-extractor
#=======================================================#
add_executable(vmap-extractor
    vmap-extractor/adtfile.cpp
    vmap-extractor/adtfile.h
    vmap-extractor/dbcfile.cpp
    vmap-extractor/dbcfile.h
    vmap-extractor/assembler.cpp
    vmap-extractor/model.cpp
    vmap-extractor/model.h
    vmap-extractor/modelheaders.h
    vmap-extractor/vec3d.h
    vmap-extractor/vmapexport.cpp
    vmap-extractor/vmapexport.h
    vmap-extractor/wdtfile.cpp
    vmap-extractor/wdtfile.h
    vmap-extractor/wmo.cpp
    vmap-extractor/wmo.h
    ${SHARED_SRCS}
    $<$<BOOL:${WIN32}>:vmap-extractor/vmap-extractor.rc>
)

target_include_directories(vmap-extractor
    PUBLIC
        shared
        vmap-extractor
)

target_link_libraries(vmap-extractor
    PUBLIC
        loadlib
        vmap2
)

install(
    TARGETS vmap-extractor
    DESTINATION ${BIN_DIR}/${TOOLS_DIR}
)

if(WIN32 AND MSVC)
    install(
        FILES $<TARGET_PDB_FILE:vmap-extractor>
        DESTINATION ${BIN_DIR}/${TOOLS_DIR}
        OPTIONAL
)
endif()

#=======================================================#
#mmap-extractor
#=======================================================#
add_executable(mmap-extractor
    Movemap-Generator/generator.cpp
    Movemap-Generator/IntermediateValues.cpp
    Movemap-Generator/IntermediateValues.h
    Movemap-Generator/MangosMap.h
    Movemap-Generator/MapBuilder.cpp
    Movemap-Generator/MapBuilder.h
    Movemap-Generator/MMapCommon.h
    Movemap-Generator/TerrainBuilder.cpp
    Movemap-Generator/TerrainBuilder.h
    Movemap-Generator/TileMsgBlock.h
    Movemap-Generator/TileThreadPool.cpp
    Movemap-Generator/TileThreadPool.h
    Movemap-Generator/VMapExtensions.cpp
    shared/ExtractorCommon.cpp
    shared/ExtractorCommon.h
    $<$<BOOL:${WIN32}>:Movemap-Generator/Movemap-Generator.rc>
)

target_include_directories(mmap-extractor
    PUBLIC
        shared
        Movemap-Generator
)

target_link_libraries(mmap-extractor
    PUBLIC
        loadlib
        vmap2
        shared
        RecastNavigation::Recast
        Threads::Threads
        DL::DL
)

install(
    TARGETS mmap-extractor
    DESTINATION ${BIN_DIR}/${TOOLS_DIR}
)

install (
    FILES "${EXTRACTOR_BINARIES_DIR}/MoveMapGen.sh"
         DESTINATION ${BIN_DIR}/${TOOLS_DIR}
)
install (
    FILES "${EXTRACTOR_BINARIES_DIR}/offmesh.txt"
         DESTINATION ${BIN_DIR}/${TOOLS_DIR}
)

install (
    FILES "${EXTRACTOR_BINARIES_DIR}/mmap_excluded.txt"
         DESTINATION ${BIN_DIR}/${TOOLS_DIR}
)

if(WIN32 AND MSVC)
    install(
        FILES $<TARGET_PDB_FILE:mmap-extractor>
        DESTINATION ${BIN_DIR}/${TOOLS_DIR}
        OPTIONAL
)
endif()
