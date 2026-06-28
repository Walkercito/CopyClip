#pragma once

// Pure session detection from environment variables only — no Qt/Xlib/D-Bus, so
// it stays unit-testable with no display server. Mirrors reference core/platform.py.

#include "core/Enums.hpp"

namespace copyclip::core {

// Detect the active display-server session. Resolution order (per the reference):
//   1. XDG_SESSION_TYPE, ASCII-lowercased: only "x11"/"wayland" match; any other
//      value (including "unknown" or empty) falls through.
//   2. WAYLAND_DISPLAY set and non-empty -> Wayland.
//   3. DISPLAY set and non-empty -> X11.
//   4. Otherwise -> Unknown.
[[nodiscard]] SessionType detect_session();

} // namespace copyclip::core
