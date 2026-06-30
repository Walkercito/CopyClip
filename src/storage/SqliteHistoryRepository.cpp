#include "storage/SqliteHistoryRepository.hpp"

#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include <sqlite3.h>

#include <spdlog/spdlog.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
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
constexpr const char* kSchemaMetadata =
    "CREATE TABLE IF NOT EXISTS metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL)";

constexpr const char* kMetadataOwnerKey = "owner";
constexpr const char* kMetadataOwnerValue = "copyclip";
constexpr const char* kInsertOwner = "INSERT OR REPLACE INTO metadata (key, value) VALUES (?, ?)";
constexpr const char* kSelectOwner = "SELECT value FROM metadata WHERE key = ?";
constexpr const char* kCheckTable = "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ?";
// Any user (non-internal) table exists -- distinguishes an empty database from a
// foreign one that already holds data.
constexpr const char* kAnyUserTable =
    "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%' LIMIT 1";
constexpr const char* kTableEntries = "entries";
constexpr const char* kTableMetadata = "metadata";

// Suffix (with a UTC timestamp) appended when moving a foreign database aside, plus
// the SQLite sidecar files that must move with the main database file.
constexpr std::string_view kArchiveMarker = ".incompatible.";
constexpr std::array<const char*, 3> kSqliteSidecarSuffixes{"-wal", "-shm", "-journal"};

// A second process can touch the same DB (e.g. the global-shortcut launch running
// alongside a COPYCLIP_STANDALONE dev instance). WAL allows a reader and a writer
// to coexist, and the busy timeout makes a writer wait briefly for the lock rather
// than failing immediately with "database is locked".
constexpr int kBusyTimeoutMs = 3000;
constexpr const char* kPragmaWal = "PRAGMA journal_mode=WAL";

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
constexpr int kColMetadataValue = 0; // kSelectOwner projection
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

// Create db_path's parent directory if missing; returns db_path for chaining.
std::filesystem::path ensure_parent(const std::filesystem::path& db_path) {
    const std::filesystem::path parent = db_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    return db_path;
}

[[nodiscard]] bool table_exists(SQLite::Database& database, const char* name) {
    SQLite::Statement query{database, kCheckTable};
    query.bind(kParamFirst, name);
    return query.executeStep();
}

// Whether an existing file is, or looks like, a CopyClip database we should keep: it
// carries our owner marker, has the `entries` table (covering databases made before
// the marker existed, which must be migrated in place, never wiped), or has no user
// tables at all -- an empty file, including a sibling instance's database caught
// mid-initialization, which has nothing to preserve and must not be archived out
// from under a racing first run. A genuinely foreign database (real tables, none
// ours) returns false. A file that is not a database is archived; one we merely
// cannot read right now (locked, mid-write) is kept, so a transient lock never costs
// the user their history.
[[nodiscard]] bool looks_like_our_db(const std::filesystem::path& db_path) {
    try {
        SQLite::Database database{db_path.string(), SQLite::OPEN_READONLY};
        database.setBusyTimeout(kBusyTimeoutMs);
        if (table_exists(database, kTableMetadata)) {
            SQLite::Statement owner{database, kSelectOwner};
            owner.bind(kParamFirst, kMetadataOwnerKey);
            if (owner.executeStep() &&
                owner.getColumn(kColMetadataValue).getString() == kMetadataOwnerValue) {
                return true;
            }
        }
        if (table_exists(database, kTableEntries)) {
            return true;
        }
        SQLite::Statement any_table{database, kAnyUserTable};
        return !any_table.executeStep(); // no user tables: an empty file -- adopt it
    } catch (const SQLite::Exception& error) {
        if (error.getErrorCode() == SQLITE_NOTADB) {
            spdlog::warn("file at {} is not a database; archiving it aside", db_path.string());
            return false;
        }
        spdlog::warn("could not read the existing history database ({}); leaving it in place",
                     error.what());
        return true;
    }
}

// Move a foreign database aside so a fresh CopyClip database can take its place. The
// main file moves first and a failure THROWS -- we must never fall through and
// adopt/stamp a stranger's database. Its WAL/SHM/journal sidecars then move
// best-effort with the same suffix (renaming only the main file would orphan them
// and let the fresh database recover stale data from a leftover WAL). Returns
// db_path for chaining.
[[nodiscard]] std::filesystem::path archive_foreign_db(const std::filesystem::path& db_path) {
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    const std::string suffix = std::string{kArchiveMarker} + std::format("{:%Y%m%d_%H%M%S}", now);
    std::filesystem::rename(db_path, db_path.string() + suffix); // throws on failure
    for (const char* sidecar : kSqliteSidecarSuffixes) {
        const std::filesystem::path from{db_path.string() + sidecar};
        std::error_code error;
        if (std::filesystem::exists(from, error)) {
            std::filesystem::rename(from, from.string() + suffix, error);
            if (error) {
                spdlog::warn("could not archive sidecar {}: {}", from.string(), error.message());
            }
        }
    }
    spdlog::warn("history database at {} is not ours; archived it aside ({}) and starting fresh",
                 db_path.string(), suffix);
    return db_path;
}

// Ensure the parent directory exists, then move aside any pre-existing file that
// isn't ours so the constructor opens onto a clean CopyClip database.
[[nodiscard]] std::filesystem::path prepare_db_path(const std::filesystem::path& db_path) {
    ensure_parent(db_path);
    if (std::filesystem::is_regular_file(db_path) && !looks_like_our_db(db_path)) {
        return archive_foreign_db(db_path);
    }
    return db_path;
}

} // namespace

SqliteHistoryRepository::SqliteHistoryRepository(const std::filesystem::path& db_path)
    : database_{prepare_db_path(db_path), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE} {
    database_.exec(kPragmaWal);
    database_.setBusyTimeout(kBusyTimeoutMs);
    database_.exec(kSchema);
    database_.exec(kSchemaImages);
    database_.exec(kSchemaMetadata);
    SQLite::Statement statement{database_, kInsertOwner};
    statement.bind(kParamFirst, kMetadataOwnerKey);
    statement.bind(kParamSecond, kMetadataOwnerValue);
    statement.exec();
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
