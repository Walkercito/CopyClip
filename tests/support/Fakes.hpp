#pragma once

// In-memory implementations of the core seams, shared across the engine tests.
// Direct port of the reference tests/fakes.py. Declared `struct`: the fakes are
// data bags whose observation points the tests read directly (C.2), so there is
// no invariant to hide; collaboration plumbing the tests must not touch stays
// under `private:`. Rule of Zero — the abstract bases delete copy/move, so the
// fakes are non-copyable/non-movable and tests bind them to base references.

#include "core/Enums.hpp"
#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace copyclip::testing {

// A clock pinned to a fixed moment that the test can advance() forward.
struct FakeClock final : public core::Clock {
    explicit FakeClock(std::chrono::system_clock::time_point moment) : moment_{moment} {}

    [[nodiscard]] std::chrono::system_clock::time_point now() const override { return moment_; }

    void advance(std::chrono::seconds delta) { moment_ += delta; }

private:
    std::chrono::system_clock::time_point moment_;
};

// A clipboard backed by an in-memory string; emit() simulates an external change.
struct FakeClipboardSource final : public core::ClipboardSource {
    void start(std::function<void(const std::string&)> callback) override {
        on_change = std::move(callback);
        started = true;
    }

    void stop() override { started = false; }

    [[nodiscard]] std::optional<std::string> read() const override { return text; }

    void write(const std::string& new_text) override { text = new_text; }

    // Update the contents and notify the registered listener, if any.
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

// A hotkey listener that records its binding; trigger() fires the activation
// callback. rebind() always succeeds.
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

    // Simulate the hotkey firing.
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
            core::ClipboardEntry lazy = entry;
            lazy.image.clear(); // image bytes are fetched via image(), as the real repo does
            result.push_back(std::move(lazy));
        }
        return result;
    }

    [[nodiscard]] std::vector<std::byte> image(const std::string& hash) const override {
        const auto it = entries_.find(hash);
        return it == entries_.end() ? std::vector<std::byte>{} : it->second.image;
    }

private:
    std::unordered_map<std::string, core::ClipboardEntry> entries_;
};

// Settings held in memory; the stored value is exposed as `saved` for assertions.
struct InMemorySettingsRepository final : public core::SettingsRepository {
    InMemorySettingsRepository() = default;
    explicit InMemorySettingsRepository(core::Settings settings) : saved{settings} {}

    [[nodiscard]] core::Settings load() const override { return saved; }

    void save(const core::Settings& settings) override { saved = settings; }

    core::Settings saved;
};

} // namespace copyclip::testing
