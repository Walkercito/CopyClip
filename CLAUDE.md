# CopyClip — Engineering Standards

A clipboard manager for Linux (X11 + Wayland). The shipping app is **C++/Qt6**. This file
defines its non-negotiable principles, toolchain, and conventions. They are not suggestions.

The Python code in this repository — the engine, plus the archived UI under
`reference/pyqt6-ui/` — is the **validated reference**: a working, fully-tested design that the
C++ port mirrors. It is not the shipping target; the port is executed from a separate
migration plan.

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

> `core/` includes **nothing** from `adapters/`, `runtime/`, or `ui/`. Qt, Xlib, and D-Bus
> appear **only** under `adapters/`.

This keeps `core/` pure and unit-testable with no display server.

Layout: `core/` (pure domain + application logic) · `storage/` (concrete repositories) ·
`adapters/` (Qt, Xlib, portal — the only impure layer) · `runtime/` (process glue) ·
`config/` (constants) · `ui/` (Qt6 UI).

## Toolchain

Two installed skills carry the detailed how-to; the house rules below govern the rest.

- **`cpp-coding-standards`** — invoke it for language and design rules (C++ Core Guidelines:
  type safety, resource safety, immutability, clarity).
- **`cpp-testing`** — invoke it for tests (GoogleTest + CMake/CTest).
- The **`cpp-reviewer`** agent reviews changes; **`cpp-build-resolver`** triages build errors.

### Standard & build
- **C++23**: `CMAKE_CXX_STANDARD 23`, `CXX_STANDARD_REQUIRED ON`, `CXX_EXTENSIONS OFF`.
- **CMake** (≥ 3.28); out-of-source builds; target-based, no global flags.
- **vcpkg** in manifest mode (`vcpkg.json`, pinned versions). Qt6 comes from the system or the
  official Qt SDK via `find_package`, not vcpkg. (Conan is an acceptable alternative — pick
  one and commit to it; do not mix.)

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
- Edit hooks mirror the reference project's: **clang-format** + **clang-tidy** run on every
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
