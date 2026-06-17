// Placeholder translation unit so copyclip_core compiles before any real
// sources exist. A CMake library target with zero sources fails to build; this
// gives it exactly one. Phase 1 (config/core) replaces this file with the first
// real source (e.g. Platform.cpp), at which point the placeholder is removed.
namespace copyclip::core {}  // namespace copyclip::core
