#include "core/HistoryService.hpp"

#include "core/Models.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace copyclip::core {

namespace {

// True when `text` is empty or contains only whitespace — the reject condition
// for add(), mirroring the reference's `not content or not content.strip()`. The
// default "C" locale set matches Python's ASCII str.strip() whitespace exactly.
[[nodiscard]] bool is_blank(std::string_view text) {
    return std::ranges::all_of(
        text, [](char ch) { return std::isspace(static_cast<unsigned char>(ch)) != 0; });
}

} // namespace

HistoryService::HistoryService(HistoryRepository& repository, Clock& clock, int max_items)
    : repository_{repository}, clock_{clock}, max_items_{max_items} {}

bool HistoryService::add(const std::string& content) {
    if (is_blank(content)) {
        return false;
    }
    {
        const std::scoped_lock lock{mutex_};
        repository_.remove(content); // de-dup: drop any prior copy
        repository_.add(
            ClipboardEntry{.content = content, .created_at = clock_.now(), .pinned = false});
        enforce_cap();
    }
    notify();
    return true;
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

void HistoryService::subscribe(std::function<void()> callback) {
    const std::scoped_lock lock{mutex_};
    subscribers_.push_back(std::move(callback));
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
        snapshot = subscribers_;
    }
    for (const std::function<void()>& callback : snapshot) {
        callback();
    }
}

} // namespace copyclip::core
