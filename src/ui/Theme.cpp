#include "ui/Theme.hpp"

#include <adwaita.h>

namespace copyclip::ui {

namespace {

[[nodiscard]] AdwColorScheme to_color_scheme(core::Theme theme) {
    switch (theme) {
    case core::Theme::Dark:
        return ADW_COLOR_SCHEME_FORCE_DARK;
    case core::Theme::Light:
        return ADW_COLOR_SCHEME_FORCE_LIGHT;
    case core::Theme::System:
        return ADW_COLOR_SCHEME_DEFAULT;
    }
    return ADW_COLOR_SCHEME_DEFAULT;
}

} // namespace

void apply_theme(core::Theme theme) {
    adw_style_manager_set_color_scheme(adw_style_manager_get_default(), to_color_scheme(theme));
}

} // namespace copyclip::ui
