#include "ui/GnomeShortcut.hpp"

#include "ui/ShortcutText.hpp"

#include <glibmm/error.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace copyclip::ui {

namespace {

constexpr const char* kMediaKeysSchema = "org.gnome.settings-daemon.plugins.media-keys";
constexpr const char* kCustomKeybindingSchema =
    "org.gnome.settings-daemon.plugins.media-keys.custom-keybinding";
constexpr const char* kKeybindingPath =
    "/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/copyclip/";
constexpr const char* kShortcutName = "CopyClip";

[[nodiscard]] bool gsettings_available() {
    return !Glib::find_program_in_path("gsettings").empty();
}

// Wrap a value as a GVariant string literal (CopyClip -> 'CopyClip'). Commands
// and accelerators never contain a single quote.
[[nodiscard]] std::string as_gvariant_string(const std::string& value) {
    return "'" + value + "'";
}

// Run `gsettings <args...>`, capturing stdout into `output` when given. Returns
// true only on a clean exit.
bool run_gsettings(const std::vector<std::string>& args, std::string* output) {
    std::vector<std::string> argv;
    argv.reserve(args.size() + 1);
    argv.emplace_back("gsettings");
    argv.insert(argv.end(), args.begin(), args.end());

    int wait_status = 0;
    try {
        Glib::spawn_sync("", argv, Glib::SpawnFlags::SEARCH_PATH, {}, output, nullptr,
                         &wait_status);
    } catch (const Glib::Error&) {
        return false;
    }
    return wait_status == 0;
}

[[nodiscard]] std::string trim_trailing_whitespace(std::string value) {
    while (!value.empty() &&
           (value.back() == '\n' || value.back() == '\r' || value.back() == ' ')) {
        value.pop_back();
    }
    return value;
}

[[nodiscard]] std::vector<std::string> custom_keybinding_paths() {
    std::string current;
    if (!run_gsettings({"get", kMediaKeysSchema, "custom-keybindings"}, &current)) {
        return {};
    }
    return parse_keybinding_paths(trim_trailing_whitespace(current));
}

bool set_custom_keybinding_paths(const std::vector<std::string>& paths) {
    return run_gsettings(
        {"set", kMediaKeysSchema, "custom-keybindings", build_keybinding_array(paths)}, nullptr);
}

} // namespace

std::string executable_path() {
    std::error_code error;
    const std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", error);
    return error ? std::string{} : exe.string();
}

bool register_gnome_shortcut(const std::string& command, const std::string& accelerator) {
    if (command.empty() || accelerator.empty() || !gsettings_available()) {
        return false;
    }
    std::vector<std::string> paths = custom_keybinding_paths();
    if (std::find(paths.begin(), paths.end(), kKeybindingPath) == paths.end()) {
        paths.emplace_back(kKeybindingPath);
    }
    const std::string binding_schema = std::string{kCustomKeybindingSchema} + ":" + kKeybindingPath;

    const bool ok =
        set_custom_keybinding_paths(paths) &&
        run_gsettings({"set", binding_schema, "name", as_gvariant_string(kShortcutName)},
                      nullptr) &&
        run_gsettings({"set", binding_schema, "command", as_gvariant_string(command)}, nullptr) &&
        run_gsettings({"set", binding_schema, "binding", as_gvariant_string(accelerator)}, nullptr);
    if (!ok) {
        // A half-written entry (path listed but name/command/binding unset) would
        // read as "registered" while doing nothing — roll the whole thing back.
        spdlog::warn("global shortcut registration incomplete; rolling back");
        unregister_gnome_shortcut();
        return false;
    }
    return true;
}

bool unregister_gnome_shortcut() {
    if (!gsettings_available()) {
        return false;
    }
    std::vector<std::string> paths = custom_keybinding_paths();
    const auto removed = std::remove(paths.begin(), paths.end(), kKeybindingPath);
    if (removed == paths.end()) {
        return true; // already absent
    }
    paths.erase(removed, paths.end());
    return set_custom_keybinding_paths(paths);
}

bool is_gnome_shortcut_registered() {
    if (!gsettings_available()) {
        return false;
    }
    const std::vector<std::string> paths = custom_keybinding_paths();
    return std::find(paths.begin(), paths.end(), kKeybindingPath) != paths.end();
}

} // namespace copyclip::ui
