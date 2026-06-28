#include "ui/ShortcutText.hpp"

#include <algorithm>
#include <vector>

namespace copyclip::ui {

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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): names disambiguate the order
std::string with_keybinding_path(std::string_view existing_list, std::string_view path) {
    // Pull out every single-quoted entry already in the array.
    std::vector<std::string> paths;
    for (std::size_t open = existing_list.find('\''); open != std::string_view::npos;) {
        const std::size_t close = existing_list.find('\'', open + 1);
        if (close == std::string_view::npos) {
            break;
        }
        paths.emplace_back(existing_list.substr(open + 1, close - open - 1));
        open = existing_list.find('\'', close + 1);
    }

    if (std::find(paths.begin(), paths.end(), path) == paths.end()) {
        paths.emplace_back(path);
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
