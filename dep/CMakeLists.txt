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

if(NOT ACE_USE_EXTERNAL)
  add_subdirectory(acelite)
endif()

if(SOAP)
    add_subdirectory(gsoap)
endif()

if(SCRIPT_LIB_ELUNA)
    add_subdirectory(lualib)
endif()

if(USE_STORMLIB)
    add_subdirectory(StormLib)
else()
    add_subdirectory(libmpq)
endif()

add_subdirectory(recastnavigation)
add_subdirectory(g3dlite)
add_subdirectory(zlib)
add_subdirectory(loadlib)
add_subdirectory(bzip2)
