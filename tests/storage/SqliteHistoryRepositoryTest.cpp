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

#include <chrono>
#include <cstddef>
#include <filesystem>
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

} // namespace
