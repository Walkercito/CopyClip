#include "ui/KeyAction.hpp"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <string>

namespace copyclip::ui {

namespace {

[[nodiscard]] bool is_return_key(unsigned int keyval) {
    return keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter || keyval == GDK_KEY_ISO_Enter;
}

// Keys that activate a focused button (Return family and space).
[[nodiscard]] bool is_activate_key(unsigned int keyval) {
    return is_return_key(keyval) || keyval == GDK_KEY_space;
}

} // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): keyval/modifiers read clearly.
bool BoundAccelerator::matches(unsigned int event_keyval, unsigned int event_modifiers) const {
    if (keyval == 0) {
        return false;
    }
    if (modifiers != event_modifiers) {
        return false;
    }
    if (keyval == event_keyval) {
        return true;
    }
    // One binding for Enter covers the physical Return and keypad variants.
    return is_return_key(keyval) && is_return_key(event_keyval);
}

BoundAccelerator bound_accelerator(std::string_view accelerator) {
    if (accelerator.empty()) {
        return {};
    }
    const std::string name{accelerator};
    guint keyval = 0;
    auto mods = static_cast<GdkModifierType>(0);
    gtk_accelerator_parse(name.c_str(), &keyval, &mods);
    if (keyval == 0) {
        return {};
    }
    // Keep only the bits gtk_accelerator_get_default_mod_mask cares about, so a
    // match against the event's masked state is an equality check.
    const auto mask = static_cast<unsigned int>(gtk_accelerator_get_default_mod_mask());
    return BoundAccelerator{.keyval = keyval, .modifiers = static_cast<unsigned int>(mods) & mask};
}

KeyAction key_action(unsigned int keyval, unsigned int modifiers, const KeyContext& context,
                     const WindowShortcuts& shortcuts) {
    switch (keyval) {
    case GDK_KEY_Escape:
        // First Escape drops an active search, a second dismisses — so narrowing a
        // search is undoable without losing the window. Modifiers on Escape are
        // ignored: Esc is always the window's own.
        return context.search_active ? KeyAction::ClearSearch : KeyAction::Dismiss;
    default:
        break;
    }

    // Configured actions before the fixed arrows, so a user can rebind over them.
    if (shortcuts.pin.matches(keyval, modifiers)) {
        return KeyAction::TogglePin;
    }
    if (shortcuts.paste.matches(keyval, modifiers)) {
        // A focused button owns unmodified activate keys (Return, space). Modified
        // paste bindings still paste — Settings/Clear never consume Ctrl+V.
        if (context.button_focused && modifiers == 0 && is_activate_key(keyval)) {
            return KeyAction::None;
        }
        return KeyAction::Paste;
    }

    // Bare arrows only: with modifiers, leave Ctrl/Shift+arrows to the search entry
    // (word jump / extend selection). Capture-phase would otherwise steal them.
    if (modifiers == 0) {
        switch (keyval) {
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            return KeyAction::SelectPrevious;
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            return KeyAction::SelectNext;
        default:
            break;
        }
    }
    return KeyAction::None; // typing, Tab, modified arrows, everything else
}

} // namespace copyclip::ui
