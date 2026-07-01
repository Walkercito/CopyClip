// Port of the reference oracle tests/core/test_hotkeys.py.
//
// Hotkeys is the catalog of supported presets as pure data (no Xlib). These cases
// assert every preset resolves to a well-formed spec, the SUPER_V mapping is
// exact, kDefaultPreset is SuperV, and the catalog exposes all four presets.

#include "core/Hotkeys.hpp"
#include "config/Constants.hpp"
#include "core/Enums.hpp"
#include "core/Models.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include <gtest/gtest.h>

namespace {

namespace core = copyclip::core;
namespace config = copyclip::config;

// The closed set of presets under test, mirroring iteration over the Python
// HotkeyPreset enum. Declared once so the cases stay DRY.
constexpr std::array<core::HotkeyPreset, 4> kAllPresetEnumerators{
    core::HotkeyPreset::SuperV,
    core::HotkeyPreset::CtrlAltV,
    core::HotkeyPreset::SuperC,
    core::HotkeyPreset::CtrlShiftV,
};

// Every preset resolves to a spec. The enum is closed and type-safe, so "valid"
// reduces to "non-empty": at least one modifier and a key with a string form.
TEST(HotkeysTest, EachPresetMapsToASpec) {
    for (const core::HotkeyPreset preset : kAllPresetEnumerators) {
        const core::HotkeySpec spec = core::get_spec(preset);
        EXPECT_FALSE(spec.modifiers.empty());
        EXPECT_FALSE(core::to_string(spec.key).empty());
        for (const core::Modifier modifier : spec.modifiers) {
            EXPECT_FALSE(core::to_string(modifier).empty());
        }
    }
}

// SUPER_V maps to ((Super,), V) with display name "Super+V".
TEST(HotkeysTest, SuperVSpecHasSuperModifierAndVKey) {
    const core::HotkeySpec spec = core::get_spec(core::HotkeyPreset::SuperV);
    const std::vector<core::Modifier> expected_modifiers{core::Modifier::Super};
    EXPECT_EQ(spec.modifiers, expected_modifiers);
    EXPECT_EQ(spec.key, core::Key::V);
    EXPECT_EQ(spec.display_name(), "Super+V");
}

TEST(HotkeysTest, DefaultPresetIsSuperV) {
    EXPECT_EQ(core::kDefaultPreset, core::HotkeyPreset::SuperV);
}

// The remaining catalog entries match the reference dict exactly.
TEST(HotkeysTest, CatalogMapsRemainingPresetsExactly) {
    const core::HotkeySpec ctrl_alt_v = core::get_spec(core::HotkeyPreset::CtrlAltV);
    EXPECT_EQ(ctrl_alt_v.modifiers,
              (std::vector<core::Modifier>{core::Modifier::Ctrl, core::Modifier::Alt}));
    EXPECT_EQ(ctrl_alt_v.key, core::Key::V);

    const core::HotkeySpec super_c = core::get_spec(core::HotkeyPreset::SuperC);
    EXPECT_EQ(super_c.modifiers, (std::vector<core::Modifier>{core::Modifier::Super}));
    EXPECT_EQ(super_c.key, core::Key::C);

    const core::HotkeySpec ctrl_shift_v = core::get_spec(core::HotkeyPreset::CtrlShiftV);
    EXPECT_EQ(ctrl_shift_v.modifiers,
              (std::vector<core::Modifier>{core::Modifier::Ctrl, core::Modifier::Shift}));
    EXPECT_EQ(ctrl_shift_v.key, core::Key::V);
}

// all_presets() returns a copy of the full catalog: one entry per enumerator,
// each agreeing with get_spec. Mirrors all_presets() == dict(_PRESETS).
TEST(HotkeysTest, AllPresetsReturnsFullCatalog) {
    const auto catalog = core::all_presets();
    EXPECT_EQ(catalog.size(), kAllPresetEnumerators.size());
    for (const core::HotkeyPreset preset : kAllPresetEnumerators) {
        const auto found =
            std::find_if(catalog.begin(), catalog.end(),
                         [preset](const auto& entry) { return entry.first == preset; });
        ASSERT_NE(found, catalog.end());
        const core::HotkeySpec direct = core::get_spec(preset);
        EXPECT_EQ(found->second.modifiers, direct.modifiers);
        EXPECT_EQ(found->second.key, direct.key);
    }
}

// Each preset maps to its exact GNOME accelerator string (the form gsettings and
// the capture UI agree on).
TEST(HotkeysTest, AcceleratorForMapsEveryPreset) {
    EXPECT_EQ(core::accelerator_for(core::HotkeyPreset::SuperV), "<Super>v");
    EXPECT_EQ(core::accelerator_for(core::HotkeyPreset::CtrlAltV), "<Control><Alt>v");
    EXPECT_EQ(core::accelerator_for(core::HotkeyPreset::SuperC), "<Super>c");
    EXPECT_EQ(core::accelerator_for(core::HotkeyPreset::CtrlShiftV), "<Control><Shift>v");
}

// accelerator_from_stored migrates legacy preset tokens, passes a raw accelerator
// through unchanged, and falls back to the default for an empty value.
TEST(HotkeysTest, AcceleratorFromStoredHandlesLegacyCustomAndEmpty) {
    EXPECT_EQ(core::accelerator_from_stored("super_v"), "<Super>v");
    EXPECT_EQ(core::accelerator_from_stored("ctrl_shift_v"), "<Control><Shift>v");
    EXPECT_EQ(core::accelerator_from_stored("<Primary>grave"), "<Primary>grave");
    EXPECT_EQ(core::accelerator_from_stored(""), config::kDefaultHotkeyAccelerator);
}

} // namespace
