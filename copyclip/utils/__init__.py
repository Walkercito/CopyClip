"""Utilities and constants for CopyClip."""

from copyclip.utils.constants import (
    APP_NAME,
    CLIPBOARD_CHECK_INTERVAL,
    DATA_DIR,
    DEFAULT_SETTINGS,
    FEEDBACK_DURATION,
    MAX_CHARS_DISPLAY,
)
from copyclip.utils.environment import get_session_type, is_wayland, is_x11

__all__ = [
    "APP_NAME",
    "DATA_DIR",
    "DEFAULT_SETTINGS",
    "MAX_CHARS_DISPLAY",
    "CLIPBOARD_CHECK_INTERVAL",
    "FEEDBACK_DURATION",
    "get_session_type",
    "is_wayland",
    "is_x11",
]
