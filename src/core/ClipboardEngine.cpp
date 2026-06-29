#include "core/ClipboardEngine.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace copyclip::core {

ClipboardEngine::ClipboardEngine(ClipboardSource& clipboard, HotkeyListener& hotkey,
                                 HistoryService& history, SettingsService& settings)
    : clipboard_{clipboard}, hotkey_{hotkey}, history_{history}, settings_{settings} {}

void ClipboardEngine::start() {
    clipboard_.get().start([this](const ClipContent& content) { on_clipboard_change(content); });
    hotkey_.get().start([this] { on_hotkey(); });
    spdlog::info("engine started");
}

void ClipboardEngine::stop() {
    clipboard_.get().stop();
    hotkey_.get().stop();
    spdlog::info("engine stopped");
}

HistoryService& ClipboardEngine::history() {
    return history_.get();
}

SettingsService& ClipboardEngine::settings() {
    return settings_.get();
}

void ClipboardEngine::on_show_requested(std::function<void()> callback) {
    show_callbacks_.push_back(std::move(callback));
}

void ClipboardEngine::copy_to_clipboard(const std::string& content) {
    clipboard_.get().write(ClipContent{.kind = ClipKind::Text, .text = content});
    history_.get().add(content);
}

void ClipboardEngine::on_clipboard_change(const ClipContent& content) {
    history_.get().add(content);
}

void ClipboardEngine::on_hotkey() {
    for (const std::function<void()>& callback : show_callbacks_) {
        callback();
    }
}

} // namespace copyclip::core
