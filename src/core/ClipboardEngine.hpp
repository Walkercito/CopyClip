#pragma once

// The ClipboardEngine facade: the composition seam between the platform sources
// and the application services.
//
// Mirrors the reference module copyclip/core/engine.py. It holds the injected
// adapters and services, wires their events together on start(), and exposes a
// show-request observer so a UI can subscribe without the core importing any UI
// toolkit. Collaborators are injected as non-owning std::reference_wrapper
// members (I.11/R.3): observed, never owned, outliving the engine. Pure core
// layer — no Qt, Xlib, or D-Bus.

#include "core/HistoryService.hpp"
#include "core/Interfaces.hpp"
#include "core/SettingsService.hpp"

#include <functional>
#include <string>
#include <vector>

namespace copyclip::core {

class ClipboardEngine {
public:
    ClipboardEngine(ClipboardSource& clipboard, HotkeyListener& hotkey, HistoryService& history,
                    SettingsService& settings);

    // Wire the clipboard source and hotkey listener to their handlers and begin
    // listening.
    void start();

    // Stop both sources.
    void stop();

    [[nodiscard]] HistoryService& history();
    [[nodiscard]] SettingsService& settings();

    // Register a callback fired whenever the hotkey requests the UI to show.
    void on_show_requested(std::function<void()> callback);

    // Write text to the system clipboard and record it in the history.
    void copy_to_clipboard(const std::string& content);

private:
    void on_clipboard_change(const std::string& text);
    void on_hotkey();

    std::reference_wrapper<ClipboardSource> clipboard_;
    std::reference_wrapper<HotkeyListener> hotkey_;
    std::reference_wrapper<HistoryService> history_;
    std::reference_wrapper<SettingsService> settings_;
    std::vector<std::function<void()>> show_callbacks_;
};

} // namespace copyclip::core
