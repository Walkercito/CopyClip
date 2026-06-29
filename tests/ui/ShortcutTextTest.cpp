#include "ui/ShortcutText.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

namespace {

using copyclip::core::HotkeyPreset;
using copyclip::ui::accelerator_for;
using copyclip::ui::build_keybinding_array;
using copyclip::ui::hotkey_display_names;
using copyclip::ui::index_of_preset;
using copyclip::ui::parse_keybinding_paths;
using copyclip::ui::preset_at;

TEST(ShortcutTextTest, MapsPresetsToGnomeAccelerators) {
    EXPECT_EQ(accelerator_for(HotkeyPreset::SuperV), "<Super>v");
    EXPECT_EQ(accelerator_for(HotkeyPreset::CtrlAltV), "<Control><Alt>v");
    EXPECT_EQ(accelerator_for(HotkeyPreset::SuperC), "<Super>c");
    EXPECT_EQ(accelerator_for(HotkeyPreset::CtrlShiftV), "<Control><Shift>v");
}

TEST(ShortcutTextTest, HotkeyPresetIndexRoundTrips) {
    const std::vector<std::string> names = hotkey_display_names();
    ASSERT_FALSE(names.empty());
    for (unsigned int i = 0; i < names.size(); ++i) {
        const std::optional<HotkeyPreset> preset = preset_at(i);
        ASSERT_TRUE(preset.has_value());
        EXPECT_EQ(index_of_preset(*preset), i);
    }
    EXPECT_FALSE(preset_at(static_cast<unsigned int>(names.size())).has_value());
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
