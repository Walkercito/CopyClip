# Contributing to CopyClip

Thanks for taking the time. CopyClip is a clipboard history manager for Linux,
written in C++23 with a GTK4 + libadwaita UI. Bug reports, fixes, docs, and
features are all welcome.

By participating you agree to follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Ways to contribute

- **Report a bug.** Open an issue. Include your distro, the CopyClip version
  (the release tag you installed), and whether you're on an X11 or a Wayland session —
  clipboard behavior differs between them and that detail saves a round trip.
- **Request a feature.** Open an issue describing the use case, not just the
  solution you have in mind.
- **Send a patch.** For anything larger than a small fix, open an issue or a
  discussion first so we can agree on the approach before you spend time on it.
- **Improve the docs.** README, this file, code comments — all fair game.

Search existing issues before opening a new one.

## Prerequisites

You need a C++23 toolchain and a few build tools:

- **CMake** ≥ 3.28 and **Ninja**
- A C++23 compiler — recent GCC (≥ 13) or Clang (≥ 17)
- **vcpkg** in manifest mode, with `VCPKG_ROOT` exported. Keep the path short
  and free of spaces. The C++ deps (gtest, spdlog, nlohmann-json, sqlitecpp)
  are declared in `vcpkg.json` and pulled automatically at configure time — you
  don't install them by hand.

GTK4, libadwaita, gtkmm-4.0, and the Qt/X11 headless adapter come from the
system, not vcpkg. On Ubuntu 24.04:

```sh
sudo apt install -y qt6-base-dev qt6-tools-dev libgl-dev libx11-dev \
                    libgtkmm-4.0-dev libadwaita-1-dev ninja-build valgrind
```

(`libgtkmm-4.0-dev` pulls in `libgtk-4-dev`, `libglibmm-2.68-dev`, and
`libsigc++-3.0-dev`.) The versions in use: Qt6 6.4.2, GTK4 4.14.5, gtkmm-4.0
4.10.0, libadwaita-1 1.5.0, glibmm-2.68 2.78.1, libX11 1.8.7.

Set up vcpkg once:

```sh
git clone https://github.com/microsoft/vcpkg ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg   # add this to your shell profile
```

For formatting and linting, install the tools (an isolated `uv tool` or `pipx`
install keeps them off the system Python):

```sh
uv tool install pre-commit
# clang-format and clang-tidy come from your distro's LLVM packages
```

## Building

The build is driven by CMake presets (`CMakePresets.json`). The toolchain file
is wired through `VCPKG_ROOT`, so as long as that's set you don't pass extra
flags. There are three presets:

```sh
# Debug — unoptimized, with debug info
cmake --preset debug
cmake --build --preset debug

# Release — optimized at -O2
cmake --preset release
cmake --build --preset release

# ASan+UBSan+LSan — Debug base with sanitizers attached
cmake --preset asan
cmake --build --preset asan
```

Builds go into `build/<preset>/` (out-of-source). The first configure run
downloads and builds the vcpkg dependencies and takes a while; later runs are
cached.

The project builds with `-Wall -Wextra -Wpedantic -Wconversion -Wshadow
-Werror`. A warning fails the build — that's deliberate, it's the type-safety
gate. Don't disable it; fix the warning.

Don't commit `CMakeUserPresets.json` — it's for your local overrides. The
shared `CMakePresets.json` and `vcpkg.json` are checked in.

## Running the app

After a build, the binary lands under `build/<preset>/`. Launch it directly:

```sh
./build/debug/src/ui/copyclip-gtk
```

CopyClip runs in the background and shows its window when you press the hotkey
(default `Super+C`). A couple of runtime notes:

- **Auto-paste needs `/dev/uinput`.** It injects Ctrl+V through a virtual
  keyboard, which requires write access to `/dev/uinput`. Add yourself to the
  `input` group and re-login: `sudo usermod -aG input $USER`. Without it,
  CopyClip falls back to `xdotool` (X11) or `wtype`/`ydotool` (Wayland) when
  they're installed.
- **Wayland rides XWayland.** When a `DISPLAY` is set, CopyClip forces
  `GDK_BACKEND=x11` so it reads the X11 selection, which the compositor mirrors
  to and from the Wayland clipboard. This is the only focus-free way to capture
  in the background under GNOME's Mutter.
- **Running alongside an installed copy.** Set `COPYCLIP_STANDALONE=1` in the environment
  to run your dev build next to an installed instance without the two fighting
  over the same socket and storage.

Settings and history live in `~/.local/share/copyclip/`
(`settings.json` and `history.db`).

## Tests

Tests use GoogleTest and run through CTest:

```sh
ctest --preset debug
```

Before a change merges, it has to pass the sanitizer build clean:

```sh
cmake --build --preset asan
ctest --preset asan
```

Zero leaks and zero undefined behavior is the bar. The `core/` and `storage/`
layers are covered headless with fakes — no display server needed. **A change
to `core/` ships with tests.** If you're fixing a bug there, add a test that
fails before your fix and passes after.

## Code style and linting

- **clang-format** is enforced by the committed `.clang-format`. Every file is
  formatted, no exceptions. The pre-commit hook runs it for you; or run
  `clang-format -i` yourself before pushing.
- **clang-tidy** runs against the committed `.clang-tidy` (bugprone, modernize,
  performance, cppcoreguidelines, readability). Fix the findings. Suppress only
  with a justified `// NOLINT(check, reason)` — never a bare `NOLINT`.
- Install the hooks so format and hygiene checks run on every commit:

  ```sh
  pre-commit install
  ```

  clang-tidy is intentionally left to CI and editor hooks rather than the
  commit hook — it needs the compile database and is too slow to run per commit.

The deeper engineering rules — the layering / one-dependency rule (`core/`
depends on nothing from `adapters/`, `runtime/`, or `ui/`), RAII for every
resource, no owning raw pointers, no magic values, const-correctness — live in
[`CLAUDE.md`](CLAUDE.md). Read it before designing anything non-trivial.

## Commits

We use [Conventional Commits](https://www.conventionalcommits.org/):

```
type(scope): subject
```

- **Types:** `feat`, `fix`, `refactor`, `perf`, `test`, `docs`, `chore`,
  `build`, `ci`.
- Subject in the imperative mood, ≤ 72 characters, no trailing period.
  Example: `feat(hotkeys): grab a single combo via XGrabKey`.
- Add a body only when it adds signal — what changed and why, wrapped at ~72
  columns. Reference issues when relevant (`Closes #12`).
- **No AI or assistant attribution of any kind** — no "Generated with", no
  `Co-Authored-By` trailers, no tool credits, no emoji. Commits read as if the
  maintainer wrote them.

## Pull requests

1. Branch off `main`. Never push to a shared branch directly.
2. Keep the PR focused — one logical change. Split unrelated work.
3. Before opening it, check that:
   - the build is clean (warnings are errors)
   - `ctest --preset debug` passes
   - the `asan` build and tests pass
   - clang-format and clang-tidy are clean
   - docs are updated if behavior changed
4. Open the PR against `main` and describe what changed and why. Expect review
   comments and be ready to revise — rebase rather than pile on merge commits.

A green PR has no failing checks, no merge conflicts, and no unresolved review
threads.

## Reporting security issues

Don't open a public issue for a security problem. Email
walkercitoliver@gmail.com directly with the details and we'll coordinate a fix
before disclosure.

## License

CopyClip is MIT-licensed (© 2024 Walkercito). By contributing, you agree your
contributions are licensed under the same terms.
