#include "ui/widgets/ShortcutChooser.hpp"

#include "ui/Constants.hpp"
#include "ui/ShortcutText.hpp"

#include <adwaita.h>
#include <gdk/gdkkeysyms.h>

#include <string>
#include <utility>
#include <vector>

namespace copyclip::ui {

namespace {

// The trailing combo entry that opens the capture sheet.
constexpr const char* kCustomLabel = "Custom…";

// Shown in the capture sheet until keys are held.
constexpr const char* kCapturePrompt =
    "Use at least one modifier, e.g. Super or Ctrl. Esc to cancel.";

// The accelerator mask for a lone modifier keyval, so the live preview can show a
// modifier the moment it is pressed (its bit isn't in the event state yet).
[[nodiscard]] GdkModifierType modifier_mask(guint keyval) {
    switch (keyval) {
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
        return GDK_CONTROL_MASK;
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
        return GDK_SHIFT_MASK;
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
        return GDK_ALT_MASK;
    case GDK_KEY_Super_L:
    case GDK_KEY_Super_R:
        return GDK_SUPER_MASK;
    default:
        return static_cast<GdkModifierType>(0);
    }
}

// A lone modifier press is not a shortcut on its own — wait for the real key.
[[nodiscard]] bool is_modifier_keyval(guint keyval) {
    switch (keyval) {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
    case GDK_KEY_Super_L:
    case GDK_KEY_Super_R:
    case GDK_KEY_Hyper_L:
    case GDK_KEY_Hyper_R:
    case GDK_KEY_ISO_Level3_Shift:
    case GDK_KEY_Caps_Lock:
    case GDK_KEY_Num_Lock:
        return true;
    default:
        return false;
    }
}

// Human label for an accelerator, e.g. "<Super>v" -> "Super+V".
[[nodiscard]] std::string accelerator_label(const std::string& accelerator) {
    guint keyval = 0;
    auto mods = static_cast<GdkModifierType>(0);
    gtk_accelerator_parse(accelerator.c_str(), &keyval, &mods);
    char* label = gtk_accelerator_get_label(keyval, mods);
    std::string out = (label != nullptr && *label != '\0') ? label : accelerator;
    g_free(label);
    return out;
}

} // namespace

ShortcutChooser::ShortcutChooser(GtkWidget* parent, AdwPreferencesGroup* group, std::string initial,
                                 AcceleratorCallback on_changed)
    : parent_{parent}, on_changed_{std::move(on_changed)}, accelerator_{std::move(initial)},
      combo_row_{ADW_COMBO_ROW(adw_combo_row_new())} {
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(combo_row_), "Open CopyClip");
    gtk_widget_set_tooltip_text(GTK_WIDGET(combo_row_), "Key combination that summons the window");
    adw_preferences_group_add(group, GTK_WIDGET(combo_row_));
    g_signal_connect(combo_row_, "notify::selected", G_CALLBACK(&ShortcutChooser::on_selected),
                     this);
    rebuild();
}

ShortcutChooser::~ShortcutChooser() {
    if (pending_ != 0) {
        g_source_remove(pending_);
    }
    if (capture_dialog_ != nullptr) {
        // Drop every handler bound to this before closing: the "closed" handler on
        // the dialog, and the key handlers on its controller. close() animates, so
        // without this a key event could still reach a destroyed chooser.
        if (key_controller_ != nullptr) {
            g_signal_handlers_disconnect_by_data(key_controller_, this);
        }
        g_signal_handlers_disconnect_by_data(capture_dialog_, this);
        adw_dialog_close(capture_dialog_);
    }
}

bool ShortcutChooser::is_custom() const {
    return current_index() == static_cast<unsigned int>(quick_picks().size());
}

unsigned int ShortcutChooser::current_index() const {
    const std::vector<QuickPick> picks = quick_picks();
    for (unsigned int i = 0; i < picks.size(); ++i) {
        if (picks.at(i).accelerator == accelerator_) {
            return i;
        }
    }
    return static_cast<unsigned int>(picks.size()); // the custom entry, listed after the presets
}

void ShortcutChooser::rebuild() {
    suppress_ = true;
    GtkStringList* model = gtk_string_list_new(nullptr);
    for (const QuickPick& pick : quick_picks()) {
        gtk_string_list_append(model, pick.label.c_str());
    }
    if (is_custom()) {
        gtk_string_list_append(model, accelerator_label(accelerator_).c_str());
    }
    gtk_string_list_append(model, kCustomLabel);
    adw_combo_row_set_model(combo_row_, G_LIST_MODEL(model));
    g_object_unref(model);
    adw_combo_row_set_selected(combo_row_, current_index());
    suppress_ = false;
}

void ShortcutChooser::on_selected(GObject* /*row*/, GParamSpec* /*spec*/, gpointer self_ptr) {
    auto* self = static_cast<ShortcutChooser*>(self_ptr);
    // Never touch the combo's model/selection from inside its own notify::selected
    // emission — set_model/set_selected re-enter AdwComboRow and spin forever.
    // Handle the choice on idle, out of the signal. suppress_ covers rebuild()'s
    // own (suppressed) notifies; pending_ coalesces rapid changes into one dispatch.
    if (self->suppress_ || self->pending_ != 0) {
        return;
    }
    self->pending_ = g_idle_add(&ShortcutChooser::dispatch_selection, self);
}

gboolean ShortcutChooser::dispatch_selection(gpointer self_ptr) {
    auto* self = static_cast<ShortcutChooser*>(self_ptr);
    self->pending_ = 0;
    const unsigned int index = adw_combo_row_get_selected(self->combo_row_);
    const std::vector<QuickPick> picks = quick_picks();
    const auto preset_count = static_cast<unsigned int>(picks.size());
    if (index < preset_count) {
        self->apply(picks.at(index).accelerator);
    } else if (index == (self->is_custom() ? preset_count + 1 : preset_count)) {
        // "Custom…": restore the selection so cancelling keeps the current shortcut,
        // then open the capture sheet.
        self->suppress_ = true;
        adw_combo_row_set_selected(self->combo_row_, self->current_index());
        self->suppress_ = false;
        self->open_capture();
    }
    // index == preset_count while custom is active is the current custom entry — no-op.
    return G_SOURCE_REMOVE;
}

void ShortcutChooser::on_capture_closed(AdwDialog* /*dialog*/, gpointer self) {
    auto* chooser = static_cast<ShortcutChooser*>(self);
    chooser->capture_dialog_ = nullptr;
    chooser->key_controller_ = nullptr;
    chooser->capture_status_ = nullptr;
    chooser->captured_.clear();
}

gboolean ShortcutChooser::on_key_pressed(GtkEventControllerKey* /*controller*/, guint keyval,
                                         guint /*keycode*/, GdkModifierType state, gpointer self) {
    auto* chooser = static_cast<ShortcutChooser*>(self);
    if (keyval == GDK_KEY_Escape) {
        return TRUE; // the dialog closes itself on Escape (cancel)
    }
    const int held = state & gtk_accelerator_get_default_mod_mask();
    if (is_modifier_keyval(keyval)) {
        // Preview the modifiers held so far — the pressed one isn't in state yet.
        chooser->captured_.clear();
        chooser->show_keys(0, static_cast<GdkModifierType>(held | modifier_mask(keyval)));
        return TRUE;
    }
    const auto mods = static_cast<GdkModifierType>(held);
    if (held != 0 && gtk_accelerator_valid(keyval, mods) != FALSE) {
        // A complete, valid combo: preview and remember it, but commit on release.
        char* name = gtk_accelerator_name(keyval, mods);
        chooser->captured_ = name != nullptr ? name : std::string{};
        g_free(name);
        chooser->show_keys(keyval, mods);
    }
    return TRUE;
}

void ShortcutChooser::on_key_released(GtkEventControllerKey* /*controller*/, guint keyval,
                                      guint /*keycode*/, GdkModifierType state, gpointer self) {
    auto* chooser = static_cast<ShortcutChooser*>(self);
    if (!chooser->captured_.empty()) {
        // Accept the held combo as soon as the user starts releasing keys.
        const std::string accelerator = chooser->captured_;
        chooser->captured_.clear();
        chooser->apply(accelerator);
        if (chooser->capture_dialog_ != nullptr) {
            adw_dialog_close(chooser->capture_dialog_);
        }
        return;
    }
    // No combo captured yet (or one was just committed): preview any modifiers
    // still held, minus the released key's bit.
    const int remaining = (state & gtk_accelerator_get_default_mod_mask()) & ~modifier_mask(keyval);
    chooser->show_keys(0, static_cast<GdkModifierType>(remaining));
}

void ShortcutChooser::show_keys(guint keyval, GdkModifierType mods) {
    if (capture_status_ == nullptr) {
        return;
    }
    char* label = gtk_accelerator_get_label(keyval, mods);
    const bool have = label != nullptr && *label != '\0';
    adw_status_page_set_description(capture_status_, have ? label : kCapturePrompt);
    g_free(label);
}

void ShortcutChooser::open_capture() {
    capture_dialog_ = adw_dialog_new();
    adw_dialog_set_title(capture_dialog_, "Set Shortcut");
    adw_dialog_set_content_width(capture_dialog_, kDialogContentWidth);
    adw_dialog_set_presentation_mode(capture_dialog_, ADW_DIALOG_BOTTOM_SHEET);

    GtkWidget* status = adw_status_page_new();
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(status),
                                  "preferences-desktop-keyboard-shortcuts-symbolic");
    adw_status_page_set_title(ADW_STATUS_PAGE(status), "Press your shortcut");
    adw_status_page_set_description(ADW_STATUS_PAGE(status), kCapturePrompt);
    capture_status_ = ADW_STATUS_PAGE(status); // live-updated as keys are pressed
    captured_.clear();

    GtkWidget* toolbar = adw_toolbar_view_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), adw_header_bar_new());
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), status);
    adw_dialog_set_child(capture_dialog_, toolbar);

    // Capture phase so the keypress is read before any child widget consumes it.
    // Kept as a member so the destructor can disconnect it (see ~ShortcutChooser).
    key_controller_ = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(key_controller_, GTK_PHASE_CAPTURE);
    g_signal_connect(key_controller_, "key-pressed", G_CALLBACK(&ShortcutChooser::on_key_pressed),
                     this);
    g_signal_connect(key_controller_, "key-released", G_CALLBACK(&ShortcutChooser::on_key_released),
                     this);
    gtk_widget_add_controller(GTK_WIDGET(capture_dialog_), key_controller_);

    g_signal_connect(capture_dialog_, "closed", G_CALLBACK(&ShortcutChooser::on_capture_closed),
                     this);
    adw_dialog_present(capture_dialog_, parent_);
}

void ShortcutChooser::apply(const std::string& accelerator) {
    if (accelerator == accelerator_) {
        return; // no change — skip the rebuild so a reselection can never cycle
    }
    accelerator_ = accelerator;
    rebuild();
    on_changed_(accelerator_);
}

} // namespace copyclip::ui
