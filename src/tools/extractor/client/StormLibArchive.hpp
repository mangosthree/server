#pragma once

// Real client data, backed by StormLib. Archives are opened lowest-priority first
// and searched last-first, so the patch MPQs override the base ones exactly as the
// client resolves them.

#include "IMpqArchive.hpp"

#include <string>
#include <vector>

namespace world::terrain
{
    class StormLibArchive : public IMpqArchive
    {
    public:
        StormLibArchive() = default;
        ~StormLibArchive() override;

        StormLibArchive(const StormLibArchive&) = delete;
        StormLibArchive& operator=(const StormLibArchive&) = delete;

        bool AddArchive(const std::string& mpqPath);

        // Both lists are lowest-priority first and the locale ones are opened last.
        // Which archives exist is the largest difference between client versions, so
        // the names are passed in rather than known here; {locale} expands to `locale`
        // and the locale paths are relative to the locale directory. Missing names are
        // skipped, not an error.
        int OpenClientData(const std::string& dataDir,
                           const std::vector<std::string>& archives,
                           const std::vector<std::string>& localeArchives,
                           const std::string& locale);

        /**
         * @brief The same, plus a patch chain -- what 4.3.4 and later need.
         *
         * `updates` and `localeUpdates` are the wow-update-* archives IN ASCENDING BUILD
         * ORDER. Each is attached to every base archive of its own side with
         * SFileOpenPatchArchive AND opened as an ordinary high-priority handle, because
         * both are required and for different reasons -- see the note on Read().
         */
        int OpenPatchedClientData(const std::string& dataDir,
                                  const std::vector<std::string>& archives,
                                  const std::vector<std::string>& localeArchives,
                                  const std::vector<std::string>& updates,
                                  const std::vector<std::string>& localeUpdates,
                                  const std::string& locale);

        bool Read(const std::string& path, std::vector<uint8_t>& out) override;
        bool Contains(const std::string& path) const override;

        std::vector<std::string> FindFiles(const std::string& pattern) const;

        size_t ArchiveCount() const { return m_handles.size(); }

    private:
        std::vector<void*> m_handles;
    };

    // The 3.3.5a archive chain, lowest priority first.
    const std::vector<std::string>& ClientArchives335a();
    const std::vector<std::string>& ClientLocaleArchives335a();

    // The 4.3.4 chain. Cataclysm drops common/expansion/lichking for
    // world/world2/art/expansion1-3, and adds a real patch chain: the wow-update-*
    // archives carry PTCH deltas, not replacement files.
    const std::vector<std::string>& ClientArchives434();
    const std::vector<std::string>& ClientLocaleArchives434();
    const std::vector<std::string>& ClientUpdates434();
    const std::vector<std::string>& ClientLocaleUpdates434();
}
