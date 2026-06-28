// CopyClip engine entry point — headless composition root.
//
// Mirrors the reference main.py: this runs the engine with no window. It wires
// the QClipboard source, the session-appropriate hotkey listener, SQLite
// history, and JSON settings, then logs clipboard captures and hotkey
// activations. The single-instance guard is acquired BEFORE Qt is built, so a
// second launch signals the running instance and exits without constructing a
// QGuiApplication. The UI is a separate effort.

#include "adapters/Factory.hpp"
#include "config/Constants.hpp"
#include "core/ClipboardEngine.hpp"
#include "core/HistoryService.hpp"
#include "core/Hotkeys.hpp"
#include "core/Platform.hpp"
#include "core/SettingsService.hpp"
#include "core/SystemClock.hpp"
#include "runtime/Logging.hpp"
#include "runtime/SingleInstanceGuard.hpp"
#include "storage/JsonSettingsRepository.hpp"
#include "storage/SqliteHistoryRepository.hpp"

#include <spdlog/spdlog.h>

#include <QGuiApplication>

#include <exception>
#include <memory>

int main(int argc, char** argv) {
    using namespace copyclip;

    runtime::configure_logging();

    runtime::SingleInstanceGuard guard{config::instance_socket()};
    if (!guard.acquire([] { spdlog::info("show requested via IPC (UI pending)"); })) {
        spdlog::info("CopyClip is already running; signalling it to show");
        if (!guard.signal_show()) {
            spdlog::warn("failed to signal the running instance");
        }
        return 0;
    }

    try {
        // A QGuiApplication (no window) is required for QClipboard.
        [[maybe_unused]] QGuiApplication app{argc, argv};

        storage::JsonSettingsRepository settings_repo{config::settings_file()};
        core::SettingsService settings{settings_repo};

        storage::SqliteHistoryRepository history_repo{config::history_db()};
        core::SystemClock clock;
        core::HistoryService history{history_repo, clock, settings.settings().max_history_items};

        const std::unique_ptr<core::ClipboardSource> clipboard = adapters::build_clipboard_source();
        const std::unique_ptr<core::HotkeyListener> listener = adapters::select_hotkey_listener(
            core::detect_session(), core::get_spec(settings.settings().hotkey), "copyclip-show-ui");

        core::ClipboardEngine engine{*clipboard, *listener, history, settings};
        engine.on_show_requested([] { spdlog::info("show requested (UI pending)"); });
        history.subscribe(
            [&history] { spdlog::info("history now holds {} entries", history.entries().size()); });

        engine.start();
        spdlog::info("CopyClip engine running (headless). Ctrl+C to quit.");

        const int code = QGuiApplication::exec();

        engine.stop();
        guard.release();
        return code;
    } catch (const std::exception& error) {
        spdlog::critical("fatal: {}", error.what());
        guard.release();
        return 1;
    }
}
