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

// Golden vector: pins the exact algorithm + hex encoding. A change to the digest
// would orphan every stored image blob (the hash is the images-table key).
TEST(HashTest, GoldenVectorPinsTheAlgorithm) {
    const std::vector<std::byte> abc{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};
    EXPECT_EQ(content_hash(abc), "e71fa2190541574b"); // FNV-1a 64-bit of "abc"
}

} // namespace
