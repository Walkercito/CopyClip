# copyclip_core: pure domain + application logic.
#
# Dependency rule (CLAUDE.md): this target must link NO Qt, Xlib, or D-Bus,
# ever. Sources are pure C++ that read only the process environment.
add_library(copyclip_core STATIC Platform.cpp Hotkeys.cpp HistoryService.cpp
                                 SettingsService.cpp ClipboardEngine.cpp Hash.cpp)
add_library(copyclip::core ALIAS copyclip_core)

target_include_directories(copyclip_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# spdlog provides engine diagnostics (CLAUDE.md mandates a logging library, never
# cout/printf). It is a pure logging dependency — not Qt, Xlib, or D-Bus — so the
# core layer stays display-server-free. Linked PRIVATE: no spdlog type leaks into
# the public core headers.
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(copyclip_core PRIVATE copyclip::warnings copyclip::sanitizers
                                            spdlog::spdlog)
