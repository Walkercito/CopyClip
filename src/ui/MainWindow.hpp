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
#include <gtkmm/searchentry.h>
#include <gtkmm/stack.h>

#include <glib.h>

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
    void build_ui(GtkApplication* application);
    // Create or drop the panel icon to match the show_panel_icon setting.
    void refresh_tray();
    void schedule_refresh();
    void rebuild_cards();
    void apply_filter();
    // Paste the row the keyboard navigation landed on — Enter (or double-click)
    // activates it, taking the same path as a plain click on the card.
    void on_row_activated(Gtk::ListBoxRow* row);
    // Escape hides the window; every other key falls through. Returns true when handled.
    bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
    void copy(const core::ClipboardEntry& entry);
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
    AdwApplicationWindow* window_ = nullptr;
    Gtk::Stack* stack_ = nullptr;
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
