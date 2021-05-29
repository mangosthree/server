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
#include <set>

#include "dbcfile.h"
#include <mpq.h>

#include <adt.h>
#include <wdt.h>
#include "ExtractorCommon.h"

#include <regex>

#ifndef WIN32
#include <unistd.h>
/* This isn't the nicest way to do things..
 * TODO: Fix this with snprintf instead and check that it still works
 */
#define sprintf_s sprintf
#endif

#ifdef WIN32
#include "direct.h"
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif
extern ArchiveSet gOpenArchives;    /**< TODO */

/**
 * @brief
 *
 */
typedef struct
{
    char name[64];                  /**< TODO */
    uint32 id;                      /**< TODO */
} map_id;

map_id* map_ids;                    /**< TODO */
uint16* areas;                      /**< TODO */
uint16* LiqType;                    /**< TODO */
//char output_path[128] = ".";        /**< TODO */
//char input_path[128] = ".";         /**< TODO */

std::string *input_path = new std::string(".");
std::string *output_path = new std::string(".");

uint32 maxAreaId = 0;               /**< TODO */
int iCoreNumber = 0;
/**
 * @brief Data types which can be extracted
 *
 */
enum Extract
{
    EXTRACT_MAP = 1,
    EXTRACT_DBC = 2
};

int   CONF_extract = EXTRACT_MAP | EXTRACT_DBC; /**< Select data for extract */
bool  CONF_allow_height_limit       = true;     /**< Allows to limit minimum height */
float CONF_use_minHeight            = -500.0f;  /**< Default minimum height */

bool  CONF_allow_float_to_int      = false;      /**< Allows float to int conversion */
float CONF_float_to_int8_limit     = 2.0f;      /**< Max accuracy = val/256 */
float CONF_float_to_int16_limit    = 2048.0f;   /**< Max accuracy = val/65536 */
float CONF_flat_height_delta_limit = 0.005f;    /**< If max - min less this value - surface is flat */
float CONF_flat_liquid_delta_limit = 0.001f;    /**< If max - min less this value - liquid surface is flat */

int MAP_LIQUID_TYPE_NO_WATER = 0x00;
int MAP_LIQUID_TYPE_MAGMA    = 0x01;
int MAP_LIQUID_TYPE_OCEAN    = 0x02;
int MAP_LIQUID_TYPE_SLIME    = 0x04;
int MAP_LIQUID_TYPE_WATER    = 0x08;

static const int LANG_COUNT = 12;

std::vector<std::string> kClassicMPQList 
{
    "base.MPQ",
    "dbc.MPQ",
    "misc.MPQ",
    "model.MPQ",
    "patch.MPQ",
    "patch-2.MPQ",
    "sound.MPQ",
    "speech.MPQ",
    "terrain.MPQ",
    "texture.MPQ",
    "wmo.MPQ",
};

std::vector<std::string> kTBCMPQList
{
    "common.MPQ",
    "expansion.MPQ",
    "patch.MPQ",
    "patch-2.MPQ",
    "%s/expansion-locale-%s.MPQ",
    "%s/expansion-speech-%s.MPQ",
    "%s/locale-%s.MPQ", 
    "%s/patch-%s-2.MPQ",
    "%s/patch-%s.MPQ",
    "%s/speech-%s.MPQ"
};

std::vector<std::string> kWOTLKMPQList 
{
    "common.MPQ",
    "common-2.MPQ",
    "expansion.MPQ",
    "lichking.MPQ",
    "patch.MPQ",
    "patch-2.MPQ",
    "patch-3.MPQ",
    "%s/expansion-locale-%s.MPQ",
    "%s/expansion-speech-%s.MPQ",
    "%s/lichking-locale-%s.MPQ",
    "%s/lichking-speech-%s.MPQ",
    "%s/locale-%s.MPQ",
    "%s/patch-%s.MPQ",
    "%s/patch-%s-2.MPQ",
    "%s/patch-%s-3.MPQ",
    "%s/speech-%s.MPQ"
};

std::vector<std::string> kCATAMPQList
{
    "alternate.MPQ",
    "art.MPQ",
    "expansion1.MPQ",
    "expansion2.MPQ",
    "expansion3.MPQ",
    "world.MPQ",
    "world2.MPQ",
    "%s/expansion1-locale-%s.MPQ",
    "%s/expansion1-speech-%s.MPQ",
    "%s/expansion2-locale-%s.MPQ",
    "%s/expansion2-speech-%s.MPQ",
    "%s/expansion3-locale-%s.MPQ",
    "%s/expansion3-speech-%s.MPQ",
    "%s/locale-%s.MPQ",
    "%s/speech-%s.MPQ"
};

/**
 * @brief
 *
 * @param program
 */
void Usage(char* program)
{
    std::cout << " Usage: " << program << std::endl;
    std::cout << " Extract client database files and generate map files." << std::endl;
    std::cout << "   -h, --help            show the usage" << std::endl;
    std::cout << "   -i, --input <path>    search path for game client archives" << std::endl;
    std::cout << "   -o, --output <path>   target path for generated files" << std::endl;
    std::cout << "   -f, --flat #          store height information as integers reducing map" << std::endl;
    std::cout << "                         size, but also accuracy" << std::endl;
    std::cout << "   -e, --extract #       extract specified client data. 1 = maps, 2 = DBCs," << std::endl;
    std::cout << "                         3 = both. Defaults to extracting both." << std::endl;
    std::cout << std::endl;
    std::cout << " Example:" << std::endl;
    std::cout << " - use input path and do not flatten maps:" << std::endl;
    std::cout << "   " << program << " -f 0 -i c:\\games\\world of warcraft\" -o  c:\\games\\world of warcraft\\extract" << std::endl;
}

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return bool
 */
bool HandleArgs(int argc, char **argv)
{

    try
    {
        for (int i = 1; i < argc; ++i)
        {
            std::string *arg = new std::string(argv[i]);
            if (arg->compare("-i") == 0 || arg->compare("--input") == 0)
            {
                i++;
                input_path = new std::string(argv[i]);
            }
            else if (arg->compare("-o") == 0 || arg->compare("--output") == 0)
            {
                i++;
                output_path = new std::string(argv[i]);
            }
            else if (arg->compare("-f") == 0 || arg->compare("--flat") == 0)
            {
                i++;
                if (std::atoi(argv[i]) != 0)
                {
                    CONF_allow_float_to_int = true;
                }
            }
            else if (arg->compare("-e") == 0 || arg->compare("--extract") == 0)
            {
                i++;
                CONF_extract = std::atoi(argv[i]);
                if (CONF_extract < 1 || CONF_extract > 3)
                {
                    throw std::invalid_argument("extract parameter must be between 1-3");
                }
            }
            else if (arg->compare("-h") == 0 || arg->compare("--help") == 0)
            {
                Usage(argv[0]);
                // this isn't a mistake
                exit(0);
            }
            delete (arg);
        }
        return true;
    }
    catch (...)
    {
        Usage(argv[0]);
        return false;
    }
}

void AppendDBCFileListTo(HANDLE mpqHandle, std::set<std::string>& filelist)
{
    SFILE_FIND_DATA findFileData;

    HANDLE searchHandle = SFileFindFirstFile(mpqHandle, "*.dbc", &findFileData, NULL);
    if (!searchHandle)
    {
        return;
    }

    filelist.insert(findFileData.cFileName);

    while (SFileFindNextFile(searchHandle, &findFileData))
        filelist.insert(findFileData.cFileName);

    SFileFindClose(searchHandle);
}

void AppendDB2FileListTo(HANDLE mpqHandle, std::set<std::string>& filelist)
{
    SFILE_FIND_DATA findFileData;

    HANDLE searchHandle = SFileFindFirstFile(mpqHandle, "*.db2", &findFileData, NULL);
    if (!searchHandle)
    {
        return;
    }

    filelist.insert(findFileData.cFileName);

    while (SFileFindNextFile(searchHandle, &findFileData))
        filelist.insert(findFileData.cFileName);

    SFileFindClose(searchHandle);
}

/**
 * @brief
 *
 * @return uint32
 */
uint32 ReadMapDBC()
{
    HANDLE dbcFile;
    if (!OpenNewestFile("DBFilesClient\\Map.dbc", &dbcFile))
    {
        std::cerr << "Error: Cannot find Map.dbc in archive!" << std::endl;
        exit(1);
    }

    std::cout << " Found Map.dbc in archive!" << std::endl;
    std::cout << std::endl<< " Reading maps from Map.dbc... " << std::endl;

    DBCFile dbc(dbcFile);
    if (!dbc.open())
    {
        std::cerr << "Fatal error: Could not read Map.dbc!" << std::endl;
        exit(1);
    }

    size_t map_count = dbc.getRecordCount();
    map_ids = new map_id[map_count];
    for (uint32 x = 0; x < map_count; ++x)
    {
        map_ids[x].id = dbc.getRecord(x).getUInt(0);
        strcpy(map_ids[x].name, dbc.getRecord(x).getString(1));
        std::cout << map_ids[x].name << std::endl;
    }

    return map_count;
}

/**
 * @brief
 *
 */
void ReadAreaTableDBC()
{
    std::cout << std::endl << " Read areas from AreaTable.dbc ...";

    HANDLE dbcFile;
    if (!OpenNewestFile("DBFilesClient\\AreaTable.dbc", &dbcFile))
    {
        std::cerr << "Error: Cannot find AreaTable.dbc in archive!" << std::endl;
        exit(1);
    }

    DBCFile dbc(dbcFile);

    if (!dbc.open())
    {
        std::cerr << "Fatal error: Could not read AreaTable.dbc!" << std::endl;
        exit(1);
    }

    size_t area_count = dbc.getRecordCount();
    size_t maxid = dbc.getMaxId();
    areas = new uint16[maxid + 1];
    memset(areas, 0xff, (maxid + 1) * sizeof(uint16));

    for (uint32 x = 0; x < area_count; ++x)
    {
        areas[dbc.getRecord(x).getUInt(0)] = dbc.getRecord(x).getUInt(3);
    }

    maxAreaId = dbc.getMaxId();

    std::cout << " Success! " << area_count << " areas loaded." << std::endl;
}

/**
 * @brief
 *
 */
void ReadLiquidTypeTableDBC()
{
    std::cout << std::endl << " Reading liquid types from LiquidType.dbc..." << std::endl;

    HANDLE dbcFile;
    if (!OpenNewestFile("DBFilesClient\\LiquidType.dbc", &dbcFile))
    {
        std::cout << "Error: Cannot find LiquidType.dbc in archive!" << std::endl;
        exit(1);
    }

    DBCFile dbc(dbcFile);
    if (!dbc.open())
    {
        std::cout << "Fatal error: Could not read LiquidType.dbc!" << std:: endl;
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

    std::cout << " Success! " << LiqType_count << " liquid types loaded." << std::endl;
}

//
// Adt file convertor function and data
//

// Map file format data
static char const MAP_MAGIC[]           = "MAPS"; /**< TODO */
static char       MAP_VERSION_MAGIC[32] = "0000"; /**< TODO */
static char const MAP_AREA_MAGIC[]      = "AREA"; /**< TODO */
static char const MAP_HEIGHT_MAGIC[]    = "MHGT"; /**< TODO */
static char const MAP_LIQUID_MAGIC[]    = "MLIQ"; /**< TODO */

/**
 * @brief
 *
 */
struct map_fileheader
{
    uint32 mapMagic;        /**< TODO */
    uint32 versionMagic;    /**< TODO */
    uint32 buildMagic;
    uint32 areaMapOffset;   /**< TODO */
    uint32 areaMapSize;     /**< TODO */
    uint32 heightMapOffset; /**< TODO */
    uint32 heightMapSize;   /**< TODO */
    uint32 liquidMapOffset; /**< TODO */
    uint32 liquidMapSize;   /**< TODO */
    uint32 holesOffset;     /**< TODO */
    uint32 holesSize;       /**< TODO */
};

#define MAP_AREA_NO_AREA      0x0001

/**
 * @brief
 *
 */
struct map_areaHeader
{
    uint32 fourcc;          /**< TODO */
    uint16 flags;           /**< TODO */
    uint16 gridArea;        /**< TODO */
};

#define MAP_HEIGHT_NO_HEIGHT  0x0001
#define MAP_HEIGHT_AS_INT16   0x0002
#define MAP_HEIGHT_AS_INT8    0x0004

/**
 * @brief
 *
 */
struct map_heightHeader
{
    uint32 fourcc;          /**< TODO */
    uint32 flags;           /**< TODO */
    float  gridHeight;      /**< TODO */
    float  gridMaxHeight;   /**< TODO */
};



#define MAP_LIQUID_TYPE_DARK_WATER  0x10
#define MAP_LIQUID_TYPE_WMO_WATER   0x20


#define MAP_LIQUID_NO_TYPE    0x0001
#define MAP_LIQUID_NO_HEIGHT  0x0002

/**
 * @brief
 *
 */
struct map_liquidHeader
{
    uint32 fourcc;          /**< TODO */
    uint16 flags;           /**< TODO */
    uint16 liquidType;      /**< TODO */
    uint8  offsetX;         /**< TODO */
    uint8  offsetY;         /**< TODO */
    uint8  width;           /**< TODO */
    uint8  height;          /**< TODO */
    float  liquidLevel;     /**< TODO */
};

/**
 * @brief
 *
 * @param maxDiff
 * @return float
 */
float selectUInt8StepStore(float maxDiff)
{
    return 255 / maxDiff;
}

/**
 * @brief
 *
 * @param maxDiff
 * @return float
 */
float selectUInt16StepStore(float maxDiff)
{
    return 65535 / maxDiff;
}

uint16 area_flags[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];      /**< Temporary grid data store */

float V8[ADT_GRID_SIZE][ADT_GRID_SIZE];                         /**< TODO */
float V9[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];                 /**< TODO */
uint16 uint16_V8[ADT_GRID_SIZE][ADT_GRID_SIZE];                 /**< TODO */
uint16 uint16_V9[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];         /**< TODO */
uint8  uint8_V8[ADT_GRID_SIZE][ADT_GRID_SIZE];                  /**< TODO */
uint8  uint8_V9[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];          /**< TODO */

uint16 liquid_entry[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];    /**< TODO */
uint8 liquid_flags[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];     /**< TODO */
bool  liquid_show[ADT_GRID_SIZE][ADT_GRID_SIZE];                /**< TODO */
float liquid_height[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];      /**< TODO */

/**
 * @brief
 *
 * @param filename
 * @param filename2
 * @param build
 * @return bool
 */
bool ConvertADT(char* filename, char* filename2, uint32 build)
{
    ADT_file adt;

    if (!adt.loadFile(filename))
    {
        return false;
    }

    adt_MCIN* cells = adt.a_grid->getMCIN();
    if (!cells)
    {
        //std::cout << "Can not find cells in '" << filename << "'." << std::endl;
        return false;
    }

    memset(liquid_show, 0, sizeof(liquid_show));
    memset(liquid_flags, 0, sizeof(liquid_flags));
    memset(liquid_entry, 0, sizeof(liquid_entry));

    // Prepare map header
    map_fileheader map;
    map.mapMagic = *(uint32 const*)MAP_MAGIC;
    map.versionMagic = *(uint32 const*)MAP_VERSION_MAGIC;
    map.buildMagic = build;

    // Get area flags data
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
        {
            adt_MCNK* cell = cells->getMCNK(i, j);
            uint32 areaid = cell->areaid;
            if (areaid && areaid <= maxAreaId)
            {
                if (areas[areaid] != 0xffff)
                {
                    area_flags[i][j] = areas[areaid];
                    continue;
                }
                std::cout << "File: " << filename << std::endl;
                std::cout << "Can not find area flag for area " << areaid << " [" << cell->ix << "," << cell->iy << "]" << std::endl;
            }
            area_flags[i][j] = 0xffff;
        }
    }
    //============================================
    // Try pack area data
    //============================================
    bool fullAreaData = false;
    uint32 areaflag = area_flags[0][0];
    for (int y = 0; y < ADT_CELLS_PER_GRID; y++)
    {
        for (int x = 0; x < ADT_CELLS_PER_GRID; x++)
        {
            if (area_flags[y][x] != areaflag)
            {
                fullAreaData = true;
                break;
            }
        }
    }

    map.areaMapOffset = sizeof(map);
    map.areaMapSize   = sizeof(map_areaHeader);

    map_areaHeader areaHeader;
    areaHeader.fourcc = *(uint32 const*)MAP_AREA_MAGIC;
    areaHeader.flags = 0;
    if (fullAreaData)
    {
        areaHeader.gridArea = 0;
        map.areaMapSize += sizeof(area_flags);
    }
    else
    {
        areaHeader.flags |= MAP_AREA_NO_AREA;
        areaHeader.gridArea = (uint16)areaflag;
    }

    //
    // Get Height map from grid
    //
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
        {
            adt_MCNK* cell = cells->getMCNK(i, j);
            if (!cell)
            {
                continue;
            }
            // Height values for triangles stored in order:
            // 1     2     3     4     5     6     7     8     9
            //    10    11    12    13    14    15    16    17
            // 18    19    20    21    22    23    24    25    26
            //    27    28    29    30    31    32    33    34
            // . . . . . . . .
            // For better get height values merge it to V9 and V8 map
            // V9 height map:
            // 1     2     3     4     5     6     7     8     9
            // 18    19    20    21    22    23    24    25    26
            // . . . . . . . .
            // V8 height map:
            //    10    11    12    13    14    15    16    17
            //    27    28    29    30    31    32    33    34
            // . . . . . . . .

            // Set map height as grid height
            for (int y = 0; y <= ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x <= ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V9[cy][cx] = cell->ypos;
                }
            }
            for (int y = 0; y < ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x < ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V8[cy][cx] = cell->ypos;
                }
            }
            // Get custom height
            adt_MCVT* v = cell->getMCVT();
            if (!v)
            {
                continue;
            }
            // get V9 height map
            for (int y = 0; y <= ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x <= ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V9[cy][cx] += v->height_map[y * (ADT_CELL_SIZE * 2 + 1) + x];
                }
            }
            // get V8 height map
            for (int y = 0; y < ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x < ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V8[cy][cx] += v->height_map[y * (ADT_CELL_SIZE * 2 + 1) + ADT_CELL_SIZE + 1 + x];
                }
            }
        }
    }
    //============================================
    // Try pack height data
    //============================================
    float maxHeight = -20000;
    float minHeight =  20000;
    for (int y = 0; y < ADT_GRID_SIZE; y++)
    {
        for (int x = 0; x < ADT_GRID_SIZE; x++)
        {
            float h = V8[y][x];
            if (maxHeight < h)
            {
                maxHeight = h;
            }
            if (minHeight > h)
            {
                minHeight = h;
            }
        }
    }
    for (int y = 0; y <= ADT_GRID_SIZE; y++)
    {
        for (int x = 0; x <= ADT_GRID_SIZE; x++)
        {
            float h = V9[y][x];
            if (maxHeight < h)
            {
                maxHeight = h;
            }
            if (minHeight > h)
            {
                minHeight = h;
            }
        }
    }

    // Check for allow limit minimum height (not store height in deep ochean - allow save some memory)
    if (CONF_allow_height_limit && minHeight < CONF_use_minHeight)
    {
        for (int y = 0; y < ADT_GRID_SIZE; y++)
            for (int x = 0; x < ADT_GRID_SIZE; x++)
                if (V8[y][x] < CONF_use_minHeight)
                {
                    V8[y][x] = CONF_use_minHeight;
                }
        for (int y = 0; y <= ADT_GRID_SIZE; y++)
            for (int x = 0; x <= ADT_GRID_SIZE; x++)
                if (V9[y][x] < CONF_use_minHeight)
                {
                    V9[y][x] = CONF_use_minHeight;
                }
        if (minHeight < CONF_use_minHeight)
        {
            minHeight = CONF_use_minHeight;
        }
        if (maxHeight < CONF_use_minHeight)
        {
            maxHeight = CONF_use_minHeight;
        }
    }

    map.heightMapOffset = map.areaMapOffset + map.areaMapSize;
    map.heightMapSize = sizeof(map_heightHeader);

    map_heightHeader heightHeader;
    heightHeader.fourcc = *(uint32 const*)MAP_HEIGHT_MAGIC;
    heightHeader.flags = 0;
    heightHeader.gridHeight    = minHeight;
    heightHeader.gridMaxHeight = maxHeight;

    if (maxHeight == minHeight)
    {
        heightHeader.flags |= MAP_HEIGHT_NO_HEIGHT;
    }

    // Not need store if flat surface
    if (CONF_allow_float_to_int && (maxHeight - minHeight) < CONF_flat_height_delta_limit)
    {
        heightHeader.flags |= MAP_HEIGHT_NO_HEIGHT;
    }

    // Try store as packed in uint16 or uint8 values
    if (!(heightHeader.flags & MAP_HEIGHT_NO_HEIGHT))
    {
        float step = 0.0f;
        // Try Store as uint values
        if (CONF_allow_float_to_int)
        {
            float diff = maxHeight - minHeight;
            if (diff < CONF_float_to_int8_limit)      // As uint8 (max accuracy = CONF_float_to_int8_limit/256)
            {
                heightHeader.flags |= MAP_HEIGHT_AS_INT8;
                step = selectUInt8StepStore(diff);
            }
            else if (diff < CONF_float_to_int16_limit) // As uint16 (max accuracy = CONF_float_to_int16_limit/65536)
            {
                heightHeader.flags |= MAP_HEIGHT_AS_INT16;
                step = selectUInt16StepStore(diff);
            }
        }

        // Pack it to int values if need
        if (heightHeader.flags & MAP_HEIGHT_AS_INT8)
        {
            for (int y = 0; y < ADT_GRID_SIZE; y++)
                for (int x = 0; x < ADT_GRID_SIZE; x++)
                {
                    uint8_V8[y][x] = uint8((V8[y][x] - minHeight) * step + 0.5f);
                }
            for (int y = 0; y <= ADT_GRID_SIZE; y++)
                for (int x = 0; x <= ADT_GRID_SIZE; x++)
                {
                    uint8_V9[y][x] = uint8((V9[y][x] - minHeight) * step + 0.5f);
                }
            map.heightMapSize += sizeof(uint8_V9) + sizeof(uint8_V8);
        }
        else if (heightHeader.flags & MAP_HEIGHT_AS_INT16)
        {
            for (int y = 0; y < ADT_GRID_SIZE; y++)
                for (int x = 0; x < ADT_GRID_SIZE; x++)
                {
                    uint16_V8[y][x] = uint16((V8[y][x] - minHeight) * step + 0.5f);
                }
            for (int y = 0; y <= ADT_GRID_SIZE; y++)
                for (int x = 0; x <= ADT_GRID_SIZE; x++)
                {
                    uint16_V9[y][x] = uint16((V9[y][x] - minHeight) * step + 0.5f);
                }
            map.heightMapSize += sizeof(uint16_V9) + sizeof(uint16_V8);
        }
        else
        {
            map.heightMapSize += sizeof(V9) + sizeof(V8);
        }
    }

    // Get from MCLQ chunk (old)
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
        {
            adt_MCNK* cell = cells->getMCNK(i, j);
            if (!cell)
            {
                continue;
            }

            adt_MCLQ* liquid = cell->getMCLQ();
            int count = 0;
            if (!liquid || cell->sizeMCLQ <= 8)
            {
                continue;
            }

            for (int y = 0; y < ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x < ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    if (liquid->flags[y][x] != 0x0F)
                    {
                        liquid_show[cy][cx] = true;
                        if (liquid->flags[y][x] & (1 << 7))
                        {
                            liquid_flags[i][j] |= MAP_LIQUID_TYPE_DARK_WATER;
                        }
                        ++count;
                    }
                }
            }

            uint32 c_flag = cell->flags;
            if (c_flag & (1 << 2))
            {
                liquid_entry[i][j] = 1;
                liquid_flags[i][j] |= MAP_LIQUID_TYPE_WATER;            // water
            }
            if (c_flag & (1 << 3))
            {
                liquid_entry[i][j] = 2;
                liquid_flags[i][j] |= MAP_LIQUID_TYPE_OCEAN;            // ocean
            }
            if (c_flag & (1 << 4))
            {
                liquid_entry[i][j] = 3;
                liquid_flags[i][j] |= MAP_LIQUID_TYPE_MAGMA;            // magma/slime
            }

            if (!count && liquid_flags[i][j])
            {
                fprintf(stderr, "Wrong liquid type detected in MCLQ chunk");
            }

            for (int y = 0; y <= ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x <= ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    liquid_height[cy][cx] = liquid->liquid[y][x].height;
                }
            }
        }
    }

    // Get liquid map for grid (in WOTLK used MH2O chunk)
    adt_MH2O* h2o = adt.a_grid->getMH2O();
    if (h2o)
    {
        for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
        {
            for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
            {
                adt_liquid_header* h = h2o->getLiquidData(i, j);
                if (!h)
                {
                    continue;
                }

                int count = 0;
                uint64 show = h2o->getLiquidShowMap(h);
                for (int y = 0; y < h->height; y++)
                {
                    int cy = i * ADT_CELL_SIZE + y + h->yOffset;
                    for (int x = 0; x < h->width; x++)
                    {
                        int cx = j * ADT_CELL_SIZE + x + h->xOffset;
                        if (show & 1)
                        {
                            liquid_show[cy][cx] = true;
                            ++count;
                        }
                        show >>= 1;
                    }
                }

                liquid_entry[i][j] = h->liquidType;
                switch (LiqType[h->liquidType])
                {
                    case LIQUID_TYPE_WATER: liquid_flags[i][j] |= MAP_LIQUID_TYPE_WATER; break;
                    case LIQUID_TYPE_OCEAN: liquid_flags[i][j] |= MAP_LIQUID_TYPE_OCEAN; break;
                    case LIQUID_TYPE_MAGMA: liquid_flags[i][j] |= MAP_LIQUID_TYPE_MAGMA; break;
                    case LIQUID_TYPE_SLIME: liquid_flags[i][j] |= MAP_LIQUID_TYPE_SLIME; break;
                    default:
                        std::cout << std::endl << "Can not find liquid type " << h->liquidType << \
                            " for map " << filename << std::endl << \
                            " chunk " << i << ", " << j << std::endl;
                        break;
                }
                // Dark water detect
                if (LiqType[h->liquidType] == LIQUID_TYPE_OCEAN)
                {
                    uint8* lm = h2o->getLiquidLightMap(h);
                    if (!lm)
                    {
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_DARK_WATER;
                    }
                }

                if (!count && liquid_flags[i][j])
                {
                    std::cout << "Wrong liquid type detected in MH2O chunk." << std::endl;
                }

                float* height = h2o->getLiquidHeightMap(h);
                int pos = 0;
                for (int y = 0; y <= h->height; y++)
                {
                    int cy = i * ADT_CELL_SIZE + y + h->yOffset;
                    for (int x = 0; x <= h->width; x++)
                    {
                        int cx = j * ADT_CELL_SIZE + x + h->xOffset;
                        if (height)
                        {
                            liquid_height[cy][cx] = height[pos];
                        }
                        else
                        {
                            liquid_height[cy][cx] = h->heightLevel1;
                        }
                        pos++;
                    }
                }
            }
        }
    }
    //============================================
    // Pack liquid data
    //============================================
    uint8 type = liquid_flags[0][0];
    bool fullType = false;
    for (int y = 0; y < ADT_CELLS_PER_GRID; y++)
    {
        for (int x = 0; x < ADT_CELLS_PER_GRID; x++)
        {
            if (liquid_flags[y][x] != type)
            {
                fullType = true;
                y = ADT_CELLS_PER_GRID;
                break;
            }
        }
    }

    map_liquidHeader liquidHeader;

    // no water data (if all grid have 0 liquid type)
    if (type == 0 && !fullType)
    {
        // No liquid data
        map.liquidMapOffset = 0;
        map.liquidMapSize   = 0;
    }
    else
    {
        int minX = 255, minY = 255;
        int maxX = 0, maxY = 0;
        maxHeight = -20000;
        minHeight = 20000;
        for (int y = 0; y < ADT_GRID_SIZE; y++)
        {
            for (int x = 0; x < ADT_GRID_SIZE; x++)
            {
                if (liquid_show[y][x])
                {
                    if (minX > x)
                    {
                        minX = x;
                    }
                    if (maxX < x)
                    {
                        maxX = x;
                    }
                    if (minY > y)
                    {
                        minY = y;
                    }
                    if (maxY < y)
                    {
                        maxY = y;
                    }
                    float h = liquid_height[y][x];
                    if (maxHeight < h)
                    {
                        maxHeight = h;
                    }
                    if (minHeight > h)
                    {
                        minHeight = h;
                    }
                }
                else
                {
                    liquid_height[y][x] = CONF_use_minHeight;
                }
            }
        }
        map.liquidMapOffset = map.heightMapOffset + map.heightMapSize;
        map.liquidMapSize = sizeof(map_liquidHeader);
        liquidHeader.fourcc = *(uint32 const*)MAP_LIQUID_MAGIC;
        liquidHeader.flags = 0;
        liquidHeader.liquidType = 0;
        liquidHeader.offsetX = minX;
        liquidHeader.offsetY = minY;
        liquidHeader.width   = maxX - minX + 1 + 1;
        liquidHeader.height  = maxY - minY + 1 + 1;
        liquidHeader.liquidLevel = minHeight;

        if (maxHeight == minHeight)
        {
            liquidHeader.flags |= MAP_LIQUID_NO_HEIGHT;
        }

        // Not need store if flat surface
        if (CONF_allow_float_to_int && (maxHeight - minHeight) < CONF_flat_liquid_delta_limit)
        {
            liquidHeader.flags |= MAP_LIQUID_NO_HEIGHT;
        }

        if (!fullType)
        {
            liquidHeader.flags |= MAP_LIQUID_NO_TYPE;
        }

        if (liquidHeader.flags & MAP_LIQUID_NO_TYPE)
        {
            liquidHeader.liquidType = type;
        }
        else
        {
            map.liquidMapSize += sizeof(liquid_entry) + sizeof(liquid_flags);
        }

        if (!(liquidHeader.flags & MAP_LIQUID_NO_HEIGHT))
        {
            map.liquidMapSize += sizeof(float) * liquidHeader.width * liquidHeader.height;
        }
    }

    // map hole info
    uint16 holes[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];

    if (map.liquidMapOffset)
    {
        map.holesOffset = map.liquidMapOffset + map.liquidMapSize;
    }
    else
    {
        map.holesOffset = map.heightMapOffset + map.heightMapSize;
    }

    map.holesSize = sizeof(holes);
    memset(holes, 0, map.holesSize);

    for (int i = 0; i < ADT_CELLS_PER_GRID; ++i)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; ++j)
        {
            adt_MCNK* cell = cells->getMCNK(i, j);
            if (!cell)
            {
                continue;
            }
            holes[i][j] = cell->holes;
        }
    }

    // Ok all data prepared - store it
    FILE* output = fopen(filename2, "wb");
    if (!output)
    {
        std::cout << "Can not create the output file '" << filename2 << "'" << std::endl;
        return false;
    }
    fwrite(&map, sizeof(map), 1, output);
    // Store area data
    fwrite(&areaHeader, sizeof(areaHeader), 1, output);
    if (!(areaHeader.flags & MAP_AREA_NO_AREA))
    {
        fwrite(area_flags, sizeof(area_flags), 1, output);
    }

    // Store height data
    fwrite(&heightHeader, sizeof(heightHeader), 1, output);
    if (!(heightHeader.flags & MAP_HEIGHT_NO_HEIGHT))
    {
        if (heightHeader.flags & MAP_HEIGHT_AS_INT16)
        {
            fwrite(uint16_V9, sizeof(uint16_V9), 1, output);
            fwrite(uint16_V8, sizeof(uint16_V8), 1, output);
        }
        else if (heightHeader.flags & MAP_HEIGHT_AS_INT8)
        {
            fwrite(uint8_V9, sizeof(uint8_V9), 1, output);
            fwrite(uint8_V8, sizeof(uint8_V8), 1, output);
        }
        else
        {
            fwrite(V9, sizeof(V9), 1, output);
            fwrite(V8, sizeof(V8), 1, output);
        }
    }

    // Store liquid data if need
    if (map.liquidMapOffset)
    {
        fwrite(&liquidHeader, sizeof(liquidHeader), 1, output);
        if (!(liquidHeader.flags & MAP_LIQUID_NO_TYPE))
        {
            fwrite(liquid_entry, sizeof(liquid_entry), 1, output);
            fwrite(liquid_flags, sizeof(liquid_flags), 1, output);
        }
        if (!(liquidHeader.flags & MAP_LIQUID_NO_HEIGHT))
        {
            for (int y = 0; y < liquidHeader.height; y++)
            {
                fwrite(&liquid_height[y + liquidHeader.offsetY][liquidHeader.offsetX], sizeof(float), liquidHeader.width, output);
            }
        }
    }

    // store hole data
    fwrite(holes, map.holesSize, 1, output);

    fclose(output);

    return true;
}

/**
 * @brief
 *
 */
void ExtractMapsFromMpq(uint32 build)
{
    char mpq_filename[1024];
    char output_filename[1024];
    char mpq_map_name[1024];

    std::cout << std::endl << " Extracting maps... on build type " << build << std::endl;

    uint32 map_count = ReadMapDBC();

    ReadAreaTableDBC();
    ReadLiquidTypeTableDBC();

    std::string path = output_path->c_str();
    path.append("/maps/");
    CreateDir(path);

    std::cout << std::endl << " Converting map files" << std::endl;
    WDT_file wdt;
    for (uint32 z = 0; z < map_count; ++z)
    {
        std::cout << " Extract " << map_ids[z].name << "(" << z + 1 << "/" << map_count << ")" << std:: endl;
        // Loadup map grid data
        sprintf(mpq_map_name, "World\\Maps\\%s\\%s.wdt", map_ids[z].name, map_ids[z].name);
        
        if (!wdt.loadFile(mpq_map_name, false))
        {
            std::cout << " Warning: Failed loading map " << map_ids[z].name << ".wdt (This message can be safely ignored)" << std::endl;
            continue;
        }

        for (uint32 y = 0; y < WDT_MAP_SIZE; ++y)
        {
            for (uint32 x = 0; x < WDT_MAP_SIZE; ++x)
            {
                if (!wdt.main->adt_list[y][x].exist)
                {
                    continue;
                }
                std::cout << "this seams to exist" << mpq_map_name << std::endl;
                sprintf(mpq_filename, "World\\Maps\\%s\\%s_%u_%u.adt", map_ids[z].name, map_ids[z].name, x, y);
                std::cout << "adt to fetch" << mpq_filename << std::endl;
                sprintf(output_filename, "%s/maps/%04u%02u%02u.map", output_path->c_str(), map_ids[z].id, y, x);
                ConvertADT(mpq_filename, output_filename, build);// , y, x);
            }
            // draw progress bar
            std::cout << " Processing........................" << ((100 * (y + 1)) / WDT_MAP_SIZE) << "%\r"; // ??
        }
    }
    delete [] areas;
    delete [] map_ids;
}

/**
 * @brief
 *
 */
void ExtractDBCFiles(int locale, bool basicLocale)
{
    std::cout << " ___________________________________    " << std::endl;
    std::cout << std:: endl << " Extracting client database files (" << Locales[locale] << ")..." << std::endl;

    std::set<std::string> dbcfiles;

    // get DBC file list
    ArchiveSetBounds archives = GetArchivesBounds();
    for (ArchiveSet::const_iterator i = archives.first; i != archives.second; ++i)
    {
        AppendDBCFileListTo(*i, dbcfiles);
        AppendDB2FileListTo(*i, dbcfiles);
    }

    std::string path = output_path->c_str();
    path.append("/dbc/");
    CreateDir(path);
    if (iCoreNumber == CLIENT_TBC)
    {
        // extract Build info file
        string mpq_name = std::string("component.wow-") + Locales[locale] + ".txt";
        string filename = path + mpq_name;

        std::cout << "    " << filename << std::endl;
        ExtractFile(mpq_name.c_str(), filename);
    }
    if (iCoreNumber == CLIENT_TBC || iCoreNumber == CLIENT_WOTLK || iCoreNumber == CLIENT_CATA)
    {
        if (!basicLocale)
        {
            path += Locales[locale];
            path += "/";
            CreateDir(path);
        }

        // extract Build info file
        {
            string mpq_name = std::string("component.wow-") + Locales[locale] + ".txt";
            string filename = path + mpq_name;

            std::cout << "    " << filename;
            ExtractFile(mpq_name.c_str(), filename);
        }
    }

    // extract DBCs
    int count = 0;
    for (set<string>::iterator iter = dbcfiles.begin(); iter != dbcfiles.end(); ++iter)
    {
        string filename = path;
        filename += (iter->c_str() + strlen("DBFilesClient\\"));

        std::cout << "    " << filename << std::endl;
        if (ExtractFile(iter->c_str(), filename))
        {
            ++count;
        }
    }
    std::cout << "Extracted " << count << " DBC/DB2 files" << std::endl << std::endl;
}

typedef std::pair < std::string /*full_filename*/, char const* /*locale_prefix*/ > UpdatesPair;
typedef std::map < int /*build*/, UpdatesPair > Updates;

void AppendPatchMPQFilesToList(char const* subdir, char const* suffix, char const* section, Updates& updates)
{
    char dirname[512];
    if (subdir)
    {
        sprintf(dirname, "%s/Data/%s", input_path->c_str(), subdir);
    }
    else
    {
        sprintf(dirname, "%s/Data", input_path->c_str());
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
        }
        while (FindNextFile(hFind, &ffd) != 0);

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
    //sprintf(filename, "%s/Data/%s/locale-%s.MPQ", input_path, Locales[locale], Locales[locale]);
    std::stringstream filename;
    // first base old version of dbc files
    filename << input_path->c_str() << "/Data/" << Locales[locale] << "/locale-" << Locales[locale] << ".MPQ";

    HANDLE localeMpqHandle;

    if (!OpenArchive(filename.str().c_str(), &localeMpqHandle))
    {
        std::cout << "Error open archive: " << filename.str() << std::endl;
        return;
    }

    switch (iCoreNumber) {
        case CLIENT_TBC:
        case CLIENT_WOTLK:
            for (int i = 1; i < 5; ++i)
            {
                filename.str("");
                filename << input_path->c_str() << "/Data/" << Locales[locale] << "/patch-" << Locales[locale];
                // on wolk we add an extension
                if (i > 1) filename << "-" << i;

                filename << ".MPQ";

                if (ClientFileExists(filename.str().c_str()) && !OpenArchive(filename.str().c_str()))
                {
                    std::cout << "Error open patch archive: " << filename.str() << std::endl;
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
            for ( int i = 0; Builds[i] && Builds[i] == CONF_TargetBuild; ++i)
            {
                filename.str("");
                //sprintf(filename, "%s/Data/wow-update-base-%u.MPQ", input_path, Builds[i]);
                filename  << input_path->c_str() << "/Data/wow-update-base-" << Builds[i] << ".MPQ";

                std::cout << std::endl << "Patching : " << filename.str() << std::endl;

                //if (!OpenArchive(filename))
                if (!SFileOpenPatchArchive(localeMpqHandle, filename.str().c_str(), "", 0))
                {
                    std::cout << "Error open patch archive: " << filename.str() << std::endl << std::endl;
                }
            }

            for (Updates::const_iterator itr = updates.begin(); itr != updates.end(); ++itr)
            {
                filename.str("");

                if (!itr->second.second)
                {
                    //sprintf(filename, "%s/Data/%s/%s", input_path, Locales[locale], itr->second.first.c_str());
                    filename << input_path->c_str() << "/Data/" << Locales[locale] << "/" << itr->second.first;
                }
                else
                {
                    //sprintf(filename, "%s/Data/%s", input_path, itr->second.first.c_str());
                    filename << input_path->c_str() << "/Data/" << itr->second.first;
                }

                std::cout << std::endl << "Patching : "  << filename.str() << std::endl;

                //if (!OpenArchive(filename))
                if (!SFileOpenPatchArchive(localeMpqHandle, filename.str().c_str(), itr->second.second ? itr->second.second : "", 0))
                {
                    std::cout << "Error open patch archive: " << filename.str() << std::endl;
                }
            }

            // ./Data/Cache patch-base files
            for (int i = 0; Builds[i] && Builds[i] == CONF_TargetBuild; ++i)
            {
                filename.str("");
                //sprintf(filename, "%s/Data/Cache/patch-base-%u.MPQ", input_path, Builds[i]);
                filename << input_path->c_str() << "/Data/Cache/patch-base-" << Builds[i] << ".MPQ";

                std::cout << std::endl << " Patching : " << filename.str() << std::endl;

                //if (!OpenArchive(filename))
                if (!SFileOpenPatchArchive(localeMpqHandle, filename.str().c_str(), "", 0))
                {
                    std::cout << "Error open patch archive: " << filename.str() << std::endl << std::endl;
                }
            }

            // ./Data/Cache/<locale> patch files
            for (int i = 0; Builds[i] && Builds[i] == CONF_TargetBuild; ++i)
            {
                filename.str("");
                //sprintf(filename, "%s/Data/Cache/%s/patch-%s-%u.MPQ", input_path, Locales[locale], Locales[locale], Builds[i]);
                filename << input_path->c_str() << "/Data/Cache/" << Locales[locale] << "/patch-" << Locales[locale] << "-" <<   Builds[i] << ".MPQ";

                std::cout << std::endl << "Patching : " << filename.str() << std::endl;

                //if (!OpenArchive(filename))
                if (!SFileOpenPatchArchive(localeMpqHandle, filename.str().c_str(), "", 0))
                {
                    std::cout << "Error open patch archive: " << filename.str() << std::endl << std::endl;
                }
            }
            break;
    }
}

/////////////////// this function make no sence ////////////////////////////
/**
 * @brief
 *
 */
void LoadCommonMPQFiles(int client)
{
    std::stringstream filename;
    std::vector<std::string> kList;
    switch (client)
    {
    case CLIENT_CLASSIC:
        kList = kClassicMPQList;
        break;
    case CLIENT_TBC:
        kList = kTBCMPQList;
        break;
    case CLIENT_WOTLK:
        kList = kWOTLKMPQList;
        break;
    case CLIENT_CATA:
        kList = kCATAMPQList;
        break;
    }

    struct stat info;
    string locale;
    for (int i = 0; i < LOCALES_COUNT; i++)
    {
        filename.str("");
        filename << input_path->c_str() << "/Data/" << Locales[i] << "/locale-" << Locales[i] << ".MPQ";
        if (ClientFileExists(filename.str().c_str()))
        {
            locale = Locales[i];
            std::cout << " Detected locale: " << locale << std::endl;
            break;
        }
    }

    // Loading patches in reverse-order.
    for (int i = (kList.size() - 1); i >= 0; i--)
    {
        filename.str("");
        filename << input_path->c_str() << "/Data/" << kList[i];
        // %s elemets with real language
        std::string localefile = std::regex_replace(filename.str(), std::regex("\\%s"), locale);

        if (ClientFileExists(localefile.c_str()))
        {
            HANDLE fileHandle;
            /// we open multiple file handler without close?
            if (!OpenArchive(localefile.c_str(), &fileHandle))
            {
                std::cout << "Error open archive: "<< localefile << std::endl;
            }
            //new MPQFile(fileHandle, filename);
        }
    }
}

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char** argv)
{
    showWebsiteBanner();

    if (!HandleArgs(argc, argv))
    {
        exit(1);
    }

    std::cout << "Selected Options:" << std::endl;
    std::cout << "Input Path: " << input_path->c_str() << std::endl;
    std::cout << "Output Path: "<<  output_path->c_str() << std::endl;
    std::cout << "Flat export: " << (CONF_allow_float_to_int ? "true" : "false") << std::endl;
    std::cout << "Extract dbc: " <<  ((CONF_extract | EXTRACT_DBC) ? "true" : "false") << std::endl;
    std::cout << "Extract maps: " << ((CONF_extract | EXTRACT_MAP) ? "true" : "false") << std::endl;

    int thisBuild = getBuildNumber(input_path->c_str());
    iCoreNumber = getCoreNumberFromBuild(thisBuild);
    showBanner("DBC Extractor & Map Generator", iCoreNumber);

    setMapMagicVersion(iCoreNumber, MAP_VERSION_MAGIC);

    if (iCoreNumber == CLIENT_CLASSIC || iCoreNumber == CLIENT_TBC)
    {
        MAP_LIQUID_TYPE_NO_WATER = 0x00;
        MAP_LIQUID_TYPE_MAGMA = 0x01;
        MAP_LIQUID_TYPE_OCEAN = 0x02;
        MAP_LIQUID_TYPE_SLIME = 0x04;
        MAP_LIQUID_TYPE_WATER = 0x08;
    }

    if (iCoreNumber == CLIENT_WOTLK || iCoreNumber == CLIENT_CATA)
    {
        MAP_LIQUID_TYPE_NO_WATER = 0x00;
        MAP_LIQUID_TYPE_WATER = 0x01;
        MAP_LIQUID_TYPE_OCEAN = 0x02;
        MAP_LIQUID_TYPE_MAGMA = 0x04;
        MAP_LIQUID_TYPE_SLIME = 0x08;
    }

    int FirstLocale = -1;

    switch (iCoreNumber) {
        case CLIENT_CLASSIC:
            // Open MPQs
            LoadCommonMPQFiles(iCoreNumber);

            // Extract dbc
            if (CONF_extract & EXTRACT_DBC)
            {
                ExtractDBCFiles(0, true);
            }

            // Extract maps
            if (CONF_extract & EXTRACT_MAP)
            {
                ExtractMapsFromMpq(thisBuild);
            }

            // Close MPQs
            CloseArchives();
            break;
        case CLIENT_TBC:
        case CLIENT_WOTLK:
        case CLIENT_CATA:
        case CLIENT_MOP:
            for (int i = 0; i < LANG_COUNT; i++)
            {
                std::stringstream clientfiles;
                //sprintf(tmp1, "%s/Data/%s/locale-%s.MPQ", input_path, Locales[i], Locales[i]);
                clientfiles << input_path->c_str() << "/Data/" << Locales[i] << "/locale-" << Locales[i] << ".MPQ";

                std::cout << clientfiles.str();

                if (ClientFileExists(clientfiles.str().c_str()))
                {
                    std::cout << " Detected locale: " << Locales[i] << std::endl;

                    //Open MPQs
                    LoadLocaleMPQFiles(i);
                    if ((CONF_extract & EXTRACT_DBC) == 0)
                    {
                        FirstLocale = i;
                        std::cout << " Detected client build: " << thisBuild << std::endl;
                        break;
                    }

                    //Extract DBC files
                    if (FirstLocale < 0)
                    {
                        FirstLocale = i;
                        std::cout << " Detected client build: " << thisBuild << std::endl;
                        ExtractDBCFiles(i, true);
                    }
                    else
                    {
                        ExtractDBCFiles(i, false);
                    }

                    //Close MPQs
                    CloseArchives();
                    break;
                }
            }

            if (FirstLocale < 0)
            {
                std::cout << "No locale detected" << std::endl;
                return 0;
            }

            if (CONF_extract & EXTRACT_MAP)
            {
                std::cout << " Using locale: " << Locales[FirstLocale] << std::endl;

                // Open MPQs
                LoadLocaleMPQFiles(FirstLocale);
                LoadCommonMPQFiles(iCoreNumber);

                // Extract maps
                ExtractMapsFromMpq(thisBuild);

                // Close MPQs
                CloseArchives();
            }
            break;
    }
    return 0;
}
