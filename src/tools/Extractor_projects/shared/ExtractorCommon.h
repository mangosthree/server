/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2022 MaNGOS <https://getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "sl/loadlib.h"

FILE* openWoWExe();
int getBuildNumber();
int getCoreNumber();
int getCoreNumberFromBuild(int iBuildNumber);
void showBanner(const std::string& title, int iCoreNumber);
void showWebsiteBanner();
void setMapMagicVersion(int iCoreNumber, char* magic);
void setVMapMagicVersion(int iCoreNumber, char* magic);
bool CreateDir(const std::string& sPath);
bool ClientFileExists(const char* sFileName);
bool isTransportMap(int mapID);
bool shouldSkipMap(int mapID, bool m_skipContinents, bool m_skipJunkMaps, bool m_skipBattlegrounds);

uint32 const Builds[] = { 13164, 13205, 13287, 13329, 13596, 13623, 13914, 14007, 14333, 14480, 14545, 15005, 15050, 15211, 15354, 15595, 0 };
static uint32 CONF_TargetBuild = 15595;              // 4.3.4.15595

static char const* Locales[] = { "enGB", "enUS", "deDE", "esES", "frFR", "koKR", "zhCN", "zhTW", "enCN", "enTW", "esMX", "ruRU" };
#define LOCALES_COUNT 12

/// Enumerated Core Numbers
enum CoreNumber
{
    CLIENT_CLASSIC = 0,
    CLIENT_TBC = 1,
    CLIENT_WOTLK = 2,
    CLIENT_CATA = 3,
    CLIENT_MOP = 4,
    CLIENT_WOD = 5,
    CLIENT_LEGION = 6
};