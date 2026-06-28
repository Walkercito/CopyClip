#pragma once

// Single-instance enforcement and "show" IPC over a Unix domain socket, mirroring
// the reference module copyclip/runtime/single_instance.py. The first instance to
// acquire() binds, listens, and serves "show" requests on a background thread;
// later instances fail to bind and call signal_show() to wake it.

#include <filesystem>
#include <functional>
#include <stop_token>
#include <thread>
#include <utility>

#include <unistd.h>

namespace copyclip::runtime {

// Move-only RAII wrapper around a POSIX file descriptor: closes it on destruction
// so a socket fd is never owned as a bare int.
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

    // Tries to become the single instance: on success binds, listens, serves
    // "show" on a background thread, returns true; returns false if another live
    // instance holds the socket. A stale socket file (on disk, no listener) is
    // reclaimed and acquisition proceeds.
    bool acquire(std::function<void()> on_show);

    // Connect to the running instance and send the one-byte "show" command.
    // Returns false if no instance is reachable.
    [[nodiscard]] bool signal_show() const;

    // Stop serving, close the socket, and remove the socket file. Idempotent;
    // also called by the destructor. A no-op on a guard that never acquired: it
    // does not own the socket file (the running instance does) and must not delete
    // it.
    void release();

private:
    // Accept loop on the background thread: waits for connections and runs the
    // callback on a valid "show" command. Exits promptly when a stop is requested
    // (release()/destruction).
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
