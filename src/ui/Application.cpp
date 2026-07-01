#include "ui/Application.hpp"

#include "core/Enums.hpp"
#include "core/Platform.hpp"
#include "ui/Constants.hpp"
#include "ui/GnomeShortcut.hpp"

#include <adwaita.h>

#include <giomm/simpleaction.h>
#include <glibmm/main.h>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

namespace copyclip::ui {

namespace {

// Opt-in dev/test escape hatch (COPYCLIP_STANDALONE=1): run with no single-instance
// registration, so a freshly-built binary can run alongside an installed copy
// instead of becoming a remote instance of it (which would just exit immediately).
[[nodiscard]] Gio::Application::Flags app_flags() {
    const char* standalone = std::getenv("COPYCLIP_STANDALONE");
    return (standalone != nullptr && *standalone != '\0') ? Gio::Application::Flags::NON_UNIQUE
                                                          : Gio::Application::Flags::DEFAULT_FLAGS;
}

} // namespace

Application::Application(core::HistoryService& history, core::SettingsService& settings,
                         std::filesystem::path clipboard_state_file, bool start_hidden)
    : history_{history}, settings_{settings},
      clipboard_state_file_{std::move(clipboard_state_file)}, start_hidden_{start_hidden},
      paster_{core::detect_session()},
      application_{Gtk::Application::create(std::string{kApplicationId}, app_flags())} {
    application_->signal_startup().connect([] { adw_init(); });
    application_->signal_activate().connect(sigc::mem_fun(*this, &Application::on_activate));

    auto quit_action = Gio::SimpleAction::create("quit");
    quit_action->signal_activate().connect(
        [this](const Glib::VariantBase& /*parameter*/) { application_->quit(); });
    application_->add_action(quit_action);
    application_->set_accel_for_action("app.quit", "<Control>q");
}

int Application::run(int argc, char** argv) {
    return application_->run(argc, argv);
}

void Application::on_activate() {
    // Built once; later activations (a relaunch from the global shortcut) just
    // toggle the existing window.
    const bool first_activation = !window_;
    if (!window_) {
        // One clipboard backend for X11 and Wayland alike: GTK's X11 selection
        // clipboard. X11 selections need no focus, and on Wayland the app runs on
        // XWayland (forced in main()), which the compositor mirrors to and from the
        // Wayland clipboard. GNOME exposes no wlr/ext-data-control protocol, so this
        // XWayland route — the one the arboard-based reference uses on GNOME — is the
        // only focus-free path; it is event-driven and spawns no helper processes.
        clipboard_ = std::make_unique<GdkClipboardSource>(clipboard_state_file_);
        window_ = std::make_unique<MainWindow>(application_->gobj(), history_.get(),
                                               settings_.get(), *clipboard_, paster_);
        clipboard_->start(
            [this](const core::ClipContent& content) { history_.get().add(content); });
        application_->hold(); // keep capturing in the background after the window hides
        // On idle (so gsettings calls don't delay the window): onboard a new user,
        // otherwise refresh the binding only if the shortcut is still enabled.
        Glib::signal_idle().connect_once([this] {
            if (settings_.get().is_first_run()) {
                first_run_dialog_ = std::make_unique<FirstRunDialog>(
                    window_->native(), settings_.get().settings().hotkey,
                    [this](const std::string& accelerator) {
                        settings_.get().complete_first_run(accelerator);
                        register_gnome_shortcut(executable_path(), accelerator);
                    });
            } else if (is_gnome_shortcut_registered()) {
                register_gnome_shortcut(executable_path(), settings_.get().settings().hotkey);
            }
        });
    }
    // A background autostart builds the window and starts capturing but stays
    // hidden — except on first run, when the setup dialog needs a visible window.
    if (first_activation && start_hidden_ && !settings_.get().is_first_run()) {
        return;
    }
    window_->toggle();
}

} // namespace copyclip::ui
