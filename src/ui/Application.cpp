#include "ui/Application.hpp"

#include "core/Enums.hpp"
#include "ui/Constants.hpp"
#include "ui/GnomeShortcut.hpp"

#include <adwaita.h>

#include <giomm/simpleaction.h>
#include <glibmm/main.h>

#include <filesystem>
#include <string>
#include <utility>

namespace copyclip::ui {

Application::Application(core::HistoryService& history, core::SettingsService& settings,
                         std::filesystem::path clipboard_state_file)
    : history_{history}, settings_{settings},
      clipboard_state_file_{std::move(clipboard_state_file)},
      application_{Gtk::Application::create(std::string{kApplicationId})} {
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
    if (!window_) {
        clipboard_ = std::make_unique<GdkClipboardSource>(clipboard_state_file_);
        window_ = std::make_unique<MainWindow>(application_->gobj(), history_.get(),
                                               settings_.get(), *clipboard_);
        clipboard_->start([this](const std::string& text) { history_.get().add(text); });
        application_->hold(); // keep capturing in the background after the window hides
        // On idle (so gsettings calls don't delay the window): onboard a new user,
        // otherwise refresh the binding only if the shortcut is still enabled.
        Glib::signal_idle().connect_once([this] {
            if (settings_.get().is_first_run()) {
                first_run_dialog_ = std::make_unique<FirstRunDialog>(
                    window_->native(), settings_.get().settings().hotkey,
                    [this](core::HotkeyPreset hotkey) {
                        settings_.get().complete_first_run(hotkey);
                        register_gnome_shortcut(executable_path(), hotkey);
                    });
            } else if (is_gnome_shortcut_registered()) {
                register_gnome_shortcut(executable_path(), settings_.get().settings().hotkey);
            }
        });
    }
    window_->toggle();
}

} // namespace copyclip::ui
