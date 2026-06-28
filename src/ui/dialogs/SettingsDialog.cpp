#include "ui/dialogs/SettingsDialog.hpp"

#include "core/Enums.hpp"
#include "core/Hotkeys.hpp"
#include "core/Models.hpp"
#include "ui/Constants.hpp"
#include "ui/GnomeShortcut.hpp"

#include <adwaita.h>

#include <array>
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
                               ThemeChangedCallback on_theme_changed)
    : settings_{settings}, on_theme_changed_{std::move(on_theme_changed)} {
    const core::Settings& current = settings.settings();

    auto* dialog = ADW_PREFERENCES_DIALOG(adw_preferences_dialog_new());
    adw_dialog_set_content_width(ADW_DIALOG(dialog), kDialogContentWidth);
    auto* page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
    adw_preferences_dialog_add(dialog, page);

    AdwComboRow* theme_row = add_combo_row(add_group(page, "Appearance"), "Theme");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(theme_row), "Light, dark, or match the system");
    fill_combo(theme_row, {"Follow system", "Light", "Dark"}, theme_index(current.theme));
    g_signal_connect(theme_row, "notify::selected", G_CALLBACK(&SettingsDialog::on_theme_selected),
                     this);

    AdwPreferencesGroup* shortcut_group = add_group(page, "Shortcut");

    auto* shortcut_enabled_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(shortcut_enabled_row), "Global shortcut");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(shortcut_enabled_row),
                                "Open CopyClip from anywhere with a keyboard shortcut");
    adw_switch_row_set_active(shortcut_enabled_row,
                              static_cast<gboolean>(is_gnome_shortcut_registered()));
    adw_preferences_group_add(shortcut_group, GTK_WIDGET(shortcut_enabled_row));
    g_signal_connect(shortcut_enabled_row, "notify::active",
                     G_CALLBACK(&SettingsDialog::on_shortcut_toggled), this);

    AdwComboRow* hotkey_row = add_combo_row(shortcut_group, "Open CopyClip");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(hotkey_row),
                                "Key combination that summons the window");
    std::vector<std::string> hotkey_names;
    unsigned int hotkey_selected = 0;
    const std::vector<std::pair<core::HotkeyPreset, core::HotkeySpec>> presets =
        core::all_presets();
    for (unsigned int i = 0; i < presets.size(); ++i) {
        hotkey_names.push_back(presets.at(i).second.display_name());
        if (presets.at(i).first == current.hotkey) {
            hotkey_selected = i;
        }
    }
    fill_combo(hotkey_row, hotkey_names, hotkey_selected);
    g_signal_connect(hotkey_row, "notify::selected",
                     G_CALLBACK(&SettingsDialog::on_hotkey_selected), this);

    auto* auto_hide_row = ADW_SWITCH_ROW(adw_switch_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(auto_hide_row), "Hide after copying");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(auto_hide_row),
                                "Hide the window right after you pick a clip");
    adw_switch_row_set_active(auto_hide_row, static_cast<gboolean>(current.auto_hide_on_copy));
    adw_preferences_group_add(add_group(page, "Behaviour"), GTK_WIDGET(auto_hide_row));
    g_signal_connect(auto_hide_row, "notify::active",
                     G_CALLBACK(&SettingsDialog::on_auto_hide_toggled), this);

    adw_dialog_present(ADW_DIALOG(dialog), parent);
}

void SettingsDialog::on_theme_selected(GObject* row, GParamSpec* /*spec*/, gpointer self) {
    static_cast<SettingsDialog*>(self)->apply_theme(adw_combo_row_get_selected(ADW_COMBO_ROW(row)));
}

void SettingsDialog::on_hotkey_selected(GObject* row, GParamSpec* /*spec*/, gpointer self) {
    static_cast<SettingsDialog*>(self)->apply_hotkey(
        adw_combo_row_get_selected(ADW_COMBO_ROW(row)));
}

void SettingsDialog::on_shortcut_toggled(GObject* row, GParamSpec* /*spec*/, gpointer self) {
    static_cast<SettingsDialog*>(self)->apply_shortcut_enabled(
        adw_switch_row_get_active(ADW_SWITCH_ROW(row)) != FALSE);
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

void SettingsDialog::apply_hotkey(unsigned int index) {
    const std::vector<std::pair<core::HotkeyPreset, core::HotkeySpec>> presets =
        core::all_presets();
    if (index >= presets.size()) {
        return;
    }
    core::Settings updated = settings_.get().settings();
    updated.hotkey = presets.at(index).first;
    settings_.get().update(updated);
    // Only refresh the binding if the shortcut is currently enabled.
    if (is_gnome_shortcut_registered()) {
        register_gnome_shortcut(executable_path(), updated.hotkey);
    }
}

void SettingsDialog::apply_shortcut_enabled(bool active) {
    if (active) {
        register_gnome_shortcut(executable_path(), settings_.get().settings().hotkey);
    } else {
        unregister_gnome_shortcut();
    }
}

void SettingsDialog::apply_auto_hide(bool active) {
    core::Settings updated = settings_.get().settings();
    updated.auto_hide_on_copy = active;
    settings_.get().update(updated);
}

} // namespace copyclip::ui
