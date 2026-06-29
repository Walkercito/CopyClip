#pragma once

// core::ClipboardSource backed by the GTK display clipboard. Watches for changes
// (asynchronous text reads) and writes text, all on the GLib main loop — the
// GTK counterpart to the Qt clipboard source, with no Qt.

#include "core/Interfaces.hpp"

#include <gdkmm/clipboard.h>

#include <giomm/cancellable.h>
#include <glibmm/refptr.h>
#include <sigc++/connection.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace copyclip::ui {

class GdkClipboardSource final : public core::ClipboardSource {
public:
    // `state_file` remembers the last-seen clipboard text across launches so
    // content already present at startup (which Wayland re-offers on focus) isn't
    // captured again every time.
    explicit GdkClipboardSource(std::filesystem::path state_file);
    ~GdkClipboardSource() override;

    GdkClipboardSource(const GdkClipboardSource&) = delete;
    GdkClipboardSource& operator=(const GdkClipboardSource&) = delete;
    GdkClipboardSource(GdkClipboardSource&&) = delete;
    GdkClipboardSource& operator=(GdkClipboardSource&&) = delete;

    void start(std::function<void(const std::string&)> on_change) override;
    void stop() override;
    [[nodiscard]] std::optional<std::string> read() const override;
    void write(const std::string& text) override;

private:
    void on_changed();

    Glib::RefPtr<Gdk::Clipboard> clipboard_;
    std::filesystem::path state_file_;
    std::function<void(const std::string&)> on_change_;
    std::optional<std::string> last_text_;
    sigc::connection changed_connection_;
    // Cancelled on stop/destruction; the async read callback captures a copy and
    // bails before touching a possibly-destroyed `this`.
    Glib::RefPtr<Gio::Cancellable> cancellable_;
};

} // namespace copyclip::ui
