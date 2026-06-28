#include "ui/Application.hpp"

#include "ui/Constants.hpp"

#include <string>

namespace copyclip::ui {

Application::Application(core::SettingsService& settings)
    : settings_{settings}, application_{adw_application_new(std::string{kApplicationId}.c_str(),
                                                            G_APPLICATION_DEFAULT_FLAGS)} {
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
    window_ = std::make_unique<MainWindow>(application_, settings_.get());
    window_->present();
}

} // namespace copyclip::ui
