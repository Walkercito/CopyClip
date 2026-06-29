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
#include "ui/dialogs/SettingsDialog.hpp"

#include <adwaita.h>

#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/stack.h>

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
    // The underlying window widget, for parenting dialogs.
    [[nodiscard]] GtkWidget* native() const;

private:
    void build_ui(GtkApplication* application);
    void schedule_refresh();
    void rebuild_cards();
    void apply_filter();
    void copy(const core::ClipboardEntry& entry);
    void pin(const std::string& content);
    void clear_history();
    void open_settings();
    [[nodiscard]] bool matches(const std::string& content) const;

    std::reference_wrapper<core::HistoryService> history_;
    std::reference_wrapper<core::SettingsService> settings_;
    CopyAction copy_action_;
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
    core::HistoryService::Subscription history_subscription_;
};

} // namespace copyclip::ui
