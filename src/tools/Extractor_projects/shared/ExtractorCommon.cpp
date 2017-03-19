/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2017  MaNGOS project <http://getmangos.eu>
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
FILE* openWoWExe()
{
    FILE *pFile;
    const char* ExeFileName[] = { "WoW.exe", "Wow.exe", "wow.exe" ,"World of Warcraft.exe", "World of Warcraft.app/Contents/MacOS/World of Warcraft"};
    int iExeSpelling = 5; ///> WoW.exe (Classic, CATA), Wow.exe (TBC, MoP, WoD), wow.exe (WOTLK) and a variant

    /// loop through all possible file names
    for (int iFileCount = 0; iFileCount < iExeSpelling; iFileCount++)
    {
#ifdef WIN32
        if (fopen_s(&pFile, ExeFileName[iFileCount], "rb") == 0)
            return pFile; ///< successfully located the WoW executable
#else
        if ((pFile = fopen(ExeFileName[iFileCount], "rb")))
            return pFile; ///< successfully located the WoW executable
#endif
    }

    return 0; ///< failed to locate WoW executable
}

/**
*  This function loads up a binary file (WoW executable), then searches for a specified 
*  group of hex values in order to locate and return the Build # of the file
* 
*  @PARAM sFilename is the filename of the WoW executable to be loaded
*  @RETURN iBuild the build number of the WoW executable, or 0 if failed
*/
int getBuildNumber()
{
    int iBuild = -1; ///< build version # of the WoW executable (returned value)

    bool bBuildFound = false;

    /// hex values of the text/bytes we need to search for:
    /// WoW [
    int iHexValue_W = 0x57;
    int iHexValue_o = 0x6F;
    int iHexValue_space = 0x20;
    int iHexValue_OpeningBracket = 0x5B; // [

    /// buffers used for working on the file's bytes
    unsigned char byteSearchBuffer[1]; ///< used for reading in a single character, ready to be
                                       ///< tested for the required text we are searching for: "WoW [Rel"
    unsigned char jumpBytesBuffer[128]; ///< used for skipping past the bytes from the file's start
                                        ///< to the base # area, before we start searching for the base #, for faster processing
    unsigned char jumpNonAssertionBytesBuffer[7]; ///< used for skipping past the bytes between the text being
                                        ///< searched for and the Base #, so that we can then get at the Base #
    unsigned char jumpAssertionBytesBuffer[26]; ///< skip the 'assertions enabled' part of the version string in an OS X based
                                        ///< version of the game.
    unsigned char buildNumber[6]; ///< stored here prior to conversion to an integer

    FILE *pFile;
    if (!(pFile = openWoWExe()))
        return 0; ///> failed to locate exe file

    /// jump over as much of the file as possible, before we start searching for the base #
    for (int i = 0; i < 3300; i++)
        fread(jumpBytesBuffer, sizeof(jumpBytesBuffer), 1, pFile);

    /// Search for the build #
    while (!bBuildFound && fread(byteSearchBuffer, 1, 1, pFile))
    {
        /// find W
        if (byteSearchBuffer[0] == 0x57)
        {
            /// is the next byte an o
            fread(byteSearchBuffer, 1, 1, pFile);
            if (byteSearchBuffer[0] == iHexValue_o)
            {
                /// is the next byte a W
                fread(byteSearchBuffer, 1, 1, pFile);
                if (byteSearchBuffer[0] == iHexValue_W)
                {
                    /// is the next byte a space
                    fread(byteSearchBuffer, 1, 1, pFile);
                    if (byteSearchBuffer[0] == iHexValue_space)
                    {
                        /// is the next byte an open square bracket
                        fread(byteSearchBuffer, 1, 1, pFile);
                        if (byteSearchBuffer[0] == iHexValue_OpeningBracket)
                        {
                            // Check for assertions
                            unsigned char releaseBuffer[7];
                            fread(releaseBuffer, sizeof(releaseBuffer), 1, pFile);

                            fread(byteSearchBuffer, 1, 1, pFile);
                            if (byteSearchBuffer[0] == iHexValue_space)
                            {
                                /*
                                 * Longer version name found. E.g.;
                                 * WoW [Release Assertions Enabled] Build 5875
                                 */
                                fread(jumpAssertionBytesBuffer, sizeof(jumpAssertionBytesBuffer), 1, pFile);
                            } else {
                                /**
                                 * Regular build version found. E.g.;
                                 * WoW [Release] Build 5875
                                 */
                                /// grab data leading up to the build #
                                fread(jumpNonAssertionBytesBuffer, sizeof(jumpNonAssertionBytesBuffer), 1, pFile);
                            }

                            bBuildFound = true;
                        }
                    }
                }
            }
        }
    }

    if (!bBuildFound)
    {
        /// close the file
        fclose(pFile); ///< housekeping
        return 0; ///< we reached the end of the file without locating the build #, exit funcion
    }

    /// grab the bytes containing the number
    fread(buildNumber, sizeof(buildNumber), 1, pFile);

    /// place the build number into a string (easy conversion to int)
    std::stringstream ss;
    std::string sbuildNumber;
    ss << buildNumber[0] << buildNumber[1] << buildNumber[2] << buildNumber[3] << buildNumber[4];
    ss >> sbuildNumber;

    fclose(pFile); ///< housekeping

    /// convert build number into an int
    iBuild = atoi(sbuildNumber.c_str());

    return iBuild; ///< build # found
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
    printf(
        "        __  __      _  _  ___  ___  ___      \n"
        "       |  \\/  |__ _| \\| |/ __|/ _ \\/ __|  \n"
        "       | |\\/| / _` | .` | (_ | (_) \\__ \\  \n"
        "       |_|  |_\\__,_|_|\\_|\\___|\\___/|___/ \n"
        "       %s for ", sTitle.c_str());

    switch (iCoreNumber)
    {
    case CLIENT_CLASSIC:
        printf("MaNGOSZero\n");
        break;
    case CLIENT_TBC:
        printf("MaNGOSOne\n");
        break;
    case CLIENT_WOTLK:
        printf("MaNGOSTwo\n");
        break;
    case CLIENT_CATA:
        printf("MaNGOSThree\n");
        break;
    case CLIENT_MOP:
        printf("MaNGOSFour\n");
        break;
    case CLIENT_WOD:
        printf("MaNGOSFive\n");
        break;
    case CLIENT_LEGION:
        printf("MaNGOSSix\n");
        break;
    default:
        printf("Unknown Version\n");
        break;
    }
    printf("  ________________________________________________\n");

}

/**
*  This function displays the standard mangos help banner to the console
*/
void showWebsiteBanner()
{
    printf(
        "  ________________________________________________\n\n"
        "    For help and support please visit:            \n"
        "    Website / Forum / Wiki: https://getmangos.eu  \n"
        "  ________________________________________________\n"
        );
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
        std::strcpy(magic,"z1.4");
        break;
    case CLIENT_TBC:
        std::strcpy(magic,"s1.4");
        break;
    case CLIENT_WOTLK:
        std::strcpy(magic,"v1.4");
        break;
    case CLIENT_CATA:
        std::strcpy(magic,"c1.4");
        break;
    case CLIENT_MOP:
        std::strcpy(magic,"p1.4");
        break;
    case CLIENT_WOD:
        std::strcpy(magic,"w1.4");
        break;
    case CLIENT_LEGION:
        std::strcpy(magic,"l1.4");
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
        std::strcpy(magic,"VMAPz06");
        break;
    case CLIENT_TBC:
        std::strcpy(magic,"VMAPs06");
        break;
    case CLIENT_WOTLK:
        std::strcpy(magic,"VMAPt06");
        break;
    case CLIENT_CATA:
        std::strcpy(magic,"VMAPc06");
        break;
    case CLIENT_MOP:
        std::strcpy(magic,"VMAPp06");
        break;
    case CLIENT_WOD:
        std::strcpy(magic,"VMAPw06");
        break;
    case CLIENT_LEGION:
        std::strcpy(magic,"VMAPl06");
        break;
    default:
        std::strcpy(magic,"VMAPUNK");
        break;
    }
}

/**
* @Create Folders based on the path provided
*
* @param sPath
*/
bool CreateDir(const std::string& sPath)
{
#ifdef WIN32
    if (_mkdir(sPath.c_str()) == 0)
        return 1;
#else
    if (mkdir(sPath.c_str(), 0777) == 0)
        return 1;
#endif
    return 0; // failed to create the directory
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

