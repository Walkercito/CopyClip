#pragma once

// RAII environment-variable override for tests.
//
// Brings an environment variable to a known state on construction and restores
// the previous state (original value, or unset) on destruction, so tests that
// redirect XDG paths or probe session-detection variables cannot leak state into
// one another. This mirrors pytest's `monkeypatch.setenv` / `monkeypatch.delenv`
// used by the Python reference tests, but with deterministic scope-based cleanup.
//
// The desired state is expressed as std::optional<std::string>: a value sets the
// variable, std::nullopt unsets it. Either way the prior value (or its absence)
// is captured at construction and restored in the destructor.

#include <cstdlib>
#include <optional>
#include <string>

namespace copyclip::test {

class ScopedEnv {
public:
    // Bring `name` to `desired` for this scope: a value sets it, std::nullopt
    // unsets it. The prior value or absence is captured here and restored in the
    // destructor. A single constructor keeps the two call styles — `{"NAME",
    // "value"}` and `{"NAME", std::nullopt}` — unambiguous: a string literal
    // converts to std::optional<std::string>, std::nullopt selects the unset
    // state.
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
    // unsets it. Used for both the requested state and the restore.
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
