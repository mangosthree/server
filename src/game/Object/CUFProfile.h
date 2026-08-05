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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

/**
 * @file CUFProfile.h
 * @brief CompactUnitFrame (raid frame) profiles, stored per character.
 *
 * The 4.3.4 client keeps its raid-frame layout server-side. On login it waits for
 * SMSG_LOAD_CUF_PROFILES before firing COMPACT_UNIT_FRAME_PROFILES_LOADED; until
 * that event arrives Blizzard_CompactUnitFrameProfiles.lua never activates a
 * profile, so CompactRaidFrameManager keeps `container.enabled` nil and the raid
 * frames stay hidden. Sending the packet -- even with no profiles -- lets the
 * client build its default one.
 */

#ifndef MANGOS_H_CUFPROFILE
#define MANGOS_H_CUFPROFILE

#include "Platform/Define.h"

#include <bitset>
#include <string>

/// Maximum number of CompactUnitFrame profiles the client accepts
#define MAX_CUF_PROFILES 5

/// Bit index of each boolean option inside CUFProfile::BoolOptions
enum CUFBoolOptions
{
    CUF_KEEP_GROUPS_TOGETHER,
    CUF_DISPLAY_PETS,
    CUF_DISPLAY_MAIN_TANK_AND_ASSIST,
    CUF_DISPLAY_HEAL_PREDICTION,
    CUF_DISPLAY_AGGRO_HIGHLIGHT,
    CUF_DISPLAY_ONLY_DISPELLABLE_DEBUFFS,
    CUF_DISPLAY_POWER_BAR,
    CUF_DISPLAY_BORDER,
    CUF_USE_CLASS_COLORS,
    CUF_DISPLAY_NON_BOSS_DEBUFFS,
    CUF_DISPLAY_HORIZONTAL_GROUPS,
    CUF_LOCKED,
    CUF_SHOWN,
    CUF_AUTO_ACTIVATE_2_PLAYERS,
    CUF_AUTO_ACTIVATE_3_PLAYERS,
    CUF_AUTO_ACTIVATE_5_PLAYERS,
    CUF_AUTO_ACTIVATE_10_PLAYERS,
    CUF_AUTO_ACTIVATE_15_PLAYERS,
    CUF_AUTO_ACTIVATE_25_PLAYERS,
    CUF_AUTO_ACTIVATE_40_PLAYERS,
    CUF_AUTO_ACTIVATE_SPEC_1,
    CUF_AUTO_ACTIVATE_SPEC_2,
    CUF_AUTO_ACTIVATE_PVP,
    CUF_AUTO_ACTIVATE_PVE,
    CUF_UNK_145,
    CUF_UNK_156,
    CUF_UNK_157,

    CUF_BOOL_OPTIONS_COUNT
};

/// One CompactUnitFrame layout as the client stores it
struct CUFProfile
{
    CUFProfile() :
        ProfileName(),
        FrameHeight(0),
        FrameWidth(0),
        SortBy(0),
        HealthText(0),
        TopPoint(0),
        BottomPoint(0),
        LeftPoint(0),
        TopOffset(0),
        BottomOffset(0),
        LeftOffset(0),
        BoolOptions()
    {
    }

    std::string ProfileName;
    uint16 FrameHeight;
    uint16 FrameWidth;
    uint8  SortBy;
    uint8  HealthText;

    uint8  TopPoint;                                        ///< anchor points
    uint8  BottomPoint;
    uint8  LeftPoint;

    uint16 TopOffset;                                       ///< anchor offsets
    uint16 BottomOffset;
    uint16 LeftOffset;

    std::bitset<CUF_BOOL_OPTIONS_COUNT> BoolOptions;
};

#endif
