#pragma once

// The production Clock: reports the real wall-clock time. Injected wherever a
// Clock seam is needed (e.g. HistoryService) so that tests can substitute a
// FakeClock. Pure core layer — no Qt, Xlib, or D-Bus.

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
