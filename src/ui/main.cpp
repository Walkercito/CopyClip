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
#include "ui/Constants.hpp"

#include <glib.h>
#include <spdlog/spdlog.h>

#include <cstddef>
#include <exception>
#include <span>
#include <string_view>
#include <vector>

#ifdef __GLIBC__
#include <malloc.h> // mallopt / M_ARENA_MAX — cap per-thread arenas (glibc only)
#endif

int main(int argc, char** argv) {
    using namespace copyclip;

    // Cap glibc's malloc arenas before anything allocates much: GLib/GTK spawn
    // several worker threads and unbounded per-thread arenas keep freed memory
    // off the OS, so a background app's RSS only ever climbs. mallopt is reliable
    // here (the env var is read by glibc too early, before main runs).
#ifdef __GLIBC__
    mallopt(M_ARENA_MAX, ui::kMallocArenaMax);
#endif

    // --background (used by the autostart entry) starts hidden but keeps capturing.
    // Consume it here so GTK's option parser never sees this non-GTK flag; forward
    // the rest of argv unchanged (and NUL-terminated, as argv conventionally is).
    const std::span<char*> args{argv, static_cast<std::size_t>(argc)};
    bool start_hidden = false;
    std::vector<char*> forwarded;
    forwarded.reserve(args.size() + 1);
    for (char* const arg : args) {
        if (std::string_view{arg} == ui::kBackgroundFlag) {
            start_hidden = true;
            continue;
        }
        forwarded.push_back(arg);
    }
    const int forwarded_argc = static_cast<int>(forwarded.size());
    forwarded.push_back(nullptr);
    char** forwarded_argv = forwarded.data();

    // Prefer the X11 GDK backend (native X11, or XWayland when an X display exists).
    // GNOME's Wayland compositor exposes no wlr/ext-data-control protocol, so a
    // background manager cannot read the Wayland clipboard without focus. X11
    // selections need no focus, and on Wayland the compositor mirrors them through
    // XWayland — the same path the arboard-based reference uses on GNOME. An
    // explicit GDK_BACKEND is respected (the final argument is "don't overwrite").
    if (g_getenv(ui::kDisplayEnv.data()) != nullptr) {
        g_setenv(ui::kGdkBackendEnv.data(), ui::kGdkBackendX11.data(), FALSE);
    }

    // Render in software: this window is a rarely-shown popup, so the GL renderer
    // only pulls the Mesa+LLVM GPU stack into RAM for no benefit. FALSE leaves any
    // user-set GSK_RENDERER untouched.
    g_setenv(ui::kGskRendererEnv.data(), ui::kGskRendererCairo.data(), FALSE);

    runtime::configure_logging();
    try {
        storage::JsonSettingsRepository settings_repo{config::settings_file()};
        core::SettingsService settings{settings_repo};

        storage::SqliteHistoryRepository history_repo{config::history_db()};
        core::SystemClock clock;
        core::HistoryService history{history_repo, clock, settings.settings().max_history_items};

        ui::Application app{history, settings, config::data_dir() / "last-clipboard", start_hidden};
        return app.run(forwarded_argc, forwarded_argv);
    } catch (const std::exception& error) {
        spdlog::critical("fatal: {}", error.what());
        return 1;
    }
}
