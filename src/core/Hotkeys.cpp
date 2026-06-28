#include "core/Hotkeys.hpp"

#include "core/Enums.hpp"
#include "core/Models.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace copyclip::core {

namespace {

// The single source of truth: every preset paired with its spec, in
// HotkeyPreset declaration order. Mirrors the reference _PRESETS dict exactly.
// Both get_spec and all_presets read this table, so the mapping is never
// duplicated. The HotkeySpec modifier lists own their storage, so the catalog is
// built once at first use rather than being a compile-time constant.
[[nodiscard]] const std::array<std::pair<HotkeyPreset, HotkeySpec>, 4>& catalog() {
    static const std::array<std::pair<HotkeyPreset, HotkeySpec>, 4> presets{{
        {HotkeyPreset::SuperV, HotkeySpec{.modifiers = {Modifier::Super}, .key = Key::V}},
        {HotkeyPreset::CtrlAltV,
         HotkeySpec{.modifiers = {Modifier::Ctrl, Modifier::Alt}, .key = Key::V}},
        {HotkeyPreset::SuperC, HotkeySpec{.modifiers = {Modifier::Super}, .key = Key::C}},
        {HotkeyPreset::CtrlShiftV,
         HotkeySpec{.modifiers = {Modifier::Ctrl, Modifier::Shift}, .key = Key::V}},
    }};
    return presets;
}

} // namespace

HotkeySpec get_spec(HotkeyPreset preset) {
    const auto& presets = catalog();
    const auto* const found =
        std::find_if(presets.begin(), presets.end(),
                     [preset](const auto& entry) { return entry.first == preset; });
    if (found == presets.end()) {
        throw std::out_of_range("copyclip::core::get_spec: unknown HotkeyPreset value " +
                                std::string{to_string(preset)});
    }
    return found->second;
}

std::vector<std::pair<HotkeyPreset, HotkeySpec>> all_presets() {
    const auto& presets = catalog();
    return std::vector<std::pair<HotkeyPreset, HotkeySpec>>{presets.begin(), presets.end()};
}

} // namespace copyclip::core
