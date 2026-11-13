#pragma once

// Fuzzy matching for the search box — no GTK, so it unit-tests headless. A clip
// matches when every character of the query appears in its text in order (not
// necessarily adjacent), case-insensitively: "spng" still finds "screenshot.png".
// That forgiveness lets a long clip match almost any query by scattered letters,
// so a match is also ranked: the query found as one contiguous run outranks the
// same characters strewn across the text.

#include <cstdint>
#include <string_view>
#include <vector>

namespace copyclip::ui {

// How a text answers a search query, best first — so comparing two values with <
// ranks them.
enum class MatchQuality : std::uint8_t {
    Substring,   // the query appears as one contiguous run
    Subsequence, // the query's characters appear in order, with gaps between them
    None,        // the text does not match
};

// How `text` answers `query`, folding ASCII case. An empty query is a substring of
// any text. For full Unicode case-folding, pass already-lowercased input (the
// ASCII fold here is then a no-op).
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): query/text read clearly.
[[nodiscard]] MatchQuality match_quality(std::string_view query, std::string_view text);

// Which bytes of `text` answer `query`, for highlighting it in the card: every
// verbatim occurrence when there is one, else the in-order characters that make
// it a subsequence match. Same case rules as match_quality. All false for an
// empty query or no match.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): query/text read clearly.
[[nodiscard]] std::vector<bool> match_mask(std::string_view query, std::string_view text);

} // namespace copyclip::ui
