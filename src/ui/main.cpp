// CopyClip GTK4 + libadwaita application entry point.
//
// Composition root for the windowed frontend: builds the settings service from
// the JSON repository and runs the UI. Links only the pure engine (plus
// gtkmm/libadwaita) — never Qt.

#include "config/Constants.hpp"
#include "core/SettingsService.hpp"
#include "runtime/Logging.hpp"
#include "storage/JsonSettingsRepository.hpp"
#include "ui/Application.hpp"

#include <spdlog/spdlog.h>

#include <exception>

int main(int argc, char** argv) {
    using namespace copyclip;

    runtime::configure_logging();
    try {
        storage::JsonSettingsRepository settings_repo{config::settings_file()};
        core::SettingsService settings{settings_repo};
        ui::Application app{settings};
        return app.run(argc, argv);
    } catch (const std::exception& error) {
        spdlog::critical("fatal: {}", error.what());
        return 1;
    }
}
