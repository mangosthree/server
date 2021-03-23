//#define _CRT_SECURE_NO_DEPRECATE

#include "loadlib.h"
#include <cstdio>

u_map_fcc MverMagic = { {'R','E','V','M'} };

ChunkedFile::ChunkedFile()
{
    data = 0;
    data_size = 0;
}

ChunkedFile::~ChunkedFile()
{
    free();
}

bool ChunkedFile::loadFile(HANDLE mpq, char* filename, bool log)
{
    free();
    HANDLE file;
    if (!SFileOpenFileEx(mpq, filename, SFILE_OPEN_FROM_MPQ, &file))
    {
        if (log)
            printf("No such file %s\n", filename);
        return false;
    }

    data_size = SFileGetFileSize(file, NULL);
    data = new uint8[data_size];
    SFileReadFile(file, data, data_size, NULL/*bytesRead*/, NULL);
    parseChunks();
    if (prepareLoadedData())
    {
        SFileCloseFile(file);
        return true;
    }

    printf("Error loading %s\n", filename);
    SFileCloseFile(file);
    free();
    return false;
}

bool ChunkedFile::prepareLoadedData()
{
    FileChunk* chunk = GetChunk("MVER");
    if (!chunk)
        return false;

    // Check version
    file_MVER* version = chunk->As<file_MVER>();
    if (version->fcc != MverMagic.fcc)
        return false;
    if (version->ver != FILE_FORMAT_VERSION)
        return false;
    return true;
}

void ChunkedFile::free()
{
    for (auto chunk : chunks)
        delete chunk.second;

    chunks.clear();

    delete[] data;
    data = 0;
    data_size = 0;
}

u_map_fcc InterestingChunks[] = {
    { 'R', 'E', 'V', 'M' },
    { 'N', 'I', 'A', 'M' },
    { 'O', '2', 'H', 'M' },
    { 'K', 'N', 'C', 'M' },
    { 'T', 'V', 'C', 'M' },
    { 'Q', 'L', 'C', 'M' }
};

bool IsInterestingChunk(u_map_fcc const& fcc)
{
    for (u_map_fcc const& f : InterestingChunks)
        if (f.fcc == fcc.fcc)
            return true;

    return false;
}

void ChunkedFile::parseChunks()
{
    uint8* ptr = GetData();
    while (ptr < GetData() + GetDataSize())
    {
        u_map_fcc header = *(u_map_fcc*)ptr;
        uint32 size = 0;
        if (IsInterestingChunk(header))
        {
            size = *(uint32*)(ptr + 4);
            if (size <= data_size)
            {
                std::swap(header.fcc_txt[0], header.fcc_txt[3]);
                std::swap(header.fcc_txt[1], header.fcc_txt[2]);

                FileChunk* chunk = new FileChunk{ ptr, size };
                chunk->parseSubChunks();
                chunks.insert({ std::string(header.fcc_txt, 4), chunk });
            }
        }

        // move to next chunk
        ptr += size + 8;
    }
}

FileChunk* ChunkedFile::GetChunk(std::string const& name)
{
    auto range = chunks.equal_range(name);
    if (std::distance(range.first, range.second) == 1)
        return range.first->second;

    return NULL;
}

FileChunk::~FileChunk()
{
    for (auto subchunk : subchunks)
        delete subchunk.second;

    subchunks.clear();
}

void FileChunk::parseSubChunks()
{
    uint8* ptr = data + 8; // skip self
    while (ptr < data + size)
    {
        u_map_fcc header = *(u_map_fcc*)ptr;
        uint32 subsize = 0;
        if (IsInterestingChunk(header))
        {
            subsize = *(uint32*)(ptr + 4);
            if (subsize < size)
            {
                std::swap(header.fcc_txt[0], header.fcc_txt[3]);
                std::swap(header.fcc_txt[1], header.fcc_txt[2]);

                FileChunk* chunk = new FileChunk{ ptr, subsize };
                chunk->parseSubChunks();
                subchunks.insert({ std::string(header.fcc_txt, 4), chunk });
            }
        }

        // move to next chunk
        ptr += subsize + 8;
    }
}

FileChunk* FileChunk::GetSubChunk(std::string const& name)
{
    auto range = subchunks.equal_range(name);
    if (std::distance(range.first, range.second) == 1)
        return range.first->second;

    return NULL;
}

// list of mpq files for lookup most recent file version
ArchiveSet gOpenArchives;

ArchiveSetBounds GetArchivesBounds()
{
    return ArchiveSetBounds(gOpenArchives.begin(), gOpenArchives.end());
}

bool OpenArchive(char const* mpqFileName, HANDLE* mpqHandlePtr /*= NULL*/)
{
    HANDLE mpqHandle;

    if (!SFileOpenArchive(mpqFileName, 0, MPQ_OPEN_READ_ONLY, &mpqHandle))
        return false;

    gOpenArchives.push_back(mpqHandle);

    if (mpqHandlePtr)
        *mpqHandlePtr = mpqHandle;

    return true;
}

bool OpenNewestFile(char const* filename, HANDLE* fileHandlerPtr)
{
    for (ArchiveSet::const_reverse_iterator i = gOpenArchives.rbegin(); i != gOpenArchives.rend(); ++i)
    {
        // always prefer get updated file version
        if (SFileOpenFileEx(*i, filename, SFILE_OPEN_FROM_MPQ, fileHandlerPtr))
            return true;
    }

    return false;
}

bool ExtractFile(char const* mpq_name, std::string const& filename)
{
    for (ArchiveSet::const_reverse_iterator i = gOpenArchives.rbegin(); i != gOpenArchives.rend(); ++i)
    {
        HANDLE fileHandle;
        if (!SFileOpenFileEx(*i, mpq_name, SFILE_OPEN_FROM_MPQ, &fileHandle))
            continue;

        if (SFileGetFileSize(fileHandle, NULL) == 0)              // some files removed in next updates and its reported  size 0
        {
            SFileCloseFile(fileHandle);
            return true;
        }

        SFileCloseFile(fileHandle);

        if (!SFileExtractFile(*i, mpq_name, filename.c_str(), SFILE_OPEN_FROM_MPQ))
        {
            printf("Can't extract file: %s\n", mpq_name);
            return false;
        }

        return true;
    }

    printf("Extracting file not found: %s\n", filename.c_str());
    return false;
}


void CloseArchives()
{
    for (ArchiveSet::const_iterator i = gOpenArchives.begin(); i != gOpenArchives.end(); ++i)
        SFileCloseArchive(*i);
    gOpenArchives.clear();
}

FileLoader::FileLoader()
{
    data = 0;
    data_size = 0;
    version = 0;
}

FileLoader::~FileLoader()
{
    free();
}

bool FileLoader::loadFile(char* filename, bool log)
{
    free();

    HANDLE fileHandle = 0;

    if (!OpenNewestFile(filename, &fileHandle))
    {
        if (log)
            printf("No such file %s\n", filename);
        return false;
    }

    data_size = SFileGetFileSize(fileHandle, NULL);

    data = new uint8 [data_size];
    if (!data)
    {
        SFileCloseFile(fileHandle);
        return false;
    }

    if (!SFileReadFile(fileHandle, data, data_size, NULL, NULL))
    {
        if (log)
            printf("Can't read file %s\n", filename);
        SFileCloseFile(fileHandle);
        return false;
    }

    SFileCloseFile(fileHandle);

    // ToDo: Fix WDT errors...
    if (!prepareLoadedData())
    {
        //printf("Error loading %s\n\n", filename);
        //free();
        return false;
    }

    return true;
}

bool FileLoader::prepareLoadedData()
{
    // Check version
    version = (file_MVER*) data;
    if (version->fcc != 'MVER')
        return false;
    if (version->ver != FILE_FORMAT_VERSION)
        return false;
    return true;
}

void FileLoader::free()
{
    if (data) delete[] data;
    data = 0;
    data_size = 0;
    version = 0;
}