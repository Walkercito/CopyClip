#include "ui/dialogs/SettingsDialog.hpp"

#include "core/Enums.hpp"
#include "core/Models.hpp"
#include "ui/Constants.hpp"
#include "ui/GnomeShortcut.hpp"

#include <adwaita.h>

#include <spdlog/spdlog.h>

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace copyclip::ui {

namespace {

// Theme combo options, in display order; the index maps to a core::Theme.
constexpr std::array<core::Theme, 3> kThemeOrder{core::Theme::System, core::Theme::Light,
                                                 core::Theme::Dark};

[[nodiscard]] unsigned int theme_index(core::Theme theme) {
    for (unsigned int i = 0; i < kThemeOrder.size(); ++i) {
        if (kThemeOrder.at(i) == theme) {
            return i;
        }
    }
    return 0;
}

// Populate a combo row with `options` and select `selected`, before signals are
// connected. The row keeps its own reference to the model.
void fill_combo(AdwComboRow* row, const std::vector<std::string>& options, unsigned int selected) {
    GtkStringList* model = gtk_string_list_new(nullptr);
    for (const std::string& option : options) {
        gtk_string_list_append(model, option.c_str());
    }
    adw_combo_row_set_model(row, G_LIST_MODEL(model));
    g_object_unref(model);
    adw_combo_row_set_selected(row, selected);
}

[[nodiscard]] AdwComboRow* add_combo_row(AdwPreferencesGroup* group, const char* title) {
    auto* row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
    adw_preferences_group_add(group, GTK_WIDGET(row));
    return row;
}

[[nodiscard]] AdwPreferencesGroup* add_group(AdwPreferencesPage* page, const char* title) {
    auto* group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, title);
    adw_preferences_page_add(page, group);
    return group;
}

} // namespace

SettingsDialog::SettingsDialog(GtkWidget* parent, core::SettingsService& settings,
                               ThemeChangedCallback on_theme_changed,
                               PanelIconChangedCallback on_panel_icon_changed,
                               ClosedCallback on_closed)
    : settings_{settings}, on_theme_changed_{std::move(on_theme_changed)},
      on_panel_icon_changed_{std::move(on_panel_icon_changed)}, on_closed_{std::move(on_closed)} {
    const core::Settings& current = settings.settings();

    // A plain AdwDialog (not AdwPreferencesDialog) so it presents as a bottom sheet
    // like the welcome dialog, instead of floating centered.
    AdwDialog* dialog = adw_dialog_new();
    adw_dialog_set_title(dialog, "Settings");
    adw_dialog_set_content_width(dialog, kDialogContentWidth);
    adw_dialog_set_presentation_mode(dialog, ADW_DIALOG_BOTTOM_SHEET);
    auto* page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());

    AdwComboRow* theme_row = add_combo_row(add_group(page, "Appearance"), "Theme");
    gtk_widget_set_tooltip_text(GTK_WIDGET(theme_row), "Light, dark, or match the system");
    fill_combo(theme_row, {"Follow system", "Light", "Dark"}, theme_index(current.theme));
    g_signal_connect(theme_row, "notify::selected", G_CALLBACK(&SettingsDialog::on_theme_selected),
                     this);

    AdwPreferencesGroup* shortcut_group = add_group(page, "Shortcut");

    auto* shortcut_enabled_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(shortcut_enabled_row), "Global shortcut");
    gtk_widget_set_tooltip_text(GTK_WIDGET(shortcut_enabled_row),
                                "Open CopyClip from anywhere with a keyboard shortcut");
    adw_switch_row_set_active(shortcut_enabled_row,
                              static_cast<gboolean>(is_gnome_shortcut_registered()));
    adw_preferences_group_add(shortcut_group, GTK_WIDGET(shortcut_enabled_row));
    g_signal_connect(shortcut_enabled_row, "notify::active",
                     G_CALLBACK(&SettingsDialog::on_shortcut_toggled), this);

    // Free-form shortcut capture plus preset quick-picks; applies on change.
    shortcut_chooser_ = std::make_unique<ShortcutChooser>(
        parent, shortcut_group, current.hotkey,
        [this](const std::string& accelerator) { apply_accelerator(accelerator); });

    AdwPreferencesGroup* behaviour_group = add_group(page, "Behaviour");

    auto* auto_hide_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(auto_hide_row), "Hide after copying");
    gtk_widget_set_tooltip_text(GTK_WIDGET(auto_hide_row),
                                "Hide the window right after you pick a clip");
    adw_switch_row_set_active(auto_hide_row, static_cast<gboolean>(current.auto_hide_on_copy));
    adw_preferences_group_add(behaviour_group, GTK_WIDGET(auto_hide_row));
    g_signal_connect(auto_hide_row, "notify::active",
                     G_CALLBACK(&SettingsDialog::on_auto_hide_toggled), this);

    auto* auto_paste_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(auto_paste_row), "Auto-paste");
    gtk_widget_set_tooltip_text(GTK_WIDGET(auto_paste_row),
                                "Paste the clip into the focused window after copying");
    adw_switch_row_set_active(auto_paste_row, static_cast<gboolean>(current.auto_paste));
    adw_preferences_group_add(behaviour_group, GTK_WIDGET(auto_paste_row));
    g_signal_connect(auto_paste_row, "notify::active",
                     G_CALLBACK(&SettingsDialog::on_auto_paste_toggled), this);

    auto* panel_icon_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(panel_icon_row), "Show panel icon");
    gtk_widget_set_tooltip_text(
        GTK_WIDGET(panel_icon_row),
        "Show a panel icon to open CopyClip (needs a tray/AppIndicator host)");
    adw_switch_row_set_active(panel_icon_row, static_cast<gboolean>(current.show_panel_icon));
    adw_preferences_group_add(behaviour_group, GTK_WIDGET(panel_icon_row));
    g_signal_connect(panel_icon_row, "notify::active",
                     G_CALLBACK(&SettingsDialog::on_panel_icon_toggled), this);

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), adw_header_bar_new());
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), GTK_WIDGET(page));
    adw_dialog_set_child(dialog, toolbar);
    g_signal_connect(dialog, "closed", G_CALLBACK(&SettingsDialog::on_dialog_closed), this);
    adw_dialog_present(dialog, parent);
}

void SettingsDialog::on_dialog_closed(AdwDialog* /*dialog*/, gpointer self) {
    static_cast<SettingsDialog*>(self)->on_closed_();
}

void SettingsDialog::on_theme_selected(GObject* row, GParamSpec* /*spec*/, gpointer self) {
    static_cast<SettingsDialog*>(self)->apply_theme(adw_combo_row_get_selected(ADW_COMBO_ROW(row)));
}

void SettingsDialog::on_shortcut_toggled(GObject* row, GParamSpec* /*spec*/, gpointer self) {
    static_cast<SettingsDialog*>(self)->apply_shortcut_enabled(
        adw_switch_row_get_active(ADW_SWITCH_ROW(row)) != FALSE);
    // Registration can fail (e.g. off GNOME); make the switch reflect what actually
    // happened rather than the user's intent, so it can't show "on" while unbound.
    adw_switch_row_set_active(ADW_SWITCH_ROW(row), is_gnome_shortcut_registered() ? TRUE : FALSE);
}

void SettingsDialog::on_auto_hide_toggled(GObject* row, GParamSpec* /*spec*/, gpointer self) {
    static_cast<SettingsDialog*>(self)->apply_auto_hide(
        adw_switch_row_get_active(ADW_SWITCH_ROW(row)) != FALSE);
}

void SettingsDialog::apply_theme(unsigned int index) {
    core::Settings updated = settings_.get().settings();
    updated.theme = kThemeOrder.at(index < kThemeOrder.size() ? index : 0);
    settings_.get().update(updated);
    on_theme_changed_();
}

void SettingsDialog::apply_accelerator(const std::string& accelerator) {
    core::Settings updated = settings_.get().settings();
    updated.hotkey = accelerator;
    settings_.get().update(updated);
    // Only refresh the binding if the shortcut is currently enabled.
    if (is_gnome_shortcut_registered()) {
        register_gnome_shortcut(executable_path(), updated.hotkey);
    }
}

void SettingsDialog::apply_shortcut_enabled(bool active) {
    const bool ok =
        active ? register_gnome_shortcut(executable_path(), settings_.get().settings().hotkey)
               : unregister_gnome_shortcut();
    if (!ok) {
        spdlog::warn("global shortcut could not be {}", active ? "registered" : "removed");
    }
}

void SettingsDialog::apply_auto_hide(bool active) {
    core::Settings updated = settings_.get().settings();
    updated.auto_hide_on_copy = active;
    settings_.get().update(updated);
}

void SettingsDialog::on_auto_paste_toggled(GObject* row, GParamSpec* /*spec*/, gpointer self) {
    static_cast<SettingsDialog*>(self)->apply_auto_paste(
        adw_switch_row_get_active(ADW_SWITCH_ROW(row)) != FALSE);
}

void SettingsDialog::apply_auto_paste(bool active) {
    core::Settings updated = settings_.get().settings();
    updated.auto_paste = active;
    settings_.get().update(updated);
}

void SettingsDialog::on_panel_icon_toggled(GObject* row, GParamSpec* /*spec*/, gpointer self) {
    static_cast<SettingsDialog*>(self)->apply_panel_icon(
        adw_switch_row_get_active(ADW_SWITCH_ROW(row)) != FALSE);
}

void SettingsDialog::apply_panel_icon(bool active) {
    core::Settings updated = settings_.get().settings();
    updated.show_panel_icon = active;
    settings_.get().update(updated);
    on_panel_icon_changed_(); // let the window add/remove the tray icon live
}

} // namespace copyclip::ui
