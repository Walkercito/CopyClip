// Port of the reference oracle tests/core/test_enums.py.
//
// The Python reference uses StrEnum, so each enumerator *is* its string value and
// E(value) constructs from one. The C++ port models this with `enum class` plus
// two free functions per enum: to_string(E) yields the StrEnum value, and
// <enum>_from_string(s) returns std::optional<E> (nullopt replaces the raise).
// These cases assert the values, the round-trip, and nullopt on unknown input.

#include "core/Enums.hpp"

#include <optional>
#include <string_view>

#include <gtest/gtest.h>

namespace {

namespace core = copyclip::core;

// DRY round-trip check covering all five enums: to_string reproduces the value
// and from_string recovers the enumerator. `parse` is the per-enum from_string,
// passed as a callable so one helper serves every enum.
template <typename Enum, typename Parse>
void expect_round_trip(Enum value, std::string_view text, Parse parse) {
    EXPECT_EQ(core::to_string(value), text);
    // Compare optionals directly: checks engagement and value at once, with no
    // dereference of a possibly-empty optional.
    EXPECT_EQ(parse(text), std::optional<Enum>{value});
}

// to_string half: the exact StrEnum values.
TEST(EnumsTest, ToStringMatchesReferenceValues) {
    EXPECT_EQ(core::to_string(core::SessionType::X11), "x11");
    EXPECT_EQ(core::to_string(core::SessionType::Wayland), "wayland");
    EXPECT_EQ(core::to_string(core::SessionType::Unknown), "unknown");

    EXPECT_EQ(core::to_string(core::Modifier::Ctrl), "ctrl");
    EXPECT_EQ(core::to_string(core::Modifier::Alt), "alt");
    EXPECT_EQ(core::to_string(core::Modifier::Super), "super");
    EXPECT_EQ(core::to_string(core::Modifier::Shift), "shift");

    EXPECT_EQ(core::to_string(core::Key::V), "v");
    EXPECT_EQ(core::to_string(core::Key::C), "c");
    EXPECT_EQ(core::to_string(core::Key::X), "x");
    EXPECT_EQ(core::to_string(core::Key::Space), "space");

    EXPECT_EQ(core::to_string(core::Theme::Dark), "dark");
    EXPECT_EQ(core::to_string(core::Theme::Light), "light");
    EXPECT_EQ(core::to_string(core::Theme::System), "system");

    EXPECT_EQ(core::to_string(core::HotkeyPreset::SuperV), "super_v");
    EXPECT_EQ(core::to_string(core::HotkeyPreset::CtrlAltV), "ctrl_alt_v");
    EXPECT_EQ(core::to_string(core::HotkeyPreset::SuperC), "super_c");
    EXPECT_EQ(core::to_string(core::HotkeyPreset::CtrlShiftV), "ctrl_shift_v");
}

// from_string round-trip for every enumerator (StrEnum's value<->member map).
TEST(EnumsTest, FromStringRoundTripsEveryEnumerator) {
    expect_round_trip(core::SessionType::X11, "x11", core::session_type_from_string);
    expect_round_trip(core::SessionType::Wayland, "wayland", core::session_type_from_string);
    expect_round_trip(core::SessionType::Unknown, "unknown", core::session_type_from_string);

    expect_round_trip(core::Modifier::Ctrl, "ctrl", core::modifier_from_string);
    expect_round_trip(core::Modifier::Alt, "alt", core::modifier_from_string);
    expect_round_trip(core::Modifier::Super, "super", core::modifier_from_string);
    expect_round_trip(core::Modifier::Shift, "shift", core::modifier_from_string);

    expect_round_trip(core::Key::V, "v", core::key_from_string);
    expect_round_trip(core::Key::C, "c", core::key_from_string);
    expect_round_trip(core::Key::X, "x", core::key_from_string);
    expect_round_trip(core::Key::Space, "space", core::key_from_string);

    expect_round_trip(core::Theme::Dark, "dark", core::theme_from_string);
    expect_round_trip(core::Theme::Light, "light", core::theme_from_string);
    expect_round_trip(core::Theme::System, "system", core::theme_from_string);

    expect_round_trip(core::HotkeyPreset::SuperV, "super_v", core::hotkey_preset_from_string);
    expect_round_trip(core::HotkeyPreset::CtrlAltV, "ctrl_alt_v", core::hotkey_preset_from_string);
    expect_round_trip(core::HotkeyPreset::SuperC, "super_c", core::hotkey_preset_from_string);
    expect_round_trip(core::HotkeyPreset::CtrlShiftV, "ctrl_shift_v",
                      core::hotkey_preset_from_string);
}

// Unknown values yield std::nullopt — the replacement for StrEnum's raise.
// Covers all five enums, plus empty input and a case-only near-miss.
TEST(EnumsTest, FromStringReturnsNulloptForUnknownValue) {
    EXPECT_FALSE(core::session_type_from_string("mir").has_value());
    EXPECT_FALSE(core::session_type_from_string("X11").has_value());
    EXPECT_FALSE(core::modifier_from_string("meta").has_value());
    EXPECT_FALSE(core::key_from_string("z").has_value());
    EXPECT_FALSE(core::theme_from_string("solarized").has_value());
    EXPECT_FALSE(core::hotkey_preset_from_string("super_x").has_value());
    EXPECT_FALSE(core::session_type_from_string("").has_value());
}

} // namespace
