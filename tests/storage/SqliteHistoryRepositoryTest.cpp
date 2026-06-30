// Port of the reference oracle tests/storage/test_sqlite_history.py.
//
// Exercises SqliteHistoryRepository against a real on-disk SQLite database in a
// per-test TempDir. The four reference cases (round-trip, replace-by-content,
// pin/clear, cross-instance persistence) port verbatim; one extra case guards the
// created_at ISO-8601 round-trip at sub-second precision, which the HistoryService
// ordering contract depends on and the reference does not cover.

#include "storage/SqliteHistoryRepository.hpp"
#include "core/Models.hpp"
#include "support/TempDir.hpp"

#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Database.h>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

namespace core = copyclip::core;
namespace storage = copyclip::storage;

using copyclip::testing::TempDir;

// A system_clock::time_point for a calendar date at midnight UTC, mirroring the
// reference's datetime(Y, M, D) and keeping cases free of raw chrono plumbing.
[[nodiscard]] std::chrono::system_clock::time_point date_utc(int year, unsigned month,
                                                             unsigned day) {
    const std::chrono::year_month_day ymd{std::chrono::year{year}, std::chrono::month{month},
                                          std::chrono::day{day}};
    return std::chrono::system_clock::time_point{std::chrono::sys_days{ymd}};
}

// The content strings from a snapshot, in the order returned.
[[nodiscard]] std::vector<std::string>
contents_of(const std::vector<core::ClipboardEntry>& entries) {
    std::vector<std::string> result;
    result.reserve(entries.size());
    for (const auto& entry : entries) {
        result.push_back(entry.content);
    }
    return result;
}

// Fixture: a fresh TempDir and database path per test. The repository is built
// per-case because several tests open a second instance on the same path.
class SqliteHistoryRepositoryTest : public ::testing::Test {
protected:
    [[nodiscard]] std::filesystem::path db_path() const { return temp_dir_.path() / "history.db"; }

    [[nodiscard]] storage::SqliteHistoryRepository repo() const {
        return storage::SqliteHistoryRepository{db_path()};
    }

    // The first archived (.incompatible) database copy beside the live one, if any.
    [[nodiscard]] std::filesystem::path archived_copy_path() const {
        for (const auto& entry : std::filesystem::directory_iterator(temp_dir_.path())) {
            if (entry.path().filename().string().find(".incompatible.") != std::string::npos) {
                return entry.path();
            }
        }
        return {};
    }

    [[nodiscard]] bool archived_copy_exists() const { return !archived_copy_path().empty(); }

private:
    TempDir temp_dir_;
};

// One entry round-trips with its content, unpinned by default.
TEST_F(SqliteHistoryRepositoryTest, AddThenAllRoundtrip) {
    auto repository = repo();
    repository.add(core::ClipboardEntry{
        .content = "hello", .created_at = date_utc(2026, 1, 1), .pinned = false});

    const std::vector<core::ClipboardEntry> entries = repository.all();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries[0].content, "hello");
    EXPECT_FALSE(entries[0].pinned);
}

// Re-adding the same content replaces the row (content is the primary key), so
// the history holds a single entry.
TEST_F(SqliteHistoryRepositoryTest, AddSameContentReplaces) {
    auto repository = repo();
    repository.add(core::ClipboardEntry{
        .content = "dup", .created_at = date_utc(2026, 1, 1), .pinned = false});
    repository.add(core::ClipboardEntry{
        .content = "dup", .created_at = date_utc(2026, 1, 2), .pinned = false});

    EXPECT_EQ(repository.all().size(), 1U);
}

// Pinning an existing entry succeeds, a missing one fails, and clear_unpinned
// keeps only the pinned entry.
TEST_F(SqliteHistoryRepositoryTest, SetPinnedAndClearUnpinned) {
    auto repository = repo();
    repository.add(core::ClipboardEntry{
        .content = "keep", .created_at = date_utc(2026, 1, 1), .pinned = false});
    repository.add(core::ClipboardEntry{
        .content = "drop", .created_at = date_utc(2026, 1, 2), .pinned = false});

    EXPECT_TRUE(repository.set_pinned("keep", true));
    EXPECT_FALSE(repository.set_pinned("missing", true));

    repository.clear_unpinned();
    EXPECT_EQ(contents_of(repository.all()), std::vector<std::string>{"keep"});
}

// Data written by one instance is visible to a second on the same path.
TEST_F(SqliteHistoryRepositoryTest, PersistsAcrossInstances) {
    {
        auto writer = repo();
        writer.add(core::ClipboardEntry{
            .content = "persisted", .created_at = date_utc(2026, 1, 1), .pinned = false});
    }

    auto reader = repo();
    EXPECT_EQ(contents_of(reader.all()), std::vector<std::string>{"persisted"});
}

// Extra (no reference analogue): created_at survives the ISO-8601 text round-trip
// with sub-second fidelity. HistoryService orders by created_at, so a lossy
// timestamp would silently corrupt that ordering.
TEST_F(SqliteHistoryRepositoryTest, CreatedAtRoundTripsWithSubSecondPrecision) {
    const auto created_at = date_utc(2026, 1, 1) + std::chrono::hours{13} +
                            std::chrono::minutes{37} + std::chrono::seconds{42} +
                            std::chrono::microseconds{123456};

    auto repository = repo();
    repository.add(
        core::ClipboardEntry{.content = "ts", .created_at = created_at, .pinned = false});

    const std::vector<core::ClipboardEntry> entries = repository.all();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries[0].created_at, created_at);
}

// An image entry round-trips its kind + dimensions; the PNG bytes are NOT carried
// by all() (lazy), but are fetched by content hash via image().
TEST_F(SqliteHistoryRepositoryTest, ImageEntryRoundTripsLazily) {
    const std::vector<std::byte> png{std::byte{0x89}, std::byte{0x50}, std::byte{0x4E},
                                     std::byte{0x47}};
    auto repository = repo();
    repository.add(core::ClipboardEntry{.kind = core::ClipKind::Image,
                                        .content = "imghash",
                                        .image = png,
                                        .image_width = 800,
                                        .image_height = 600,
                                        .created_at = date_utc(2026, 1, 1)});

    const std::vector<core::ClipboardEntry> entries = repository.all();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries[0].kind, core::ClipKind::Image);
    EXPECT_EQ(entries[0].image_width, 800);
    EXPECT_EQ(entries[0].image_height, 600);
    EXPECT_TRUE(entries[0].image.empty()); // lazy: not loaded by all()
    EXPECT_EQ(repository.image("imghash"), png);
    EXPECT_TRUE(repository.image("missing").empty());
}

// Removing an image entry drops its blob from the images table.
TEST_F(SqliteHistoryRepositoryTest, RemovingImageEntryDropsBlob) {
    const std::vector<std::byte> png{std::byte{0x01}, std::byte{0x02}};
    auto repository = repo();
    repository.add(core::ClipboardEntry{.kind = core::ClipKind::Image,
                                        .content = "h1",
                                        .image = png,
                                        .created_at = date_utc(2026, 1, 1)});
    repository.remove("h1");
    EXPECT_TRUE(repository.image("h1").empty());
}

// A rich-text entry round-trips its kind and HTML markup.
TEST_F(SqliteHistoryRepositoryTest, RichTextEntryRoundTrips) {
    auto repository = repo();
    repository.add(core::ClipboardEntry{.kind = core::ClipKind::RichText,
                                        .content = "bold text",
                                        .html = "<b>bold text</b>",
                                        .created_at = date_utc(2026, 1, 1)});

    const std::vector<core::ClipboardEntry> entries = repository.all();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries[0].kind, core::ClipKind::RichText);
    EXPECT_EQ(entries[0].html, "<b>bold text</b>");
}

// A database created before the rich-content columns existed is migrated on open;
// its legacy rows read back as plain text and keep their pin.
TEST_F(SqliteHistoryRepositoryTest, MigratesPreRichContentSchema) {
    {
        SQLite::Database old{db_path(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
        old.exec("CREATE TABLE entries (content TEXT PRIMARY KEY, created_at TEXT NOT NULL, "
                 "pinned INTEGER NOT NULL DEFAULT 0)");
        old.exec("INSERT INTO entries (content, created_at, pinned) "
                 "VALUES ('legacy', '2026-01-01T00:00:00', 1)");
    }
    auto repository = repo(); // opens and migrates the schema

    const std::vector<core::ClipboardEntry> entries = repository.all();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries[0].content, "legacy");
    EXPECT_EQ(entries[0].kind, core::ClipKind::Text);
    EXPECT_TRUE(entries[0].pinned);
}

// clear_unpinned drops the blobs of unpinned image entries but preserves a pinned
// image's blob and row (guarding the WHERE pinned = 0 subquery and its ordering).
TEST_F(SqliteHistoryRepositoryTest, ClearUnpinnedDropsUnpinnedImageBlobsKeepsPinned) {
    const std::vector<std::byte> keep{std::byte{0x0A}, std::byte{0x0B}};
    const std::vector<std::byte> drop{std::byte{0x0C}, std::byte{0x0D}};
    auto repository = repo();
    repository.add(core::ClipboardEntry{.kind = core::ClipKind::Image,
                                        .content = "keep",
                                        .image = keep,
                                        .created_at = date_utc(2026, 1, 1),
                                        .pinned = true});
    repository.add(core::ClipboardEntry{.kind = core::ClipKind::Image,
                                        .content = "drop",
                                        .image = drop,
                                        .created_at = date_utc(2026, 1, 2),
                                        .pinned = false});
    repository.clear_unpinned();

    EXPECT_TRUE(repository.image("drop").empty()); // orphan blob removed
    EXPECT_EQ(repository.image("keep"), keep);     // pinned blob preserved
    const std::vector<core::ClipboardEntry> entries = repository.all();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries.front().content, "keep");
}

// A corrupt row (bad timestamp or unknown kind) is skipped, not thrown on, so a
// single bad row can't make the whole history unreadable.
TEST_F(SqliteHistoryRepositoryTest, SkipsMalformedRowsInsteadOfThrowing) {
    {
        SQLite::Database old{db_path(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
        old.exec(
            "CREATE TABLE entries (content TEXT PRIMARY KEY, kind TEXT NOT NULL DEFAULT 'text', "
            "html TEXT NOT NULL DEFAULT '', image_width INTEGER NOT NULL DEFAULT 0, "
            "image_height INTEGER NOT NULL DEFAULT 0, created_at TEXT NOT NULL, "
            "pinned INTEGER NOT NULL DEFAULT 0)");
        old.exec("INSERT INTO entries (content, kind, created_at) "
                 "VALUES ('good', 'text', '2026-01-01T00:00:00')");
        old.exec("INSERT INTO entries (content, kind, created_at) "
                 "VALUES ('badtime', 'text', 'not-a-date')");
        old.exec("INSERT INTO entries (content, kind, created_at) "
                 "VALUES ('badkind', 'martian', '2026-01-01T00:00:00')");
    }
    auto repository = repo();

    const std::vector<core::ClipboardEntry> entries = repository.all(); // never throws
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries.front().content, "good");
}

// Reopening our own database keeps its data and never trips the archiver -- the
// concurrent-open case the WAL/busy-timeout support exists for.
TEST_F(SqliteHistoryRepositoryTest, ReopeningOwnDatabaseKeepsDataAndDoesNotArchive) {
    {
        auto repository = repo();
        repository.add(core::ClipboardEntry{
            .content = "mine", .created_at = date_utc(2026, 1, 1), .pinned = false});
    }
    auto reopened = repo();
    const std::vector<core::ClipboardEntry> entries = reopened.all();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries.front().content, "mine");
    EXPECT_FALSE(archived_copy_exists());
}

// A database from before the owner marker existed (only the `entries` table, no
// `metadata`) is migrated in place, never archived -- guards against wiping a real
// user's history on upgrade.
TEST_F(SqliteHistoryRepositoryTest, LegacyDatabaseWithoutMarkerIsPreserved) {
    {
        SQLite::Database old{db_path(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
        old.exec("CREATE TABLE entries (content TEXT PRIMARY KEY, created_at TEXT NOT NULL, "
                 "pinned INTEGER NOT NULL DEFAULT 0)");
        old.exec("INSERT INTO entries (content, created_at) "
                 "VALUES ('legacy', '2026-01-01T00:00:00')");
    }

    auto repository = repo();
    const std::vector<core::ClipboardEntry> entries = repository.all();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries.front().content, "legacy");
    EXPECT_FALSE(archived_copy_exists());
}

// A genuinely foreign database (real tables, but none of ours and no owner marker)
// is moved aside -- non-destructively -- so CopyClip starts on a clean, owned database.
TEST_F(SqliteHistoryRepositoryTest, ForeignDatabaseIsArchived) {
    {
        SQLite::Database foreign{db_path(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
        foreign.exec("CREATE TABLE bookmarks (id INTEGER PRIMARY KEY, url TEXT NOT NULL)");
        foreign.exec("INSERT INTO bookmarks (url) VALUES ('https://example.com')");
    }

    auto repository = repo();
    EXPECT_TRUE(repository.all().empty());
    EXPECT_TRUE(std::filesystem::exists(db_path()));

    // The foreign data survived in the archived copy (archiving is non-destructive).
    const std::filesystem::path archived = archived_copy_path();
    ASSERT_FALSE(archived.empty());
    SQLite::Database moved{archived.string(), SQLite::OPEN_READONLY};
    EXPECT_EQ(moved.execAndGet("SELECT COUNT(*) FROM bookmarks").getInt(), 1);

    // The fresh database in its place is stamped as ours.
    SQLite::Database fresh{db_path().string(), SQLite::OPEN_READONLY};
    EXPECT_EQ(fresh.execAndGet("SELECT value FROM metadata WHERE key = 'owner'").getString(),
              "copyclip");
}

// A database explicitly stamped with a different owner is archived and replaced.
TEST_F(SqliteHistoryRepositoryTest, DatabaseWithWrongOwnerIsArchived) {
    {
        SQLite::Database old{db_path(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE};
        old.exec("CREATE TABLE metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL)");
        old.exec("INSERT INTO metadata (key, value) VALUES ('owner', 'other-app')");
    }

    auto repository = repo();
    EXPECT_TRUE(repository.all().empty());
    EXPECT_TRUE(archived_copy_exists());
}

// An empty pre-existing database (no user tables) -- e.g. a sibling instance's file
// caught mid-initialization -- is adopted, not archived, so a racing first run can't
// lose its captures.
TEST_F(SqliteHistoryRepositoryTest, EmptyDatabaseIsAdoptedNotArchived) {
    { SQLite::Database empty{db_path(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE}; }

    auto repository = repo();
    EXPECT_TRUE(repository.all().empty());
    EXPECT_FALSE(archived_copy_exists());
}

// A file at the database path that is not a SQLite database is archived aside, so the
// app starts on a fresh database instead of failing to open on every launch.
TEST_F(SqliteHistoryRepositoryTest, CorruptFileIsArchivedAndAppStartsFresh) {
    {
        std::ofstream out{db_path(), std::ios::binary};
        out << "this is definitely not a sqlite database";
    }

    auto repository = repo();
    EXPECT_TRUE(repository.all().empty());
    EXPECT_TRUE(archived_copy_exists());
}

} // namespace
