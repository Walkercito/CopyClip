// Port of the reference oracle tests/core/test_interfaces.py.
//
// The Python reference uses runtime_checkable Protocols and asserts each fake
// satisfies its seam via isinstance(...). The C++ port expresses the same
// relationship at COMPILE TIME: each fake is checked to publicly derive from the
// abstract interface it implements with static_assert(std::derived_from<...>).
// A small runtime case then binds each fake to a base-class reference and
// exercises one call through the seam, so the suite has a runnable case and the
// polymorphic dispatch is verified end to end.

#include "core/Interfaces.hpp"
#include "core/Enums.hpp"
#include "core/Models.hpp"
#include "support/Fakes.hpp"

#include <chrono>
#include <concepts>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace {

namespace core = copyclip::core;
namespace testing = copyclip::testing;

// test_fakes_satisfy_protocols (compile-time half): each in-memory fake must be
// a concrete implementation of its abstract seam. std::derived_from is the
// idiomatic compile-time replacement for Python's isinstance against a
// runtime_checkable Protocol.
static_assert(std::derived_from<testing::FakeClock, core::Clock>);
static_assert(std::derived_from<testing::FakeClipboardSource, core::ClipboardSource>);
static_assert(std::derived_from<testing::FakeHotkeyListener, core::HotkeyListener>);
static_assert(std::derived_from<testing::InMemoryHistoryRepository, core::HistoryRepository>);
static_assert(std::derived_from<testing::InMemorySettingsRepository, core::SettingsRepository>);

// The seams are pure interfaces: no fake may be sliceable through the base, and
// the bases carry no state. Confirm the interfaces are abstract.
static_assert(std::is_abstract_v<core::Clock>);
static_assert(std::is_abstract_v<core::ClipboardSource>);
static_assert(std::is_abstract_v<core::HotkeyListener>);
static_assert(std::is_abstract_v<core::HistoryRepository>);
static_assert(std::is_abstract_v<core::SettingsRepository>);

// Runtime half: bind each fake to a reference of its abstract base and exercise
// one call through the seam, proving virtual dispatch reaches the fake.
TEST(InterfacesTest, ClockSeamDispatchesToFake) {
    const auto moment = std::chrono::system_clock::time_point{std::chrono::seconds{1000}};
    testing::FakeClock fake{moment};
    const core::Clock& seam = fake;
    EXPECT_EQ(seam.now(), moment);
}

TEST(InterfacesTest, ClipboardSourceSeamDispatchesToFake) {
    testing::FakeClipboardSource fake;
    core::ClipboardSource& seam = fake;
    seam.write("hello");
    EXPECT_EQ(seam.read(), std::optional<std::string>{"hello"});
}

TEST(InterfacesTest, HotkeyListenerSeamDispatchesToFake) {
    testing::FakeHotkeyListener fake;
    core::HotkeyListener& seam = fake;
    const core::HotkeySpec spec{.modifiers = {core::Modifier::Super}, .key = core::Key::V};
    EXPECT_TRUE(seam.rebind(spec));
    // HotkeySpec has no operator==; assert the stored spec is engaged and its
    // observable label matches what was bound. A plain has_value() guard (which
    // the optional-access analysis models, unlike GoogleTest's ASSERT_* macros)
    // makes the dereference provably safe; the else branch fails the test.
    if (fake.spec.has_value()) {
        EXPECT_EQ(fake.spec->display_name(), "Super+V");
    } else {
        ADD_FAILURE() << "rebind did not store a hotkey spec";
    }
}

TEST(InterfacesTest, HistoryRepositorySeamDispatchesToFake) {
    testing::InMemoryHistoryRepository fake;
    core::HistoryRepository& seam = fake;
    const core::ClipboardEntry entry{
        .content = "note", .created_at = std::chrono::system_clock::time_point{}, .pinned = false};
    seam.add(entry);
    const std::vector<core::ClipboardEntry> entries = seam.all();
    ASSERT_EQ(entries.size(), 1U);
    EXPECT_EQ(entries.front().content, "note");
}

TEST(InterfacesTest, SettingsRepositorySeamDispatchesToFake) {
    testing::InMemorySettingsRepository fake;
    core::SettingsRepository& seam = fake;
    core::Settings settings{};
    settings.first_run_completed = true;
    seam.save(settings);
    EXPECT_TRUE(seam.load().first_run_completed);
}

} // namespace
