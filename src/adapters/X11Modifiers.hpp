#pragma once

// Pure modifier-to-X11-mask mapping, deliberately free of any Xlib include.
//
// Xlib's headers define macros (None, Bool, ...) that collide with GoogleTest's
// template headers, so this declaration is kept Xlib-free: tests can include it
// alongside <gtest/gtest.h> without pulling in <X11/Xlib.h>. The implementation
// (X11HotkeyListener.cpp) provides the body using the Xlib mask constants.

#include "core/Enums.hpp"

#include <span>

namespace copyclip::adapters {

// Combine modifier enums into an X11 modifier mask (ShiftMask/ControlMask/...).
[[nodiscard]] unsigned int modifiers_to_mask(std::span<const core::Modifier> modifiers);

} // namespace copyclip::adapters
