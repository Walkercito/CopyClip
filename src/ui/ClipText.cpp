#include "ui/ClipText.hpp"

#include <string>

namespace copyclip::ui {

namespace {

// The single ellipsis character (U+2026) appended to a truncated preview.
constexpr std::string_view kEllipsis{"…"};

// Bytes in the UTF-8 sequence introduced by `lead`. An invalid lead byte counts
// as one byte so a malformed string still advances and terminates.
[[nodiscard]] std::size_t sequence_length(unsigned char lead) {
    if (lead < 0x80) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3;
    }
    if ((lead & 0xF8) == 0xF0) {
        return 4;
    }
    return 1;
}

[[nodiscard]] bool is_ascii_whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v';
}

// Structural whitespace makes the flattened preview hide something (line breaks,
// tab layout), so the card should offer to expand even when it fits the limit.
[[nodiscard]] bool is_structural_whitespace(char ch) {
    return ch == '\n' || ch == '\r' || ch == '\t';
}

} // namespace

Preview make_preview(std::string_view content, std::size_t max_code_points) {
    // Flatten to a single line: collapse every run of ASCII whitespace to one
    // space and trim the ends. Multi-byte UTF-8 bytes are never ASCII whitespace,
    // so they pass through untouched.
    std::string flat;
    flat.reserve(content.size());
    bool has_structure = false;
    bool pending_space = false;
    for (const char ch : content) {
        if (is_ascii_whitespace(ch)) {
            has_structure = has_structure || is_structural_whitespace(ch);
            pending_space = !flat.empty(); // trim leading; mark an internal gap
            continue;
        }
        if (pending_space) {
            flat.push_back(' ');
            pending_space = false;
        }
        flat.push_back(ch);
    }

    if (max_code_points == 0) {
        return {flat, has_structure};
    }

    std::size_t byte = 0;
    std::size_t code_points = 0;
    while (byte < flat.size() && code_points < max_code_points) {
        byte += sequence_length(static_cast<unsigned char>(flat[byte]));
        ++code_points;
    }

    const bool length_truncated = byte < flat.size();
    std::string text = length_truncated ? flat.substr(0, byte) + std::string{kEllipsis} : flat;
    return {std::move(text), length_truncated || has_structure};
}

} // namespace copyclip::ui
