#pragma once

// Locale-independent ASCII letter case folding, shared within the core layer.
//
// Works on char throughout — avoiding the int round-trip and unsigned-char casts
// that std::toupper/std::tolower require — and is -Wconversion-safe. Only A-Z /
// a-z are folded; every other byte (including non-ASCII) passes through
// unchanged. Used by HotkeySpec::display_name (Models) and detect_session
// (Platform), so the folding logic lives in exactly one place.

#include <string>
#include <string_view>

namespace copyclip::core::detail {

// Distance between an uppercase ASCII letter and its lowercase counterpart.
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

// Fully lower-cases an ASCII view, matching Python str.lower() for ASCII input.
[[nodiscard]] inline std::string ascii_lower(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const char ch : text) {
        result.push_back(ascii_to_lower(ch));
    }
    return result;
}

// Title-cases an ASCII view: first character upper, the rest lower (mirrors
// Python str.capitalize() for the modifier half: "ctrl" -> "Ctrl"). An empty
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

} // namespace copyclip::core::detail
