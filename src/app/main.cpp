// CopyClip engine entry point — headless composition root (mirrors the
// reference main.py: the engine runs with no window; the UI is a separate
// effort). The single-instance guard is acquired BEFORE Qt is built, so a second
// launch signals the running instance and exits without a QGuiApplication.

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
        // ponytail: this headless engine is dev-only (UI pending) and grabs via a
        // closed-enum HotkeySpec, which can't express a free-form accelerator, so
        // it uses the default preset. The shipped GTK app honours the user's custom
        // shortcut through GNOME's settings-daemon instead.
        const std::unique_ptr<core::HotkeyListener> listener = adapters::select_hotkey_listener(
            core::detect_session(), core::get_spec(core::kDefaultPreset), "copyclip-show-ui");

        // Declared last so it is destroyed first: the clipboard, listener,
        // history, and settings it references all outlive it.
        core::ClipboardEngine engine{*clipboard, *listener, history, settings};
        engine.on_show_requested([] { spdlog::info("show requested (UI pending)"); });
        const auto history_subscription = history.subscribe(
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
