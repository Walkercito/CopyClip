#include "storage/SqliteHistoryRepository.hpp"

#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include <spdlog/spdlog.h>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace copyclip::storage {

namespace {

// Named so no bare SQL literal appears in the logic. `content` is the primary
// key (the plain text, or the image content hash), so INSERT OR REPLACE upserts
// by content; pinned is stored as 0/1; kind is stored as its enum string. Image
// bytes live in a separate `images` table keyed by the same content hash, read
// lazily so the history list query never carries blobs.
constexpr const char* kSchema = "CREATE TABLE IF NOT EXISTS entries ("
                                "content      TEXT PRIMARY KEY, "
                                "kind         TEXT NOT NULL DEFAULT 'text', "
                                "html         TEXT NOT NULL DEFAULT '', "
                                "image_width  INTEGER NOT NULL DEFAULT 0, "
                                "image_height INTEGER NOT NULL DEFAULT 0, "
                                "created_at   TEXT NOT NULL, "
                                "pinned       INTEGER NOT NULL DEFAULT 0)";
constexpr const char* kSchemaImages =
    "CREATE TABLE IF NOT EXISTS images (hash TEXT PRIMARY KEY, png BLOB NOT NULL)";

constexpr const char* kInsertOrReplace =
    "INSERT OR REPLACE INTO entries "
    "(content, kind, html, image_width, image_height, created_at, pinned) "
    "VALUES (?, ?, ?, ?, ?, ?, ?)";
constexpr const char* kInsertImage = "INSERT OR REPLACE INTO images (hash, png) VALUES (?, ?)";
constexpr const char* kSelectImage = "SELECT png FROM images WHERE hash = ?";
constexpr const char* kDeleteByContent = "DELETE FROM entries WHERE content = ?";
constexpr const char* kDeleteImageByHash = "DELETE FROM images WHERE hash = ?";
constexpr const char* kUpdatePinned = "UPDATE entries SET pinned = ? WHERE content = ?";
constexpr const char* kDeleteUnpinned = "DELETE FROM entries WHERE pinned = 0";
// Run before kDeleteUnpinned: it still needs the rows it references.
constexpr const char* kDeleteUnpinnedImages =
    "DELETE FROM images WHERE hash IN (SELECT content FROM entries WHERE pinned = 0)";
constexpr const char* kSelectAll =
    "SELECT content, kind, html, image_width, image_height, created_at, pinned FROM entries";

// Column ordinals for the kSelectAll projection, named to avoid bare indices.
constexpr int kColContent = 0;
constexpr int kColKind = 1;
constexpr int kColHtml = 2;
constexpr int kColImageWidth = 3;
constexpr int kColImageHeight = 4;
constexpr int kColCreatedAt = 5;
constexpr int kColPinned = 6;
constexpr int kColImagePng = 0;      // kSelectImage projection
constexpr int kPragmaNameColumn = 1; // PRAGMA table_info: column 1 is the name

// Bind-parameter ordinals (1-based, as SQLite numbers them).
constexpr int kParamFirst = 1;
constexpr int kParamSecond = 2;
constexpr int kParamThird = 3;
constexpr int kParamFourth = 4;
constexpr int kParamFifth = 5;
constexpr int kParamSixth = 6;
constexpr int kParamSeventh = 7;

// Pinned is persisted as a SQLite INTEGER: 1 for pinned, 0 otherwise.
constexpr int kPinnedTrue = 1;
constexpr int kPinnedFalse = 0;

// created_at round-trips as ISO-8601 TEXT, mirroring the reference's
// isoformat()/fromisoformat(). std::format "{:%FT%T}" emits the full nanosecond
// subsecond field, so the round-trip is lossless. libstdc++ 13 lacks
// std::chrono::from_stream for sys_time, so reading back uses a hand-rolled
// parser: locale-independent, -Wconversion-safe, and reporting failure via
// std::optional rather than silently accepting garbage.

// Number of decimal digits std::format emits for the nanosecond subsecond field.
constexpr std::size_t kSubsecondDigits = 9;

// Field widths in the canonical "YYYY-MM-DDTHH:MM:SS" layout.
constexpr std::size_t kYearDigits = 4;
constexpr std::size_t kTwoDigits = 2;

[[nodiscard]] std::string format_iso8601(std::chrono::system_clock::time_point moment) {
    return std::format("{:%FT%T}", moment);
}

// Decimal radix for the digit accumulation below.
constexpr long long kRadix = 10;

// Parses exactly `count` decimal digits at `pos`, advancing `pos` on success;
// std::nullopt on too few characters or a non-digit. Locale-independent by
// accumulating digit by digit.
[[nodiscard]] std::optional<long long> parse_digits(std::string_view text, std::size_t& pos,
                                                    std::size_t count) {
    if (count > text.size() - pos) {
        return std::nullopt;
    }
    long long value = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const char ch = text[pos + i];
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }
        value = value * kRadix + (ch - '0');
    }
    pos += count;
    return value;
}

// Consume the single character `expected` at `pos`, advancing on success.
[[nodiscard]] bool consume(std::string_view text, std::size_t& pos, char expected) {
    if (pos >= text.size() || text[pos] != expected) {
        return false;
    }
    ++pos;
    return true;
}

// Parses "YYYY-MM-DDTHH:MM:SS[.fffffffff]" back into a time_point. The fractional
// part is optional and right-padded to nanoseconds so any emitted precision
// round-trips. std::nullopt on any malformed field.
[[nodiscard]] std::optional<std::chrono::system_clock::time_point>
parse_iso8601(std::string_view text) {
    std::size_t pos = 0;

    const std::optional<long long> year = parse_digits(text, pos, kYearDigits);
    if (!year || !consume(text, pos, '-')) {
        return std::nullopt;
    }
    const std::optional<long long> month = parse_digits(text, pos, kTwoDigits);
    if (!month || !consume(text, pos, '-')) {
        return std::nullopt;
    }
    const std::optional<long long> day = parse_digits(text, pos, kTwoDigits);
    if (!day || !consume(text, pos, 'T')) {
        return std::nullopt;
    }
    const std::optional<long long> hour = parse_digits(text, pos, kTwoDigits);
    if (!hour || !consume(text, pos, ':')) {
        return std::nullopt;
    }
    const std::optional<long long> minute = parse_digits(text, pos, kTwoDigits);
    if (!minute || !consume(text, pos, ':')) {
        return std::nullopt;
    }
    const std::optional<long long> second = parse_digits(text, pos, kTwoDigits);
    if (!second) {
        return std::nullopt;
    }

    long long nanos = 0;
    if (pos < text.size() && text[pos] == '.') {
        ++pos;
        std::size_t digits = 0;
        while (pos < text.size() && digits < kSubsecondDigits && text[pos] >= '0' &&
               text[pos] <= '9') {
            nanos = nanos * kRadix + (text[pos] - '0');
            ++pos;
            ++digits;
        }
        for (std::size_t i = digits; i < kSubsecondDigits; ++i) {
            nanos *= kRadix;
        }
    }
    if (pos != text.size()) {
        return std::nullopt;
    }

    const std::chrono::year_month_day ymd{std::chrono::year{static_cast<int>(*year)},
                                          std::chrono::month{static_cast<unsigned>(*month)},
                                          std::chrono::day{static_cast<unsigned>(*day)}};
    if (!ymd.ok()) {
        return std::nullopt;
    }

    const auto whole = std::chrono::sys_time<std::chrono::nanoseconds>{std::chrono::sys_days{ymd}} +
                       std::chrono::hours{*hour} + std::chrono::minutes{*minute} +
                       std::chrono::seconds{*second} + std::chrono::nanoseconds{nanos};
    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(whole);
}

// Whether the `entries` table already has `column` — drives the ADD COLUMN
// migration so a DB created before the rich-content columns existed is upgraded.
[[nodiscard]] bool has_column(SQLite::Database& database, std::string_view column) {
    SQLite::Statement statement{database, "PRAGMA table_info(entries)"};
    while (statement.executeStep()) {
        if (statement.getColumn(kPragmaNameColumn).getString() == column) {
            return true;
        }
    }
    return false;
}

// Add `column` to `entries` when missing. `column`/`declaration` are fixed
// literals from the schema, never caller input, so the concatenation is safe.
void add_column_if_missing(SQLite::Database& database, const char* column,
                           const char* declaration) {
    if (!has_column(database, column)) {
        database.exec(std::string{"ALTER TABLE entries ADD COLUMN "} + column + " " + declaration);
    }
}

} // namespace

std::filesystem::path SqliteHistoryRepository::ensure_parent(const std::filesystem::path& db_path) {
    const std::filesystem::path parent = db_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    return db_path;
}

SqliteHistoryRepository::SqliteHistoryRepository(const std::filesystem::path& db_path)
    : database_{ensure_parent(db_path), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE} {
    database_.exec(kSchema);
    database_.exec(kSchemaImages);
    // Upgrade a DB created before rich-text/image support added these columns.
    add_column_if_missing(database_, "kind", "TEXT NOT NULL DEFAULT 'text'");
    add_column_if_missing(database_, "html", "TEXT NOT NULL DEFAULT ''");
    add_column_if_missing(database_, "image_width", "INTEGER NOT NULL DEFAULT 0");
    add_column_if_missing(database_, "image_height", "INTEGER NOT NULL DEFAULT 0");
}

void SqliteHistoryRepository::add(const core::ClipboardEntry& entry) {
    SQLite::Statement statement{database_, kInsertOrReplace};
    statement.bind(kParamFirst, entry.content);
    statement.bind(kParamSecond, std::string{core::to_string(entry.kind)});
    statement.bind(kParamThird, entry.html);
    statement.bind(kParamFourth, entry.image_width);
    statement.bind(kParamFifth, entry.image_height);
    statement.bind(kParamSixth, format_iso8601(entry.created_at));
    statement.bind(kParamSeventh, entry.pinned ? kPinnedTrue : kPinnedFalse);
    statement.exec();

    if (entry.kind == core::ClipKind::Image && !entry.image.empty()) {
        SQLite::Statement image_statement{database_, kInsertImage};
        image_statement.bind(kParamFirst, entry.content);
        image_statement.bind(kParamSecond, entry.image.data(),
                             static_cast<int>(entry.image.size()));
        image_statement.exec();
    }
}

void SqliteHistoryRepository::remove(const std::string& content) {
    SQLite::Statement statement{database_, kDeleteByContent};
    statement.bind(kParamFirst, content);
    statement.exec();
    // Drop any image blob keyed by this content (a no-op for text entries).
    SQLite::Statement image_statement{database_, kDeleteImageByHash};
    image_statement.bind(kParamFirst, content);
    image_statement.exec();
}

bool SqliteHistoryRepository::set_pinned(const std::string& content, bool pinned) {
    SQLite::Statement statement{database_, kUpdatePinned};
    statement.bind(kParamFirst, pinned ? kPinnedTrue : kPinnedFalse);
    statement.bind(kParamSecond, content);
    return statement.exec() > 0;
}

void SqliteHistoryRepository::clear_unpinned() {
    database_.exec(kDeleteUnpinnedImages); // before the entries it references are gone
    SQLite::Statement statement{database_, kDeleteUnpinned};
    statement.exec();
}

std::vector<core::ClipboardEntry> SqliteHistoryRepository::all() const {
    std::vector<core::ClipboardEntry> entries;
    SQLite::Statement statement{database_, kSelectAll};
    while (statement.executeStep()) {
        const std::string created_at_text = statement.getColumn(kColCreatedAt).getString();
        const std::optional<std::chrono::system_clock::time_point> created_at =
            parse_iso8601(created_at_text);
        const std::string kind_text = statement.getColumn(kColKind).getString();
        const std::optional<core::ClipKind> kind = core::clip_kind_from_string(kind_text);
        if (!created_at || !kind) {
            // One corrupt row (a manual edit, or a kind written by a newer version)
            // must not make the whole history unreadable — skip it, leaving a trail.
            spdlog::warn("skipping malformed history row: created_at='{}' kind='{}'",
                         created_at_text, kind_text);
            continue;
        }
        // Image bytes stay lazy: left empty here, fetched on demand via image().
        entries.push_back(core::ClipboardEntry{
            .kind = *kind,
            .content = statement.getColumn(kColContent).getString(),
            .html = statement.getColumn(kColHtml).getString(),
            .image_width = statement.getColumn(kColImageWidth).getInt(),
            .image_height = statement.getColumn(kColImageHeight).getInt(),
            .created_at = *created_at,
            .pinned = statement.getColumn(kColPinned).getInt() != kPinnedFalse});
    }
    return entries;
}

std::vector<std::byte> SqliteHistoryRepository::image(const std::string& hash) const {
    SQLite::Statement statement{database_, kSelectImage};
    statement.bind(kParamFirst, hash);
    if (!statement.executeStep()) {
        return {};
    }
    const SQLite::Column column = statement.getColumn(kColImagePng);
    const int size = column.getBytes();
    if (size <= 0) {
        return {};
    }
    const auto* bytes = static_cast<const std::byte*>(column.getBlob());
    const std::span<const std::byte> view{bytes, static_cast<std::size_t>(size)};
    return std::vector<std::byte>{view.begin(), view.end()};
}

} // namespace copyclip::storage
