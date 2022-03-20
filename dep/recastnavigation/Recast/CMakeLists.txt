project(Recast VERSION 1.0.0)

file(GLOB RCT_SOURCES Source/*.cpp)
file(GLOB RCT_INCLUDES Include/*.h)

add_library(Recast STATIC ${RCT_SOURCES} ${RCT_INCLUDES})
add_library(RecastNavigation::Recast ALIAS Recast)

target_include_directories(Recast
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

set_target_properties(Recast
    PROPERTIES
        VERSION 1.0.0
)
