#include "ui/ShortcutText.hpp"

#include "core/Hotkeys.hpp"

#include <utility>

namespace copyclip::ui {

std::vector<std::string> hotkey_display_names() {
    std::vector<std::string> names;
    for (const std::pair<core::HotkeyPreset, core::HotkeySpec>& entry : core::all_presets()) {
        names.push_back(entry.second.display_name());
    }
    return names;
}

unsigned int index_of_preset(core::HotkeyPreset preset) {
    const std::vector<std::pair<core::HotkeyPreset, core::HotkeySpec>> presets =
        core::all_presets();
    for (unsigned int i = 0; i < presets.size(); ++i) {
        if (presets.at(i).first == preset) {
            return i;
        }
    }
    return 0;
}

std::optional<core::HotkeyPreset> preset_at(unsigned int index) {
    const std::vector<std::pair<core::HotkeyPreset, core::HotkeySpec>> presets =
        core::all_presets();
    if (index >= presets.size()) {
        return std::nullopt;
    }
    return presets.at(index).first;
}

std::string accelerator_for(core::HotkeyPreset preset) {
    switch (preset) {
    case core::HotkeyPreset::SuperV:
        return "<Super>v";
    case core::HotkeyPreset::CtrlAltV:
        return "<Control><Alt>v";
    case core::HotkeyPreset::SuperC:
        return "<Super>c";
    case core::HotkeyPreset::CtrlShiftV:
        return "<Control><Shift>v";
    }
    return "<Super>v";
}

std::vector<std::string> parse_keybinding_paths(std::string_view list) {
    std::vector<std::string> paths;
    for (std::size_t open = list.find('\''); open != std::string_view::npos;) {
        const std::size_t close = list.find('\'', open + 1);
        if (close == std::string_view::npos) {
            break;
        }
        paths.emplace_back(list.substr(open + 1, close - open - 1));
        open = list.find('\'', close + 1);
    }
    return paths;
}

std::string build_keybinding_array(const std::vector<std::string>& paths) {
    if (paths.empty()) {
        return "@as []";
    }
    std::string result{"["};
    for (std::size_t i = 0; i < paths.size(); ++i) {
        if (i != 0) {
            result += ", ";
        }
        result += '\'';
        result += paths.at(i);
        result += '\'';
    }
    result += ']';
    return result;
}

} // namespace copyclip::ui
