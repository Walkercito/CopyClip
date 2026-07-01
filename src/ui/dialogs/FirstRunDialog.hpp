#pragma once

// One-time welcome dialog: introduces CopyClip and lets the user choose the
// summon shortcut (free-form capture or a preset). On close it reports the chosen
// GNOME accelerator; the caller completes first run and registers the shortcut.
// Built with the libadwaita C API.

#include "ui/widgets/ShortcutChooser.hpp"

#include <adwaita.h>

#include <functional>
#include <memory>
#include <string>

namespace copyclip::ui {

class FirstRunDialog {
public:
    using FinishedCallback = std::function<void(const std::string&)>;

    FirstRunDialog(GtkWidget* parent, std::string initial_accelerator,
                   FinishedCallback on_finished);
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
    std::string accelerator_;
    AdwDialog* dialog_;
    std::unique_ptr<ShortcutChooser> chooser_;
};

} // namespace copyclip::ui
