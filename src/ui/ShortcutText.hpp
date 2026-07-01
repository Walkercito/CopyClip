#pragma once

// Pure helpers for desktop-shortcut registration — no GTK, so they unit-test
// headless. The GNOME side (GnomeShortcut) shells out to gsettings using these.

#include <string>
#include <string_view>
#include <vector>

namespace copyclip::ui {

// A built-in preset offered as a one-tap quick-pick in the shortcut UI.
struct QuickPick {
    std::string label;       // human label, e.g. "Super+V"
    std::string accelerator; // GNOME accelerator, e.g. "<Super>v"
};

// The built-in presets as quick-picks, in catalog order, so the capture UI can
// offer common combos alongside free-form capture.
[[nodiscard]] std::vector<QuickPick> quick_picks();

// Parse a gsettings string-array value ("[]", "@as []", or "['/a/', '/b/']")
// into its entries.
[[nodiscard]] std::vector<std::string> parse_keybinding_paths(std::string_view list);

// Render entries back into a gsettings string-array value ("@as []" when empty).
[[nodiscard]] std::string build_keybinding_array(const std::vector<std::string>& paths);

} // namespace copyclip::ui
