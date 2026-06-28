#pragma once

// One-time welcome dialog: introduces CopyClip and lets the user pick the summon
// shortcut. On "Get Started" it reports the chosen preset (the caller completes
// first run and registers the shortcut). Built with the libadwaita C API.

#include "core/Enums.hpp"

#include <adwaita.h>

#include <functional>

namespace copyclip::ui {

class FirstRunDialog {
public:
    using FinishedCallback = std::function<void(core::HotkeyPreset)>;

    FirstRunDialog(GtkWidget* parent, core::HotkeyPreset initial, FinishedCallback on_finished);
    ~FirstRunDialog() = default;

    FirstRunDialog(const FirstRunDialog&) = delete;
    FirstRunDialog& operator=(const FirstRunDialog&) = delete;
    FirstRunDialog(FirstRunDialog&&) = delete;
    FirstRunDialog& operator=(FirstRunDialog&&) = delete;

private:
    static void on_get_started(GtkButton* button, gpointer self);
    static void on_closed(AdwDialog* dialog, gpointer self);
    void finish();

    FinishedCallback on_finished_;
    AdwComboRow* hotkey_row_;
    AdwDialog* dialog_;
};

} // namespace copyclip::ui
