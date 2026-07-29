#include <string>
#include <vector>
#include "StormLibArchive.hpp"
#include <cstring>

#include <StormLib.h>

#include <cstdio>
#include <unordered_set>

namespace world::terrain
{
    namespace
    {
        std::string ExpandLocale(std::string s, const std::string& locale)
        {
            const std::string token = "{locale}";
            for (size_t at = s.find(token); at != std::string::npos;
                 at = s.find(token, at + locale.size()))
            {
                s.replace(at, token.size(), locale);
            }
            return s;
        }
    }

    // Lowest priority first: Read walks the handles in reverse, so the last archive
    // opened is the first searched. Verified against StormLib's own patch chain
    // (SFileOpenPatchArchive) over every DBC and WDT plus a sample of ADTs -- 1047
    // files, byte-identical.
    const std::vector<std::string>& ClientArchives335a()
    {
        static const std::vector<std::string> archives = {
            "common.MPQ",   "common-2.MPQ", "expansion.MPQ", "lichking.MPQ",
            "patch.MPQ",    "patch-2.MPQ",  "patch-3.MPQ",
        };
        return archives;
    }

    const std::vector<std::string>& ClientLocaleArchives335a()
    {
        // Lowest priority first, so the NUMBERED locale patches come last and win. Get
        // that backwards and Map.dbc is read from patch-enUS.MPQ instead of
        // patch-enUS-3.MPQ -- an older file that parses perfectly and is simply missing
        // the maps added since. This repo's own reference extractor has it backwards:
        // kWOTLKMPQList lists "patch-<locale>.MPQ" first and its loader gives the first
        // entry the highest priority.
        //
        // 3.3.5a spells them "patch-<locale>-2.MPQ", not "patch-2-<locale>.MPQ".
        //
        // The speech archives are deliberately absent: they were measured to hold no
        // DBC, WDT, ADT, WMO or M2 at all -- 8460 files of audio across the three -- so
        // opening them would cost handles and buy nothing.
        static const std::vector<std::string> archives = {
            "base-{locale}.MPQ",
            "locale-{locale}.MPQ",
            "expansion-locale-{locale}.MPQ",
            "lichking-locale-{locale}.MPQ",
            "patch-{locale}.MPQ",
            "patch-{locale}-2.MPQ",
            "patch-{locale}-3.MPQ",
        };
        return archives;
    }

    StormLibArchive::~StormLibArchive()
    {
        for (void* h : m_handles)
        {
            if (h)
            {
                SFileCloseArchive(h);
            }
        }
    }

    bool StormLibArchive::AddArchive(const std::string& mpqPath)
    {
        HANDLE h = nullptr;
        if (!SFileOpenArchive(mpqPath.c_str(), 0, MPQ_OPEN_READ_ONLY, &h))
        {
            return false;
        }
        m_handles.push_back(h);
        return true;
    }

    int StormLibArchive::OpenClientData(const std::string& dataDir,
                                        const std::vector<std::string>& archives,
                                        const std::vector<std::string>& localeArchives,
                                        const std::string& locale)
    {
        int opened = 0;
        for (const std::string& rel : archives)
        {
            if (AddArchive(dataDir + "/" + ExpandLocale(rel, locale)))
            {
                ++opened;
            }
        }

        // No locale at all is a real client, not a mistake: a 1.12 install keeps every
        // archive, DBCs included, at the Data root and has no locale directory. Skip the
        // whole side rather than probing "Data//locale-.MPQ" a dozen times.
        if (locale.empty())
        {
            return opened;
        }

        const std::string localeDir = dataDir + "/" + locale;
        for (const std::string& rel : localeArchives)
        {
            if (AddArchive(localeDir + "/" + ExpandLocale(rel, locale)))
            {
                ++opened;
            }
        }
        return opened;
    }

    // The 4.3.4 archive set, and the update chain in ASCENDING BUILD ORDER: each is a
    // delta on top of the one before, so the update order is not cosmetic.
    //
    // The order among the BASES is, though, and that was measured rather than assumed:
    // the seven partition the data. Every member shared between any two of them is
    // byte-identical (world/world2 share 1691 files and 1689 match; the other two are
    // the MPQ's own (attributes)/(listfile)). Only the updates carry newer content.
    // sound.MPQ is deliberately absent -- no geometry, 1.6 GB to index.
    const std::vector<std::string>& ClientArchives434()
    {
        static const std::vector<std::string> archives = {
            "base-Win.MPQ",
            "alternate.MPQ",
            "world.MPQ", "world2.MPQ", "art.MPQ",
            "expansion1.MPQ", "expansion2.MPQ", "expansion3.MPQ",
        };
        return archives;
    }

    const std::vector<std::string>& ClientLocaleArchives434()
    {
        static const std::vector<std::string> archives = {
            "locale-{locale}.MPQ",
            "expansion1-locale-{locale}.MPQ",
            "expansion2-locale-{locale}.MPQ",
            "expansion3-locale-{locale}.MPQ",
        };
        return archives;
    }

    const std::vector<std::string>& ClientUpdates434()
    {
        static const std::vector<std::string> updates = {
            "wow-update-base-15211.MPQ",
            "wow-update-base-15354.MPQ",
            "wow-update-base-15595.MPQ",
        };
        return updates;
    }

    const std::vector<std::string>& ClientLocaleUpdates434()
    {
        static const std::vector<std::string> updates = {
            "wow-update-{locale}-15211.MPQ",
            "wow-update-{locale}-15354.MPQ",
            "wow-update-{locale}-15595.MPQ",
        };
        return updates;
    }

    int StormLibArchive::OpenPatchedClientData(
        const std::string& dataDir,
        const std::vector<std::string>& archives,
        const std::vector<std::string>& localeArchives,
        const std::vector<std::string>& updates,
        const std::vector<std::string>& localeUpdates,
        const std::string& locale)
    {
        const std::string localeDir = dataDir + "/" + locale;

        // The bases first, in priority order, remembering where each side starts so the
        // right updates can be attached to the right bases: wow-update-base-* patches the
        // non-locale archives and wow-update-<loc>-* the locale ones.
        const size_t baseBegin = m_handles.size();
        for (const std::string& rel : archives)
        {
            AddArchive(dataDir + "/" + ExpandLocale(rel, locale));
        }
        const size_t localeBegin = m_handles.size();
        if (!locale.empty())
        {
            for (const std::string& rel : localeArchives)
            {
                AddArchive(localeDir + "/" + ExpandLocale(rel, locale));
            }
        }
        const size_t localeEnd = m_handles.size();

        // ATTACH, so a patched file resolves. NULL prefix, never a forced one: StormLib's
        // FindPatchPrefix says WoW patches "mostly do not use prefix" and derives one only
        // when base\(patch_metadata) is present. Forcing "base" or the locale fails with
        // ERROR_CANT_FIND_PATCH_PREFIX.
        auto attach = [&](const std::vector<std::string>& names, const std::string& dir,
                          size_t from, size_t to)
        {
            for (size_t i = from; i < to; ++i)
            {
                for (const std::string& rel : names)
                {
                    const std::string path = dir + "/" + ExpandLocale(rel, locale);
                    SFileOpenPatchArchive(m_handles[i], path.c_str(), nullptr, 0);
                }
            }
        };

        attach(updates, dataDir, baseBegin, localeBegin);
        attach(localeUpdates, localeDir, localeBegin, localeEnd);

        // ...AND open them ordinarily, at the top of the priority chain, because an update
        // also carries WHOLE files that exist in no base at all -- a model added after
        // release is reachable no other way.
        for (const std::string& rel : updates)
        {
            AddArchive(dataDir + "/" + ExpandLocale(rel, locale));
        }
        if (!locale.empty())
        {
            for (const std::string& rel : localeUpdates)
            {
                AddArchive(localeDir + "/" + ExpandLocale(rel, locale));
            }
        }

        return int(m_handles.size());
    }

    /**
     * @brief Read the file, skipping any handle that answers with an incremental patch.
     *
     * A PTCH blob is not a file. The update archives sit high in the priority chain so
     * that whole files added after release are found, which means for a file that was
     * PATCHED rather than replaced they answer first, with the delta. Rejecting it and
     * carrying on lands on the base archive, which has the same updates attached and so
     * returns the file with the chain already applied.
     *
     * Measured, not assumed: on 4.3.4 every one of the 20 .dbc and 5 .db2 members of
     * wow-update-enUS-15595.MPQ is a PTCH, and Spell.dbc reads as a 136 KB delta here
     * against 17.3 MB and 73253 records through the chain.
     */
    bool StormLibArchive::Read(const std::string& path, std::vector<uint8_t>& out)
    {
        for (auto it = m_handles.rbegin(); it != m_handles.rend(); ++it)
        {
            HANDLE hFile = nullptr;
            if (!SFileOpenFileEx(*it, path.c_str(), 0, &hFile))
            {
                continue;
            }

            DWORD high = 0;
            const DWORD size = SFileGetFileSize(hFile, &high);
            if (size == SFILE_INVALID_SIZE)
            {
                SFileCloseFile(hFile);
                continue;
            }

            out.resize(size);
            DWORD got = 0;
            if (size > 0)
            {
                SFileReadFile(hFile, out.data(), size, &got, nullptr);
            }
            SFileCloseFile(hFile);
            if (got == size)
            {
                if (out.size() >= 4 && std::memcmp(out.data(), "PTCH", 4) == 0)
                {
                    out.clear();
                    continue;               // a delta, not the file -- keep looking
                }
                return true;
            }
            out.clear();
        }
        return false;
    }

    bool StormLibArchive::Contains(const std::string& path) const
    {
        for (auto it = m_handles.rbegin(); it != m_handles.rend(); ++it)
        {
            if (SFileHasFile(*it, const_cast<char*>(path.c_str())))
            {
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> StormLibArchive::FindFiles(const std::string& pattern) const
    {
        std::vector<std::string> result;
        if (m_handles.empty() || pattern.empty())
        {
            return result;
        }

        std::unordered_set<std::string> seen;
        for (auto it = m_handles.rbegin(); it != m_handles.rend(); ++it)
        {
            SFILE_FIND_DATA findData{};
            HANDLE hFind = SFileFindFirstFile(*it, pattern.c_str(), &findData, nullptr);
            if (!hFind)
            {
                continue;
            }
            do
            {
                std::string name(findData.cFileName);
                if (seen.insert(name).second)
                {
                    result.push_back(std::move(name));
                }
            } while (SFileFindNextFile(hFind, &findData));
            SFileFindClose(hFind);
        }
        return result;
    }
}
