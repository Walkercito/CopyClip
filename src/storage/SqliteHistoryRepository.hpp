#pragma once

// SQLite-backed core::HistoryRepository, mirroring the reference module
// copyclip/storage/sqlite_history.py. SQLite::Database closes the handle in its
// destructor, so the type needs no custom special members (Rule of Zero).

#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <SQLiteCpp/Database.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace copyclip::storage {

class SqliteHistoryRepository final : public core::HistoryRepository {
public:
    // Opens (creating if absent) the database at `db_path`, creating the parent
    // directory first so the open never fails on a missing dir. A pre-existing file
    // that isn't ours is moved aside first. Throws SQLite::Exception on a database
    // error, or std::filesystem::filesystem_error if a foreign database can't be
    // archived (so we never adopt and stamp a stranger's file).
    explicit SqliteHistoryRepository(const std::filesystem::path& db_path);

    void add(const core::ClipboardEntry& entry) override;
    void remove(const std::string& content) override;
    [[nodiscard]] bool set_pinned(const std::string& content, bool pinned) override;
    void clear_unpinned() override;
    [[nodiscard]] std::vector<core::ClipboardEntry> all() const override;

    // PNG bytes for an image entry, keyed by its content hash; empty if none.
    [[nodiscard]] std::vector<std::byte> image(const std::string& hash) const override;

private:
    // SQLite::Database methods aren't const (executeStep advances a cursor, exec
    // mutates), yet all() is a logical read that must stay const per the
    // interface. `mutable` lets the const path run statements without casting away
    // const (ES.50).
    mutable SQLite::Database database_;
};

} // namespace copyclip::storage
