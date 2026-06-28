#pragma once

// Pure presentation helpers for clip content — no GTK, so they unit-test
// headless. Kept out of the widget code (which is hard to test) and in their own
// toolkit-free library.

#include <cstddef>
#include <string>
#include <string_view>

namespace copyclip::ui {

struct Preview {
    std::string text; // content, truncated to the limit with an ellipsis
    bool truncated;   // true when the original ran past the limit
};

// Truncate `content` to at most `max_code_points` UTF-8 code points, appending an
// ellipsis when it was longer. Cuts only on code-point boundaries, never inside a
// multi-byte character. A limit of 0 disables truncation.
[[nodiscard]] Preview make_preview(std::string_view content, std::size_t max_code_points);

} // namespace copyclip::ui
