#include "ui/Fuzzy.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <string_view>
#include <vector>

namespace copyclip::ui {

namespace {

[[nodiscard]] char lower(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

// True when `query` appears in `text` as one contiguous run.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): query/text read clearly.
[[nodiscard]] bool contains(std::string_view query, std::string_view text) {
    return !std::ranges::search(text, query, std::ranges::equal_to{}, lower, lower).empty();
}

// Offsets in `text` of the characters that spell a non-empty `query` in order,
// taking each at its first chance — empty when the query is not spelled out.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): query/text read clearly.
[[nodiscard]] std::vector<std::size_t> in_order_offsets(std::string_view query,
                                                        std::string_view text) {
    std::vector<std::size_t> offsets;
    offsets.reserve(query.size());
    for (std::size_t at = 0; at < text.size(); ++at) {
        if (lower(text[at]) == lower(query[offsets.size()])) {
            offsets.push_back(at);
            if (offsets.size() == query.size()) {
                return offsets;
            }
        }
    }
    return {};
}

} // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): query/text read clearly.
MatchQuality match_quality(std::string_view query, std::string_view text) {
    if (query.empty()) {
        return MatchQuality::Substring;
    }
    if (contains(query, text)) {
        return MatchQuality::Substring;
    }
    if (!in_order_offsets(query, text).empty()) {
        return MatchQuality::Subsequence;
    }
    return MatchQuality::None;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): query/text read clearly.
std::vector<bool> match_mask(std::string_view query, std::string_view text) {
    std::vector<bool> mask(text.size(), false);
    if (query.empty()) {
        return mask;
    }
    bool verbatim = false;
    for (std::string_view rest = text;;) {
        const auto hit = std::ranges::search(rest, query, std::ranges::equal_to{}, lower, lower);
        if (hit.empty()) {
            break;
        }
        const auto start = static_cast<std::size_t>(hit.begin() - text.begin());
        std::fill_n(mask.begin() + static_cast<std::ptrdiff_t>(start), query.size(), true);
        rest = text.substr(start + query.size());
        verbatim = true;
    }
    if (!verbatim) {
        for (const std::size_t at : in_order_offsets(query, text)) {
            mask[at] = true;
        }
    }
    return mask;
}

} // namespace copyclip::ui
