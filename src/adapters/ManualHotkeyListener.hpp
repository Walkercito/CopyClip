#pragma once

// Fallback hotkey listener: no in-process key grabbing.
//
// Mirrors the reference adapters/hotkeys/manual.py (NullHotkeyListener). Used by
// the factory when no global-shortcut mechanism is available for the session; it
// logs guidance telling the user how to bind the shortcut manually and is
// otherwise inert (it never fires the activation callback).

#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <functional>
#include <string>

namespace copyclip::adapters {

class ManualHotkeyListener final : public core::HotkeyListener {
public:
    ManualHotkeyListener(core::HotkeySpec spec, std::string command);

    // Log manual-setup guidance. The activation callback is intentionally never
    // invoked — there is no mechanism here to grab the key.
    void start(std::function<void()> on_activate) override;
    void stop() override;
    bool rebind(const core::HotkeySpec& spec) override;

private:
    core::HotkeySpec spec_;
    std::string command_;
};

} // namespace copyclip::adapters
