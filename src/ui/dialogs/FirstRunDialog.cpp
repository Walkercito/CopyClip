#include "ui/dialogs/FirstRunDialog.hpp"

#include "ui/Constants.hpp"

#include <memory>
#include <string>
#include <utility>

namespace copyclip::ui {

namespace {

constexpr int kContentSpacing = 12;

} // namespace

FirstRunDialog::FirstRunDialog(GtkWidget* parent, std::string initial_accelerator,
                               FinishedCallback on_finished)
    : on_finished_{std::move(on_finished)}, accelerator_{std::move(initial_accelerator)},
      dialog_{adw_dialog_new()} {
    adw_dialog_set_title(dialog_, "Welcome");
    adw_dialog_set_content_width(dialog_, kDialogContentWidth);
    adw_dialog_set_presentation_mode(dialog_, ADW_DIALOG_BOTTOM_SHEET);

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), adw_header_bar_new());

    GtkWidget* status = adw_status_page_new();
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(status), "edit-paste-symbolic");
    adw_status_page_set_title(ADW_STATUS_PAGE(status), "Welcome to CopyClip");
    adw_status_page_set_description(ADW_STATUS_PAGE(status),
                                    "Your clipboard history, one shortcut away.");

    GtkWidget* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, kContentSpacing);

    GtkWidget* group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Open shortcut");
    // The chooser presents its capture sheet on the same window, over the welcome
    // dialog. Its callback tracks the chosen accelerator for finish().
    chooser_ = std::make_unique<ShortcutChooser>(
        parent, ADW_PREFERENCES_GROUP(group), accelerator_,
        [this](const std::string& accelerator) { accelerator_ = accelerator; });
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
    // Complete first run however the dialog is closed (Get Started, the close
    // button, click-outside, or Escape) so the welcome never reappears.
    g_signal_connect(dialog_, "closed", G_CALLBACK(&FirstRunDialog::on_closed), this);
    adw_dialog_present(dialog_, parent);
}

void FirstRunDialog::on_get_started(GtkButton* /*button*/, gpointer self) {
    adw_dialog_close(static_cast<FirstRunDialog*>(self)->dialog_);
}

void FirstRunDialog::on_closed(AdwDialog* /*dialog*/, gpointer self) {
    static_cast<FirstRunDialog*>(self)->finish();
}

void FirstRunDialog::finish() {
    on_finished_(accelerator_);
}

} // namespace copyclip::ui
