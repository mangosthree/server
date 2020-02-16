project(StormLib VERSION 9.22.0)

add_library(stormlib STATIC
    src/adpcm/adpcm.cpp
    src/huffman/huff.cpp
    src/jenkins/lookup3.c
    src/lzma/C/LzFind.c
    src/lzma/C/LzmaDec.c
    src/lzma/C/LzmaEnc.c
    src/pklib/explode.c
    src/pklib/implode.c
    src/sparse/sparse.cpp
    src/rsa/rsa_verify_simple.c
    src/FileStream.cpp
    src/SBaseCommon.cpp
    src/SBaseDumpData.cpp
    src/SBaseFileTable.cpp
    src/SBaseSubTypes.cpp
    src/SCompression.cpp
    src/SFileAddFile.cpp
    src/SFileAttributes.cpp
    src/SFileCompactArchive.cpp
    src/SFileCreateArchive.cpp
    src/SFileExtractFile.cpp
    src/SFileFindFile.cpp
    src/SFileGetFileInfo.cpp
    src/SFileListFile.cpp
    src/SFileOpenArchive.cpp
    src/SFileOpenFileEx.cpp
    src/SFilePatchArchives.cpp
    src/SFileReadFile.cpp
    src/SFileVerify.cpp
)

target_include_directories(stormlib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(stormlib
    PUBLIC
        _7ZIP_ST
        __SYS_BZLIB
        __SYS_ZLIB
        LTM_DESC
    INTERFACE
        STORMLIB_NO_AUTO_LINK
)

target_link_libraries(stormlib
    PRIVATE
        ZLIB::ZLIB
        BZip2::BZip2
        TomLib::Crypt
        TomLib::Math
)