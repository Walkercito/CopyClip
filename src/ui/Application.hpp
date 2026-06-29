#pragma once

// The UI's DI root: owns a Gtk::Application (which initializes gtkmm) and, on
// activation, the clipboard source and the MainWindow. libadwaita is initialized
// at startup. Engine services are injected here and handed to the window;
// captured clipboard text is recorded into the history.

#include "core/HistoryService.hpp"
#include "core/SettingsService.hpp"
#include "ui/GdkClipboardSource.hpp"
#include "ui/KeystrokePaster.hpp"
#include "ui/MainWindow.hpp"
#include "ui/dialogs/FirstRunDialog.hpp"

#include <gtkmm/application.h>

#include <glibmm/refptr.h>

#include <filesystem>
#include <functional>
#include <memory>

namespace copyclip::ui {

class Application {
public:
    // `start_hidden` (set by --background, used for autostart) builds the window and
    // begins capturing but leaves the window hidden on the initial activation.
    Application(core::HistoryService& history, core::SettingsService& settings,
                std::filesystem::path clipboard_state_file, bool start_hidden = false);
    ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    [[nodiscard]] int run(int argc, char** argv);

private:
    void on_activate();

    std::reference_wrapper<core::HistoryService> history_;
    std::reference_wrapper<core::SettingsService> settings_;
    std::filesystem::path clipboard_state_file_;
    bool start_hidden_;
    KeystrokePaster paster_;
    Glib::RefPtr<Gtk::Application> application_;
    // GTK's X11 selection clipboard, used for both X11 and Wayland (via XWayland).
    std::unique_ptr<core::ClipboardSource> clipboard_;
    std::unique_ptr<MainWindow> window_;
    std::unique_ptr<FirstRunDialog> first_run_dialog_;
};

} // namespace copyclip::ui
