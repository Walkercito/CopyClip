// Port of the reference oracle tests/storage/test_json_settings.py.
//
// Exercises JsonSettingsRepository against a real on-disk JSON file in a per-test
// TempDir, mirroring the reference suite's `tmp_path` fixture. The three reference
// cases (missing -> defaults, save/load round-trip, corrupt -> defaults) are ported
// verbatim in behavior; two extra cases guard robustness the reference does not
// cover but the C++ spec mandates: a present-but-invalid enum value falls back to
// defaults (rather than crashing as the reference would), and a successful save
// leaves no temp file behind (proving the atomic write renamed it away).
//
// Settings has no operator== (it lives in the already-committed core layer and a
// storage task must not reach across to add one), so each case compares the five
// fields individually through expect_settings_eq.

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

// Field-by-field equality for Settings. Settings deliberately exposes no
// operator== (Models.hpp is owned by the core layer), so the round-trip and
// fallback cases assert each field rather than the whole object.
void expect_settings_eq(const core::Settings& actual, const core::Settings& expected) {
    EXPECT_EQ(actual.theme, expected.theme);
    EXPECT_EQ(actual.hotkey, expected.hotkey);
    EXPECT_EQ(actual.first_run_completed, expected.first_run_completed);
    EXPECT_EQ(actual.max_history_items, expected.max_history_items);
    EXPECT_EQ(actual.auto_hide_on_copy, expected.auto_hide_on_copy);
}

// Write `text` verbatim to `path`, used by the cases that seed a corrupt or
// hand-crafted settings file before loading it.
void write_text(const std::filesystem::path& path, std::string_view text) {
    std::ofstream stream{path, std::ios::binary | std::ios::trunc};
    stream << text;
}

// Fixture: a fresh TempDir and a settings path beneath it for each test. The
// repository is constructed per-case because several tests build a second
// instance against the same path to prove cross-instance persistence.
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

// test_load_returns_defaults_when_missing: loading a path that does not exist
// yields the documented Settings defaults.
TEST_F(JsonSettingsRepositoryTest, LoadReturnsDefaultsWhenMissing) {
    expect_settings_eq(repo().load(), core::Settings{});
}

// test_save_then_load_roundtrip: a saved non-default Settings reloads identically,
// including across a fresh repository instance on the same path.
TEST_F(JsonSettingsRepositoryTest, SaveThenLoadRoundtrip) {
    const core::Settings saved{.theme = core::Theme::Light,
                               .hotkey = core::HotkeyPreset::CtrlAltV,
                               .first_run_completed = true,
                               .max_history_items = config::kDefaultMaxHistoryItems,
                               .auto_hide_on_copy = true};
    repo().save(saved);

    const core::Settings loaded = storage::JsonSettingsRepository{settings_path()}.load();
    expect_settings_eq(loaded, saved);
}

// test_corrupted_file_falls_back_to_defaults: an unparseable file ("{not json")
// degrades to defaults instead of throwing.
TEST_F(JsonSettingsRepositoryTest, CorruptedFileFallsBackToDefaults) {
    write_text(settings_path(), "{not json");
    expect_settings_eq(repo().load(), core::Settings{});
}

// Extra robustness (no reference analogue): a syntactically valid file carrying an
// invalid enumerator (theme "rainbow" is not a Theme) is treated as corrupt and
// falls back to defaults. The reference would raise on this; the C++ spec requires
// load() to never crash.
TEST_F(JsonSettingsRepositoryTest, InvalidEnumValueFallsBackToDefaults) {
    write_text(settings_path(), R"({"theme": "rainbow"})");
    expect_settings_eq(repo().load(), core::Settings{});
}

// Extra robustness (no reference analogue): after a successful save the temp file
// used for the atomic write must not remain — the write-temp-then-rename sequence
// renames it onto the final path, leaving only settings.json behind.
TEST_F(JsonSettingsRepositoryTest, SaveLeavesNoTempFileBehind) {
    repo().save(core::Settings{});

    std::filesystem::path temp_path = settings_path();
    temp_path.replace_extension(config::kSettingsTempSuffix);

    EXPECT_TRUE(std::filesystem::exists(settings_path()));
    EXPECT_FALSE(std::filesystem::exists(temp_path));
}

} // namespace
