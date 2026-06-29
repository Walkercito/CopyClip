#include "ui/GdkClipboardSource.hpp"

#include "core/Hash.hpp"

#include <gdkmm/contentformats.h>
#include <gdkmm/contentprovider.h>
#include <gdkmm/display.h>
#include <gdkmm/texture.h>

#include <giomm/asyncresult.h>
#include <giomm/inputstream.h>
#include <glibmm/bytes.h>
#include <glibmm/error.h>
#include <glibmm/main.h>
#include <glibmm/ustring.h>
#include <glibmm/value.h>

#include <spdlog/spdlog.h>

#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace copyclip::ui {

namespace {

constexpr const char* kMimeHtml = "text/html";
constexpr gsize kStreamChunk = 4096;

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

// Read a Gio input stream fully into a string (GTK does not NUL-terminate the
// text/html payload, so the byte count is authoritative).
[[nodiscard]] std::string drain_stream(const Glib::RefPtr<Gio::InputStream>& stream) {
    std::string result;
    while (true) {
        Glib::RefPtr<Glib::Bytes> chunk;
        try {
            chunk = stream->read_bytes(kStreamChunk, {});
        } catch (const Glib::Error& error) {
            // A failure here (vs the EOF break below) may leave the HTML empty or
            // truncated; surface it rather than swallowing it silently.
            spdlog::warn("clipboard HTML stream read failed after {} bytes: {}", result.size(),
                         error.what());
            break;
        }
        if (!chunk) {
            break;
        }
        gsize size = 0;
        const auto* data = static_cast<const char*>(chunk->get_data(size));
        if (size == 0) {
            break;
        }
        result.append(data, size);
    }
    try {
        stream->close();
    } catch (const Glib::Error& error) {
        spdlog::trace("clipboard stream close failed: {}", error.what());
    }
    return result;
}

// Copy a Glib::Bytes buffer into a byte vector without raw pointer arithmetic.
[[nodiscard]] std::vector<std::byte> to_bytes(const Glib::RefPtr<const Glib::Bytes>& bytes) {
    gsize size = 0;
    const auto* data = static_cast<const std::byte*>(bytes->get_data(size));
    const std::span<const std::byte> view{data, size};
    return std::vector<std::byte>{view.begin(), view.end()};
}

} // namespace

GdkClipboardSource::GdkClipboardSource(std::filesystem::path state_file)
    : clipboard_{default_clipboard()}, state_file_{std::move(state_file)} {}

GdkClipboardSource::~GdkClipboardSource() {
    if (cancellable_) {
        cancellable_->cancel();
    }
    changed_connection_.disconnect();
}

void GdkClipboardSource::start(std::function<void(const core::ClipContent&)> on_change) {
    on_change_ = std::move(on_change);
    cancellable_ = Gio::Cancellable::create();
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
    if (cancellable_) {
        cancellable_->cancel();
    }
    changed_connection_.disconnect();
    on_change_ = nullptr;
}

std::optional<std::string> GdkClipboardSource::read() const {
    return last_text_;
}

bool GdkClipboardSource::write(const core::ClipContent& content) {
    switch (content.kind) {
    case core::ClipKind::Image: {
        try {
            const Glib::RefPtr<Glib::Bytes> bytes =
                Glib::Bytes::create(content.image.data(), content.image.size());
            clipboard_->set_texture(Gdk::Texture::create_from_bytes(bytes));
            last_image_hash_ = core::content_hash(content.image);
            last_text_.reset();
            return true;
        } catch (const Glib::Error& error) {
            spdlog::error("failed to write image to clipboard: {}", error.what());
            return false;
        }
    }
    case core::ClipKind::RichText: {
        Glib::Value<Glib::ustring> text_value;
        text_value.init(Glib::Value<Glib::ustring>::value_type());
        text_value.set(content.text);
        const Glib::RefPtr<Glib::Bytes> html_bytes =
            Glib::Bytes::create(content.html.data(), content.html.size());
        // Offer HTML first (richer), then plain text as the fallback format.
        const Glib::RefPtr<Gdk::ContentProvider> provider =
            Gdk::ContentProvider::create(std::vector<Glib::RefPtr<Gdk::ContentProvider>>{
                Gdk::ContentProvider::create(kMimeHtml, html_bytes),
                Gdk::ContentProvider::create(text_value)});
        last_text_ = content.text;
        last_image_hash_.clear();
        return clipboard_->set_content(provider);
    }
    case core::ClipKind::Text:
        clipboard_->set_text(content.text);
        last_text_ = content.text;
        last_image_hash_.clear();
        return true;
    }
    return false; // unreachable: every ClipKind is handled above
}

void GdkClipboardSource::on_changed() {
    // get_formats() is unreliable at this instant on X11: ownership changes before
    // the TARGETS list is parsed, so an image (e.g. a screenshot) may not yet appear
    // in the formats. Rather than sniff, try reading an image first; a failed texture
    // read means it isn't an image, so we fall back to text. This reliably catches
    // screenshots, where sniffing the formats would miss them.
    read_image();
}

void GdkClipboardSource::read_text_or_rich() {
    // Reached after a failed texture read. By now the format list has been negotiated
    // (the texture attempt forced the round-trip), so the HTML check is reliable here.
    const Glib::RefPtr<const Gdk::ContentFormats> formats = clipboard_->get_formats();
    if (formats && formats->contain_mime_type(kMimeHtml)) {
        read_rich_text();
    } else {
        read_plain_text();
    }
}

void GdkClipboardSource::read_plain_text() {
    const Glib::RefPtr<Gio::Cancellable> cancellable = cancellable_;
    clipboard_->read_text_async(
        [this, cancellable](Glib::RefPtr<Gio::AsyncResult>& result) {
            if (cancellable->is_cancelled()) {
                return;
            }
            Glib::ustring text;
            try {
                text = clipboard_->read_text_finish(result);
            } catch (const Glib::Error& error) {
                spdlog::trace("clipboard text read skipped: {}", error.what());
                return;
            }
            // Skip ownership/focus changes that don't change the content.
            if (text.empty() || text.raw() == last_text_) {
                return;
            }
            last_text_ = text.raw();
            last_image_hash_.clear();
            write_state(state_file_, text.raw());
            deliver(core::ClipContent{.kind = core::ClipKind::Text, .text = text.raw()});
        },
        cancellable_);
}

void GdkClipboardSource::read_rich_text() {
    const Glib::RefPtr<Gio::Cancellable> cancellable = cancellable_;
    // Read the plain-text form first (the dedup key + display text), then the HTML.
    clipboard_->read_text_async(
        [this, cancellable](Glib::RefPtr<Gio::AsyncResult>& text_result) {
            if (cancellable->is_cancelled()) {
                return;
            }
            std::string text;
            try {
                text = clipboard_->read_text_finish(text_result).raw();
            } catch (const Glib::Error& error) {
                spdlog::trace("rich-text plain-text read skipped: {}", error.what());
                return;
            }
            if (text.empty() || text == last_text_) {
                return;
            }
            clipboard_->read_async(
                {kMimeHtml}, Glib::PRIORITY_DEFAULT,
                [this, cancellable, text](Glib::RefPtr<Gio::AsyncResult>& html_result) {
                    if (cancellable->is_cancelled()) {
                        return;
                    }
                    std::string html;
                    Glib::ustring chosen_mime;
                    Glib::RefPtr<Gio::InputStream> stream;
                    try {
                        stream = clipboard_->read_finish(html_result, chosen_mime);
                    } catch (const Glib::Error& error) {
                        // The clipboard advertised text/html but the read failed; we
                        // store the clip as plain text, so leave a breadcrumb.
                        spdlog::warn("clipboard text/html read failed; storing as plain text: {}",
                                     error.what());
                        stream.reset();
                    }
                    if (stream) {
                        html = drain_stream(stream);
                    }
                    last_text_ = text;
                    last_image_hash_.clear();
                    write_state(state_file_, text);
                    // Fall back to plain text if the HTML payload was unreadable.
                    const core::ClipKind kind =
                        html.empty() ? core::ClipKind::Text : core::ClipKind::RichText;
                    deliver(core::ClipContent{.kind = kind, .text = text, .html = html});
                },
                cancellable_);
        },
        cancellable_);
}

void GdkClipboardSource::read_image() {
    const Glib::RefPtr<Gio::Cancellable> cancellable = cancellable_;
    clipboard_->read_texture_async(
        [this, cancellable](Glib::RefPtr<Gio::AsyncResult>& result) {
            if (cancellable->is_cancelled()) {
                return;
            }
            Glib::RefPtr<Gdk::Texture> texture;
            try {
                texture = clipboard_->read_texture_finish(result);
            } catch (const Glib::Error&) {
                // No image on the clipboard (it holds text) — fall back to text.
                read_text_or_rich();
                return;
            }
            if (!texture) {
                read_text_or_rich();
                return;
            }
            const Glib::RefPtr<Glib::Bytes> png = texture->save_to_png_bytes();
            if (!png) {
                spdlog::warn("failed to encode clipboard image to PNG; skipping it");
                return;
            }
            std::vector<std::byte> bytes = to_bytes(png);
            if (bytes.empty()) {
                spdlog::warn("clipboard image encoded to zero bytes; skipping it");
                return;
            }
            const std::string hash = core::content_hash(bytes);
            if (hash == last_image_hash_) {
                return;
            }
            last_image_hash_ = hash;
            last_text_.reset();
            deliver(core::ClipContent{.kind = core::ClipKind::Image,
                                      .image = std::move(bytes),
                                      .image_width = texture->get_width(),
                                      .image_height = texture->get_height()});
        },
        cancellable_);
}

void GdkClipboardSource::deliver(const core::ClipContent& content) {
    try {
        if (on_change_) {
            on_change_(content);
        }
    } catch (const std::exception& error) {
        // The callback records to the history DB; a storage failure must not escape
        // across the GLib C callback boundary.
        spdlog::error("failed to record clipboard entry: {}", error.what());
    }
}

} // namespace copyclip::ui
