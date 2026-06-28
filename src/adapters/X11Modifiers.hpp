#pragma once

// Pure modifier-to-X11-mask mapping, deliberately Xlib-free: Xlib's macros
// (None, Bool, ...) collide with GoogleTest's template headers, so tests can
// include this alongside <gtest/gtest.h>. The body lives in X11HotkeyListener.cpp.

#include "core/Enums.hpp"

#include <span>

namespace copyclip::adapters {

// Combine modifier enums into an X11 modifier mask (ShiftMask/ControlMask/...).
[[nodiscard]] unsigned int modifiers_to_mask(std::span<const core::Modifier> modifiers);

} // namespace copyclip::adapters
