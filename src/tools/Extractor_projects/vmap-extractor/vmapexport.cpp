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

#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include <errno.h>

#if defined WIN32
#include <Windows.h>
#include <sys/stat.h>
#include <direct.h>
#define mkdir _mkdir
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
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
#include "mpqfile.h"
#include "vmapexport.h"

#include "../shared/ExtractorCommon.h"

//------------------------------------------------------------------------------
// Defines

#define MPQ_BLOCK_SIZE 0x1000
//-----------------------------------------------------------------------------

HANDLE WorldMpq = NULL;
HANDLE LocaleMpq = NULL;

// List MPQ for extract maps from
char const* CONF_mpq_list[] =
{
    "world.MPQ",
    "art.MPQ",
    "expansion1.MPQ",
    "expansion2.MPQ",
    "expansion3.MPQ",
    "world2.MPQ",
};

#define LAST_DBC_IN_DATA_BUILD 13623    // after this build mpqs with dbc are back to locale folder

TCHAR const* LocalesT[] =
{
    _T("enGB"), _T("enUS"),
    _T("deDE"), _T("esES"),
    _T("frFR"), _T("koKR"),
    _T("zhCN"), _T("zhTW"),
    _T("enCN"), _T("enTW"),
    _T("esMX"), _T("ruRU"),
};

#define LOCALES_COUNT 12

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
bool preciseVectorData = true;
uint32 CONF_max_build = 0;

// Constants

//static const char * szWorkDirMaps = ".\\Maps";
const char* szWorkDirWmo = "./Buildings";
const char* szRawVMAPMagic = "VMAPc06";

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

typedef std::pair < std::string /*full_filename*/, char const* /*locale_prefix*/ > UpdatesPair;
typedef std::map < int /*build*/, UpdatesPair > Updates;

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
            if (sscanf(ffd.cFileName, scanname, &ubuild) == 1 && (!CONF_max_build || ubuild <= CONF_max_build))
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
            if (sscanf(dirp->d_name, scanname, &ubuild) == 1 && (!CONF_max_build || ubuild <= CONF_max_build))
            {
                updates[ubuild] = UpdatesPair(dirp->d_name, section);
            }

        closedir(dp);
    }

#endif
}

bool LoadLocaleMPQFile(int const locale)
{
    char filename[512];

    // first base old version of dbc files
    sprintf(filename, "%s/Data/%s/locale-%s.MPQ", input_path, Locales[locale], Locales[locale]);

    if (!OpenArchive(filename, &LocaleMpq))
    {
        printf("Error open archive: %s\n\n", filename);
        return false;
    }

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
        if (!SFileOpenPatchArchive(LocaleMpq, filename, "", 0))
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
        if (!SFileOpenPatchArchive(LocaleMpq, filename, itr->second.second ? itr->second.second : "", 0))
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
        if (!SFileOpenPatchArchive(LocaleMpq, filename, "", 0))
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
        if (!SFileOpenPatchArchive(LocaleMpq, filename, "", 0))
        {
            printf("Error open patch archive: %s\n\n", filename);
        }
    }

    return true;
}

/**
 * @brief
 *
 */
void LoadCommonMPQFiles(uint32 build)
{
    TCHAR filename[512];
    _stprintf(filename, _T("%s/Data/world.MPQ"), input_path);
    if (!SFileOpenArchive(filename, 0, MPQ_OPEN_READ_ONLY, &WorldMpq))
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            _tprintf(_T("Cannot open archive %s\n"), filename);
        }
        return;
    }

    int count = sizeof(CONF_mpq_list) / sizeof(char*);
    for (int i = 1; i < count; ++i)
    {
        if (build < 15211 && !strcmp("world2.MPQ", CONF_mpq_list[i]))   // 4.3.2 and higher MPQ
        {
            continue;
        }

        _stprintf(filename, _T("%s/Data/%s"), input_path, CONF_mpq_list[i]);
        if (!SFileOpenPatchArchive(WorldMpq, filename, "", 0))
        {
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
            {
                _tprintf(_T("Cannot open archive %s\n"), filename);
            }
            else
            {
                _tprintf(_T("Not found %s\n"), filename);
            }
        }
        else
        {
            _tprintf(_T("Loaded %s\n"), filename);

            bool found = false;
            int count = 0;
            SFILE_FIND_DATA data;
            HANDLE find = SFileFindFirstFile(WorldMpq, "*.*", &data, NULL);
            if (find != NULL)
            {
                do
                {
                    ++count;
                    if (data.dwFileFlags & MPQ_FILE_PATCH_FILE)
                    {
                        found = true;
                        break;
                    }
                }
                while (SFileFindNextFile(find, &data));
            }
            SFileFindClose(find);
            printf("Scanned %d files, found patch = %d\n", count, found);
        }
    }

    char const* prefix = NULL;
    for (int i = 0; Builds[i] && Builds[i] <= CONF_TargetBuild; ++i)
    {
        memset(filename, 0, sizeof(filename));
        if (Builds[i] > LAST_DBC_IN_DATA_BUILD)
        {
            prefix = "";
            _stprintf(filename, _T("%s/Data/wow-update-base-%u.MPQ"), input_path, Builds[i]);
        }
        else
        {
            prefix = "base";
            _stprintf(filename, _T("%s/Data/wow-update-%u.MPQ"), input_path, Builds[i]);
        }

        if (!SFileOpenPatchArchive(WorldMpq, filename, prefix, 0))
        {
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
            {
                _tprintf(_T("Cannot open patch archive %s\n"), filename);
            }
            else
            {
                _tprintf(_T("Not found %s\n"), filename);
            }
            continue;
        }
        else
        {
            _tprintf(_T("Loaded %s\n"), filename);


            bool found = false;
            int count = 0;
            SFILE_FIND_DATA data;
            HANDLE find = SFileFindFirstFile(WorldMpq, "*.*", &data, NULL);
            if (find != NULL)
            {
                do
                {
                    ++count;
                    if (data.dwFileFlags & MPQ_FILE_PATCH_FILE)
                    {
                        found = true;
                        break;
                    }
                }
                while (SFileFindNextFile(find, &data));
            }
            SFileFindClose(find);
            printf("Scanned %d files, found patch = %d\n", count, found);
        }
    }

    // ./Data/Cache patch-base files
    for (int i = 0; Builds[i] && Builds[i] <= CONF_TargetBuild; ++i)
    {
        sprintf(filename, "%s/Data/Cache/patch-base-%u.MPQ", input_path, Builds[i]);

        printf("\nPatching : %s\n", filename);

        if (!SFileOpenPatchArchive(WorldMpq, filename, "", 0))
        {
            printf("Error open patch archive: %s\n\n", filename);
        }
    }
}
void strToLower(char* str)
{
    while (*str)
    {
        *str = tolower(*str);
        ++str;
    }
}

// copied from contrib/extractor/System.cpp
void ReadLiquidTypeTableDBC()
{
    printf(" Reading liquid types from LiquidType.dbc...");
    DBCFile dbc(LocaleMpq, "DBFilesClient\\LiquidType.dbc");
    if (!dbc.open())
    {
        printf("Fatal error: Could not read LiquidType.dbc!\n");
        exit(1);
    }

    size_t LiqType_count = dbc.getRecordCount();
    size_t LiqType_maxid = dbc.getRecord(LiqType_count - 1).getUInt(0);
    LiqType = new uint16[LiqType_maxid + 1];
    memset(LiqType, 0xff, (LiqType_maxid + 1) * sizeof(uint16));

    for (uint32 x = 0; x < LiqType_count; ++x)
    {
        LiqType[dbc.getRecord(x).getUInt(0)] = dbc.getRecord(x).getUInt(3);
    }

    printf(" Success! (%u LiqTypes loaded)\n", (unsigned int)LiqType_count);
}

void Usage(char* prg)
{
    printf(" Usage: %s [OPTION]\n\n", prg);
    printf(" Extract client database files and generate map files.\n");
    printf("   -s : (default) small size (data size optimization), ~500MB less vmap data.\n");
    printf("   -l : large size, ~500MB more vmap data. (might contain more details)\n");
    printf("   -d <path>: Path to the vector data source folder.\n");
    printf("   -b : target build (default %u)", CONF_TargetBuild);
    printf("   -? : This message.\n");
}

void ParsMapFiles()
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
        WDTFile WDT(fn, map_ids[i].name);
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
                        ADT->init(map_ids[i].id, x, y, failedPaths);
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

bool ExtractWmo()
{
    bool success = false;

    //const char* ParsArchiveNames[] = {"patch-2.MPQ", "patch.MPQ", "common.MPQ", "expansion.MPQ"};

    SFILE_FIND_DATA data;
    HANDLE find = SFileFindFirstFile(WorldMpq, "*.wmo", &data, NULL);
    if (find != NULL)
    {
        do
        {
            std::string str = data.cFileName;
            //printf("Extracting wmo %s\n", str.c_str());
            success |= ExtractSingleWmo(str);
        }
        while (SFileFindNextFile(find, &data));
    }
    SFileFindClose(find);

    if (success)
    {
        printf("\nExtract wmo complete (No (fatal) errors)\n");
    }

    return success;
}

bool ExtractSingleWmo(std::string& fname)
{
    // Copy files from archive

    char szLocalFile[1024];
    const char* plain_name = GetPlainName(fname.c_str());
    sprintf(szLocalFile, "%s/%s", szWorkDirWmo, plain_name);
    fixnamen(szLocalFile, strlen(szLocalFile));

    if (FileExists(szLocalFile))
    {
        return true;
    }

    int p = 0;
    //Select root wmo files
    const char* rchr = strrchr(plain_name, '_');
    if (rchr != NULL)
    {
        char cpy[4];
        strncpy((char*)cpy, rchr, 4);
        for (int i = 0; i < 4; ++i)
        {
            int m = cpy[i];
            if (isdigit(m))
            {
                p++;
            }
        }
    }

    if (p == 3)
    {
        return true;
    }

    bool file_ok = true;
    std::cout << "Extracting " << fname << std::endl;
    WMORoot froot(fname);
    if (!froot.open())
    {
        printf("Couldn't open RootWmo!!!\n");
        return true;
    }
    FILE* output = fopen(szLocalFile, "wb");
    if (!output)
    {
        printf("couldn't open %s for writing!\n", szLocalFile);
        return false;
    }
    froot.ConvertToVMAPRootWmo(output);
    int Wmo_nVertices = 0;
    //printf("root has %d groups\n", froot->nGroups);
    if (froot.nGroups != 0)
    {
        for (uint32 i = 0; i < froot.nGroups; ++i)
        {
            char temp[1024];
            strcpy(temp, fname.c_str());
            temp[fname.length() - 4] = 0;
            char groupFileName[1024];
            sprintf(groupFileName, "%s_%03d.wmo", temp, i);
            //printf("Trying to open groupfile %s\n",groupFileName);

            string s = groupFileName;
            WMOGroup fgroup(s);
            if (!fgroup.open())
            {
                printf("Could not open all Group file for: %s\n", plain_name);
                file_ok = false;
                break;
            }

            Wmo_nVertices += fgroup.ConvertToVMAPGroupWmo(output, &froot, preciseVectorData);
        }
    }

    fseek(output, 8, SEEK_SET); // store the correct no of vertices
    fwrite(&Wmo_nVertices, sizeof(int), 1, output);
    fclose(output);

    // Delete the extracted file in the case of an error
    if (!file_ok)
    {
        remove(szLocalFile);
    }
    return true;
}

void getGamePath()
{
#ifdef _WIN32
    strcpy(input_path, "Data\\");
#else
    strcpy(input_path, "Data/");
#endif
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

bool processArgv(int argc, char** argv)
{
    bool result = true;
    bool hasInputPathParam = false;
    bool preciseVectorData = true;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp("-s", argv[i]) == 0)
        {
            preciseVectorData = false;
        }
        else if (strcmp("-d", argv[i]) == 0)
        {
            if ((i + 1) < argc)
            {
                hasInputPathParam = true;
                strcpy(input_path, argv[i + 1]);
                if (input_path[strlen(input_path) - 1] != '\\' || input_path[strlen(input_path) - 1] != '/')
                {
                    strcat(input_path, "/");
                }
                ++i;
            }
            else
            {
                result = false;
            }
        }
        else if (strcmp("-?", argv[1]) == 0)
        {
            result = false;
        }
        else if (strcmp("-l", argv[i]) == 0)
        {
            preciseVectorData = true;
        }
        else if (strcmp("-b", argv[i]) == 0)
        {
            if (i + 1 < argc)                            // all ok
            {
                CONF_TargetBuild = atoi(argv[i++ + 1]);
            }
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
    bool bCreatedVmapsFolder = false;
    bool bExtractedWMOfiles = false;
    std::string outDir = std::string(output_path) + "/vmaps";

    // Use command line arguments, when some
    if (!processArgv(argc, argv))
    {
        return 1;
    }

    // some simple check if working dir is dirty
    else
    {
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
    }

    printf(" Beginning work ....\n");
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // Create the working and ouput directories
    CreateDir(std::string(szWorkDirWmo));
    bCreatedVmapsFolder = CreateDir(outDir);

    printf("Loading common MPQ files\n");
    LoadCommonMPQFiles(CONF_TargetBuild);

    int FirstLocale = -1;

    for (int i = 0; i < LOCALES_COUNT; ++i)
    {
        //Open MPQs
        if (!LoadLocaleMPQFile(i))
        {
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
            {
                printf("Unable to load %s locale archives!\n", Locales[i]);
            }
            continue;
        }

        printf("Detected and using locale locale: %s\n", Locales[i]);
        break;
    }
    ReadLiquidTypeTableDBC();

    // extract data
    if (bCreatedVmapsFolder)
    {
        printf("Extracting WMO file\n");
        bExtractedWMOfiles = ExtractWmo();
    }

    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    //map.dbc
    if (bExtractedWMOfiles)
    {
        DBCFile* dbc = new DBCFile(LocaleMpq, "DBFilesClient\\Map.dbc");
        if (!dbc->open())
        {
            delete dbc;
            printf("FATAL ERROR: Map.dbc not found in data file.\n");
            return 1;
        }
        map_count = dbc->getRecordCount();
        map_ids = new map_id[map_count];
        for (unsigned int x = 0; x < map_count; ++x)
        {
            map_ids[x].id = dbc->getRecord(x).getUInt(0);
            strcpy(map_ids[x].name, dbc->getRecord(x).getString(1));
            printf(" Map %d - %s\n", map_ids[x].id, map_ids[x].name);
        }


        delete dbc;
        ParsMapFiles();
        delete [] map_ids;
        //nError = ERROR_SUCCESS;
        // Extract models, listed in DameObjectDisplayInfo.dbc
        ExtractGameobjectModels();
    }

    SFileCloseArchive(LocaleMpq);
    SFileCloseArchive(WorldMpq);

    printf("\n");
    if (!bExtractedWMOfiles)
    {
        printf("ERROR: Extract for %s. Work NOT complete.\n   Precise vector data=%d.\nPress any key.\n", szRawVMAPMagic, preciseVectorData);
        getchar();
        return 1;
    }

    printf("Extract for %s. Work complete. ", szRawVMAPMagic);
    if (!bCreatedVmapsFolder || !bExtractedWMOfiles)
    {
        printf("There were errors.\n");
    }
    else
    {
        printf("No errors.\n");
    }

    delete [] LiqType;
    return 0;
}
