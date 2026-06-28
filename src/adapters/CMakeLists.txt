# copyclip_adapters: the ONLY impure layer — Qt, Xlib, and the desktop portal
# live here and nowhere else. Other layers must not link them; this target may.
# Qt6/X11 links are added alongside the sources that need them (the Qt clipboard
# source, the X11 listener, the portal listener); the manual fallback needs
# neither.
add_library(copyclip_adapters STATIC ManualHotkeyListener.cpp QtClipboardSource.cpp
                                     X11HotkeyListener.cpp PortalHotkeyListener.cpp)
add_library(copyclip::adapters ALIAS copyclip_adapters)

# PortalHotkeyListener is a QObject (it receives the portal's Activated D-Bus
# signal through a slot), so this target needs Qt's meta-object compiler.
set_target_properties(copyclip_adapters PROPERTIES AUTOMOC ON)

target_include_directories(copyclip_adapters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(spdlog CONFIG REQUIRED)
# Qt6::Gui backs the QClipboard source and Qt6::DBus the portal listener. Their
# Qt types appear in the adapter headers, so they are linked PUBLIC.
find_package(Qt6 CONFIG REQUIRED COMPONENTS Gui DBus)
# libX11 backs the X11 hotkey listener; Xlib types appear in its header, PUBLIC.
find_package(X11 REQUIRED)

target_link_libraries(copyclip_adapters PUBLIC copyclip::core Qt6::Gui Qt6::DBus X11::X11)
target_link_libraries(copyclip_adapters PRIVATE spdlog::spdlog copyclip::warnings
                                                copyclip::sanitizers)
