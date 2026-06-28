#pragma once

// Builds and presents an AdwPreferencesDialog for theme, the open shortcut, and
// behaviour, persisting changes through the SettingsService. Applying the theme
// live is delegated to a callback (the window owns the style manager). libadwaita
// has no C++ binding, so the rows are driven through its C API.

#include "core/SettingsService.hpp"

#include <gtk/gtk.h>

#include <functional>

namespace copyclip::ui {

class SettingsDialog {
public:
    using ThemeChangedCallback = std::function<void()>;

    SettingsDialog(GtkWidget* parent, core::SettingsService& settings,
                   ThemeChangedCallback on_theme_changed);
    ~SettingsDialog() = default;

    SettingsDialog(const SettingsDialog&) = delete;
    SettingsDialog& operator=(const SettingsDialog&) = delete;
    SettingsDialog(SettingsDialog&&) = delete;
    SettingsDialog& operator=(SettingsDialog&&) = delete;

private:
    static void on_theme_selected(GObject* row, GParamSpec* spec, gpointer self);
    static void on_hotkey_selected(GObject* row, GParamSpec* spec, gpointer self);
    static void on_auto_hide_toggled(GObject* row, GParamSpec* spec, gpointer self);

    void apply_theme(unsigned int index);
    void apply_hotkey(unsigned int index);
    void apply_auto_hide(bool active);

    std::reference_wrapper<core::SettingsService> settings_;
    ThemeChangedCallback on_theme_changed_;
};

} // namespace copyclip::ui
