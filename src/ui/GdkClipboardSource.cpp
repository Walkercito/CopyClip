#include "ui/GdkClipboardSource.hpp"

#include <gdkmm/display.h>

#include <giomm/asyncresult.h>
#include <glibmm/error.h>
#include <glibmm/ustring.h>

#include <spdlog/spdlog.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
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

[[nodiscard]] std::string read_state(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream) {
        return {};
    }
    return std::string{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
}

void write_state(const std::filesystem::path& path, const std::string& content) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream stream{path, std::ios::binary | std::ios::trunc};
    stream << content;
    if (error || !stream) {
        // Best-effort: failing to persist just means the current clipboard may be
        // re-captured as a duplicate next launch. Worth a breadcrumb, not a throw.
        spdlog::debug("could not persist clipboard state to {}: {}", path.string(),
                      error ? error.message() : "write failed");
    }
}

} // namespace

GdkClipboardSource::GdkClipboardSource(std::filesystem::path state_file)
    : clipboard_{default_clipboard()}, state_file_{std::move(state_file)} {}

GdkClipboardSource::~GdkClipboardSource() {
    changed_connection_.disconnect();
}

void GdkClipboardSource::start(std::function<void(const std::string&)> on_change) {
    on_change_ = std::move(on_change);
    // Seed from the remembered clipboard so the content already present at launch
    // isn't re-captured.
    const std::string remembered = read_state(state_file_);
    if (!remembered.empty()) {
        last_text_ = remembered;
    }
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
        Glib::ustring text;
        try {
            text = clipboard_->read_text_finish(result);
        } catch (const Glib::Error& error) {
            // Non-text content (an image, etc.) or a transient read error — skip.
            spdlog::trace("clipboard read skipped: {}", error.what());
            return;
        }
        // React only to a genuine content change. GdkClipboard::changed also fires
        // on ownership changes (focus/window events) without the text changing;
        // without this guard, clearing the history and then moving the window would
        // re-capture the unchanged clipboard.
        if (text.empty() || text.raw() == last_text_) {
            return;
        }
        last_text_ = text.raw();
        write_state(state_file_, text.raw());
        try {
            if (on_change_) {
                on_change_(text.raw());
            }
        } catch (const std::exception& error) {
            // The callback records to the history DB; a storage failure must not
            // escape across the GLib C callback boundary.
            spdlog::error("failed to record clipboard entry: {}", error.what());
        }
    });
}

} // namespace copyclip::ui
