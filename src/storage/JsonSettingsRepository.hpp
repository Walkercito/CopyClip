#pragma once

// JSON-backed core::SettingsRepository, mirroring the reference module
// copyclip/storage/json_settings.py. load() degrades to defaults whenever the
// file is absent, unreadable, unparseable, or malformed (never throws); save()
// writes atomically (temp file + rename) so a reader never sees a half-written
// file. nlohmann-json is confined to the .cpp so it never leaks through this
// interface.

#include "core/Interfaces.hpp"
#include "core/Models.hpp"

#include <filesystem>

namespace copyclip::storage {

class JsonSettingsRepository final : public core::SettingsRepository {
public:
    // The file need not exist yet: load() treats a missing file as "use defaults"
    // and save() creates the parent directory.
    explicit JsonSettingsRepository(std::filesystem::path path);

    [[nodiscard]] core::Settings load() const override;

    void save(const core::Settings& settings) override;

private:
    std::filesystem::path path_;
};

} // namespace copyclip::storage
