cmake_minimum_required(VERSION 3.28)

# --- vcpkg toolchain guard ----------------------------------------------------
# The presets point CMAKE_TOOLCHAIN_FILE at $env{VCPKG_ROOT}/scripts/... . When
# VCPKG_ROOT is unset that expands to an empty prefix and CMake fails with a
# cryptic "Could not find toolchain file: /scripts/buildsystems/vcpkg.cmake".
# Fail early with an actionable message instead. This must come before the first
# project()/enable-language, which is when the toolchain file is read.
if(NOT DEFINED ENV{VCPKG_ROOT})
  message(
    FATAL_ERROR
      "VCPKG_ROOT is not set. Export it before configuring: export VCPKG_ROOT=\"$HOME/vcpkg\""
  )
endif()

project(
  copyclip_engine
  VERSION 0.1.0
  DESCRIPTION "Native C++23 port of the CopyClip clipboard engine (headless, no UI)."
  LANGUAGES CXX)

# --- Language standard (non-negotiable: C++23, no extensions) -----------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json so clang-tidy and editors can find compile flags.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Warnings-as-errors gate --------------------------------------------------
# An INTERFACE target carrying the project's static type/safety flags. Every
# target links it PRIVATEly so the flags never leak to consumers. This keeps the
# build target-based with no global flag pollution.
add_library(copyclip_warnings INTERFACE)
add_library(copyclip::warnings ALIAS copyclip_warnings)

target_compile_options(
  copyclip_warnings
  INTERFACE -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Werror)

# --- Sanitizer gate -----------------------------------------------------------
# An INTERFACE target carrying ASan+UBSan+LSan (LSan ships with ASan on Linux).
# Targets link it PRIVATEly so sanitization is opt-in per project target and
# never leaks onto third-party/vcpkg targets. When COPYCLIP_SANITIZE is OFF the
# target carries no flags and is a harmless no-op, so call sites link it
# unconditionally. The asan preset turns the option ON.
option(COPYCLIP_SANITIZE "Build with ASan+UBSan+LSan" OFF)

add_library(copyclip_sanitizers INTERFACE)
add_library(copyclip::sanitizers ALIAS copyclip_sanitizers)

if(COPYCLIP_SANITIZE)
  target_compile_options(
    copyclip_sanitizers INTERFACE -fsanitize=address,undefined
                                  -fno-omit-frame-pointer)
  target_link_options(copyclip_sanitizers INTERFACE
                      -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

# --- Layers -------------------------------------------------------------------
# src/CMakeLists.txt adds one subdirectory per layer as each comes to exist.
add_subdirectory(src)

# --- Packaging ----------------------------------------------------------------
# Install rules + CPack config (.deb/.rpm/.tar.gz). Added after src so the
# copyclip-gtk target exists when packaging installs it.
add_subdirectory(packaging)

# --- Tests --------------------------------------------------------------------
# GoogleTest + CTest. Enabled unconditionally: the quality gate runs ctest on
# every preset. tests/ provides a copyclip_add_test() helper and one target per
# unit under test.
enable_testing()
add_subdirectory(tests)

# --- Memory-check convenience target ------------------------------------------
# `cmake --build <dir> --target memcheck` builds the tests, then runs Valgrind
# over the Qt/X11-free test binaries (the adapters/app tests are Qt-noisy under
# Valgrind and are covered by the asan preset instead). Defined only when
# Valgrind is installed, and meaningful only on a non-sanitized build (the debug
# preset) since Valgrind and ASan conflict.
find_program(VALGRIND_COMMAND valgrind)
if(VALGRIND_COMMAND)
  add_custom_target(
    memcheck
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}"
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/memcheck.sh" "${CMAKE_BINARY_DIR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    USES_TERMINAL
    COMMENT "Valgrind memcheck over the Qt/X11-free test binaries")
endif()
