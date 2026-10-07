#include "ui/Fuzzy.hpp"

#include <gtest/gtest.h>

namespace {

using copyclip::ui::match_quality;
using copyclip::ui::MatchQuality;

TEST(FuzzyTest, EmptyQueryMatchesAnything) {
    EXPECT_EQ(match_quality("", "anything"), MatchQuality::Substring);
    EXPECT_EQ(match_quality("", ""), MatchQuality::Substring);
}

TEST(FuzzyTest, ContiguousRunIsASubstringMatch) {
    EXPECT_EQ(match_quality("png", "screenshot.png"), MatchQuality::Substring);
    EXPECT_EQ(match_quality("hello", "hello"), MatchQuality::Substring);
    EXPECT_EQ(match_quality("ell", "hello"), MatchQuality::Substring);
}

TEST(FuzzyTest, ScatteredCharactersAreASubsequenceMatch) {
    EXPECT_EQ(match_quality("spng", "screenshot.png"), MatchQuality::Subsequence);
    EXPECT_EQ(match_quality("hlo", "hello"), MatchQuality::Subsequence);
    EXPECT_EQ(match_quality("dl", "download"), MatchQuality::Subsequence);
}

TEST(FuzzyTest, IsCaseInsensitive) {
    EXPECT_EQ(match_quality("PNG", "screenshot.png"), MatchQuality::Substring);
    EXPECT_EQ(match_quality("png", "SCREENSHOT.PNG"), MatchQuality::Substring);
    EXPECT_EQ(match_quality("SPNG", "screenshot.png"), MatchQuality::Subsequence);
}

TEST(FuzzyTest, RespectsOrderAndExhaustsQuery) {
    EXPECT_EQ(match_quality("gnp", "png"), MatchQuality::None);      // out of order
    EXPECT_EQ(match_quality("xyz", "hello"), MatchQuality::None);    // characters absent
    EXPECT_EQ(match_quality("helloo", "hello"), MatchQuality::None); // query longer than the match
}

TEST(FuzzyTest, SubstringOutranksSubsequence) {
    EXPECT_LT(MatchQuality::Substring, MatchQuality::Subsequence);
    EXPECT_LT(MatchQuality::Subsequence, MatchQuality::None);
}

// A long clip spells almost any query out of scattered letters; the clip that
// holds the query verbatim must still rank above it.
TEST(FuzzyTest, VerbatimClipOutranksLongClipWithScatteredLetters) {
    const auto scattered =
        match_quality("123soleil", "v1.2.3 changelog: fix the solver, release it later");
    const auto verbatim = match_quality("123soleil", "pass: 123Soleil!");
    EXPECT_EQ(scattered, MatchQuality::Subsequence);
    EXPECT_EQ(verbatim, MatchQuality::Substring);
    EXPECT_LT(verbatim, scattered);
}

} // namespace
