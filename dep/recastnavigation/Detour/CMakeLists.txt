project(Detour VERSION 1.0.0)

file(GLOB DTR_SOURCES Source/*.cpp)
file(GLOB DTR_INCLUDES Include/*.h)

add_library(Detour STATIC ${DTR_SOURCES} ${DTR_INCLUDES})
add_library(RecastNavigation::Detour ALIAS Detour)

target_include_directories(Detour
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

target_compile_definitions(Detour
    PUBLIC
        DT_POLYREF64
)

set_target_properties(Detour
    PROPERTIES
        VERSION 1.0.0
)
