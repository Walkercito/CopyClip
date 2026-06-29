#include "ui/CopyAction.hpp"

#include "core/HistoryService.hpp"
#include "core/Models.hpp"
#include "core/SettingsService.hpp"
#include "support/Fakes.hpp"
#include "ui/Paster.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace {

using copyclip::core::HistoryService;
using copyclip::core::Settings;
using copyclip::core::SettingsService;
using copyclip::testing::FakeClipboardSource;
using copyclip::testing::FakeClock;
using copyclip::testing::InMemoryHistoryRepository;
using copyclip::testing::InMemorySettingsRepository;

// Records paste() calls without spawning any input tool.
struct FakePaster final : public copyclip::ui::Paster {
    void paste() const override { ++pastes; }
    mutable int pastes = 0;
};

// Wires CopyAction to in-memory fakes; tweak behaviour via set().
struct CopyActionHarness {
    FakeClipboardSource clipboard;
    FakeClock clock{std::chrono::system_clock::time_point{}};
    InMemoryHistoryRepository history_repo;
    HistoryService history{history_repo, clock, 100};
    InMemorySettingsRepository settings_repo;
    SettingsService settings{settings_repo};
    FakePaster paster;
    copyclip::ui::CopyAction action{clipboard, history, settings, paster};

    void set(bool auto_hide, bool auto_paste) {
        Settings updated = settings.settings();
        updated.auto_hide_on_copy = auto_hide;
        updated.auto_paste = auto_paste;
        settings.update(updated);
    }
};

TEST(CopyActionTest, WritesClipboardAndRecordsHistory) {
    CopyActionHarness harness;
    EXPECT_TRUE(harness.action.run("hello")); // default auto-hide -> hide
    EXPECT_EQ(harness.clipboard.text, "hello");
    EXPECT_EQ(harness.history.entries().size(), 1U);
}

TEST(CopyActionTest, HidesWhenAutoHideOn) {
    CopyActionHarness harness;
    harness.set(true, false);
    EXPECT_TRUE(harness.action.run("x"));
}

TEST(CopyActionTest, DoesNotHideWhenBothOff) {
    CopyActionHarness harness;
    harness.set(false, false);
    EXPECT_FALSE(harness.action.run("x"));
}

TEST(CopyActionTest, HidesWhenAutoPasteEvenWithAutoHideOff) {
    CopyActionHarness harness;
    harness.set(false, true);
    EXPECT_TRUE(harness.action.run("x"));
}

} // namespace
