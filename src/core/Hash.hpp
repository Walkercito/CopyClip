#pragma once

// A stable content fingerprint (FNV-1a, 64-bit, lowercase hex) used as the dedup
// key and images-table key for image clips. Not cryptographic — just a fast,
// collision-resistant fingerprint that is stable across runs, unlike std::hash.

#include <cstddef>
#include <span>
#include <string>

namespace copyclip::core {

// 16-character hex digest of `bytes`.
[[nodiscard]] std::string content_hash(std::span<const std::byte> bytes);

} // namespace copyclip::core
