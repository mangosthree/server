project(bzip2 VERSION 1.0.6)

add_library(bzip2 STATIC
    blocksort.c
    bzlib.c
    compress.c
    crctable.c
    decompress.c
    huffman.c
    randtable.c
    bzlib.h
    bzlib_private.h
)

add_library(BZip2::BZip2 ALIAS bzip2)

target_include_directories(bzip2
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(bzip2
    PUBLIC
        BZ_STRICT_ANSI
)
