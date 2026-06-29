#pragma once

// Simulates the paste shortcut (Ctrl+V) into the focused window, using the input
// tool that fits the session (xdotool on X11; wtype then ydotool on Wayland).
// Best-effort: a no-op if no tool succeeds — the clip is on the clipboard either
// way, so the user can still paste manually.

#include "core/Enums.hpp"

namespace copyclip::ui {

class Paster {
public:
    explicit Paster(core::SessionType session);

    void paste() const;

private:
    core::SessionType session_;
};

} // namespace copyclip::ui
