#pragma once

// Registers (or refreshes) a GNOME custom keybinding via gsettings so the running
// app is summoned by the preset's key combo. GNOME's settings-daemon owns the
// grab, so it works under Wayland too. Other custom shortcuts are preserved.

#include "core/Enums.hpp"

#include <string>

namespace copyclip::ui {

// Absolute path of the running executable — the command GNOME runs on the
// keypress. Empty if it cannot be resolved.
[[nodiscard]] std::string executable_path();

// Bind `preset` to run `command`. Returns false (a no-op) when gsettings is
// unavailable, e.g. on non-GNOME desktops.
bool register_gnome_shortcut(const std::string& command, core::HotkeyPreset preset);

} // namespace copyclip::ui
