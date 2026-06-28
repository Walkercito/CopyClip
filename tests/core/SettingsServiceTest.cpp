#include "core/SettingsService.hpp"

#include "core/Enums.hpp"
#include "core/Models.hpp"
#include "support/Fakes.hpp"

#include <gtest/gtest.h>

namespace {

using copyclip::core::HotkeyPreset;
using copyclip::core::Settings;
using copyclip::core::SettingsService;
using copyclip::core::Theme;
using copyclip::testing::InMemorySettingsRepository;

// Mirrors tests/core/test_settings.py: the service loads on construction,
// persists every change, and drives the first-run lifecycle.

TEST(SettingsServiceTest, LoadsOnConstruction) {
    InMemorySettingsRepository repository{Settings{.theme = Theme::Light}};
    const SettingsService service{repository};
    EXPECT_EQ(service.settings().theme, Theme::Light);
}

TEST(SettingsServiceTest, UpdatePersists) {
    InMemorySettingsRepository repository;
    SettingsService service{repository};

    Settings next = service.settings();
    next.theme = Theme::Light;
    service.update(next);

    EXPECT_EQ(service.settings().theme, Theme::Light);
    EXPECT_EQ(repository.load().theme, Theme::Light);
}

TEST(SettingsServiceTest, FirstRunLifecycle) {
    InMemorySettingsRepository repository;
    SettingsService service{repository};

    EXPECT_TRUE(service.is_first_run());

    service.complete_first_run(HotkeyPreset::SuperC);

    EXPECT_FALSE(service.is_first_run());
    EXPECT_EQ(service.settings().hotkey, HotkeyPreset::SuperC);
    EXPECT_TRUE(repository.load().first_run_completed);
}

} // namespace
