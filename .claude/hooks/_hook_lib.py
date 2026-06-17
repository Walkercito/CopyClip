"""Shared helpers for CopyClip C++ quality hooks (PostToolUse).

Each hook receives the tool payload as JSON on stdin and decides whether to act on the
edited file. These helpers centralise payload parsing, path scoping, and tool execution so
the individual hooks stay tiny and consistent. This is the C++ counterpart of the reference
Python engine's ``_hook_lib.py``: it drives clang-format / clang-tidy instead of ruff / ty.
"""

import json
import os
import subprocess
import sys
from pathlib import Path

# Project subtrees the lint gate applies to. Everything else (build output, vcpkg installs,
# docs) is out of scope for the editing hooks.
SOURCE_ROOTS = ("src", "tests")

# C/C++ file extensions the hooks recognise. Headers are included so format can tidy them up;
# tidy itself decides whether a header has a usable compile command.
CPP_SUFFIXES = (".cpp", ".cc", ".cxx", ".hpp", ".hh", ".hxx", ".h")


def project_dir() -> Path:
    """Return the project root, preferring the directory Claude Code exposes to hooks."""
    return Path(os.environ.get("CLAUDE_PROJECT_DIR", os.getcwd()))


def edited_cpp_file() -> Path | None:
    """Return the edited C/C++ file from the PostToolUse payload, or ``None``.

    ``None`` signals the caller to exit cleanly: the payload was unreadable, the tool did
    not touch a file, the file is not a C/C++ source/header, or it no longer exists.
    """
    try:
        payload = json.load(sys.stdin)
    except (json.JSONDecodeError, ValueError):
        return None

    raw = (payload.get("tool_input") or {}).get("file_path")
    if not raw:
        return None

    path = Path(raw)
    if path.suffix not in CPP_SUFFIXES or not path.exists():
        return None
    return path


def is_source(path: Path) -> bool:
    """Return ``True`` when ``path`` lives under a subtree the quality gates cover."""
    try:
        relative = path.resolve().relative_to(project_dir().resolve())
    except ValueError:
        return False
    return len(relative.parts) > 0 and relative.parts[0] in SOURCE_ROOTS


def compile_db_dir() -> Path | None:
    """Return a build directory containing a ``compile_commands.json``, or ``None``.

    clang-tidy needs a compilation database. We prefer the sanitizer build (it carries the
    same flags CI gates on), then the debug build, then any other configured build. A clean
    checkout that has never been configured has no database; the caller must no-op then
    rather than emit spurious errors.
    """
    root = project_dir()
    preferred = (root / "build" / "asan", root / "build" / "debug")
    for candidate in preferred:
        if (candidate / "compile_commands.json").is_file():
            return candidate

    for candidate in sorted((root / "build").glob("*")):
        if (candidate / "compile_commands.json").is_file():
            return candidate
    return None


def run_tool(command: list[str]) -> subprocess.CompletedProcess[str] | None:
    """Run a tool from the project root, returning ``None`` if the tool is unavailable.

    A missing tool must never break the editing session, so ``FileNotFoundError`` is
    swallowed and reported as ``None`` for the caller to treat as a no-op.
    """
    try:
        return subprocess.run(
            command,
            cwd=project_dir(),
            capture_output=True,
            text=True,
            check=False,
        )
    except FileNotFoundError:
        return None
