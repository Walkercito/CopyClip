#pragma once

// Application logic for clipboard history: dedup, pinning, ordering, capacity.
//
// Mirrors the reference module copyclip/core/history.py. The service owns the
// RULES of the history and delegates persistence to a HistoryRepository: a new
// item de-duplicates any prior copy and moves to the front with a fresh
// timestamp, pinned items sort first and survive eviction, and an over-capacity
// history sheds its oldest UNPINNED entries.
//
// The collaborators are injected as reference members (I.11/R.3): the repository
// and the clock are observed, never owned, and outlive the service. A std::mutex
// member serializes the mutating rules and makes the type intrinsically
// non-copyable/non-movable (Rule of Zero — no special members declared). Reads
// take the same lock through a `mutable` mutex so entries() stays const.
//
// Subscriber notification deliberately happens OUTSIDE the lock (CP.22): the
// callbacks are unknown user code that may re-enter the service (e.g. call
// entries()), so notify() snapshots the subscriber list under the lock and then
// invokes each callback after releasing it. Pure core layer: no Qt, Xlib, or
// D-Bus — only the core seams, the value models, and the standard library.

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

    // Record `content` as the newest history item. Blank input (empty or only
    // whitespace) is ignored and returns false; otherwise any prior copy is
    // dropped, the item is re-added with a fresh timestamp, capacity is
    // enforced, subscribers are notified, and the call returns true.
    bool add(const std::string& content);

    // Flip the pinned flag of `content`. Returns the NEW pin state; like the
    // reference, a false result also means "no such entry" (the dual meaning is
    // intentional). A missing entry leaves the history untouched.
    bool toggle_pin(const std::string& content);

    // Drop `content` from the history if present, then notify subscribers.
    void remove(const std::string& content);

    // Drop every unpinned entry, then notify subscribers.
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
