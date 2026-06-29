#pragma once

// Pure helpers for desktop-shortcut registration — no GTK, so they unit-test
// headless. The GNOME side (GnomeShortcut) shells out to gsettings using these.

#include "core/Enums.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace copyclip::ui {

// The GNOME accelerator string for a preset, e.g. SuperV -> "<Super>v".
[[nodiscard]] std::string accelerator_for(core::HotkeyPreset preset);

// Display names of every hotkey preset, in preset order — the model for a combo
// row. Paired with index_of_preset / preset_at so the two dialogs share one source
// of truth instead of each re-deriving it from all_presets().
[[nodiscard]] std::vector<std::string> hotkey_display_names();

// The combo index of `preset` (0 if it is somehow absent).
[[nodiscard]] unsigned int index_of_preset(core::HotkeyPreset preset);

// The preset at combo index `index`, or nullopt if out of range.
[[nodiscard]] std::optional<core::HotkeyPreset> preset_at(unsigned int index);

// Parse a gsettings string-array value ("[]", "@as []", or "['/a/', '/b/']")
// into its entries.
[[nodiscard]] std::vector<std::string> parse_keybinding_paths(std::string_view list);

// Render entries back into a gsettings string-array value ("@as []" when empty).
[[nodiscard]] std::string build_keybinding_array(const std::vector<std::string>& paths);

} // namespace copyclip::ui
