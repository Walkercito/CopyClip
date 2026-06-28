#pragma once

// The main clipboard window: an AdwApplicationWindow with a header (search +
// Clear) and a Gtk::Stack that shows either the boxed-list of clip cards or an
// empty / no-results page. Encapsulates the libadwaita C widgets behind a small
// C++ object. Cards are rebuilt only when the history changes; search toggles
// each card's visibility in place. Rebuilds are deferred to an idle so a card may
// safely trigger one from inside its own click handler.

#include "core/HistoryService.hpp"
#include "core/Interfaces.hpp"
#include "core/SettingsService.hpp"

#include <adwaita.h>

#include <gtkmm/label.h>
#include <gtkmm/listbox.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/stack.h>

#include <cstddef>
#include <functional>
#include <string>

namespace copyclip::ui {

class MainWindow {
public:
    MainWindow(GtkApplication* application, core::HistoryService& history,
               core::SettingsService& settings, core::ClipboardSource& clipboard);
    ~MainWindow() = default;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    void present();

private:
    void build_ui(GtkApplication* application);
    void schedule_refresh();
    void rebuild_cards();
    void apply_filter();
    void copy(const std::string& content);
    void pin(const std::string& content);
    void clear_history();
    [[nodiscard]] bool matches(const std::string& content) const;

    std::reference_wrapper<core::HistoryService> history_;
    std::reference_wrapper<core::ClipboardSource> clipboard_;
    bool refresh_pending_ = false;
    std::size_t card_count_ = 0;
    std::string search_text_;
    AdwApplicationWindow* window_ = nullptr;
    Gtk::Stack* stack_ = nullptr;
    Gtk::ListBox* list_ = nullptr;
    Gtk::SearchEntry* search_ = nullptr;
    Gtk::Label* empty_title_ = nullptr;
    Gtk::Label* empty_description_ = nullptr;
};

} // namespace copyclip::ui
