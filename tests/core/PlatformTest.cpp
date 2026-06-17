// Port of the reference oracle tests/core/test_platform.py.
//
// detect_session() reads three environment variables — XDG_SESSION_TYPE,
// WAYLAND_DISPLAY, DISPLAY — to classify the display server. The host or CI may
// already have any of these set, so each case brings ALL THREE to a known state
// via ScopedEnv (set or unset, restored on scope exit) before asserting; this
// makes the tests deterministic regardless of the ambient environment.

#include "core/Platform.hpp"
#include "core/Enums.hpp"
#include "support/ScopedEnv.hpp"

#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace {

namespace core = copyclip::core;
using copyclip::test::ScopedEnv;

// Desired state of the three session-detection variables for a single test. A
// field holding a value sets that variable; std::nullopt unsets it. A named
// struct with designated initializers makes each variable's state explicit at the
// call site — the three states share a type, so positional arguments would be
// trivially swappable; naming them removes that hazard entirely.
struct SessionEnv {
    std::optional<std::string> xdg_session_type;
    std::optional<std::string> wayland_display;
    std::optional<std::string> display;
};

// Fixture: pins the three session-detection variables for the lifetime of each
// test, then restores the prior environment on teardown. This keeps the
// per-variable wiring DRY across cases — no test re-states the variable names —
// and guarantees a clean, known starting point regardless of the host.
//
// Each guard lives in its own std::optional member rather than a vector:
// ScopedEnv is deliberately non-movable (moving an RAII env override risks a
// double restore), so emplacing in place avoids any relocation while preserving
// deterministic reverse-declaration teardown.
class PlatformTest : public ::testing::Test {
protected:
    void pin(const SessionEnv& env) {
        xdg_session_type_guard_.emplace("XDG_SESSION_TYPE", env.xdg_session_type);
        wayland_display_guard_.emplace("WAYLAND_DISPLAY", env.wayland_display);
        display_guard_.emplace("DISPLAY", env.display);
    }

private:
    std::optional<ScopedEnv> xdg_session_type_guard_;
    std::optional<ScopedEnv> wayland_display_guard_;
    std::optional<ScopedEnv> display_guard_;
};

// test_explicit_session_type_wins: declared wayland beats any display vars.
TEST_F(PlatformTest, ExplicitWaylandSessionTypeWins) {
    pin({.xdg_session_type = "wayland", .wayland_display = "wayland-0", .display = ":0"});
    EXPECT_EQ(core::detect_session(), core::SessionType::Wayland);
}

// Companion to the above: declared x11 selects X11 directly.
TEST_F(PlatformTest, ExplicitX11SessionTypeWins) {
    pin({.xdg_session_type = "x11", .wayland_display = "wayland-0", .display = ":0"});
    EXPECT_EQ(core::detect_session(), core::SessionType::X11);
}

// The declared value is ASCII-case-insensitive, mirroring the reference's
// `.lower()`.
TEST_F(PlatformTest, ExplicitSessionTypeIsCaseInsensitive) {
    pin({.xdg_session_type = "WAYLAND", .wayland_display = std::nullopt, .display = std::nullopt});
    EXPECT_EQ(core::detect_session(), core::SessionType::Wayland);
}

// test_falls_back_to_display_vars: no declared type, no WAYLAND_DISPLAY, DISPLAY
// set -> X11.
TEST_F(PlatformTest, FallsBackToDisplayVars) {
    pin({.xdg_session_type = std::nullopt, .wayland_display = std::nullopt, .display = ":0"});
    EXPECT_EQ(core::detect_session(), core::SessionType::X11);
}

// WAYLAND_DISPLAY set (and no declared type) -> Wayland, ahead of DISPLAY.
TEST_F(PlatformTest, WaylandDisplayImpliesWayland) {
    pin({.xdg_session_type = std::nullopt, .wayland_display = "wayland-0", .display = ":0"});
    EXPECT_EQ(core::detect_session(), core::SessionType::Wayland);
}

// An unrecognized XDG_SESSION_TYPE must NOT short-circuit: it falls through to
// the display-var logic (matches the reference, which only special-cases
// "x11"/"wayland").
TEST_F(PlatformTest, UnrecognizedSessionTypeFallsThroughToDisplayVars) {
    pin({.xdg_session_type = "tty", .wayland_display = std::nullopt, .display = ":0"});
    EXPECT_EQ(core::detect_session(), core::SessionType::X11);
}

// "unknown" is not special-cased either: with no display vars it still yields
// Unknown, but only via the fall-through (not by parsing the literal).
TEST_F(PlatformTest, UnknownLiteralFallsThroughAndYieldsUnknown) {
    pin({.xdg_session_type = "unknown", .wayland_display = std::nullopt, .display = std::nullopt});
    EXPECT_EQ(core::detect_session(), core::SessionType::Unknown);
}

// An empty WAYLAND_DISPLAY is treated as unset; DISPLAY then decides.
TEST_F(PlatformTest, EmptyWaylandDisplayIsIgnored) {
    pin({.xdg_session_type = std::nullopt, .wayland_display = "", .display = ":0"});
    EXPECT_EQ(core::detect_session(), core::SessionType::X11);
}

// An empty DISPLAY is treated as unset; with nothing else set -> Unknown.
TEST_F(PlatformTest, EmptyDisplayIsIgnored) {
    pin({.xdg_session_type = std::nullopt, .wayland_display = std::nullopt, .display = ""});
    EXPECT_EQ(core::detect_session(), core::SessionType::Unknown);
}

// test_unknown_when_nothing_set: all three unset -> Unknown.
TEST_F(PlatformTest, UnknownWhenNothingSet) {
    pin({.xdg_session_type = std::nullopt,
         .wayland_display = std::nullopt,
         .display = std::nullopt});
    EXPECT_EQ(core::detect_session(), core::SessionType::Unknown);
}

} // namespace
