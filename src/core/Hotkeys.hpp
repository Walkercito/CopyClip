#pragma once

// The catalog of supported hotkey presets, as pure data.
//
// Mirrors the reference module copyclip/core/hotkeys.py: a fixed dict mapping
// each HotkeyPreset to its HotkeySpec, a default preset, and two readers —
// get_spec(preset) and all_presets(). This lives in the core layer, so it is
// pure data with no Xlib/Qt/D-Bus: it only relates the domain enums (core/Enums)
// to the HotkeySpec value object (core/Models). The catalog itself is defined
// once in Hotkeys.cpp; both readers consume that single source of truth.

#include "core/Enums.hpp"
#include "core/Models.hpp"

#include <utility>
#include <vector>

namespace copyclip::core {

// The preset selected when none is configured (reference DEFAULT_PRESET).
inline constexpr HotkeyPreset kDefaultPreset = HotkeyPreset::SuperV;

// Return the spec for a preset (reference get_spec). The enum is closed and the
// catalog is exhaustive over it, so this always resolves; an unknown preset
// (only reachable via a bogus cast) throws std::out_of_range rather than
// returning a wrong or empty spec — the C++ analogue of the reference's KeyError.
[[nodiscard]] HotkeySpec get_spec(HotkeyPreset preset);

// Return a copy of the full catalog, one entry per preset (reference
// all_presets() == dict(_PRESETS)). The order matches the HotkeyPreset
// declaration order.
[[nodiscard]] std::vector<std::pair<HotkeyPreset, HotkeySpec>> all_presets();

} // namespace copyclip::core
