#include "ui/ClipText.hpp"

#include <gtest/gtest.h>

namespace {

using copyclip::ui::make_preview;
using copyclip::ui::Preview;

TEST(ClipTextTest, ShortContentIsNotTruncated) {
    const Preview preview = make_preview("hello", 10);
    EXPECT_EQ(preview.text, "hello");
    EXPECT_FALSE(preview.truncated);
}

TEST(ClipTextTest, ContentAtTheLimitIsNotTruncated) {
    const Preview preview = make_preview("hello", 5);
    EXPECT_EQ(preview.text, "hello");
    EXPECT_FALSE(preview.truncated);
}

TEST(ClipTextTest, LongerContentGetsAnEllipsis) {
    const Preview preview = make_preview("hello world", 5);
    EXPECT_EQ(preview.text, "hello…");
    EXPECT_TRUE(preview.truncated);
}

TEST(ClipTextTest, TruncationCutsOnCodePointBoundaries) {
    // U+00E1 U+00E9 U+00ED (a/e/i with acute) — 3 code points, 6 bytes.
    const Preview preview = make_preview("áéí", 2);
    EXPECT_EQ(preview.text, "áé…");
    EXPECT_TRUE(preview.truncated);
}

TEST(ClipTextTest, ZeroLimitDisablesTruncation) {
    const Preview preview = make_preview("hello world", 0);
    EXPECT_EQ(preview.text, "hello world");
    EXPECT_FALSE(preview.truncated);
}

} // namespace
