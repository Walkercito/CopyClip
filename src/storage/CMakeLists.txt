# copyclip_storage: concrete repositories backing the core/ persistence seams.
#
# Dependency rule (CLAUDE.md): this target depends only on copyclip::core (for the
# HistoryRepository interface + ClipboardEntry value type) and SQLiteCpp/sqlite3.
# It must link NO Qt, Xlib, or D-Bus, ever.
find_package(SQLiteCpp CONFIG REQUIRED)

add_library(copyclip_storage STATIC SqliteHistoryRepository.cpp)
add_library(copyclip::storage ALIAS copyclip_storage)

target_include_directories(copyclip_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# core is PUBLIC: the interface + value types appear in copyclip_storage's public
# headers. SQLiteCpp is PRIVATE: it is an implementation detail of the .cpp and
# must not leak onto consumers. The warning/sanitizer gates stay PRIVATE so they
# never propagate to third-party targets.
target_link_libraries(
  copyclip_storage
  PUBLIC copyclip::core
  PRIVATE copyclip::warnings copyclip::sanitizers SQLiteCpp)
