#pragma once

// core::ClipboardSource backed by the GTK display clipboard. Watches for changes
// and reads/writes plain text, rich text (text/html), and images (encoded as PNG),
// all on the GLib main loop — the GTK counterpart to the Qt clipboard source, with
// no Qt.

#include "core/Interfaces.hpp"
#include "core/Models.hpp"

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

    void start(std::function<void(const core::ClipContent&)> on_change) override;
    void stop() override;
    [[nodiscard]] std::optional<std::string> read() const override;
    void write(const core::ClipContent& content) override;

private:
    // Dispatch by the formats the clipboard offers: image first, then rich text,
    // then plain text. Each reads asynchronously and delivers a ClipContent.
    void on_changed();
    void read_image();
    void read_rich_text();
    void read_plain_text();

    // Hand a captured clip to on_change_, swallowing (and logging) any storage
    // error so it never crosses the GLib C callback boundary.
    void deliver(const core::ClipContent& content);

    Glib::RefPtr<Gdk::Clipboard> clipboard_;
    std::filesystem::path state_file_;
    std::function<void(const core::ClipContent&)> on_change_;
    // The last captured key, so a mere ownership/focus change isn't re-captured.
    // Only one is active at a time (text vs image): capturing one clears the other.
    std::optional<std::string> last_text_;
    std::string last_image_hash_;
    sigc::connection changed_connection_;
    // Cancelled on stop/destruction; the async callbacks capture a copy and bail
    // before touching a possibly-destroyed `this`.
    Glib::RefPtr<Gio::Cancellable> cancellable_;
};

} // namespace copyclip::ui
