#pragma once

// Pure helpers for desktop-shortcut registration — no GTK, so they unit-test
// headless. The GNOME side (GnomeShortcut) shells out to gsettings using these.

#include "core/Enums.hpp"

#include <string>
#include <string_view>

namespace copyclip::ui {

// The GNOME accelerator string for a preset, e.g. SuperV -> "<Super>v".
[[nodiscard]] std::string accelerator_for(core::HotkeyPreset preset);

// Return the gsettings custom-keybindings array `existing_list` with `path`
// appended if it isn't already present, preserving any other entries so a user's
// own shortcuts are never clobbered. Accepts "[]", "@as []", or "['/a/', '/b/']".
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): names disambiguate the order
[[nodiscard]] std::string with_keybinding_path(std::string_view existing_list,
                                               std::string_view path);

} // namespace copyclip::ui
