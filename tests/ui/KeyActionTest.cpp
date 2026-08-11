#include "ui/KeyAction.hpp"

#include <gtest/gtest.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

namespace {

using copyclip::ui::bound_accelerator;
using copyclip::ui::BoundAccelerator;
using copyclip::ui::key_action;
using copyclip::ui::KeyAction;
using copyclip::ui::KeyContext;
using copyclip::ui::WindowShortcuts;

// Default bindings: Enter pastes, Ctrl+Enter pins.
const WindowShortcuts kDefaults{
    .paste = BoundAccelerator{.keyval = GDK_KEY_Return, .modifiers = 0},
    .pin = BoundAccelerator{.keyval = GDK_KEY_Return, .modifiers = GDK_CONTROL_MASK},
};

// The window as it sits when it opens: nothing typed, focus in the search entry.
constexpr KeyContext kFresh{.search_active = false, .button_focused = false};
// Mid-search: text sits in the filter.
constexpr KeyContext kSearching{.search_active = true, .button_focused = false};
// Tabbed onto a button (header Settings/Clear, or a card's expand toggle).
constexpr KeyContext kOnButton{.search_active = false, .button_focused = true};

[[nodiscard]] KeyAction act(unsigned int keyval, unsigned int mods = 0,
                            const KeyContext& context = kFresh,
                            const WindowShortcuts& shortcuts = kDefaults) {
    return key_action(keyval, mods, context, shortcuts);
}

// Type to filter, press Enter — it must paste, not fall through to the search
// entry's own Return binding (issue #7).
TEST(KeyActionTest, EnterPastesWhileSearching) {
    EXPECT_EQ(act(GDK_KEY_Return, 0, kSearching), KeyAction::Paste);
}

TEST(KeyActionTest, EnterPastesFromEveryReturnKey) {
    EXPECT_EQ(act(GDK_KEY_Return), KeyAction::Paste);
    EXPECT_EQ(act(GDK_KEY_KP_Enter), KeyAction::Paste);
    EXPECT_EQ(act(GDK_KEY_ISO_Enter), KeyAction::Paste);
}

// A focused button keeps plain Enter, or tabbing to Settings would paste instead.
TEST(KeyActionTest, EnterOnAButtonIsNotOurs) {
    EXPECT_EQ(act(GDK_KEY_Return, 0, kOnButton), KeyAction::None);
}

// Ctrl+Enter pins the highlighted clip — the keyboard twin of Ctrl+click.
TEST(KeyActionTest, CtrlEnterTogglesPin) {
    EXPECT_EQ(act(GDK_KEY_Return, GDK_CONTROL_MASK), KeyAction::TogglePin);
    EXPECT_EQ(act(GDK_KEY_KP_Enter, GDK_CONTROL_MASK), KeyAction::TogglePin);
    EXPECT_EQ(act(GDK_KEY_ISO_Enter, GDK_CONTROL_MASK), KeyAction::TogglePin);
}

// Ctrl+Enter still pins mid-search and with a button focused: pin is about the
// list selection, not the focused widget.
TEST(KeyActionTest, CtrlEnterPinsRegardlessOfFocusOrSearch) {
    EXPECT_EQ(act(GDK_KEY_Return, GDK_CONTROL_MASK, kSearching), KeyAction::TogglePin);
    EXPECT_EQ(act(GDK_KEY_Return, GDK_CONTROL_MASK, kOnButton), KeyAction::TogglePin);
    constexpr KeyContext both{.search_active = true, .button_focused = true};
    EXPECT_EQ(act(GDK_KEY_Return, GDK_CONTROL_MASK, both), KeyAction::TogglePin);
}

// First Escape clears the filter, a second dismisses — narrowing is undoable
// without losing the window. (hide() separately clears the filter on every path.)
TEST(KeyActionTest, EscapeClearsSearchBeforeDismissing) {
    EXPECT_EQ(act(GDK_KEY_Escape, 0, kSearching), KeyAction::ClearSearch);
    EXPECT_EQ(act(GDK_KEY_Escape, 0, kFresh), KeyAction::Dismiss);
}

// Escape dismisses from a button too — unlike Enter, no widget owns it.
TEST(KeyActionTest, EscapeIsOursEvenOnAButton) {
    EXPECT_EQ(act(GDK_KEY_Escape, 0, kOnButton), KeyAction::Dismiss);
}

// The one state where the two rules could collide: mid-search with a button
// focused. Enter is still the button's, Escape still clears.
TEST(KeyActionTest, SearchingWithAButtonFocusedKeepsBothRules) {
    constexpr KeyContext both{.search_active = true, .button_focused = true};
    EXPECT_EQ(act(GDK_KEY_Return, 0, both), KeyAction::None);
    EXPECT_EQ(act(GDK_KEY_Escape, 0, both), KeyAction::ClearSearch);
}

TEST(KeyActionTest, ArrowsMoveTheSelection) {
    EXPECT_EQ(act(GDK_KEY_Up), KeyAction::SelectPrevious);
    EXPECT_EQ(act(GDK_KEY_KP_Up), KeyAction::SelectPrevious);
    EXPECT_EQ(act(GDK_KEY_Down), KeyAction::SelectNext);
    EXPECT_EQ(act(GDK_KEY_KP_Down), KeyAction::SelectNext);
}

// The arrows stay ours mid-search: that is the whole point of navigating the
// results without leaving the search entry.
TEST(KeyActionTest, ArrowsMoveTheSelectionWhileSearching) {
    EXPECT_EQ(act(GDK_KEY_Down, 0, kSearching), KeyAction::SelectNext);
}

// Modified arrows fall through so the search entry keeps word-jump / extend.
TEST(KeyActionTest, ModifiedArrowsFallThrough) {
    EXPECT_EQ(act(GDK_KEY_Down, GDK_CONTROL_MASK), KeyAction::None);
    EXPECT_EQ(act(GDK_KEY_Up, GDK_SHIFT_MASK), KeyAction::None);
    EXPECT_EQ(act(GDK_KEY_KP_Down, GDK_CONTROL_MASK | GDK_SHIFT_MASK), KeyAction::None);
}

// Delete removes the highlighted clip when the user is not filtering.
TEST(KeyActionTest, DeleteRemovesTheSelectionWhenNotSearching) {
    EXPECT_EQ(act(GDK_KEY_Delete), KeyAction::RemoveSelected);
    EXPECT_EQ(act(GDK_KEY_KP_Delete), KeyAction::RemoveSelected);
}

// Mid-search, Delete belongs to the entry (forward-delete a character), or the
// filter can't be edited with it.
TEST(KeyActionTest, DeleteEditsTextWhileSearching) {
    EXPECT_EQ(act(GDK_KEY_Delete, 0, kSearching), KeyAction::None);
    EXPECT_EQ(act(GDK_KEY_KP_Delete, 0, kSearching), KeyAction::None);
}

// Modified Delete falls through so the entry keeps Ctrl+Delete (word delete).
TEST(KeyActionTest, ModifiedDeleteFallsThrough) {
    EXPECT_EQ(act(GDK_KEY_Delete, GDK_CONTROL_MASK), KeyAction::None);
}

// Typing must reach the search entry untouched, or the filter stops working.
TEST(KeyActionTest, OtherKeysFallThrough) {
    EXPECT_EQ(act(GDK_KEY_a, 0, kSearching), KeyAction::None);
    EXPECT_EQ(act(GDK_KEY_space, 0, kSearching), KeyAction::None);
    EXPECT_EQ(act(GDK_KEY_BackSpace, 0, kSearching), KeyAction::None);
    EXPECT_EQ(act(GDK_KEY_Tab), KeyAction::None);
}

// Rebinding paste/pin through settings strings is the whole point of storing
// accelerators — a custom Ctrl+P pin must win over the default Ctrl+Enter.
TEST(KeyActionTest, CustomBindingsAreHonoured) {
    const WindowShortcuts custom{
        .paste = BoundAccelerator{.keyval = GDK_KEY_v, .modifiers = GDK_CONTROL_MASK},
        .pin = BoundAccelerator{.keyval = GDK_KEY_p, .modifiers = GDK_CONTROL_MASK},
    };
    EXPECT_EQ(act(GDK_KEY_v, GDK_CONTROL_MASK, kFresh, custom), KeyAction::Paste);
    EXPECT_EQ(act(GDK_KEY_p, GDK_CONTROL_MASK, kFresh, custom), KeyAction::TogglePin);
    // Defaults no longer apply once rebound.
    EXPECT_EQ(act(GDK_KEY_Return, 0, kFresh, custom), KeyAction::None);
    EXPECT_EQ(act(GDK_KEY_Return, GDK_CONTROL_MASK, kFresh, custom), KeyAction::None);
}

// Modified paste still pastes when a button has focus — buttons only own bare
// Return/space, not Ctrl+V.
TEST(KeyActionTest, ModifiedPasteIgnoresButtonFocus) {
    const WindowShortcuts custom{
        .paste = BoundAccelerator{.keyval = GDK_KEY_v, .modifiers = GDK_CONTROL_MASK},
        .pin = BoundAccelerator{.keyval = GDK_KEY_p, .modifiers = GDK_CONTROL_MASK},
    };
    EXPECT_EQ(act(GDK_KEY_v, GDK_CONTROL_MASK, kOnButton, custom), KeyAction::Paste);
}

// bound_accelerator parses the same GNOME strings Settings stores.
TEST(KeyActionTest, BoundAcceleratorParsesStoredStrings) {
    const BoundAccelerator paste = bound_accelerator("Return");
    EXPECT_EQ(paste.keyval, static_cast<unsigned int>(GDK_KEY_Return));
    EXPECT_EQ(paste.modifiers, 0U);

    const BoundAccelerator pin = bound_accelerator("<Control>Return");
    EXPECT_EQ(pin.keyval, static_cast<unsigned int>(GDK_KEY_Return));
    EXPECT_EQ(pin.modifiers, static_cast<unsigned int>(GDK_CONTROL_MASK));

    EXPECT_EQ(bound_accelerator("").keyval, 0U);
    EXPECT_EQ(bound_accelerator("not-a-key").keyval, 0U);
}

} // namespace
