#include "ui/Application.hpp"

#include "ui/Constants.hpp"

#include <adwaita.h>

#include <string>

namespace copyclip::ui {

Application::Application(core::HistoryService& history, core::SettingsService& settings)
    : history_{history}, settings_{settings},
      application_{Gtk::Application::create(std::string{kApplicationId})} {
    application_->signal_startup().connect([] { adw_init(); });
    application_->signal_activate().connect(sigc::mem_fun(*this, &Application::on_activate));
}

int Application::run(int argc, char** argv) {
    return application_->run(argc, argv);
}

void Application::on_activate() {
    clipboard_ = std::make_unique<GdkClipboardSource>();
    window_ = std::make_unique<MainWindow>(application_->gobj(), history_.get(), settings_.get(),
                                           *clipboard_);
    clipboard_->start([this](const std::string& text) { history_.get().add(text); });
    window_->present();
}

} // namespace copyclip::ui
