"""Environment detection utilities."""

import os
import shutil


def get_session_type() -> str:
    """Detect the current session type (X11 or Wayland).

    Returns:
        'x11', 'wayland', or 'unknown'
    """
    session_type = os.getenv("XDG_SESSION_TYPE", "").lower()

    if session_type in ("x11", "wayland"):
        return session_type

    if os.getenv("WAYLAND_DISPLAY"):
        return "wayland"
    elif os.getenv("DISPLAY"):
        return "x11"

    return "unknown"


def is_wayland() -> bool:
    """Check if running on Wayland.

    Returns:
        True if running on Wayland, False otherwise
    """
    return get_session_type() == "wayland"


def is_x11() -> bool:
    """Check if running on X11.

    Returns:
        True if running on X11, False otherwise
    """
    return get_session_type() == "x11"


def detect_paste_tool() -> str | None:
    """Detect available paste tool for auto-paste functionality.

    Returns:
        Tool name ('xdotool', 'wtype', 'ydotool') or None if none available
    """
    if is_x11():
        # On X11
        if shutil.which("xdotool"):
            return "xdotool"
    else:
        # On Wayland, prefer wtype, fallback to ydotool
        if shutil.which("wtype"):
            return "wtype"
        if shutil.which("ydotool"):
            return "ydotool"
    return None
