#include "ui/MainWindow.hpp"

#include "config/Constants.hpp"
#include "ui/ClipText.hpp"
#include "ui/Constants.hpp"
#include "ui/Fuzzy.hpp"
#include "ui/KeyAction.hpp"
#include "ui/Theme.hpp"
#include "ui/widgets/ClipCard.hpp"

#include <gdkmm/frameclock.h>

#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/image.h>
#include <gtkmm/scrolledwindow.h>

#include <glibmm/main.h>
#include <glibmm/ustring.h>

#include <algorithm>
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
// The list is what the viewport scrolls, so its adjustment reads 0 at the very top.
constexpr double kScrollTop = 0.0;

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

// True when a button holds the focus (see KeyAction for why that matters).
[[nodiscard]] bool button_focused(GtkWindow* window) {
    GtkWidget* focus = gtk_window_get_focus(window);
    return focus != nullptr && GTK_IS_BUTTON(focus) != FALSE;
}

// True while focus sits inside a popover — the search entry's right-click Cut/Copy/
// Paste menu, or the emoji chooser. Those are not dialogs, so the dialog guard misses
// them, and they own their own Escape and arrow keys.
[[nodiscard]] bool popover_focused(GtkWindow* window) {
    GtkWidget* focus = gtk_window_get_focus(window);
    return focus != nullptr && gtk_widget_get_ancestor(focus, GTK_TYPE_POPOVER) != nullptr;
}

// True once the layout has given `widget` a place in its parent; until then it has
// no size, and asking where it sits answers nothing.
[[nodiscard]] bool is_laid_out(const Gtk::Widget& widget) {
    return widget.get_height() > 0;
}

// The first row at or after `start`, scanning in display order or against it, that
// the search filter left visible — or nullptr once the scan runs off the list.
[[nodiscard]] Gtk::ListBoxRow* visible_row_from(Gtk::Widget* start, bool forward) {
    for (Gtk::Widget* candidate = start; candidate != nullptr;
         candidate = forward ? candidate->get_next_sibling() : candidate->get_prev_sibling()) {
        auto* row = dynamic_cast<Gtk::ListBoxRow*>(candidate);
        if (row != nullptr && row->get_visible()) {
            return row;
        }
    }
    return nullptr;
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
                     G_CALLBACK(+[](GtkWindow*, gpointer self) -> gboolean {
                         static_cast<MainWindow*>(self)->hide();
                         return TRUE;
                     }),
                     this);

    // Escape, Up/Down and Enter are the window's own, wherever focus sits — capture
    // phase so they are read before the focused child's stock bindings (the search
    // entry would otherwise swallow Enter, which is the whole bug).
    auto key_controller = Gtk::EventControllerKey::create();
    key_controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    key_controller->signal_key_pressed().connect(sigc::mem_fun(*this, &MainWindow::on_key_pressed),
                                                 false);
    // add_controller takes ownership of a ref, so gobj_copy() mints it — unlike the
    // plain gobj() handoff used for child widgets below, which would double-unref here.
    gtk_widget_add_controller(GTK_WIDGET(window_),
                              GTK_EVENT_CONTROLLER(key_controller->gobj_copy()));

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

    search_ = Gtk::make_managed<Gtk::SearchEntry>();
    search_->set_placeholder_text("Search clipboard history…");
    search_->signal_search_changed().connect([this] {
        search_text_ = search_->get_text().raw();
        apply_filter();
    });
    content->append(*search_);

    stack_ = Gtk::make_managed<Gtk::Stack>();
    stack_->set_vexpand(true);

    scrolled_ = Gtk::make_managed<Gtk::ScrolledWindow>();
    scrolled_->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    list_ = Gtk::make_managed<Gtk::ListBox>();
    // Single selection is the keyboard cursor: apply_filter keeps exactly one match
    // highlighted, the arrow keys move it, and Enter pastes it. Row activation is
    // left alone — the mouse is ClipCard's own gesture (copy / Ctrl-pin).
    list_->set_selection_mode(Gtk::SelectionMode::SINGLE);
    list_->add_css_class("background");
    list_->set_valign(Gtk::Align::START);
    // Keep rows ordered so incrementally-added cards land in place (see rebuild_cards).
    list_->set_sort_func(
        [](Gtk::ListBoxRow* a, Gtk::ListBoxRow* b) { return clip_card_sort(a, b); });
    scrolled_->set_child(*list_);
    stack_->add(*scrolled_, kPageList);

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

    // A fresh add() — a new copy, or pasting a clip back — stamps the newest entry
    // with clock.now(), lifting the max created_at; pin and remove never do. That
    // lift is what marks a clip as newly arrived. Its key, or "" when nothing
    // arrived: no entry can key on empty, since add() rejects blanks.
    const auto newest = std::ranges::max_element(entries, {}, &core::ClipboardEntry::created_at);
    std::string arrived_content;
    if (newest != entries.end() && newest->created_at > last_seen_newest_) {
        last_seen_newest_ = newest->created_at;
        arrived_content = newest->content;
    }

    // Pinning recreates the very card the cursor sits on, taking the selection with
    // it. Remember what it held so the cursor can be put back below, instead of
    // apply_filter snapping it to the top of the list.
    const auto* selected = dynamic_cast<ClipCard*>(list_->get_selected_row());
    const std::string selected_content = selected != nullptr ? selected->entry().content : "";

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

    // Put the cursor back on the card it was on, recreated or not. Gone for good —
    // cleared, evicted, filtered out — leaves apply_filter to pick the fallback.
    if (const auto card = cards_.find(selected_content); card != cards_.end()) {
        list_->select_row(*card->second);
    }

    list_->invalidate_sort();
    apply_filter();

    // After apply_filter has settled the filtered selection, so the arrival wins over
    // the selection restored above: it takes the cursor now if the window is up to
    // see it, and waits for the next summon otherwise.
    if (!arrived_content.empty() && (!showing() || !cursor_to(arrived_content))) {
        pending_arrival_ = arrived_content;
    }
}

bool MainWindow::cursor_to(const std::string& content) {
    const auto card = cards_.find(content);
    if (card == cards_.end() || !card->second->get_visible()) {
        return false;
    }
    list_->select_row(*card->second);
    // Scrolling to it has to wait for the frame that lays the row out: a card
    // appended moments ago — and every card in a window only just presented — is not
    // placed yet, so reveal() here would find no position and do nothing. A tick
    // callback retries once per frame, so the wait costs frames rather than a
    // spinning idle.
    list_->add_tick_callback([this, content](const Glib::RefPtr<Gdk::FrameClock>&) {
        // Re-find it: a rebuild in between may have recreated or dropped the card.
        const auto row = cards_.find(content);
        if (row == cards_.end() || !row->second->get_visible()) {
            return false; // gone, or the filter hid it: nothing left to scroll to
        }
        if (!is_laid_out(*row->second)) {
            return true; // ask again next frame
        }
        reveal(*row->second);
        return false;
    });
    return true;
}

void MainWindow::apply_filter() {
    // The search bar is only useful once there is something to search.
    search_->set_visible(card_count_ > 0);

    const Gtk::ListBoxRow* selected = list_->get_selected_row();
    bool selection_shown = false;
    std::size_t visible = 0;
    for (Gtk::Widget* child = list_->get_first_child(); child != nullptr;
         child = child->get_next_sibling()) {
        auto* card = dynamic_cast<ClipCard*>(child);
        if (card == nullptr) {
            continue;
        }
        const bool shown = matches(card->content());
        card->set_visible(shown);
        if (!shown) {
            continue;
        }
        ++visible;
        selection_shown = selection_shown || card == selected;
    }

    // Always leave a row highlighted for Enter to act on. When the filter — or a
    // rebuild that dropped the card — took the selection away, the cursor goes home.
    if (!selection_shown) {
        reset_to_top();
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

bool MainWindow::on_key_pressed(guint keyval, guint /*keycode*/, Gdk::ModifierType state) {
    // Settings and first-run are AdwDialogs presented inside this very window, and
    // menus are popovers in it — while either is up, its own keys must win.
    if (adw_application_window_get_visible_dialog(window_) != nullptr ||
        popover_focused(GTK_WINDOW(window_))) {
        return false;
    }
    // Read the entry, not search_text_: GtkSearchEntry delays search-changed by
    // ~150 ms, so the cached copy lags behind text the user just typed — Escape
    // right after the first keystroke must still read as an active search.
    // Caveat: an in-progress input-method preedit is not detected, so Enter and
    // the arrows preempt CJK candidate selection. Filter through Gtk::IMContext if
    // that ever matters.
    const KeyContext context{.search_active = !search_->get_text().empty(),
                             .button_focused = button_focused(GTK_WINDOW(window_))};
    // Same GNOME accelerator strings Settings stores; mask matches BoundAccelerator.
    const auto mod_mask = static_cast<Gdk::ModifierType>(gtk_accelerator_get_default_mod_mask());
    const auto modifiers = static_cast<unsigned int>(state & mod_mask);
    const core::Settings& cfg = settings_.get().settings();
    const WindowShortcuts shortcuts{.paste = bound_accelerator(cfg.paste_hotkey),
                                    .pin = bound_accelerator(cfg.pin_hotkey)};
    switch (key_action(keyval, modifiers, context, shortcuts)) {
    case KeyAction::ClearSearch:
        clear_search();
        return true;
    case KeyAction::Dismiss:
        hide();
        return true;
    case KeyAction::SelectPrevious:
        // Flush the debounced filter first so navigation matches what the user typed.
        sync_search_from_entry();
        move_selection(false);
        return true;
    case KeyAction::SelectNext:
        sync_search_from_entry();
        move_selection(true);
        return true;
    case KeyAction::Paste:
        sync_search_from_entry();
        return activate_selection();
    case KeyAction::TogglePin:
        // Pin targets the selected row; keep the filter in sync so the cursor
        // still points at a visible match after a just-typed query.
        sync_search_from_entry();
        return pin_selection();
    case KeyAction::RemoveSelected:
        // Only reached with an empty search (see key_action), so no filter to sync.
        return remove_selection();
    case KeyAction::None:
        return false;
    }
    // No default case, so a new KeyAction trips -Wswitch rather than being ignored.
    return false;
}

void MainWindow::move_selection(bool forward) {
    // Ctrl+click reaches GtkListBox as a toggle and leaves the list unselected, so
    // the cursor can be missing even with matches on screen. Start the scan at the
    // top then, rather than letting the arrows go dead.
    Gtk::Widget* start = list_->get_first_child();
    if (Gtk::ListBoxRow* const current = list_->get_selected_row(); current != nullptr) {
        start = forward ? current->get_next_sibling() : current->get_prev_sibling();
    }
    // Stop at the ends rather than wrapping: with the list also acting as the
    // paste target, wrapping past the last row invites pasting the wrong clip.
    if (Gtk::ListBoxRow* const next = visible_row_from(start, forward); next != nullptr) {
        list_->select_row(*next);
        reveal(*next);
    }
}

ClipCard* MainWindow::selected_card() const {
    return dynamic_cast<ClipCard*>(list_->get_selected_row());
}

void MainWindow::clear_search() {
    // Entry first so a late search-changed still sees empty; then force the
    // filter state without waiting for GtkSearchEntry's ~150 ms debounce.
    search_->set_text("");
    search_text_.clear();
    apply_filter();
}

void MainWindow::sync_search_from_entry() {
    const std::string live = search_->get_text().raw();
    if (live == search_text_) {
        return;
    }
    search_text_ = live;
    apply_filter();
}

bool MainWindow::activate_selection() {
    ClipCard* const card = selected_card();
    if (card == nullptr) {
        return false;
    }
    // Straight through, unlike ClipCard's click: the rebuild copy() sets off is
    // itself deferred (see schedule_refresh), so no widget dies under this dispatch.
    copy(card->entry());
    return true;
}

bool MainWindow::pin_selection() {
    ClipCard* const card = selected_card();
    if (card == nullptr) {
        return false;
    }
    // Same path as Ctrl+click on a card; rebuild_cards keeps the keyboard cursor.
    pin(card->entry().content);
    return true;
}

void MainWindow::reset_to_top() {
    if (Gtk::ListBoxRow* const first = visible_row_from(list_->get_first_child(), true);
        first != nullptr) {
        list_->select_row(*first);
    } else {
        list_->unselect_all(); // the filter hid every card: nothing to put it on
    }
    scrolled_->get_vadjustment()->set_value(kScrollTop);
}

bool MainWindow::remove_selection() {
    ClipCard* const card = selected_card();
    if (card == nullptr) {
        return false;
    }
    const std::string content = card->entry().content;
    // Hand the cursor to the neighbour that will outlive the removal — next match,
    // else previous — so rebuild_cards restores the selection there instead of
    // letting apply_filter snap it to the top. Delete can then be held down the list.
    Gtk::ListBoxRow* neighbour = visible_row_from(card->get_next_sibling(), true);
    if (neighbour == nullptr) {
        neighbour = visible_row_from(card->get_prev_sibling(), false);
    }
    if (neighbour != nullptr) {
        list_->select_row(*neighbour);
    }
    // remove() drops the entry whatever its pin state — Delete is an explicit user
    // act, unlike the Clear button which spares pinned clips (clear_unpinned).
    history_.get().remove(content);
    return true;
}

void MainWindow::reveal(Gtk::ListBoxRow& row) {
    double row_x = 0.0;
    double row_y = 0.0;
    if (row.translate_coordinates(*list_, 0.0, 0.0, row_x, row_y)) {
        // The list is what the viewport scrolls, so list coordinates are the
        // adjustment's own.
        scrolled_->get_vadjustment()->clamp_page(row_y, row_y + row.get_height());
    }
}

void MainWindow::copy(const core::ClipboardEntry& entry) {
    // ClipCard defers its click to an idle, so a fast double click can queue two
    // copies of the same clip — which with auto-paste on injects Ctrl+V twice into
    // the target app. The first one hid the window, so that is the signal to stop.
    if (!showing()) {
        return;
    }
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
        hide();
    }
}

void MainWindow::hide() {
    // Drop the filter on the way out, whichever path got here — pasting, clicking,
    // closing or toggling. The window is a popup: reopening it on someone's old
    // query (which grab_focus leaves unselected, so typing appends to it) is never
    // what was meant. clear_search() also syncs search_text_ immediately so a
    // fast hide→present cannot reopen with a stale filter.
    clear_search();
    gtk_widget_set_visible(GTK_WIDGET(window_), FALSE);
}

void MainWindow::pin(const std::string& content) {
    // Ctrl+click reaches ListBox as a selection toggle under SINGLE mode and can
    // leave nothing selected before rebuild_cards snapshots the cursor. Re-assert
    // the pin target as the selection so the keyboard cursor survives the rebuild.
    if (const auto it = cards_.find(content); it != cards_.end()) {
        list_->select_row(*it->second);
    }
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

bool MainWindow::showing() const {
    return gtk_widget_get_visible(GTK_WIDGET(window_)) != FALSE;
}

void MainWindow::toggle() {
    if (showing()) {
        hide();
        return;
    }
    present();
}

void MainWindow::present() {
    gtk_window_present(GTK_WINDOW(window_));
    // A summon lands on the clip copied since the list was last looked at, and at the
    // top of the list when nothing new came in — never on wherever the cursor and
    // scroll last sat. Consumed either way: an arrival is only special until shown.
    if (!cursor_to(std::exchange(pending_arrival_, {}))) {
        reset_to_top();
    }
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
