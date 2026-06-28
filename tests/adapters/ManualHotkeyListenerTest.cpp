#include "adapters/ManualHotkeyListener.hpp"

#include "core/Enums.hpp"
#include "core/Models.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

using copyclip::adapters::ManualHotkeyListener;
using copyclip::core::HotkeySpec;
using copyclip::core::Key;
using copyclip::core::Modifier;

// Mirrors tests/adapters/test_manual_hotkey.py: the fallback listener is inert —
// it never fires the activation callback — and rebind() always succeeds.

TEST(ManualHotkeyListenerTest, IsInert) {
    ManualHotkeyListener listener{HotkeySpec{{Modifier::Super}, Key::V}, "copyclip-show-ui"};

    std::vector<int> activated;
    listener.start([&activated] { activated.push_back(1); });
    listener.stop();

    EXPECT_TRUE(activated.empty());
    EXPECT_TRUE(listener.rebind(HotkeySpec{{Modifier::Ctrl}, Key::C}));
}

} // namespace
