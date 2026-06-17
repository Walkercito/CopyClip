#pragma once

// Domain enumerations and their string representations.
//
// Mirrors the reference module copyclip/core/enums.py, where each type is a
// Python StrEnum: an enumerator *is* its string value, and `E(value)`
// constructs from a string (raising on an unknown value). The C++ port keeps
// the same string values — Models::HotkeySpec::display_name and the
// storage/settings layers depend on them — but expresses the mapping idiomatically:
//
//   - `enum class` (never plain enum), enumerators in PascalCase.
//   - to_string(E)            -> std::string_view : the StrEnum value.
//   - <enum>_from_string(sv)  -> std::optional<E> : parses a value, returning
//                                std::nullopt for an unknown one (the C++
//                                replacement for StrEnum raising).
//
// Each enum has exactly one source of truth — a constexpr table of
// {enumerator, value} pairs — consumed by both directions, so the two never
// diverge and no string literal is repeated in logic. Pure, header-only, and
// free of Qt/Xlib/D-Bus: this lives in the core layer.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace copyclip::core {

// --- Enumerations ------------------------------------------------------------
//
// Each value set is tiny, so the enums are sized to a single byte; the explicit
// underlying type also fixes the representation across translation units.

enum class SessionType : std::uint8_t { X11, Wayland, Unknown };

enum class Modifier : std::uint8_t { Ctrl, Alt, Super, Shift };

enum class Key : std::uint8_t { V, C, X, Space };

enum class Theme : std::uint8_t { Dark, Light, System };

enum class HotkeyPreset : std::uint8_t { SuperV, CtrlAltV, SuperC, CtrlShiftV };

// --- Mapping machinery -------------------------------------------------------

namespace detail {

// One {enumerator, string value} pair. `Enum` is the scoped enum; the value is a
// view over a string literal with static storage duration, so it outlives any
// caller.
template <typename Enum> struct EnumEntry {
    Enum value;
    std::string_view text;
};

// Maps an enumerator to its string value by scanning the enum's single source
// of truth. Every enumerator appears in its table, so the lookup always
// succeeds; the trailing return covers the (unreachable) miss without a magic
// sentinel string.
template <typename Enum, std::size_t N>
[[nodiscard]] constexpr std::string_view
to_string_impl(Enum value, const std::array<EnumEntry<Enum>, N>& table) {
    for (const auto& entry : table) {
        if (entry.value == value) {
            return entry.text;
        }
    }
    return {};
}

// Parses a string into an enumerator, returning std::nullopt when no entry
// matches. This is the idiomatic replacement for StrEnum's exception on an
// unknown value.
template <typename Enum, std::size_t N>
[[nodiscard]] constexpr std::optional<Enum>
from_string_impl(std::string_view text, const std::array<EnumEntry<Enum>, N>& table) {
    for (const auto& entry : table) {
        if (entry.text == text) {
            return entry.value;
        }
    }
    return std::nullopt;
}

// Single source of truth per enum: the enumerator<->value correspondence used by
// both to_string and from_string. Order matches the reference declarations.
inline constexpr std::array<EnumEntry<SessionType>, 3> kSessionTypeTable{{
    {SessionType::X11, "x11"},
    {SessionType::Wayland, "wayland"},
    {SessionType::Unknown, "unknown"},
}};

inline constexpr std::array<EnumEntry<Modifier>, 4> kModifierTable{{
    {Modifier::Ctrl, "ctrl"},
    {Modifier::Alt, "alt"},
    {Modifier::Super, "super"},
    {Modifier::Shift, "shift"},
}};

inline constexpr std::array<EnumEntry<Key>, 4> kKeyTable{{
    {Key::V, "v"},
    {Key::C, "c"},
    {Key::X, "x"},
    {Key::Space, "space"},
}};

inline constexpr std::array<EnumEntry<Theme>, 3> kThemeTable{{
    {Theme::Dark, "dark"},
    {Theme::Light, "light"},
    {Theme::System, "system"},
}};

inline constexpr std::array<EnumEntry<HotkeyPreset>, 4> kHotkeyPresetTable{{
    {HotkeyPreset::SuperV, "super_v"},
    {HotkeyPreset::CtrlAltV, "ctrl_alt_v"},
    {HotkeyPreset::SuperC, "super_c"},
    {HotkeyPreset::CtrlShiftV, "ctrl_shift_v"},
}};

} // namespace detail

// --- to_string overloads -----------------------------------------------------

[[nodiscard]] constexpr std::string_view to_string(SessionType value) {
    return detail::to_string_impl(value, detail::kSessionTypeTable);
}

[[nodiscard]] constexpr std::string_view to_string(Modifier value) {
    return detail::to_string_impl(value, detail::kModifierTable);
}

[[nodiscard]] constexpr std::string_view to_string(Key value) {
    return detail::to_string_impl(value, detail::kKeyTable);
}

[[nodiscard]] constexpr std::string_view to_string(Theme value) {
    return detail::to_string_impl(value, detail::kThemeTable);
}

[[nodiscard]] constexpr std::string_view to_string(HotkeyPreset value) {
    return detail::to_string_impl(value, detail::kHotkeyPresetTable);
}

// --- from_string functions ---------------------------------------------------

[[nodiscard]] constexpr std::optional<SessionType> session_type_from_string(std::string_view text) {
    return detail::from_string_impl(text, detail::kSessionTypeTable);
}

[[nodiscard]] constexpr std::optional<Modifier> modifier_from_string(std::string_view text) {
    return detail::from_string_impl(text, detail::kModifierTable);
}

[[nodiscard]] constexpr std::optional<Key> key_from_string(std::string_view text) {
    return detail::from_string_impl(text, detail::kKeyTable);
}

[[nodiscard]] constexpr std::optional<Theme> theme_from_string(std::string_view text) {
    return detail::from_string_impl(text, detail::kThemeTable);
}

[[nodiscard]] constexpr std::optional<HotkeyPreset>
hotkey_preset_from_string(std::string_view text) {
    return detail::from_string_impl(text, detail::kHotkeyPresetTable);
}

} // namespace copyclip::core
