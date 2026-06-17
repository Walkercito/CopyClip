#pragma once

// Abstract seams for the engine's collaborators.
//
// Mirrors the reference module copyclip/core/interfaces.py, where each seam is a
// @runtime_checkable Protocol: a duck-typed contract that any implementation can
// satisfy. The C++ port expresses the same contracts as pure abstract base
// classes (the explicit-interface idiom, I.25): production adapters and the
// in-memory test fakes both derive from these, and core/ depends on the
// abstractions rather than the concretions.
//
// Each interface is a proper polymorphic base (C.35 + C.67): a public virtual
// defaulted destructor for safe deletion through a base pointer, plus deleted
// copy and move special members so an interface can never be sliced or value-
// copied. They are pure — no data members, no non-virtual logic — so the only
// inward dependency is core/Models.hpp (the value objects the seams exchange)
// plus the standard library. No Qt, Xlib, or D-Bus: this lives in the core
// layer.

#include "core/Models.hpp"

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace copyclip::core {

// A source of the current time. Injected wherever the engine needs "now" so
// tests can supply a deterministic clock instead of the wall clock.
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

// The system clipboard: a watched, readable, writable text channel. start()
// begins delivering external changes to on_change; read()/write() access the
// current contents.
class ClipboardSource {
public:
    ClipboardSource() = default;
    virtual ~ClipboardSource() = default;

    ClipboardSource(const ClipboardSource&) = delete;
    ClipboardSource& operator=(const ClipboardSource&) = delete;
    ClipboardSource(ClipboardSource&&) = delete;
    ClipboardSource& operator=(ClipboardSource&&) = delete;

    virtual void start(std::function<void(const std::string&)> on_change) = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual std::optional<std::string> read() const = 0;
    virtual void write(const std::string& text) = 0;
};

// A global hotkey listener. start() begins invoking on_activate when the bound
// combo fires; rebind() changes the combo, reporting whether the grab succeeded.
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

// Persistent storage for clipboard history, keyed by content. set_pinned reports
// whether the entry existed; clear_unpinned drops everything not pinned.
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
};

// Persistent storage for the user's Settings.
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
