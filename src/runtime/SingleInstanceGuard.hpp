#pragma once

// Single-instance enforcement and "show" IPC over a Unix domain socket.
//
// Mirrors the reference module copyclip/runtime/single_instance.py. The first
// instance to acquire() binds and listens on the socket and serves "show"
// requests on a background thread. Later instances fail to bind; they call
// signal_show() to connect and wake the running instance. POSIX-only (AF_UNIX)
// so the runtime layer stays Qt-free.

#include <filesystem>
#include <functional>
#include <stop_token>
#include <thread>
#include <utility>

#include <unistd.h>

namespace copyclip::runtime {

// Move-only RAII wrapper around a POSIX file descriptor: closes it on
// destruction. A socket fd is never owned as a bare int (CLAUDE.md: RAII for
// every resource, sockets included).
class UniqueFd {
public:
    UniqueFd() noexcept = default;
    explicit UniqueFd(int fd) noexcept : fd_{fd} {}
    ~UniqueFd() { reset(); }

    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;

    UniqueFd(UniqueFd&& other) noexcept : fd_{std::exchange(other.fd_, -1)} {}
    UniqueFd& operator=(UniqueFd&& other) noexcept {
        if (this != &other) {
            reset();
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] bool valid() const noexcept { return fd_ >= 0; }

    void reset() noexcept {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    int fd_ = -1;
};

class SingleInstanceGuard {
public:
    explicit SingleInstanceGuard(std::filesystem::path socket_path);
    ~SingleInstanceGuard();

    SingleInstanceGuard(const SingleInstanceGuard&) = delete;
    SingleInstanceGuard& operator=(const SingleInstanceGuard&) = delete;
    SingleInstanceGuard(SingleInstanceGuard&&) = delete;
    SingleInstanceGuard& operator=(SingleInstanceGuard&&) = delete;

    // Try to become the single instance. On success, binds + listens + serves
    // "show" requests on a background thread and returns true. If another live
    // instance already holds the socket, returns false. A stale socket file
    // (present on disk but with no listener) is reclaimed and acquisition
    // proceeds.
    bool acquire(std::function<void()> on_show);

    // Connect to the running instance and send the one-byte "show" command.
    // Returns false if no instance is reachable.
    [[nodiscard]] bool signal_show() const;

    // Stop serving, close the socket, and remove the socket file. Idempotent and
    // also invoked by the destructor. A no-op on a guard that never successfully
    // acquired: such a guard does not own the socket file (it belongs to the
    // running instance) and must never delete it.
    void release();

private:
    // Accept loop run on the background thread: waits for connections and, on a
    // valid "show" command, invokes the registered callback. Exits promptly when
    // a stop is requested (release()/destruction).
    void serve(const std::stop_token& stop_token);

    // True when the socket file exists but nothing is listening (a crashed
    // instance left it behind): a connect attempt fails.
    [[nodiscard]] bool is_stale() const;

    std::filesystem::path socket_path_;
    std::function<void()> on_show_;
    UniqueFd server_;
    std::jthread thread_;
    // True only between a successful acquire() and release(): gates socket
    // teardown so a non-owning guard never removes the running instance's socket.
    bool acquired_ = false;
};

} // namespace copyclip::runtime
