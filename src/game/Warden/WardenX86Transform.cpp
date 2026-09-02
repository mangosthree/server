/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include "WardenX86Transform.h"

#include <cstdint>

namespace
{
// This unsigned straight-line transcription preserves the exact x86 bootstrap
// arithmetic, including ARPL and MMX word-lane semantics. It is intentionally
// kept separate from session crypto so custody vectors can audit it directly.
using uint32_t = std::uint32_t;

static uint32_t Rotl(uint32_t v, unsigned count)
{
    return (v << count) | (v >> (32u - count));
}
static uint32_t Not(uint32_t v) { return ~v; }
static uint32_t Neg(uint32_t v) { return 0u - v; }

static uint32_t Bswap(uint32_t v)
{
    return ((v & 0x000000ffu) << 24) |
           ((v & 0x0000ff00u) << 8) |
           ((v & 0x00ff0000u) >> 8) |
           ((v & 0xff000000u) >> 24);
}

static uint32_t Arpl(uint32_t dst, uint32_t src)
{
    return (dst & 3u) < (src & 3u)
        ? (dst & ~3u) | (src & 3u)
        : dst;
}

static uint32_t OddMix(uint32_t a, uint32_t b, uint32_t c)
{
    uint32_t value = (b | 1u) * a;
    value ^= b & 1u;
    value *= c | 1u;
    return value ^ (c & 1u);
}

static std::int32_t SignedWord(uint32_t v)
{
    v &= 0xffffu;
    return v < 0x8000u ? static_cast<std::int32_t>(v)
                       : static_cast<std::int32_t>(v) - 0x10000;
}

static uint32_t WordMix(uint32_t a, uint32_t b)
{
    uint32_t p0 = static_cast<uint32_t>(SignedWord(a) * SignedWord(b));
    uint32_t p1 = static_cast<uint32_t>(SignedWord(a >> 16) * SignedWord(b >> 16));
    uint32_t packedLowProducts = (p0 & 0xffffu) | ((p1 & 0xffffu) << 16);
    return p0 + p1 + packedLowProducts;
}

static void PairMix(uint32_t* first, uint32_t* second, uint32_t x, uint32_t y)
{
    uint32_t oldFirst = *first;
    uint32_t oldSecond = *second;
    *first = WordMix(oldSecond, y);
    *second = WordMix(oldFirst, x);
}

static uint32_t TransformWords(uint32_t (&w)[4])
{
    uint32_t eax_3 = Rotl(w[0] + 0x22736694, 0xa) ^ 0x547d2e51;
    uint32_t ecx_4 = Rotl(w[1] - eax_3 + 0x2f36c3a8, 1) + 0x37a5f420;
    uint32_t edx_5 = Rotl((ecx_4 ^ eax_3 ^ 0x16e76f46) + w[2] - 0x6dbf5e3a, 0xd) ^ eax_3;
    uint32_t edi_5 = Rotl(w[3] - ecx_4 + edx_5 + eax_3 + 0x77335b1c, 4);
    uint32_t var_10 = ecx_4;
    uint32_t var_14 = eax_3;
    uint32_t var_c = edi_5 + eax_3;
    PairMix(&var_c, &var_14, edx_5, ecx_4);
    uint32_t edx_7 = w[0];
    uint32_t edi_7 = var_c;
    uint32_t ecx_6 = var_10;
    uint32_t eax_10 = Rotl(edx_7 - ecx_6 + edi_7 + edx_5 + var_14 + 0xfda58f0, 5) - edi_7;
    uint32_t ecx_9 = Rotl((((edi_7 ^ eax_10) & edx_5) ^ edi_7) + edx_7 + ecx_6 + 0x34a74d57, 0xe) + Not(edx_5);
    uint32_t edx_11 = Rotl((((ecx_9 ^ eax_10) & edi_7) ^ (ecx_9 & eax_10)) + edx_7 + edx_5 - 0x3ab3dde1, 5) ^ Bswap(Not(eax_10));
    uint32_t edi_19 = Rotl(w[3] - edx_11 + var_c + ecx_9 + eax_10 - 0x8310d5f, 0x15) - Bswap(ecx_9);
    var_c = edi_19;
    uint32_t edi_20 = w[1];
    uint32_t eax_13 = Rotl((((edx_11 ^ ecx_9) & edi_19) ^ ecx_9) + edi_20 + eax_10 - 0x584d7e57, 0x14) + Not(var_c);
    var_14 = eax_13;
    uint32_t eax_20 = Rotl(((Not(eax_13) | edx_11) ^ var_c) + edi_20 + ecx_9 + 0x2ea61982, 9) - edx_11;
    var_10 = eax_20;
    var_14 = Arpl(var_14, var_c);
    uint32_t ebx_22 = var_10;
    uint32_t ecx_10 = var_14;
    uint32_t edi_23 = Rotl((var_c ^ ebx_22 ^ ecx_10) + w[0] + edx_11 + 0x1fea086e, 0x14) - var_c;
    uint32_t ecx_14 = Rotl(OddMix(ecx_10, edi_23, ebx_22) + w[1] + var_c + 0x74e42b10, 0x17) - var_14;
    uint32_t eax_33 = Rotl((((edi_23 ^ ebx_22) & ecx_14) ^ (edi_23 & ebx_22)) + w[0] + var_14 + 0x7063dd18, 0x10) + Bswap(ebx_22);
    uint32_t edx_28 = Rotl((((edi_23 ^ eax_33) & ecx_14) ^ (edi_23 & eax_33)) + w[1] + var_10 + 0x6fce255b, 5) + eax_33 + 1;
    uint32_t ebx_30 = w[2] - ecx_14 + edi_23 + edx_28;
    var_c = ecx_14;
    var_14 = eax_33;
    var_10 = edx_28;
    var_10 = Arpl(var_10, var_c);
    uint32_t eax_36 = var_10;
    uint32_t edx_30 = var_c;
    uint32_t ecx_22 = ((((Rotl(ebx_30 + eax_33 + 0x38b63883, 0x14) + eax_33 + 1) ^ var_14) & eax_36) ^ (Rotl(ebx_30 + eax_33 + 0x38b63883, 0x14) + eax_33 + 1)) + w[3] + edx_30 - 0x16f55ba7;
    var_c = Rotl(ecx_22, 0x1f) + eax_36 + 1;
    var_14 = Arpl(var_14, Rotl(ebx_30 + eax_33 + 0x38b63883, 0x14) + eax_33 + 1);
    uint32_t eax_39 = var_10;
    uint32_t ebx_31 = var_c;
    uint32_t edx_37 = Rotl(((Not(eax_39) | ebx_31) ^ (Rotl(ebx_30 + eax_33 + 0x38b63883, 0x14) + eax_33 + 1)) + w[2] + var_14 - 0x3b819116, 0x10) + ebx_31 + 1;
    uint32_t eax_40 = eax_39 + w[1] - (Rotl(ebx_30 + eax_33 + 0x38b63883, 0x14) + eax_33 + 1) + ebx_31 + edx_37 - Bswap(ebx_31) + 0x5fd334e4;
    uint32_t edi_34 = Rotl((((eax_40 ^ edx_37) & ebx_31) ^ (eax_40 & edx_37)) + w[0] + Rotl(ebx_30 + eax_33 + 0x38b63883, 0x14) + eax_33 + 1 - 0x16b79203, 0xd) + Not(ebx_31);
    var_14 = edx_37;
    var_10 = eax_40;
    uint32_t eax_41 = OddMix(edi_34, edx_37, eax_40);
    uint32_t eax_46 = Rotl(eax_41 + w[2] + ebx_31 - 0x78a557d4, 2) - edi_34;
    var_c = eax_46;
    var_14 = Arpl(var_14, var_10);
    uint32_t ecx_39 = var_c;
    uint32_t edi_42 = Rotl((((edi_34 ^ var_10) & ecx_39) ^ edi_34) + w[0] + var_14 + 0x566edffd, 0xb) ^ Neg(ecx_39);
    uint32_t ebx_38 = Rotl((ecx_39 ^ edi_34 ^ edi_42) + w[1] + var_10 - 0x7a4a0b97, 0x17) - Bswap(edi_34);
    uint32_t eax_51 = Rotl((((ecx_39 ^ ebx_38) & edi_42) ^ var_c) + w[3] + edi_34 + 0x7baf95dd, 0x1f) ^ Neg(edi_42);
    uint32_t eax_56 = Rotl(OddMix(ebx_38, edi_42, eax_51) + w[0] + var_c + 0x770f0b73, 0xe) ^ Not(ebx_38);
    uint32_t edi_45 = Rotl((((eax_51 ^ ebx_38) & eax_56) ^ (eax_51 & ebx_38)) + w[3] + edi_42 + 0x324db2e2, 0x12) - Bswap(Neg(eax_51));
    uint32_t var_18_1 = Not(edi_45);
    uint32_t ebx_41 = Rotl((((eax_51 ^ edi_45) & eax_56) ^ (eax_51 & edi_45)) + w[3] + ebx_38 - 0x2277ca5e, 6) + Bswap(var_18_1);
    uint32_t ecx_67 = w[1] - eax_56 + eax_51;
    var_c = eax_56;
    uint32_t eax_59 = Rotl(ecx_67 + ebx_41 + edi_45 - 0x416f6a94, 0x14) ^ var_18_1;
    var_14 = edi_45;
    var_10 = ebx_41;
    var_c = Rotl(OddMix(edi_45, ebx_41, eax_59) + w[1] + var_c + 0x5e862c14, 0x16) ^ Neg(edi_45);
    PairMix(&var_c, &var_14, ebx_41, eax_59);
    uint32_t eax_66 = var_c ^ eax_59;
    uint32_t edi_48 = Bswap(Neg(eax_59));
    uint32_t var_28_6 = var_c;
    uint32_t edi_49 = edi_48 ^ ((eax_66 ^ var_10) + w[0] + var_14 + 0x2e59e176);
    uint32_t eax_70 = var_10 + (eax_66 ^ edi_49) + w[3] + var_c - 0x653f5dd3;
    var_10 = eax_70;
    uint32_t eax_72 = OddMix(edi_49, eax_70, var_28_6) + w[2];
    uint32_t edx_58 = var_10;
    uint32_t ebx_45 = Rotl(eax_72 + eax_59 + 0x2bd4a275, 0xb) + Bswap(Neg(edi_49));
    uint32_t eax_77 = OddMix(edi_49, edx_58, ebx_45) + w[2];
    uint32_t edx_59 = var_c + eax_77 + edi_49 + 0x2ed96be8;
    uint32_t ecx_82 = var_10;
    var_c = edx_59;
    uint32_t eax_79 = OddMix(ecx_82, edx_59, ebx_45);
    uint32_t ecx_83 = w[0];
    uint32_t edx_61 = var_10;
    uint32_t eax_82 = Rotl(eax_79 + ecx_83 + edi_49 - 0x4e2c8c9b, 0xe) ^ Not(edx_61);
    var_10 = Rotl((var_c ^ ebx_45 ^ eax_82) + ecx_83 + edx_61 + 0x1ef132c4, 7) ^ var_c;
    var_14 = eax_82;
    PairMix(&var_10, &var_c, eax_82, ebx_45);
    uint32_t ebx_46 = var_c;
    uint32_t edi_56 = w[1];
    uint32_t eax_83 = OddMix(var_10, var_14, ebx_46);
    uint32_t ecx_90 = var_10;
    uint32_t edx_66 = Rotl(eax_83 + edi_56 + ebx_45 + 0x49d23c42, 0xe) + ecx_90;
    uint32_t eax_85 = OddMix(ecx_90, edx_66, var_14);
    uint32_t eax_89 = Rotl(eax_85 + edi_56 + ebx_46 + 0x37ccf876, 0x18) - var_10;
    var_c = eax_89;
    uint32_t eax_90 = Arpl(edx_66, var_14);
    uint32_t eax_93 = Not(var_c) | eax_90;
    uint32_t edx_69 = var_10;
    uint32_t ebx_49 = Rotl((eax_93 ^ edx_69) + w[2] + var_14 + 0x3a6b9ddb, 0x1c) + eax_90;
    uint32_t edx_70 = var_c;
    uint32_t edi_62 = Rotl((eax_93 ^ ebx_49) + w[3] + edx_69 + 0x6d1cbb9f, 0x18) + Bswap(eax_90);
    uint32_t ecx_98 = Rotl(OddMix(edi_62, edx_70, ebx_49) + w[1] + eax_90 - 0x110f2a60, 0x12) - edi_62;
    uint32_t eax_108 = Rotl(((Not(ecx_98) | ebx_49) ^ edi_62) + w[3] + var_c + 0xc70afa9, 0x14) + Bswap(Not(ebx_49));
    var_c = eax_108;
    uint32_t eax_113 = Rotl((((ecx_98 ^ edi_62) & eax_108) ^ (ecx_98 & edi_62)) + w[1] + ebx_49 - 0x77d90420, 0x14) + edi_62;
    uint32_t ebx_56 = Rotl((var_c ^ ecx_98 ^ eax_113) + w[0] + edi_62 - 0x1e872253, 0x1b) ^ Not(var_c);
    uint32_t edi_65 = Rotl(w[2] - var_c + ecx_98 + ebx_56 + eax_113 - 0x7031bfec, 2) + ebx_56;
    uint32_t ecx_106 = Rotl((((ebx_56 ^ eax_113) & edi_65) ^ (ebx_56 & eax_113)) + w[1] + var_c + 0x4017f1d3, 0x1d) + edi_65;
    var_c = ecx_106;
    uint32_t ecx_111 = var_c;
    uint32_t edx_91 = Rotl(((Not(ecx_106) | edi_65) ^ ebx_56) + w[3] + eax_113 - 0x28328fe4, 0x1a) + edi_65 + 1;
    var_14 = edx_91;
    uint32_t eax_117 = OddMix(ecx_111, edx_91, edi_65) + w[2];
    uint32_t edx_92 = w[3];
    uint32_t eax_118 = var_14;
    uint32_t ebx_59 = Rotl(eax_117 + ebx_56 + 0x75bcad03, 2) ^ var_c;
    uint32_t ecx_113 = ebx_59 ^ eax_118;
    uint32_t var_18_3 = ebx_59 & eax_118;
    uint32_t edi_68 = Rotl(((ecx_113 & var_c) ^ var_18_3) + edx_92 + edi_65 + 0x3fefe7c7, 0x1e) ^ Bswap(Neg(var_14));
    uint32_t ecx_119 = Rotl(((ecx_113 & edi_68) ^ var_18_3) + edx_92 + var_c - 0x7c1acfae, 5) ^ Neg(edi_68);
    uint32_t edx_94 = ecx_119 ^ edi_68;
    uint32_t eax_134 = Rotl((edx_94 ^ ebx_59) + w[1] + var_14 + 0x13ca3f4b, 0x17) - Bswap(Not(ecx_119));
    uint32_t ebx_60 = ebx_59 + (edx_94 ^ eax_134) + Bswap(Neg(ecx_119)) + w[1] + 0x59dd5c07;
    uint32_t edx_101 = (((ecx_119 ^ eax_134) & ebx_60) ^ eax_134) + w[2];
    var_10 = ebx_60;
    uint32_t edi_80 = Rotl(edx_101 + edi_68 - 0x731d0f65, 0x18) + Not(ebx_60);
    uint32_t ecx_120 = w[3];
    uint32_t ebx_63 = Rotl((((edi_80 ^ ebx_60) & eax_134) ^ ebx_60) + w[1] + ecx_119 + 0x2cd7097b, 0xb) + eax_134;
    uint32_t eax_137 = Rotl(ecx_120 - var_10 + ebx_63 + edi_80 + eax_134 + 0x6399f3c5, 3) ^ Not(edi_80);
    uint32_t ecx_126 = Rotl(ecx_120 - eax_137 + ebx_63 + edi_80 + var_10 + 0x295f3be8, 0x18) + ebx_63;
    uint32_t edx_119 = (ebx_63 ^ ecx_126 ^ eax_137) + w[3];
    var_10 = ecx_126;
    uint32_t edi_83 = Rotl(edx_119 + edi_80 - 0x2414bba9, 0xb) + Bswap(Not(ecx_126));
    var_14 = eax_137;
    uint32_t eax_138 = OddMix(edi_83, ecx_126, eax_137);
    uint32_t eax_143 = Rotl(eax_138 + w[0] + ebx_63 - 0x55b63ea1, 0xd) + Bswap(Not(edi_83));
    var_c = eax_143;
    var_10 = Arpl(var_10, var_14);
    uint32_t ecx_131 = var_10;
    uint32_t edx_123 = var_c;
    uint32_t edi_87 = w[2];
    uint32_t eax_152 = Rotl((((edi_83 ^ ecx_131) & edx_123) ^ ecx_131) + w[0] + var_14 + 0x1a89a2b, 8) - edx_123;
    var_14 = eax_152;
    uint32_t ecx_134 = Rotl(edi_87 - eax_152 + edx_123 + edi_83 + ecx_131 - 0x7ae59e2e, 0x1d) + Bswap(edi_83);
    uint32_t ebx_78 = (((ecx_134 ^ eax_152) & (Rotl((edx_123 ^ ecx_134 ^ eax_152) + edi_87 + edi_83 + 0x721ea18f, 0x11) + eax_152 + 1)) ^ (ecx_134 & eax_152)) + w[2];
    var_10 = ecx_134;
    var_c = Rotl(ebx_78 + edx_123 - 0xafb1865, 3) + ecx_134 + 1;
    PairMix(&var_c, &var_10, Rotl((edx_123 ^ ecx_134 ^ eax_152) + edi_87 + edi_83 + 0x721ea18f, 0x11) + eax_152 + 1, eax_152);
    uint32_t edx_127 = var_c;
    uint32_t edi_94 = var_10;
    uint32_t ecx_140 = ((edx_127 ^ (Rotl((edx_123 ^ ecx_134 ^ eax_152) + edi_87 + edi_83 + 0x721ea18f, 0x11) + eax_152 + 1)) & edi_94) ^ (Rotl((edx_123 ^ ecx_134 ^ eax_152) + edi_87 + edi_83 + 0x721ea18f, 0x11) + eax_152 + 1);
    uint32_t ecx_144 = Rotl(ecx_140 + w[2] + var_14 - 0x40361921, 0x1f) ^ Not(edi_94);
    uint32_t edx_128 = Not(edx_127);
    uint32_t eax_160 = Rotl(((Not(Rotl((edx_123 ^ ecx_134 ^ eax_152) + edi_87 + edi_83 + 0x721ea18f, 0x11) + eax_152 + 1) | edx_127) ^ ecx_144) + w[0] + edi_94 + 0x59874142, 0x1e) + edx_128;
    var_14 = ecx_144;
    var_10 = eax_160;
    uint32_t eax_161 = Arpl(Rotl((edx_123 ^ ecx_134 ^ eax_152) + edi_87 + edi_83 + 0x721ea18f, 0x11) + eax_152 + 1, var_14);
    uint32_t ecx_145 = var_10;
    uint32_t edx_130 = var_14;
    uint32_t edi_95 = w[1];
    uint32_t eax_169 = Rotl(((Not(var_c) | ecx_145) ^ edx_130) + edi_95 + eax_161 - 0x30e0442d, 0xa) + ecx_145;
    uint32_t edi_99 = Rotl(((Not(ecx_145) | edx_130) ^ eax_169) + edi_95 + var_c + 0x7ebffa1f, 0x13) + edx_130 + 1;
    uint32_t edx_133 = Rotl(w[1] - eax_169 + edi_99 + ecx_145 + edx_130 + 0x2211d49b, 0x1f) + ecx_145 + 1;
    var_c = edi_99;
    uint32_t edi_100 = Not(edi_99);
    var_14 = edx_133;
    uint32_t ebx_95 = w[1];
    uint32_t edx_138 = Rotl(((edi_100 | edx_133) ^ eax_169) + ebx_95 + ecx_145 - 0x668374a1, 9) ^ Bswap(Not(var_14));
    uint32_t edi_102 = (edi_100 | edx_138) ^ var_14;
    var_10 = edx_138;
    uint32_t edi_106 = Rotl(edi_102 + ebx_95 + eax_169 - 0x7dcb834a, 7) + Not(edx_138);
    uint32_t eax_172 = OddMix(edi_106, edx_138, var_14);
    uint32_t eax_175 = Rotl(eax_172 + w[3] + var_c - 0x55da2ae6, 9) - Bswap(Neg(edi_106));
    var_c = eax_175;
    var_10 = Arpl(var_10, var_14);
    uint32_t ecx_152 = var_c;
    uint32_t edx_141 = w[3];
    uint32_t ebx_96 = var_10;
    uint32_t eax_183 = var_14 + (((edi_106 ^ var_10) & ecx_152) ^ var_10) + edx_141 + ecx_152 + 0x78454c09;
    var_14 = eax_183;
    uint32_t eax_184 = Bswap(eax_183);
    uint32_t ecx_156 = Rotl((((ecx_152 ^ (Rotl(((Not(eax_183) | ecx_152) ^ edi_106) + edx_141 + ebx_96 - 0x4a7f5cbd, 0x1b) + ecx_152 + 1)) & eax_183) ^ ecx_152) + edx_141 + edi_106 + 0x21f775d5, 0xd) + eax_184;
    var_10 = Rotl(((Not(eax_183) | ecx_152) ^ edi_106) + edx_141 + ebx_96 - 0x4a7f5cbd, 0x1b) + ecx_152 + 1;
    var_10 = Arpl(var_10, var_c);
    uint32_t edx_143 = var_10;
    uint32_t edi_118 = var_14;
    uint32_t ecx_164 = Rotl(((Not(edx_143) | ecx_156) ^ edi_118) + w[1] + var_c + 0x5369bfaa, 0x11) - ecx_156;
    uint32_t edx_144 = edx_143 + w[0] - ecx_156 + ecx_164;
    uint32_t eax_193 = Rotl(edx_144 + edi_118 - 0x22429bbd, 5) + ecx_164;
    uint32_t edx_147 = Rotl(edx_144 + eax_193 - 0x686e08b7, 0x12) + ecx_164;
    uint32_t ebx_105 = (ecx_164 ^ edx_147 ^ eax_193) + w[3];
    var_10 = edx_147;
    uint32_t edi_123 = Rotl(ebx_105 + ecx_156 - 0x71826c0b, 0xa) ^ Not(eax_193);
    uint32_t edx_149 = w[3];
    uint32_t ecx_167 = Rotl((((edx_147 ^ eax_193) & edi_123) ^ (edx_147 & eax_193)) + edx_149 + ecx_164 + 0x7143e5c1, 0x12) + eax_193;
    uint32_t ebx_116 = (Not(ecx_167) | edi_123) ^ var_10;
    var_c = ecx_167;
    uint32_t eax_196 = Rotl(ebx_116 + w[0] + eax_193 - 0x7829338c, 0x1c) ^ Not(edi_123);
    uint32_t ebx_123 = Rotl(edx_149 - eax_196 + ecx_167 + edi_123 + var_10 + 0x6b608f68, 0x15) - edi_123;
    var_14 = eax_196;
    uint32_t eax_198 = OddMix(ebx_123, eax_196, ecx_167) + w[0];
    uint32_t ecx_169 = var_14;
    uint32_t edi_126 = Rotl(eax_198 + edi_123 + 0x25fe2302, 0xd) - Bswap(ebx_123);
    uint32_t edi_129 = Rotl(OddMix(ecx_169, ebx_123, edi_126) + w[1] + var_c + 0x56780ee8, 9) - Bswap(Neg(var_14));
    uint32_t eax_206 = OddMix(ebx_123, edi_129, edi_126);
    uint32_t ecx_172 = w[0];
    uint32_t eax_209 = var_14 + eax_206 + ecx_172 + ebx_123 + 0x4acfee5f;
    var_14 = eax_209;
    uint32_t ebx_126 = Rotl((((edi_129 ^ edi_126) & eax_209) ^ edi_126) + ecx_172 + ebx_123 - 0x449409a4, 0x1e) ^ Neg(eax_209);
    uint32_t ecx_181 = Rotl(w[1] - eax_209 + edi_129 + edi_126 + ebx_126 - 0x6ba04540, 0xe) + Not(edi_129);
    uint32_t edi_132 = Rotl((((ecx_181 ^ ebx_126) & eax_209) ^ ecx_181) + w[2] + edi_129 - 0x6b8eb45c, 0xf) - eax_209;
    uint32_t eax_210 = OddMix(ecx_181, ebx_126, edi_132);
    uint32_t ecx_182 = w[3];
    uint32_t edx_175 = w[0] + (Rotl(eax_210 + var_14 + ecx_182 - 0xed39b81, 0x18) ^ Not(ecx_181));
    w[1] += ebx_126;
    uint32_t eax_216 = w[2];
    w[0] = edx_175;
    uint32_t result = eax_216 + ecx_181;
    w[2] = result;
    w[3] = ecx_182 + edi_132;
    return result;
}

static uint32_t LoadLe32(unsigned char const* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

static void StoreLe32(unsigned char* p, uint32_t v)
{
    p[0] = static_cast<unsigned char>(v);
    p[1] = static_cast<unsigned char>(v >> 8);
    p[2] = static_cast<unsigned char>(v >> 16);
    p[3] = static_cast<unsigned char>(v >> 24);
}

void Transform(warden::Key16& seed)
{
    uint32_t w[4];
    for (unsigned i = 0; i != 4; ++i)
        w[i] = LoadLe32(seed.data() + i * 4);
    TransformWords(w);
    for (unsigned i = 0; i != 4; ++i)
        StoreLe32(seed.data() + i * 4, w[i]);
}
}

namespace warden
{
Key16 TransformX86ArchitectureSeed(Key16 const& seed)
{
    Key16 transformed = seed;
    Transform(transformed);
    return transformed;
}
}
