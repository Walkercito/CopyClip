#pragma once

// Locale-independent ASCII case folding. Works on char throughout — avoiding
// the int round-trip and unsigned-char casts std::toupper/tolower require, and
// staying -Wconversion-safe. Only A-Z/a-z fold; every other byte passes through.

#include <string>
#include <string_view>

namespace copyclip::core::detail {

inline constexpr char kAsciiCaseGap = 'a' - 'A';

[[nodiscard]] constexpr char ascii_to_upper(char ch) {
    return (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - kAsciiCaseGap) : ch;
}

[[nodiscard]] constexpr char ascii_to_lower(char ch) {
    return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch + kAsciiCaseGap) : ch;
}

// Upper-cases an ASCII view (display_name's key half: "space" -> "SPACE").
[[nodiscard]] inline std::string ascii_upper(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const char ch : text) {
        result.push_back(ascii_to_upper(ch));
    }
    return result;
}

// Lower-cases an ASCII view (matches Python str.lower() for ASCII input).
[[nodiscard]] inline std::string ascii_lower(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const char ch : text) {
        result.push_back(ascii_to_lower(ch));
    }
    return result;
}

// Title-cases an ASCII view: first char upper, rest lower (Python
// str.capitalize(), display_name's modifier half: "ctrl" -> "Ctrl").
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
