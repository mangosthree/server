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


if(USE_STORMLIB)
    include_directories(
        ${CMAKE_SOURCE_DIR}/dep/StormLib/src
        ${CMAKE_CURRENT_SOURCE_DIR}/sl
    )
    add_library(loadlib sl/loadlib.cpp sl/adt.cpp sl/wdt.cpp sl/mpq.cpp)
    target_link_libraries(loadlib storm)
else()
    include_directories(
        ${CMAKE_SOURCE_DIR}/dep/libmpq
        ${CMAKE_CURRENT_SOURCE_DIR}/ml
    )
    add_library(loadlib ml/loadlib.cpp ml/adt.cpp ml/wdt.cpp ml/mpq.cpp)
endif()

# link loadlib with zlib
target_link_libraries(loadlib zlib)
