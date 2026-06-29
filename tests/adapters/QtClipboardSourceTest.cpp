#include "adapters/QtClipboardSource.hpp"

#include <gtest/gtest.h>

#include <QGuiApplication>

#include <optional>
#include <string>
#include <vector>

namespace {

using copyclip::adapters::QtClipboardSource;
using copyclip::core::ClipContent;
using copyclip::core::ClipKind;

// Mirrors tests/adapters/test_qt_clipboard.py, run against the offscreen QPA
// platform so it needs no real display.

TEST(QtClipboardSourceTest, WriteThenReadRoundtrip) {
    QtClipboardSource source;
    source.write(ClipContent{.kind = ClipKind::Text, .text = "hello qt"});
    EXPECT_EQ(source.read(), std::optional<std::string>{"hello qt"});
}

TEST(QtClipboardSourceTest, DataChangedInvokesCallback) {
    QtClipboardSource source;
    std::vector<std::string> seen;
    source.start([&seen](const ClipContent& content) { seen.push_back(content.text); });
    source.write(ClipContent{.kind = ClipKind::Text, .text = "changed"}); // emits dataChanged
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
