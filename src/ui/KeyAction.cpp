#include "ui/KeyAction.hpp"

#include <gdk/gdkkeysyms.h>

namespace copyclip::ui {

KeyAction key_action(unsigned int keyval, const KeyContext& context) {
    switch (keyval) {
    case GDK_KEY_Escape:
        // First Escape drops an active search, a second dismisses — so narrowing a
        // search is undoable without losing the window.
        return context.search_active ? KeyAction::ClearSearch : KeyAction::Dismiss;
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
        return KeyAction::SelectPrevious;
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
        return KeyAction::SelectNext;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
        // A focused button owns Enter: the header's Settings/Clear and a card's
        // expand toggle must activate what the user tabbed to, not paste.
        return context.button_focused ? KeyAction::None : KeyAction::Paste;
    default:
        return KeyAction::None; // typing, Tab, everything else keeps its stock behavior
    }
}

} // namespace copyclip::ui
