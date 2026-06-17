# copyclip_storage: concrete repositories backing the core/ persistence seams.
#
# Dependency rule (CLAUDE.md): this target depends only on copyclip::core (for the
# repository interfaces + value types), SQLiteCpp/sqlite3, nlohmann-json, and
# spdlog. It must link NO Qt, Xlib, or D-Bus, ever.
find_package(SQLiteCpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_library(copyclip_storage STATIC SqliteHistoryRepository.cpp
                                    JsonSettingsRepository.cpp)
add_library(copyclip::storage ALIAS copyclip_storage)

target_include_directories(copyclip_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# core is PUBLIC: the interfaces + value types appear in copyclip_storage's public
# headers. SQLiteCpp, nlohmann-json, and spdlog are PRIVATE: they are
# implementation details of the .cpp files and must not leak onto consumers. The
# warning/sanitizer gates stay PRIVATE so they never propagate to third-party
# targets.
target_link_libraries(
  copyclip_storage
  PUBLIC copyclip::core
  PRIVATE copyclip::warnings copyclip::sanitizers SQLiteCpp
          nlohmann_json::nlohmann_json spdlog::spdlog)
