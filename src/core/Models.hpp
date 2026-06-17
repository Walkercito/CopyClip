#pragma once

// Immutable value objects for the engine's domain.
//
// Mirrors the reference module copyclip/core/models.py, where each type is a
// @dataclass(frozen=True): a fixed set of fields with documented defaults.
// Python's frozen=True raises on attribute assignment; that runtime guard has
// no compile-time C++ equivalent, so these are plain value types — fully
// copyable/movable (Rule of Zero) so they can live in std::vector and be sorted
// — and immutability is a convention, not an enforced invariant. Members are
// deliberately public (entries are reconstructed, never mutated in place) and
// non-const, since const members would suppress the move/copy these containers
// need (C.12).
//
// HotkeySpec::display_name reproduces the reference property exactly: each
// modifier's string value is Title-cased and the key's string value is fully
// upper-cased, joined by '+'. It is built from to_string(Modifier)/to_string(Key)
// (core/Enums.hpp) plus small ASCII case helpers, so the enum<->string mapping
// is never duplicated. Pure, header-only, and free of Qt/Xlib/D-Bus: this lives
// in the core layer.

#include "config/Constants.hpp"
#include "core/Enums.hpp"

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

namespace copyclip::core {

// A single captured clipboard item. `content` is the text; `created_at` is when
// it entered the history; `pinned` marks items kept across eviction.
struct ClipboardEntry {
    std::string content;
    std::chrono::system_clock::time_point created_at;
    bool pinned = false;
};

namespace detail {

// ASCII letter case folding. Locale-independent and -Wconversion-safe: it works
// on char throughout, avoiding the int round-trip of std::toupper/std::tolower
// (which also require unsigned-char casts to be well-defined). Non-letters pass
// through unchanged.
inline constexpr char kAsciiCaseGap = 'a' - 'A';

[[nodiscard]] constexpr char ascii_to_upper(char ch) {
    return (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - kAsciiCaseGap) : ch;
}

[[nodiscard]] constexpr char ascii_to_lower(char ch) {
    return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch + kAsciiCaseGap) : ch;
}

// Fully upper-cases an ASCII view (the key half of display_name: "space" ->
// "SPACE").
[[nodiscard]] inline std::string ascii_upper(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const char ch : text) {
        result.push_back(ascii_to_upper(ch));
    }
    return result;
}

// Title-cases an ASCII view: first character upper, the rest lower (mirrors
// Python's str.capitalize() for the modifier half: "ctrl" -> "Ctrl"). An empty
// view yields an empty string.
[[nodiscard]] inline std::string ascii_capitalize(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    bool first = true;
    for (const char ch : text) {
        result.push_back(first ? ascii_to_upper(ch) : ascii_to_lower(ch));
        first = false;
    }
    return result;
}

} // namespace detail

// A hotkey: an ordered set of modifiers plus a single key. display_name() is the
// human-readable label, e.g. {Ctrl, Alt} + V -> "Ctrl+Alt+V".
struct HotkeySpec {
    std::vector<Modifier> modifiers;
    Key key;

    [[nodiscard]] std::string display_name() const {
        static constexpr std::string_view kSeparator = "+";
        std::string result;
        for (const Modifier modifier : modifiers) {
            result += detail::ascii_capitalize(to_string(modifier));
            result += kSeparator;
        }
        result += detail::ascii_upper(to_string(key));
        return result;
    }
};

// User-configurable application settings, with the reference defaults.
struct Settings {
    Theme theme = Theme::Dark;
    HotkeyPreset hotkey = HotkeyPreset::SuperV;
    bool first_run_completed = false;
    int max_history_items = config::kDefaultMaxHistoryItems;
    bool auto_hide_on_copy = true;
};

} // namespace copyclip::core
