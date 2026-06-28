#include "ui/MainWindow.hpp"

#include "config/Constants.hpp"
#include "ui/Constants.hpp"
#include "ui/Theme.hpp"

#include <string>

namespace copyclip::ui {

MainWindow::MainWindow(AdwApplication* application, core::SettingsService& settings)
    : window_{ADW_APPLICATION_WINDOW(adw_application_window_new(GTK_APPLICATION(application)))} {
    const std::string title{config::kAppName};
    gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(window_), kWindowDefaultWidth, kWindowDefaultHeight);
    gtk_widget_set_size_request(GTK_WIDGET(window_), kWindowMinWidth, kWindowMinHeight);

    AdwToolbarView* toolbar = ADW_TOOLBAR_VIEW(adw_toolbar_view_new());
    adw_toolbar_view_add_top_bar(toolbar, adw_header_bar_new());

    AdwStatusPage* empty_state = ADW_STATUS_PAGE(adw_status_page_new());
    adw_status_page_set_icon_name(empty_state, "edit-paste-symbolic");
    adw_status_page_set_title(empty_state, "No clipboard history yet");
    adw_status_page_set_description(empty_state, "Copy something to get started");
    adw_toolbar_view_set_content(toolbar, GTK_WIDGET(empty_state));

    adw_application_window_set_content(window_, GTK_WIDGET(toolbar));

    apply_theme(settings.settings().theme);
}

void MainWindow::present() {
    gtk_window_present(GTK_WINDOW(window_));
}

} // namespace copyclip::ui
