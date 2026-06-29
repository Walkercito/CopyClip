#pragma once

// Value objects for the engine's domain. Mirrors copyclip/core/models.py
// (frozen dataclasses); C++ has no runtime frozen guard, so these are plain
// value types — immutability is convention, not enforced. Members are public
// and non-const so the types stay fully copyable/movable (Rule of Zero): const
// members would suppress the move/copy that std::vector and sort need (C.12).
//
// HotkeySpec::display_name mirrors the reference property: each modifier Title-
// cased, the key fully UPPER-cased, joined by '+'. It reuses to_string + ascii_*
// so the enum<->string mapping is never duplicated.

#include "config/Constants.hpp"
#include "core/Enums.hpp"
#include "core/detail/AsciiCase.hpp"

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace copyclip::core {

// A captured clipboard item. `content` is the dedup key: the plain text for Text
// and RichText, or the image's content hash for Image. `html` carries RichText
// markup. `image` holds PNG bytes but is populated only when an entry is added or
// written back — reads leave it empty so the history list stays light (image
// bytes are fetched lazily by hash). `pinned` marks entries kept across eviction.
struct ClipboardEntry {
    ClipKind kind = ClipKind::Text;
    std::string content{};
    std::string html{};
    std::vector<std::byte> image{};
    int image_width = 0;
    int image_height = 0;
    std::chrono::system_clock::time_point created_at{};
    bool pinned = false;
};

// Transient clipboard content as read from or written to the clipboard, before
// it becomes a stored ClipboardEntry. `text` is the plain text (Text/RichText),
// `html` the RichText markup, `image` the PNG bytes (Image).
struct ClipContent {
    ClipKind kind = ClipKind::Text;
    std::string text{};
    std::string html{};
    std::vector<std::byte> image{};
    int image_width = 0;
    int image_height = 0;
};

// A hotkey: an ordered list of modifiers plus one key. The order drives
// display_name(), e.g. {Ctrl, Alt} + V -> "Ctrl+Alt+V".
struct HotkeySpec {
    std::vector<Modifier> modifiers;
    Key key;

    [[nodiscard]] std::string display_name() const {
        static constexpr std::string_view kSeparator = "+";
        std::string result;
        for (const Modifier modifier : modifiers) {
            result += detail::ascii_capitalize(to_string(modifier));
            result += kSeparator;
        }
        result += detail::ascii_upper(to_string(key));
        return result;
    }
};

// User-configurable settings; defaults match the reference.
struct Settings {
    Theme theme = Theme::Dark;
    HotkeyPreset hotkey = HotkeyPreset::SuperV;
    bool first_run_completed = false;
    int max_history_items = config::kDefaultMaxHistoryItems;
    bool auto_hide_on_copy = true;
    bool auto_paste = false;
};

} // namespace copyclip::core
