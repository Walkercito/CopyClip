#pragma once

// RAII temporary directory for the storage-layer tests, mirroring pytest's
// `tmp_path`: creates a unique dir under temp_directory_path() and removes it
// recursively on destruction. Uniqueness comes from mkdtemp(3), which creates the
// directory atomically, avoiding the name-then-create TOCTOU race. Owns a
// filesystem resource, so it follows the Rule of Five: copy is deleted; move
// clears the source's path so only the final owner cleans up.

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

    // The created directory's path; tests join their fixture files onto it.
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
