#pragma once

// RAII temporary directory for storage-layer tests.
//
// Creates a unique directory under std::filesystem::temp_directory_path() on
// construction and removes it (recursively) on destruction, so filesystem-backed
// repositories can be exercised against real on-disk state without leaking files
// between tests. Shared by the storage tests (SQLite history here, JSON settings
// in Task 2.2), mirroring the role of pytest's `tmp_path` fixture in the
// reference suite.
//
// Uniqueness comes from mkdtemp(3), which atomically creates a directory from a
// template ending in "XXXXXX"; this avoids the TOCTOU race of generating a name
// and then creating it. (Using mkdtemp/randomness in test support is fine — the
// no-randomness rule applies to workflow scripts, not test code.)
//
// The type owns a filesystem resource, so it follows the Rule of Five: copying
// would double-free the directory, so copy is deleted; move transfers ownership
// by clearing the source's path so only the final owner cleans up.

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace copyclip::testing {

class TempDir {
public:
    TempDir() {
        const std::filesystem::path tmpl = std::filesystem::temp_directory_path() / kNameTemplate;
        std::string buffer = tmpl.string();
        if (::mkdtemp(buffer.data()) == nullptr) {
            throw std::runtime_error{"TempDir: mkdtemp failed"};
        }
        path_ = buffer;
    }

    ~TempDir() { cleanup(); }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    TempDir(TempDir&& other) noexcept : path_{std::exchange(other.path_, {})} {}

    TempDir& operator=(TempDir&& other) noexcept {
        if (this != &other) {
            cleanup();
            path_ = std::exchange(other.path_, {});
        }
        return *this;
    }

    // The created directory's path. Tests join their fixture files onto it.
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

private:
    // mkdtemp template: the trailing "XXXXXX" is replaced with unique characters.
    static constexpr const char* kNameTemplate = "copyclip-test-XXXXXX";

    // Remove the directory tree if this instance still owns one. Errors are
    // swallowed via the non-throwing overload: a destructor must not throw
    // (E.16), and a failed cleanup of a temp dir is not worth aborting a test.
    void cleanup() noexcept {
        if (!path_.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(path_, ec);
            path_.clear();
        }
    }

    std::filesystem::path path_;
};

} // namespace copyclip::testing
