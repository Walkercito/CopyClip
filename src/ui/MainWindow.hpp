#pragma once

// The main clipboard window: an AdwApplicationWindow with a header bar and an
// empty-state page (search and history list arrive in later tasks). Encapsulates
// the libadwaita C widgets behind a small C++ object so the rest of the UI never
// touches the raw Adw API.

#include "core/SettingsService.hpp"

#include <adwaita.h>

namespace copyclip::ui {

class MainWindow {
public:
    MainWindow(AdwApplication* application, core::SettingsService& settings);
    ~MainWindow() = default;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    void present();

private:
    AdwApplicationWindow* window_; // owned by the AdwApplication
};

} // namespace copyclip::ui
