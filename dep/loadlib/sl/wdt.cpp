//#define _CRT_SECURE_NO_DEPRECATE

#include "wdt.h"

bool wdt_MWMO::prepareLoadedData()
{
    if (fcc != 'MWMO')
        return false;
    return true;
}

bool wdt_MPHD::prepareLoadedData()
{
    if (fcc != 'MPHD')
        return false;
    return true;
}

bool wdt_MAIN::prepareLoadedData()
{
    if (fcc != 'MAIN')
        return false;
    return true;
}

WDT_file::WDT_file()
{
    mphd = 0;
    main = 0;
    wmo  = 0;
}

WDT_file::~WDT_file()
{
    free();
}

void WDT_file::free()
{
    mphd = 0;
    main = 0;
    wmo  = 0;
    FileLoader::free();
}

bool WDT_file::prepareLoadedData()
{
    // Check parent
    if (!FileLoader::prepareLoadedData())
        return false;

    mphd = (wdt_MPHD*)((uint8*)version + version->size + 8);
    if (!mphd->prepareLoadedData())
        return false;
    main = (wdt_MAIN*)((uint8*)mphd + mphd->size + 8);
    if (!main->prepareLoadedData())
        return false;

    // Terrain WDTs may end right after MAIN (Cata-exported maps); MWMO+MODF only exist on WMO-only maps.
    uint8* next = (uint8*)main + main->size + 8;
    if (next + 8 <= GetData() + GetDataSize())
    {
        wmo = (wdt_MWMO*)next;
        if (!wmo->prepareLoadedData())
            return false;
    }

    return true;
}