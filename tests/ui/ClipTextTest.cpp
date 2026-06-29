#include "ui/ClipText.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {

using copyclip::ui::make_preview;
using copyclip::ui::make_valid_utf8;
using copyclip::ui::Preview;

// UTF-8 encoding of U+FFFD (REPLACEMENT CHARACTER), what make_valid_utf8 emits
// for each malformed byte.
const std::string kReplacement{"\xEF\xBF\xBD"};

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

TEST(ClipTextTest, MakeValidUtf8KeepsAsciiAndMultibyte) {
    EXPECT_EQ(make_valid_utf8("hello"), "hello");
    EXPECT_EQ(make_valid_utf8("áéí"), "áéí"); // already valid 2-byte sequences
}

TEST(ClipTextTest, MakeValidUtf8ReplacesPngSignature) {
    // The exact bytes that used to be stored as a "text" clip when a screenshot
    // raced the type list. The lead 0x89 is an invalid UTF-8 start byte.
    const std::string png_signature{"\x89PNG\r\n\x1a\n", 8};
    const std::string expected = kReplacement + std::string{"PNG\r\n\x1a\n", 7};
    EXPECT_EQ(make_valid_utf8(png_signature), expected);
}

TEST(ClipTextTest, MakeValidUtf8ReplacesStrayContinuationByte) {
    const std::string stray{"\x80", 1};
    EXPECT_EQ(make_valid_utf8(stray), kReplacement);
}

TEST(ClipTextTest, MakeValidUtf8ReplacesTruncatedSequence) {
    // 0xC3 begins a 2-byte sequence but the continuation byte is missing.
    const std::string truncated{"a\xC3", 2};
    const std::string expected = "a" + kReplacement;
    EXPECT_EQ(make_valid_utf8(truncated), expected);
}

TEST(ClipTextTest, MakeValidUtf8IsIdempotent) {
    // Sanitizing already-valid output must not change it — a proxy for "the result
    // is valid UTF-8" without pulling GLib into the test.
    const std::string binary{"\x89\xC0\x80\xED\xA0\x80\xFF text", 11};
    const std::string once = make_valid_utf8(binary);
    EXPECT_EQ(make_valid_utf8(once), once);
}

TEST(ClipTextTest, MakePreviewSanitizesBinaryContent) {
    // A binary blob flattened for display must come out as valid UTF-8 (no raw
    // 0x89/0xFF), so a GtkLabel never sees invalid bytes.
    const std::string binary{"\x89\xFF\x01ok", 5};
    const Preview preview = make_preview(binary, 100);
    EXPECT_EQ(make_valid_utf8(preview.text), preview.text);
}

} // namespace
