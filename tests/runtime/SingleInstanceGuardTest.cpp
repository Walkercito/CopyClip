#include "runtime/SingleInstanceGuard.hpp"

#include "support/TempDir.hpp"

#include <gtest/gtest.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <future>
#include <string>

namespace {

using copyclip::runtime::SingleInstanceGuard;
using copyclip::testing::TempDir;

// Create a Unix-domain socket file at `path` and close it WITHOUT unlinking,
// simulating a crashed instance that left a stale socket file behind.
void leave_stale_socket(const std::filesystem::path& path) {
    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(fd, 0);
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    const std::string text = path.string();
    ASSERT_LT(text.size(), sizeof(addr.sun_path));
    std::memcpy(addr.sun_path, text.c_str(), text.size() + 1);
    // The POSIX socket API takes a generic sockaddr*; reinterpreting sockaddr_un
    // is the standard, unavoidable idiom.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    ASSERT_EQ(::bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)), 0);
    ::close(fd);
}

// Mirrors tests/runtime/test_single_instance.py.

TEST(SingleInstanceGuardTest, SecondAcquireFailsAndSignalsFirst) {
    const TempDir dir;
    const std::filesystem::path socket_path = dir.path() / "copyclip.sock";

    std::atomic<int> show_count{0};
    std::promise<void> shown;
    std::future<void> shown_future = shown.get_future();

    SingleInstanceGuard first{socket_path};
    ASSERT_TRUE(first.acquire([&] {
        show_count.fetch_add(1, std::memory_order_relaxed);
        shown.set_value();
    }));

    SingleInstanceGuard second{socket_path};
    EXPECT_FALSE(second.acquire([&] { show_count.fetch_add(100, std::memory_order_relaxed); }));
    EXPECT_TRUE(second.signal_show());

    ASSERT_EQ(shown_future.wait_for(std::chrono::seconds{2}), std::future_status::ready);
    EXPECT_EQ(show_count.load(), 1);

    first.release();
}

TEST(SingleInstanceGuardTest, ReleaseAllowsReacquire) {
    const TempDir dir;
    const std::filesystem::path socket_path = dir.path() / "copyclip.sock";

    SingleInstanceGuard first{socket_path};
    ASSERT_TRUE(first.acquire([] {}));
    first.release();

    SingleInstanceGuard second{socket_path};
    EXPECT_TRUE(second.acquire([] {}));
    second.release();
}

TEST(SingleInstanceGuardTest, StaleSocketIsReclaimed) {
    const TempDir dir;
    const std::filesystem::path socket_path = dir.path() / "copyclip.sock";
    leave_stale_socket(socket_path);
    ASSERT_TRUE(std::filesystem::exists(socket_path));

    SingleInstanceGuard guard{socket_path};
    EXPECT_TRUE(guard.acquire([] {})); // detects the stale path and reclaims it
    guard.release();
}

} // namespace
