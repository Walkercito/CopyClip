#include "ui/MainWindow.hpp"

#include "config/Constants.hpp"
#include "ui/ClipText.hpp"
#include "ui/Constants.hpp"
#include "ui/Fuzzy.hpp"
#include "ui/Theme.hpp"
#include "ui/widgets/ClipCard.hpp"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/image.h>
#include <gtkmm/scrolledwindow.h>

#include <gdk/gdkkeysyms.h>

#include <glibmm/main.h>
#include <glibmm/ustring.h>

#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef __GLIBC__
#include <malloc.h> // malloc_trim — return freed heap to the OS on hide (glibc only)
#endif

namespace copyclip::ui {

namespace {

constexpr int kPlaceholderIconSize = 48;
constexpr const char* kPageList = "list";
constexpr const char* kPageEmpty = "empty";

// Return freed heap pages to the OS; glibc otherwise keeps them in its arenas, so
// RSS never falls back after a peak (e.g. an image card's decode buffers). A free
// function so the glibc guard stays out of the G_CALLBACK macro at the call site.
void trim_heap() {
#ifdef __GLIBC__
    malloc_trim(0);
#endif
}

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

// Orders rows like HistoryService::sorted (pinned first, then newest first) so the
// ListBox places an incrementally-inserted card in its right spot without a full
// rebuild. Rows are always ClipCards; a defensive null check keeps it total.
[[nodiscard]] int clip_card_sort(Gtk::ListBoxRow* lhs, Gtk::ListBoxRow* rhs) {
    const auto* a = dynamic_cast<const ClipCard*>(lhs);
    const auto* b = dynamic_cast<const ClipCard*>(rhs);
    if (a == nullptr || b == nullptr) {
        return 0;
    }
    const core::ClipboardEntry& ea = a->entry();
    const core::ClipboardEntry& eb = b->entry();
    if (ea.pinned != eb.pinned) {
        return ea.pinned ? -1 : 1; // pinned rows first
    }
    if (ea.created_at != eb.created_at) {
        return ea.created_at > eb.created_at ? -1 : 1; // newer first
    }
    return 0;
}

} // namespace

MainWindow::MainWindow(GtkApplication* application, core::HistoryService& history,
                       core::SettingsService& settings, core::ClipboardSource& clipboard,
                       Paster& paster)
    : history_{history}, settings_{settings}, copy_action_{clipboard, history, settings, paster},
      application_{application} {
    build_ui(application);
    history_subscription_ = history_.get().subscribe([this] { schedule_refresh(); });
    apply_theme(settings.settings().theme);
    rebuild_cards();
    refresh_tray();
}

MainWindow::~MainWindow() {
    // Unsubscribe first so a late notification can't reach a half-torn-down window,
    // then destroy the window so its child widgets — and the signal slots bound to
    // `this` — die with it rather than later with the GtkApplication.
    history_subscription_ = {};
    if (window_ != nullptr) {
        gtk_window_destroy(GTK_WINDOW(window_));
    }
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

    // Trim on every hide — close, copy, and toggle all route through this signal.
    g_signal_connect(window_, "hide", G_CALLBACK(+[](GtkWidget*, gpointer) { trim_heap(); }),
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

    // libadwaita has no gtkmm binding, so its widgets (header bar, toolbar view,
    // window) are driven through the C API; gtkmm child widgets are handed across
    // with GTK_WIDGET(...->gobj()).
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

    // Escape hides the window from anywhere inside it. Capture phase so it is read
    // before a focused child (e.g. the search entry, which would otherwise just clear
    // its text) consumes it.
    auto key_controller = Gtk::EventControllerKey::create();
    key_controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    key_controller->signal_key_pressed().connect(sigc::mem_fun(*this, &MainWindow::on_key_pressed),
                                                 false);
    content->add_controller(key_controller);

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
    // Don't activate on a single click — that is ClipCard's own gesture (copy /
    // Ctrl-pin). Keyboard activation (Enter on the focused row) still fires
    // row-activated, which pastes the clip the arrow keys landed on.
    list_->set_activate_on_single_click(false);
    list_->signal_row_activated().connect(sigc::mem_fun(*this, &MainWindow::on_row_activated));
    list_->add_css_class("background");
    list_->set_valign(Gtk::Align::START);
    // Keep rows ordered so incrementally-added cards land in place (see rebuild_cards).
    list_->set_sort_func(
        [](Gtk::ListBoxRow* a, Gtk::ListBoxRow* b) { return clip_card_sort(a, b); });
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
    const std::vector<core::ClipboardEntry> entries = history_.get().entries();
    card_count_ = entries.size();

    // Index the entries we want shown, by their key, for O(1) lookup below.
    std::map<std::string, const core::ClipboardEntry*> wanted;
    for (const core::ClipboardEntry& entry : entries) {
        wanted.emplace(entry.content, &entry);
    }

    // Drop focus if it sits on a row we may remove, so GTK never accounts a
    // destroyed focused/active child (the "Broken accounting of active state"
    // warning). The search entry lives outside the list, so its focus is untouched.
    if (GtkWidget* focus = gtk_window_get_focus(GTK_WINDOW(window_));
        focus != nullptr && gtk_widget_is_ancestor(focus, GTK_WIDGET(list_->gobj())) != FALSE) {
        gtk_window_set_focus(GTK_WINDOW(window_), nullptr);
    }

    // Remove cards that are gone, or whose displayed data (pin or timestamp)
    // changed — those few are recreated below; every unchanged card is reused.
    for (auto it = cards_.begin(); it != cards_.end();) {
        const auto found = wanted.find(it->first);
        const core::ClipboardEntry& shown = it->second->entry();
        const bool stale = found == wanted.end() || found->second->pinned != shown.pinned ||
                           found->second->created_at != shown.created_at;
        if (stale) {
            list_->remove(*it->second);
            it = cards_.erase(it);
        } else {
            ++it;
        }
    }

    // Add a card only for entries that lack one. The sort function drops each new
    // row into its ordered position, so no existing card is rebuilt.
    for (const core::ClipboardEntry& entry : entries) {
        if (cards_.contains(entry.content)) {
            continue;
        }
        std::vector<std::byte> image; // fetched lazily, only for image cards
        if (entry.kind == core::ClipKind::Image) {
            image = history_.get().image(entry.content);
        }
        auto* card = Gtk::make_managed<ClipCard>(
            entry, std::move(image), kMaxPreviewChars,
            [this](const core::ClipboardEntry& clip) { copy(clip); },
            [this](const core::ClipboardEntry& clip) { pin(clip.content); });
        list_->append(*card);
        cards_.emplace(entry.content, card);
    }

    list_->invalidate_sort();
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

bool MainWindow::on_key_pressed(guint keyval, guint /*keycode*/, Gdk::ModifierType /*state*/) {
    if (keyval == GDK_KEY_Escape) {
        gtk_widget_set_visible(GTK_WIDGET(window_), FALSE);
        return true;
    }
    return false; // everything else falls through (typing, arrow navigation, Enter, …)
}

void MainWindow::on_row_activated(Gtk::ListBoxRow* row) {
    auto* card = dynamic_cast<ClipCard*>(row);
    if (card == nullptr) {
        return;
    }
    // Defer to an idle for the same reason ClipCard does on click: copy() rebuilds the
    // list, and tearing this row down from inside the activation would corrupt GTK's
    // state accounting. Capture the entry by value so it outlives the card.
    const core::ClipboardEntry entry = card->entry();
    Glib::signal_idle().connect_once([this, entry] { copy(entry); });
}

void MainWindow::copy(const core::ClipboardEntry& entry) {
    // Reconstruct the clipboard payload for the entry's kind. Image bytes are
    // fetched lazily by hash; rich text carries its HTML alongside the plain text.
    core::ClipContent content;
    content.kind = entry.kind;
    if (entry.kind == core::ClipKind::Image) {
        content.image = history_.get().image(entry.content);
        content.image_width = entry.image_width;
        content.image_height = entry.image_height;
    } else {
        content.text = entry.content;
        content.html = entry.html;
    }
    // CopyAction handles clipboard + history + auto-paste; the window just hides.
    if (copy_action_.run(content)) {
        gtk_widget_set_visible(GTK_WIDGET(window_), FALSE);
    }
}

void MainWindow::pin(const std::string& content) {
    history_.get().toggle_pin(content);
}

void MainWindow::clear_history() {
    history_.get().clear_unpinned();
}

void MainWindow::open_settings() {
    if (settings_dialog_) {
        return; // already open — a second dialog would leave dangling row callbacks
    }
    settings_dialog_ = std::make_unique<SettingsDialog>(
        GTK_WIDGET(window_), settings_.get(),
        [this] { apply_theme(settings_.get().settings().theme); }, [this] { refresh_tray(); },
        // On close, drop the wrapper (on idle, not mid-signal) so it can reopen.
        [this] { Glib::signal_idle().connect_once([this] { settings_dialog_.reset(); }); });
}

bool MainWindow::matches(const std::string& content) const {
    if (search_text_.empty()) {
        return true;
    }
    // Fuzzy subsequence match, case-folded through Glib for Unicode correctness.
    // Sanitize the content first: a legacy clip may hold invalid UTF-8, which
    // Glib::ustring's case-folding rejects.
    return fuzzy_matches(Glib::ustring{search_text_}.lowercase().raw(),
                         Glib::ustring{make_valid_utf8(content)}.lowercase().raw());
}

GtkWidget* MainWindow::native() const {
    return GTK_WIDGET(window_);
}

void MainWindow::toggle() {
    if (gtk_widget_get_visible(GTK_WIDGET(window_)) != FALSE) {
        gtk_widget_set_visible(GTK_WIDGET(window_), FALSE);
        return;
    }
    present();
}

void MainWindow::present() {
    gtk_window_present(GTK_WINDOW(window_));
    // Start on the search field (type to filter); never leave a header button
    // showing the focus ring.
    if (gtk_widget_get_visible(GTK_WIDGET(search_->gobj())) != FALSE) {
        search_->grab_focus();
    } else {
        gtk_window_set_focus(GTK_WINDOW(window_), nullptr);
    }
}

void MainWindow::refresh_tray() {
    const bool wanted = settings_.get().settings().show_panel_icon;
    if (wanted && !tray_) {
        tray_ = std::make_unique<StatusNotifierItem>(
            std::string{kPanelIconName}, [this] { present(); },
            [this] {
                present();
                open_settings();
            },
            [this] { g_application_quit(G_APPLICATION(application_)); });
    } else if (!wanted && tray_) {
        tray_.reset();
    }
}

} // namespace copyclip::ui
