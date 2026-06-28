#include "ui/MainWindow.hpp"

#include "config/Constants.hpp"
#include "ui/Constants.hpp"
#include "ui/Theme.hpp"
#include "ui/widgets/ClipCard.hpp"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/widget.h>

#include <glibmm/main.h>
#include <glibmm/ustring.h>

#include <string>
#include <vector>

namespace copyclip::ui {

MainWindow::MainWindow(GtkApplication* application, core::HistoryService& history,
                       core::SettingsService& settings, core::ClipboardSource& clipboard)
    : history_{history}, clipboard_{clipboard} {
    build_ui(application);
    history_.get().subscribe([this] { schedule_refresh(); });
    apply_theme(settings.settings().theme);
    refresh();
}

void MainWindow::build_ui(GtkApplication* application) {
    window_ = ADW_APPLICATION_WINDOW(adw_application_window_new(application));
    const std::string title{config::kAppName};
    gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(window_), kWindowDefaultWidth, kWindowDefaultHeight);
    gtk_widget_set_size_request(GTK_WIDGET(window_), kWindowMinWidth, kWindowMinHeight);

    AdwToolbarView* toolbar = ADW_TOOLBAR_VIEW(adw_toolbar_view_new());
    AdwHeaderBar* header = ADW_HEADER_BAR(adw_header_bar_new());

    auto* clear_button = Gtk::make_managed<Gtk::Button>();
    clear_button->set_icon_name("user-trash-symbolic");
    clear_button->set_tooltip_text("Clear unpinned history");
    clear_button->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::clear_history));
    adw_header_bar_pack_end(header, GTK_WIDGET(clear_button->gobj()));
    adw_toolbar_view_add_top_bar(toolbar, GTK_WIDGET(header));

    auto* content = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, kContentMargin);
    content->set_margin(kContentMargin);

    search_ = Gtk::make_managed<Gtk::SearchEntry>();
    search_->set_placeholder_text("Search clipboard history…");
    search_->signal_search_changed().connect([this] {
        search_text_ = search_->get_text().raw();
        refresh();
    });
    content->append(*search_);

    auto* scrolled = Gtk::make_managed<Gtk::ScrolledWindow>();
    scrolled->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scrolled->set_vexpand(true);

    list_ = Gtk::make_managed<Gtk::ListBox>();
    list_->set_selection_mode(Gtk::SelectionMode::NONE);
    list_->add_css_class("boxed-list");
    list_->set_valign(Gtk::Align::START);

    placeholder_ = ADW_STATUS_PAGE(adw_status_page_new());
    adw_status_page_set_icon_name(placeholder_, "edit-paste-symbolic");
    list_->set_placeholder(*Glib::wrap(GTK_WIDGET(placeholder_)));

    scrolled->set_child(*list_);
    content->append(*scrolled);

    adw_toolbar_view_set_content(toolbar, GTK_WIDGET(content->gobj()));
    adw_application_window_set_content(window_, GTK_WIDGET(toolbar));
}

void MainWindow::schedule_refresh() {
    if (refresh_pending_) {
        return;
    }
    refresh_pending_ = true;
    Glib::signal_idle().connect_once([this] { refresh(); });
}

void MainWindow::refresh() {
    refresh_pending_ = false;
    while (Gtk::Widget* child = list_->get_first_child()) {
        list_->remove(*child);
    }

    const std::vector<core::ClipboardEntry> entries = history_.get().entries();
    for (const core::ClipboardEntry& entry : entries) {
        if (!matches(entry.content)) {
            continue;
        }
        list_->append(*Gtk::make_managed<ClipCard>(
            entry, kMaxPreviewChars, [this](const std::string& content) { copy(content); },
            [this](const std::string& content) { pin(content); }));
    }

    if (entries.empty()) {
        adw_status_page_set_title(placeholder_, "No clipboard history yet");
        adw_status_page_set_description(placeholder_, "Copy something to get started");
    } else {
        adw_status_page_set_title(placeholder_, "No results");
        adw_status_page_set_description(placeholder_, "Nothing matches your search");
    }
}

void MainWindow::copy(const std::string& content) {
    clipboard_.get().write(content);
    history_.get().add(content);
}

void MainWindow::pin(const std::string& content) {
    history_.get().toggle_pin(content);
}

void MainWindow::clear_history() {
    history_.get().clear_unpinned();
}

bool MainWindow::matches(const std::string& content) const {
    if (search_text_.empty()) {
        return true;
    }
    return Glib::ustring{content}.lowercase().find(Glib::ustring{search_text_}.lowercase()) !=
           Glib::ustring::npos;
}

void MainWindow::present() {
    gtk_window_present(GTK_WINDOW(window_));
}

} // namespace copyclip::ui
