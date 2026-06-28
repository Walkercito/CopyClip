#include "ui/Application.hpp"

#include "ui/Constants.hpp"
#include "ui/GnomeShortcut.hpp"

#include <adwaita.h>

#include <giomm/simpleaction.h>
#include <glibmm/main.h>

#include <string>

namespace copyclip::ui {

Application::Application(core::HistoryService& history, core::SettingsService& settings)
    : history_{history}, settings_{settings},
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
        clipboard_ = std::make_unique<GdkClipboardSource>();
        window_ = std::make_unique<MainWindow>(application_->gobj(), history_.get(),
                                               settings_.get(), *clipboard_);
        clipboard_->start([this](const std::string& text) { history_.get().add(text); });
        application_->hold(); // keep capturing in the background after the window hides
        // Register on idle so the synchronous gsettings calls don't delay the
        // window appearing.
        Glib::signal_idle().connect_once([this] {
            register_gnome_shortcut(executable_path(), settings_.get().settings().hotkey);
        });
    }
    window_->toggle();
}

} // namespace copyclip::ui
