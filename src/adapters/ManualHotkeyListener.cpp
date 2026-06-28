#include "adapters/ManualHotkeyListener.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace copyclip::adapters {

ManualHotkeyListener::ManualHotkeyListener(core::HotkeySpec spec, std::string command)
    : spec_{std::move(spec)}, command_{std::move(command)} {}

void ManualHotkeyListener::start(std::function<void()> /*on_activate*/) {
    spdlog::warn("Global hotkeys unavailable here. Bind {} to run '{}' in your desktop settings.",
                 spec_.display_name(), command_);
}

void ManualHotkeyListener::stop() {}

bool ManualHotkeyListener::rebind(const core::HotkeySpec& spec) {
    spec_ = spec;
    return true;
}

} // namespace copyclip::adapters
