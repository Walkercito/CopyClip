#include "core/HistoryService.hpp"

#include "core/Hash.hpp"
#include "core/Models.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace copyclip::core {

namespace {

// The code points Python's str.isspace() treats as whitespace (so str.strip()
// removes them): ASCII controls and space, the separators 0x1C-0x1F, NEL (0x85),
// and the Unicode space separators. Matching the full set, not just ASCII, means
// non-ASCII whitespace-only content — e.g. NBSP (U+00A0) or ideographic space
// (U+3000) — is rejected too, as the reference does.
[[nodiscard]] constexpr bool is_whitespace_code_point(char32_t code_point) {
    return (code_point >= 0x09 && code_point <= 0x0D) ||
           (code_point >= 0x1C && code_point <= 0x1F) || code_point == 0x20 || code_point == 0x85 ||
           code_point == 0xA0 || code_point == 0x1680 ||
           (code_point >= 0x2000 && code_point <= 0x200A) || code_point == 0x2028 ||
           code_point == 0x2029 || code_point == 0x202F || code_point == 0x205F ||
           code_point == 0x3000;
}

// Decode the UTF-8 code point beginning at text[pos], advancing pos past it.
// Returns std::nullopt on malformed UTF-8 (an invalid lead/continuation byte or a
// truncated sequence), which the caller treats as real content rather than blank.
[[nodiscard]] std::optional<char32_t> next_code_point(std::string_view text, std::size_t& pos) {
    const auto byte_at = [&text](std::size_t index) {
        return static_cast<unsigned char>(text[index]);
    };
    const unsigned char lead = byte_at(pos);
    std::size_t length = 0;
    char32_t code_point = 0;
    if (lead < 0x80) {
        length = 1;
        code_point = lead;
    } else if ((lead & 0xE0) == 0xC0) {
        length = 2;
        code_point = static_cast<char32_t>(lead & 0x1F);
    } else if ((lead & 0xF0) == 0xE0) {
        length = 3;
        code_point = static_cast<char32_t>(lead & 0x0F);
    } else if ((lead & 0xF8) == 0xF0) {
        length = 4;
        code_point = static_cast<char32_t>(lead & 0x07);
    } else {
        return std::nullopt;
    }
    if (pos + length > text.size()) {
        return std::nullopt;
    }
    for (std::size_t offset = 1; offset < length; ++offset) {
        const unsigned char continuation = byte_at(pos + offset);
        if ((continuation & 0xC0) != 0x80) {
            return std::nullopt;
        }
        code_point = static_cast<char32_t>((code_point << 6U) | (continuation & 0x3FU));
    }
    pos += length;
    return code_point;
}

// True when `text` is empty or only whitespace — add()'s reject condition,
// mirroring the reference's `not content or not content.strip()`. Malformed UTF-8
// counts as real content, erring toward keeping data.
[[nodiscard]] bool is_blank(std::string_view text) {
    std::size_t pos = 0;
    while (pos < text.size()) {
        const std::optional<char32_t> code_point = next_code_point(text, pos);
        if (!code_point.has_value() || !is_whitespace_code_point(*code_point)) {
            return false;
        }
    }
    return true;
}

} // namespace

HistoryService::HistoryService(HistoryRepository& repository, Clock& clock, int max_items)
    : repository_{repository}, clock_{clock}, max_items_{max_items} {}

bool HistoryService::add(const std::string& content) {
    return add(ClipContent{.kind = ClipKind::Text, .text = content});
}

bool HistoryService::add(const ClipContent& content) {
    ClipboardEntry entry;
    entry.kind = content.kind;
    if (content.kind == ClipKind::Image) {
        if (content.image.empty()) {
            return false;
        }
        entry.content = content_hash(content.image); // dedup key = image fingerprint
        entry.image = content.image;
        entry.image_width = content.image_width;
        entry.image_height = content.image_height;
    } else {
        if (is_blank(content.text)) {
            return false;
        }
        entry.content = content.text;
        entry.html = content.html; // empty for Text, markup for RichText
    }

    {
        const std::scoped_lock lock{mutex_};
        // Preserve the pin across a re-copy: re-adding a pinned clip (e.g. clicking
        // it, or the clipboard echoing our own write) must not silently unpin it.
        const std::optional<ClipboardEntry> existing = find(entry.content);
        entry.pinned = existing.has_value() && existing->pinned;
        entry.created_at = clock_.now();
        repository_.remove(entry.content); // de-dup: drop any prior copy
        repository_.add(entry);
        enforce_cap();
    }
    notify();
    return true;
}

std::vector<std::byte> HistoryService::image(const std::string& content) const {
    const std::scoped_lock lock{mutex_};
    return repository_.image(content);
}

bool HistoryService::toggle_pin(const std::string& content) {
    bool new_state = false;
    {
        const std::scoped_lock lock{mutex_};
        const std::optional<ClipboardEntry> current = find(content);
        if (!current.has_value()) {
            return false;
        }
        new_state = !current->pinned;
        repository_.set_pinned(content, new_state);
    }
    notify();
    return new_state;
}

void HistoryService::remove(const std::string& content) {
    {
        const std::scoped_lock lock{mutex_};
        repository_.remove(content);
    }
    notify();
}

void HistoryService::clear_unpinned() {
    {
        const std::scoped_lock lock{mutex_};
        repository_.clear_unpinned();
    }
    notify();
}

std::vector<ClipboardEntry> HistoryService::entries() const {
    const std::scoped_lock lock{mutex_};
    return sorted(repository_.all());
}

HistoryService::Subscription HistoryService::subscribe(std::function<void()> callback) {
    const std::scoped_lock lock{mutex_};
    const std::size_t id = next_subscriber_id_++;
    subscribers_.emplace(id, std::move(callback));
    return Subscription{*this, id};
}

void HistoryService::unsubscribe(std::size_t id) {
    const std::scoped_lock lock{mutex_};
    subscribers_.erase(id);
}

HistoryService::Subscription::Subscription(HistoryService& service, std::size_t id)
    : service_{&service}, id_{id} {}

HistoryService::Subscription::~Subscription() {
    if (service_ != nullptr) {
        service_->unsubscribe(id_);
    }
}

HistoryService::Subscription::Subscription(Subscription&& other) noexcept
    : service_{other.service_}, id_{other.id_} {
    other.service_ = nullptr;
}

HistoryService::Subscription&
HistoryService::Subscription::operator=(Subscription&& other) noexcept {
    if (this != &other) {
        if (service_ != nullptr) {
            service_->unsubscribe(id_);
        }
        service_ = other.service_;
        id_ = other.id_;
        other.service_ = nullptr;
    }
    return *this;
}

void HistoryService::enforce_cap() {
    const std::vector<ClipboardEntry> ordered = sorted(repository_.all());
    std::vector<ClipboardEntry> unpinned;
    unpinned.reserve(ordered.size());
    for (const ClipboardEntry& entry : ordered) {
        if (!entry.pinned) {
            unpinned.push_back(entry);
        }
    }
    // In sorted order the unpinned run is newest-first, so reverse iteration
    // visits the oldest unpinned first. Pinned entries are never removed.
    auto overflow =
        static_cast<std::ptrdiff_t>(ordered.size()) - static_cast<std::ptrdiff_t>(max_items_);
    for (auto it = unpinned.rbegin(); it != unpinned.rend() && overflow > 0; ++it) {
        repository_.remove(it->content);
        --overflow;
    }
}

std::optional<ClipboardEntry> HistoryService::find(const std::string& content) const {
    const std::vector<ClipboardEntry> all = repository_.all();
    const auto it = std::ranges::find_if(
        all, [&content](const ClipboardEntry& entry) { return entry.content == content; });
    if (it == all.end()) {
        return std::nullopt;
    }
    return *it;
}

std::vector<ClipboardEntry> HistoryService::sorted(std::vector<ClipboardEntry> items) {
    std::ranges::stable_sort(items, [](const ClipboardEntry& lhs, const ClipboardEntry& rhs) {
        if (lhs.pinned != rhs.pinned) {
            return lhs.pinned; // pinned (true) sorts before unpinned (false)
        }
        return lhs.created_at > rhs.created_at; // newer first
    });
    return items;
}

void HistoryService::notify() {
    std::vector<std::function<void()>> snapshot;
    {
        const std::scoped_lock lock{mutex_};
        snapshot.reserve(subscribers_.size());
        for (const auto& entry : subscribers_) {
            snapshot.push_back(entry.second);
        }
    }
    for (const std::function<void()>& callback : snapshot) {
        callback();
    }
}

} // namespace copyclip::core
