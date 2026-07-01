#include "storage/JsonSettingsRepository.hpp"

#include "config/Constants.hpp"
#include "core/Enums.hpp"
#include "core/Hotkeys.hpp"
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

// Named so no bare string literal appears in the logic; match the reference's
// dataclass field names verbatim (it serializes via dataclasses.asdict()).
constexpr const char* kKeyTheme = "theme";
constexpr const char* kKeyHotkey = "hotkey";
constexpr const char* kKeyFirstRunCompleted = "first_run_completed";
constexpr const char* kKeyMaxHistoryItems = "max_history_items";
constexpr const char* kKeyAutoHideOnCopy = "auto_hide_on_copy";
constexpr const char* kKeyAutoPaste = "auto_paste";

// Indentation width for the serialized JSON, mirroring the reference's
// json.dumps(..., indent=2).
constexpr int kJsonIndent = 2;

// Reads the whole file into a string; std::nullopt if it cannot be opened or a
// read error occurs, so the caller falls back to defaults rather than acting on
// partial data.
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

// Builds Settings from parsed JSON, with per-field fallback for missing keys
// (mirroring the reference's data.get(key, default)). A present-but-malformed
// field instead aborts to defaults — diverging from the reference: an invalid
// enumerator returns std::nullopt here, while a wrong JSON type makes nlohmann
// throw and propagates to the caller's catch.
[[nodiscard]] std::optional<core::Settings> settings_from_json(const nlohmann::json& json) {
    const core::Settings defaults{};

    const auto theme_text = json.value(kKeyTheme, std::string{core::to_string(defaults.theme)});
    const auto hotkey_text = json.value(kKeyHotkey, defaults.hotkey);

    const std::optional<core::Theme> theme = core::theme_from_string(theme_text);
    if (!theme) {
        return std::nullopt;
    }

    return core::Settings{
        .theme = *theme,
        // Accept both legacy preset tokens ("super_v") and raw accelerators.
        .hotkey = core::accelerator_from_stored(hotkey_text),
        .first_run_completed = json.value(kKeyFirstRunCompleted, defaults.first_run_completed),
        .max_history_items = json.value(kKeyMaxHistoryItems, defaults.max_history_items),
        .auto_hide_on_copy = json.value(kKeyAutoHideOnCopy, defaults.auto_hide_on_copy),
        .auto_paste = json.value(kKeyAutoPaste, defaults.auto_paste)};
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
                                 {kKeyHotkey, settings.hotkey},
                                 {kKeyFirstRunCompleted, settings.first_run_completed},
                                 {kKeyMaxHistoryItems, settings.max_history_items},
                                 {kKeyAutoHideOnCopy, settings.auto_hide_on_copy},
                                 {kKeyAutoPaste, settings.auto_paste}};

    std::filesystem::path temp_path = path_;
    temp_path.replace_extension(config::kSettingsTempSuffix);

    {
        std::ofstream stream{temp_path, std::ios::binary | std::ios::trunc};
        stream << json.dump(kJsonIndent);
    }

    std::filesystem::rename(temp_path, path_);
}

} // namespace copyclip::storage
