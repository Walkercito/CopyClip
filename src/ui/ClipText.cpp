#include "ui/ClipText.hpp"

#include <string>

namespace copyclip::ui {

namespace {

// The single ellipsis character (U+2026) appended to a truncated preview.
constexpr std::string_view kEllipsis{"…"};

// Classification of a UTF-8 lead byte: `length` is the encoded sequence length (1
// for ASCII) or 0 for an invalid lead (a stray continuation byte or 0xF8+).
// `min_code_point` is the smallest value the sequence may legally encode (used to
// reject overlong forms); `initial_bits` are the lead byte's payload bits. This is
// the single source of the UTF-8 lead-byte masks, shared by both consumers below.
struct LeadByte {
    std::size_t length;
    char32_t min_code_point;
    char32_t initial_bits;
};

[[nodiscard]] LeadByte classify_lead(unsigned char lead) {
    if (lead < 0x80U) {
        return {1, 0U, lead};
    }
    if ((lead & 0xE0U) == 0xC0U) {
        return {2, 0x80U, static_cast<char32_t>(lead & 0x1FU)};
    }
    if ((lead & 0xF0U) == 0xE0U) {
        return {3, 0x800U, static_cast<char32_t>(lead & 0x0FU)};
    }
    if ((lead & 0xF8U) == 0xF0U) {
        return {4, 0x10000U, static_cast<char32_t>(lead & 0x07U)};
    }
    return {0, 0U, 0U};
}

// Bytes in the UTF-8 sequence introduced by `lead`. An invalid lead byte counts
// as one byte so a malformed string still advances and terminates.
[[nodiscard]] std::size_t sequence_length(unsigned char lead) {
    const std::size_t length = classify_lead(lead).length;
    return length == 0 ? 1 : length;
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

std::string make_valid_utf8(std::string_view text) {
    // The UTF-8 encoding of U+FFFD (REPLACEMENT CHARACTER).
    static constexpr std::string_view kReplacement{"\xEF\xBF\xBD"};

    std::string out;
    out.reserve(text.size());
    std::size_t pos = 0;
    while (pos < text.size()) {
        const auto lead = static_cast<unsigned char>(text[pos]);
        const LeadByte info = classify_lead(lead);
        if (info.length == 1) { // plain ASCII
            out.push_back(static_cast<char>(lead));
            ++pos;
            continue;
        }
        if (info.length == 0) { // stray continuation / invalid lead (e.g. 0x89 in a PNG)
            out.append(kReplacement);
            ++pos;
            continue;
        }

        char32_t code_point = info.initial_bits;
        bool valid = pos + info.length <= text.size();
        for (std::size_t offset = 1; valid && offset < info.length; ++offset) {
            const auto continuation = static_cast<unsigned char>(text[pos + offset]);
            if ((continuation & 0xC0U) != 0x80U) {
                valid = false;
                break;
            }
            code_point = static_cast<char32_t>((code_point << 6U) | (continuation & 0x3FU));
        }
        // Reject truncated sequences, overlong encodings, UTF-16 surrogates, and
        // points past U+10FFFF; resync one byte at a time as the standard advises.
        if (!valid || code_point < info.min_code_point || code_point > 0x10FFFFU ||
            (code_point >= 0xD800U && code_point <= 0xDFFFU)) {
            out.append(kReplacement);
            ++pos;
            continue;
        }
        out.append(text.substr(pos, info.length));
        pos += info.length;
    }
    return out;
}

Preview make_preview(std::string_view content, std::size_t max_code_points) {
    // Work on a UTF-8-valid copy so truncation never splits a sequence and the
    // preview can't carry binary into a GtkLabel.
    const std::string safe = make_valid_utf8(content);

    // Flatten to a single line: collapse every run of ASCII whitespace to one
    // space and trim the ends. Multi-byte UTF-8 bytes are never ASCII whitespace,
    // so they pass through untouched.
    std::string flat;
    flat.reserve(safe.size());
    bool has_structure = false;
    bool pending_space = false;
    for (const char ch : safe) {
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
