#pragma once

// Light/dark via libadwaita's default style manager. AdwStyleManager + named
// colors are why the GTK UI tracks the system and stays consistent, where the
// old hand-rolled stylesheets drifted.

#include "core/Enums.hpp"

namespace copyclip::ui {

// Set libadwaita's color scheme from the engine Theme (System follows the desktop).
void apply_theme(core::Theme theme);

} // namespace copyclip::ui
