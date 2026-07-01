#pragma once

// Application logic for typed settings, backed by a SettingsRepository. Mirrors
// reference core/settings.py: load once on construction, hold the current value,
// and persist every change immediately through the injected repository.
//
// The repository is a non-owning std::reference_wrapper (I.11/R.3): observed,
// never owned, outliving the service. A reference_wrapper rather than a bare
// reference keeps the service assignable (a reference member would silently
// delete copy-assignment). Pure core layer — no Qt, Xlib, or D-Bus.

#include "core/Enums.hpp"
#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <functional>
#include <string>

namespace copyclip::core {

class SettingsService {
public:
    explicit SettingsService(SettingsRepository& repository);

    [[nodiscard]] const Settings& settings() const;

    // Replace the settings wholesale and persist them. With no Python **kwargs, a
    // caller changing one field reads settings(), copies, mutates, and passes the
    // result back; untouched fields carry over unchanged.
    void update(const Settings& settings);

    [[nodiscard]] bool is_first_run() const;

    // Record the chosen shortcut (a GNOME accelerator) and mark the first run
    // complete (persisted).
    void complete_first_run(const std::string& accelerator);

private:
    std::reference_wrapper<SettingsRepository> repository_;
    Settings settings_;
};

} // namespace copyclip::core
