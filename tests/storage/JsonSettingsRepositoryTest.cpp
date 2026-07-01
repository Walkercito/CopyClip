// Port of the reference oracle tests/storage/test_json_settings.py.
//
// Exercises JsonSettingsRepository against a real on-disk JSON file in a per-test
// TempDir. The three reference cases (missing -> defaults, round-trip, corrupt ->
// defaults) port verbatim; two extra cases guard robustness the C++ spec mandates
// but the reference does not: a present-but-invalid enum value falls back to
// defaults (the reference would crash), and a successful save leaves no temp file
// behind (proving the atomic write renamed it away).

#include "storage/JsonSettingsRepository.hpp"
#include "config/Constants.hpp"
#include "core/Enums.hpp"
#include "core/Models.hpp"
#include "support/TempDir.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace {

namespace core = copyclip::core;
namespace storage = copyclip::storage;
namespace config = copyclip::config;

using copyclip::testing::TempDir;

// Field-by-field equality: Settings exposes no operator== (Models.hpp belongs to
// the core layer, which a storage task must not modify), so cases compare fields.
void expect_settings_eq(const core::Settings& actual, const core::Settings& expected) {
    EXPECT_EQ(actual.theme, expected.theme);
    EXPECT_EQ(actual.hotkey, expected.hotkey);
    EXPECT_EQ(actual.first_run_completed, expected.first_run_completed);
    EXPECT_EQ(actual.max_history_items, expected.max_history_items);
    EXPECT_EQ(actual.auto_hide_on_copy, expected.auto_hide_on_copy);
    EXPECT_EQ(actual.auto_paste, expected.auto_paste);
    EXPECT_EQ(actual.show_panel_icon, expected.show_panel_icon);
}

// Write `text` verbatim to `path`, seeding a corrupt or hand-crafted file.
void write_text(const std::filesystem::path& path, std::string_view text) {
    std::ofstream stream{path, std::ios::binary | std::ios::trunc};
    stream << text;
}

// Fixture: a fresh TempDir and settings path per test. The repository is built
// per-case because several tests open a second instance on the same path to prove
// cross-instance persistence.
class JsonSettingsRepositoryTest : public ::testing::Test {
protected:
    [[nodiscard]] std::filesystem::path settings_path() const {
        return temp_dir_.path() / config::kSettingsFileName;
    }

    [[nodiscard]] storage::JsonSettingsRepository repo() const {
        return storage::JsonSettingsRepository{settings_path()};
    }

private:
    TempDir temp_dir_;
};

// Loading a path that does not exist yields the documented Settings defaults.
TEST_F(JsonSettingsRepositoryTest, LoadReturnsDefaultsWhenMissing) {
    expect_settings_eq(repo().load(), core::Settings{});
}

// A saved non-default Settings reloads identically, including across a fresh
// repository instance on the same path.
TEST_F(JsonSettingsRepositoryTest, SaveThenLoadRoundtrip) {
    const core::Settings saved{.theme = core::Theme::Light,
                               .hotkey = "<Control><Alt>v",
                               .first_run_completed = true,
                               .max_history_items = config::kDefaultMaxHistoryItems,
                               .auto_hide_on_copy = true,
                               .auto_paste = true,
                               .show_panel_icon = false};
    repo().save(saved);

    const core::Settings loaded = storage::JsonSettingsRepository{settings_path()}.load();
    expect_settings_eq(loaded, saved);
}

// An unparseable file degrades to defaults instead of throwing.
TEST_F(JsonSettingsRepositoryTest, CorruptedFileFallsBackToDefaults) {
    write_text(settings_path(), "{not json");
    expect_settings_eq(repo().load(), core::Settings{});
}

// Extra (no reference analogue): a valid file with an invalid enumerator (theme
// "rainbow") is treated as corrupt and falls back to defaults. The reference would
// raise; the C++ spec requires load() never to crash.
TEST_F(JsonSettingsRepositoryTest, InvalidEnumValueFallsBackToDefaults) {
    write_text(settings_path(), R"({"theme": "rainbow"})");
    expect_settings_eq(repo().load(), core::Settings{});
}

// A settings file from an older build stored the hotkey as a preset token;
// load() migrates it to the equivalent GNOME accelerator.
TEST_F(JsonSettingsRepositoryTest, MigratesLegacyPresetTokenToAccelerator) {
    write_text(settings_path(), R"({"hotkey": "super_c"})");
    EXPECT_EQ(repo().load().hotkey, "<Super>c");
}

// A current build stores a raw accelerator, including a custom one outside the
// presets; load() preserves it verbatim.
TEST_F(JsonSettingsRepositoryTest, PreservesCustomAccelerator) {
    write_text(settings_path(), R"({"hotkey": "<Primary>grave"})");
    EXPECT_EQ(repo().load().hotkey, "<Primary>grave");
}

// Extra (no reference analogue): after a successful save the atomic write's temp
// file must not remain — write-temp-then-rename moves it onto the final path.
TEST_F(JsonSettingsRepositoryTest, SaveLeavesNoTempFileBehind) {
    repo().save(core::Settings{});

    std::filesystem::path temp_path = settings_path();
    temp_path.replace_extension(config::kSettingsTempSuffix);

    EXPECT_TRUE(std::filesystem::exists(settings_path()));
    EXPECT_FALSE(std::filesystem::exists(temp_path));
}

} // namespace
