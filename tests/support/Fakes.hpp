#pragma once

// In-memory implementations of the core seams, shared across the engine tests.
//
// Direct port of the reference module tests/fakes.py. Each fake is a concrete
// implementation of one abstract interface from core/Interfaces.hpp, holding its
// state in std members so the test layer can drive and observe it without a
// display server, real clipboard, or filesystem. They back the Phase 3
// core-service tests (history, settings, engine), so they are deliberately small
// and reusable.
//
// They are declared `struct`: test fakes are data bags with no invariant to
// protect — the observation points the tests read (started, text, spec, saved)
// vary independently (C.2), so they are intentionally public, matching the
// reference fakes. Collaboration plumbing the tests must not touch (the stored
// callbacks, the history map) stays under an explicit `private:` section.
//
// The fakes follow the Rule of Zero: they declare no copy/move/destructor of
// their own. Their abstract bases delete copy and move, so the fakes inherit
// non-copyable / non-movable semantics — tests construct them as local objects
// and bind them to base references, never copy them.

#include "core/Enums.hpp"
#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace copyclip::testing {

// A clock pinned to a fixed moment, advanceable by the test. now() returns the
// stored moment; advance() moves it forward by a type-safe duration.
struct FakeClock final : public core::Clock {
    explicit FakeClock(std::chrono::system_clock::time_point moment) : moment_{moment} {}

    [[nodiscard]] std::chrono::system_clock::time_point now() const override { return moment_; }

    void advance(std::chrono::seconds delta) { moment_ += delta; }

private:
    std::chrono::system_clock::time_point moment_;
};

// A clipboard backed by an in-memory string. write() sets the contents; emit()
// also fires the registered on_change callback, simulating an external change.
// A pure data bag (all state public): tests both drive it and inspect its
// recorded state, so it has no invariant to protect (C.2).
struct FakeClipboardSource final : public core::ClipboardSource {
    void start(std::function<void(const std::string&)> callback) override {
        on_change = std::move(callback);
        started = true;
    }

    void stop() override { started = false; }

    [[nodiscard]] std::optional<std::string> read() const override { return text; }

    void write(const std::string& new_text) override { text = new_text; }

    // Simulate an external clipboard change: update the contents and notify the
    // registered listener, if any.
    void emit(const std::string& new_text) {
        text = new_text;
        if (on_change) {
            on_change(new_text);
        }
    }

    std::optional<std::string> text;
    std::function<void(const std::string&)> on_change;
    bool started = false;
};

// A hotkey listener that records its binding. rebind() stores the spec and
// always succeeds; trigger() fires the registered on_activate callback. A pure
// data bag (all state public) for the same reason as FakeClipboardSource.
struct FakeHotkeyListener final : public core::HotkeyListener {
    void start(std::function<void()> callback) override {
        on_activate = std::move(callback);
        started = true;
    }

    void stop() override { started = false; }

    bool rebind(const core::HotkeySpec& new_spec) override {
        spec = new_spec;
        return true;
    }

    // Simulate the hotkey firing: invoke the registered listener, if any.
    void trigger() {
        if (on_activate) {
            on_activate();
        }
    }

    std::optional<core::HotkeySpec> spec;
    std::function<void()> on_activate;
    bool started = false;
};

// History stored in a hash map keyed by content, mirroring the reference dict.
// add() inserts or overwrites by content; set_pinned() reports whether the entry
// existed; clear_unpinned() keeps only pinned entries.
class InMemoryHistoryRepository final : public core::HistoryRepository {
public:
    void add(const core::ClipboardEntry& entry) override { entries_[entry.content] = entry; }

    void remove(const std::string& content) override { entries_.erase(content); }

    bool set_pinned(const std::string& content, bool pinned) override {
        const auto it = entries_.find(content);
        if (it == entries_.end()) {
            return false;
        }
        it->second.pinned = pinned;
        return true;
    }

    void clear_unpinned() override {
        std::erase_if(entries_, [](const auto& pair) { return !pair.second.pinned; });
    }

    [[nodiscard]] std::vector<core::ClipboardEntry> all() const override {
        std::vector<core::ClipboardEntry> result;
        result.reserve(entries_.size());
        for (const auto& [content, entry] : entries_) {
            result.push_back(entry);
        }
        return result;
    }

private:
    std::unordered_map<std::string, core::ClipboardEntry> entries_;
};

// Settings held in memory. load() returns the current value; save() replaces it.
// The stored value is exposed as `saved` for assertions (matching the reference).
struct InMemorySettingsRepository final : public core::SettingsRepository {
    InMemorySettingsRepository() = default;
    explicit InMemorySettingsRepository(core::Settings settings) : saved{settings} {}

    [[nodiscard]] core::Settings load() const override { return saved; }

    void save(const core::Settings& settings) override { saved = settings; }

    core::Settings saved;
};

} // namespace copyclip::testing
