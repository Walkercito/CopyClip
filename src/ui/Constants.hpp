#pragma once

// Named UI constants — sizes, ids, and layout metrics — so no bare literal
// appears in the widget code.

#include <string_view>

namespace copyclip::ui {

// Reverse-DNS application id (D-Bus name, .desktop, single-instance identity).
inline constexpr std::string_view kApplicationId{"dev.walkercito.CopyClip"};

inline constexpr int kWindowDefaultWidth = 420;
inline constexpr int kWindowDefaultHeight = 640;
inline constexpr int kWindowMinWidth = 360;
inline constexpr int kWindowMinHeight = 420;

} // namespace copyclip::ui
