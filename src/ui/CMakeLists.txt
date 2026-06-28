# copyclip_ui: the GTK4 + libadwaita UI layer. Links only the pure engine — never
# Qt — so the GTK frontend stays toolkit-isolated.
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)
pkg_check_modules(ADWAITA REQUIRED IMPORTED_TARGET libadwaita-1)

# Treat the toolkit headers as SYSTEM so their own warnings never trip -Werror.
foreach(toolkit PkgConfig::GTKMM PkgConfig::ADWAITA)
  get_target_property(includes ${toolkit} INTERFACE_INCLUDE_DIRECTORIES)
  if(includes)
    set_target_properties(${toolkit} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${includes}")
  endif()
endforeach()

# Pure presentation logic (no GTK) — its own library so it unit-tests headless.
add_library(copyclip_ui_text STATIC ClipText.cpp)
add_library(copyclip::ui_text ALIAS copyclip_ui_text)
target_include_directories(copyclip_ui_text PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(copyclip_ui_text PRIVATE copyclip::warnings copyclip::sanitizers)

add_library(
  copyclip_ui STATIC
  Theme.cpp
  MainWindow.cpp
  Application.cpp
  GdkClipboardSource.cpp
  widgets/ClipCard.cpp
  dialogs/SettingsDialog.cpp)
add_library(copyclip::ui ALIAS copyclip_ui)

target_include_directories(copyclip_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(copyclip_ui PUBLIC copyclip::core PkgConfig::GTKMM PkgConfig::ADWAITA)
target_link_libraries(copyclip_ui PRIVATE copyclip::ui_text copyclip::warnings
                                          copyclip::sanitizers)

# The windowed application.
add_executable(copyclip-gtk main.cpp)
target_link_libraries(
  copyclip-gtk PRIVATE copyclip::ui copyclip::storage copyclip::runtime copyclip::config
                       copyclip::warnings copyclip::sanitizers)
