# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
#
# Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

# Cata-4.3.4 ADT/WDT/MPQ reader for the extractors. This is the StormLib
# variant formerly vendored at dep/loadlib/sl/; it carries Cata-specific
# client-format parsing and so lives with the extractors (not in the shared
# mangosDeps dep submodule, whose generic loadlib does not read Cata).

project(LOADLIB_CATA)

add_library(loadlib_cata STATIC
  loadlib.cpp
  adt.cpp
  wdt.cpp
  mpq.cpp)

target_include_directories(loadlib_cata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(loadlib_cata PUBLIC stormlib)
