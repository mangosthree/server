# This code is part of MaNGOS. Contributor & Copyright details are in AUTHORS/THANKS.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

cmake_minimum_required (VERSION 2.8)

project (MANGOS_MAP_EXTRACTOR)


set(SRC_FILES
           ../../dep/StormLib/src/adpcm/adpcm.cpp
           ../../dep/StormLib/src/huffman/huff.cpp
           ../../dep/StormLib/src/jenkins/lookup3.c
           ../../dep/StormLib/src/lzma/C/LzFind.c
           ../../dep/StormLib/src/lzma/C/LzmaDec.c
           ../../dep/StormLib/src/lzma/C/LzmaEnc.c
           ../../dep/StormLib/src/pklib/explode.c
           ../../dep/StormLib/src/pklib/implode.c
           ../../dep/StormLib/src/sparse/sparse.cpp
           ../../dep/StormLib/src/FileStream.cpp
           ../../dep/StormLib/src/SBaseCommon.cpp
           ../../dep/StormLib/src/SBaseDumpData.cpp
           ../../dep/StormLib/src/SBaseFileTable.cpp
           ../../dep/StormLib/src/SCompression.cpp
           ../../dep/StormLib/src/SFileAddFile.cpp
           ../../dep/StormLib/src/SFileAttributes.cpp
           ../../dep/StormLib/src/SFileCompactArchive.cpp
           ../../dep/StormLib/src/SFileCreateArchive.cpp
           ../../dep/StormLib/src/SFileExtractFile.cpp
           ../../dep/StormLib/src/SFileFindFile.cpp
           ../../dep/StormLib/src/SFileListFile.cpp
           ../../dep/StormLib/src/SFileOpenArchive.cpp
           ../../dep/StormLib/src/SFileOpenFileEx.cpp
           ../../dep/StormLib/src/SFilePatchArchives.cpp
           ../../dep/StormLib/src/SFileReadFile.cpp
           ../../dep/StormLib/src/SFileVerify.cpp
)

set(TOMCRYPT_FILES
           ../../dep/StormLib/src/libtomcrypt/src/hashes/hash_memory.c
           ../../dep/StormLib/src/libtomcrypt/src/hashes/md5.c
           ../../dep/StormLib/src/libtomcrypt/src/hashes/sha1.c
           ../../dep/StormLib/src/libtomcrypt/src/math/ltm_desc.c
           ../../dep/StormLib/src/libtomcrypt/src/math/multi.c
           ../../dep/StormLib/src/libtomcrypt/src/math/rand_prime.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/base64_decode.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_argchk.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_find_hash.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_find_prng.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_hash_descriptor.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_hash_is_valid.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_libc.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_ltc_mp_descriptor.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_prng_descriptor.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_prng_is_valid.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_register_hash.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/crypt_register_prng.c
           ../../dep/StormLib/src/libtomcrypt/src/misc/zeromem.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_bit_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_boolean.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_choice.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_ia5_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_integer.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_object_identifier.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_octet_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_printable_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_sequence_ex.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_sequence_flexi.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_sequence_multi.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_short_integer.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_utctime.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_decode_utf8_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_bit_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_boolean.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_ia5_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_integer.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_object_identifier.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_octet_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_printable_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_sequence.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_utctime.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_sequence_free.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_utf8_string.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/asn1/der_length_short_integer.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/ecc/ltc_ecc_map.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/ecc/ltc_ecc_mul2add.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/ecc/ltc_ecc_mulmod.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/ecc/ltc_ecc_points.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/ecc/ltc_ecc_projective_add_point.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/ecc/ltc_ecc_projective_dbl_point.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/pkcs1/pkcs_1_mgf1.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_decode.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/pkcs1/pkcs_1_pss_decode.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_decode.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/rsa/rsa_exptmod.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/rsa/rsa_free.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/rsa/rsa_import.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/rsa/rsa_make_key.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/rsa/rsa_verify_hash.c
           ../../dep/StormLib/src/libtomcrypt/src/pk/rsa/rsa_verify_simple.c
)

set(TOMMATH_FILES
           ../../dep/StormLib/src/libtommath/bncore.c
           ../../dep/StormLib/src/libtommath/bn_fast_mp_invmod.c
           ../../dep/StormLib/src/libtommath/bn_fast_mp_montgomery_reduce.c
           ../../dep/StormLib/src/libtommath/bn_fast_s_mp_mul_digs.c
           ../../dep/StormLib/src/libtommath/bn_fast_s_mp_mul_high_digs.c
           ../../dep/StormLib/src/libtommath/bn_fast_s_mp_sqr.c
           ../../dep/StormLib/src/libtommath/bn_mp_2expt.c
           ../../dep/StormLib/src/libtommath/bn_mp_abs.c
           ../../dep/StormLib/src/libtommath/bn_mp_add.c
           ../../dep/StormLib/src/libtommath/bn_mp_addmod.c
           ../../dep/StormLib/src/libtommath/bn_mp_add_d.c
           ../../dep/StormLib/src/libtommath/bn_mp_and.c
           ../../dep/StormLib/src/libtommath/bn_mp_clamp.c
           ../../dep/StormLib/src/libtommath/bn_mp_clear.c
           ../../dep/StormLib/src/libtommath/bn_mp_clear_multi.c
           ../../dep/StormLib/src/libtommath/bn_mp_cmp.c
           ../../dep/StormLib/src/libtommath/bn_mp_cmp_d.c
           ../../dep/StormLib/src/libtommath/bn_mp_cmp_mag.c
           ../../dep/StormLib/src/libtommath/bn_mp_cnt_lsb.c
           ../../dep/StormLib/src/libtommath/bn_mp_copy.c
           ../../dep/StormLib/src/libtommath/bn_mp_count_bits.c
           ../../dep/StormLib/src/libtommath/bn_mp_div.c
           ../../dep/StormLib/src/libtommath/bn_mp_div_2.c
           ../../dep/StormLib/src/libtommath/bn_mp_div_2d.c
           ../../dep/StormLib/src/libtommath/bn_mp_div_3.c
           ../../dep/StormLib/src/libtommath/bn_mp_div_d.c
           ../../dep/StormLib/src/libtommath/bn_mp_dr_is_modulus.c
           ../../dep/StormLib/src/libtommath/bn_mp_dr_reduce.c
           ../../dep/StormLib/src/libtommath/bn_mp_dr_setup.c
           ../../dep/StormLib/src/libtommath/bn_mp_exch.c
           ../../dep/StormLib/src/libtommath/bn_mp_exptmod.c
           ../../dep/StormLib/src/libtommath/bn_mp_exptmod_fast.c
           ../../dep/StormLib/src/libtommath/bn_mp_expt_d.c
           ../../dep/StormLib/src/libtommath/bn_mp_exteuclid.c
           ../../dep/StormLib/src/libtommath/bn_mp_fread.c
           ../../dep/StormLib/src/libtommath/bn_mp_fwrite.c
           ../../dep/StormLib/src/libtommath/bn_mp_gcd.c
           ../../dep/StormLib/src/libtommath/bn_mp_get_int.c
           ../../dep/StormLib/src/libtommath/bn_mp_grow.c
           ../../dep/StormLib/src/libtommath/bn_mp_init.c
           ../../dep/StormLib/src/libtommath/bn_mp_init_copy.c
           ../../dep/StormLib/src/libtommath/bn_mp_init_multi.c
           ../../dep/StormLib/src/libtommath/bn_mp_init_set.c
           ../../dep/StormLib/src/libtommath/bn_mp_init_set_int.c
           ../../dep/StormLib/src/libtommath/bn_mp_init_size.c
           ../../dep/StormLib/src/libtommath/bn_mp_invmod.c
           ../../dep/StormLib/src/libtommath/bn_mp_invmod_slow.c
           ../../dep/StormLib/src/libtommath/bn_mp_is_square.c
           ../../dep/StormLib/src/libtommath/bn_mp_jacobi.c
           ../../dep/StormLib/src/libtommath/bn_mp_karatsuba_mul.c
           ../../dep/StormLib/src/libtommath/bn_mp_karatsuba_sqr.c
           ../../dep/StormLib/src/libtommath/bn_mp_lcm.c
           ../../dep/StormLib/src/libtommath/bn_mp_lshd.c
           ../../dep/StormLib/src/libtommath/bn_mp_mod.c
           ../../dep/StormLib/src/libtommath/bn_mp_mod_2d.c
           ../../dep/StormLib/src/libtommath/bn_mp_mod_d.c
           ../../dep/StormLib/src/libtommath/bn_mp_montgomery_calc_normalization.c
           ../../dep/StormLib/src/libtommath/bn_mp_montgomery_reduce.c
           ../../dep/StormLib/src/libtommath/bn_mp_montgomery_setup.c
           ../../dep/StormLib/src/libtommath/bn_mp_mul.c
           ../../dep/StormLib/src/libtommath/bn_mp_mulmod.c
           ../../dep/StormLib/src/libtommath/bn_mp_mul_2.c
           ../../dep/StormLib/src/libtommath/bn_mp_mul_2d.c
           ../../dep/StormLib/src/libtommath/bn_mp_mul_d.c
           ../../dep/StormLib/src/libtommath/bn_mp_neg.c
           ../../dep/StormLib/src/libtommath/bn_mp_n_root.c
           ../../dep/StormLib/src/libtommath/bn_mp_or.c
           ../../dep/StormLib/src/libtommath/bn_mp_prime_fermat.c
           ../../dep/StormLib/src/libtommath/bn_mp_prime_is_divisible.c
           ../../dep/StormLib/src/libtommath/bn_mp_prime_is_prime.c
           ../../dep/StormLib/src/libtommath/bn_mp_prime_miller_rabin.c
           ../../dep/StormLib/src/libtommath/bn_mp_prime_next_prime.c
           ../../dep/StormLib/src/libtommath/bn_mp_prime_rabin_miller_trials.c
           ../../dep/StormLib/src/libtommath/bn_mp_prime_random_ex.c
           ../../dep/StormLib/src/libtommath/bn_mp_radix_size.c
           ../../dep/StormLib/src/libtommath/bn_mp_radix_smap.c
           ../../dep/StormLib/src/libtommath/bn_mp_rand.c
           ../../dep/StormLib/src/libtommath/bn_mp_read_radix.c
           ../../dep/StormLib/src/libtommath/bn_mp_read_signed_bin.c
           ../../dep/StormLib/src/libtommath/bn_mp_read_unsigned_bin.c
           ../../dep/StormLib/src/libtommath/bn_mp_reduce.c
           ../../dep/StormLib/src/libtommath/bn_mp_reduce_2k.c
           ../../dep/StormLib/src/libtommath/bn_mp_reduce_2k_l.c
           ../../dep/StormLib/src/libtommath/bn_mp_reduce_2k_setup.c
           ../../dep/StormLib/src/libtommath/bn_mp_reduce_2k_setup_l.c
           ../../dep/StormLib/src/libtommath/bn_mp_reduce_is_2k.c
           ../../dep/StormLib/src/libtommath/bn_mp_reduce_is_2k_l.c
           ../../dep/StormLib/src/libtommath/bn_mp_reduce_setup.c
           ../../dep/StormLib/src/libtommath/bn_mp_rshd.c
           ../../dep/StormLib/src/libtommath/bn_mp_set.c
           ../../dep/StormLib/src/libtommath/bn_mp_set_int.c
           ../../dep/StormLib/src/libtommath/bn_mp_shrink.c
           ../../dep/StormLib/src/libtommath/bn_mp_signed_bin_size.c
           ../../dep/StormLib/src/libtommath/bn_mp_sqr.c
           ../../dep/StormLib/src/libtommath/bn_mp_sqrmod.c
           ../../dep/StormLib/src/libtommath/bn_mp_sqrt.c
           ../../dep/StormLib/src/libtommath/bn_mp_sub.c
           ../../dep/StormLib/src/libtommath/bn_mp_submod.c
           ../../dep/StormLib/src/libtommath/bn_mp_sub_d.c
           ../../dep/StormLib/src/libtommath/bn_mp_toom_mul.c
           ../../dep/StormLib/src/libtommath/bn_mp_toom_sqr.c
           ../../dep/StormLib/src/libtommath/bn_mp_toradix.c
           ../../dep/StormLib/src/libtommath/bn_mp_toradix_n.c
           ../../dep/StormLib/src/libtommath/bn_mp_to_signed_bin.c
           ../../dep/StormLib/src/libtommath/bn_mp_to_signed_bin_n.c
           ../../dep/StormLib/src/libtommath/bn_mp_to_unsigned_bin.c
           ../../dep/StormLib/src/libtommath/bn_mp_to_unsigned_bin_n.c
           ../../dep/StormLib/src/libtommath/bn_mp_unsigned_bin_size.c
           ../../dep/StormLib/src/libtommath/bn_mp_xor.c
           ../../dep/StormLib/src/libtommath/bn_mp_zero.c
           ../../dep/StormLib/src/libtommath/bn_prime_tab.c
           ../../dep/StormLib/src/libtommath/bn_reverse.c
           ../../dep/StormLib/src/libtommath/bn_s_mp_add.c
           ../../dep/StormLib/src/libtommath/bn_s_mp_exptmod.c
           ../../dep/StormLib/src/libtommath/bn_s_mp_mul_digs.c
           ../../dep/StormLib/src/libtommath/bn_s_mp_mul_high_digs.c
           ../../dep/StormLib/src/libtommath/bn_s_mp_sqr.c
           ../../dep/StormLib/src/libtommath/bn_s_mp_sub.c
)

set(ZLIB_BZIP2_FILES
           ../../dep/StormLib/src/bzip2/blocksort.c
           ../../dep/StormLib/src/bzip2/bzlib.c
           ../../dep/StormLib/src/bzip2/compress.c
           ../../dep/StormLib/src/bzip2/crctable.c
           ../../dep/StormLib/src/bzip2/decompress.c
           ../../dep/StormLib/src/bzip2/huffman.c
           ../../dep/StormLib/src/bzip2/randtable.c
           ../../dep/StormLib/src/zlib/adler32.c
           ../../dep/StormLib/src/zlib/compress2.c
           ../../dep/StormLib/src/zlib/crc32.c
           ../../dep/StormLib/src/zlib/deflate.c
           ../../dep/StormLib/src/zlib/inffast.c
           ../../dep/StormLib/src/zlib/inflate.c
           ../../dep/StormLib/src/zlib/inftrees.c
           ../../dep/StormLib/src/zlib/trees.c
           ../../dep/StormLib/src/zlib/zutil.c
)



# set(TEST_SRC_FILES
#            test/Test.cpp
# )

add_definitions(-D_7ZIP_ST -DBZ_STRICT_ANSI)

if(WIN32)
    if(MSVC)
        message(STATUS "Using MSVC")
        add_definitions(-D_7ZIP_ST -DWIN32)
    else()
        message(STATUS "Using mingw")
    endif()
    set(SRC_ADDITIONAL_FILES ${ZLIB_BZIP2_FILES} ${TOMCRYPT_FILES} ${TOMMATH_FILES})
    set(LINK_LIBS wininet)
endif()

if(APPLE)
    message(STATUS "Using Mac OS X port")
    set(LINK_LIBS z bz2)
    set(SRC_ADDITIONAL_FILES ${TOMCRYPT_FILES} ${TOMMATH_FILES})
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    message(STATUS "Using Linux port")
    option(WITH_LIBTOMCRYPT "Use system LibTomCrypt library" OFF)
    if(WITH_LIBTOMCRYPT)
        set(LINK_LIBS z bz2 tomcrypt)
    else()
        set(LINK_LIBS z bz2)
        set(SRC_ADDITIONAL_FILES ${TOMCRYPT_FILES} ${TOMMATH_FILES})
    endif()
endif()

add_library(storm SHARED ${SRC_FILES} ${SRC_ADDITIONAL_FILES})
target_link_libraries(storm ${LINK_LIBS})

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../dep/StormLib/src
	${MANGOS_MAP_EXTRACTOR_SOURCE_DIR}/loadlib
)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../dep/StormLib/src
	${MANGOS_MAP_EXTRACTOR_SOURCE_DIR}/loadlib
)

add_subdirectory (loadlib)

add_executable (ad dbcfile.cpp System.cpp)
set_target_properties(ad PROPERTIES PUBLIC_HEADER "../../dep/StormLib/src/StormLib.h")
target_link_libraries (ad storm loadlib)


