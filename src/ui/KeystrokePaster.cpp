#include "ui/KeystrokePaster.hpp"

#include <glibmm/error.h>
#include <glibmm/miscutils.h>
#include <glibmm/spawn.h>

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

namespace copyclip::ui {

namespace {

// Run `argv` (program resolved on PATH) and report a clean exit; false when the
// program is missing or fails.
bool try_run(const std::vector<std::string>& argv) {
    if (Glib::find_program_in_path(argv.front()).empty()) {
        return false;
    }
    int wait_status = 0;
    try {
        Glib::spawn_sync("", argv, Glib::SpawnFlags::SEARCH_PATH, {}, nullptr, nullptr,
                         &wait_status);
    } catch (const Glib::Error&) {
        return false;
    }
    return wait_status == 0;
}

// Paste commands to try in order for the session. Wayland prefers the virtual
// keyboard (wtype) and falls back to uinput (ydotool); X11 uses XTEST (xdotool).
[[nodiscard]] std::vector<std::vector<std::string>> paste_commands(core::SessionType session) {
    if (session == core::SessionType::Wayland) {
        return {{"wtype", "-M", "ctrl", "v", "-m", "ctrl"}, {"ydotool", "key", "ctrl+v"}};
    }
    return {{"xdotool", "key", "--clearmodifiers", "ctrl+v"}};
}

} // namespace

KeystrokePaster::KeystrokePaster(core::SessionType session) : session_{session} {}

void KeystrokePaster::paste() const {
    for (const std::vector<std::string>& argv : paste_commands(session_)) {
        if (try_run(argv)) {
            return;
        }
    }
    spdlog::debug("auto-paste: no working input tool for this session; clip left on the clipboard");
}

} // namespace copyclip::ui
