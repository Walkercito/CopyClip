# copyclip_adapters: the ONLY impure layer — Qt, Xlib, and the desktop portal
# live here and nowhere else. Other layers must not link them; this target may.
# Qt6/X11 links are added alongside the sources that need them (the Qt clipboard
# source, the X11 listener, the portal listener); the manual fallback needs
# neither.
add_library(copyclip_adapters STATIC ManualHotkeyListener.cpp QtClipboardSource.cpp)
add_library(copyclip::adapters ALIAS copyclip_adapters)

target_include_directories(copyclip_adapters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(spdlog CONFIG REQUIRED)
# Qt6::Gui backs the QClipboard source. It is exposed in QtClipboardSource.hpp
# (QClipboard / QMetaObject types), so it is linked PUBLIC.
find_package(Qt6 CONFIG REQUIRED COMPONENTS Gui)

target_link_libraries(copyclip_adapters PUBLIC copyclip::core Qt6::Gui)
target_link_libraries(copyclip_adapters PRIVATE spdlog::spdlog copyclip::warnings
                                                copyclip::sanitizers)
