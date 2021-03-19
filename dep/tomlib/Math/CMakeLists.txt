if (NOT TARGET TomLib::Math)

file(GLOB TM_SOURCES src/*.c)
file(GLOB TM_HEADERS src/*.h)

add_library(tommath STATIC
    ${TM_SOURCES}
    ${TM_HEADERS}
)

add_library(TomLib::Math ALIAS tommath)

target_include_directories(tommath
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

endif()



