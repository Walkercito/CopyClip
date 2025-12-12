"""Application-wide constants and configuration values."""

import os
from pathlib import Path

APP_NAME = "CopyClip"
APP_VERSION = "0.1.2"

# Project paths
PROJECT_ROOT = Path(__file__).parent.parent.parent
ASSETS_DIR = PROJECT_ROOT / "assets"
APP_ICON = ASSETS_DIR / "CopyClip_logo.png"

# Data paths
DATA_DIR = (
    Path(os.getenv("XDG_DATA_HOME", os.path.expanduser("~/.local/share"))) / "clipboard-manager"
)
HISTORY_FILE = DATA_DIR / "clipboard_history.json"
SETTINGS_FILE = DATA_DIR / "settings.json"

# UI Constants
MAX_CHARS_DISPLAY = 100  # Maximum characters to display before truncation
WINDOW_MIN_WIDTH = 350
WINDOW_MIN_HEIGHT = 400
WINDOW_DEFAULT_WIDTH = 380
WINDOW_DEFAULT_HEIGHT = 550

# Timing constants
CLIPBOARD_CHECK_INTERVAL = 1000  # Check clipboard every 1 second
FEEDBACK_DURATION = 2000  # Show feedback for 2 seconds
FEEDBACK_DURATION_SHORT = 1500  # Short feedback duration
CLIPBOARD_AUTO_HIDE_DELAY = 800  # Auto-hide delay after copy

# Default settings
DEFAULT_SETTINGS = {
    # UI preferences
    "theme": "dark",
    "hotkey": "super_v",
    "first_run_completed": False,
    # Behavior settings
    "auto_hide_on_copy": True,
    "auto_paste_on_copy": False,
    # Timing settings (in milliseconds)
    "clipboard_check_interval": 1000,  # Check clipboard every 1 second
    "clipboard_auto_hide_delay": 800,  # Auto-hide delay after copy
    "auto_paste_delay": 200,  # Delay before auto-paste for focus restoration
    # Display settings
    "max_chars_display": 100,  # Maximum characters to display before truncation
    # Environment cache (detected once and cached)
    "window_manager": None,  # Auto-detected: "x11" or "wayland"
    "paste_tool": None,  # Auto-detected: "xdotool", "wtype", or "ydotool"
}

# Keyboard shortcuts
SHORTCUT_HIDE = "Esc"
SHORTCUT_QUIT = "Ctrl+Esc"


# Feedback message types
class FeedbackType:
    """Feedback message types."""

    SUCCESS = "success"
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


# Theme names
class Theme:
    """Available theme names."""

    DARK = "dark"
    LIGHT = "light"
    SYSTEM = "system"
