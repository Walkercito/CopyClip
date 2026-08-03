#pragma once

// The main window's keyboard policy, as a pure decision: which action a key press
// asks for, given what the window currently holds and the user's configured
// paste/pin accelerators. No widgets are touched here, so the policy unit-tests
// headless while MainWindow keeps only the dispatch.

#include <cstdint>
#include <string_view>

namespace copyclip::ui {

// What a key press asks the main window to do.
enum class KeyAction : std::uint8_t {
    None,           // not ours — let the focused widget have it
    ClearSearch,    // drop the active filter, keeping the window up
    Dismiss,        // hide the window without pasting
    SelectPrevious, // move the highlight to the previous match
    SelectNext,     // move the highlight to the next match
    Paste,          // paste the highlighted clip
    TogglePin,      // pin / unpin the highlighted clip
};

// The window state the policy depends on. Grouped in a struct so the call site
// reads as named fields rather than a row of anonymous bools.
struct KeyContext {
    bool search_active = false;  // the search entry holds text
    bool button_focused = false; // a button has focus, so it owns Enter
};

// A parsed GNOME accelerator (keyval + modifier mask). Produced by
// bound_accelerator() from the settings string; MainWindow parses on each key
// press (cheap) so a Settings change is live without a cache invalidation path.
struct BoundAccelerator {
    unsigned int keyval = 0;
    unsigned int modifiers = 0;

    // True when the event matches this binding. Return / KP_Enter / ISO_Enter
    // are treated as the same key so the default paste binding covers them all.
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters): keyval/modifiers read clearly.
    [[nodiscard]] bool matches(unsigned int event_keyval, unsigned int event_modifiers) const;
};

// Parse a GNOME accelerator string ("Return", "<Control>Return"). Invalid or
// empty strings yield a never-matching binding (keyval 0).
[[nodiscard]] BoundAccelerator bound_accelerator(std::string_view accelerator);

// The two in-window shortcuts Settings stores as accelerator strings.
struct WindowShortcuts {
    BoundAccelerator paste;
    BoundAccelerator pin;
};

// Map a key event to the action it asks for. `modifiers` must already be masked
// to gtk_accelerator_get_default_mod_mask() so it lines up with BoundAccelerator.
[[nodiscard]] KeyAction key_action(unsigned int keyval, unsigned int modifiers,
                                   const KeyContext& context, const WindowShortcuts& shortcuts);

} // namespace copyclip::ui
