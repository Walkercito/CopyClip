#include "ui/GnomeShortcut.hpp"

#include "ui/ShortcutText.hpp"

#include <glibmm/error.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>

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

// Wrap a value as a GVariant string literal for gsettings (e.g. CopyClip ->
// 'CopyClip'). Commands and accelerators never contain a single quote.
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

} // namespace

std::string executable_path() {
    std::error_code error;
    const std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", error);
    return error ? std::string{} : exe.string();
}

bool register_gnome_shortcut(const std::string& command, core::HotkeyPreset preset) {
    if (command.empty() || Glib::find_program_in_path("gsettings").empty()) {
        return false;
    }
    const std::string binding_schema = std::string{kCustomKeybindingSchema} + ":" + kKeybindingPath;

    std::string current_list;
    if (!run_gsettings({"get", kMediaKeysSchema, "custom-keybindings"}, &current_list)) {
        return false;
    }
    const std::string merged =
        with_keybinding_path(trim_trailing_whitespace(current_list), kKeybindingPath);

    return run_gsettings({"set", kMediaKeysSchema, "custom-keybindings", merged}, nullptr) &&
           run_gsettings({"set", binding_schema, "name", as_gvariant_string(kShortcutName)},
                         nullptr) &&
           run_gsettings({"set", binding_schema, "command", as_gvariant_string(command)},
                         nullptr) &&
           run_gsettings(
               {"set", binding_schema, "binding", as_gvariant_string(accelerator_for(preset))},
               nullptr);
}

} // namespace copyclip::ui
