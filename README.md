# CopyClip Engine (C++)

A native **C++23 / Qt6** port of CopyClip's clipboard engine for Linux (X11 + Wayland).
This repository contains the **engine only** — no UI. It is a headless library plus a
headless runner that proves the engine captures the clipboard and fires global hotkeys.

The behavioral specification and test oracle is the validated Python implementation in the
[CopyClip](../CopyClip) repository; this port mirrors it unit-for-unit with identical
behavior and equivalent test coverage.

## Architecture

Layered, with dependencies pointing inward:

- `src/core/` — pure domain + application logic (no Qt, no Xlib, no D-Bus)
- `src/storage/` — concrete repositories (SQLite, JSON)
- `src/adapters/` — the only impure layer (Qt, Xlib, portal/D-Bus)
- `src/runtime/` — process glue (logging, single-instance guard)
- `src/config/` — constants
- `src/app/` — the headless composition root

`core/` and `storage/` link no Qt/Xlib/D-Bus; the CMake target graph enforces this.

## Toolchain

- C++23, CMake ≥ 3.28, Ninja
- vcpkg (manifest mode): GoogleTest, spdlog, nlohmann-json, SQLiteCpp
- Qt6 (Core, Gui, DBus) and libX11 from the system via `find_package`
- clang-format, clang-tidy, and an ASan+UBSan+LSan build gate every commit

See `CLAUDE.md` for the full engineering standards and `docs/plans/` for the migration plan.

## Build

```sh
export VCPKG_ROOT="$HOME/vcpkg"
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```
