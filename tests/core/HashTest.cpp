#include "core/Hash.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

namespace {

using copyclip::core::content_hash;

TEST(HashTest, IdenticalBytesHashEqual) {
    const std::vector<std::byte> a{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::vector<std::byte> b{std::byte{1}, std::byte{2}, std::byte{3}};
    EXPECT_EQ(content_hash(a), content_hash(b));
    EXPECT_EQ(content_hash(a).size(), 16U);
}

TEST(HashTest, DifferentBytesHashDiffer) {
    const std::vector<std::byte> a{std::byte{1}, std::byte{2}};
    const std::vector<std::byte> b{std::byte{1}, std::byte{3}};
    EXPECT_NE(content_hash(a), content_hash(b));
}

TEST(HashTest, EmptyHashIsStableAndSized) {
    EXPECT_EQ(content_hash({}).size(), 16U);
    EXPECT_EQ(content_hash({}), content_hash({}));
}

} // namespace
