#pragma once

// One clipboard entry as a card in the history list. A plain click copies it;
// Ctrl+click toggles its pin; an inline button expands/collapses long text. Image
// clips show a thumbnail; rich-text clips show their plain text with a small
// badge. The callbacks are how the card talks back to the window — it never
// touches the engine directly, and receives the whole entry so the window can
// reconstruct the right clipboard payload per kind.

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
#include <vector>

namespace copyclip::ui {

class ClipCard : public Gtk::ListBoxRow {
public:
    using ActionCallback = std::function<void(const core::ClipboardEntry&)>;

    // `image` holds the PNG bytes for an image entry (empty otherwise); the window
    // fetches them lazily so the card doesn't reach into storage itself.
    ClipCard(const core::ClipboardEntry& entry, std::vector<std::byte> image, std::size_t max_chars,
             ActionCallback on_copy, ActionCallback on_pin);

    // The entry's dedup key (its text, or an image hash) — the window filters on it.
    [[nodiscard]] const std::string& content() const { return entry_.content; }

    // The full entry this card was built from, so the window can reconcile the list
    // incrementally — detecting a pin/timestamp change without rebuilding every card.
    [[nodiscard]] const core::ClipboardEntry& entry() const { return entry_; }

private:
    void render_text();
    void toggle_expand();
    void on_pressed(int n_press, double x, double y);

    core::ClipboardEntry entry_;
    std::vector<std::byte> image_;
    std::size_t max_chars_;
    bool expanded_ = false;
    Gtk::Label* content_label_ = nullptr;  // null for image cards
    Gtk::Button* toggle_button_ = nullptr; // null for image cards
    Glib::RefPtr<Gtk::GestureClick> gesture_;
    ActionCallback on_copy_;
    ActionCallback on_pin_;
};

} // namespace copyclip::ui
