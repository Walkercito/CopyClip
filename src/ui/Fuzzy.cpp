#include "ui/Fuzzy.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <string_view>

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

// True when every character of a non-empty `query` appears in `text`, in order.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): query/text read clearly.
[[nodiscard]] bool contains_in_order(std::string_view query, std::string_view text) {
    std::string_view pending = query; // the characters still to find, in order
    for (const char glyph : text) {
        if (lower(glyph) == lower(pending.front())) {
            pending.remove_prefix(1);
            if (pending.empty()) {
                return true;
            }
        }
    }
    return false;
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
    if (contains_in_order(query, text)) {
        return MatchQuality::Subsequence;
    }
    return MatchQuality::None;
}

} // namespace copyclip::ui
