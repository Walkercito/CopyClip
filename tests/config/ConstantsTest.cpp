// Port of the reference oracle tests/config/test_constants.py.
//
// Verifies that path helpers read the environment at call time (so tests and
// sandboxed runs can redirect them via XDG variables) and that the named
// defaults match the reference values exactly.

#include "config/Constants.hpp"
#include "support/ScopedEnv.hpp"

#include <pwd.h>
#include <unistd.h>

#include <filesystem>
#include <optional>

#include <gtest/gtest.h>

namespace {

namespace config = copyclip::config;
using copyclip::test::ScopedEnv;

TEST(ConstantsTest, DataDirFollowsXdg) {
    const ScopedEnv xdg{"XDG_DATA_HOME", "/tmp/xdg"};
    EXPECT_EQ(config::data_dir(), std::filesystem::path{"/tmp/xdg/copyclip"});
}

TEST(ConstantsTest, HistoryAndSettingsLiveUnderDataDir) {
    const ScopedEnv xdg{"XDG_DATA_HOME", "/tmp/xdg"};
    EXPECT_EQ(config::history_db().parent_path(), config::data_dir());
    EXPECT_EQ(config::settings_file().parent_path(), config::data_dir());
}

TEST(ConstantsTest, NamedDefaultsExist) {
    EXPECT_EQ(config::kAppName, "CopyClip");
    EXPECT_EQ(config::kAppId, "copyclip");
    EXPECT_GT(config::kDefaultMaxHistoryItems, 0);
}

// With both XDG_DATA_HOME and HOME unset, data_dir() falls back to the passwd
// home (mirroring Python's Path.home()) -> an absolute path, never a relative
// ".local/share".
TEST(ConstantsTest, DataDirFallsBackToPasswdHomeWhenHomeUnset) {
    const ScopedEnv xdg{"XDG_DATA_HOME", std::nullopt};
    const ScopedEnv home{"HOME", std::nullopt};

    const passwd* entry = ::getpwuid(::getuid());
    ASSERT_NE(entry, nullptr);
    const std::filesystem::path expected =
        std::filesystem::path{entry->pw_dir} / ".local/share" / "copyclip";
    EXPECT_EQ(config::data_dir(), expected);
}

} // namespace
