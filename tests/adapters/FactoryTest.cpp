#include "adapters/Factory.hpp"

#include "adapters/ManualHotkeyListener.hpp"
#include "core/Enums.hpp"
#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {

using copyclip::adapters::HotkeyBuilder;
using copyclip::adapters::ManualHotkeyListener;
using copyclip::adapters::select_hotkey_listener;
using copyclip::core::HotkeyListener;
using copyclip::core::HotkeySpec;
using copyclip::core::Key;
using copyclip::core::Modifier;
using copyclip::core::SessionType;

// A stand-in listener used to confirm a known session's builder is invoked.
class StubListener final : public HotkeyListener {
public:
    explicit StubListener(HotkeySpec spec) : spec_{std::move(spec)} {}
    void start(std::function<void()> /*on_activate*/) override {}
    void stop() override {}
    bool rebind(const HotkeySpec& spec) override {
        spec_ = spec;
        return true;
    }

private:
    HotkeySpec spec_;
};

HotkeySpec super_v() {
    return HotkeySpec{{Modifier::Super}, Key::V};
}

// Mirrors tests/adapters/test_factory.py.

TEST(FactoryTest, UnknownSessionUsesManualFallback) {
    const auto listener =
        select_hotkey_listener(SessionType::Unknown, super_v(), "copyclip-show-ui",
                               std::map<SessionType, HotkeyBuilder>{});
    EXPECT_NE(dynamic_cast<ManualHotkeyListener*>(listener.get()), nullptr);
}

TEST(FactoryTest, KnownSessionUsesItsBuilder) {
    std::map<SessionType, HotkeyBuilder> builders;
    builders[SessionType::X11] = [](const HotkeySpec& spec) {
        return std::make_unique<StubListener>(spec);
    };
    const auto listener =
        select_hotkey_listener(SessionType::X11, super_v(), "copyclip-show-ui", builders);
    EXPECT_NE(dynamic_cast<StubListener*>(listener.get()), nullptr);
}

TEST(FactoryTest, BuilderFailureFallsBackToManual) {
    std::map<SessionType, HotkeyBuilder> builders;
    builders[SessionType::X11] = [](const HotkeySpec&) -> std::unique_ptr<HotkeyListener> {
        throw std::runtime_error{"no display"};
    };
    const auto listener =
        select_hotkey_listener(SessionType::X11, super_v(), "copyclip-show-ui", builders);
    EXPECT_NE(dynamic_cast<ManualHotkeyListener*>(listener.get()), nullptr);
}

} // namespace
