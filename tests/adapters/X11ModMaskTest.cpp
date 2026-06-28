#include "adapters/X11Modifiers.hpp"

#include "core/Enums.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

using copyclip::adapters::modifiers_to_mask;
using copyclip::core::Modifier;

// The X11 modifier mask bits are stable X protocol constants (defined in
// <X11/X.h>), named here so this GoogleTest translation unit need not include
// any Xlib header — Xlib's macros (None, Bool, ...) collide with GoogleTest's
// template headers. Values cross-checked against X.h: ShiftMask = 1<<0,
// ControlMask = 1<<2, Mod4Mask = 1<<6.
constexpr unsigned int kShiftMask = 1U << 0U;
constexpr unsigned int kControlMask = 1U << 2U;
constexpr unsigned int kMod4Mask = 1U << 6U;

// Mirrors tests/adapters/test_x11_modmask.py: the pure modifier-to-mask mapping.

TEST(X11ModMaskTest, CombinesBits) {
    const std::vector<Modifier> modifiers{Modifier::Ctrl, Modifier::Shift};
    EXPECT_EQ(modifiers_to_mask(modifiers), kControlMask | kShiftMask);
}

TEST(X11ModMaskTest, SuperMapsToMod4) {
    const std::vector<Modifier> modifiers{Modifier::Super};
    EXPECT_EQ(modifiers_to_mask(modifiers), kMod4Mask);
}

TEST(X11ModMaskTest, EmptyIsZero) {
    const std::vector<Modifier> modifiers;
    EXPECT_EQ(modifiers_to_mask(modifiers), 0U);
}

} // namespace
