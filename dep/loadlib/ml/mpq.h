#ifndef MPQ_H
#define MPQ_H

#include "loadlib.h"

using namespace std;

class MPQFile
{
        //MPQHANDLE handle;
        bool eof;
        char* buffer;
        size_t pointer, size;

        // disable copying
        MPQFile(const MPQFile& f);
        void operator=(const MPQFile& f);

    public:
        MPQFile(HANDLE file, const char* filename);    // filenames are not case sensitive
        ~MPQFile()
    {
        close();
    }
        size_t read(void* dest, size_t bytes);
        size_t getSize() { return size; }
        size_t getPos() { return pointer; }
        char* getBuffer() { return buffer; }
        char* getPointer() { return buffer + pointer; }
        bool isEof() { return eof; }
        void seek(int offset);
        void seekRelative(int offset);
        void close();
};

inline void flipcc(char* fcc)
{
    char t;
    t = fcc[0];
    fcc[0] = fcc[3];
    fcc[3] = t;
    t = fcc[1];
    fcc[1] = fcc[2];
    fcc[2] = t;
}

#endif