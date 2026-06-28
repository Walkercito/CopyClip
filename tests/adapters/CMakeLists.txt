copyclip_add_test(manual_hotkey_listener_test SOURCES ManualHotkeyListenerTest.cpp LIBS
                  copyclip::adapters)
copyclip_add_test(x11_mod_mask_test SOURCES X11ModMaskTest.cpp LIBS copyclip::adapters)
copyclip_add_test(factory_test SOURCES FactoryTest.cpp LIBS copyclip::adapters)

# QtClipboardSource needs a QGuiApplication, created in a custom main(), so this
# target links GTest::gtest (not gtest_main) and provides its own entry point.
# Built only when Qt6::Gui is available (it always is once the adapters layer is).
find_package(Qt6 CONFIG REQUIRED COMPONENTS Gui)

add_executable(qt_clipboard_source_test QtClipboardSourceTest.cpp)
target_include_directories(qt_clipboard_source_test
                           PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(qt_clipboard_source_test PRIVATE copyclip::warnings copyclip::sanitizers
                                                       GTest::gtest copyclip::adapters Qt6::Gui)
gtest_discover_tests(qt_clipboard_source_test)
