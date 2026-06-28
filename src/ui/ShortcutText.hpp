#pragma once

// Pure helpers for desktop-shortcut registration — no GTK, so they unit-test
// headless. The GNOME side (GnomeShortcut) shells out to gsettings using these.

#include "core/Enums.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace copyclip::ui {

// The GNOME accelerator string for a preset, e.g. SuperV -> "<Super>v".
[[nodiscard]] std::string accelerator_for(core::HotkeyPreset preset);

// Parse a gsettings string-array value ("[]", "@as []", or "['/a/', '/b/']")
// into its entries.
[[nodiscard]] std::vector<std::string> parse_keybinding_paths(std::string_view list);

// Render entries back into a gsettings string-array value ("@as []" when empty).
[[nodiscard]] std::string build_keybinding_array(const std::vector<std::string>& paths);

} // namespace copyclip::ui
