#include "config/Constants.hpp"

#include <pwd.h>
#include <unistd.h>

#include <cstdlib>
#include <string>
#include <string_view>

namespace copyclip::config {

namespace {

// Reads an environment variable, returning the fallback when the variable is
// unset OR set to the empty string. This mirrors the reference's `os.environ
// .get(name) or fallback`, where an empty value is treated as absent. The name
// is a null-terminated C string (the kCamelCase env-name constants are views
// over string literals), so getenv can consume it directly.
[[nodiscard]] std::string env_or(const char* name, std::string_view fallback) {
    if (const char* raw = std::getenv(name); raw != nullptr) {
        if (const std::string_view value{raw}; !value.empty()) {
            return std::string{value};
        }
    }
    return std::string{fallback};
}

// The user's home directory, mirroring Python's Path.home(): $HOME when set and
// non-empty, otherwise the passwd database (getpwuid). This keeps data_dir()
// absolute even when HOME is unset (a relative ".local/share" would otherwise
// result). Empty only if neither source is available.
[[nodiscard]] std::string home_dir() {
    if (const char* home = std::getenv(kHomeEnv.data()); home != nullptr && *home != '\0') {
        return std::string{home};
    }
    if (const passwd* entry = ::getpwuid(::getuid());
        entry != nullptr && entry->pw_dir != nullptr) {
        return std::string{entry->pw_dir};
    }
    return std::string{};
}

} // namespace

std::filesystem::path data_dir() {
    const std::string xdg = env_or(kXdgDataHomeEnv.data(), "");
    const std::filesystem::path base = xdg.empty()
                                           ? std::filesystem::path{home_dir()} / kLocalShareSubdir
                                           : std::filesystem::path{xdg};
    return base / kAppId;
}

std::filesystem::path history_db() {
    return data_dir() / kHistoryDbName;
}

std::filesystem::path settings_file() {
    return data_dir() / kSettingsFileName;
}

std::filesystem::path runtime_dir() {
    return std::filesystem::path{env_or(kXdgRuntimeDirEnv.data(), kRuntimeDirFallback)};
}

std::filesystem::path instance_socket() {
    return runtime_dir() / kInstanceSocketName;
}

} // namespace copyclip::config
