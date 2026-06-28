#pragma once

// The production Clock: real wall-clock time. Tests substitute a FakeClock.

#include "core/Interfaces.hpp"

#include <chrono>

namespace copyclip::core {

class SystemClock final : public Clock {
public:
    [[nodiscard]] std::chrono::system_clock::time_point now() const override {
        return std::chrono::system_clock::now();
    }
};

} // namespace copyclip::core
