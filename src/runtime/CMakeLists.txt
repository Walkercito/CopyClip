# copyclip_runtime: process glue (logging, single-instance enforcement).
#
# Dependency rule (CLAUDE.md): the runtime layer stays Qt-free — it uses spdlog
# for logging and POSIX primitives for IPC, never Qt/Xlib/D-Bus.
add_library(copyclip_runtime STATIC Logging.cpp)
add_library(copyclip::runtime ALIAS copyclip_runtime)

target_include_directories(copyclip_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# spdlog is part of the public logging API (configure_logging takes a spdlog
# level), so it is linked PUBLIC; consumers of copyclip::runtime see it.
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(copyclip_runtime PUBLIC spdlog::spdlog)
target_link_libraries(copyclip_runtime PRIVATE copyclip::config copyclip::warnings
                                               copyclip::sanitizers)
