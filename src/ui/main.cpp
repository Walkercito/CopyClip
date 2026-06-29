// CopyClip GTK4 + libadwaita application entry point.
//
// Composition root for the windowed frontend: builds the engine (SQLite history,
// system clock, JSON settings) and runs the UI. Links only the pure engine (plus
// gtkmm/libadwaita) — never Qt.

#include "config/Constants.hpp"
#include "core/HistoryService.hpp"
#include "core/SettingsService.hpp"
#include "core/SystemClock.hpp"
#include "runtime/Logging.hpp"
#include "storage/JsonSettingsRepository.hpp"
#include "storage/SqliteHistoryRepository.hpp"
#include "ui/Application.hpp"

#include <spdlog/spdlog.h>

#include <exception>

int main(int argc, char** argv) {
    using namespace copyclip;

    runtime::configure_logging();
    try {
        storage::JsonSettingsRepository settings_repo{config::settings_file()};
        core::SettingsService settings{settings_repo};

        storage::SqliteHistoryRepository history_repo{config::history_db()};
        core::SystemClock clock;
        core::HistoryService history{history_repo, clock, settings.settings().max_history_items};

        ui::Application app{history, settings, config::data_dir() / "last-clipboard"};
        return app.run(argc, argv);
    } catch (const std::exception& error) {
        spdlog::critical("fatal: {}", error.what());
        return 1;
    }
}
