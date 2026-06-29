#pragma once

// Abstract seams for the engine's collaborators. Mirrors
// copyclip/core/interfaces.py (@runtime_checkable Protocols) as pure abstract
// base classes (I.25): production adapters and in-memory test fakes both derive
// from these, and core/ depends on the abstractions, not the concretions.
//
// Each is a proper polymorphic base (C.35 + C.67): public virtual defaulted
// destructor for safe deletion through a base pointer, plus deleted copy/move so
// an interface can never be sliced or value-copied.

#include "core/Models.hpp"

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace copyclip::core {

// A source of "now", injected so tests can supply a deterministic clock.
class Clock {
public:
    Clock() = default;
    virtual ~Clock() = default;

    Clock(const Clock&) = delete;
    Clock& operator=(const Clock&) = delete;
    Clock(Clock&&) = delete;
    Clock& operator=(Clock&&) = delete;

    [[nodiscard]] virtual std::chrono::system_clock::time_point now() const = 0;
};

// The system clipboard: a watched, readable, writable channel carrying text, rich
// text, or images. start() delivers external changes to on_change as ClipContent;
// write() puts a ClipContent back. read() exposes the last text for tests/seeding.
class ClipboardSource {
public:
    ClipboardSource() = default;
    virtual ~ClipboardSource() = default;

    ClipboardSource(const ClipboardSource&) = delete;
    ClipboardSource& operator=(const ClipboardSource&) = delete;
    ClipboardSource(ClipboardSource&&) = delete;
    ClipboardSource& operator=(ClipboardSource&&) = delete;

    virtual void start(std::function<void(const ClipContent&)> on_change) = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual std::optional<std::string> read() const = 0;
    // Put `content` on the clipboard; returns whether it was set successfully.
    virtual bool write(const ClipContent& content) = 0;
};

// A global hotkey listener. rebind() returns whether the new grab succeeded.
class HotkeyListener {
public:
    HotkeyListener() = default;
    virtual ~HotkeyListener() = default;

    HotkeyListener(const HotkeyListener&) = delete;
    HotkeyListener& operator=(const HotkeyListener&) = delete;
    HotkeyListener(HotkeyListener&&) = delete;
    HotkeyListener& operator=(HotkeyListener&&) = delete;

    virtual void start(std::function<void()> on_activate) = 0;
    virtual void stop() = 0;
    virtual bool rebind(const HotkeySpec& spec) = 0;
};

// History storage, keyed by content. set_pinned returns whether the entry
// existed; clear_unpinned drops everything not pinned.
class HistoryRepository {
public:
    HistoryRepository() = default;
    virtual ~HistoryRepository() = default;

    HistoryRepository(const HistoryRepository&) = delete;
    HistoryRepository& operator=(const HistoryRepository&) = delete;
    HistoryRepository(HistoryRepository&&) = delete;
    HistoryRepository& operator=(HistoryRepository&&) = delete;

    virtual void add(const ClipboardEntry& entry) = 0;
    virtual void remove(const std::string& content) = 0;
    virtual bool set_pinned(const std::string& content, bool pinned) = 0;
    virtual void clear_unpinned() = 0;
    [[nodiscard]] virtual std::vector<ClipboardEntry> all() const = 0;

    // PNG bytes for an image entry, keyed by its content hash; empty when absent.
    // Read lazily so all() never carries blobs.
    [[nodiscard]] virtual std::vector<std::byte> image(const std::string& hash) const = 0;
};

class SettingsRepository {
public:
    SettingsRepository() = default;
    virtual ~SettingsRepository() = default;

    SettingsRepository(const SettingsRepository&) = delete;
    SettingsRepository& operator=(const SettingsRepository&) = delete;
    SettingsRepository(SettingsRepository&&) = delete;
    SettingsRepository& operator=(SettingsRepository&&) = delete;

    [[nodiscard]] virtual Settings load() const = 0;
    virtual void save(const Settings& settings) = 0;
};

} // namespace copyclip::core
