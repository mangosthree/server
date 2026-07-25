# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
#
# Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
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

# =============================================================================
# Which vendored dependencies this fork builds.
#
# `dep` is the shared `mangos/mangosDeps` submodule and is never modified here,
# so the choice cannot live in its own CMakeLists: that file still lists
# acelite, which the Stage 2 ACE-removal campaign deleted from this fork's own
# source but cannot delete from the shared submodule. Adding the subdirectories
# from the outside is how the selection is attached without touching the
# submodule -- the same mechanism cmake/SubmoduleCompat.cmake already uses for
# Eluna/SD3.
#
# Add a dependency here rather than in dep/CMakeLists.txt. Anything left out is
# simply never configured, so acelite costs no build time and cannot be linked
# back in by accident. This fork still uses g3dlite (unlike MangosTwo, which
# replaced it with src/shared/Geometry and dropped it) -- keep it.
# =============================================================================

set(MANGOS_DEP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dep")

if(NOT EXISTS "${MANGOS_DEP_DIR}")
    message(FATAL_ERROR
        "The 'dep' submodule is missing. Clone recursively, or run "
        "'git submodule update --init --recursive'.")
endif()

if(NOT TARGET "ZLIB::ZLIB")
    add_subdirectory(${MANGOS_DEP_DIR}/zlib dep/zlib)
endif()

if(NOT TARGET "BZip2::BZip2")
    add_subdirectory(${MANGOS_DEP_DIR}/bzip2 dep/bzip2)
endif()

add_subdirectory(${MANGOS_DEP_DIR}/utf8cpp dep/utf8cpp)

if(BUILD_MANGOSD)
    if(SCRIPT_LIB_ELUNA)
        add_subdirectory(${MANGOS_DEP_DIR}/lualib dep/lualib)
    endif()
    if(SOAP)
        add_subdirectory(${MANGOS_DEP_DIR}/gsoap dep/gsoap)
    endif()
endif()

if(BUILD_MANGOSD OR BUILD_TOOLS)
    add_subdirectory(${MANGOS_DEP_DIR}/recastnavigation dep/recastnavigation)
    add_subdirectory(${MANGOS_DEP_DIR}/g3dlite dep/g3dlite)
endif()

if(BUILD_TOOLS)
    add_subdirectory(${MANGOS_DEP_DIR}/StormLib dep/StormLib)
endif()
