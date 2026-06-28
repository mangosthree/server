# MaNGOS unit-test harness (doctest).
#
# Enabled with -DBUILD_TESTS=ON. Off by default so normal/CI server builds are
# unaffected. doctest is a single, vendored header (tests/doctest/doctest.h);
# nothing extra is fetched at configure time.
#
# This is the standalone harness scaffold: it has no dependency on the game
# library yet. Linkage against `game`/`shared` is introduced together with the
# first tests that exercise engine code.

file(GLOB MANGOS_TEST_SOURCES *.cpp)

add_executable(mangos_tests ${MANGOS_TEST_SOURCES})

target_include_directories(mangos_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(mangos_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_test(NAME mangos_tests COMMAND mangos_tests)
