#include "model.h"
#include "dbcfile.h"
#include "adtfile.h"
#include "vmapexport.h"

#include <algorithm>
#include <stdio.h>
#include <condition_variable>
#include <mutex>
#include <set>

// Dedup model extraction across parallel tile workers. The old
// FileExists()-then-write check is a TOCTOU race once tiles run concurrently.
// One worker extracts a given model; the rest must WAIT until its file is on
// disk, because the placement written straight after (ModelInstance) opens that
// file and silently drops the spawn if it is missing. A plain "claimed" flag is
// not enough -- the file has to exist before any referencing worker proceeds.
static std::mutex s_modelExtractMutex;
static std::condition_variable s_modelExtractCv;
static std::set<std::string> s_modelsInProgress;
static std::set<std::string> s_modelsDone;

bool ExtractSingleModel(std::string& origPath, std::string& fixedName, StringSet& failedPaths)
{
    char const* ext = GetExtension(GetPlainName(origPath.c_str()));

    // < 3.1.0 ADT MMDX section store filename.mdx filenames for corresponded .m2 file
    if (!strcmp(ext, ".mdx"))
    {
        // replace .mdx -> .m2
        origPath.erase(origPath.length() - 2, 2);
        origPath.append("2");
    }
    // >= 3.1.0 ADT MMDX section store filename.m2 filenames for corresponded .m2 file
    // nothing do

    fixedName = GetPlainName(origPath.c_str());

    std::string output(szWorkDirWmo);                       // Stores output filename (possible changed)
    output += "/";
    output += fixedName;

    {
        std::unique_lock<std::mutex> lock(s_modelExtractMutex);
        if (s_modelsDone.count(fixedName))
        {
            return true;
        }
        if (s_modelsInProgress.count(fixedName))
        {
            // Another worker is extracting this model; block until its file is
            // written so the placement that follows can read it.
            s_modelExtractCv.wait(lock, [&] { return s_modelsDone.count(fixedName) != 0; });
            return true;
        }
        if (FileExists(output.c_str()))
        {
            s_modelsDone.insert(fixedName);
            return true;
        }
        // Claim it, then extract outside the lock so distinct models convert in
        // parallel.
        s_modelsInProgress.insert(fixedName);
    }

    Model mdl(origPath);                                    // Possible changed fname
    bool ok = mdl.open(failedPaths) && mdl.ConvertToVMAPModel(output.c_str());

    {
        std::lock_guard<std::mutex> lock(s_modelExtractMutex);
        s_modelsInProgress.erase(fixedName);
        s_modelsDone.insert(fixedName);
    }
    s_modelExtractCv.notify_all();
    return ok;
}

extern HANDLE LocaleMpq;

void ExtractGameobjectModels()
{
    printf("\n");
    printf("Extracting GameObject models...\n");
    DBCFile dbc(LocaleMpq, "DBFilesClient\\GameObjectDisplayInfo.dbc");
    if (!dbc.open())
    {
        printf("Fatal error: Invalid GameObjectDisplayInfo.dbc file format!\n");
        exit(1);
    }

    std::string basepath = szWorkDirWmo;
    basepath += "/";
    std::string path;
    StringSet failedPaths;

    FILE* model_list = fopen((basepath + "temp_gameobject_models").c_str(), "wb");

    for (DBCFile::Iterator it = dbc.begin(); it != dbc.end(); ++it)
    {
        path = it->getString(1);

        if (path.length() < 4)
        {
            continue;
        }

        fixnamen((char*)path.c_str(), path.size());
        char* name = GetPlainName((char*)path.c_str());
        fixname2(name, strlen(name));

        char const* ch_ext = GetExtension(name);
        if (!ch_ext)
        {
            continue;
        }

        //strToLower(ch_ext);

        bool result = false;
        if (!strcmp(ch_ext, ".wmo"))
        {
            result = ExtractSingleWmo(path);
        }
        else if (!strcmp(ch_ext, ".mdl"))
        {
            // TODO: extract .mdl files, if needed
            continue;
        }
        else //if (!strcmp(ch_ext, ".mdx") || !strcmp(ch_ext, ".m2"))
        {
            std::string fixedName;
            result = ExtractSingleModel(path, fixedName, failedPaths);
        }

        if (result)
        {
            uint32 displayId = it->getUInt(0);
            uint32 path_length = strlen(name);
            fwrite(&displayId, sizeof(uint32), 1, model_list);
            fwrite(&path_length, sizeof(uint32), 1, model_list);
            fwrite(name, sizeof(char), path_length, model_list);
        }
    }

    fclose(model_list);

    if (!failedPaths.empty())
    {
        printf("Warning: Some models could not be extracted, see below\n");
        for (StringSet::const_iterator itr = failedPaths.begin(); itr != failedPaths.end(); ++itr)
        {
            printf("Could not find file of model %s\n", itr->c_str());
        }
        printf("A few of these warnings are expected to happen, so be not alarmed!\n");
    }

    printf("Done!\n");
}
