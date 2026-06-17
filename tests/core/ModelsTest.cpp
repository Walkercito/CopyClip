// Port of the reference oracle tests/core/test_models.py.
//
// Models.hpp holds the engine's immutable value objects. The Python reference
// uses @dataclass(frozen=True): a fixed set of fields with documented defaults.
// The frozen/"raises on set" behavior is a Python-ism with no compile-time C++
// equivalent, so these cases assert what does translate — the default field
// values and the HotkeySpec::display_name formatting contract.

#include "core/Models.hpp"
#include "config/Constants.hpp"
#include "core/Enums.hpp"

#include <chrono>

#include <gtest/gtest.h>

namespace {

namespace core = copyclip::core;
namespace config = copyclip::config;

// test_clipboard_entry_is_frozen (default-value half): a fresh entry is not
// pinned. The Python "assignment raises" check has no C++ analogue.
TEST(ModelsTest, ClipboardEntryDefaultsToUnpinned) {
    const core::ClipboardEntry entry{
        .content = "hi", .created_at = std::chrono::system_clock::time_point{}, .pinned = {}};
    EXPECT_EQ(entry.content, "hi");
    EXPECT_FALSE(entry.pinned);
}

// test_hotkey_spec_display_name: each modifier's value is Title-cased and the
// key's value is fully upper-cased, joined by '+'.
TEST(ModelsTest, DisplayNameTitleCasesModifiersAndUppercasesKey) {
    const core::HotkeySpec spec{.modifiers = {core::Modifier::Ctrl, core::Modifier::Alt},
                                .key = core::Key::V};
    EXPECT_EQ(spec.display_name(), "Ctrl+Alt+V");
}

// A single modifier plus a single-letter key.
TEST(ModelsTest, DisplayNameHandlesSingleModifier) {
    const core::HotkeySpec spec{.modifiers = {core::Modifier::Super}, .key = core::Key::V};
    EXPECT_EQ(spec.display_name(), "Super+V");
}

// A multi-character key value is upper-cased in full ("space" -> "SPACE").
TEST(ModelsTest, DisplayNameUppercasesMultiCharacterKey) {
    const core::HotkeySpec spec{.modifiers = {core::Modifier::Super}, .key = core::Key::Space};
    EXPECT_EQ(spec.display_name(), "Super+SPACE");
}

// test_settings_defaults: a default-constructed Settings mirrors the reference
// defaults, including max_history_items bound to the shared config constant.
TEST(ModelsTest, SettingsHaveReferenceDefaults) {
    const core::Settings settings{};
    EXPECT_EQ(settings.theme, core::Theme::Dark);
    EXPECT_EQ(settings.hotkey, core::HotkeyPreset::SuperV);
    EXPECT_FALSE(settings.first_run_completed);
    EXPECT_EQ(settings.max_history_items, config::kDefaultMaxHistoryItems);
    EXPECT_EQ(settings.max_history_items, 200);
    EXPECT_TRUE(settings.auto_hide_on_copy);
}

} // namespace
