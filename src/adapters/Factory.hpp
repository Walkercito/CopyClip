#pragma once

// Builds the concrete adapters for the detected session, with safe fallbacks.
//
// Mirrors the reference adapters/factory.py. Builders are injected (defaulting
// to the real adapters) so the selection logic is testable without pulling in
// Qt or Xlib. This header is deliberately free of any Qt/Xlib include: the
// concrete adapters are referenced only in Factory.cpp, so a test (or any
// consumer) can include this header alongside <gtest/gtest.h> safely.

#include "core/Enums.hpp"
#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace copyclip::adapters {

using HotkeyBuilder = std::function<std::unique_ptr<core::HotkeyListener>(const core::HotkeySpec&)>;

// Select the hotkey listener for `session`. Falls back to the manual listener on
// an unknown session or when the chosen builder throws. `builders` is injectable
// for testing; std::nullopt uses the real session -> adapter map.
[[nodiscard]] std::unique_ptr<core::HotkeyListener> select_hotkey_listener(
    core::SessionType session, const core::HotkeySpec& spec, const std::string& command,
    const std::optional<std::map<core::SessionType, HotkeyBuilder>>& builders = std::nullopt);

// Build the production clipboard source (QClipboard-backed). Requires a
// QGuiApplication to exist.
[[nodiscard]] std::unique_ptr<core::ClipboardSource> build_clipboard_source();

} // namespace copyclip::adapters
