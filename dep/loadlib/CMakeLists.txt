# This code is part of MaNGOS. Contributor & Copyright details are in AUTHORS/THANKS.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


# Since the different tools from mpqlib and StormLib are fairly different, so is the loadlib.
# Therefore, I have temporarily split the lib into two versions depending on which toolset is compiled.
# These can definitely be merged at some point, however we need to document and understand the differences to do so.


add_library(loadlib STATIC
  loadlib.cpp
  adt.cpp
  wdt.cpp
  mpq.cpp)

target_include_directories(loadlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/sl)

target_link_libraries(loadlib PUBLIC stormlib)