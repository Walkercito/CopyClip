#pragma once

// Named UI constants — sizes, ids, and layout metrics — so no bare literal
// appears in the widget code.

#include <cstddef>
#include <string_view>

namespace copyclip::ui {

// Reverse-DNS application id (D-Bus name, .desktop, single-instance identity).
inline constexpr std::string_view kApplicationId{"dev.walkercito.CopyClip"};

inline constexpr int kWindowDefaultWidth = 420;
inline constexpr int kWindowDefaultHeight = 640;
inline constexpr int kWindowMinWidth = 360;
inline constexpr int kWindowMinHeight = 420;

// Margin around the content area and the list, in pixels.
inline constexpr int kContentMargin = 12;

// Width of presented dialogs, so they sit as inset cards rather than full-width
// sheets.
inline constexpr int kDialogContentWidth = 400;

// Code points shown on a collapsed card before truncation.
inline constexpr std::size_t kMaxPreviewChars = 120;

// Startup flag (written into the autostart .desktop Exec) that begins hidden in
// the background while still capturing. Kept in sync with scripts/install.sh.
inline constexpr std::string_view kBackgroundFlag{"--background"};

// Environment knobs read/set at startup to prefer the X11/XWayland GDK backend
// (.data() is NUL-terminated, as the GLib env helpers require).
inline constexpr std::string_view kDisplayEnv{"DISPLAY"};
inline constexpr std::string_view kGdkBackendEnv{"GDK_BACKEND"};
inline constexpr std::string_view kGdkBackendX11{"x11"};

// Force GTK's software (cairo) GSK renderer. The default GL renderer pulls the
// Mesa+LLVM GPU stack into the process for a popup that is hidden almost all the
// time, where GPU acceleration buys nothing; cairo draws the small window with
// no GL context.
inline constexpr std::string_view kGskRendererEnv{"GSK_RENDERER"};
inline constexpr std::string_view kGskRendererCairo{"cairo"};

// Cap glibc's per-thread malloc arenas. GLib/GTK spawn ~10 helper threads, and
// glibc's default (8×CPUs arenas) keeps freed memory off the OS so RSS only ever
// grows; two arenas suffice for a mostly-idle background app.
inline constexpr int kMallocArenaMax = 2;

} // namespace copyclip::ui
