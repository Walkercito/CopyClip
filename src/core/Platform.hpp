#pragma once

// Pure session detection (no display-server libraries).
//
// Mirrors the reference module copyclip/core/platform.py: classify the current
// session as X11, Wayland, or Unknown by inspecting environment variables only.
// This lives in the core layer, so it deliberately links no Qt, Xlib, or D-Bus —
// it reads XDG_SESSION_TYPE, WAYLAND_DISPLAY, and DISPLAY via getenv and nothing
// more, which keeps it unit-testable with no display server present.

#include "core/Enums.hpp"

namespace copyclip::core {

// Detect the active display-server session from the environment.
//
// Resolution order (matching the reference exactly):
//   1. XDG_SESSION_TYPE, ASCII-lowercased: "x11" -> X11, "wayland" -> Wayland.
//      Any other value (including "unknown" or empty) falls through.
//   2. WAYLAND_DISPLAY set and non-empty -> Wayland.
//   3. DISPLAY set and non-empty -> X11.
//   4. Otherwise -> Unknown.
[[nodiscard]] SessionType detect_session();

} // namespace copyclip::core
