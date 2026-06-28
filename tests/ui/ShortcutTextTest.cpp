#include "ui/ShortcutText.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

using copyclip::core::HotkeyPreset;
using copyclip::ui::accelerator_for;
using copyclip::ui::build_keybinding_array;
using copyclip::ui::parse_keybinding_paths;

TEST(ShortcutTextTest, MapsPresetsToGnomeAccelerators) {
    EXPECT_EQ(accelerator_for(HotkeyPreset::SuperV), "<Super>v");
    EXPECT_EQ(accelerator_for(HotkeyPreset::CtrlAltV), "<Control><Alt>v");
    EXPECT_EQ(accelerator_for(HotkeyPreset::SuperC), "<Super>c");
    EXPECT_EQ(accelerator_for(HotkeyPreset::CtrlShiftV), "<Control><Shift>v");
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
