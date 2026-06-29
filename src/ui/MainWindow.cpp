#include "ui/MainWindow.hpp"

#include "config/Constants.hpp"
#include "core/Platform.hpp"
#include "ui/Constants.hpp"
#include "ui/Theme.hpp"
#include "ui/widgets/ClipCard.hpp"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/scrolledwindow.h>

#include <glibmm/main.h>
#include <glibmm/ustring.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace copyclip::ui {

namespace {

constexpr int kPlaceholderIconSize = 48;
constexpr unsigned int kPasteDelayMs = 120;
constexpr const char* kPageList = "list";
constexpr const char* kPageEmpty = "empty";

// The window decoration layout with the maximize button removed (it appears at
// most once, in either button group).
[[nodiscard]] std::string layout_without_maximize(std::string layout) {
    for (const std::string_view token : {",maximize", "maximize,", "maximize"}) {
        if (const std::size_t pos = layout.find(token); pos != std::string::npos) {
            layout.erase(pos, token.size());
            break;
        }
    }
    return layout;
}

} // namespace

MainWindow::MainWindow(GtkApplication* application, core::HistoryService& history,
                       core::SettingsService& settings, core::ClipboardSource& clipboard)
    : history_{history}, clipboard_{clipboard}, settings_{settings},
      paster_{core::detect_session()} {
    build_ui(application);
    history_.get().subscribe([this] { schedule_refresh(); });
    apply_theme(settings.settings().theme);
    rebuild_cards();
}

void MainWindow::build_ui(GtkApplication* application) {
    window_ = ADW_APPLICATION_WINDOW(adw_application_window_new(application));

    // Closing hides the window so the app keeps capturing in the background.
    g_signal_connect(window_, "close-request",
                     G_CALLBACK(+[](GtkWindow* window, gpointer) -> gboolean {
                         gtk_widget_set_visible(GTK_WIDGET(window), FALSE);
                         return TRUE;
                     }),
                     nullptr);

    // The window must stay resizable for libadwaita to present dialogs as bottom
    // sheets (a non-resizable window forces them to float), so instead snap back
    // from maximize/fullscreen to keep it at a sane popup size.
    g_signal_connect(window_, "notify::maximized",
                     G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
                         if (gtk_window_is_maximized(GTK_WINDOW(obj)) != FALSE) {
                             gtk_window_unmaximize(GTK_WINDOW(obj));
                         }
                     }),
                     nullptr);
    g_signal_connect(window_, "notify::fullscreened",
                     G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
                         if (gtk_window_is_fullscreen(GTK_WINDOW(obj)) != FALSE) {
                             gtk_window_unfullscreen(GTK_WINDOW(obj));
                         }
                     }),
                     nullptr);

    const std::string title{config::kAppName};
    gtk_window_set_title(GTK_WINDOW(window_), title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(window_), kWindowDefaultWidth, kWindowDefaultHeight);
    gtk_widget_set_size_request(GTK_WIDGET(window_), kWindowMinWidth, kWindowMinHeight);

    AdwToolbarView* toolbar = ADW_TOOLBAR_VIEW(adw_toolbar_view_new());
    AdwHeaderBar* header = ADW_HEADER_BAR(adw_header_bar_new());

    // Drop the maximize button (the window stays resizable for the bottom-sheet
    // dialogs, but isn't maximizable) while keeping the system's other controls.
    GValue layout_value = G_VALUE_INIT;
    g_value_init(&layout_value, G_TYPE_STRING);
    g_object_get_property(G_OBJECT(gtk_settings_get_default()), "gtk-decoration-layout",
                          &layout_value);
    const char* system_layout = g_value_get_string(&layout_value);
    const std::string decoration_layout =
        layout_without_maximize(system_layout != nullptr ? system_layout : ":minimize,close");
    adw_header_bar_set_decoration_layout(header, decoration_layout.c_str());
    g_value_unset(&layout_value);

    auto* clear_button = Gtk::make_managed<Gtk::Button>();
    clear_button->set_icon_name("user-trash-symbolic");
    clear_button->set_tooltip_text("Clear unpinned history");
    clear_button->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::clear_history));
    adw_header_bar_pack_end(header, GTK_WIDGET(clear_button->gobj()));

    auto* settings_button = Gtk::make_managed<Gtk::Button>();
    settings_button->set_icon_name("emblem-system-symbolic");
    settings_button->set_tooltip_text("Settings");
    settings_button->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::open_settings));
    adw_header_bar_pack_start(header, GTK_WIDGET(settings_button->gobj()));

    adw_toolbar_view_add_top_bar(toolbar, GTK_WIDGET(header));

    auto* content = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, kContentMargin);
    content->set_margin(kContentMargin);

    search_ = Gtk::make_managed<Gtk::SearchEntry>();
    search_->set_placeholder_text("Search clipboard history…");
    search_->signal_search_changed().connect([this] {
        search_text_ = search_->get_text().raw();
        apply_filter();
    });
    content->append(*search_);

    stack_ = Gtk::make_managed<Gtk::Stack>();
    stack_->set_vexpand(true);

    auto* scrolled = Gtk::make_managed<Gtk::ScrolledWindow>();
    scrolled->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    list_ = Gtk::make_managed<Gtk::ListBox>();
    list_->set_selection_mode(Gtk::SelectionMode::NONE);
    list_->add_css_class("background");
    list_->set_valign(Gtk::Align::START);
    scrolled->set_child(*list_);
    stack_->add(*scrolled, kPageList);

    auto* empty = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, kContentMargin);
    empty->set_valign(Gtk::Align::CENTER);
    empty->set_halign(Gtk::Align::CENTER);
    auto* icon = Gtk::make_managed<Gtk::Image>();
    icon->set_from_icon_name("edit-paste-symbolic");
    icon->set_pixel_size(kPlaceholderIconSize);
    icon->add_css_class("dim-label");
    empty->append(*icon);
    empty_title_ = Gtk::make_managed<Gtk::Label>();
    empty_title_->add_css_class("title-2");
    empty->append(*empty_title_);
    empty_description_ = Gtk::make_managed<Gtk::Label>();
    empty_description_->add_css_class("dim-label");
    empty->append(*empty_description_);
    stack_->add(*empty, kPageEmpty);

    content->append(*stack_);

    adw_toolbar_view_set_content(toolbar, GTK_WIDGET(content->gobj()));
    adw_application_window_set_content(window_, GTK_WIDGET(toolbar));
}

void MainWindow::schedule_refresh() {
    if (refresh_pending_) {
        return;
    }
    refresh_pending_ = true;
    Glib::signal_idle().connect_once([this] { rebuild_cards(); });
}

void MainWindow::rebuild_cards() {
    refresh_pending_ = false;
    while (Gtk::Widget* child = list_->get_first_child()) {
        list_->remove(*child);
    }

    const std::vector<core::ClipboardEntry> entries = history_.get().entries();
    card_count_ = entries.size();
    for (const core::ClipboardEntry& entry : entries) {
        list_->append(*Gtk::make_managed<ClipCard>(
            entry, kMaxPreviewChars, [this](const std::string& content) { copy(content); },
            [this](const std::string& content) { pin(content); }));
    }

    apply_filter();
}

void MainWindow::apply_filter() {
    // The search bar is only useful once there is something to search.
    search_->set_visible(card_count_ > 0);

    std::size_t visible = 0;
    for (Gtk::Widget* child = list_->get_first_child(); child != nullptr;
         child = child->get_next_sibling()) {
        auto* card = dynamic_cast<ClipCard*>(child);
        if (card == nullptr) {
            continue;
        }
        const bool shown = matches(card->content());
        card->set_visible(shown);
        if (shown) {
            ++visible;
        }
    }

    if (visible > 0) {
        stack_->set_visible_child(kPageList);
        return;
    }
    if (card_count_ == 0) {
        empty_title_->set_text("No clipboard history yet");
        empty_description_->set_text("Copy something to get started");
    } else {
        empty_title_->set_text("No results");
        empty_description_->set_text("Nothing matches your search");
    }
    stack_->set_visible_child(kPageEmpty);
}

void MainWindow::copy(const std::string& content) {
    clipboard_.get().write(content);
    history_.get().add(content);
    const core::Settings& settings = settings_.get().settings();
    // Auto-paste needs the window hidden so focus returns to the target window.
    if (settings.auto_hide_on_copy || settings.auto_paste) {
        gtk_widget_set_visible(GTK_WIDGET(window_), FALSE);
    }
    if (settings.auto_paste) {
        // Give focus time to return before sending the paste keystroke.
        Glib::signal_timeout().connect_once([this] { paster_.paste(); }, kPasteDelayMs);
    }
}

void MainWindow::pin(const std::string& content) {
    history_.get().toggle_pin(content);
}

void MainWindow::clear_history() {
    history_.get().clear_unpinned();
}

void MainWindow::open_settings() {
    settings_dialog_ =
        std::make_unique<SettingsDialog>(GTK_WIDGET(window_), settings_.get(),
                                         [this] { apply_theme(settings_.get().settings().theme); });
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

GtkWidget* MainWindow::native() const {
    return GTK_WIDGET(window_);
}

void MainWindow::toggle() {
    if (gtk_widget_get_visible(GTK_WIDGET(window_)) != FALSE) {
        gtk_widget_set_visible(GTK_WIDGET(window_), FALSE);
        return;
    }
    gtk_window_present(GTK_WINDOW(window_));
    // Start on the search field (type to filter); never leave a header button
    // showing the focus ring.
    if (gtk_widget_get_visible(GTK_WIDGET(search_->gobj())) != FALSE) {
        search_->grab_focus();
    } else {
        gtk_window_set_focus(GTK_WINDOW(window_), nullptr);
    }
}

} // namespace copyclip::ui
