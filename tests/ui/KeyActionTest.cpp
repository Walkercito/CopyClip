#include "ui/KeyAction.hpp"

#include <gtest/gtest.h>

#include <gdk/gdkkeysyms.h>

namespace {

using copyclip::ui::key_action;
using copyclip::ui::KeyAction;
using copyclip::ui::KeyContext;

// The window as it sits when it opens: nothing typed, focus in the search entry.
constexpr KeyContext kFresh{.search_active = false, .button_focused = false};
// Mid-search: text sits in the filter.
constexpr KeyContext kSearching{.search_active = true, .button_focused = false};
// Tabbed onto a button (header Settings/Clear, or a card's expand toggle).
constexpr KeyContext kOnButton{.search_active = false, .button_focused = true};

// Type to filter, press Enter — it must paste, not fall through to the search
// entry's own Return binding (issue #7).
TEST(KeyActionTest, EnterPastesWhileSearching) {
    EXPECT_EQ(key_action(GDK_KEY_Return, kSearching), KeyAction::Paste);
}

TEST(KeyActionTest, EnterPastesFromEveryReturnKey) {
    EXPECT_EQ(key_action(GDK_KEY_Return, kFresh), KeyAction::Paste);
    EXPECT_EQ(key_action(GDK_KEY_KP_Enter, kFresh), KeyAction::Paste);
    EXPECT_EQ(key_action(GDK_KEY_ISO_Enter, kFresh), KeyAction::Paste);
}

// A focused button keeps Enter, or tabbing to Settings would paste instead.
TEST(KeyActionTest, EnterOnAButtonIsNotOurs) {
    EXPECT_EQ(key_action(GDK_KEY_Return, kOnButton), KeyAction::None);
}

// First Escape clears the filter, a second dismisses — narrowing is undoable
// without losing the window. (hide() separately clears the filter on every path.)
TEST(KeyActionTest, EscapeClearsSearchBeforeDismissing) {
    EXPECT_EQ(key_action(GDK_KEY_Escape, kSearching), KeyAction::ClearSearch);
    EXPECT_EQ(key_action(GDK_KEY_Escape, kFresh), KeyAction::Dismiss);
}

// Escape dismisses from a button too — unlike Enter, no widget owns it.
TEST(KeyActionTest, EscapeIsOursEvenOnAButton) {
    EXPECT_EQ(key_action(GDK_KEY_Escape, kOnButton), KeyAction::Dismiss);
}

// The one state where the two rules could collide: mid-search with a button
// focused. Enter is still the button's, Escape still clears.
TEST(KeyActionTest, SearchingWithAButtonFocusedKeepsBothRules) {
    constexpr KeyContext both{.search_active = true, .button_focused = true};
    EXPECT_EQ(key_action(GDK_KEY_Return, both), KeyAction::None);
    EXPECT_EQ(key_action(GDK_KEY_Escape, both), KeyAction::ClearSearch);
}

TEST(KeyActionTest, ArrowsMoveTheSelection) {
    EXPECT_EQ(key_action(GDK_KEY_Up, kFresh), KeyAction::SelectPrevious);
    EXPECT_EQ(key_action(GDK_KEY_KP_Up, kFresh), KeyAction::SelectPrevious);
    EXPECT_EQ(key_action(GDK_KEY_Down, kFresh), KeyAction::SelectNext);
    EXPECT_EQ(key_action(GDK_KEY_KP_Down, kFresh), KeyAction::SelectNext);
}

// The arrows stay ours mid-search: that is the whole point of navigating the
// results without leaving the search entry.
TEST(KeyActionTest, ArrowsMoveTheSelectionWhileSearching) {
    EXPECT_EQ(key_action(GDK_KEY_Down, kSearching), KeyAction::SelectNext);
}

// Typing must reach the search entry untouched, or the filter stops working.
TEST(KeyActionTest, OtherKeysFallThrough) {
    EXPECT_EQ(key_action(GDK_KEY_a, kSearching), KeyAction::None);
    EXPECT_EQ(key_action(GDK_KEY_space, kSearching), KeyAction::None);
    EXPECT_EQ(key_action(GDK_KEY_BackSpace, kSearching), KeyAction::None);
    EXPECT_EQ(key_action(GDK_KEY_Tab, kFresh), KeyAction::None);
}

} // namespace
