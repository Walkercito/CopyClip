#pragma once

// The UI's DI root: owns the AdwApplication and builds the MainWindow when the
// application activates. Engine services are injected here and handed to the
// window.

#include "core/SettingsService.hpp"
#include "ui/MainWindow.hpp"

#include <adwaita.h>

#include <functional>
#include <memory>

namespace copyclip::ui {

class Application {
public:
    explicit Application(core::SettingsService& settings);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    [[nodiscard]] int run(int argc, char** argv);

private:
    void on_activate();

    std::reference_wrapper<core::SettingsService> settings_;
    AdwApplication* application_;
    std::unique_ptr<MainWindow> window_;
};

} // namespace copyclip::ui
