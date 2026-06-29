#include "core/Hash.hpp"

#include <cstdint>
#include <format>

namespace copyclip::core {

namespace {

constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

} // namespace

std::string content_hash(std::span<const std::byte> bytes) {
    std::uint64_t hash = kFnvOffsetBasis;
    for (const std::byte byte : bytes) {
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= kFnvPrime;
    }
    return std::format("{:016x}", hash);
}

} // namespace copyclip::core
