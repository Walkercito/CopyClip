#pragma once

// JSON-backed user settings store.
//
// Concrete copyclip::core::SettingsRepository persisting a Settings value to a
// JSON file, mirroring the reference module copyclip/storage/json_settings.py.
// load() reads the file with per-field fallback for missing keys and degrades to
// the documented defaults whenever the file is absent, unreadable, unparseable, or
// carries a malformed field — it never throws on bad data. save() writes
// atomically: it serializes to a sibling temp file and renames it onto the final
// path, so a reader never observes a half-written settings file.
//
// The path is stored by value (the type owns no OS resource); the file handle for
// each read/write is opened and closed within the call via RAII, so the Rule of
// Zero applies and no custom special members are needed.
//
// Dependency rule (CLAUDE.md): the storage layer depends only on core/ and
// nlohmann-json — never on Qt, Xlib, or D-Bus. nlohmann-json is confined to the
// .cpp so it never leaks through this interface onto consumers.

#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <filesystem>

namespace copyclip::storage {

class JsonSettingsRepository final : public core::SettingsRepository {
public:
    // Bind the repository to `path`. The file need not exist yet; load() treats a
    // missing file as "use defaults" and save() creates the parent directory.
    explicit JsonSettingsRepository(std::filesystem::path path);

    // Read settings from the file, falling back to Settings{} defaults when the
    // file is missing, unreadable, unparseable, or contains a malformed field.
    [[nodiscard]] core::Settings load() const override;

    // Serialize `settings` and write them atomically (temp file + rename).
    void save(const core::Settings& settings) override;

private:
    std::filesystem::path path_;
};

} // namespace copyclip::storage
