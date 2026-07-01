#pragma once

// A reusable open-shortcut picker for the welcome and settings dialogs. It is a
// single AdwComboRow listing the built-in presets plus a trailing "Custom…"
// entry; choosing "Custom…" opens a sheet that records the next key combo. A
// custom shortcut appears as its own selected entry. Reports the chosen GNOME
// accelerator through a callback. libadwaita has no C++ binding, so it is driven
// through the C API.

#include <adwaita.h>

#include <functional>
#include <string>

namespace copyclip::ui {

class ShortcutChooser {
public:
    // Called with the new GNOME accelerator (e.g. "<Super>v") whenever it changes.
    using AcceleratorCallback = std::function<void(const std::string&)>;

    // Builds the combo row into `group`; `parent` is the widget capture sheets are
    // presented on. `initial` is the accelerator shown at first.
    ShortcutChooser(GtkWidget* parent, AdwPreferencesGroup* group, std::string initial,
                    AcceleratorCallback on_changed);
    // Tears down the capture sheet's signal handlers (bound to this) if one is
    // still open, so a later key/close event can't reach a destroyed chooser.
    ~ShortcutChooser();

    ShortcutChooser(const ShortcutChooser&) = delete;
    ShortcutChooser& operator=(const ShortcutChooser&) = delete;
    ShortcutChooser(ShortcutChooser&&) = delete;
    ShortcutChooser& operator=(ShortcutChooser&&) = delete;

    [[nodiscard]] const std::string& accelerator() const { return accelerator_; }

private:
    static void on_selected(GObject* row, GParamSpec* spec, gpointer self);
    static gboolean dispatch_selection(gpointer self);
    static void on_capture_closed(AdwDialog* dialog, gpointer self);
    static gboolean on_key_pressed(GtkEventControllerKey* controller, guint keyval, guint keycode,
                                   GdkModifierType state, gpointer self);
    static void on_key_released(GtkEventControllerKey* controller, guint keyval, guint keycode,
                                GdkModifierType state, gpointer self);

    void rebuild();
    void open_capture();
    void show_keys(guint keyval, GdkModifierType mods);
    void apply(const std::string& accelerator);
    [[nodiscard]] bool is_custom() const;
    [[nodiscard]] unsigned int current_index() const;

    GtkWidget* parent_;
    AcceleratorCallback on_changed_;
    std::string accelerator_;
    AdwComboRow* combo_row_;
    AdwDialog* capture_dialog_ = nullptr;          // non-null only while capturing
    GtkEventController* key_controller_ = nullptr; // capture-sheet key handler (bound to this)
    AdwStatusPage* capture_status_ = nullptr;      // the sheet's live "keys pressed" text
    std::string captured_;                         // valid combo held now, committed on release
    bool suppress_ = false;                        // ignore programmatic selection changes
    guint pending_ = 0;                            // idle source dispatching a user selection
};

} // namespace copyclip::ui
