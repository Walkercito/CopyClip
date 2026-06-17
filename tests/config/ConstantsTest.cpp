// Port of the reference oracle tests/config/test_constants.py.
//
// Verifies that path helpers read the environment at call time (so tests and
// sandboxed runs can redirect them via XDG variables) and that the named
// defaults match the reference values exactly.

#include "config/Constants.hpp"
#include "support/ScopedEnv.hpp"

#include <filesystem>

#include <gtest/gtest.h>

namespace {

namespace config = copyclip::config;
using copyclip::test::ScopedEnv;

// test_data_dir_follows_xdg
TEST(ConstantsTest, DataDirFollowsXdg) {
    const ScopedEnv xdg{"XDG_DATA_HOME", "/tmp/xdg"};
    EXPECT_EQ(config::data_dir(), std::filesystem::path{"/tmp/xdg/copyclip"});
}

// test_history_and_settings_live_under_data_dir
TEST(ConstantsTest, HistoryAndSettingsLiveUnderDataDir) {
    const ScopedEnv xdg{"XDG_DATA_HOME", "/tmp/xdg"};
    EXPECT_EQ(config::history_db().parent_path(), config::data_dir());
    EXPECT_EQ(config::settings_file().parent_path(), config::data_dir());
}

// test_named_defaults_exist
TEST(ConstantsTest, NamedDefaultsExist) {
    EXPECT_EQ(config::kAppName, "CopyClip");
    EXPECT_EQ(config::kAppId, "copyclip");
    EXPECT_GT(config::kDefaultMaxHistoryItems, 0);
}

} // namespace
