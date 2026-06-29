# CopyClip — Engineering Standards

A clipboard manager for Linux (X11 + Wayland). The shipping app is **C++**: a pure engine
with Qt/X11 adapters and a **GTK4 + libadwaita** UI. This file defines its non-negotiable
principles, toolchain, and conventions. They are not suggestions.

This repository is the **standalone working copy** for the C++ port — develop here
directly (there is no separate parent checkout). The Python code it also carries — the
engine, plus the archived PyQt6 UI under `reference/pyqt6-ui/` — is the **validated
reference**: a working, fully-tested design the C++ port mirrors. It is not the shipping
target; the port is executed from a separate migration plan.

## Reference implementations — consult before designing clipboard behavior

- **Python reference (this repo)** — the validated design described above; the C++
  port mirrors it.

## Non-negotiable principles

- **OOP.** Model behavior as small classes with one clear responsibility and explicit
  collaborators. Depend on abstractions (abstract base classes / concepts), not concretions,
  and inject collaborators. No god-objects, no grab-bag modules.
- **DRY.** No copy-paste. If logic appears twice, extract it — in production code, tooling,
  and tests alike.
- **No magic values.** No literal numbers or strings with hidden meaning in logic. Use named
  `constexpr` constants gathered in a dedicated constants header, and **`enum class`** (never
  plain `enum`). No ad-hoc constants scattered through implementation files.

## Architecture — one dependency rule

The app is layered, and dependencies point inward:

> `core/` includes **nothing** from `adapters/`, `runtime/`, or `ui/`. Qt, Xlib, D-Bus, and
> GTK appear **only** under `adapters/` (toolkit adapters) and `ui/` (the GTK4 UI).

This keeps `core/` pure and unit-testable with no display server. A frontend links only the
pure engine (`core/` · `storage/` · `runtime/`) plus the adapters it needs — so the GTK UI
never pulls in Qt.

Layout: `core/` (pure domain + application logic) · `storage/` (concrete repositories) ·
`adapters/` (Qt, Xlib, portal — the only impure non-UI layer) · `runtime/` (process glue) ·
`config/` (constants) · `ui/` (GTK4 + libadwaita UI, including the clipboard sources).

### Clipboard capture — the X11/XWayland selection backend (critical)

`core::ClipboardSource` has one implementation, `ui/GdkClipboardSource` (GTK
`Gdk::Clipboard`), used for **both** X11 and Wayland sessions — X11 selections need no
window focus, so the GTK clipboard works for a background manager.

On Wayland it rides **XWayland**: `main.cpp` forces `GDK_BACKEND=x11` whenever a
`DISPLAY` exists, so GTK talks to the X11 selection, which the compositor mirrors to
and from the Wayland clipboard. This is required because GNOME's Mutter exposes no
`wlr`/`ext-data-control` protocol — the only focus-free way to read a Wayland clipboard
in the background — so a native `wl_data_device` clipboard would capture only while
focused.

Capture is **event-driven** (`Gdk::Clipboard::signal_changed`, no polling). Reads are
asynchronous: an image (`read_texture_async`) is tried first, else text, else rich text
— and the `text/html` stream is drained with `splice_async`, because a synchronous read
would block the GLib main loop the X11/INCR transfer depends on and deadlock the UI.
Pasting into the previously-focused window (`ui/KeystrokePaster`, when auto-paste is on)
injects Ctrl+V through a transient `/dev/uinput` virtual keyboard — kernel-level, so it
reaches X11 and Wayland apps alike; it needs `/dev/uinput` write access (the `input`
group), falling back to `xdotool`/`wtype`/`ydotool`.

## Toolchain

Two installed skills carry the detailed how-to; the house rules below govern the rest.

- **`cpp-coding-standards`** — invoke it for language and design rules (C++ Core Guidelines:
  type safety, resource safety, immutability, clarity).
- **`cpp-testing`** — invoke it for tests (GoogleTest + CMake/CTest).
- The **`cpp-reviewer`** agent reviews changes; **`cpp-build-resolver`** triages build errors.

### Standard & build
- **C++23**: `CMAKE_CXX_STANDARD 23`, `CXX_STANDARD_REQUIRED ON`, `CXX_EXTENSIONS OFF`.
- **CMake** (≥ 3.28); out-of-source builds; target-based, no global flags.
- **vcpkg** in manifest mode (`vcpkg.json`, pinned versions). Qt6, GTK4/gtkmm, and libadwaita
  come from the system (`find_package` / `pkg-config`), not vcpkg. (Conan is an acceptable
  alternative — pick one and commit to it; do not mix.)

### System dependencies (apt, Ubuntu 24.04)

The C++ library deps (gtest, spdlog, nlohmann-json, sqlitecpp) come from vcpkg; clang-format,
clang-tidy, and pre-commit are isolated `uv tool` installs; vcpkg is bootstrapped under
`$VCPKG_ROOT`. The remaining build/link prerequisites are apt **dev** packages — listed here so
they can be removed once no longer needed:

- **Engine — Qt headless adapter + X11 hotkey:** `qt6-base-dev` `qt6-tools-dev` `libgl-dev`
  `libx11-dev`
- **GTK4 UI — gtkmm + libadwaita:** `libgtkmm-4.0-dev` `libadwaita-1-dev` (these pull
  `libgtk-4-dev`, `libglibmm-2.68-dev`, `libsigc++-3.0-dev`)
- **Build / analysis:** `ninja-build` `valgrind`

```sh
# install
sudo apt install -y qt6-base-dev qt6-tools-dev libgl-dev libx11-dev \
                    libgtkmm-4.0-dev libadwaita-1-dev ninja-build valgrind
# remove when done
sudo apt remove --autoremove qt6-base-dev qt6-tools-dev libgl-dev libx11-dev \
                             libgtkmm-4.0-dev libadwaita-1-dev valgrind
```

Versions in use: Qt6 6.4.2 · GTK4 4.14.5 · gtkmm-4.0 4.10.0 · libadwaita-1 1.5.0 ·
glibmm-2.68 2.78.1 · libX11 1.8.7 · Valgrind 3.22.0.

### Formatting & linting
- **clang-format** with a committed `.clang-format`. Every file is formatted; no exceptions.
- **clang-tidy** with a committed `.clang-tidy` enabling `bugprone-*`, `modernize-*`,
  `performance-*`, `cppcoreguidelines-*`, and `readability-*`. Fix findings; suppress only with
  a justified `// NOLINT(check, reason)`.
- **Warnings are errors** — the static type/safety gate. Build with `-Wall -Wextra -Wpedantic
  -Wconversion -Wshadow -Werror`. A clean build is non-negotiable.

### Memory & resource safety (non-negotiable)
- **RAII for every resource** — memory, files, sockets, Qt objects, locks — released in a
  destructor.
- **No owning raw pointers; no manual `new`/`delete`.** `std::unique_ptr` by default,
  `std::shared_ptr` only for genuine shared ownership. Raw pointers/references observe only.
- **Rule of zero** — prefer types needing no custom special members; if you write one, write
  all five.
- **`const`-correct** by default; `constexpr`/`consteval` where possible; pass by value or
  `const&` / `std::string_view` / `std::span`, never by owning pointer.
- **Sanitizers are part of the workflow.** A dedicated build runs the test suite under
  AddressSanitizer + UndefinedBehaviorSanitizer + LeakSanitizer, and CI runs it; Valgrind for
  deeper analysis. **Zero leaks and zero UB gate merges.**

### Error handling
- No silent failure. Report errors via exceptions or `std::expected` / `std::optional` —
  never ignored return codes. Use a logging library (spdlog or Qt logging), never
  `std::cout` / `printf` for diagnostics.

### Performance (profile first)
- Measure before optimizing — `perf`, `valgrind --tool=callgrind`, or a sampling profiler. No
  speculative micro-optimization. Release builds at `-O2`; prefer move semantics; avoid
  needless copies and heap allocations; `reserve()` containers; pass non-owning views
  (`std::span`, `std::string_view`).

### Testing & hooks
- **GoogleTest + CTest** (per the `cpp-testing` skill). `core/` and `storage/` are covered
  headless with fakes; a change to `core/` ships with tests.
- Edit hooks run **clang-format** + **clang-tidy** on every
  edited `.cpp`/`.hpp`, and the suite runs under sanitizers. Do not fight them — fix what they
  surface.

## Commits

Industry-standard, not changelog essays.

- **Conventional Commits**: `type(scope): subject` — e.g. `feat(hotkeys): grab a single
  combo via XGrabKey`. Types: `feat`, `fix`, `refactor`, `perf`, `test`, `docs`, `chore`,
  `build`, `ci`.
- Subject in the imperative mood, ≤ 72 chars, no trailing period.
- Body only when it adds signal: what changed and why, wrapped at ~72 cols. A few lines,
  not a hundred. Reference issues (`Closes #12`) when relevant.
- **No AI/assistant references of any kind.** No "Generated with", no
  `Co-Authored-By: Claude`, no tool credits, no emoji robots. Commits read as if written
  by the maintainer.
- Commit only when asked. Branch off `main` for feature work; never commit straight to a
  shared branch without being told to.
