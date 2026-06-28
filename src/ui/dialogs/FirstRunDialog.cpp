#include "ui/dialogs/FirstRunDialog.hpp"

#include "core/Hotkeys.hpp"
#include "core/Models.hpp"
#include "ui/Constants.hpp"

#include <string>
#include <utility>
#include <vector>

namespace copyclip::ui {

namespace {

constexpr int kContentSpacing = 12;

} // namespace

FirstRunDialog::FirstRunDialog(GtkWidget* parent, core::HotkeyPreset initial,
                               FinishedCallback on_finished)
    : on_finished_{std::move(on_finished)}, hotkey_row_{ADW_COMBO_ROW(adw_combo_row_new())},
      dialog_{adw_dialog_new()} {
    adw_dialog_set_title(dialog_, "Welcome");
    adw_dialog_set_content_width(dialog_, kDialogContentWidth);

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), adw_header_bar_new());

    GtkWidget* status = adw_status_page_new();
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(status), "edit-paste-symbolic");
    adw_status_page_set_title(ADW_STATUS_PAGE(status), "Welcome to CopyClip");
    adw_status_page_set_description(ADW_STATUS_PAGE(status),
                                    "Your clipboard history, one shortcut away.");

    GtkWidget* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, kContentSpacing);

    GtkWidget* group = adw_preferences_group_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(hotkey_row_), "Open shortcut");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(hotkey_row_), "Press these keys to open CopyClip");
    GtkStringList* model = gtk_string_list_new(nullptr);
    const std::vector<std::pair<core::HotkeyPreset, core::HotkeySpec>> presets =
        core::all_presets();
    unsigned int selected = 0;
    for (unsigned int i = 0; i < presets.size(); ++i) {
        const std::string name = presets.at(i).second.display_name();
        gtk_string_list_append(model, name.c_str());
        if (presets.at(i).first == initial) {
            selected = i;
        }
    }
    adw_combo_row_set_model(hotkey_row_, G_LIST_MODEL(model));
    g_object_unref(model);
    adw_combo_row_set_selected(hotkey_row_, selected);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(hotkey_row_));
    gtk_box_append(GTK_BOX(content), group);

    GtkWidget* button = gtk_button_new_with_label("Get Started");
    gtk_widget_add_css_class(button, "suggested-action");
    gtk_widget_add_css_class(button, "pill");
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    g_signal_connect(button, "clicked", G_CALLBACK(&FirstRunDialog::on_get_started), this);
    gtk_box_append(GTK_BOX(content), button);

    adw_status_page_set_child(ADW_STATUS_PAGE(status), content);
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), status);
    adw_dialog_set_child(dialog_, toolbar);
    adw_dialog_present(dialog_, parent);
}

void FirstRunDialog::on_get_started(GtkButton* /*button*/, gpointer self) {
    static_cast<FirstRunDialog*>(self)->finish();
}

void FirstRunDialog::finish() {
    const unsigned int index = adw_combo_row_get_selected(hotkey_row_);
    const std::vector<std::pair<core::HotkeyPreset, core::HotkeySpec>> presets =
        core::all_presets();
    if (index < presets.size()) {
        on_finished_(presets.at(index).first);
    }
    adw_dialog_close(dialog_);
}

} // namespace copyclip::ui
