#include "ui/ShortcutText.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

using copyclip::ui::build_keybinding_array;
using copyclip::ui::parse_keybinding_paths;
using copyclip::ui::paste_quick_picks;
using copyclip::ui::pin_quick_picks;
using copyclip::ui::quick_picks;
using copyclip::ui::QuickPick;

// quick_picks() surfaces the presets as label/accelerator pairs for the capture
// UI's one-tap buttons; Super+V leads, and none is blank.
TEST(ShortcutTextTest, QuickPicksExposePresetLabelsAndAccelerators) {
    const std::vector<QuickPick> picks = quick_picks();
    ASSERT_FALSE(picks.empty());
    EXPECT_EQ(picks.front().label, "Super+V");
    EXPECT_EQ(picks.front().accelerator, "<Super>v");
    for (const QuickPick& pick : picks) {
        EXPECT_FALSE(pick.label.empty());
        EXPECT_FALSE(pick.accelerator.empty());
    }
}

// In-window paste/pin choosers share the same QuickPick shape with defaults that
// match config::kDefaultPasteAccelerator / kDefaultPinAccelerator.
TEST(ShortcutTextTest, PasteAndPinQuickPicksExposeDefaults) {
    const std::vector<QuickPick> paste = paste_quick_picks();
    ASSERT_FALSE(paste.empty());
    EXPECT_EQ(paste.front().accelerator, "Return");

    const std::vector<QuickPick> pin = pin_quick_picks();
    ASSERT_FALSE(pin.empty());
    EXPECT_EQ(pin.front().accelerator, "<Control>Return");
}

TEST(ShortcutTextTest, ParsesEntries) {
    EXPECT_EQ(parse_keybinding_paths("['/x/', '/y/']"), (std::vector<std::string>{"/x/", "/y/"}));
}

TEST(ShortcutTextTest, ParsesEmptyForms) {
    EXPECT_TRUE(parse_keybinding_paths("[]").empty());
    EXPECT_TRUE(parse_keybinding_paths("@as []").empty());
}

TEST(ShortcutTextTest, BuildsArray) {
    EXPECT_EQ(build_keybinding_array({"/x/", "/y/"}), "['/x/', '/y/']");
}

TEST(ShortcutTextTest, BuildsTypedEmptyArray) {
    EXPECT_EQ(build_keybinding_array({}), "@as []");
}

TEST(ShortcutTextTest, RoundTripsAppendedPath) {
    std::vector<std::string> paths = parse_keybinding_paths("['/x/']");
    paths.emplace_back("/a/");
    EXPECT_EQ(build_keybinding_array(paths), "['/x/', '/a/']");
}

} // namespace
