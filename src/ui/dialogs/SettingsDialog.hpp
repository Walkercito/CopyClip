#pragma once

// Builds and presents a plain AdwDialog (as a bottom sheet, like the welcome
// dialog — an AdwPreferencesDialog floats centered on a fixed-size window) holding
// theme, the open shortcut, and behaviour rows, persisting changes through the
// SettingsService. Applying the theme live is delegated to a callback (the window
// owns the style manager). libadwaita has no C++ binding, so the dialog and its
// rows are driven through its C API.

#include "core/SettingsService.hpp"
#include "ui/widgets/ShortcutChooser.hpp"

#include <adwaita.h>

#include <functional>
#include <memory>
#include <string>

namespace copyclip::ui {

class SettingsDialog {
public:
    using ThemeChangedCallback = std::function<void()>;
    using ClosedCallback = std::function<void()>;

    SettingsDialog(GtkWidget* parent, core::SettingsService& settings,
                   ThemeChangedCallback on_theme_changed, ClosedCallback on_closed);
    ~SettingsDialog() = default;

    SettingsDialog(const SettingsDialog&) = delete;
    SettingsDialog& operator=(const SettingsDialog&) = delete;
    SettingsDialog(SettingsDialog&&) = delete;
    SettingsDialog& operator=(SettingsDialog&&) = delete;

private:
    static void on_theme_selected(GObject* row, GParamSpec* spec, gpointer self);
    static void on_shortcut_toggled(GObject* row, GParamSpec* spec, gpointer self);
    static void on_auto_hide_toggled(GObject* row, GParamSpec* spec, gpointer self);
    static void on_auto_paste_toggled(GObject* row, GParamSpec* spec, gpointer self);
    static void on_dialog_closed(AdwDialog* dialog, gpointer self);

    void apply_theme(unsigned int index);
    void apply_accelerator(const std::string& accelerator);
    void apply_shortcut_enabled(bool active);
    void apply_auto_hide(bool active);
    void apply_auto_paste(bool active);

    std::reference_wrapper<core::SettingsService> settings_;
    ThemeChangedCallback on_theme_changed_;
    ClosedCallback on_closed_;
    std::unique_ptr<ShortcutChooser> shortcut_chooser_;
};

} // namespace copyclip::ui
