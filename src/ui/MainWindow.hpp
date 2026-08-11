#pragma once

// The main clipboard window: an AdwApplicationWindow whose header holds the
// Settings and Clear buttons, with a search entry above a Gtk::Stack that shows
// either the list of clip cards or an empty / no-results page. Encapsulates the
// libadwaita C widgets behind a small C++ object. On each history change the list
// is reconciled incrementally — only added, removed, or changed (pin/timestamp)
// cards are touched, and a ListBox sort function repositions rows — so a single
// new clip touches one widget, not all N. Search toggles each card's visibility in
// place. Refreshes are deferred to an idle so a card may safely trigger one from
// inside its own click handler.
//
// The window is keyboard-first: focus stays in the search entry, Up/Down move the
// highlighted row through the filtered list, Enter pastes it, and Escape drops the
// search before dismissing. A window-level key controller owns that policy, so the
// keys work wherever focus happens to sit.

#include "core/HistoryService.hpp"
#include "core/Interfaces.hpp"
#include "core/SettingsService.hpp"
#include "ui/CopyAction.hpp"
#include "ui/Paster.hpp"
#include "ui/StatusNotifierItem.hpp"
#include "ui/dialogs/SettingsDialog.hpp"

#include <adwaita.h>

#include <gdkmm/enums.h>

#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/listboxrow.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/stack.h>

#include <glib.h>

#include <chrono>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace copyclip::ui {

class ClipCard;

class MainWindow {
public:
    MainWindow(GtkApplication* application, core::HistoryService& history,
               core::SettingsService& settings, core::ClipboardSource& clipboard, Paster& paster);
    ~MainWindow();

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    // Show and focus the window if hidden, otherwise hide it (global-shortcut
    // relaunch toggles visibility).
    void toggle();
    // Show and focus the window (used by the panel icon's "Open"; also the show
    // half of toggle()).
    void present();
    // The underlying window widget, for parenting dialogs.
    [[nodiscard]] GtkWidget* native() const;

private:
    // True while the window is on screen, as opposed to hidden in the background.
    [[nodiscard]] bool showing() const;
    void build_ui(GtkApplication* application);
    // Create or drop the panel icon to match the show_panel_icon setting.
    void refresh_tray();
    void schedule_refresh();
    void rebuild_cards();
    void apply_filter();
    // The window's keyboard policy — Escape, Up/Down, Enter and Ctrl+Enter.
    // Returns true when the key was consumed; everything else falls through to
    // the focused widget.
    bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
    // Move the highlighted row to the next/previous match, and scroll it into view.
    void move_selection(bool forward);
    // Paste the highlighted clip. False when there is nothing highlighted, so the
    // key keeps its stock behavior.
    bool activate_selection();
    // Pin / unpin the highlighted clip. False when nothing is selected.
    bool pin_selection();
    // Send the cursor home: highlight the first match and scroll the list back up to
    // it, or leave nothing highlighted when the filter hid every card.
    void reset_to_top();
    // Put the cursor on `content`'s card and scroll it into view — pinned clips sort
    // above the newest, so that is rarely the top. False when no such card is shown
    // (gone, or hidden by the filter), leaving the cursor untouched for the caller to
    // decide what that means.
    bool cursor_to(const std::string& content);
    // The selected ClipCard, or nullptr when the list has no keyboard cursor.
    [[nodiscard]] ClipCard* selected_card() const;
    // Drop the filter immediately (entry + cached query + list). GtkSearchEntry's
    // search-changed is delayed ~150 ms; Escape and hide must not wait for it.
    void clear_search();
    // Pull the live entry into search_text_ and refilter if it changed. Call before
    // paste/arrows so a fast type→Enter does not act on the pre-debounce list.
    void sync_search_from_entry();
    // Scroll the list so `row` is visible — moving the selection from the search
    // entry never moves focus, so GTK won't scroll for us.
    void reveal(Gtk::ListBoxRow& row);
    void copy(const core::ClipboardEntry& entry);
    // Hide the window; the app keeps capturing in the background.
    void hide();
    void pin(const std::string& content);
    void clear_history();
    void open_settings();
    [[nodiscard]] bool matches(const std::string& content) const;

    std::reference_wrapper<core::HistoryService> history_;
    std::reference_wrapper<core::SettingsService> settings_;
    CopyAction copy_action_;
    GtkApplication* application_ = nullptr; // borrowed, for the tray "Quit"
    bool refresh_pending_ = false;
    std::size_t card_count_ = 0;
    std::string search_text_;
    // The newest created_at seen so far. When a rebuild's max exceeds it, a clip was
    // just added (a copy or a paste) — the cue to put the cursor on it. Starts at
    // startup time so the history already on disk, all of it older, counts as history
    // rather than as one big arrival: only clips copied from now on move the cursor.
    std::chrono::system_clock::time_point last_seen_newest_{std::chrono::system_clock::now()};
    // The arrived clip the user has not been shown yet — the usual case, since a copy
    // happens in another app with this window down. Empty once a summon has honoured
    // it, or when nothing is waiting.
    std::string pending_arrival_;
    AdwApplicationWindow* window_ = nullptr;
    Gtk::Stack* stack_ = nullptr;
    Gtk::ScrolledWindow* scrolled_ = nullptr; // holds list_; its vadjustment scrolls it
    Gtk::ListBox* list_ = nullptr;
    // Live cards keyed by their clip content, for incremental reconciliation. The
    // cards are owned by `list_`; these are non-owning observers kept in sync with
    // it (an entry is erased the moment its card is removed from the list).
    std::map<std::string, ClipCard*> cards_;
    Gtk::SearchEntry* search_ = nullptr;
    Gtk::Label* empty_title_ = nullptr;
    Gtk::Label* empty_description_ = nullptr;
    std::unique_ptr<SettingsDialog> settings_dialog_;
    std::unique_ptr<StatusNotifierItem> tray_;
    core::HistoryService::Subscription history_subscription_;
};

} // namespace copyclip::ui
