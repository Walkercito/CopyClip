#pragma once

// Builds and presents a plain AdwDialog (as a bottom sheet, like the welcome
// dialog — an AdwPreferencesDialog floats centered on a fixed-size window) holding
// theme, shortcuts, and behaviour rows, persisting changes through the
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
    using PanelIconChangedCallback = std::function<void()>;
    using ClosedCallback = std::function<void()>;

    SettingsDialog(GtkWidget* parent, core::SettingsService& settings,
                   ThemeChangedCallback on_theme_changed,
                   PanelIconChangedCallback on_panel_icon_changed, ClosedCallback on_closed);
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
    static void on_panel_icon_toggled(GObject* row, GParamSpec* spec, gpointer self);
    static void on_dialog_closed(AdwDialog* dialog, gpointer self);

    void apply_theme(unsigned int index);
    void apply_open_accelerator(const std::string& accelerator);
    void apply_paste_accelerator(const std::string& accelerator);
    void apply_pin_accelerator(const std::string& accelerator);
    void apply_shortcut_enabled(bool active);
    void apply_auto_hide(bool active);
    void apply_auto_paste(bool active);
    void apply_panel_icon(bool active);

    // Shared "read-modify-write" for string settings fields (paste/pin/open).
    void update_string(std::string core::Settings::*field, const std::string& value);
    // Toast on the dialog's overlay (collision warnings, etc.).
    void show_toast(const char* title);

    std::reference_wrapper<core::SettingsService> settings_;
    ThemeChangedCallback on_theme_changed_;
    PanelIconChangedCallback on_panel_icon_changed_;
    ClosedCallback on_closed_;
    AdwToastOverlay* toast_overlay_ = nullptr; // dialog child; presents AdwToasts
    // Three choosers share ShortcutChooser; kept alive for the dialog's lifetime.
    std::unique_ptr<ShortcutChooser> open_chooser_;
    std::unique_ptr<ShortcutChooser> paste_chooser_;
    std::unique_ptr<ShortcutChooser> pin_chooser_;
};

} // namespace copyclip::ui
