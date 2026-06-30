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

#define _CRT_SECURE_NO_WARNINGS

#include "vmapexport.h"
#include "wmo.h"
#include "vec3d.h"
#include "adtfile.h"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <map>
#include <fstream>
#include <mutex>
#undef min
#undef max
#include "mpqfile.h"
#include <G3D/Matrix3.h>
#include <G3D/Quat.h>
#include <G3D/Vector3.h>

using namespace std;
extern uint16* LiqType;

WMORoot::WMORoot(std::string& filename) : filename(filename)
{
}

extern HANDLE WorldMpq;

bool WMORoot::open()
{
    MPQFile f(WorldMpq, filename.c_str());
    if (f.isEof())
    {
        printf(" No such file %s.\n", filename.c_str());
        return false;
    }

    uint32 size;
    char fourcc[5];

    while (!f.isEof())
    {
        f.read(fourcc, 4);
        f.read(&size, 4);

        flipcc(fourcc);
        fourcc[4] = 0;

        size_t nextpos = f.getPos() + size;

        if (!strcmp(fourcc, "MOHD")) //header
        {
            f.read(&nTextures, 4);
            f.read(&nGroups, 4);
            f.read(&nP, 4);
            f.read(&nLights, 4);
            f.read(&nModels, 4);
            f.read(&nDoodads, 4);
            f.read(&nDoodadSets, 4);
            f.read(&col, 4);
            f.read(&RootWMOID, 4);
            f.read(bbcorn1, 12);
            f.read(bbcorn2, 12);
            f.read(&liquidType, 4);
        }
        else if (!strcmp(fourcc, "MODS")) // doodad sets
        {
            DoodadData.Sets.resize(size / sizeof(WMODoodad::MODS));
            if (size)
            {
                f.read(DoodadData.Sets.data(), size);
            }
        }
        else if (!strcmp(fourcc, "MODN")) // doodad name block
        {
            if (size)
            {
                DoodadData.Paths.reset(new char[size]);
                f.read(DoodadData.Paths.get(), size);
                DoodadData.Paths[size - 1] = '\0'; // no-op for well-formed MODN, guards strlen on corrupt data
                DoodadData.PathsLen = size;
                // Extract every referenced model now, so doodad spawn emission
                // can find its collision geometry under szWorkDirWmo later.
                StringSet failedDoodadPaths;
                char* ptr = DoodadData.Paths.get();
                char* end = ptr + size;
                while (ptr < end)
                {
                    size_t len = 0;
                    while (ptr + len < end && ptr[len])
                    {
                        ++len;
                    }
                    std::string path(ptr, len);
                    ptr += len + 1;
                    if (path.length() < 4 || !GetExtension(GetPlainName(path.c_str())))
                    {
                        continue;
                    }
                    fixnamen(&path[0], path.length());
                    char* s = GetPlainName(&path[0]);
                    fixname2(s, strlen(s));
                    std::string uName;
                    ExtractSingleModel(path, uName, failedDoodadPaths);
                }
            }
        }
        else if (!strcmp(fourcc, "MODD")) // doodad definitions (40 bytes each)
        {
            DoodadData.Spawns.resize(size / sizeof(WMODoodad::MODD));
            if (size)
            {
                f.read(DoodadData.Spawns.data(), size);
            }
        }
        else if (!strcmp(fourcc, "MOGN")) // packed group-name blob
        {
            GroupNames.resize(size);
            if (size)
            {
                f.read(GroupNames.data(), size);
            }
        }
        f.seek((int)nextpos);
    }
    f.close();
    return true;
}

bool WMORoot::ConvertToVMAPRootWmo(FILE* pOutfile)
{
    //printf("Convert RootWmo...\n");

    fwrite(szRawVMAPMagic, 1, 8, pOutfile);
    unsigned int nVectors = 0;
    fwrite(&nVectors, sizeof(nVectors), 1, pOutfile); // will be filled later
    fwrite(&nGroups, 4, 1, pOutfile);
    fwrite(&RootWMOID, 4, 1, pOutfile);
    return true;
}

WMORoot::~WMORoot()
{
}

WMOGroup::WMOGroup(std::string& filename) : filename(filename),
    MOPY(0), MOVI(0), MoviEx(0), MOVT(0), MOBA(0), MobaEx(0), hlq(0), LiquEx(0), LiquBytes(0)
{
}

bool WMOGroup::ShouldSkip(WMORoot const* root) const
{
    // Unreachable groups (MOGP 0x80) are level-streaming hints, not walkable areas.
    if (mogpFlags & 0x80)
    {
        return true;
    }
    // Antiportal groups (MOGP 0x4000000) are invisible renderer occluders;
    // they must not contribute collision/LoS mesh. Mirrors TC vmap4_extractor.
    if (mogpFlags & 0x4000000)
    {
        return true;
    }
    // Some WMOs flag antiportals by group name instead of the MOGP bit.
    if (root && groupName >= 0 &&
        static_cast<std::size_t>(groupName) < root->GroupNames.size() &&
        !strcmp(&root->GroupNames[groupName], "antiportal"))
    {
        return true;
    }
    return false;
}

bool WMOGroup::open()
{
    MPQFile f(WorldMpq, filename.c_str());
    if (f.isEof())
    {
        printf(" No such file.\n");
        return false;
    }
    uint32 size;
    char fourcc[5];
    while (!f.isEof())
    {
        f.read(fourcc, 4);
        f.read(&size, 4);
        flipcc(fourcc);
        if (!strcmp(fourcc, "MOGP")) //Fix sizeoff = Data size.
        {
            size = 68;
        }
        fourcc[4] = 0;
        size_t nextpos = f.getPos() + size;

        // A corrupt/truncated chunk header can declare a size past the end of
        // the file; stop here rather than drive a wild new[]/read off it.
        if (nextpos > f.getSize())
        {
            break;
        }

        LiquEx_size = 0;
        liquflags = 0;

        if (!strcmp(fourcc, "MOGP")) //header
        {
            f.read(&groupName, 4);
            f.read(&descGroupName, 4);
            f.read(&mogpFlags, 4);
            f.read(bbcorn1, 12);
            f.read(bbcorn2, 12);
            f.read(&moprIdx, 2);
            f.read(&moprNItems, 2);
            f.read(&nBatchA, 2);
            f.read(&nBatchB, 2);
            f.read(&nBatchC, 4);
            f.read(&fogIdx, 4);
            f.read(&liquidType, 4);
            f.read(&groupWMOID, 4);

        }
        else if (!strcmp(fourcc, "MOPY"))
        {
            MOPY = new char[size];
            mopy_size = size;
            nTriangles = (int)size / 2;
            f.read(MOPY, size);
        }
        else if (!strcmp(fourcc, "MOVI"))
        {
            MOVI = new uint16[size / 2];
            f.read(MOVI, size);
        }
        else if (!strcmp(fourcc, "MOVT"))
        {
            MOVT = new float[size / 4];
            f.read(MOVT, size);
            nVertices = (int)size / 12;
        }
        else if (!strcmp(fourcc, "MONR"))
        {
        }
        else if (!strcmp(fourcc, "MOTV"))
        {
        }
        else if (!strcmp(fourcc, "MOBA"))
        {
            MOBA = new uint16[size / 2];
            moba_size = size / 2;
            f.read(MOBA, size);
        }
        else if (!strcmp(fourcc, "MLIQ"))
        {
            liquflags |= 1;
            // Value-initialize so the 2 padding bytes after `type` are zeroed:
            // the read below fills only the 30 (0x1E) data bytes, but the whole
            // struct (sizeof = 32) is later written out, so stale padding made
            // the extracted .wmo bytes non-deterministic run-to-run.
            hlq = new WMOLiquidHeader();
            f.read(hlq, 0x1E);
            LiquEx_size = sizeof(WMOLiquidVert) * hlq->xverts * hlq->yverts;
            int nLiquBytes = hlq->xtiles * hlq->ytiles;
            // Validate the declared liquid geometry against the chunk size before
            // allocating, so a corrupt MLIQ header can't drive a wild allocation.
            if (hlq->xverts < 0 || hlq->yverts < 0 || hlq->xtiles < 0 || hlq->ytiles < 0 ||
                0x1E + LiquEx_size + size_t(nLiquBytes) > size)
            {
                printf("Warning: skipping malformed MLIQ chunk in WMO group.\n");
                liquflags &= ~1;
                delete hlq;
                hlq = 0;
            }
            else
            {
                LiquEx = new WMOLiquidVert[hlq->xverts * hlq->yverts];
                f.read(LiquEx, LiquEx_size);
                LiquBytes = new char[nLiquBytes];
                f.read(LiquBytes, nLiquBytes);
            }

            /* std::ofstream llog("Buildings/liquid.log", ios_base::out | ios_base::app);
            llog << filename;
            llog << "\nbbox: " << bbcorn1[0] << ", " << bbcorn1[1] << ", " << bbcorn1[2] << " | " << bbcorn2[0] << ", " << bbcorn2[1] << ", " << bbcorn2[2];
            llog << "\nlpos: " << hlq->pos_x << ", " << hlq->pos_y << ", " << hlq->pos_z;
            llog << "\nx-/yvert: " << hlq->xverts << "/" << hlq->yverts << " size: " << size << " expected size: " << 30 + hlq->xverts*hlq->yverts*8 + hlq->xtiles*hlq->ytiles << std::endl;
            llog.close(); */
        }
        f.seek((int)nextpos);
    }
    f.close();
    return true;
}

int WMOGroup::ConvertToVMAPGroupWmo(FILE* output, WMORoot* rootWMO, bool pPreciseVectorData)
{
    fwrite(&mogpFlags, sizeof(uint32), 1, output);
    fwrite(&groupWMOID, sizeof(uint32), 1, output);
    // group bound
    fwrite(bbcorn1, sizeof(float), 3, output);
    fwrite(bbcorn2, sizeof(float), 3, output);
    fwrite(&liquflags, sizeof(uint32), 1, output);
    int nColTriangles = 0;
    if (pPreciseVectorData)
    {
        char GRP[] = "GRP ";
        fwrite(GRP, 1, 4, output);

        int k = 0;
        int moba_batch = moba_size / 12;
        MobaEx = new int[moba_batch * 4];
        for (int i = 8; i < moba_size; i += 12)
        {
            MobaEx[k++] = MOBA[i];
        }
        int moba_size_grp = moba_batch * 4 + 4;
        fwrite(&moba_size_grp, 4, 1, output);
        fwrite(&moba_batch, 4, 1, output);
        fwrite(MobaEx, 4, k, output);
        delete [] MobaEx;

        uint32 nIdexes = nTriangles * 3;

        if (fwrite("INDX", 4, 1, output) != 1)
        {
            printf("Error while writing file nbraches ID");
            exit(0);
        }
        int wsize = sizeof(uint32) + sizeof(unsigned short) * nIdexes;
        if (fwrite(&wsize, sizeof(int), 1, output) != 1)
        {
            printf("Error while writing file wsize");
            // no need to exit?
        }
        if (fwrite(&nIdexes, sizeof(uint32), 1, output) != 1)
        {
            printf("Error while writing file nIndexes");
            exit(0);
        }
        if (nIdexes > 0)
        {
            if (fwrite(MOVI, sizeof(unsigned short), nIdexes, output) != nIdexes)
            {
                printf("Error while writing file indexarray");
                exit(0);
            }
        }

        if (fwrite("VERT", 4, 1, output) != 1)
        {
            printf("Error while writing file nbraches ID");
            exit(0);
        }
        wsize = sizeof(int) + sizeof(float) * 3 * nVertices;
        if (fwrite(&wsize, sizeof(int), 1, output) != 1)
        {
            printf("Error while writing file wsize");
            // no need to exit?
        }
        if (fwrite(&nVertices, sizeof(int), 1, output) != 1)
        {
            printf("Error while writing file nVertices");
            exit(0);
        }
        if (nVertices > 0)
        {
            if (fwrite(MOVT, sizeof(float) * 3, nVertices, output) != nVertices)
            {
                printf("Error while writing file vectors");
                exit(0);
            }
        }

        nColTriangles = nTriangles;
    }
    else
    {
        char GRP[] = "GRP ";
        fwrite(GRP, 1, 4, output);
        int k = 0;
        int moba_batch = moba_size / 12;
        MobaEx = new int[moba_batch * 4];
        for (int i = 8; i < moba_size; i += 12)
        {
            MobaEx[k++] = MOBA[i];
        }

        int moba_size_grp = moba_batch * 4 + 4;
        fwrite(&moba_size_grp, 4, 1, output);
        fwrite(&moba_batch, 4, 1, output);
        fwrite(MobaEx, 4, k, output);
        delete [] MobaEx;

        //-------INDX------------------------------------
        //-------MOPY--------
        MoviEx = new uint16[nTriangles * 3]; // "worst case" size...
        int* IndexRenum = new int[nVertices];
        memset(IndexRenum, 0xFF, nVertices * sizeof(int));
        for (int i = 0; i < nTriangles; ++i)
        {
            // Keep collidable triangles only: collision faces, or rendered
            // non-detail faces (wowdev SMOPoly::isCollidable).
            bool isRenderFace = (MOPY[2 * i] & WMO_MATERIAL_RENDER) && !(MOPY[2 * i] & WMO_MATERIAL_DETAIL);
            bool isCollision = (MOPY[2 * i] & WMO_MATERIAL_COLLISION) || isRenderFace;
            if (!isCollision)
            {
                continue;
            }
            // Use this triangle
            for (int j = 0; j < 3; ++j)
            {
                IndexRenum[MOVI[3 * i + j]] = 1;
                MoviEx[3 * nColTriangles + j] = MOVI[3 * i + j];
            }
            ++nColTriangles;
        }

        // assign new vertex index numbers
        int nColVertices = 0;
        for (uint32 i = 0; i < nVertices; ++i)
        {
            if (IndexRenum[i] == 1)
            {
                IndexRenum[i] = nColVertices;
                ++nColVertices;
            }
        }

        // translate triangle indices to new numbers
        for (int i = 0; i < 3 * nColTriangles; ++i)
        {
            assert(MoviEx[i] < nVertices);
            MoviEx[i] = IndexRenum[MoviEx[i]];
        }

        // write triangle indices
        int INDX[] = {0x58444E49, nColTriangles * 6 + 4, nColTriangles * 3};
        fwrite(INDX, 4, 3, output);
        fwrite(MoviEx, 2, nColTriangles * 3, output);

        // write vertices
        int VERT[] = {0x54524556, nColVertices * 3 * sizeof(float) + 4, nColVertices}; // "VERT"
        int check = 3 * nColVertices;
        fwrite(VERT, 4, 3, output);
        for (uint32 i = 0; i < nVertices; ++i)
            if (IndexRenum[i] >= 0)
            {
                check -= fwrite(MOVT + 3 * i, sizeof(float), 3, output);
            }

        assert(check == 0);

        delete [] MoviEx;
        delete [] IndexRenum;
    }

    //------LIQU------------------------
    if (LiquEx_size != 0)
    {
        int LIQU_h[] = {0x5551494C, sizeof(WMOLiquidHeader) + LiquEx_size + hlq->xtiles* hlq->ytiles}; // "LIQU"
        fwrite(LIQU_h, 4, 2, output);

        // according to WoW.Dev Wiki:
        uint32 liquidEntry;
        if (rootWMO->liquidType & 4)
        {
            liquidEntry = liquidType;
        }
        else if (liquidType == 15)
        {
            liquidEntry = 0;
        }
        else
        {
            liquidEntry = liquidType + 1;
        }

        if (!liquidEntry)
        {
            int v1; // edx@1
            int v2; // eax@1

            v1 = hlq->xtiles * hlq->ytiles;
            v2 = 0;
            if (v1 > 0)
            {
                while ((LiquBytes[v2] & 0xF) == 15)
                {
                    ++v2;
                    if (v2 >= v1)
                    {
                        break;
                    }
                }

                if (v2 < v1 && (LiquBytes[v2] & 0xF) != 15)
                {
                    liquidEntry = (LiquBytes[v2] & 0xF) + 1;
                }
            }
        }

        if (liquidEntry && liquidEntry < 21)
        {
            switch (((uint8)liquidEntry - 1) & 3)
            {
                case 0:
                    {
                        liquidEntry = ((mogpFlags & 0x80000) != 0) + 13;
                    }
                    break;
                case 1:
                    liquidEntry = 14;
                    break;
                case 2:
                    liquidEntry = 19;
                    break;
                case 3:
                    {
                    liquidEntry = 20;
                    }
                    break;
                default:
                    break;
            }
        }

        hlq->type = liquidEntry;

        /* std::ofstream llog("Buildings/liquid.log", ios_base::out | ios_base::app);
        llog << filename;
        llog << ":\nliquidEntry: " << liquidEntry << " type: " << hlq->type << " (root:" << rootWMO->liquidType << " group:" << liquidType << ")\n";
        llog.close(); */

        fwrite(hlq, sizeof(WMOLiquidHeader), 1, output);
        // only need height values, the other values are unknown anyway
        for (uint32 i = 0; i < LiquEx_size / sizeof(WMOLiquidVert); ++i)
        {
            fwrite(&LiquEx[i].height, sizeof(float), 1, output);
        }
        // todo: compress to bit field
        fwrite(LiquBytes, 1, hlq->xtiles * hlq->ytiles, output);
    }

    return nColTriangles;
}

WMOGroup::~WMOGroup()
{
    delete [] MOPY;
    delete [] MOVI;
    delete [] MOVT;
    delete [] MOBA;
    delete hlq;
    delete [] LiquEx;
    delete [] LiquBytes;
}

//WmoInstName is in the form MD5/name.wmo
WMOInstance::WMOInstance(MPQFile& f, const char* WmoInstName, uint32 mapID, uint32 tileX, uint32 tileY, FILE* pDirfile)
{
    pos = Vec3D(0, 0, 0);

    float ff[3];
    f.read(&id, 4);
    f.read(ff, 12);
    pos = Vec3D(ff[0], ff[1], ff[2]);
    f.read(ff, 12);
    rot = Vec3D(ff[0], ff[1], ff[2]);
    f.read(ff, 12);
    pos2 = Vec3D(ff[0], ff[1], ff[2]);
    f.read(ff, 12);
    pos3 = Vec3D(ff[0], ff[1], ff[2]);
    f.read(&d2, 4);

    uint16 trash, adtId;
    f.read(&adtId, 2);
    f.read(&trash, 2);

    //-----------add_in _dir_file----------------

    char tempname[512];
    sprintf(tempname, "%s/%s", szWorkDirWmo, WmoInstName);
    FILE* input;
    input = fopen(tempname, "r+b");

    if (!input)
    {
        printf("WMOInstance::WMOInstance: couldn't open %s\n", tempname);
        return;
    }

    fseek(input, 8, SEEK_SET); // get the correct no of vertices
    int nVertices;
    fread(&nVertices, sizeof(int), 1, input);
    fclose(input);

    if (nVertices == 0)
    {
        return;
    }

    float x, z;
    x = pos.x;
    z = pos.z;
    if (x == 0 && z == 0)
    {
        pos.x = 533.33333f * 32;
        pos.z = 533.33333f * 32;
    }
    pos = fixCoords(pos);
    pos2 = fixCoords(pos2);
    pos3 = fixCoords(pos3);

    float scale = 1.0f;
    uint32 flags = MOD_HAS_BOUND;
    if (tileX == 65 && tileY == 65)
    {
        flags |= MOD_WORLDSPAWN;
    }
    //write mapID, tileX, tileY, Flags, ID, Pos, Rot, Scale, Bound_lo, Bound_hi, name
    fwrite(&mapID, sizeof(uint32), 1, pDirfile);
    fwrite(&tileX, sizeof(uint32), 1, pDirfile);
    fwrite(&tileY, sizeof(uint32), 1, pDirfile);
    fwrite(&flags, sizeof(uint32), 1, pDirfile);
    fwrite(&adtId, sizeof(uint16), 1, pDirfile);
    fwrite(&id, sizeof(uint32), 1, pDirfile);
    fwrite(&pos, sizeof(float), 3, pDirfile);
    fwrite(&rot, sizeof(float), 3, pDirfile);
    fwrite(&scale, sizeof(float), 1, pDirfile);
    fwrite(&pos2, sizeof(float), 3, pDirfile);
    fwrite(&pos3, sizeof(float), 3, pDirfile);
    uint32 nlen = strlen(WmoInstName);
    fwrite(&nlen, sizeof(uint32), 1, pDirfile);
    fwrite(WmoInstName, sizeof(char), nlen, pDirfile);

    ExtractDoodadSet(WmoInstName, mapID, tileX, tileY, pDirfile);
}

/// Stable per-(WMO instance, doodad index) spawn ids from the top half of the
/// uint32 range, so they cannot collide with client MODF/MDDF unique ids.
static uint32 GenerateDoodadUniqueId(uint32 wmoUniqueId, uint32 doodadIndex)
{
    static std::mutex s_doodadIdMutex;
    static std::map<std::pair<uint32, uint32>, uint32> doodadIdMap;
    static uint32 nextId = 0x80000000;

    // Guards the shared map + counter against concurrent tile workers. The id a
    // given (wmo, doodad) gets still depends on first-seen order, so threaded
    // runs assign different (but equally valid, unique) ids than serial runs --
    // the values are opaque spawn tags, so output stays functionally correct.
    std::lock_guard<std::mutex> lock(s_doodadIdMutex);
    uint32& uid = doodadIdMap[std::make_pair(wmoUniqueId, doodadIndex)];
    if (uid == 0)
    {
        uid = nextId++;
    }
    return uid;
}

void WMOInstance::ExtractDoodadSet(const char* WmoInstName, uint32 mapID, uint32 tileX, uint32 tileY, FILE* pDirfile)
{
    std::map<std::string, WMODoodadData>::const_iterator doodadItr = g_WmoDoodads.find(WmoInstName);
    if (doodadItr == g_WmoDoodads.end())
    {
        return;
    }

    const WMODoodadData& doodadData = doodadItr->second;
    if (doodadData.Sets.empty() || !doodadData.Paths)
    {
        return;
    }

    // Only the always-active doodad set 0 (Set_$DefaultGlobal) for now.
    const WMODoodad::MODS& set = doodadData.Sets[0];

    // pos is already fixCoords'd above; rot is the raw MODF Euler degrees.
    G3D::Vector3 wmoPos(pos.x, pos.y, pos.z);
    G3D::Matrix3 wmoRot = G3D::Matrix3::fromEulerAnglesZYX(G3D::toRadians(rot.y), G3D::toRadians(rot.x), G3D::toRadians(rot.z));

    size_t spawnEnd = size_t(set.StartIndex) + set.Count;
    if (spawnEnd > doodadData.Spawns.size())
    {
        spawnEnd = doodadData.Spawns.size();
    }

    for (size_t i = set.StartIndex; i < spawnEnd; ++i)
    {
        const WMODoodad::MODD& doodad = doodadData.Spawns[i];
        if (doodad.NameIndex >= doodadData.PathsLen)
        {
            continue;
        }

        char ModelInstName[1024];
        const char* plainName = GetPlainName(&doodadData.Paths[doodad.NameIndex]);
        uint32 nlen = strlen(plainName);
        if (nlen <= 3 || nlen >= sizeof(ModelInstName))
        {
            continue;
        }
        strcpy(ModelInstName, plainName);
        fixnamen(ModelInstName, nlen);
        fixname2(ModelInstName, nlen);
        // Extracted models are stored as .m2
        char* extension = &ModelInstName[nlen - 4];
        if (!strcmp(extension, ".mdx") || !strcmp(extension, ".mdl"))
        {
            ModelInstName[nlen - 2] = '2';
            ModelInstName[nlen - 1] = '\0';
            --nlen;
        }

        // Skip doodads without extracted collision geometry.
        char tempname[1036];
        sprintf(tempname, "%s/%s", szWorkDirWmo, ModelInstName);
        FILE* input = fopen(tempname, "r+b");
        if (!input)
        {
            continue;
        }
        fseek(input, 8, SEEK_SET); // get the correct no of vertices
        int nVertices = 0;
        size_t count = fread(&nVertices, sizeof(int), 1, input);
        fclose(input);
        if (count != 1 || nVertices == 0)
        {
            continue;
        }

        G3D::Vector3 worldPos = wmoPos + wmoRot * G3D::Vector3(doodad.Position.x, doodad.Position.y, doodad.Position.z);
        Vec3D position(worldPos.x, worldPos.y, worldPos.z);

        Vec3D rotation;
        (G3D::Quat(doodad.RotX, doodad.RotY, doodad.RotZ, doodad.RotW).toRotationMatrix() * wmoRot)
            .toEulerAnglesXYZ(rotation.z, rotation.x, rotation.y);
        rotation.x = G3D::toDegrees(rotation.x);
        rotation.y = G3D::toDegrees(rotation.y);
        rotation.z = G3D::toDegrees(rotation.z);

        uint16 adtId = 0; // not used for models
        uint32 uniqueId = GenerateDoodadUniqueId(id, uint32(i));
        uint32 flags = MOD_M2;
        if (tileX == 65 && tileY == 65)
        {
            flags |= MOD_WORLDSPAWN;
        }
        float scale = doodad.Scale; // MODD scale is a direct float, unlike MDDF

        //write mapID, tileX, tileY, Flags, ID, Pos, Rot, Scale, name
        fwrite(&mapID, sizeof(uint32), 1, pDirfile);
        fwrite(&tileX, sizeof(uint32), 1, pDirfile);
        fwrite(&tileY, sizeof(uint32), 1, pDirfile);
        fwrite(&flags, sizeof(uint32), 1, pDirfile);
        fwrite(&adtId, sizeof(uint16), 1, pDirfile);
        fwrite(&uniqueId, sizeof(uint32), 1, pDirfile);
        fwrite(&position, sizeof(float), 3, pDirfile);
        fwrite(&rotation, sizeof(float), 3, pDirfile);
        fwrite(&scale, sizeof(float), 1, pDirfile);
        fwrite(&nlen, sizeof(uint32), 1, pDirfile);
        fwrite(ModelInstName, sizeof(char), nlen, pDirfile);
    }
}
