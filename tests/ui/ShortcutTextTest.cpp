#include "ui/ShortcutText.hpp"

#include <gtest/gtest.h>

namespace {

using copyclip::core::HotkeyPreset;
using copyclip::ui::accelerator_for;
using copyclip::ui::with_keybinding_path;

TEST(ShortcutTextTest, MapsPresetsToGnomeAccelerators) {
    EXPECT_EQ(accelerator_for(HotkeyPreset::SuperV), "<Super>v");
    EXPECT_EQ(accelerator_for(HotkeyPreset::CtrlAltV), "<Control><Alt>v");
    EXPECT_EQ(accelerator_for(HotkeyPreset::SuperC), "<Super>c");
    EXPECT_EQ(accelerator_for(HotkeyPreset::CtrlShiftV), "<Control><Shift>v");
}

TEST(ShortcutTextTest, AppendsToAnEmptyList) {
    EXPECT_EQ(with_keybinding_path("[]", "/a/"), "['/a/']");
    EXPECT_EQ(with_keybinding_path("@as []", "/a/"), "['/a/']");
}

TEST(ShortcutTextTest, PreservesExistingEntries) {
    EXPECT_EQ(with_keybinding_path("['/x/', '/y/']", "/a/"), "['/x/', '/y/', '/a/']");
}

TEST(ShortcutTextTest, IsIdempotentWhenAlreadyPresent) {
    EXPECT_EQ(with_keybinding_path("['/x/', '/a/']", "/a/"), "['/x/', '/a/']");
}

} // namespace
