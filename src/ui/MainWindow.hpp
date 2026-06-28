#pragma once

// The main clipboard window: an AdwApplicationWindow with a header (search +
// Clear), a boxed-list of clip cards from the HistoryService, and an empty /
// no-results placeholder. Encapsulates the libadwaita C widgets behind a small
// C++ object. Refreshes are deferred to an idle so a card may safely trigger one
// from inside its own click handler.

#include "core/HistoryService.hpp"
#include "core/Interfaces.hpp"
#include "core/SettingsService.hpp"

#include <adwaita.h>

#include <gtkmm/listbox.h>
#include <gtkmm/searchentry.h>

#include <functional>
#include <string>

namespace copyclip::ui {

class MainWindow {
public:
    MainWindow(AdwApplication* application, core::HistoryService& history,
               core::SettingsService& settings, core::ClipboardSource& clipboard);
    ~MainWindow() = default;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    void present();

private:
    void build_ui(AdwApplication* application);
    void schedule_refresh();
    void refresh();
    void copy(const std::string& content);
    void pin(const std::string& content);
    void clear_history();
    [[nodiscard]] bool matches(const std::string& content) const;

    std::reference_wrapper<core::HistoryService> history_;
    std::reference_wrapper<core::ClipboardSource> clipboard_;
    bool refresh_pending_ = false;
    std::string search_text_;
    AdwApplicationWindow* window_ = nullptr;
    Gtk::ListBox* list_ = nullptr;
    Gtk::SearchEntry* search_ = nullptr;
    AdwStatusPage* placeholder_ = nullptr;
};

} // namespace copyclip::ui
