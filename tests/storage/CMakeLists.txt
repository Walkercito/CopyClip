copyclip_add_test(sqlite_history_repository_test SOURCES SqliteHistoryRepositoryTest.cpp
                  LIBS copyclip::storage)

copyclip_add_test(json_settings_repository_test SOURCES JsonSettingsRepositoryTest.cpp
                  LIBS copyclip::storage)
