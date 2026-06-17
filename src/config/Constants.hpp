#pragma once

// Application-wide constants: identity, paths, and named defaults.
//
// Paths are computed from the environment at call time (not at static-init time)
// so tests and sandboxed runs can redirect them via XDG variables. Mirrors the
// reference module copyclip/config/constants.py.

#include <filesystem>
#include <string_view>

namespace copyclip::config {

// --- Identity ----------------------------------------------------------------
inline constexpr std::string_view kAppName = "CopyClip";
inline constexpr std::string_view kAppId = "copyclip";

// Version when running from a source tree without installed package metadata.
inline constexpr std::string_view kAppVersion = "0.0.0";

// --- Named defaults ----------------------------------------------------------
inline constexpr int kDefaultMaxHistoryItems = 200;
inline constexpr std::string_view kHistoryDbName = "history.db";
inline constexpr std::string_view kSettingsFileName = "settings.json";
inline constexpr std::string_view kInstanceSocketName = "copyclip.sock";

// --- Logging -----------------------------------------------------------------
inline constexpr std::string_view kLogFormat = "%(asctime)s %(levelname)-7s %(name)s: %(message)s";

// --- Single-instance IPC -----------------------------------------------------
// A one-byte "show the window" command sent over the Unix socket.
inline constexpr char kInstanceShowCommand = 'S';
inline constexpr int kInstanceSocketBacklog = 1;

// Suffix for the temp file used to write settings atomically (write temp, then
// rename).
inline constexpr std::string_view kSettingsTempSuffix = ".json.tmp";

// --- Environment names (used to derive paths at call time) -------------------
inline constexpr std::string_view kXdgDataHomeEnv = "XDG_DATA_HOME";
inline constexpr std::string_view kHomeEnv = "HOME";
inline constexpr std::string_view kXdgRuntimeDirEnv = "XDG_RUNTIME_DIR";

// --- Path fallback fragments -------------------------------------------------
inline constexpr std::string_view kLocalShareSubdir = ".local/share";
inline constexpr std::string_view kRuntimeDirFallback = "/tmp";

// --- Environment-derived paths (evaluated at call time) ----------------------
// Base data directory: $XDG_DATA_HOME/copyclip. When XDG_DATA_HOME is unset or
// empty it falls back to $HOME/.local/share/copyclip, reading $HOME from the
// HOME environment variable. This mirrors the Python reference's behavior
// whenever HOME is set.
[[nodiscard]] std::filesystem::path data_dir();

// History database file under the data directory.
[[nodiscard]] std::filesystem::path history_db();

// Settings file under the data directory.
[[nodiscard]] std::filesystem::path settings_file();

// Runtime directory: $XDG_RUNTIME_DIR, falling back to /tmp when unset or empty.
[[nodiscard]] std::filesystem::path runtime_dir();

// Single-instance Unix socket path under the runtime directory.
[[nodiscard]] std::filesystem::path instance_socket();

} // namespace copyclip::config
