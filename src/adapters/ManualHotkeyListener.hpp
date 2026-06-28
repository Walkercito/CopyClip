#pragma once

// Fallback for sessions with no global-shortcut mechanism: logs manual-binding
// guidance and is otherwise inert (never fires the callback). Mirrors the
// reference adapters/hotkeys/manual.py (NullHotkeyListener).

#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <functional>
#include <string>

namespace copyclip::adapters {

class ManualHotkeyListener final : public core::HotkeyListener {
public:
    ManualHotkeyListener(core::HotkeySpec spec, std::string command);

    // The activation callback is intentionally never invoked — nothing here can
    // grab the key.
    void start(std::function<void()> on_activate) override;
    void stop() override;
    bool rebind(const core::HotkeySpec& spec) override;

private:
    core::HotkeySpec spec_;
    std::string command_;
};

} // namespace copyclip::adapters
