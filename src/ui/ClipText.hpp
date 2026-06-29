#pragma once

// Pure presentation helpers for clip content — no GTK, so they unit-test
// headless. Kept out of the widget code (which is hard to test) and in their own
// toolkit-free library.

#include <cstddef>
#include <string>
#include <string_view>

namespace copyclip::ui {

struct Preview {
    std::string text; // single-line, length-capped preview of the content
    bool truncated;   // true when the preview hides any of the original
};

// Build a single-line card preview: collapse every run of ASCII whitespace
// (including newlines and tabs) to one space, trim the ends, then cap to
// `max_code_points` UTF-8 code points with an ellipsis (cutting only on code-
// point boundaries). `truncated` is true when the preview omits something — the
// content ran past the limit, or it spanned multiple lines / used tabs — so the
// card knows to offer "Show all". A limit of 0 keeps the full flattened text.
// The returned text is always valid UTF-8 (see make_valid_utf8).
[[nodiscard]] Preview make_preview(std::string_view content, std::size_t max_code_points);

// A copy of `text` that is guaranteed valid UTF-8: every malformed byte or
// sequence (overlong, surrogate, out-of-range, truncated) is replaced with the
// Unicode replacement character U+FFFD. Pure (no GLib), so it stays headless-
// testable. Used to keep binary that slipped into a clip from reaching GTK's
// label/Pango or Glib::ustring search paths, which reject invalid UTF-8.
[[nodiscard]] std::string make_valid_utf8(std::string_view text);

} // namespace copyclip::ui
