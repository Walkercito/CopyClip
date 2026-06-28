#include "core/SettingsService.hpp"

namespace copyclip::core {

SettingsService::SettingsService(SettingsRepository& repository)
    : repository_{repository}, settings_{repository.load()} {}

const Settings& SettingsService::settings() const {
    return settings_;
}

void SettingsService::update(const Settings& settings) {
    settings_ = settings;
    repository_.get().save(settings_);
}

bool SettingsService::is_first_run() const {
    return !settings_.first_run_completed;
}

void SettingsService::complete_first_run(HotkeyPreset hotkey) {
    Settings next = settings_;
    next.hotkey = hotkey;
    next.first_run_completed = true;
    update(next);
}

} // namespace copyclip::core
