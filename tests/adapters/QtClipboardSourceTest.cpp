#include "adapters/QtClipboardSource.hpp"

#include <gtest/gtest.h>

#include <QGuiApplication>

#include <optional>
#include <string>
#include <vector>

namespace {

using copyclip::adapters::QtClipboardSource;

// Mirrors tests/adapters/test_qt_clipboard.py, run against the offscreen QPA
// platform so it needs no real display.

TEST(QtClipboardSourceTest, WriteThenReadRoundtrip) {
    QtClipboardSource source;
    source.write("hello qt");
    EXPECT_EQ(source.read(), std::optional<std::string>{"hello qt"});
}

TEST(QtClipboardSourceTest, DataChangedInvokesCallback) {
    QtClipboardSource source;
    std::vector<std::string> seen;
    source.start([&seen](const std::string& text) { seen.push_back(text); });
    source.write("changed"); // setText emits dataChanged
    QCoreApplication::processEvents();
    EXPECT_FALSE(seen.empty());
    if (!seen.empty()) {
        EXPECT_EQ(seen.back(), "changed");
    }
    source.stop();
}

} // namespace

// Custom main(): QtClipboardSource needs a live QGuiApplication for the whole run.
int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    const QGuiApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
