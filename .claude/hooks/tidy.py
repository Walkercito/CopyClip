#!/usr/bin/env python3
"""PostToolUse hook: lint edited C/C++ files with clang-tidy.

Scoped to the project source subtrees. When clang-tidy reports findings the hook exits with
code 2 so the diagnostics are surfaced back to the agent and get fixed; a clean check exits
0. This is the C++ counterpart of the reference engine's ``typecheck.py``.

clang-tidy needs a compilation database. We locate one under ``build/`` (asan, then debug,
then any configured build). If none exists yet — a clean checkout that has never been
configured — the hook no-ops cleanly rather than break the editing session.

Headers (``.hpp``/``.h``/…) usually have no direct compile command, so running clang-tidy on
them yields spurious "compile command not found" noise. We therefore enforce only ``.cpp``-
family translation units and no-op on headers; their contents are still linted transitively
when an including ``.cpp`` is checked (HeaderFilterRegex covers ``src/``).
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from _hook_lib import compile_db_dir, edited_cpp_file, is_source, run_tool  # noqa: E402

# Translation-unit extensions clang-tidy can lint directly (they have compile commands).
TRANSLATION_UNIT_SUFFIXES = (".cpp", ".cc", ".cxx")


def main() -> int:
    path = edited_cpp_file()
    if path is None or not is_source(path):
        return 0

    # Only .cpp-family files carry a compile command; headers no-op (see module docstring).
    if path.suffix not in TRANSLATION_UNIT_SUFFIXES:
        return 0

    build_dir = compile_db_dir()
    if build_dir is None:
        # Not configured yet — nothing to lint against. Never break the session.
        return 0

    result = run_tool(["clang-tidy", "-p", str(build_dir), str(path)])
    if result is None or result.returncode == 0:
        return 0

    sys.stderr.write("clang-tidy reported findings:\n")
    sys.stderr.write(result.stdout + result.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
