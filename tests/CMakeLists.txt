# Test aggregator. GoogleTest comes from vcpkg (manifest mode). Each test target
# links only the layer under test plus the warnings/sanitizer gates, and is
# discovered into CTest. A helper keeps per-test wiring to a single call so the
# pattern stays DRY as layers are added.
find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

# copyclip_add_test(<name> SOURCES <src...> [LIBS <lib...>])
#
# Defines a GoogleTest executable, links the project gates (warnings-as-errors +
# sanitizers) and GoogleTest's main, makes the tree's include roots visible
# (src/ for production headers, tests/ for shared test support), and registers
# the cases with CTest via gtest_discover_tests.
function(copyclip_add_test name)
  cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

  add_executable(${name} ${ARG_SOURCES})
  target_include_directories(${name} PRIVATE ${PROJECT_SOURCE_DIR}/src
                                             ${PROJECT_SOURCE_DIR}/tests)
  target_link_libraries(
    ${name}
    PRIVATE copyclip::warnings copyclip::sanitizers GTest::gtest_main
            ${ARG_LIBS})
  gtest_discover_tests(${name})
endfunction()

add_subdirectory(config)
add_subdirectory(core)
add_subdirectory(storage)
add_subdirectory(runtime)
