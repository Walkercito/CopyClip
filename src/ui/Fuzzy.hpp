#pragma once

// Fuzzy subsequence matching for the search box — no GTK, so it unit-tests
// headless. A clip matches when every character of the query appears in its text
// in order (not necessarily adjacent), case-insensitively. This is more forgiving
// than a substring search: "spng" still finds "screenshot.png".

#include <string_view>

namespace copyclip::ui {

// True when `query` is a case-insensitive (ASCII) subsequence of `text`. An empty
// query always matches. For full Unicode case-folding, pass already-lowercased
// input (the ASCII fold here is then a no-op).
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): query/text read clearly.
[[nodiscard]] bool fuzzy_matches(std::string_view query, std::string_view text);

} // namespace copyclip::ui
