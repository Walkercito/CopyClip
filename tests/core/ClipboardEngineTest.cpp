#include "core/ClipboardEngine.hpp"

#include "core/HistoryService.hpp"
#include "core/SettingsService.hpp"
#include "support/Fakes.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace {

using copyclip::core::ClipboardEngine;
using copyclip::core::HistoryService;
using copyclip::core::SettingsService;
using copyclip::testing::FakeClipboardSource;
using copyclip::testing::FakeClock;
using copyclip::testing::FakeHotkeyListener;
using copyclip::testing::InMemoryHistoryRepository;
using copyclip::testing::InMemorySettingsRepository;

// Mirrors tests/core/test_engine.py: the engine wires the clipboard source and
// hotkey listener to the services and exposes a show-request observer.
class ClipboardEngineTest : public ::testing::Test {
public:
    [[nodiscard]] std::vector<std::string> contents() const {
        std::vector<std::string> out;
        for (const auto& entry : history.entries()) {
            out.push_back(entry.content);
        }
        return out;
    }

    FakeClipboardSource clipboard;
    FakeHotkeyListener hotkey;
    InMemoryHistoryRepository history_repo;
    FakeClock clock{std::chrono::system_clock::time_point{}};
    HistoryService history{history_repo, clock, 200};
    InMemorySettingsRepository settings_repo;
    SettingsService settings{settings_repo};
    ClipboardEngine engine{clipboard, hotkey, history, settings};
};

TEST_F(ClipboardEngineTest, StartWiresSources) {
    engine.start();
    EXPECT_TRUE(clipboard.started);
    EXPECT_TRUE(hotkey.started);
}

TEST_F(ClipboardEngineTest, ClipboardChangeIsRecorded) {
    engine.start();
    clipboard.emit("copied text");
    EXPECT_EQ(contents(), (std::vector<std::string>{"copied text"}));
}

TEST_F(ClipboardEngineTest, HotkeyEmitsShowRequest) {
    std::vector<int> shown;
    engine.on_show_requested([&shown] { shown.push_back(1); });
    engine.start();
    hotkey.trigger();
    EXPECT_EQ(shown, (std::vector<int>{1}));
}

TEST_F(ClipboardEngineTest, CopyToClipboardWritesAndRecords) {
    engine.copy_to_clipboard("pick me");
    EXPECT_EQ(clipboard.read(), std::optional<std::string>{"pick me"});
    EXPECT_EQ(contents(), (std::vector<std::string>{"pick me"}));
}

TEST_F(ClipboardEngineTest, StopStopsSources) {
    engine.start();
    engine.stop();
    EXPECT_FALSE(clipboard.started);
    EXPECT_FALSE(hotkey.started);
}

} // namespace
