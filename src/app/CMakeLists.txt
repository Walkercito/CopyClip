# copyclip: the headless composition root. Wires every layer together and runs
# the Qt event loop. A QGuiApplication (no window) backs QClipboard.
find_package(Qt6 CONFIG REQUIRED COMPONENTS Gui)

add_executable(copyclip main.cpp)

target_include_directories(copyclip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(
  copyclip
  PRIVATE copyclip::core copyclip::storage copyclip::adapters copyclip::runtime
          copyclip::config Qt6::Gui copyclip::warnings copyclip::sanitizers)
