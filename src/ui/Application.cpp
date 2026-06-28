#include "ui/Application.hpp"

#include "ui/Constants.hpp"

#include <string>

namespace copyclip::ui {

Application::Application(core::HistoryService& history, core::SettingsService& settings)
    : history_{history}, settings_{settings},
      application_{
          adw_application_new(std::string{kApplicationId}.c_str(), G_APPLICATION_DEFAULT_FLAGS)} {
    g_signal_connect(application_, "activate",
                     G_CALLBACK(+[](AdwApplication* /*app*/, gpointer data) {
                         static_cast<Application*>(data)->on_activate();
                     }),
                     this);
}

Application::~Application() {
    g_object_unref(application_);
}

int Application::run(int argc, char** argv) {
    return g_application_run(G_APPLICATION(application_), argc, argv);
}

void Application::on_activate() {
    clipboard_ = std::make_unique<GdkClipboardSource>();
    window_ =
        std::make_unique<MainWindow>(application_, history_.get(), settings_.get(), *clipboard_);
    clipboard_->start([this](const std::string& text) { history_.get().add(text); });
    window_->present();
}

} // namespace copyclip::ui
