#pragma once

// The main window's keyboard policy, as a pure decision: which action a key press
// asks for, given what the window currently holds. No widgets are touched here, so
// the policy unit-tests headless while MainWindow keeps only the dispatch.

#include <cstdint>

namespace copyclip::ui {

// What a key press asks the main window to do.
enum class KeyAction : std::uint8_t {
    None,           // not ours — let the focused widget have it
    ClearSearch,    // drop the active filter, keeping the window up
    Dismiss,        // hide the window without pasting
    SelectPrevious, // move the highlight to the previous match
    SelectNext,     // move the highlight to the next match
    Paste,          // paste the highlighted clip
};

// The window state the policy depends on. Grouped in a struct so the call site
// reads as named fields rather than a row of anonymous bools.
struct KeyContext {
    bool search_active = false;  // the search entry holds text
    bool button_focused = false; // a button has focus, so it owns Enter
};

// Map a GDK keyval (GDK_KEY_*) to the action it asks for. Modifiers are
// deliberately ignored: Ctrl+Enter and the like read as their plain form, since
// none of them mean anything else in this window.
[[nodiscard]] KeyAction key_action(unsigned int keyval, const KeyContext& context);

} // namespace copyclip::ui
