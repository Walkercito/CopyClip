#pragma once

// RAII environment-variable override for tests: brings a variable to a known
// state and restores the prior value (or absence) on scope exit, so tests that
// redirect XDG paths or session-detection vars cannot leak state into each other.
// Mirrors pytest's monkeypatch.setenv/delenv with deterministic scoped cleanup.
// The desired state is std::optional<std::string>: a value sets it, nullopt unsets.

#include <cstdlib>
#include <optional>
#include <string>

namespace copyclip::test {

class ScopedEnv {
public:
    // Bring `name` to `desired` for this scope: a value sets it, std::nullopt
    // unsets it. The prior value or absence is captured here and restored in the
    // destructor.
    ScopedEnv(std::string name, const std::optional<std::string>& desired)
        : name_{std::move(name)} {
        if (const char* previous = std::getenv(name_.c_str()); previous != nullptr) {
            previous_ = std::string{previous};
        }
        apply(desired);
    }

    ~ScopedEnv() { apply(previous_); }

    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;
    ScopedEnv(ScopedEnv&&) = delete;
    ScopedEnv& operator=(ScopedEnv&&) = delete;

private:
    // Drive the variable to `state`: a value sets it (overwriting), std::nullopt
    // unsets it. Shared by the initial apply and the restore.
    void apply(const std::optional<std::string>& state) const {
        if (state.has_value()) {
            ::setenv(name_.c_str(), state->c_str(), 1 /*overwrite*/);
        } else {
            ::unsetenv(name_.c_str());
        }
    }

    std::string name_;
    std::optional<std::string> previous_;
};

} // namespace copyclip::test
