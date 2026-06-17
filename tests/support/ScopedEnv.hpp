#pragma once

// RAII environment-variable override for tests.
//
// Sets an environment variable on construction and restores the previous state
// (original value, or unset) on destruction, so tests that redirect XDG paths
// cannot leak state into one another. This mirrors pytest's `monkeypatch.setenv`
// used by the Python reference tests, but with deterministic scope-based cleanup.

#include <cstdlib>
#include <optional>
#include <string>

namespace copyclip::test {

class ScopedEnv {
public:
    ScopedEnv(std::string name, const std::string& value) : name_{std::move(name)} {
        if (const char* previous = std::getenv(name_.c_str()); previous != nullptr) {
            previous_ = std::string{previous};
        }
        ::setenv(name_.c_str(), value.c_str(), 1 /*overwrite*/);
    }

    ~ScopedEnv() {
        if (previous_.has_value()) {
            ::setenv(name_.c_str(), previous_->c_str(), 1 /*overwrite*/);
        } else {
            ::unsetenv(name_.c_str());
        }
    }

    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;
    ScopedEnv(ScopedEnv&&) = delete;
    ScopedEnv& operator=(ScopedEnv&&) = delete;

private:
    std::string name_;
    std::optional<std::string> previous_;
};

} // namespace copyclip::test
