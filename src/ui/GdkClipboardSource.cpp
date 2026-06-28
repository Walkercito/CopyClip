#include "ui/GdkClipboardSource.hpp"

#include <gdkmm/display.h>

#include <giomm/asyncresult.h>
#include <glibmm/error.h>
#include <glibmm/ustring.h>

#include <stdexcept>
#include <utility>

namespace copyclip::ui {

namespace {

[[nodiscard]] Glib::RefPtr<Gdk::Clipboard> default_clipboard() {
    const Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
    if (!display) {
        throw std::runtime_error{"no GDK display for the clipboard"};
    }
    return display->get_clipboard();
}

} // namespace

GdkClipboardSource::GdkClipboardSource() : clipboard_{default_clipboard()} {}

GdkClipboardSource::~GdkClipboardSource() {
    changed_connection_.disconnect();
}

void GdkClipboardSource::start(std::function<void(const std::string&)> on_change) {
    on_change_ = std::move(on_change);
    changed_connection_ =
        clipboard_->signal_changed().connect(sigc::mem_fun(*this, &GdkClipboardSource::on_changed));
}

void GdkClipboardSource::stop() {
    changed_connection_.disconnect();
    on_change_ = nullptr;
}

std::optional<std::string> GdkClipboardSource::read() const {
    return last_text_;
}

void GdkClipboardSource::write(const std::string& text) {
    clipboard_->set_text(text);
}

void GdkClipboardSource::on_changed() {
    clipboard_->read_text_async([this](Glib::RefPtr<Gio::AsyncResult>& result) {
        try {
            const Glib::ustring text = clipboard_->read_text_finish(result);
            if (!text.empty()) {
                last_text_ = text.raw();
                if (on_change_) {
                    on_change_(text.raw());
                }
            }
            // NOLINTNEXTLINE(bugprone-empty-catch): non-text content (an image, etc.) is expected
        } catch (const Glib::Error&) {
        }
    });
}

} // namespace copyclip::ui
