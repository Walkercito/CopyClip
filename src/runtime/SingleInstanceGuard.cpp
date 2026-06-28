#include "runtime/SingleInstanceGuard.hpp"

#include "config/Constants.hpp"

#include <spdlog/spdlog.h>

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cerrno>
#include <cstring>
#include <string>
#include <utility>

namespace copyclip::runtime {

namespace {

// How long the accept loop blocks in poll() before re-checking the stop token.
constexpr int kPollTimeoutMs = 100;

// The POSIX socket API takes a generic sockaddr*; reinterpreting the concrete
// sockaddr_un is the standard, unavoidable idiom. Confined to one helper so the
// suppression lives in exactly one place.
const sockaddr* as_sockaddr(const sockaddr_un& addr) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<const sockaddr*>(&addr);
}

// Build an AF_UNIX address for `path`. Returns false if the path does not fit in
// sun_path (with its NUL terminator).
bool fill_address(const std::string& path, sockaddr_un& addr) {
    if (path.size() >= sizeof(addr.sun_path)) {
        return false;
    }
    addr.sun_family = AF_UNIX;
    std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);
    return true;
}

UniqueFd make_unix_socket() {
    return UniqueFd{::socket(AF_UNIX, SOCK_STREAM, 0)};
}

} // namespace

SingleInstanceGuard::SingleInstanceGuard(std::filesystem::path socket_path)
    : socket_path_{std::move(socket_path)} {}

SingleInstanceGuard::~SingleInstanceGuard() {
    release();
}

bool SingleInstanceGuard::acquire(std::function<void()> on_show) {
    const std::string path = socket_path_.string();
    sockaddr_un addr{};
    if (!fill_address(path, addr)) {
        return false;
    }
    const auto addr_len = static_cast<socklen_t>(sizeof(addr));

    UniqueFd server = make_unix_socket();
    if (!server.valid()) {
        return false;
    }

    if (::bind(server.get(), as_sockaddr(addr), addr_len) != 0) {
        // The bind failed because the socket file already exists. If nothing is
        // listening on it, a previous instance crashed without cleaning up:
        // reclaim the stale file and try once more with a fresh socket.
        if (!is_stale()) {
            return false;
        }
        std::error_code remove_error;
        std::filesystem::remove(socket_path_, remove_error);
        server = make_unix_socket();
        if (!server.valid() || ::bind(server.get(), as_sockaddr(addr), addr_len) != 0) {
            return false;
        }
    }

    if (::listen(server.get(), config::kInstanceSocketBacklog) != 0) {
        return false;
    }

    server_ = std::move(server);
    on_show_ = std::move(on_show);
    thread_ = std::jthread{[this](const std::stop_token& stop_token) { serve(stop_token); }};
    acquired_ = true;
    return true;
}

bool SingleInstanceGuard::signal_show() const {
    const std::string path = socket_path_.string();
    sockaddr_un addr{};
    if (!fill_address(path, addr)) {
        return false;
    }

    UniqueFd client = make_unix_socket();
    if (!client.valid()) {
        return false;
    }
    if (::connect(client.get(), as_sockaddr(addr), static_cast<socklen_t>(sizeof(addr))) != 0) {
        spdlog::warn("could not signal running instance: {}", std::strerror(errno));
        return false;
    }

    const char command = config::kInstanceShowCommand;
    return ::send(client.get(), &command, 1, 0) == 1;
}

void SingleInstanceGuard::release() {
    // A guard that never acquired does not own the socket file (it belongs to the
    // running instance); tearing down here would delete a live instance's socket.
    if (!acquired_) {
        return;
    }
    if (thread_.joinable()) {
        thread_.request_stop();
        thread_.join();
    }
    server_.reset();
    if (!socket_path_.empty()) {
        std::error_code remove_error;
        std::filesystem::remove(socket_path_, remove_error);
    }
    acquired_ = false;
}

void SingleInstanceGuard::serve(const std::stop_token& stop_token) {
    while (!stop_token.stop_requested()) {
        pollfd descriptor{};
        descriptor.fd = server_.get();
        descriptor.events = POLLIN;

        const int ready = ::poll(&descriptor, 1, kPollTimeoutMs);
        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (ready == 0 || (descriptor.revents & POLLIN) == 0) {
            continue;
        }

        UniqueFd connection{::accept(server_.get(), nullptr, nullptr)};
        if (!connection.valid()) {
            continue;
        }
        char command = 0;
        const ssize_t received = ::recv(connection.get(), &command, 1, 0);
        if (received == 1 && command == config::kInstanceShowCommand && on_show_) {
            on_show_();
        }
    }
}

bool SingleInstanceGuard::is_stale() const {
    const std::string path = socket_path_.string();
    sockaddr_un addr{};
    if (!fill_address(path, addr)) {
        return false;
    }
    UniqueFd probe = make_unix_socket();
    if (!probe.valid()) {
        return false;
    }
    // A successful connect means a live instance is listening (not stale).
    return ::connect(probe.get(), as_sockaddr(addr), static_cast<socklen_t>(sizeof(addr))) != 0;
}

} // namespace copyclip::runtime
