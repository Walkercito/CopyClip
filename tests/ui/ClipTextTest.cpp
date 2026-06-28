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

TEST(ClipTextTest, ZeroLimitKeepsTheFlattenedText) {
    const Preview preview = make_preview("hello world", 0);
    EXPECT_EQ(preview.text, "hello world");
    EXPECT_FALSE(preview.truncated);
}

TEST(ClipTextTest, FlattensNewlinesToOneLineAndOffersExpand) {
    const Preview preview = make_preview("line1\nline2\nline3", 100);
    EXPECT_EQ(preview.text, "line1 line2 line3");
    EXPECT_TRUE(preview.truncated);
}

TEST(ClipTextTest, CollapsesSpaceRunsWithoutOfferingExpand) {
    const Preview preview = make_preview("a   b", 100);
    EXPECT_EQ(preview.text, "a b");
    EXPECT_FALSE(preview.truncated);
}

TEST(ClipTextTest, TrimsLeadingAndTrailingWhitespace) {
    const Preview preview = make_preview("   hi   ", 100);
    EXPECT_EQ(preview.text, "hi");
    EXPECT_FALSE(preview.truncated);
}

TEST(ClipTextTest, TabsAreStructuralAndOfferExpand) {
    const Preview preview = make_preview("a\tb", 100);
    EXPECT_EQ(preview.text, "a b");
    EXPECT_TRUE(preview.truncated);
}

} // namespace
