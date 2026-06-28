#pragma once

// Application logic for clipboard history: dedup, pinning, ordering, capacity.
// Mirrors reference core/history.py — a new item de-dups any prior copy and moves
// to the front with a fresh timestamp, pinned items sort first and survive
// eviction, and an over-capacity history sheds its oldest UNPINNED entries.
// Persistence is delegated to a HistoryRepository.
//
// Collaborators are injected as reference members (I.11/R.3): the repository and
// clock are observed, never owned, and outlive the service. The std::mutex makes
// the type non-copyable/non-movable (Rule of Zero); reads take it through a
// `mutable` mutex so entries() stays const.
//
// Notification happens OUTSIDE the lock (CP.22): callbacks are unknown user code
// that may re-enter the service (e.g. call entries()), so notify() snapshots the
// subscriber list under the lock and invokes each after releasing it. Pure core
// layer — no Qt, Xlib, or D-Bus.

#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace copyclip::core {

class HistoryService {
public:
    HistoryService(HistoryRepository& repository, Clock& clock, int max_items);

    // Record `content` as the newest item. Blank input (empty or whitespace-only)
    // is ignored and returns false; otherwise any prior copy is dropped, the item
    // is re-added with a fresh timestamp, and capacity is enforced.
    bool add(const std::string& content);

    // Flip the pinned flag of `content` and return the NEW pin state. As in the
    // reference, a false result also means "no such entry" (intentional dual
    // meaning); a missing entry leaves the history untouched.
    bool toggle_pin(const std::string& content);

    void remove(const std::string& content);

    void clear_unpinned();

    // Snapshot of the history, sorted pinned-first then most-recent-first.
    [[nodiscard]] std::vector<ClipboardEntry> entries() const;

    // Register a callback fired after every mutation (invoked outside the lock).
    void subscribe(std::function<void()> callback);

private:
    // Shed the oldest unpinned entries until the history fits max_items. Pinned
    // entries are never candidates. Called with the lock held.
    void enforce_cap();

    // Linear lookup of the entry whose content matches. Called with the lock
    // held; returns a copy so it never dangles past the repository snapshot.
    [[nodiscard]] std::optional<ClipboardEntry> find(const std::string& content) const;

    // Stable sort by (pinned desc, created_at desc): pinned first, newest first,
    // ties keeping input order. Mirrors Python's sorted(key=..., reverse=True).
    [[nodiscard]] static std::vector<ClipboardEntry> sorted(std::vector<ClipboardEntry> items);

    // Snapshot subscribers under the lock, then invoke them after releasing it.
    void notify();

    HistoryRepository& repository_;
    Clock& clock_;
    int max_items_;
    mutable std::mutex mutex_;
    std::vector<std::function<void()>> subscribers_;
};

} // namespace copyclip::core
