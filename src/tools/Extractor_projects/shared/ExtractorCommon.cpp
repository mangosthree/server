/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2021 MaNGOS <https://getmangos.eu>
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
#include <deque>
#include <set>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "ExtractorCommon.h"
#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <fcntl.h>

#ifndef WIN32
#include <unistd.h>
 /* This isn't the nicest way to do things..
 * TODO: Fix this with snprintf instead and check that it still works
 */
#define sprintf_s sprintf
#endif

#if defined( __GNUC__ )
#define _open   open
#define _close close
#ifndef O_BINARY
#define O_BINARY 0
#endif

#else
#include <io.h>
#endif

#ifdef O_LARGEFILE
#define OPEN_FLAGS  (O_RDONLY | O_BINARY | O_LARGEFILE)
#else
#define OPEN_FLAGS (O_RDONLY | O_BINARY)
#endif

/**
*  This function searches for and opens the WoW exe file, using all known variations on its spelling
*
*  @RETURN pFile the pointer to the file, so that it can be worked on
*/
FILE* openWoWExe(const char *path)
{
    FILE *pFile;
    std::vector<std::string> excec_names {
        "wow.exe",
        "Wow.exe",
        "WoW.exe",
        "wow.exe",
        "World of Warcraft.exe",
        "World of Warcraft.app/Contents/MacOS/World of Warcraft"
    };

    for (int i = 0; i < excec_names.size(); i++)
    {
        std::stringstream exec;
        exec << path << "/" << excec_names[i].c_str();
#ifdef WIN32
        if (fopen_s(&pFile, exec.str().c_str(), "rb") == 0)
        {
            std::cout << "Found exec: " << exec.str() << std::endl;
            return pFile;
        }
#else
        if ((pFile = fopen(exec.str().c_str(), "rb")))
        {
            std::cout << "Found exec: " << exec.str() << std::endl;
            return pFile;
        }
#endif
    }

    return 0; ///< failed to locate WoW executable
}

/**
*  This function loads up a binary file (WoW executable), then searches for and returns
*  the build number of the file. The build number is searched for in hex form.
*
*  @PARAM sFilename is the filename of the WoW executable to be loaded
*  @RETURN iBuild the build number of the WoW executable, or 0 if failed
*/
int getBuildNumber(const char *path)
{
    int iBuild = -1; ///< build version # of the WoW executable (returned value)

    /// buffers used for working on the file's bytes
    unsigned char byteSearchBuffer[1]; ///< used for reading in a single character, ready to be
                                       ///< tested for the required text we are searching for: 1, 5, 6, or 8
    unsigned char jumpBytesBuffer[128]; ///< used for skipping past the bytes from the file's start
                                        ///< to the base # area, before we start searching for the base #, for faster processing
    unsigned char preWOTLKbuildNumber[3]; ///< will hold the last 3 digits of the build number
    unsigned char postTBCbuildNumber[4]; ///< will hold the last 4 digits of the build number

    // These do not include the first digit
    // as the first digit is used to locate the possible location of the build number
    // then the following bytes are grabbed, 3 for pre WOTLK, 4 for WOTLK and later.
    // Those grabbed bytes are then compared with the below variables in order to idenity the exe's build
    unsigned char vanillaBuild1[3] = { 0x38, 0x37, 0x35 };       // (5)875
    unsigned char vanillaBuild2[3] = { 0x30, 0x30, 0x35 };       // (6)005
    unsigned char vanillaBuild3[3] = { 0x31, 0x34, 0x31 };       // (6)141
    unsigned char tbcBuild[3]      = { 0x36, 0x30, 0x36 };       // (8)606
    unsigned char wotlkBuild[4]    = { 0x32, 0x33, 0x34, 0x30 }; // (1)2340
    unsigned char cataBuild[4]     = { 0x35, 0x35, 0x39, 0x35 }; // (1)5595
    unsigned char mopBuild[4]      = { 0x38, 0x34, 0x31, 0x34 }; // (1)8414

    FILE *pFile;
    if (!(pFile = openWoWExe(path)))
    {
        std::cout << "Fatal Error: failed to locate the WoW executable!" << std::endl;
        std::cout << "Exiting program!!" << std::endl;
        exit(1); ///> failed to locate exe file
    }

    /// jump over as much of the file as possible, before we start searching for the base #
    for (int i = 0; i < 3300; i++)
    {
        fread(jumpBytesBuffer, sizeof(jumpBytesBuffer), 1, pFile);
    }

    /// Search for the build #
    while (fread(byteSearchBuffer, 1, 1, pFile))
    {
        /// we are looking for 1, 5, 6, or 8
        /// these values are the first digit of the build versions we are interested in

        // Vanilla and TBC
        if (byteSearchBuffer[0] == 0x35 || byteSearchBuffer[0] == 0x36 || byteSearchBuffer[0] == 0x38)
        {
            /// grab the next 3 bytes
            fread(preWOTLKbuildNumber, sizeof(preWOTLKbuildNumber), 1, pFile);

            if (!memcmp(preWOTLKbuildNumber, vanillaBuild1, sizeof(preWOTLKbuildNumber))) /// build is Vanilla?
            {
                return 5875;
            }
            else if (!memcmp(preWOTLKbuildNumber, vanillaBuild2, sizeof(preWOTLKbuildNumber))) /// build is Vanilla?
            {
                return 6005;
            }
            else if (!memcmp(preWOTLKbuildNumber, vanillaBuild3, sizeof(preWOTLKbuildNumber))) /// build is Vanilla?
            {
                return 6141;
            }
            else if (!memcmp(preWOTLKbuildNumber, tbcBuild, sizeof(preWOTLKbuildNumber))) /// build is TBC?
            {
                return 8606;
            }
        }

        /// WOTLK, CATA, MoP
        if (byteSearchBuffer[0] == 0x31)
        {
            /// grab the next 4 bytes
            fread(postTBCbuildNumber, sizeof(postTBCbuildNumber), 1, pFile);

            if (!memcmp(postTBCbuildNumber, wotlkBuild, sizeof(postTBCbuildNumber))) /// build is WOTLK?
            {
                return 12340;
            }
            else if (!memcmp(postTBCbuildNumber, cataBuild, sizeof(postTBCbuildNumber))) /// build is CATA?
            {
                return 15595;
            }
            else if (!memcmp(postTBCbuildNumber, mopBuild, sizeof(postTBCbuildNumber))) /// build is MoP?
            {
                return 18414;
            }
        }
    }

    std::cout << "Fatal Error: failed to identify build version!" << std::endl << std::endl;
    std::cout << "Supported build versions:" << std::endl;
    std::cout << "Vanilla: 5875, 6005, 6141" << std::endl;
    std::cout << "TBC:      8606" << std::endl;
    std::cout << "WOTLK:    12340" << std::endl;
    std::cout << "CATA:     15595" << std::endl;
    std::cout << "MOP:      18414"  << std::endl;
    std::cout << "Exiting program!!" << std::endl;
    exit(1);
}

/**
*  This function looks up the Core Version based in the found build Number
*
*  @RETURN iCoreNumber the build number of the WoW executable, or -1 if failed
*/
int getCoreNumber()
{
    return getCoreNumberFromBuild(getBuildNumber());
}

/**
*  This function looks up the Core Version based in the found build Number
*
*  @PARAM iBuildNumber is the build number of the WoW executable
*  @RETURN iCoreNumber the build number of the WoW executable, or -1 if failed
*/
int getCoreNumberFromBuild(int iBuildNumber)
{
    switch (iBuildNumber)
    {
    case 5875:  //CLASSIC
    case 6005:  //CLASSIC
    case 6141:  //CLASSIC
        return CLIENT_CLASSIC;
        break;
    case 8606:  //TBC
        return CLIENT_TBC;
        break;
    case 12340: //WOTLK
        return CLIENT_WOTLK;
        break;
    case 15595: //CATA
        return CLIENT_CATA;
        break;
    case 18414: //MOP
        return CLIENT_MOP;
        break;
    case 21355: //WOD
        return CLIENT_WOD;
        break;
    case 20740: //LEGION ALPHA
        return CLIENT_LEGION;
        break;

    default:
        return -1;
        break;
    }
}

/**
*  This function displays the standard mangos banner to the console
*
*  @PARAM sTitle is the Title text (directly under the MaNGOS logo)
*  @PARAM iCoreNumber is the Core Number
*/
void showBanner(const std::string& sTitle, int iCoreNumber)
{
    std::cout << \
        "        __  __      _  _  ___  ___  ___      " << std::endl << \
        "       |  \\/  |__ _| \\| |/ __|/ _ \\/ __|  " << std::endl << \
        "       | |\\/| / _` | .` | (_ | (_) \\__ \\  " << std::endl << \
        "       |_|  |_\\__,_|_|\\_|\\___|\\___/|___/ " << std::endl << \
        std::endl << \
        "       " << sTitle << " for ";

    switch (iCoreNumber)
    {
    case CLIENT_CLASSIC:
        std::cout << "MaNGOSZero" << std::endl;
        break;
    case CLIENT_TBC:
        std::cout << "MaNGOSOne" << std::endl;
        break;
    case CLIENT_WOTLK:
        std::cout << "MaNGOSTwo" << std::endl;
        break;
    case CLIENT_CATA:
        std::cout << "MaNGOSThree\n" << std::endl;
        break;
    case CLIENT_MOP:
        std::cout << "MaNGOSFour" << std::endl;
        break;
    case CLIENT_WOD:
        std::cout << "MaNGOSFive" << std::endl;
        break;
    case CLIENT_LEGION:
        std::cout << "MaNGOSSix" << std::endl;
        break;
    default:
        std::cout << "Unknown Version" << std::endl;
        break;
    }
    std::cout << "  ________________________________________________" << std::endl;

}

/**
*  This function displays the standard mangos help banner to the console
*/
void showWebsiteBanner()
{
    std::cout << \
        "  ________________________________________________" << std::endl << std::endl << \
        "    For help and support please visit:            " << std::endl << \
        "    Website / Forum / Wiki: https://getmangos.eu  " << std::endl << \
        "  ________________________________________________" << std::endl ;
}


/**
*  This function returns the .map file 'magic' number based on the core number
*
*  @PARAM iCoreNumber is the Core Number
*/
void setMapMagicVersion(int iCoreNumber, char* magic)
{
    switch (iCoreNumber)
    {
    case CLIENT_CLASSIC:
        std::strcpy(magic,"z1.5");
        break;
    case CLIENT_TBC:
        std::strcpy(magic,"s1.5");
        break;
    case CLIENT_WOTLK:
        std::strcpy(magic,"v1.5");
        break;
    case CLIENT_CATA:
        std::strcpy(magic,"c1.5");
        break;
    case CLIENT_MOP:
        std::strcpy(magic,"p1.5");
        break;
    case CLIENT_WOD:
        std::strcpy(magic,"w1.5");
        break;
    case CLIENT_LEGION:
        std::strcpy(magic,"l1.5");
        break;
    default:
        std::strcpy(magic,"UNKN");
        break;
    }
}

/**
*  This function returns the .vmap file 'magic' number based on the core number
*
*  @PARAM iCoreNumber is the Core Number
*/
void setVMapMagicVersion(int iCoreNumber, char* magic)
{
    switch (iCoreNumber)
    {
    case CLIENT_CLASSIC:
        std::strcpy(magic,"VMAPz07");
        break;
    case CLIENT_TBC:
        std::strcpy(magic,"VMAPs07");
        break;
    case CLIENT_WOTLK:
        std::strcpy(magic,"VMAPt07");
        break;
    case CLIENT_CATA:
        std::strcpy(magic,"VMAPc07");
        break;
    case CLIENT_MOP:
        std::strcpy(magic,"VMAPp07");
        break;
    case CLIENT_WOD:
        std::strcpy(magic,"VMAPw07");
        break;
    case CLIENT_LEGION:
        std::strcpy(magic,"VMAPl07");
        break;
    default:
        std::strcpy(magic,"VMAPUNK");
        break;
    }
}

/**
*  This function returns the .mmap file 'magic' number based on the core number
*
*  @PARAM iCoreNumber is the Core Number
*/
void setMMapMagicVersion(int iCoreNumber, char* magic)
{
    switch (iCoreNumber)
    {
    case CLIENT_CLASSIC:
        std::strcpy(magic, "z06");
        break;
    case CLIENT_TBC:
        std::strcpy(magic, "s06");
        break;
    case CLIENT_WOTLK:
        std::strcpy(magic, "t06");
        break;
    case CLIENT_CATA:
        std::strcpy(magic, "c06");
        break;
    case CLIENT_MOP:
        std::strcpy(magic, "p06");
        break;
    case CLIENT_WOD:
        std::strcpy(magic, "w06");
        break;
    case CLIENT_LEGION:
        std::strcpy(magic, "l06");
        break;
    default:
        std::strcpy(magic, "UNK");
        break;
    }
}

//#define MMAP_VERSION 4



/**
* @Create Folders based on the path provided
*
* @param sPath
*/
void CreateDir(const std::string& sPath)
{
#ifdef WIN32
    _mkdir(sPath.c_str());
#else
    mkdir(sPath.c_str(), 0777);
#endif

}

/**
* @Checks whether the Filename in the client exists
*
* @param sFileName
* @return bool
*/
bool ClientFileExists(const char* sFileName)
{
    int fp = _open(sFileName, OPEN_FLAGS);
    if (fp != -1)
    {
        _close(fp);
        return true;
    }

    return false;
}

/**************************************************************************/
bool isTransportMap(int mapID)
{
    switch (mapID)
    {
        // transport maps
    case 582:    // Transport: Rut'theran to Auberdine
    case 584:    // Transport: Menethil to Theramore
    case 586:    // Transport: Exodar to Auberdine
    case 587:    // Transport: Feathermoon Ferry - (TBC / WOTLK / CATA / MOP)
    case 588:    // Transport: Menethil to Auberdine - (TBC / WOTLK / CATA / MOP)
    case 589:    // Transport: Orgrimmar to Grom'Gol - (TBC / WOTLK / CATA / MOP)
    case 590:    // Transport: Grom'Gol to Undercity - (TBC / WOTLK / CATA / MOP)
    case 591:    // Transport: Undercity to Orgrimmar - (TBC / WOTLK / CATA / MOP)
    case 592:    // Transport: Borean Tundra Test - (WOTLK / CATA / MOP)
    case 593:    // Transport: Booty Bay to Ratchet - (TBC / WOTLK / CATA / MOP)
    case 594:    // Transport: Howling Fjord Sister Mercy (Quest) - (WOTLK / CATA / MOP)
    case 596:    // Transport: Naglfar - (WOTLK / CATA / MOP)
    case 610:    // Transport: Tirisfal to Vengeance Landing - (WOTLK / CATA / MOP)
    case 612:    // Transport: Menethil to Valgarde - (WOTLK / CATA / MOP)
    case 613:    // Transport: Orgrimmar to Warsong Hold - (WOTLK / CATA / MOP)
    case 614:    // Transport: Stormwind to Valiance Keep - (WOTLK / CATA / MOP)
    case 620:    // Transport: Moa'ki to Unu'pe - (WOTLK / CATA / MOP)
    case 621:    // Transport: Moa'ki to Kamagua - (WOTLK / CATA / MOP)
    case 622:    // Transport: Orgrim's Hammer - (WOTLK / CATA / MOP)
    case 623:    // Transport: The Skybreaker - (WOTLK / CATA / MOP)
    case 641:    // Transport: Alliance Airship BG - (WOTLK / CATA / MOP)
    case 642:    // Transport: HordeAirshipBG - (WOTLK / CATA / MOP)
    case 647:    // Transport: Orgrimmar to Thunder Bluff - (WOTLK / CATA / MOP)
    case 662:    // Transport: Alliance Vashj'ir Ship - (WOTLK / CATA / MOP)
    case 672:    // Transport: The Skybreaker (Icecrown Citadel Raid) - (WOTLK / CATA / MOP)
    case 673:    // Transport: Orgrim's Hammer (Icecrown Citadel Raid) - (WOTLK / CATA / MOP)
    case 674:    // Transport: Ship to Vashj'ir - (WOTLK / CATA / MOP)
    case 712:    // Transport: The Skybreaker (IC Dungeon) - (WOTLK / CATA / MOP)
    case 713:    // Transport: Orgrim's Hammer (IC Dungeon) - (WOTLK / CATA / MOP)
    case 718:    // Transport: The Mighty Wind (Icecrown Citadel Raid) - (WOTLK / CATA / MOP)
    case 738:    // Ship to Vashj'ir (Orgrimmar -> Vashj'ir) - (CATA / MOP)
    case 739:    // Vashj'ir Sub - Horde - (CATA / MOP)
    case 740:    // Vashj'ir Sub - Alliance - (CATA / MOP)
    case 741:    // Twilight Highlands Horde Transport - (CATA / MOP)
    case 742:    // Vashj'ir Sub - Horde - Circling Abyssal Maw - (CATA / MOP)
    case 743:    // Vashj'ir Sub - Alliance circling Abyssal Maw - (CATA / MOP)
    case 746:    // Uldum Phase Oasis - (CATA / MOP)
    case 747:    // Transport: Deepholm Gunship - (CATA / MOP)
    case 748:    // Transport: Onyxia/Nefarian Elevator - (CATA / MOP)
    case 749:    // Transport: Gilneas Moving Gunship - (CATA / MOP)
    case 750:    // Transport: Gilneas Static Gunship - (CATA / MOP)
    case 762:    // Twilight Highlands Zeppelin 1 - (CATA / MOP)
    case 763:    // Twilight Highlands Zeppelin 2 - (CATA / MOP)
    case 765:    // Krazzworks Attack Zeppelin - (CATA / MOP)
    case 766:    // Transport: Gilneas Moving Gunship 02 - (CATA / MOP)
    case 767:    // Transport: Gilneas Moving Gunship 03 - (CATA / MOP)
    case 1113:   // Transport: DarkmoonCarousel - (MOP)
    case 1132:   // Transport218599 - The Skybag (Brawl'gar Arena) - (MOP)
    case 1133:   // Transport218600 - Zandalari Ship (Mogu Island) - (MOP)
    case 1172:   // Transport: Siege of Orgrimmar (Alliance) - (MOP)
    case 1173:   // Transport: Siege of Orgrimmar (Horde) - (MOP)
    case 1192:   // Transport: Iron_Horde_Gorgrond_Train - (WOD)
    case 1231:   // Transport: Wavemurder Barge - (WOD)
        return true;
    default: // no transport maps
        return false;
    }
}

/**************************************************************************/
bool shouldSkipMap(int mapID,bool m_skipContinents, bool m_skipJunkMaps, bool m_skipBattlegrounds)
{
    if (m_skipContinents)
        switch (mapID)
        {
        case 0:      // Eastern Kingdoms - (CLASSIC / TBC / WOTLK / CATA / MOP)
        case 1:      // Kalimdor - (CLASSIC - TBC / WOTLK / CATA / MOP)
        case 530:    // Outland - (TBC / WOTLK / CATA / MOP)
        case 571:    // Northrend - (WOTLK / CATA / MOP)
        case 870:    // Pandaria - (MOP)
        case 1116:   // Draenor - (WOD)
            return true;
        default:
            break;
        }

    if (m_skipJunkMaps)
        switch (mapID)
        {
        case 13:    // test.wdt
        case 25:    // ScottTest.wdt
        case 29:    // Test.wdt
        case 35:    // StornWind Crypt (Unused Instance)
        case 37:    // Ashara.wdt (Unused Raid Area)
        case 42:    // Colin.wdt
        case 44:    // Monastry.wdt (Unused Old SM)
        case 169:   // EmeraldDream.wdt (unused, and very large)
        case 451:   // development.wdt
        case 573:   // ExteriorTest.wdt - (WOTLK / CATA / MOP)
        case 597:   // CraigTest.wdt - (WOTLK / CATA / MOP)
        case 605:   // development_nonweighted.wdt - (WOTLK / CATA / MOP)
        case 606:   // QA_DVD.wdt - (WOTLK / CATA / MOP)
        case 627:   // unused.wdt - (CATA / MOP)
        case 930:    // (UNUSED) Scenario: Alcaz Island - (MOP)
        case 995:    // The Depths [UNUSED] - (MOP)
        case 1010:  // MistsCTF3
        case 1014:  // (UNUSED) Peak of Serenity Scenario - (MOP)
        case 1028:  // (UNUSED) Scenario: Mogu Ruins - (MOP)
        case 1029:  // (UNUSED) Scenario: Mogu Crypt - (MOP)
        case 1049:  // (UNUSED) Scenario: Black Ox Temple - (MOP)
        case 1060:  // Level Design Land - Dev Only - (MOP)
        case 1181:  // PattyMack Test Garrison Bldg Map - (WOD)
        case 1250:  // Alliance - Garrison - Herb Garden 1 (UNUSED) - (WOD)
        case 1251:  // Alliance - Garrison - Inn 1 DONT USE - (WOD)
        case 1264:  // Propland - Dev Only - (WOD)
        case 1270:  // Development Land 3 - (WOD)
        case 1310:  // Expansion 5 QA Model Map - (WOD)
        case 1407:  // GorgrondFinaleScenarioMap(zzzOld) - (WOD)
        case 1427:  // PattyMack Test Garrison Bld Map2 - (WOD)

            return true;
        default:
            if (isTransportMap(mapID))
            {
                return true;
            }
            break;
        }

    if (m_skipBattlegrounds)
        switch (mapID)
        {
        case 30:    // AV
        case 37:    // AC
        case 489:   // WSG
        case 529:   // AB
        case 566:   // EotS - (TBC / WOTLK / CATA / MOP)
        case 607:   // SotA - (WOTLK / CATA / MOP)
        case 628:   // IoC - (WOTLK / CATA / MOP)
        case 726:   // TP - (CATA / MOP)
        case 727:   // SM - (CATA / MOP)
        case 728:   // BfG - (CATA / MOP)
        case 761:   // BfG2 - (CATA / MOP)
        case 968:   // EotS2 - (CATA / MOP)
        case 998:    // VOP - (MOP)
        case 1010:  // CTF3 - (MOP)
        case 1101:  // DOTA - (MOP)
        case 1105:  // GR - (MOP)
        case 1166:  // Small Battleground - (WOD)
        case 1431:  // SparringArenaLevel3Stadium  'The Coliseum' - (WOD)

            return true;
        default:
            break;
        }

    return false;
}

