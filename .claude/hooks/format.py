#!/usr/bin/env python3
"""PostToolUse hook: auto-format edited C/C++ files with clang-format.

Formatting is always safe, so this hook applies to every edited C/C++ file in the project
and never blocks the session. It mirrors the reference engine's ``format.py``.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from _hook_lib import edited_cpp_file, run_tool  # noqa: E402


def main() -> int:
    path = edited_cpp_file()
    if path is None:
        return 0

    run_tool(["clang-format", "-i", str(path)])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
