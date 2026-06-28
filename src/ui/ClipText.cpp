#include "ui/ClipText.hpp"

#include <utility>

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

} // namespace

Preview make_preview(std::string_view content, std::size_t max_code_points) {
    if (max_code_points == 0) {
        return {std::string{content}, false};
    }

    std::size_t byte = 0;
    std::size_t code_points = 0;
    while (byte < content.size() && code_points < max_code_points) {
        byte += sequence_length(static_cast<unsigned char>(content[byte]));
        ++code_points;
    }

    if (byte >= content.size()) {
        return {std::string{content}, false};
    }

    std::string text{content.substr(0, byte)};
    text += kEllipsis;
    return {std::move(text), true};
}

} // namespace copyclip::ui
