#pragma once

// Application logic for typed settings, backed by a SettingsRepository.
//
// Mirrors the reference module copyclip/core/settings.py: the service loads the
// settings once on construction, holds the current value, and persists every
// change immediately through the injected repository.
//
// The repository is injected as a non-owning std::reference_wrapper (I.11/R.3):
// it is observed, never owned, and outlives the service. A reference_wrapper
// (rather than a bare reference) keeps the service assignable and avoids the
// silent loss of copy-assignment that a reference data member would cause. Pure
// core layer — no Qt, Xlib, or D-Bus.

#include "core/Enums.hpp"
#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <functional>

namespace copyclip::core {

class SettingsService {
public:
    explicit SettingsService(SettingsRepository& repository);

    // The current settings.
    [[nodiscard]] const Settings& settings() const;

    // Replace the settings wholesale and persist them. C++ has no Python
    // **kwargs, so a caller that wants to change a single field reads settings(),
    // copies it, mutates the field(s) it wants, and passes the result here; the
    // untouched fields are carried over unchanged.
    void update(const Settings& settings);

    // True until the first run has been completed.
    [[nodiscard]] bool is_first_run() const;

    // Record the chosen hotkey and mark the first run complete (persisted).
    void complete_first_run(HotkeyPreset hotkey);

private:
    std::reference_wrapper<SettingsRepository> repository_;
    Settings settings_;
};

} // namespace copyclip::core
