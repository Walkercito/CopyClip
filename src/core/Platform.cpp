#include "core/Platform.hpp"

#include "core/Enums.hpp"

#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace copyclip::core {

namespace {

// Environment variables consulted by detect_session, named so no magic string
// appears in the logic below. Stored as std::string_view over string literals
// with static storage duration; .data() yields the NUL-terminated C string
// getenv requires.
inline constexpr std::string_view kXdgSessionTypeEnv = "XDG_SESSION_TYPE";
inline constexpr std::string_view kWaylandDisplayEnv = "WAYLAND_DISPLAY";
inline constexpr std::string_view kDisplayEnv = "DISPLAY";

// Distance between an uppercase ASCII letter and its lowercase counterpart.
// Named so the case-folding arithmetic carries no bare literal.
inline constexpr char kAsciiCaseShift = 'a' - 'A';

// Locale-independent ASCII lowercase of a single character. Only A-Z are folded;
// every other byte (including non-ASCII) is returned unchanged. Kept narrow and
// -Wconversion-safe: the arithmetic stays in `int` and the result is narrowed
// back with an explicit static_cast.
[[nodiscard]] char ascii_to_lower(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch + kAsciiCaseShift);
    }
    return ch;
}

// ASCII-lowercase a whole string, matching the reference's str.lower() for the
// ASCII inputs detect_session compares against.
[[nodiscard]] std::string ascii_to_lower(std::string_view text) {
    std::string lowered;
    lowered.reserve(text.size());
    for (const char ch : text) {
        lowered.push_back(ascii_to_lower(ch));
    }
    return lowered;
}

// Read an environment variable, returning std::nullopt when it is unset. An
// empty value is reported as an empty string (the caller decides whether empty
// counts as "set").
[[nodiscard]] std::optional<std::string_view> env_value(std::string_view name) {
    if (const char* raw = std::getenv(name.data()); raw != nullptr) {
        return std::string_view{raw};
    }
    return std::nullopt;
}

// Whether an environment variable is set to a non-empty value. Mirrors the
// reference's truthiness check (`os.environ.get(var)` is falsy when unset or
// empty).
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
        declared_raw.has_value() ? ascii_to_lower(*declared_raw) : std::string{};
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
