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

#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <errno.h>

#if defined WIN32
#include <Windows.h>
#include <sys/stat.h>
#include <direct.h>
#define mkdir _mkdir
#else
#include <sys/stat.h>

#include <dirent.h>
/* This isn't the nicest way to do things
 * TODO: Fix this with snprintf instead and check it still works
 */
#define sprintf_s sprintf
#endif

#undef min
#undef max

//#pragma warning(disable : 4505)
//#pragma comment(lib, "Winmm.lib")

#include <map>

//From Extractor
#include "adtfile.h"
#include "wdtfile.h"
#include "dbcfile.h"
#include "wmo.h"
#include <mpq.h>
#include "vmapexport.h"
#include "Auth/md5.h"

#include "ExtractorCommon.h"

//------------------------------------------------------------------------------
// Defines

#define MPQ_BLOCK_SIZE 0x1000
//-----------------------------------------------------------------------------

bool AssembleVMAP(std::string src, std::string dest, const char* szMagic);
extern ArchiveSet gOpenArchives;

typedef struct
{
    char name[64];
    unsigned int id;
} map_id;

map_id* map_ids;
uint16* LiqType = 0;
uint32 map_count;
char output_path[128] = ".";
char input_path[1024] = ".";
bool hasInputPathParam = false;
bool preciseVectorData = true;
int iCoreNumber;
typedef std::pair < std::string /*full_filename*/, char const* /*locale_prefix*/ > UpdatesPair;
typedef std::map < int /*build*/, UpdatesPair > Updates;

// Constants
static const int LANG_COUNT = 12;

static const char* kClassicMPQList[] =
{
    "patch-2.MPQ",
    "patch.MPQ",
    "wmo.MPQ",
    "texture.MPQ",
    "terrain.MPQ",
    "speech.MPQ",
    "sound.MPQ",
    "model.MPQ",
    "misc.MPQ",
    "dbc.MPQ",
    "base.MPQ"
};

static const char* kTBCMPQList[] =
{
    "patch-2.MPQ",
    "patch.MPQ",
    "%s/patch-%s-2.MPQ",
    "%s/patch-%s.MPQ",
    "expansion.MPQ",
    "common.MPQ",
    "%s/locale-%s.MPQ",
    "%s/speech-%s.MPQ",
    "%s/expansion-locale-%s.MPQ",
    "%s/expansion-speech-%s.MPQ"
};

static const char* kWOTLKMPQList[] =
{
    "%s/patch-%s.MPQ",
    "patch.MPQ",
    "%s/patch-%s-2.MPQ",
    "%s/patch-%s-3.MPQ",
    "patch-2.MPQ",
    "patch-3.MPQ",
    "expansion.MPQ",
    "lichking.MPQ",
    "common.MPQ",
    "common-2.MPQ",
    "%s/locale-%s.MPQ",
    "%s/speech-%s.MPQ",
    "%s/expansion-locale-%s.MPQ",
    "%s/lichking-locale-%s.MPQ",
    "%s/expansion-speech-%s.MPQ",
    "%s/lichking-speech-%s.MPQ"
};

static const char* kCATAMPQList[] =
{
    "art.MPQ",
    "alternate.MPQ",
    "world1.MPQ",
    "world2.MPQ",
    "%s/locale-%s.MPQ",
    "%s/speech-%s.MPQ",
    "expansion1.MPQ",
    "%s/expansion1-locale-%s.MPQ",
    "%s/expansion1-speech-%s.MPQ",
    "expansion2.MPQ",
    "%s/expansion2-locale-%s.MPQ",
    "%s/expansion2-speech-%s.MPQ"
    "expansion3.MPQ",
    "%s/expansion3-locale-%s.MPQ",
    "%s/expansion3-speech-%s.MPQ"
};

//static const char * szWorkDirMaps = ".\\Maps";
char const szWorkDirWmo[]   = "./Buildings";
char       szRawVMAPMagic[] = "VMAP000";

// Local testing functions

bool FileExists(const char* file)
{
    if (FILE* n = std::fopen(file, "rb"))
    {
        fclose(n);
        return true;
    }
    return false;
}

void compute_md5(const char* value, char* result)
{
    md5_byte_t digest[16];
    md5_state_t ctx;

    mangos_md5_init(&ctx);
    md5_append(&ctx, (const unsigned char*)value, strlen(value));
    md5_finish(&ctx, digest);

    for(int i=0;i<16;i++)
    {
        sprintf(result+2*i,"%02x",digest[i]);
    }
    result[32]='\0';
}

std::string GetUniformName(std::string& path)
{
    std::transform(path.begin(),path.end(),path.begin(),::tolower);

    string tempPath;
    string file;
    char digest[33];

    std::size_t found = path.find_last_of("/\\");
    if (found != string::npos)
    {
      file = path.substr(found+1);
      tempPath = path.substr(0,found);
    }
    else
    {
        file = tempPath = path;
    }

    if(!tempPath.empty())
    {
        compute_md5(tempPath.c_str(),digest);
    }
    else
    {
        compute_md5("\\",digest);
    }

    string result;
    result = result.assign(digest) + "-" + file;

    return result;
}

std::string GetExtension(std::string& path)
{
    string ext;
    size_t foundExt = path.find_last_of(".");
    if (foundExt != std::string::npos)
    {
        ext=path.substr(foundExt+1);
    }
    else
    {
        ext.clear();
    }
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

/**
 * @brief
 *
 */
void ReadLiquidTypeTableDBC()
{
    printf("\n Reading liquid types from LiquidType.dbc...");

    HANDLE dbcFile;
    if (!OpenNewestFile("DBFilesClient\\LiquidType.dbc", &dbcFile))
    {
        printf("Error: Cannot find LiquidType.dbc in archive!\n");
        exit(1);
    }

    DBCFile dbc(dbcFile);
    if (!dbc.open())
    {
        printf("Fatal error: Could not read LiquidType.dbc!\n");
        exit(1);
    }

    size_t LiqType_count = dbc.getRecordCount();
    size_t LiqType_maxid = dbc.getMaxId();
    LiqType = new uint16[LiqType_maxid + 1];
    memset(LiqType, 0xff, (LiqType_maxid + 1) * sizeof(uint16));

    for (uint32 x = 0; x < LiqType_count; ++x)
    {
        LiqType[dbc.getRecord(x).getUInt(0)] = dbc.getRecord(x).getUInt(3);
    }

    printf(" Success! %zu liquid types loaded.\n", LiqType_count);
}

void ParseMapFiles(int iCoreNumber)
{
    char fn[512];
    //char id_filename[64];
    char id[10];
    StringSet failedPaths;
    printf("\n");
    for (unsigned int i = 0; i < map_count; ++i)
    {
        sprintf(id, "%03u", map_ids[i].id);
        sprintf(fn, "World\\Maps\\%s\\%s.wdt", map_ids[i].name, map_ids[i].name);

        HANDLE handleWDT;
        if (!OpenNewestFile(fn, &handleWDT))
        {
            printf("Error opening WDT file %s\n", fn);
            continue;
        }

        WDTFile WDT(handleWDT, fn, map_ids[i].name);
        if (WDT.init(id, map_ids[i].id))
        {
            printf(" Processing Map %u (%s)\n[", map_ids[i].id, map_ids[i].name);
            for (int x = 0; x < 64; ++x)
            {
                for (int y = 0; y < 64; ++y)
                {
                    if (ADTFile* ADT = WDT.GetMap(x, y))
                    {
                        //sprintf(id_filename,"%02u %02u %03u",x,y,map_ids[i].id);//!!!!!!!!!
                        ADT->init(map_ids[i].id, x, y, failedPaths, iCoreNumber, szRawVMAPMagic);
                        delete ADT;
                    }
                }
                printf("#");
                fflush(stdout);
            }
            printf("]\n");
        }
    }

    if (!failedPaths.empty())
    {
        printf(" Warning: Some models could not be extracted, see below\n");
        for (StringSet::const_iterator itr = failedPaths.begin(); itr != failedPaths.end(); ++itr)
        {
            printf("Could not find file of model %s\n", itr->c_str());
        }
        printf(" A few not found models can be expected and are not alarming.\n");
    }
}

void getGamePath()
{
    strcpy(input_path, "Data/");
}

bool scan_patches(char* scanmatch, std::vector<std::string>& pArchiveNames)
{
    int i;
    char path[512];

    for (i = 1; i <= 99; i++)
    {
        if (i != 1)
        {
            sprintf(path, "%s-%d.MPQ", scanmatch, i);
        }
        else
        {
            sprintf(path, "%s.MPQ", scanmatch);
        }
#ifdef __linux__
        if (FILE* h = fopen64(path, "rb"))
#else
        if (FILE* h = fopen(path, "rb"))
#endif
        {
            fclose(h);
            //matches.push_back(path);
            pArchiveNames.push_back(path);
        }
    }

    return(true);
}

void AppendPatchMPQFilesToList(char const* subdir, char const* suffix, char const* section, Updates& updates)
{
    char dirname[512];
    if (subdir)
    {
        sprintf(dirname, "%s/Data/%s", input_path, subdir);
    }
    else
    {
        sprintf(dirname, "%s/Data", input_path);
    }

    char scanname[512];
    if (suffix)
    {
        sprintf(scanname, "wow-update-%s-%%u.MPQ", suffix);
    }
    else
    {
        sprintf(scanname, "wow-update-%%u.MPQ");
    }

#ifdef WIN32

    char maskname[512];
    if (suffix)
    {
        sprintf(maskname, "%s/wow-update-%s-*.MPQ", dirname, suffix);
    }
    else
    {
        sprintf(maskname, "%s/wow-update-*.MPQ", dirname);
    }

    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(maskname, &ffd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                continue;
            }

            uint32 ubuild = 0;
            if (sscanf(ffd.cFileName, scanname, &ubuild) == 1 && (!iCoreNumber || ubuild <= iCoreNumber))
            {
                updates[ubuild] = UpdatesPair(ffd.cFileName, section);
            }
        } while (FindNextFile(hFind, &ffd) != 0);

        FindClose(hFind);
    }

#else

    if (DIR* dp = opendir(dirname))
    {
        int ubuild = 0;
        dirent* dirp;
        while ((dirp = readdir(dp)) != NULL)
            if (sscanf(dirp->d_name, scanname, &ubuild) == 1 && (!iCoreNumber || ubuild <= iCoreNumber))
            {
                updates[ubuild] = UpdatesPair(dirp->d_name, section);
            }

        closedir(dp);
    }

#endif
}

void LoadLocaleMPQFiles(int const locale)
{
    char filename[512];

    // first base old version of dbc files
    sprintf(filename, "%s/Data/%s/locale-%s.MPQ", input_path, Locales[locale], Locales[locale]);

    HANDLE localeMpqHandle;

    if (!OpenArchive(filename, &localeMpqHandle))
    {
        printf("Error open archive: %s\n\n", filename);
        return;
    }

    switch (iCoreNumber) {
        case CLIENT_TBC:
        case CLIENT_WOTLK:
            for (int i = 1; i < 5; ++i)
            {
                char ext[3] = "";
                if (i > 1)
                {
                    sprintf(ext, "-%i", i);
                }

                sprintf(filename, "%s/Data/%s/patch-%s%s.MPQ", input_path, Locales[locale], Locales[locale], ext);
                if (!OpenArchive(filename))
                {
                    printf("Error open patch archive: %s\n", filename);
                }
            }
            break;
        case CLIENT_CATA:
            // prepare sorted list patches in locale dir and Data root
            Updates updates;
            // now update to newer view, locale
            AppendPatchMPQFilesToList(Locales[locale], Locales[locale], NULL, updates);
            // now update to newer view, root
            AppendPatchMPQFilesToList(NULL, NULL, Locales[locale], updates);

            // ./Data wow-update-base files
            for (int i = 0; Builds[i] && Builds[i] <= CONF_TargetBuild; ++i)
            {
                sprintf(filename, "%s/Data/wow-update-base-%u.MPQ", input_path, Builds[i]);

                printf("\nPatching : %s\n", filename);

                //if (!OpenArchive(filename))
                if (!SFileOpenPatchArchive(localeMpqHandle, filename, "", 0))
                {
                    printf("Error open patch archive: %s\n\n", filename);
                }
            }

            for (Updates::const_iterator itr = updates.begin(); itr != updates.end(); ++itr)
            {
                if (!itr->second.second)
                {
                    sprintf(filename, "%s/Data/%s/%s", input_path, Locales[locale], itr->second.first.c_str());
                }
                else
                {
                    sprintf(filename, "%s/Data/%s", input_path, itr->second.first.c_str());
                }

                printf("\nPatching : %s\n", filename);

                //if (!OpenArchive(filename))
                if (!SFileOpenPatchArchive(localeMpqHandle, filename, itr->second.second ? itr->second.second : "", 0))
                {
                    printf("Error open patch archive: %s\n\n", filename);
                }
            }

            // ./Data/Cache patch-base files
            for (int i = 0; Builds[i] && Builds[i] <= CONF_TargetBuild; ++i)
            {
                sprintf(filename, "%s/Data/Cache/patch-base-%u.MPQ", input_path, Builds[i]);

                printf("\nPatching : %s\n", filename);

                //if (!OpenArchive(filename))
                if (!SFileOpenPatchArchive(localeMpqHandle, filename, "", 0))
                {
                    printf("Error open patch archive: %s\n\n", filename);
                }
            }

            // ./Data/Cache/<locale> patch files
            for (int i = 0; Builds[i] && Builds[i] <= CONF_TargetBuild; ++i)
            {
                sprintf(filename, "%s/Data/Cache/%s/patch-%s-%u.MPQ", input_path, Locales[locale], Locales[locale], Builds[i]);

                printf("\nPatching : %s\n", filename);

                //if (!OpenArchive(filename))
                if (!SFileOpenPatchArchive(localeMpqHandle, filename, "", 0))
                {
                    printf("Error open patch archive: %s\n\n", filename);
                }
            }
            break;
        }
}

/**
 * @brief
 *
 */
void LoadCommonMPQFiles(int client)
{
    char filename[512];
    char temp_file[512];
    int count = 0;
    string temp[256];
    switch (client)
    {
        case CLIENT_CLASSIC:
            count = sizeof(kClassicMPQList) / sizeof(char*);
            std::copy(std::begin(kClassicMPQList), std::end(kClassicMPQList), std::begin(temp));
            break;
        case CLIENT_TBC:
            count = sizeof(kTBCMPQList) / sizeof(char*);
            std::copy(std::begin(kTBCMPQList), std::end(kTBCMPQList), std::begin(temp));
            break;
        case CLIENT_WOTLK:
            count = sizeof(kWOTLKMPQList) / sizeof(char*);
            std::copy(std::begin(kWOTLKMPQList), std::end(kWOTLKMPQList), std::begin(temp));
            break;
        case CLIENT_CATA:
            count = sizeof(kCATAMPQList) / sizeof(char*);
            std::copy(std::begin(kCATAMPQList), std::end(kCATAMPQList), std::begin(temp));
            break;
    }

    char dirname[512];
    struct stat info;
    string locale;
    for (int i = 0; i < LOCALES_COUNT; i++)
    {
        sprintf_s(dirname, "%s/Data/%s", input_path, Locales[i]);
        if (!stat(dirname, &info))
        {
            locale = Locales[i];
            printf("Detected locale: %s\n", locale.c_str());
            break;
        }
    }

    for (int i = (count-1); i >= 0; i--)
    {
        // Replace possible locale info.
        sprintf_s(temp_file, temp[i].c_str(), locale.c_str(), locale.c_str());
        // Definitive filename.
        sprintf_s(filename, "%s/Data/%s", input_path, temp_file);
        printf("Loading archive %s\n", filename);
        if (ClientFileExists(filename))
        {
            HANDLE fileHandle;
            if (!OpenArchive(filename, &fileHandle))
            {
                printf("Error open archive: %s\n\n", filename);
            }
            //new MPQFile(fileHandle, filename);
        }
    }
}

void Usage(char* prg)
{
    printf(" Usage: %s [OPTION]\n\n", prg);
    printf(" Extract client database files and generate map files.\n");
    printf("   -h, --help            show the usage\n");
    printf("   -i, --input <path>     search path for game client archives\n");
    printf("   -s, --small           extract smaller vmaps by optimizing data. Reduces\n");
    printf("                         size by ~ 500MB\n");
    printf("\n");
    printf(" Example:\n");
    printf(" - use data path and create larger vmaps:\n");
    printf("   %s -l -i \"c:\\games\\world of warcraft\"\n", prg);
}

bool processArgv(int argc, char** argv)
{
    bool result = true;
    char* param = NULL;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 )
        {
            result = false;
            break;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--small") == 0 )
        {
            result = true;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0 )
        {
            param = argv[++i];
            if (!param)
            {
                result = false;
                break;
            }

            result = true;
            strcpy(input_path, param);
        }
        else
        {
            result = false;
            break;
        }
    }

    if (!result)
    {
        Usage(argv[0]);
    }
    return result;
}


//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// Main
//
// The program must be run with two command line arguments
//
// Arg1 - The source MPQ name (for testing reading and file find)
// Arg2 - Listfile name
//

int main(int argc, char** argv)
{
    // Use command line arguments, when some
    if (!processArgv(argc, argv))
    {
        return 1;
    }

    int thisBuild = getBuildNumber(input_path);
    iCoreNumber = getCoreNumberFromBuild(thisBuild);
    std::string outDir = std::string(output_path) + "/vmaps";

    showBanner("Vertical Map Asset Extractor", iCoreNumber);
    setVMapMagicVersion(iCoreNumber, szRawVMAPMagic);
    showWebsiteBanner();

    bool success = true;
    std::string sdir = std::string(szWorkDirWmo) + "/dir";
    std::string sdir_bin = std::string(szWorkDirWmo) + "/dir_bin";
    struct stat status;
    bool dirty = false;

    if (!stat(sdir.c_str(), &status) || !stat(sdir_bin.c_str(), &status))
    {
        printf(" Your %s directory seems to exist, please delete it!\n", szWorkDirWmo);
        dirty = true;
    }

    if (!stat(outDir.c_str(), &status))
    {
        printf(" Your %s directory seems to exist, please delete it!\n", outDir.c_str());
        dirty = true;
    }

    if (dirty)
    {
        printf(" <press return to exit>");
        char garbage[2];
        int ret = scanf("%c", garbage);
        return 1;
    }

    printf(" Beginning work ....\n");
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // Create the working and ouput directories
    CreateDir(std::string(szWorkDirWmo));
    CreateDir(outDir);

    // prepare archive name list
    LoadCommonMPQFiles(iCoreNumber);

    if (gOpenArchives.empty())
    {
        printf("FATAL ERROR: None MPQ archive found by path '%s'. Use -d option with proper path.\n", input_path);
        return 1;
    }
    if (iCoreNumber == CLIENT_CLASSIC)
    {
        ReadLiquidTypeTableDBC();
    }

    // extract data
    if (success)
    {
        success = ExtractWmo(iCoreNumber, szRawVMAPMagic);
    }

    // Open map.dbc
    if (success)
    {
        HANDLE dbcFile;
        if (!OpenNewestFile("DBFilesClient\\Map.dbc", &dbcFile))
        {
            printf("Error: Cannot find Map.dbc in archive!\n");
            exit(1);
        }

        printf("Found Map.dbc in archive!\n");
        printf("\n Reading maps from Map.dbc... ");

        DBCFile dbc(dbcFile);
        if (!dbc.open())
        {
            printf("Fatal error: Could not read Map.dbc!\n");
            exit(1);
        }

        map_count = dbc.getRecordCount();
        map_ids = new map_id[map_count];
        for (unsigned int x = 0; x < map_count; ++x)
        {
            map_ids[x].id = dbc.getRecord(x).getUInt(0);
            strcpy(map_ids[x].name, dbc.getRecord(x).getString(1));
            printf(" Map %d - %s\n", map_ids[x].id, map_ids[x].name);
        }


        ParseMapFiles(iCoreNumber);
        delete [] map_ids;
        //nError = ERROR_SUCCESS;
        // Extract models, listed in DameObjectDisplayInfo.dbc
        ExtractGameobjectModels(iCoreNumber, szRawVMAPMagic);
    }

    delete [] LiqType;

    if (!success)
    {
        printf("ERROR: Extract for %s. Work NOT complete.\n   Precise vector data=%d.\nPress any key.\n", szRawVMAPMagic, preciseVectorData);
        getchar();
        return 1;
    }

    success = AssembleVMAP(std::string(szWorkDirWmo), outDir, szRawVMAPMagic);

    if (!success)
    {
        printf("ERROR: VMAP building for %s NOT completed", szRawVMAPMagic);
        getchar();
        return 1;
    }

    printf("\n");
    printf(" VMAP building complete. No errors.\n");

    return 0;
}
