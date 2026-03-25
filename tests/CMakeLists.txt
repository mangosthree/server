# MaNGOS is a full featured server for World of Warcraft, supporting
# the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
#
# Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

cmake_minimum_required(VERSION 3.18)

project(MaNGOSTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable CTest
enable_testing()

# Test executable sources
set(TEST_SOURCES
    test_main.cpp
    test_ByteBuffer.cpp
    test_ByteConverter.cpp
    test_ObjectGuid.cpp
    test_Timer.cpp
    test_Util.cpp
    test_Common.cpp
    test_WorldPacket.cpp
    test_GameLogic.cpp
    test_AuthTypes.cpp
    test_EventProcessor.cpp
    test_SpellEffects.cpp
    test_AuraSystem.cpp
    test_ThreatSystem.cpp
    test_CombatFormulas.cpp
    test_InventorySystem.cpp
    test_MovementSystem.cpp
    test_GridSystem.cpp
    test_ChatParser.cpp
    test_LootSystem.cpp
    test_PlayerStats.cpp
)

# Build test executable
add_executable(mangos_tests ${TEST_SOURCES})

target_include_directories(mangos_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Platform-specific settings
if(UNIX)
    target_compile_options(mangos_tests PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
        -Wno-sign-compare
    )
endif()

if(MSVC)
    target_compile_options(mangos_tests PRIVATE
        /W3
        /wd4100  # unreferenced formal parameter
        /wd4244  # conversion, possible loss of data
    )
endif()

# Register with CTest
add_test(
    NAME MaNGOSTests
    COMMAND mangos_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Individual test suite targets for targeted runs
add_test(NAME ByteBufferTests    COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME ByteConverterTests COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME ObjectGuidTests    COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME TimerTests         COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME UtilTests          COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME CommonTests        COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME WorldPacketTests   COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME GameLogicTests     COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME AuthTypesTests     COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME EventProcessorTests  COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME SpellEffectsTests    COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME AuraSystemTests      COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME ThreatSystemTests    COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME CombatFormulasTests  COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME InventorySystemTests COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME MovementSystemTests  COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME GridSystemTests      COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME ChatParserTests      COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME LootSystemTests      COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME PlayerStatsTests     COMMAND mangos_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "MaNGOS test suite configured with ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Build with: cmake <path> && cmake --build .")
message(STATUS "Run with:   ctest -V  or  ./mangos_tests")
