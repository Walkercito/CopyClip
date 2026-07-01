#pragma once

// Registers (or refreshes) a GNOME custom keybinding via gsettings so the running
// app is summoned by the given key combo. GNOME's settings-daemon owns the grab,
// so it works under Wayland too. Other custom shortcuts are preserved.

#include <string>

namespace copyclip::ui {

// Absolute path of the running executable — the command GNOME runs on the
// keypress. Empty if it cannot be resolved.
[[nodiscard]] std::string executable_path();

// Bind `accelerator` (a GNOME accelerator, e.g. "<Super>v") to run `command`.
// Returns false (a no-op) when gsettings is unavailable, e.g. on non-GNOME
// desktops.
bool register_gnome_shortcut(const std::string& command, const std::string& accelerator);

// Remove CopyClip's keybinding, leaving the user's other shortcuts intact.
bool unregister_gnome_shortcut();

// Whether CopyClip's keybinding is currently present in gsettings.
[[nodiscard]] bool is_gnome_shortcut_registered();

} // namespace copyclip::ui
