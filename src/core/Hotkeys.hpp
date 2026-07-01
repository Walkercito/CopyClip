#pragma once

// Catalog of supported hotkey presets as pure data: a fixed HotkeyPreset ->
// HotkeySpec mapping plus a default. Mirrors reference core/hotkeys.py. The
// catalog is defined once in Hotkeys.cpp; both readers share that source.

#include "core/Enums.hpp"
#include "core/Models.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace copyclip::core {

// The preset selected when none is configured (reference DEFAULT_PRESET).
inline constexpr HotkeyPreset kDefaultPreset = HotkeyPreset::SuperV;

// The GNOME accelerator string for a preset, e.g. SuperV -> "<Super>v". Lives
// in core (pure data) so the storage layer can migrate legacy settings without
// depending on the UI.
[[nodiscard]] std::string accelerator_for(HotkeyPreset preset);

// Resolve a stored hotkey value to a GNOME accelerator. Legacy builds stored a
// preset token ("super_v"); current builds store the accelerator directly. An
// empty value falls back to the default shortcut.
[[nodiscard]] std::string accelerator_from_stored(std::string_view stored);

// Spec for a preset. The enum is closed and the catalog exhaustive, so this
// always resolves; a bogus cast throws std::out_of_range (the reference's
// KeyError analogue) rather than returning a wrong/empty spec.
[[nodiscard]] HotkeySpec get_spec(HotkeyPreset preset);

// Copy of the full catalog in HotkeyPreset declaration order (reference
// all_presets()).
[[nodiscard]] std::vector<std::pair<HotkeyPreset, HotkeySpec>> all_presets();

} // namespace copyclip::core
