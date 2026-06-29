#include "ui/Fuzzy.hpp"

#include <gtest/gtest.h>

namespace {

using copyclip::ui::fuzzy_matches;

TEST(FuzzyTest, EmptyQueryMatchesAnything) {
    EXPECT_TRUE(fuzzy_matches("", "anything"));
    EXPECT_TRUE(fuzzy_matches("", ""));
}

TEST(FuzzyTest, MatchesSubsequence) {
    EXPECT_TRUE(fuzzy_matches("spng", "screenshot.png"));
    EXPECT_TRUE(fuzzy_matches("hlo", "hello"));
    EXPECT_TRUE(fuzzy_matches("dl", "download"));
    EXPECT_TRUE(fuzzy_matches("hello", "hello"));
}

TEST(FuzzyTest, IsCaseInsensitive) {
    EXPECT_TRUE(fuzzy_matches("PNG", "screenshot.png"));
    EXPECT_TRUE(fuzzy_matches("png", "SCREENSHOT.PNG"));
}

TEST(FuzzyTest, RespectsOrderAndExhaustsQuery) {
    EXPECT_FALSE(fuzzy_matches("gnp", "png"));      // out of order
    EXPECT_FALSE(fuzzy_matches("xyz", "hello"));    // characters absent
    EXPECT_FALSE(fuzzy_matches("helloo", "hello")); // query longer than the match
}

} // namespace
