#pragma once

// Domain enumerations and their string representations.
//
// Mirrors copyclip/core/enums.py (Python StrEnum, where an enumerator *is* its
// string value). The string values must match the reference: HotkeySpec::
// display_name and the storage/settings layers depend on them. Each enum has a
// single constexpr {enumerator, value} table feeding both to_string (the value)
// and <enum>_from_string (parse, std::nullopt on miss — replacing StrEnum's
// raise), so the mapping never diverges and no literal is repeated in logic.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace copyclip::core {

// Each value set is tiny, so the enums use a single-byte underlying type; the
// explicit type also fixes the representation across translation units.

enum class SessionType : std::uint8_t { X11, Wayland, Unknown };

enum class Modifier : std::uint8_t { Ctrl, Alt, Super, Shift };

enum class Key : std::uint8_t { V, C, X, Space };

enum class Theme : std::uint8_t { Dark, Light, System };

enum class HotkeyPreset : std::uint8_t { SuperV, CtrlAltV, SuperC, CtrlShiftV };

// The kind of content a clipboard entry holds.
enum class ClipKind : std::uint8_t { Text, RichText, Image };

namespace detail {

// One {enumerator, value} pair. `text` views a string literal (static storage),
// so it outlives any caller.
template <typename Enum> struct EnumEntry {
    Enum value;
    std::string_view text;
};

// Every enumerator appears in its table, so this always finds a match; the
// trailing return covers the unreachable miss without a magic sentinel.
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

// Returns std::nullopt when no entry matches (replacing StrEnum's raise).
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

// Single source of truth per enum, feeding both to_string and from_string.
// Order matches the reference declarations.
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

inline constexpr std::array<EnumEntry<ClipKind>, 3> kClipKindTable{{
    {ClipKind::Text, "text"},
    {ClipKind::RichText, "richtext"},
    {ClipKind::Image, "image"},
}};

} // namespace detail

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

[[nodiscard]] constexpr std::string_view to_string(ClipKind value) {
    return detail::to_string_impl(value, detail::kClipKindTable);
}

[[nodiscard]] constexpr std::optional<ClipKind> clip_kind_from_string(std::string_view text) {
    return detail::from_string_impl(text, detail::kClipKindTable);
}

} // namespace copyclip::core
