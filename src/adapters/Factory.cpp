#include "adapters/Factory.hpp"

#include "adapters/ManualHotkeyListener.hpp"
#include "adapters/PortalHotkeyListener.hpp"
#include "adapters/QtClipboardSource.hpp"
#include "adapters/X11HotkeyListener.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <map>
#include <memory>
#include <utility>

namespace copyclip::adapters {

namespace {

// The real session -> hotkey-backend map. Kept out of the header (and built
// lazily here) so that merely declaring select_hotkey_listener never pulls in
// Qt or Xlib.
std::map<core::SessionType, HotkeyBuilder> default_builders() {
    std::map<core::SessionType, HotkeyBuilder> builders;
    builders[core::SessionType::X11] = [](const core::HotkeySpec& spec) {
        return std::make_unique<X11HotkeyListener>(spec);
    };
    builders[core::SessionType::Wayland] = [](const core::HotkeySpec& spec) {
        return std::make_unique<PortalHotkeyListener>(spec);
    };
    return builders;
}

} // namespace

std::unique_ptr<core::HotkeyListener>
select_hotkey_listener(core::SessionType session, const core::HotkeySpec& spec,
                       const std::string& command,
                       const std::optional<std::map<core::SessionType, HotkeyBuilder>>& builders) {
    const std::map<core::SessionType, HotkeyBuilder> resolved =
        builders.has_value() ? *builders : default_builders();

    const auto entry = resolved.find(session);
    if (entry != resolved.end() && entry->second) {
        try {
            return entry->second(spec);
        } catch (const std::exception& error) {
            // Any backend failure degrades to the manual fallback.
            spdlog::warn("hotkey backend for {} unavailable ({}); using manual", to_string(session),
                         error.what());
        }
    }
    return std::make_unique<ManualHotkeyListener>(spec, command);
}

std::unique_ptr<core::ClipboardSource> build_clipboard_source() {
    return std::make_unique<QtClipboardSource>();
}

} // namespace copyclip::adapters
