#include "storage/SqliteHistoryRepository.hpp"

#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace copyclip::storage {

namespace {

// --- SQL --------------------------------------------------------------------
// The schema and statements, named so no bare SQL literal appears in the logic.
// `content` is the primary key (INSERT OR REPLACE upserts by content); pinned is
// stored as 0/1. Mirrors the reference module's _SCHEMA and statements verbatim.
constexpr const char* kSchema = "CREATE TABLE IF NOT EXISTS entries ("
                                "content    TEXT PRIMARY KEY, "
                                "created_at TEXT NOT NULL, "
                                "pinned     INTEGER NOT NULL DEFAULT 0)";

constexpr const char* kInsertOrReplace =
    "INSERT OR REPLACE INTO entries (content, created_at, pinned) VALUES (?, ?, ?)";
constexpr const char* kDeleteByContent = "DELETE FROM entries WHERE content = ?";
constexpr const char* kUpdatePinned = "UPDATE entries SET pinned = ? WHERE content = ?";
constexpr const char* kDeleteUnpinned = "DELETE FROM entries WHERE pinned = 0";
constexpr const char* kSelectAll = "SELECT content, created_at, pinned FROM entries";

// Column ordinals for the kSelectAll projection, named to avoid bare indices.
constexpr int kColContent = 0;
constexpr int kColCreatedAt = 1;
constexpr int kColPinned = 2;

// Bind-parameter ordinals (1-based, as SQLite numbers them).
constexpr int kParamFirst = 1;
constexpr int kParamSecond = 2;
constexpr int kParamThird = 3;

// Pinned is persisted as a SQLite INTEGER: 1 for pinned, 0 otherwise.
constexpr int kPinnedTrue = 1;
constexpr int kPinnedFalse = 0;

// --- ISO-8601 timestamp round-trip ------------------------------------------
// created_at is a system_clock::time_point stored as ISO-8601 TEXT, mirroring the
// reference's datetime.isoformat()/fromisoformat(). system_clock on this
// toolchain (GCC 13 / libstdc++) ticks in nanoseconds, and std::format with
// "{:%FT%T}" emits the full sub-second field, so the text carries every tick:
// the round-trip is exact.
//
// Reading back must reconstruct that exact time_point. libstdc++ 13 does not yet
// implement std::chrono::from_stream/parse for sys_time (the headers mark it
// "// TODO"), so a small hand-rolled ISO-8601 parser is used instead. It is
// locale-independent, -Wconversion-safe, and reports failure via std::optional so
// the caller can raise rather than silently accepting garbage.

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

// Parse exactly `count` decimal digits starting at `pos`, advancing `pos` on
// success. Accumulates digit by digit (no pointer arithmetic, locale-independent)
// and returns std::nullopt if there are too few characters or a non-digit is
// encountered.
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

// Parse "YYYY-MM-DDTHH:MM:SS[.fffffffff]" back into a system_clock::time_point.
// The fractional part is optional and right-padded to nanoseconds so any emitted
// precision round-trips. Returns std::nullopt on any malformed field.
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
}

void SqliteHistoryRepository::add(const core::ClipboardEntry& entry) {
    SQLite::Statement statement{database_, kInsertOrReplace};
    statement.bind(kParamFirst, entry.content);
    statement.bind(kParamSecond, format_iso8601(entry.created_at));
    statement.bind(kParamThird, entry.pinned ? kPinnedTrue : kPinnedFalse);
    statement.exec();
}

void SqliteHistoryRepository::remove(const std::string& content) {
    SQLite::Statement statement{database_, kDeleteByContent};
    statement.bind(kParamFirst, content);
    statement.exec();
}

bool SqliteHistoryRepository::set_pinned(const std::string& content, bool pinned) {
    SQLite::Statement statement{database_, kUpdatePinned};
    statement.bind(kParamFirst, pinned ? kPinnedTrue : kPinnedFalse);
    statement.bind(kParamSecond, content);
    return statement.exec() > 0;
}

void SqliteHistoryRepository::clear_unpinned() {
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
        if (!created_at) {
            throw std::runtime_error{"SqliteHistoryRepository: malformed created_at timestamp '" +
                                     created_at_text + "'"};
        }
        entries.push_back(core::ClipboardEntry{
            .content = statement.getColumn(kColContent).getString(),
            .created_at = *created_at,
            .pinned = statement.getColumn(kColPinned).getInt() != kPinnedFalse});
    }
    return entries;
}

} // namespace copyclip::storage
