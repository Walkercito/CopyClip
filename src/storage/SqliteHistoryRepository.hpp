#pragma once

// SQLite-backed clipboard history store.
//
// Concrete copyclip::core::HistoryRepository persisting entries to a SQLite
// database file, mirroring the reference module copyclip/storage/sqlite_history.py
// behavior exactly. The connection is owned by RAII via SQLiteCpp's
// SQLite::Database (a wrapper over sqlite3 that closes the handle in its
// destructor), so this type needs no custom special members beyond what the move
// semantics of its members already provide (Rule of Zero).
//
// Dependency rule (CLAUDE.md): the storage layer depends only on core/ and
// SQLiteCpp/sqlite3 — never on Qt, Xlib, or D-Bus.

#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <SQLiteCpp/Database.h>

#include <filesystem>
#include <string>
#include <vector>

namespace copyclip::storage {

class SqliteHistoryRepository final : public core::HistoryRepository {
public:
    // Open (creating if absent) the database at `db_path`. The parent directory
    // is created first so opening never fails on a missing directory, then the
    // schema is ensured. Throws SQLite::Exception on a database error.
    explicit SqliteHistoryRepository(const std::filesystem::path& db_path);

    void add(const core::ClipboardEntry& entry) override;
    void remove(const std::string& content) override;
    [[nodiscard]] bool set_pinned(const std::string& content, bool pinned) override;
    void clear_unpinned() override;
    [[nodiscard]] std::vector<core::ClipboardEntry> all() const override;

private:
    // Create the parent directory of `db_path` (if any) and return `db_path`
    // unchanged. Runs in the member initializer list before `database_` is
    // constructed, so the file's directory always exists by the time SQLite opens
    // it — matching the reference's `db_path.parent.mkdir(...)`.
    [[nodiscard]] static std::filesystem::path ensure_parent(const std::filesystem::path& db_path);

    // SQLite query and execution are not const-qualified on SQLite::Database
    // (executeStep advances a cursor, exec mutates), yet all() is a logical read
    // and must stay const per the interface. `mutable` lets the const read path
    // run statements without casting away const (ES.50).
    mutable SQLite::Database database_;
};

} // namespace copyclip::storage
