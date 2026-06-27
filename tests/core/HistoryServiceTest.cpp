// Port of the reference oracle tests/core/test_history.py.
//
// Exercises HistoryService's rules — blank rejection, dedup/move-to-front,
// pinned-first ordering, capacity eviction (pinned never evicted), clear, and
// subscriber notification — against the in-memory fakes. One extra case beyond
// the Python oracle proves notify() invokes subscribers OUTSIDE the lock: a
// callback that re-enters entries() must neither deadlock nor miss the new
// entry (C++ Core Guidelines CP.22).

#include "core/HistoryService.hpp"
#include "core/Models.hpp"
#include "support/Fakes.hpp"

#include <chrono>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

namespace core = copyclip::core;

using copyclip::testing::FakeClock;
using copyclip::testing::InMemoryHistoryRepository;

// The reference default cap (the Python _service helper uses max_items=200).
constexpr int kDefaultMaxItems = 200;

// One type-safe minute, mirroring the reference's clock.advance(60).
constexpr std::chrono::seconds kStep{60};

// Fixed clock origin for the suite, mirroring the reference's datetime(2026, 1,
// 1). Only relative ordering matters here, so the absolute instant is arbitrary
// but fixed.
[[nodiscard]] std::chrono::system_clock::time_point clock_origin() {
    const std::chrono::year_month_day ymd{std::chrono::year{2026}, std::chrono::January,
                                          std::chrono::day{1}};
    return std::chrono::system_clock::time_point{std::chrono::sys_days{ymd}};
}

// Owns a repository, a fake clock, and the service under test, wired so the
// service's reference collaborators outlive it. Mirrors the reference
// _service(max_items) factory; constructed as a local in each case because the
// service is intentionally non-copyable/non-movable.
struct ServiceHarness {
    explicit ServiceHarness(int max_items = kDefaultMaxItems)
        : service{repository, clock, max_items} {}

    InMemoryHistoryRepository repository;
    FakeClock clock{clock_origin()};
    core::HistoryService service;
};

// Content strings in returned (sorted) order — for the order-sensitive cases.
[[nodiscard]] std::vector<std::string>
contents_in_order(const std::vector<core::ClipboardEntry>& entries) {
    std::vector<std::string> result;
    result.reserve(entries.size());
    for (const auto& entry : entries) {
        result.push_back(entry.content);
    }
    return result;
}

// Content strings as a set — for the membership-only cap cases (the reference
// asserts on a set comprehension).
[[nodiscard]] std::set<std::string> content_set(const std::vector<core::ClipboardEntry>& entries) {
    std::set<std::string> result;
    for (const auto& entry : entries) {
        result.insert(entry.content);
    }
    return result;
}

// test_add_ignores_blank
TEST(HistoryServiceTest, AddIgnoresBlank) {
    ServiceHarness harness;
    EXPECT_FALSE(harness.service.add("   "));
    EXPECT_TRUE(harness.service.entries().empty());
}

// test_add_then_entries_lists_it
TEST(HistoryServiceTest, AddThenEntriesListsIt) {
    ServiceHarness harness;
    EXPECT_TRUE(harness.service.add("hello"));
    EXPECT_EQ(contents_in_order(harness.service.entries()), (std::vector<std::string>{"hello"}));
}

// test_re_add_moves_to_front
TEST(HistoryServiceTest, ReAddMovesToFront) {
    ServiceHarness harness;
    harness.service.add("first");
    harness.clock.advance(kStep);
    harness.service.add("second");
    harness.clock.advance(kStep);
    harness.service.add("first"); // touched most recently
    EXPECT_EQ(contents_in_order(harness.service.entries()),
              (std::vector<std::string>{"first", "second"}));
}

// test_pinned_sort_first
TEST(HistoryServiceTest, PinnedSortFirst) {
    ServiceHarness harness;
    harness.service.add("a");
    harness.clock.advance(kStep);
    harness.service.add("b");
    harness.service.toggle_pin("a");
    EXPECT_EQ(contents_in_order(harness.service.entries()), (std::vector<std::string>{"a", "b"}));
}

// test_cap_evicts_oldest_unpinned
TEST(HistoryServiceTest, CapEvictsOldestUnpinned) {
    ServiceHarness harness{2};
    for (const auto* text : {"a", "b", "c"}) {
        harness.service.add(text);
        harness.clock.advance(kStep);
    }
    EXPECT_EQ(content_set(harness.service.entries()), (std::set<std::string>{"b", "c"}));
}

// test_subscribers_notified_on_change
TEST(HistoryServiceTest, SubscribersNotifiedOnChange) {
    ServiceHarness harness;
    std::vector<int> calls;
    harness.service.subscribe([&calls] { calls.push_back(1); });
    harness.service.add("x");
    EXPECT_EQ(calls, (std::vector<int>{1}));
}

// test_clear_unpinned_keeps_only_pinned
TEST(HistoryServiceTest, ClearUnpinnedKeepsOnlyPinned) {
    ServiceHarness harness;
    harness.service.add("keep");
    harness.clock.advance(kStep);
    harness.service.add("drop");
    harness.service.toggle_pin("keep");
    harness.service.clear_unpinned();
    EXPECT_EQ(contents_in_order(harness.service.entries()), (std::vector<std::string>{"keep"}));
}

// test_toggle_pin_missing_returns_false
TEST(HistoryServiceTest, TogglePinMissingReturnsFalse) {
    ServiceHarness harness;
    EXPECT_FALSE(harness.service.toggle_pin("nope"));
}

// test_pinned_entries_are_never_evicted
TEST(HistoryServiceTest, PinnedEntriesAreNeverEvicted) {
    ServiceHarness harness{2};
    harness.service.add("p1");
    harness.clock.advance(kStep);
    harness.service.add("p2");
    harness.service.toggle_pin("p1");
    harness.service.toggle_pin("p2");
    harness.clock.advance(kStep);
    harness.service.add("new"); // cap is full of pinned items
    const std::set<std::string> contents = content_set(harness.service.entries());
    EXPECT_TRUE(contents.contains("p1"));
    EXPECT_TRUE(contents.contains("p2"));
    EXPECT_FALSE(contents.contains("new")); // the unpinned newcomer is evicted
}

// test_eviction_keeps_pinned_and_most_recent_unpinned
TEST(HistoryServiceTest, EvictionKeepsPinnedAndMostRecentUnpinned) {
    ServiceHarness harness{2};
    harness.service.add("old");
    harness.clock.advance(kStep);
    harness.service.add("pinned");
    harness.service.toggle_pin("pinned");
    harness.clock.advance(kStep);
    harness.service.add("recent");
    EXPECT_EQ(content_set(harness.service.entries()),
              (std::set<std::string>{"pinned", "recent"})); // oldest unpinned evicted
}

// Extra (beyond the Python oracle): proves notify() invokes subscribers OUTSIDE
// the lock. The callback re-enters entries(), which locks the same non-reentrant
// mutex; holding the lock during notification would deadlock here. It must also
// observe the freshly added entry, confirming add() completed its mutation
// before notifying (CP.22).
TEST(HistoryServiceTest, NotifiesSubscribersOutsideTheLock) {
    ServiceHarness harness;
    std::vector<std::size_t> observed_sizes;
    harness.service.subscribe([&] { observed_sizes.push_back(harness.service.entries().size()); });
    harness.service.add("x");
    ASSERT_EQ(observed_sizes.size(), 1U);
    EXPECT_EQ(observed_sizes.front(), 1U);
}

} // namespace
