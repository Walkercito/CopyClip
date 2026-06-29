#pragma once

// Light/dark via libadwaita's default style manager. AdwStyleManager + libadwaita
// named colors keep the UI tracking the system light/dark setting without any
// custom stylesheet.

#include "core/Enums.hpp"

namespace copyclip::ui {

// Set libadwaita's color scheme from the engine Theme (System follows the desktop).
void apply_theme(core::Theme theme);

} // namespace copyclip::ui
