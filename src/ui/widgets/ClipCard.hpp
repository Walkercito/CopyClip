#pragma once

// One clipboard entry as a card in the history list. A plain click copies it;
// Ctrl+click toggles its pin; an inline button expands/collapses long content.
// The callbacks are how the card talks back to the window — it never touches the
// engine directly.

#include "core/Models.hpp"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/label.h>
#include <gtkmm/listboxrow.h>

#include <glibmm/refptr.h>

#include <cstddef>
#include <functional>
#include <string>

namespace copyclip::ui {

class ClipCard : public Gtk::ListBoxRow {
public:
    using ActionCallback = std::function<void(const std::string&)>;

    ClipCard(const core::ClipboardEntry& entry, std::size_t max_chars, ActionCallback on_copy,
             ActionCallback on_pin);

    // The full clip text, used by the window to filter on search.
    [[nodiscard]] const std::string& content() const { return content_; }

private:
    void render_content();
    void toggle_expand();
    void on_pressed(int n_press, double x, double y);

    std::string content_;
    std::size_t max_chars_;
    bool expanded_ = false;
    Gtk::Label* content_label_ = nullptr;
    Gtk::Button* toggle_button_ = nullptr;
    Glib::RefPtr<Gtk::GestureClick> gesture_;
    ActionCallback on_copy_;
    ActionCallback on_pin_;
};

} // namespace copyclip::ui
