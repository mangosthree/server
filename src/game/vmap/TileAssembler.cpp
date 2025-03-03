/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

#include "TileAssembler.h"
#include "MapTree.h"
#include "BIH.h"
#include "VMapDefinitions.h"

#include <set>
#include <iomanip>
#include <sstream>
#include <iomanip>

using G3D::Vector3;
using G3D::AABox;
using G3D::inf;
using std::pair;

// Template specialization for getting bounds of ModelSpawn objects
template<> struct BoundsTrait<VMAP::ModelSpawn*>
{
    static void getBounds(const VMAP::ModelSpawn* const& obj, G3D::AABox& out) { out = obj->getBounds(); }
};

namespace VMAP
{
    /**
     * @brief Reads a chunk of data from a file and compares it with a given string.
     *
     * @param rf The file to read from.
     * @param dest The destination buffer to read into.
     * @param compare The string to compare with.
     * @param len The length of the string to compare.
     * @return bool True if the read data matches the string, false otherwise.
     */
    bool readChunk(FILE* rf, char* dest, const char* compare, uint32 len)
    {
        if (fread(dest, sizeof(char), len, rf) != len)
        {
            return false;
        }
        return memcmp(dest, compare, len) == 0;
    }

    /**
     * @brief Transforms a given vector by the model's position and rotation.
     *
     * @param pIn The input vector to transform.
     * @return Vector3 The transformed vector.
     */
    Vector3 ModelPosition::transform(const Vector3& pIn) const
    {
        Vector3 out = pIn * iScale;
        out = iRotation * out;
        return(out);
    }

    //=================================================================

    /**
     * @brief Constructor to initialize the TileAssembler.
     *
     * @param pSrcDirName The source directory name.
     * @param pDestDirName The destination directory name.
     */
    TileAssembler::TileAssembler(const std::string& pSrcDirName, const std::string& pDestDirName)
    {
        iCurrentUniqueNameId = 0;
        iFilterMethod = NULL;
        iSrcDir = pSrcDirName;
        iDestDir = pDestDirName;
        // mkdir(iDestDir);
        // init();
    }

    /**
     * @brief Destructor to clean up resources.
     */
    TileAssembler::~TileAssembler()
    {
        // delete iCoordModelMapping;
    }

    bool TileAssembler::convertWorld2()
    {
        bool success = readMapSpawns();
        if (!success)
        {
            return false;
        }

        // Export map data
        for (MapData::iterator map_iter = mapData.begin(); map_iter != mapData.end() && success; ++map_iter)
        {
            // Build global map tree
            std::vector<ModelSpawn*> mapSpawns;
            UniqueEntryMap::iterator entry;

            std::cout << "Calculating model bounds for map " << map_iter->first << std::endl;
            for (entry = map_iter->second->UniqueEntries.begin(); entry != map_iter->second->UniqueEntries.end(); ++entry)
            {
                // M2 models don't have a bound set in WDT/ADT placement data, I still think they're not used for LoS at all on retail
                if (entry->second.flags & MOD_M2)
                {
                    if (!calculateTransformedBound(entry->second))
                    {
                        break;
                    }
                }
                else if (entry->second.flags & MOD_WORLDSPAWN) // WMO maps and terrain maps use different origin, so we need to adapt :/
                {
                    // TODO: remove extractor hack and uncomment below line:
                    // entry->second.iPos += Vector3(533.33333f*32, 533.33333f*32, 0.f);
                    entry->second.iBound = entry->second.iBound + Vector3(533.33333f * 32, 533.33333f * 32, 0.f);
                }
                mapSpawns.push_back(&(entry->second));
                spawnedModelFiles.insert(entry->second.name);
            }

            std::cout << "Creating map tree..." << std::endl;
            BIH pTree;
            pTree.build(mapSpawns, BoundsTrait<ModelSpawn*>::getBounds);

            // ===> possibly move this code to StaticMapTree class
            std::map<uint32, uint32> modelNodeIdx;
            for (uint32 i = 0; i < mapSpawns.size(); ++i)
            {
                modelNodeIdx.insert(pair<uint32, uint32>(mapSpawns[i]->ID, i));
            }

            // Write map tree file
            std::stringstream mapfilename;
            mapfilename << iDestDir << "/" << std::setfill('0') << std::setw(3) << map_iter->first << ".vmtree";
            FILE* mapfile = fopen(mapfilename.str().c_str(), "wb");
            if (!mapfile)
            {
                success = false;
                std::cout << "Can not open " << mapfilename.str();
                break;
            }

            // General info
            if (success && fwrite(VMAP_MAGIC, 1, 8, mapfile) != 8)
            {
                success = false;
            }
            uint32 globalTileID = StaticMapTree::packTileID(65, 65);
            pair<TileMap::iterator, TileMap::iterator> globalRange = map_iter->second->TileEntries.equal_range(globalTileID);
            char isTiled = globalRange.first == globalRange.second; // Only maps without terrain (tiles) have global WMO
            if (success && fwrite(&isTiled, sizeof(char), 1, mapfile) != 1)
            {
                success = false;
            }
            // Nodes
            if (success && fwrite("NODE", 4, 1, mapfile) != 1)
            {
                success = false;
            }
            if (success)
            {
                success = pTree.WriteToFile(mapfile);
            }
            // Global map spawns (WDT), if any (most instances)
            if (success && fwrite("GOBJ", 4, 1, mapfile) != 1)
            {
                success = false;
            }

            for (TileMap::iterator glob = globalRange.first; glob != globalRange.second && success; ++glob)
            {
                success = ModelSpawn::WriteToFile(mapfile, map_iter->second->UniqueEntries[glob->second]);
            }

            fclose(mapfile);

            // <====

            // Write map tile files, similar to ADT files, only with extra BSP tree node info
            TileMap& tileEntries = map_iter->second->TileEntries;
            TileMap::iterator tile;
            for (tile = tileEntries.begin(); tile != tileEntries.end(); ++tile)
            {
                const ModelSpawn& spawn = map_iter->second->UniqueEntries[tile->second];
                if (spawn.flags & MOD_WORLDSPAWN)           // WDT spawn, saved as tile 65/65 currently...
                {
                    continue;
                }
                uint32 nSpawns = tileEntries.count(tile->first);
                std::stringstream tilefilename;
                tilefilename.fill('0');
                tilefilename << iDestDir << "/" << std::setw(3) << map_iter->first << "_";
                uint32 x, y;
                StaticMapTree::unpackTileID(tile->first, x, y);
                tilefilename << std::setw(2) << x << "_" << std::setw(2) << y << ".vmtile";
                FILE* tilefile = fopen(tilefilename.str().c_str(), "wb");
                // File header
                if (success && fwrite(VMAP_MAGIC, 1, 8, tilefile) != 8)
                {
                    success = false;
                }
                // Write number of tile spawns
                if (success && fwrite(&nSpawns, sizeof(uint32), 1, tilefile) != 1)
                {
                    success = false;
                }
                // Write tile spawns
                for (uint32 s = 0; s < nSpawns; ++s)
                {
                    if (s && tile != tileEntries.end())
                    {
                        ++tile;
                    }
                    const ModelSpawn& spawn2 = map_iter->second->UniqueEntries[tile->second];
                    success = success && ModelSpawn::WriteToFile(tilefile, spawn2);
                    // MapTree nodes to update when loading tile:
                    std::map<uint32, uint32>::iterator nIdx = modelNodeIdx.find(spawn2.ID);
                    if (success && fwrite(&nIdx->second, sizeof(uint32), 1, tilefile) != 1)
                    {
                        success = false;
                    }
                }
                fclose(tilefile);
            }
            // break; // test, extract only first map; TODO: remove this line
        }

        // add an object models, listed in temp_gameobject_models file
        exportGameobjectModels();

        // export objects
        std::cout <<  std::endl << "Converting Model Files" << std::endl;
        for (std::set<std::string>::iterator mfile = spawnedModelFiles.begin(); mfile != spawnedModelFiles.end(); ++mfile)
        {
            std::cout << "Converting " << *mfile << std::endl;
            if (!convertRawFile(*mfile))
            {
                std::cout << "error converting " << *mfile << std::endl;
                success = false;
                break;
            }
        }

        // Cleanup:
        for (MapData::iterator map_iter = mapData.begin(); map_iter != mapData.end(); ++map_iter)
        {
            delete map_iter->second;
        }
        return success;
    }

    /**
     * @brief Reads the map spawns from a file.
     *
     * @return bool True if successful, false otherwise.
     */
    bool TileAssembler::readMapSpawns()
    {
        std::string fname = iSrcDir + "/dir_bin";
        FILE* dirf = fopen(fname.c_str(), "rb");
        if (!dirf)
        {
            std::cout << "Could not read dir_bin file!" << std::endl;
            return false;
        }
        std::cout << "Read coordinate mapping..." << std::endl;
        uint32 mapID, tileX, tileY, check = 0;
        // Prepare vectors for reading bounding coordinates
        G3D::Vector3 v1, v2;
        // Temporary storage for reading ModelSpawn entries
        ModelSpawn spawn;

        // Read until the end of the file is reached
        while (!feof(dirf))
        {
            check = 0;
            // read mapID, tileX, tileY, Flags, adtID, ID, Pos, Rot, Scale, Bound_lo, Bound_hi, name
            check += fread(&mapID, sizeof(uint32), 1, dirf);
            if (check == 0)
            {
                break;
            }

            // Read tile coordinates
            check += fread(&tileX, sizeof(uint32), 1, dirf);
            check += fread(&tileY, sizeof(uint32), 1, dirf);

            // Read the spawn details from the file into 'spawn'
            if (!ModelSpawn::ReadFromFile(dirf, spawn))
            {
                // If reading fails, end the loop
                break;
            }

            // Find or create a MapSpawns object for the current mapID
            MapSpawns* current;
            MapData::iterator map_iter = mapData.find(mapID);
            if (map_iter == mapData.end())
            {
                std::cout << "spawning Map " <<  mapID << std::endl;
                mapData[mapID] = current = new MapSpawns();
            }
            else
            {
                current = (*map_iter).second;
            }

            // Store the model spawn in the UniqueEntries container
            current->UniqueEntries.insert(pair<uint32, ModelSpawn>(spawn.ID, spawn));
            // Associate the tile ID (constructed from tileX, tileY) with the spawn ID
            current->TileEntries.insert(pair<uint32, uint32>(StaticMapTree::packTileID(tileX, tileY), spawn.ID));
        }

        // Check for any file read errors
        bool success = (ferror(dirf) == 0);
        fclose(dirf);
        return success;
    }

    bool TileAssembler::calculateTransformedBound(ModelSpawn& spawn)
    {
        // Construct full path to the model file
        std::string modelFilename = iSrcDir + "/" + spawn.name;

        // Initialize ModelPosition with rotation and scale
        ModelPosition modelPosition;
        modelPosition.iDir = spawn.iRot;
        modelPosition.iScale = spawn.iScale;
        modelPosition.init();

        // Load the raw model data from disk
        WorldModel_Raw raw_model;
        if (!raw_model.Read(modelFilename.c_str()))
        {
            return false;
        }

        // If the model has multiple groups, it might indicate it's not an M2
        uint32 groups = raw_model.groupsArray.size();
        if (groups != 1)
        {
            std::cout << "Warning: '" << modelFilename << "' does not seem to be a M2 model!" << std::endl;
        }

        // We'll track the bounding box of the entire model
        AABox modelBound;
        bool boundEmpty = true;

        // Iterate over each group to accumulate vertex data
        // Should be only one for M2 files...
        for (uint32 g = 0; g < groups; ++g)
        {
            std::vector<Vector3>& vertices = raw_model.groupsArray[g].vertexArray;

            // If no vertices, log an error about missing geometry
            if (vertices.empty())
            {
                std::cout << "error: model '" << spawn.name << "' has no geometry!" << std::endl;
                continue;
            }

            // Transform all vertices by rotation and scale, then update bounding box
            uint32 nvectors = vertices.size();
            for (uint32 i = 0; i < nvectors; ++i)
            {
                Vector3 v = modelPosition.transform(vertices[i]);
                if (boundEmpty)
                {
                    modelBound = AABox(v, v);
                    boundEmpty = false;
                }
                else
                {
                    modelBound.merge(v);
                }
            }
        }

        // Add the spawn position to shift the bounding box into world space
        spawn.iBound = modelBound + spawn.iPos;
        spawn.flags |= MOD_HAS_BOUND;
        return true;
    }

    struct WMOLiquidHeader
    {
        int xverts, yverts, xtiles, ytiles;
        float pos_x;
        float pos_y;
        float pos_z;
        short type;
    };

    bool TileAssembler::convertRawFile(const std::string& pModelFilename)
    {
        bool success = true;
        std::string filename = iSrcDir;
        if (filename.length() > 0)
        {
            filename.append("/");
        }
        filename.append(pModelFilename);

        // Read the raw model into a temporary structure
        WorldModel_Raw raw_model;
        if (!raw_model.Read(filename.c_str()))
        {
            return false;
        }

        // Prepare final WorldModel and set root WMO ID
        WorldModel model;
        model.SetRootWmoID(raw_model.RootWMOID);

        // If any group data was found, transform it into GroupModel structures
        if (raw_model.groupsArray.size())
        {
            std::vector<GroupModel> groupsArray;
            uint32 groups = raw_model.groupsArray.size();

            // Copy geometry and bounding data from raw model group to final group
            for (uint32 g = 0; g < groups; ++g)
            {
                GroupModel_Raw& raw_group = raw_model.groupsArray[g];
                groupsArray.push_back(GroupModel(raw_group.mogpflags, raw_group.GroupWMOID, raw_group.bounds));
                groupsArray.back().SetMeshData(raw_group.vertexArray, raw_group.triangles);
                groupsArray.back().setLiquidData(raw_group.liquid);
            }

            model.SetGroupModels(groupsArray);
        }

        success = model.writeFile(iDestDir + "/" + pModelFilename + ".vmo");

        //std::cout << "readRawFile2: '" << pModelFilename << "' tris: " << nElements << " nodes: " << nNodes << std::endl;
        return success;
    }

    void TileAssembler::exportGameobjectModels()
    {
        // Open the file that lists gameobject models
        FILE* model_list = fopen((iSrcDir + "/" + GAMEOBJECT_MODELS).c_str(), "rb");
        if (!model_list)
        {
            return;
        }

        // Open a new file in the destination to copy & enrich the data
        FILE* model_list_copy = fopen((iDestDir + "/" + GAMEOBJECT_MODELS).c_str(), "wb");
        if (!model_list_copy)
        {
            fclose(model_list);
            return;
        }

        // Buffers to read the display ID and model name
        uint32 name_length, displayId;
        char buff[500];

        // Read until reaching the end of the model list file
        while (!feof(model_list))
        {
            // Attempt to read displayId field
            if (fread(&displayId, sizeof(uint32), 1, model_list) <= 0)
            {
                // If we haven't truly reached EOF, the file may be corrupt
                if (!feof(model_list))
                {
                    std::cout << std::endl << "File '" << GAMEOBJECT_MODELS << "' seems to be corrupted" << std::endl;
                }
                break;
            }

            // Attempt to read the length of the model name
            if (fread(&name_length, sizeof(uint32), 1, model_list) <= 0)
            {
                std::cout << std::endl << "File '" << GAMEOBJECT_MODELS << "' seems to be corrupted" << std::endl;
                break;
            }

            // Check for overly large read sizes
            if (name_length >= sizeof(buff))
            {
                std::cout << std::endl << "File '" << GAMEOBJECT_MODELS << "' seems to be corrupted" << std::endl;
                break;
            }

            // Read the actual model name
            if (fread(&buff, sizeof(char), name_length, model_list) <= 0)
            {
                std::cout << std::endl << "File '" << GAMEOBJECT_MODELS << "' seems to be corrupted" << std::endl;
                break;
            }
            std::string model_name(buff, name_length);

            // Load model to help gather bounding box info
            WorldModel_Raw raw_model;
            if (!raw_model.Read((iSrcDir + "/" + model_name).c_str()))
            {
                // If it fails, skip updating bounding box data
                continue;
            }

            // Register this model in the 'spawnedModelFiles' set
            spawnedModelFiles.insert(model_name);

            // Compute bounding box from the vertices in all groups
            AABox bounds;
            bool boundEmpty = true;
            for (uint32 g = 0; g < raw_model.groupsArray.size(); ++g)
            {
                std::vector<Vector3>& vertices = raw_model.groupsArray[g].vertexArray;

                uint32 nvectors = vertices.size();
                for (uint32 i = 0; i < nvectors; ++i)
                {
                    Vector3& v = vertices[i];
                    if (boundEmpty)
                    {
                        bounds = AABox(v, v);
                        boundEmpty = false;
                    }
                    else
                    {
                        bounds.merge(v);
                    }
                }
            }

            // Copy data to the new file and append bounding box info
            fwrite(&displayId, sizeof(uint32), 1, model_list_copy);
            fwrite(&name_length, sizeof(uint32), 1, model_list_copy);
            fwrite(&buff, sizeof(char), name_length, model_list_copy);
            fwrite(&bounds.low(), sizeof(Vector3), 1, model_list_copy);
            fwrite(&bounds.high(), sizeof(Vector3), 1, model_list_copy);
        }

        // Close the files once done
        fclose(model_list);
        fclose(model_list_copy);
    }

    //-----------------------------------------------------------------------------

    // Macros used to simplify chunk reading and comparison for GroupModel_Raw
#define READ_OR_RETURN(V,S) \
    if(fread((V), (S), 1, rf) != 1) \
    { \
        fclose(rf); \
        std::cout << "readfail, op = " << readOperation << std::endl; \
        return(false); \
    }
#define CMP_OR_RETURN(V,S) \
    if(strcmp((V),(S)) != 0) \
    { \
        fclose(rf); \
        std::cout << "cmpfail, " << (V) << "!=" << (S) << std::endl; \
        return(false); \
    }

/**
 * @brief Reads group data from a raw file, including bounding box, indices, vertices, and liquid.
 *
 * @param rf The file handle to read from.
 * @return bool True if the read was successful, otherwise false.
 */
    bool GroupModel_Raw::Read(FILE* rf)
    {
        char blockId[5];
        blockId[4] = 0;   // Ensure string terminator
        int blocksize;
        int readOperation = 0;

        // Read basic properties
        READ_OR_RETURN(&mogpflags, sizeof(uint32));
        READ_OR_RETURN(&GroupWMOID, sizeof(uint32));

        // Read bounding box data
        Vector3 vec1, vec2;
        READ_OR_RETURN(&vec1, sizeof(Vector3));
        READ_OR_RETURN(&vec2, sizeof(Vector3));
        bounds.set(vec1, vec2);

        // Read custom liquid flags
        READ_OR_RETURN(&liquidflags, sizeof(uint32));

        // Read group branches for debugging or future use (currently not used)
        uint32 branches;
        READ_OR_RETURN(&blockId, 4);
        CMP_OR_RETURN(blockId, "GRP ");
        READ_OR_RETURN(&blocksize, sizeof(int));
        READ_OR_RETURN(&branches, sizeof(uint32));
        for (uint32 b = 0; b < branches; ++b)
        {
            uint32 indexes;
            READ_OR_RETURN(&indexes, sizeof(uint32));
            // This data block is a placeholder for further expansions
        }

        // Read indices block
        READ_OR_RETURN(&blockId, 4);
        CMP_OR_RETURN(blockId, "INDX");
        READ_OR_RETURN(&blocksize, sizeof(int));
        uint32 nindexes;
        READ_OR_RETURN(&nindexes, sizeof(uint32));

        if (nindexes > 0)
        {
            // Allocate memory for index data
            uint16* indexarray = new uint16[nindexes];
            if (fread(indexarray, nindexes * sizeof(uint16), 1, rf) != 1)
            {
                fclose(rf);
                delete[] indexarray;
                std::cout << "readfail, op = " << readOperation << std::endl;
                return false;
            }

            // Convert sets of three indices into triangles
            triangles.reserve(nindexes / 3);
            for (uint32 i = 0; i < nindexes; i += 3)
            {
                triangles.push_back(MeshTriangle(indexarray[i], indexarray[i + 1], indexarray[i + 2]));
            }

            delete[] indexarray;
        }

        // Read vertex block
        READ_OR_RETURN(&blockId, 4);
        CMP_OR_RETURN(blockId, "VERT");
        READ_OR_RETURN(&blocksize, sizeof(int));
        uint32 nvectors;
        READ_OR_RETURN(&nvectors, sizeof(uint32));

        if (nvectors > 0)
        {
            // Allocate temporary memory for vector data
            float* vectorarray = new float[nvectors * 3];
            if (fread(vectorarray, nvectors * sizeof(float) * 3, 1, rf) != 1)
            {
                fclose(rf);
                delete[] vectorarray;
                std::cout << "readfail, op = " << readOperation;
                return false;
            }

            // Store the vectors in the vertexArray
            for (uint32 i = 0; i < nvectors; ++i)
            {
                vertexArray.push_back(Vector3(vectorarray + 3 * i));
            }
            delete[] vectorarray;
        }

        // Read liquid data if liquid flags are present
        liquid = 0;
        if (liquidflags & 1)
        {
            WMOLiquidHeader hlq;
            READ_OR_RETURN(&blockId, 4);
            CMP_OR_RETURN(blockId, "LIQU");
            READ_OR_RETURN(&blocksize, sizeof(int));
            READ_OR_RETURN(&hlq, sizeof(WMOLiquidHeader));

            liquid = new WmoLiquid(hlq.xtiles, hlq.ytiles, Vector3(hlq.pos_x, hlq.pos_y, hlq.pos_z), hlq.type);
            uint32 size = hlq.xverts * hlq.yverts;
            READ_OR_RETURN(liquid->GetHeightStorage(), size * sizeof(float));

            size = hlq.xtiles * hlq.ytiles;
            READ_OR_RETURN(liquid->GetFlagsStorage(), size);
        }

        return true;
    }

    /**
     * @brief Destructor to clean up allocated liquid data.
     */
    GroupModel_Raw::~GroupModel_Raw()
    {
        // Ensure we don't leak the allocated WmoLiquid object
        delete liquid;
    }

    bool WorldModel_Raw::Read(const char* path)
    {
        // Attempt to open the file
        FILE* rf = fopen(path, "rb");
        if (!rf)
        {
            std::cout << "ERROR: Can't open raw model file: " << path << std::endl;
            return false;
        }

        char ident[8];
        int readOperation = 0;

        // Check the file magic to ensure it's the correct type
        READ_OR_RETURN(&ident, 8);
        CMP_OR_RETURN(ident, RAW_VMAP_MAGIC);

        // Skip reading a 32-bit int, which was used during export
        uint32 tempNVectors;
        READ_OR_RETURN(&tempNVectors, sizeof(tempNVectors));

        // Read the number of groups and the root WMO ID
        uint32 groups;
        READ_OR_RETURN(&groups, sizeof(uint32));
        READ_OR_RETURN(&RootWMOID, sizeof(uint32));

        // Resize array to hold all group structures
        groupsArray.resize(groups);

        // For every group, read the contents into GroupModel_Raw
        bool succeed = true;
        for (uint32 g = 0; g < groups && succeed; ++g)
        {
            succeed = groupsArray[g].Read(rf);
        }

        fclose(rf);
        return succeed;
    }

    // Undefine macros used for reading, to avoid scope pollution
#undef READ_OR_RETURN
#undef CMP_OR_RETURN
}
