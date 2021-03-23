#
# Copyright (C) 1995-2011 Mark Adler
# For conditions of distribution and use, see copyright notice in zlib.h
#

set(zlib_SRCS
  adler32.c
  compress.c
  crc32.c
  deflate.c
  gzclose.c
  gzlib.c
  gzread.c
  gzwrite.c
  infback.c
  inffast.c
  inflate.c
  inftrees.c
  trees.c
  uncompr.c
  zutil.c
)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_library(zlib STATIC
  ${zlib_SRCS}
)
