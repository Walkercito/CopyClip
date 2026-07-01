#pragma once

// Application-wide constants. Paths are computed from the environment at call
// time (not static-init) so tests and sandboxed runs can redirect them via XDG
// variables. Mirrors copyclip/config/constants.py.

#include <filesystem>
#include <string_view>

namespace copyclip::config {

inline constexpr std::string_view kAppName = "CopyClip";
inline constexpr std::string_view kAppId = "copyclip";

// Version when running from a source tree without installed package metadata.
inline constexpr std::string_view kAppVersion = "0.0.0";

inline constexpr int kDefaultMaxHistoryItems = 70;

// Default open shortcut as a GNOME accelerator (matches core::kDefaultPreset,
// Super+V). Stored verbatim in settings and written straight to gsettings.
inline constexpr std::string_view kDefaultHotkeyAccelerator = "<Super>v";
inline constexpr std::string_view kHistoryDbName = "history.db";
inline constexpr std::string_view kSettingsFileName = "settings.json";
inline constexpr std::string_view kInstanceSocketName = "copyclip.sock";

// One-byte "show the window" command sent over the single-instance socket.
inline constexpr char kInstanceShowCommand = 'S';
inline constexpr int kInstanceSocketBacklog = 1;

// Temp-file suffix for atomic settings writes (write temp, then rename).
inline constexpr std::string_view kSettingsTempSuffix = ".json.tmp";

inline constexpr std::string_view kXdgDataHomeEnv = "XDG_DATA_HOME";
inline constexpr std::string_view kHomeEnv = "HOME";
inline constexpr std::string_view kXdgRuntimeDirEnv = "XDG_RUNTIME_DIR";

inline constexpr std::string_view kLocalShareSubdir = ".local/share";
inline constexpr std::string_view kRuntimeDirFallback = "/tmp";

// $XDG_DATA_HOME/copyclip, falling back to $HOME/.local/share/copyclip when
// XDG_DATA_HOME is unset or empty. Mirrors the Python reference when HOME is set.
[[nodiscard]] std::filesystem::path data_dir();

[[nodiscard]] std::filesystem::path history_db();

[[nodiscard]] std::filesystem::path settings_file();

// Falls back to /tmp when $XDG_RUNTIME_DIR is unset or empty.
[[nodiscard]] std::filesystem::path runtime_dir();

[[nodiscard]] std::filesystem::path instance_socket();

} // namespace copyclip::config
