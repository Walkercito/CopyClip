#include "storage/JsonSettingsRepository.hpp"

#include "config/Constants.hpp"
#include "core/Enums.hpp"
#include "core/Models.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <string>
#include <utility>

namespace copyclip::storage {

namespace {

// --- JSON keys --------------------------------------------------------------
// The object keys, named so no bare string literal appears in the read/write
// logic. They match the reference module's dataclass field names verbatim, since
// the reference serializes via dataclasses.asdict().
constexpr const char* kKeyTheme = "theme";
constexpr const char* kKeyHotkey = "hotkey";
constexpr const char* kKeyFirstRunCompleted = "first_run_completed";
constexpr const char* kKeyMaxHistoryItems = "max_history_items";
constexpr const char* kKeyAutoHideOnCopy = "auto_hide_on_copy";

// Indentation width for the serialized JSON, mirroring the reference's
// json.dumps(..., indent=2).
constexpr int kJsonIndent = 2;

// Read the whole file at `path` into a string. Returns std::nullopt when the file
// cannot be opened or a read error occurs, so the caller can warn and fall back to
// defaults rather than acting on partial data. The stream closes via RAII on every
// path out.
[[nodiscard]] std::optional<std::string> read_file(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream) {
        return std::nullopt;
    }
    std::string contents{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    if (stream.bad()) {
        return std::nullopt;
    }
    return contents;
}

// Build a Settings from a parsed JSON object, applying per-field fallback for
// missing keys (j.value(key, default), mirroring the reference's data.get(key,
// default)). A present-but-malformed field aborts the conversion: an invalid
// enumerator makes <enum>_from_string return std::nullopt and a wrong JSON type
// makes nlohmann throw; either way the caller treats the file as corrupt and
// returns defaults. Returns std::nullopt for the invalid-enumerator case; nlohmann
// type errors propagate as exceptions to the caller's catch.
[[nodiscard]] std::optional<core::Settings> settings_from_json(const nlohmann::json& json) {
    const core::Settings defaults{};

    const auto theme_text = json.value(kKeyTheme, std::string{core::to_string(defaults.theme)});
    const auto hotkey_text = json.value(kKeyHotkey, std::string{core::to_string(defaults.hotkey)});

    const std::optional<core::Theme> theme = core::theme_from_string(theme_text);
    const std::optional<core::HotkeyPreset> hotkey = core::hotkey_preset_from_string(hotkey_text);
    if (!theme || !hotkey) {
        return std::nullopt;
    }

    return core::Settings{
        .theme = *theme,
        .hotkey = *hotkey,
        .first_run_completed = json.value(kKeyFirstRunCompleted, defaults.first_run_completed),
        .max_history_items = json.value(kKeyMaxHistoryItems, defaults.max_history_items),
        .auto_hide_on_copy = json.value(kKeyAutoHideOnCopy, defaults.auto_hide_on_copy)};
}

} // namespace

JsonSettingsRepository::JsonSettingsRepository(std::filesystem::path path)
    : path_{std::move(path)} {}

core::Settings JsonSettingsRepository::load() const {
    if (!std::filesystem::exists(path_)) {
        return core::Settings{};
    }

    const std::optional<std::string> contents = read_file(path_);
    if (!contents) {
        spdlog::warn("settings unreadable ({}); using defaults", path_.string());
        return core::Settings{};
    }

    try {
        const nlohmann::json json = nlohmann::json::parse(*contents);
        const std::optional<core::Settings> settings = settings_from_json(json);
        if (!settings) {
            spdlog::warn("settings malformed ({}); using defaults", path_.string());
            return core::Settings{};
        }
        return *settings;
    } catch (const std::exception& error) {
        spdlog::warn("settings unreadable ({}); using defaults", error.what());
        return core::Settings{};
    }
}

void JsonSettingsRepository::save(const core::Settings& settings) {
    const std::filesystem::path parent = path_.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    const nlohmann::json json = {{kKeyTheme, core::to_string(settings.theme)},
                                 {kKeyHotkey, core::to_string(settings.hotkey)},
                                 {kKeyFirstRunCompleted, settings.first_run_completed},
                                 {kKeyMaxHistoryItems, settings.max_history_items},
                                 {kKeyAutoHideOnCopy, settings.auto_hide_on_copy}};

    std::filesystem::path temp_path = path_;
    temp_path.replace_extension(config::kSettingsTempSuffix);

    {
        std::ofstream stream{temp_path, std::ios::binary | std::ios::trunc};
        stream << json.dump(kJsonIndent);
    }

    std::filesystem::rename(temp_path, path_);
}

} // namespace copyclip::storage
