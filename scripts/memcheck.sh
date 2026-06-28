#!/usr/bin/env bash
# Run Valgrind memcheck over the engine's GoogleTest binaries.
#
# Binaries that link Qt6/X11 are skipped: those toolkits make many internal
# static allocations Valgrind reports as false positives, so the adapters/app
# tests rely on the ASan+UBSan+LSan build (the `asan` preset) instead. The pure
# config/core/storage/runtime tests must be Valgrind-clean.
#
# Usage: scripts/memcheck.sh [build-dir]   (default: build/debug)
# The build dir must be a non-sanitized build — Valgrind and ASan conflict.
set -euo pipefail

build_dir="${1:-build/debug}"
if [[ ! -d "${build_dir}/tests" ]]; then
  echo "error: no tests under '${build_dir}' — configure & build the debug preset first" >&2
  exit 2
fi

valgrind_opts=(
  --leak-check=full
  --show-leak-kinds=definite,indirect
  --errors-for-leak-kinds=definite,indirect
  --track-origins=yes
  --error-exitcode=1
)

failures=0
while IFS= read -r bin; do
  name="$(basename "${bin}")"
  if ldd "${bin}" 2>/dev/null | grep -qiE 'libQt6|libX11'; then
    echo "SKIP  ${name}  (links Qt6/X11 — covered by the asan build)"
    continue
  fi
  printf 'CHECK %s ... ' "${name}"
  if valgrind "${valgrind_opts[@]}" "${bin}" >/dev/null 2>&1; then
    echo "clean"
  else
    echo "*** VALGRIND ERRORS (rerun without redirect to see them) ***"
    failures=$((failures + 1))
  fi
done < <(find "${build_dir}/tests" -type f -executable -name '*_test' | sort)

if ((failures > 0)); then
  echo "memcheck: ${failures} binar(ies) reported errors" >&2
  exit 1
fi
echo "memcheck: all Qt/X11-free test binaries clean"
