#include "ui/widgets/ClipCard.hpp"

#include "ui/ClipText.hpp"

#include <gtkmm/image.h>

#include <gdkmm/enums.h>

#include <glib.h>

#include <chrono>
#include <ctime>
#include <utility>

namespace copyclip::ui {

namespace {

constexpr int kCardSpacing = 4;
constexpr int kCardMargin = 6;

// Gap below each card so they read as separate elevated cards (clearer than a
// grouped boxed-list, especially in light mode).
constexpr int kCardGap = 6;

// Lines of content shown on a collapsed card before it ellipsizes.
constexpr int kCollapsedLines = 2;

// Local-time "YYYY-MM-DD HH:MM" for the card's timestamp, via GLib's date API.
[[nodiscard]] std::string format_timestamp(std::chrono::system_clock::time_point when) {
    const auto unix_seconds = static_cast<gint64>(std::chrono::system_clock::to_time_t(when));
    GDateTime* moment = g_date_time_new_from_unix_local(unix_seconds);
    if (moment == nullptr) {
        return {};
    }
    char* formatted = g_date_time_format(moment, "%Y-%m-%d %H:%M");
    std::string result = formatted != nullptr ? formatted : std::string{};
    g_free(formatted);
    g_date_time_unref(moment);
    return result;
}

} // namespace

ClipCard::ClipCard(const core::ClipboardEntry& entry, std::size_t max_chars, ActionCallback on_copy,
                   ActionCallback on_pin)
    : content_{entry.content}, max_chars_{max_chars}, on_copy_{std::move(on_copy)},
      on_pin_{std::move(on_pin)} {
    add_css_class("card");
    set_margin_bottom(kCardGap);

    auto* layout = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, kCardSpacing);
    layout->set_margin(kCardMargin);

    auto* top_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, kCardSpacing);
    if (entry.pinned) {
        auto* pin = Gtk::make_managed<Gtk::Image>();
        pin->set_from_icon_name("view-pin-symbolic");
        pin->add_css_class("accent");
        top_row->append(*pin);
    }
    auto* spacer = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 0);
    spacer->set_hexpand(true);
    top_row->append(*spacer);

    auto* time_label = Gtk::make_managed<Gtk::Label>(format_timestamp(entry.created_at));
    time_label->add_css_class("dim-label");
    time_label->add_css_class("caption");
    top_row->append(*time_label);
    layout->append(*top_row);

    content_label_ = Gtk::make_managed<Gtk::Label>();
    content_label_->set_halign(Gtk::Align::START);
    content_label_->set_xalign(0.0F);
    content_label_->set_wrap(true);
    content_label_->set_wrap_mode(Pango::WrapMode::WORD_CHAR);
    layout->append(*content_label_);

    toggle_button_ = Gtk::make_managed<Gtk::Button>();
    toggle_button_->add_css_class("flat");
    toggle_button_->set_halign(Gtk::Align::END);
    toggle_button_->signal_clicked().connect(sigc::mem_fun(*this, &ClipCard::toggle_expand));
    layout->append(*toggle_button_);

    set_child(*layout);
    render_content();

    gesture_ = Gtk::GestureClick::create();
    gesture_->set_button(GDK_BUTTON_PRIMARY);
    gesture_->signal_pressed().connect(sigc::mem_fun(*this, &ClipCard::on_pressed));
    add_controller(gesture_);
}

void ClipCard::render_content() {
    const Preview preview = make_preview(content_, max_chars_);

    content_label_->set_text(expanded_ ? content_ : preview.text);
    content_label_->set_ellipsize(expanded_ ? Pango::EllipsizeMode::NONE
                                            : Pango::EllipsizeMode::END);
    content_label_->set_lines(expanded_ ? -1 : kCollapsedLines);

    toggle_button_->set_visible(preview.truncated);
    if (preview.truncated) {
        toggle_button_->set_label(expanded_ ? "Show less" : "Show all");
    }
}

void ClipCard::toggle_expand() {
    expanded_ = !expanded_;
    render_content();
}

void ClipCard::on_pressed(int /*n_press*/, double x, double y) {
    // Presses on the expand/collapse button are handled by the button itself; the
    // card must not also copy/pin (which would rebuild the list and undo the
    // toggle).
    if (Gtk::Widget* target = pick(x, y, Gtk::PickFlags::DEFAULT);
        target != nullptr && (target == toggle_button_ || target->is_ancestor(*toggle_button_))) {
        return;
    }
    const Gdk::ModifierType state = gesture_->get_current_event_state();
    const bool ctrl = (state & Gdk::ModifierType::CONTROL_MASK) == Gdk::ModifierType::CONTROL_MASK;
    if (ctrl) {
        on_pin_(content_);
    } else {
        on_copy_(content_);
    }
}

} // namespace copyclip::ui
