#include "core/Platform.hpp"

#include "core/Enums.hpp"
#include "core/detail/AsciiCase.hpp"

#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace copyclip::core {

namespace {

// Named to keep magic strings out of the logic. string_view over a static
// literal: .data() is NUL-terminated, as getenv requires.
inline constexpr std::string_view kXdgSessionTypeEnv = "XDG_SESSION_TYPE";
inline constexpr std::string_view kWaylandDisplayEnv = "WAYLAND_DISPLAY";
inline constexpr std::string_view kDisplayEnv = "DISPLAY";

// nullopt when unset; an empty value reads back as an empty string (callers
// decide whether empty counts as "set").
[[nodiscard]] std::optional<std::string_view> env_value(std::string_view name) {
    if (const char* raw = std::getenv(name.data()); raw != nullptr) {
        return std::string_view{raw};
    }
    return std::nullopt;
}

// Set to a non-empty value — mirrors the reference truthiness check
// (`os.environ.get(var)` is falsy when unset or empty).
[[nodiscard]] bool env_is_set_nonempty(std::string_view name) {
    const std::optional<std::string_view> value = env_value(name);
    return value.has_value() && !value->empty();
}

} // namespace

SessionType detect_session() {
    // 1. An explicitly declared session type wins, case-insensitively. Only the
    //    canonical "x11"/"wayland" values short-circuit; anything else (empty,
    //    "unknown", "tty", ...) falls through to the display-var checks below.
    const std::optional<std::string_view> declared_raw = env_value(kXdgSessionTypeEnv);
    const std::string declared =
        declared_raw.has_value() ? detail::ascii_lower(*declared_raw) : std::string{};
    if (declared == to_string(SessionType::X11)) {
        return SessionType::X11;
    }
    if (declared == to_string(SessionType::Wayland)) {
        return SessionType::Wayland;
    }

    // 2-3. Fall back to the display variables, Wayland taking precedence.
    if (env_is_set_nonempty(kWaylandDisplayEnv)) {
        return SessionType::Wayland;
    }
    if (env_is_set_nonempty(kDisplayEnv)) {
        return SessionType::X11;
    }

    // 4. Nothing decisive in the environment.
    return SessionType::Unknown;
}

} // namespace copyclip::core
